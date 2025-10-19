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

    // Split the packet on ';'
    char *saveptr;
    char *cmd = strtok_r(msg, ";", &saveptr);
    while (cmd != NULL) {
        // Trim leading/trailing spaces if needed
        while (*cmd == ' ') cmd++;

        if (cmd[0] == 'K') {
            int code, value;
            if (sscanf(cmd, "K,%d,%d", &code, &value) == 2) {
                handle_key_event((uint8_t)code, value == 1);
            }
        } else if (cmd[0] == 'M') {
            int type, value;
            if (sscanf(cmd, "M,%d,%d", &type, &value) == 2) {
                switch (type) {
                    case 0: hid_send_mouse_move((int8_t)value, 0, 0); break;
                    case 1: hid_send_mouse_move(0, (int8_t)value, 0); break;
                    case 8: hid_send_mouse_move(0, 0, (int8_t)value); break;
                    case 272: hid_send_mouse_button(1, value == 1); break;
                    case 273: hid_send_mouse_button(2, value == 1); break;
                    case 274: hid_send_mouse_button(4, value == 1); break;
                }
            }
        }

        // Move to next command
        cmd = strtok_r(NULL, ";", &saveptr);
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
    init_key_table();

    while (true) {
        tud_task();
        // Process packet queue populated by UDP callbacks on core1
        Packet pkt;
        while (dequeue_packet(&pkt)) process_packet(&pkt);
        sleep_us(100);
    }
}