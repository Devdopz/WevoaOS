#ifndef WEVOA_NET_H
#define WEVOA_NET_H

#include <stdint.h>

struct wevoa_http_page {
    char title[64];
    char line1[80];
    char line2[80];
    char line3[80];
};

int net_init(void);
int net_set_link(uint8_t up);
int net_get_link(uint8_t* out_up);

int net_dns_resolve(const char* host, uint8_t out_ip[4]);

int net_firewall_set_port(uint16_t port, uint8_t allow);
int net_firewall_get_port(uint16_t port, uint8_t* out_allow);

int net_http_get(const char* url, struct wevoa_http_page* out_page, uint8_t out_ip[4], uint16_t* out_port);

#endif
