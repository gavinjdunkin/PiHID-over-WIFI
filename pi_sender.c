// pi_sender.c
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
    if (sock < 0) { perror("socket"); close(fd); return 1; }

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
    char buf[64];
    while (1) {
        ssize_t r = read(fd, &ev, sizeof(ev));
        if (r != sizeof(ev)) {
            if (r < 0) perror("read");
            else fprintf(stderr, "short read\n");
            break;
        }

        // Only send key and relative mouse events (filter others)
        if (ev.type == EV_KEY) {
            // format: K,<code>,<value>\n   (value: 1=press, 0=release, 2=repeat)
            int n = snprintf(buf, sizeof(buf), "K,%d,%d\n", ev.code, ev.value);
            sendto(sock, buf, n, 0, (struct sockaddr*)&addr, sizeof(addr));
        } else if (ev.type == EV_REL) {
            // format: M,<rel_type>,<value>\n  (rel_type: 0=X,1=Y,8=wheel etc)
            int n = snprintf(buf, sizeof(buf), "M,%d,%d\n", ev.code, ev.value);
            sendto(sock, buf, n, 0, (struct sockaddr*)&addr, sizeof(addr));
        }
        // Ignore other events for now
    }

    close(sock);
    close(fd);
    return 0;
}
