#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "hardware/gpio.h"
#include "tusb.h"
#include "hid_server.h"
#include <stdio.h>
#include <string.h>

#define UDP_PORT 50037
#define PACKET_BUF_SIZE 256
#define PACKET_QUEUE_SIZE 64

// For Pico 2 W, LED is controlled by CYW43 chip, not GPIO

// Shared data between cores
static volatile int udp_packet_count = 0;
static volatile int processed_packet_count = 0;
static volatile bool core1_ready = false;

typedef struct {
    uint16_t len;
    char data[PACKET_BUF_SIZE];
} Packet;

static Packet packet_queue[PACKET_QUEUE_SIZE];
static volatile int packet_head = 0, packet_tail = 0;

static struct udp_pcb *udp_server;

// ───────────────────────────────
// Utility: ring buffer
// ───────────────────────────────
static bool enqueue_packet(const char *data, uint16_t len) {
    int next = (packet_head + 1) % PACKET_QUEUE_SIZE;
    if (next == packet_tail) return false; // full, drop
    if (len > PACKET_BUF_SIZE) len = PACKET_BUF_SIZE;
    memcpy(packet_queue[packet_head].data, data, len);
    packet_queue[packet_head].len = len;
    packet_head = next;
    return true;
}

static bool dequeue_packet(Packet *pkt) {
    if (packet_tail == packet_head) return false;
    *pkt = packet_queue[packet_tail];
    packet_tail = (packet_tail + 1) % PACKET_QUEUE_SIZE;
    return true;
}

// ───────────────────────────────
// LwIP UDP receive callback
// ───────────────────────────────
static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                                 const ip_addr_t *addr, u16_t port) {
    if (!p) return;
    if (p->tot_len > 0 && p->payload) {
        bool queued = enqueue_packet((char *)p->payload, p->tot_len);
    }
    pbuf_free(p); // must free immediately
}

static void process_packet(const Packet *pkt) {
    processed_packet_count++;

    char msg[PACKET_BUF_SIZE + 1];
    memcpy(msg, pkt->data, pkt->len);
    msg[pkt->len] = '\0';

    // New packet format: "HID,<modifiers>,<key1>,<key2>,...<key6>,M,<delta_x>,<delta_y>,<wheel>,<buttons>"
    if (strncmp(msg, "HID,", 4) == 0) {
        char *saveptr;
        char *token = strtok_r(msg, ",", &saveptr);
        
        if (token == NULL || strcmp(token, "HID") != 0) return;
        
        // Parse modifiers
        token = strtok_r(NULL, ",", &saveptr);
        if (token == NULL) return;
        uint8_t modifiers = (uint8_t)atoi(token);
        
        // Parse 6 key slots
        uint8_t keys[6] = {0};
        for (int i = 0; i < 6; i++) {
            token = strtok_r(NULL, ",", &saveptr);
            if (token == NULL) return;
            keys[i] = (uint8_t)atoi(token);
        }
        
        // Parse "M" marker
        token = strtok_r(NULL, ",", &saveptr);
        if (token == NULL || strcmp(token, "M") != 0) return;
        
        // Parse mouse data
        token = strtok_r(NULL, ",", &saveptr);
        if (token == NULL) return;
        int8_t delta_x = (int8_t)atoi(token);
        
        token = strtok_r(NULL, ",", &saveptr);
        if (token == NULL) return;
        int8_t delta_y = (int8_t)atoi(token);
        
        token = strtok_r(NULL, ",", &saveptr);
        if (token == NULL) return;
        int8_t wheel = (int8_t)atoi(token);
        
        token = strtok_r(NULL, ",", &saveptr);
        if (token == NULL) return;
        uint8_t buttons = (uint8_t)atoi(token);
        
        // Apply complete HID state
        hid_set_complete_state(modifiers, keys, delta_x, delta_y, wheel, buttons);
        return;
    }
}


void core1_entry() {
    // Wi-Fi + UDP server here
    if (cyw43_arch_init()) return;
    cyw43_arch_enable_sta_mode();
    cyw43_arch_wifi_connect_timeout_ms("the woods", "rector7task8was", CYW43_AUTH_WPA2_AES_PSK, 30000);
    udp_server = udp_new_ip_type(IPADDR_TYPE_ANY);
    udp_bind(udp_server, IP_ANY_TYPE, UDP_PORT);
    udp_recv(udp_server, udp_receive_callback, NULL);
    while (true) {
        cyw43_arch_poll();
        sleep_us(1000);
    }
}

int main() {
    stdio_init_all();
    multicore_launch_core1(core1_entry);

    tusb_init();

    while (true) {
        tud_task();
        // Process packet queue populated by UDP callbacks on core1
        Packet pkt;
        while (dequeue_packet(&pkt)) process_packet(&pkt);
        sleep_us(100);
    }
}