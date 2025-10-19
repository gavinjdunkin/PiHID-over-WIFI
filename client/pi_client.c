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

#define MAX_PACKET_LEN        1024
#define MAX_EVENTS_PER_BATCH  64
#define BATCH_SEND_TIMEOUT_US 5000
#define MAX_INPUT_DEVS        8  // up to 8 devices

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
    if (argc < 5) {
        fprintf(stderr, "Usage: %s DEST_IP DEST_PORT /dev/input/eventX [/dev/input/eventY ...]\n", argv[0]);
        return 1;
    }

    const char *dest_ip = argv[1];
    int dest_port = atoi(argv[2]);
    int dev_count = argc - 3;
    if (dev_count > MAX_INPUT_DEVS) {
        fprintf(stderr, "Too many input devices (max %d)\n", MAX_INPUT_DEVS);
        return 1;
    }

    int fds[MAX_INPUT_DEVS];
    for (int i = 0; i < dev_count; i++) {
        const char *devpath = argv[i + 3];
        fds[i] = open(devpath, O_RDONLY | O_NONBLOCK);
        if (fds[i] < 0) {
            perror(devpath);
            return 1;
        }
        printf("Opened: %s (fd=%d)\n", devpath, fds[i]);
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dest_port);
    if (inet_pton(AF_INET, dest_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP: %s\n", dest_ip);
        return 1;
    }

    printf("Sending to %s:%d\n", dest_ip, dest_port);
    fflush(stdout);

    struct input_event ev;
    char packet[MAX_PACKET_LEN];
    int packet_len = 0, batch_events = 0, total_packets_sent = 0;

    struct timespec last_send;
    clock_gettime(CLOCK_MONOTONIC, &last_send);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = -1;
        for (int i = 0; i < dev_count; i++) {
            FD_SET(fds[i], &readfds);
            if (fds[i] > max_fd) max_fd = fds[i];
        }

        struct timeval timeout = {0, 2000};
        int ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret < 0) {
            perror("select");
            break;
        }

        for (int i = 0; i < dev_count; i++) {
            if (FD_ISSET(fds[i], &readfds)) {
                while (1) {
                    ssize_t r = read(fds[i], &ev, sizeof(ev));
                    if (r == sizeof(ev)) {
                        char entry[64];
                        int n = 0;

                        if (ev.type == EV_REL || ev.code == 272 || ev.code == 273 || ev.code == 274) {
                            n = snprintf(entry, sizeof(entry), "M,%d,%d;", ev.code, ev.value);
                        } else if (ev.type == EV_KEY && ev.value < 2) {
                            n = snprintf(entry, sizeof(entry), "K,%d,%d;", ev.code, ev.value);
                        } else continue;

                        if (packet_len + n < MAX_PACKET_LEN) {
                            memcpy(packet + packet_len, entry, n);
                            packet_len += n;
                            batch_events++;
                        }

                        if (batch_events >= MAX_EVENTS_PER_BATCH) {
                            sendto(sock, packet, packet_len, 0,
                                   (struct sockaddr*)&addr, sizeof(addr));
                            packet_len = 0;
                            batch_events = 0;
                            clock_gettime(CLOCK_MONOTONIC, &last_send);
                            total_packets_sent++;
                        }
                    } else if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        break;
                    } else {
                        perror("read");
                        goto cleanup;
                    }
                }
            }
        }

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_us = diff_us_since(&now, &last_send);
        if (packet_len > 0 && elapsed_us >= BATCH_SEND_TIMEOUT_US) {
            sendto(sock, packet, packet_len, 0, (struct sockaddr*)&addr, sizeof(addr));
            packet_len = 0;
            batch_events = 0;
            clock_gettime(CLOCK_MONOTONIC, &last_send);
            total_packets_sent++;
        }
    }

cleanup:
    printf("\nTotal packets sent: %d\n", total_packets_sent);
    for (int i = 0; i < dev_count; i++) close(fds[i]);
    close(sock);
    return 0;
}
