#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/select.h>

#define MAX_PACKET_LEN       1024   // Max UDP payload
#define MAX_EVENTS_PER_BATCH 64     // Max events per batch
#define BATCH_SEND_TIMEOUT_US 5000  // Send after 5ms if not full

static void sleep_us_rel(unsigned int us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

static long diff_us_since(struct timespec *a, struct timespec *b) {
    return (a->tv_sec - b->tv_sec) * 1000000L + (a->tv_nsec - b->tv_nsec) / 1000L;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s /dev/input/eventX DEST_IP DEST_PORT\n", argv[0]);
        return 1;
    }

    const char *devpath = argv[1];
    const char *dest_ip = argv[2];
    int dest_port = atoi(argv[3]);

    int fd = open(devpath, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open input device");
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        close(fd);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dest_port);
    if (inet_pton(AF_INET, dest_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP: %s\n", dest_ip);
        close(fd);
        close(sock);
        return 1;
    }

    printf("Listening to %s → %s:%d\n", devpath, dest_ip, dest_port);

    struct input_event ev;
    char packet[MAX_PACKET_LEN];
    int packet_len = 0;
    int batch_events = 0;

    struct timespec last_send;
    clock_gettime(CLOCK_MONOTONIC, &last_send);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        struct timeval timeout = {0, 2000}; // Wait 2ms for events
        int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);

        if (ret < 0) {
            perror("select");
            break;
        }

        if (ret > 0 && FD_ISSET(fd, &readfds)) {
            // Read all available events in non-blocking mode
            while (1) {
                ssize_t r = read(fd, &ev, sizeof(ev));
                if (r == sizeof(ev)) {
                    char entry[64];
                    int n = 0;

                    if (ev.type == EV_REL || ev.code == 272 || ev.code == 273 || ev.code == 274) {
                        n = snprintf(entry, sizeof(entry), "M,%d,%d;", ev.code, ev.value);
                    } else if (ev.type == EV_KEY) {
                        n = snprintf(entry, sizeof(entry), "K,%d,%d;", ev.code, ev.value);
                    } else {
                        continue; // skip other event types
                    }

                    if (packet_len + n < MAX_PACKET_LEN) {
                        memcpy(packet + packet_len, entry, n);
                        packet_len += n;
                        batch_events++;
                    }

                    if (batch_events >= MAX_EVENTS_PER_BATCH) {
                        // Send immediately if full
                        ssize_t sent = sendto(sock, packet, packet_len, 0,
                                              (struct sockaddr*)&addr, sizeof(addr));
                        if (sent < 0)
                            perror("sendto");
                        packet_len = 0;
                        batch_events = 0;
                        clock_gettime(CLOCK_MONOTONIC, &last_send);
                    }
                } else if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    // No more events
                    break;
                } else {
                    perror("read");
                    goto cleanup;
                }
            }
        }

        // Check time since last send
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (packet_len > 0 && diff_us_since(&now, &last_send) >= BATCH_SEND_TIMEOUT_US) {
            // Send accumulated events after timeout
            ssize_t sent = sendto(sock, packet, packet_len, 0,
                                  (struct sockaddr*)&addr, sizeof(addr));
            if (sent < 0)
                perror("sendto");
            packet_len = 0;
            batch_events = 0;
            clock_gettime(CLOCK_MONOTONIC, &last_send);
        }
    }

cleanup:
    close(sock);
    close(fd);
    return 0;
}
