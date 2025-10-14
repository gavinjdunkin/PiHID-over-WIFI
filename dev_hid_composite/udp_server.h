#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <stdbool.h>

void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                       const ip_addr_t *addr, u16_t port);
void init_udp_server(void);
void init_wifi_and_udp_server(void);

extern bool wifi_init_done;

#endif