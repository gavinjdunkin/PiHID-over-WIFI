#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "hid_handler.h"
#include <stdio.h>
#include <string.h>

struct udp_pcb *udp_server;

bool wifi_init_done = false;

void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                       const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        // Null-terminate the received data
        char command[256];
        int len = p->len > 255 ? 255 : p->len;
        memcpy(command, p->payload, len);
        command[len] = '\0';
        
        printf("Received %d bytes: %s\n", p->len, command);
        
        // Parse and execute HID command
        hid_action_t action;
        if (hid_parse_command(command, &action)) {
            hid_process_action(&action);
            printf("Executed HID action for command: %s\n", command);
        } else {
            printf("Failed to parse HID command: %s\n", command);
        }
        
        pbuf_free(p);
    }
}

void init_wifi_and_udp_server(void) {
    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return;
    }
    
    cyw43_arch_enable_sta_mode();
    
    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms("the woods", "rector7task8was", CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Failed to connect to WiFi\n");
        return;
    }
    
    printf("WiFi connected!\n");
    wifi_init_done = true;
    
    udp_server = udp_new();
    udp_bind(udp_server, IP_ADDR_ANY, 50037);
    udp_recv(udp_server, udp_recv_callback, NULL);
    
    printf("UDP server started on port 50037\n");
}

void init_udp_server(void) {
    // Legacy function for compatibility
    init_wifi_and_udp_server();
}