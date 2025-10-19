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

#define BATCH_SIZE 32         
#define MAX_PACKET_LEN 1024   
#define SEND_INTERVAL_US 2000 

static void sleep_us_rel(unsigned int us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

int main(int argc, char **argv) {
    printf("pi_client starting with %d arguments\n", argc);
    fflush(stdout);
    
    if (argc < 4) {
        fprintf(stderr, "ERROR: Not enough arguments (got %d, need 4)\n", argc);
        fprintf(stderr, "Usage: %s /dev/input/eventX DEST_IP DEST_PORT\n", argv[0]);
        return 1;
    }

    const char *devpath = argv[1];
    const char *dest_ip = argv[2];
    int dest_port = atoi(argv[3]);
    
    printf("Arguments: device=%s, ip=%s, port=%d\n", devpath, dest_ip, dest_port);
    fflush(stdout);

    printf("Opening device: %s\n", devpath);
    fflush(stdout);
    
    int fd = open(devpath, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Failed to open device %s: %s\n", devpath, strerror(errno));
        perror("open input device");
        return 1;
    }
    
    // ADD THIS: Set non-blocking mode for batched reading
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        fprintf(stderr, "ERROR: Failed to get file flags: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        fprintf(stderr, "ERROR: Failed to set non-blocking mode: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("Device opened successfully (fd=%d) in non-blocking mode\n", fd);
    fflush(stdout);

    printf("Creating socket\n");
    fflush(stdout);
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        fprintf(stderr, "ERROR: Failed to create socket: %s\n", strerror(errno));
        perror("socket");
        close(fd);
        return 1;
    }
    
    printf("Socket created successfully (sock=%d)\n", sock);
    fflush(stdout);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dest_port);
    
    printf("Converting IP address: %s\n", dest_ip);
    fflush(stdout);
    
    if (inet_pton(AF_INET, dest_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: Invalid dest IP: %s\n", dest_ip);
        close(fd);
        close(sock);
        return 1;
    }
    
    printf("IP address converted successfully\n");
    fflush(stdout);

    struct input_event ev;
    char packet[MAX_PACKET_LEN];
    int packet_len = 0;
    int event_count = 0;

    struct timespec last_send;
    clock_gettime(CLOCK_MONOTONIC, &last_send);

    printf("Starting main event loop with select()...\n");
    printf("Move mouse or press keys to generate events...\n");
    fflush(stdout);

    int total_events = 0;
    int loop_count = 0;
    
    while (1) {
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        // Wait up to 2ms for events (match SEND_INTERVAL_US)
        timeout.tv_sec = 0;
        timeout.tv_usec = 2000;  // 2ms
        
        int select_result = select(fd + 1, &readfds, NULL, NULL, &timeout);
        
        loop_count++;
        if (loop_count % 1000 == 0) {
            printf("Loop %d: select_result=%d, total_events=%d\n", loop_count, select_result, total_events);
            fflush(stdout);
        }
        
        if (select_result > 0 && FD_ISSET(fd, &readfds)) {
            // Events available - read as many as possible
            int events_this_batch = 0;
            
            while (events_this_batch < BATCH_SIZE) {
                ssize_t r = read(fd, &ev, sizeof(ev));
                
                if (r == sizeof(ev)) {
                    total_events++;
                    events_this_batch++;
                    
                    if (total_events <= 10) {
                        printf("Event %d: type=%d, code=%d, value=%d\n", 
                               total_events, ev.type, ev.code, ev.value);
                        fflush(stdout);
                    }
                    
                    // Process event
                    if (ev.type == EV_KEY || ev.type == EV_REL) {
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
                    }
                } else if (r < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // No more events available right now - this is normal
                        break;
                    } else {
                        fprintf(stderr, "Read error: %s\n", strerror(errno));
                        goto exit_loop;
                    }
                } else {
                    fprintf(stderr, "Short read: got %zd bytes\n", r);
                    goto exit_loop;
                }
            }
            
            if (events_this_batch > 0) {
                printf("Read %d events in this batch\n", events_this_batch);
                fflush(stdout);
            }
        } else if (select_result == 0) {
            // Timeout - this is normal, no events to read
        } else {
            // Select error
            fprintf(stderr, "Select error: %s\n", strerror(errno));
            goto exit_loop;
        }

        // Check if we should send
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long diff_us = (now.tv_sec - last_send.tv_sec) * 1000000L +
                       (now.tv_nsec - last_send.tv_nsec) / 1000L;

        if (event_count >= BATCH_SIZE || (packet_len > 0 && diff_us >= SEND_INTERVAL_US)) {
            if (packet_len > 0) {
                printf("Sending packet with %d events\n", event_count);
                fflush(stdout);
                
                ssize_t sent = sendto(sock, packet, packet_len, 0, 
                                    (struct sockaddr*)&addr, sizeof(addr));
                if (sent < 0) {
                    fprintf(stderr, "ERROR: sendto failed: %s\n", strerror(errno));
                } else {
                    printf("Sent %zd bytes successfully\n", sent);
                    fflush(stdout);
                }
                
                packet_len = 0;
                event_count = 0;
            }
            last_send = now;
        }
    }

exit_loop:
    printf("pi_client exiting\n");
    close(sock);
    close(fd);
    return 0;
}
