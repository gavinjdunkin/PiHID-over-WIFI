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
    printf("Press keys or move mouse to generate events...\n");
    fflush(stdout);

    struct input_event ev;
    char packet[MAX_PACKET_LEN];
    int packet_len = 0;
    int batch_events = 0;
    int total_packets_sent = 0;

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
            int events_this_loop = 0;
            while (1) {
                ssize_t r = read(fd, &ev, sizeof(ev));
                if (r == sizeof(ev)) {
                    events_this_loop++;
                    char entry[64];
                    int n = 0;

                    if (ev.type == EV_REL || ev.code == 272 || ev.code == 273 || ev.code == 274) {
                        n = snprintf(entry, sizeof(entry), "M,%d,%d;", ev.code, ev.value);
                        printf("Mouse event: code=%d, value=%d → %s", ev.code, ev.value, entry);
                    } else if (ev.type == EV_KEY) {
                        n = snprintf(entry, sizeof(entry), "K,%d,%d;", ev.code, ev.value);
                        printf("Key event: code=%d, value=%d → %s", ev.code, ev.value, entry);
                    } else {
                        printf("Skipped event: type=%d, code=%d, value=%d\n", ev.type, ev.code, ev.value);
                        continue; // skip other event types
                    }

                    if (packet_len + n < MAX_PACKET_LEN) {
                        memcpy(packet + packet_len, entry, n);
                        packet_len += n;
                        batch_events++;
                        printf("Added to batch (events: %d, packet_len: %d)\n", batch_events, packet_len);
                    } else {
                        printf("WARNING: Packet buffer full, dropping event\n");
                    }

                    if (batch_events >= MAX_EVENTS_PER_BATCH) {
                        // Send immediately if full
                        total_packets_sent++;
                        printf("\n=== SENDING PACKET #%d (BATCH FULL) ===\n", total_packets_sent);
                        printf("Events in batch: %d\n", batch_events);
                        printf("Packet length: %d bytes\n", packet_len);
                        printf("Packet content: %.*s\n", packet_len, packet);
                        printf("========================================\n\n");
                        
                        ssize_t sent = sendto(sock, packet, packet_len, 0,
                                              (struct sockaddr*)&addr, sizeof(addr));
                        if (sent < 0) {
                            perror("sendto");
                        } else {
                            printf("Successfully sent %zd bytes\n", sent);
                        }
                        
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
            
            if (events_this_loop > 0) {
                printf("Read %d events in this loop\n", events_this_loop);
                fflush(stdout);
            }
        }

        // Check time since last send
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_us = diff_us_since(&now, &last_send);
        
        if (packet_len > 0 && elapsed_us >= BATCH_SEND_TIMEOUT_US) {
            // Send accumulated events after timeout
            total_packets_sent++;
            printf("\n=== SENDING PACKET #%d (TIMEOUT) ===\n", total_packets_sent);
            printf("Events in batch: %d\n", batch_events);
            printf("Packet length: %d bytes\n", packet_len);
            printf("Timeout elapsed: %ld μs\n", elapsed_us);
            printf("Packet content: %.*s\n", packet_len, packet);
            printf("===================================\n\n");
            
            ssize_t sent = sendto(sock, packet, packet_len, 0,
                                  (struct sockaddr*)&addr, sizeof(addr));
            if (sent < 0) {
                perror("sendto");
            } else {
                printf("Successfully sent %zd bytes\n", sent);
            }
            
            packet_len = 0;
            batch_events = 0;
            clock_gettime(CLOCK_MONOTONIC, &last_send);
        }
        
        fflush(stdout);
    }

cleanup:
    printf("\nTotal packets sent: %d\n", total_packets_sent);
    close(sock);
    close(fd);
    return 0;
}
