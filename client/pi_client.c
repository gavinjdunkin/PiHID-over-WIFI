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

#define BATCH_SIZE 32         // number of events to batch before sending
#define MAX_PACKET_LEN 1024   // max UDP payload size
#define SEND_INTERVAL_US 2000 // send at most every 2ms (500Hz)

static void sleep_us_rel(unsigned int us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s /dev/input/eventX DEST_IP DEST_PORT\n", argv[0]);
        return 1;
    }

    const char *devpath = argv[1];
    const char *dest_ip = argv[2];
    int dest_port = atoi(argv[3]);

    int fd = open(devpath, O_RDONLY);
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

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dest_port);
    if (inet_pton(AF_INET, dest_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid dest IP\n");
        close(fd);
        close(sock);
        return 1;
    }

    struct input_event ev;
    char packet[MAX_PACKET_LEN];
    int packet_len = 0;
    int event_count = 0;

    struct timespec last_send;
    clock_gettime(CLOCK_MONOTONIC, &last_send);

    while (1) {
        ssize_t r = read(fd, &ev, sizeof(ev));
        if (r != sizeof(ev)) {
            if (r < 0) perror("read");
            else fprintf(stderr, "short read\n");
            break;
        }

        // filter interesting events
        if (!(ev.type == EV_KEY || ev.type == EV_REL)) continue;

        char entry[64];
        int n = 0;

        if (ev.type == EV_REL || ev.code == 272 || ev.code == 273 || ev.code == 274) {
            n = snprintf(entry, sizeof(entry), "M,%d,%d;", ev.code, ev.value);
        } else if (ev.type == EV_KEY) {
            n = snprintf(entry, sizeof(entry), "K,%d,%d;", ev.code, ev.value);
        }

        if (packet_len + n < MAX_PACKET_LEN) {
            memcpy(packet + packet_len, entry, n);
            packet_len += n;
            event_count++;
        }

        // time since last send
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long diff_us = (now.tv_sec - last_send.tv_sec) * 1000000L +
                       (now.tv_nsec - last_send.tv_nsec) / 1000L;

        // send if batch full or interval elapsed
        if (event_count >= BATCH_SIZE || diff_us >= SEND_INTERVAL_US) {
            if (packet_len > 0) {
                sendto(sock, packet, packet_len, 0, (struct sockaddr*)&addr, sizeof(addr));
                packet_len = 0;
                event_count = 0;
            }
            last_send = now;
            sleep_us_rel(SEND_INTERVAL_US); // pacing
        }
    }

    close(sock);
    close(fd);
    return 0;
}
