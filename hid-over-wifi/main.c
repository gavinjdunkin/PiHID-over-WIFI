#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "hid_server.h"
#include "tusb.h"
#include <string.h>
#include <stdio.h>

#define UDP_PORT 50037



static struct udp_pcb *udp_server;

static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                                 const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        printf("Received %d bytes from %s:%d\n", p->tot_len, ipaddr_ntoa(addr), port);
        
        // Extract the message from the pbuf
        char msg[256];
        size_t copy_len = (p->tot_len < sizeof(msg) - 1) ? p->tot_len : sizeof(msg) - 1;
        memcpy(msg, (char*)p->payload, copy_len);
        msg[copy_len] = '\0';
        
        // Remove trailing newline if present
        if (copy_len > 0 && msg[copy_len - 1] == '\n') {
            msg[copy_len - 1] = '\0';
        }
        
        printf("Received command: %s\n", msg);
        
        if (msg[0] == 'K') {
            int code, value;
            if (sscanf(msg, "K,%d,%d", &code, &value) == 2) {
                hid_send_key((uint8_t)code, value == 1);
                printf("Keyboard: code=%d, value=%d\n", code, value);
            }
        }
        // For mouse relative movement: "M,<type>,<value>"
        else if (msg[0] == 'M') {
            int type, value;
            if (sscanf(msg, "M,%d,%d", &type, &value) == 2) {
                printf("Mouse: type=%d, value=%d\n", type, value);
                switch (type) {
                    case 0:  // REL_X
                        hid_send_mouse_move((int8_t)value, 0, 0);
                        break;
                    case 1:  // REL_Y
                        hid_send_mouse_move(0, (int8_t)value, 0);
                        break;
                    case 8:  // REL_WHEEL
                        hid_send_mouse_move(0, 0, (int8_t)value);
                        break;
                    case 272: // Left click
                        hid_send_mouse_button(1, value == 1);
                        break;
                    case 273: // Right click
                        hid_send_mouse_button(2, value == 1);
                        break;
                    case 274: // Middle click
                        hid_send_mouse_button(4, value == 1);
                        break;
                }
            }
        }
        pbuf_free(p);
    }
}

int main() {
    stdio_init_all();

    printf("Starting HID-over-WiFi UDP server...\n");
    

    if (cyw43_arch_init()) {
        printf("Failed to initialize Wi-Fi!\n");
        return -1;
    }
    // Initialize TinyUSB
    tusb_init();

    cyw43_arch_enable_sta_mode();

    // 🔑 Replace with your Wi-Fi credentials
    const char *ssid = "the woods";
    const char *password = "rector7task8was";

    printf("Connecting to Wi-Fi...\n");
    int err = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 10000);
    if (err) {
        printf("Wi-Fi connection failed! Error %d\n", err);
        return -1;
    }

    printf("Connected to Wi-Fi!\n");
    printf("IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));

    // Create UDP control block
    udp_server = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!udp_server) {
        printf("Failed to create UDP PCB\n");
        return -1;
    }

    err_t bind_err = udp_bind(udp_server, IP_ANY_TYPE, UDP_PORT);
    if (bind_err != ERR_OK) {
        printf("UDP bind failed: %d\n", bind_err);
        return -1;
    }

    udp_recv(udp_server, udp_receive_callback, NULL);
    printf("UDP server listening on port %d\n", UDP_PORT);

    while (true) {
        hid_task();
        sleep_ms(5);
    }
}