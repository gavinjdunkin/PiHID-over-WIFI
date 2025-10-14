#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"

#define UDP_PORT 50037



static struct udp_pcb *udp_server;

static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                                 const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        printf("Received %d bytes from %s:%d\n", p->tot_len, ipaddr_ntoa(addr), port);

        

        pbuf_free(p);
    }
}

int main() {
    stdio_init_all();

    printf("Starting UDP server...\n");

    if (cyw43_arch_init()) {
        printf("Failed to initialize Wi-Fi!\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    // 🔑 Replace with your Wi-Fi credentials
    const char *ssid = "YOUR_WIFI_SSID";
    const char *password = "YOUR_WIFI_PASSWORD";

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
        // Let the background thread handle Wi-Fi and networking
        sleep_ms(100);
    }
}