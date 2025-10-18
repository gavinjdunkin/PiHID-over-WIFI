#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Common settings used in most of the pico_w examples
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html for details)

// allow override in some examples
#ifndef NO_SYS
#define NO_SYS                      1
#endif
// allow override in some examples
#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 0
#endif
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1
#else
// MEM_LIBC_MALLOC is incompatible with non polling versions
#define MEM_LIBC_MALLOC             0
#endif
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETCONN                0
#define MEM_STATS                   0
#define SYS_STATS                   0
#define MEMP_STATS                  0
#define LINK_STATS                  0
// #define ETH_PAD_SIZE                2
#define IP_FORWARD                  0
#define IP_OPTIONS_ALLOWED          0
#define IP_REASSEMBLY               0
#define IP_FRAG                     0
#define ARP_QUEUEING                0
#define UDP_TTL                     255
#define TCP_TTL                     255
#define TCP_KEEPALIVE               1
#define TCP_KEEPIDLE                7200000UL
#define TCP_KEEPINTVL               75000UL
#define TCP_KEEPCNT                 9
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_DHCP                   1
#define LWIP_AUTOIP                 1
#define LWIP_DNS                    1
#define LWIP_SO_RCVTIMEO            1
#define LWIP_SO_SNDTIMEO            1
#define LWIP_SO_RCVBUF              1
#define LWIP_SO_REUSEADDR           1
#define SO_REUSE                    1

// UDP
#define LWIP_UDP                    1
#define LWIP_UDPLITE                0
#define UDP_TTL                     255

#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#endif

#define LWIP_DBG_MIN_LEVEL          LWIP_DBG_LEVEL_ALL
#define LWIP_DBG_TYPES_ON           LWIP_DBG_ON
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define IGMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TIMERS_DEBUG                LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF
#define AUTOIP_DEBUG                LWIP_DBG_OFF
#define DNS_DEBUG                   LWIP_DBG_OFF
#define IP6_DEBUG                   LWIP_DBG_OFF

#define LWIP_IPV6                   0

#undef TCP_WND
#define TCP_WND  16384

#endif /* __LWIPOPTS_H__ */