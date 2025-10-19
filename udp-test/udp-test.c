#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"

// Packet queue structure
#define QUEUE_SIZE 200
static struct packet_info {
    uint8_t data[64];  // Store packet data
    uint16_t length;
    bool valid;
} packet_queue[QUEUE_SIZE];

static volatile int queue_head = 0;  // Where to write next packet
static volatile int queue_tail = 0;  // Where to read next packet
static int packets_received = 0;
static int packets_processed = 0;
static int packets_dropped = 0;

static absolute_time_t start_time;
static ip_addr_t sender_addr;
static u16_t sender_port;
static bool has_sender = false;

struct udp_pcb *udp_server;
struct udp_pcb *udp_client;

static void udp_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                              const ip_addr_t *addr, u16_t port)
{
    packets_received++;
    
    // Store sender information for the first packet
    if (!has_sender) {
        ip_addr_copy(sender_addr, *addr);
        sender_port = port;
        has_sender = true;
        printf("Sender detected: %s:%d\n", ip4addr_ntoa(addr), port);
    }
    
    // Calculate next queue position
    int next_head = (queue_head + 1) % QUEUE_SIZE;
    
    // Check if queue is full
    if (next_head == queue_tail) {
        packets_dropped++;
        printf("Queue full! Dropped packet %d\n", packets_dropped);
        pbuf_free(p);
        return;
    }
    
    // Queue the packet data
    if (p->len <= 64) {
        packet_queue[queue_head].length = p->len;
        memcpy(packet_queue[queue_head].data, p->payload, p->len);
        packet_queue[queue_head].valid = true;
        queue_head = next_head;
    } else {
        packets_dropped++;
        printf("Packet too large (%d bytes), dropped\n", p->len);
    }
    
    pbuf_free(p);
}

int main()
{
    stdio_init_all();

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // Enable wifi station
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms("the woods", "rector7task8was", CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
        // Read the ip address in a human readable way
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
        
        udp_server = udp_new_ip_type(IPADDR_TYPE_ANY);
        udp_bind(udp_server, IP_ANY_TYPE, 50037);
        udp_recv(udp_server, udp_recv_callback, NULL);
        
        // Create UDP client for sending responses
        udp_client = udp_new_ip_type(IPADDR_TYPE_ANY);
        
        // Record the start time
        start_time = get_absolute_time();
    }

    while (true) {
        cyw43_arch_poll();
        
        // Process one packet from queue if available
        if (queue_tail != queue_head) {
            struct packet_info *packet = &packet_queue[queue_tail];
            if (packet->valid) {
                // Simulate processing time (you can adjust this)
                sleep_us(100); // 100 microseconds processing delay
                
                // Here you would process the packet into HID commands
                // For now, just mark it as processed
                packets_processed++;
                packet->valid = false;
            }
            queue_tail = (queue_tail + 1) % QUEUE_SIZE;
        }
        
        // Check if one minute has passed
        if (absolute_time_diff_us(start_time, get_absolute_time()) >= 60000000) {
            printf("=== 1 Minute Statistics ===\n");
            printf("Packets received: %d\n", packets_received);
            printf("Packets processed: %d\n", packets_processed);
            printf("Packets dropped: %d\n", packets_dropped);
            printf("Queue utilization: %d/%d\n", 
                   (queue_head - queue_tail + QUEUE_SIZE) % QUEUE_SIZE, QUEUE_SIZE);
            
            // Send response back to sender if we have one
            if (has_sender) {
                char response[128];
                sprintf(response, "Received:%d Processed:%d Dropped:%d", 
                       packets_received, packets_processed, packets_dropped);
                
                struct pbuf *response_buf = pbuf_alloc(PBUF_TRANSPORT, strlen(response), PBUF_RAM);
                if (response_buf) {
                    memcpy(response_buf->payload, response, strlen(response));
                    err_t err = udp_sendto(udp_client, response_buf, &sender_addr, sender_port);
                    if (err == ERR_OK) {
                        printf("Response sent to %s:%d\n", ip4addr_ntoa(&sender_addr), sender_port);
                    } else {
                        printf("Failed to send response, error: %d\n", err);
                    }
                    pbuf_free(response_buf);
                }
            }
            
            // Reset counters and timer for next minute
            packets_received = 0;
            packets_processed = 0;
            packets_dropped = 0;
            start_time = get_absolute_time();
        }
    }
}
