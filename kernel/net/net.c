#include "wevoa/net.h"

#include "wevoa/string.h"
#include "wevoa/wstatus.h"

#include <stdint.h>

#define NET_FW_RULE_MAX 24u
#define NET_HOST_MAX 63u
#define NET_PATH_MAX 63u
#define NET_URL_BUF_MAX 127u

struct net_firewall_rule {
    uint16_t port;
    uint8_t allow;
    uint8_t used;
};

struct net_dns_record {
    const char* host;
    uint8_t ip[4];
    const char* title;
    const char* line1;
    const char* line2;
    const char* line3;
};

static uint8_t g_inited = 0u;
static uint8_t g_link_up = 0u;
static struct net_firewall_rule g_fw_rules[NET_FW_RULE_MAX];

static const struct net_dns_record g_dns_records[] = {
    {"DEVDOPZ.COM", {104u, 21u, 45u, 182u}, "DEVDOPZ.COM", "SITE PROFILE", "CUSTOM WEB PRESENCE", "HTTPS CONTENT PREVIEW MODE"},
    {"OPENAI.COM", {104u, 18u, 33u, 45u}, "OPENAI.COM", "AI PLATFORM", "MODEL + API + CHAT", "HTTPS CONTENT PREVIEW MODE"},
    {"EXAMPLE.COM", {93u, 184u, 216u, 34u}, "EXAMPLE.COM", "IANA DEMO SITE", "DOCUMENTATION EXAMPLE", "HTTPS CONTENT PREVIEW MODE"},
};

static char upper_char(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - ('a' - 'A'));
    }
    return c;
}

static void text_copy(char* dst, uint32_t cap, const char* src) {
    uint32_t i = 0u;
    if (dst == 0 || cap == 0u) {
        return;
    }
    if (src != 0) {
        while (i + 1u < cap && src[i] != '\0') {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static void text_upper_inplace(char* s) {
    if (s == 0) {
        return;
    }
    for (uint32_t i = 0u; s[i] != '\0'; ++i) {
        s[i] = upper_char(s[i]);
    }
}

static uint8_t text_equal(const char* a, const char* b) {
    if (a == 0 || b == 0) {
        return 0u;
    }
    uint32_t i = 0u;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0u;
        }
        i++;
    }
    return (a[i] == b[i]) ? 1u : 0u;
}

static uint8_t parse_u16(const char* s, uint16_t* out) {
    uint32_t v = 0u;
    if (s == 0 || out == 0 || s[0] == '\0') {
        return 0u;
    }
    for (uint32_t i = 0u; s[i] != '\0'; ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return 0u;
        }
        v = v * 10u + (uint32_t)(s[i] - '0');
        if (v > 65535u) {
            return 0u;
        }
    }
    *out = (uint16_t)v;
    return 1u;
}

static int parse_url(const char* url, char host[NET_HOST_MAX + 1u], char path[NET_PATH_MAX + 1u], uint16_t* out_port) {
    char tmp[NET_URL_BUF_MAX + 1u];
    uint32_t i = 0u;
    uint32_t h = 0u;
    uint32_t p = 0u;
    uint32_t pos = 0u;
    uint16_t port = 443u;

    if (url == 0 || host == 0 || path == 0 || out_port == 0) {
        return W_ERR_INVALID_ARG;
    }

    text_copy(tmp, sizeof(tmp), url);
    text_upper_inplace(tmp);
    if (tmp[0] == '\0') {
        return W_ERR_INVALID_ARG;
    }

    if (kstrncmp(tmp, "HTTPS://", 8u) == 0) {
        pos = 8u;
        port = 443u;
    } else if (kstrncmp(tmp, "HTTP://", 7u) == 0) {
        pos = 7u;
        port = 80u;
    }

    for (i = pos; tmp[i] != '\0' && tmp[i] != '/' && tmp[i] != ':'; ++i) {
        if (h < NET_HOST_MAX) {
            host[h++] = tmp[i];
        }
    }
    host[h] = '\0';
    if (h == 0u) {
        return W_ERR_INVALID_ARG;
    }

    if (tmp[i] == ':') {
        char port_buf[8];
        uint32_t pi = 0u;
        i++;
        while (tmp[i] != '\0' && tmp[i] != '/' && pi + 1u < sizeof(port_buf)) {
            port_buf[pi++] = tmp[i++];
        }
        port_buf[pi] = '\0';
        if (!parse_u16(port_buf, &port)) {
            return W_ERR_INVALID_ARG;
        }
    }

    if (tmp[i] == '/') {
        while (tmp[i] != '\0' && p < NET_PATH_MAX) {
            path[p++] = tmp[i++];
        }
    } else {
        path[p++] = '/';
    }
    path[p] = '\0';

    *out_port = port;
    return W_OK;
}

int net_init(void) {
    for (uint32_t i = 0u; i < NET_FW_RULE_MAX; ++i) {
        g_fw_rules[i].used = 0u;
        g_fw_rules[i].port = 0u;
        g_fw_rules[i].allow = 0u;
    }
    g_link_up = 0u;
    g_inited = 1u;
    return W_OK;
}

int net_set_link(uint8_t up) {
    if (!g_inited) {
        return W_ERR_UNSUPPORTED;
    }
    g_link_up = up ? 1u : 0u;
    return W_OK;
}

int net_get_link(uint8_t* out_up) {
    if (!g_inited || out_up == 0) {
        return W_ERR_INVALID_ARG;
    }
    *out_up = g_link_up;
    return W_OK;
}

int net_firewall_set_port(uint16_t port, uint8_t allow) {
    if (!g_inited || port == 0u) {
        return W_ERR_INVALID_ARG;
    }
    for (uint32_t i = 0u; i < NET_FW_RULE_MAX; ++i) {
        if (g_fw_rules[i].used && g_fw_rules[i].port == port) {
            g_fw_rules[i].allow = allow ? 1u : 0u;
            return W_OK;
        }
    }
    for (uint32_t i = 0u; i < NET_FW_RULE_MAX; ++i) {
        if (!g_fw_rules[i].used) {
            g_fw_rules[i].used = 1u;
            g_fw_rules[i].port = port;
            g_fw_rules[i].allow = allow ? 1u : 0u;
            return W_OK;
        }
    }
    return W_ERR_NO_SPACE;
}

int net_firewall_get_port(uint16_t port, uint8_t* out_allow) {
    if (!g_inited || port == 0u || out_allow == 0) {
        return W_ERR_INVALID_ARG;
    }
    for (uint32_t i = 0u; i < NET_FW_RULE_MAX; ++i) {
        if (g_fw_rules[i].used && g_fw_rules[i].port == port) {
            *out_allow = g_fw_rules[i].allow ? 1u : 0u;
            return W_OK;
        }
    }
    if (port == 80u || port == 443u) {
        *out_allow = 1u;
    } else {
        *out_allow = 0u;
    }
    return W_OK;
}

int net_dns_resolve(const char* host, uint8_t out_ip[4]) {
    char up_host[NET_HOST_MAX + 1u];
    if (!g_inited || host == 0 || out_ip == 0) {
        return W_ERR_INVALID_ARG;
    }
    text_copy(up_host, sizeof(up_host), host);
    text_upper_inplace(up_host);
    for (uint32_t i = 0u; i < sizeof(g_dns_records) / sizeof(g_dns_records[0]); ++i) {
        if (text_equal(up_host, g_dns_records[i].host)) {
            out_ip[0] = g_dns_records[i].ip[0];
            out_ip[1] = g_dns_records[i].ip[1];
            out_ip[2] = g_dns_records[i].ip[2];
            out_ip[3] = g_dns_records[i].ip[3];
            return W_OK;
        }
    }
    return W_ERR_NOT_FOUND;
}

int net_http_get(const char* url, struct wevoa_http_page* out_page, uint8_t out_ip[4], uint16_t* out_port) {
    char host[NET_HOST_MAX + 1u];
    char path[NET_PATH_MAX + 1u];
    uint8_t ip[4];
    uint8_t allow = 0u;
    uint16_t port = 0u;
    int rc;

    if (!g_inited) {
        return W_ERR_UNSUPPORTED;
    }
    if (url == 0 || out_page == 0) {
        return W_ERR_INVALID_ARG;
    }
    if (!g_link_up) {
        return W_ERR_IO;
    }

    rc = parse_url(url, host, path, &port);
    if (rc != W_OK) {
        return rc;
    }

    rc = net_firewall_get_port(port, &allow);
    if (rc != W_OK) {
        return rc;
    }
    if (!allow) {
        return W_ERR_UNSUPPORTED;
    }

    rc = net_dns_resolve(host, ip);
    if (rc != W_OK) {
        return rc;
    }

    if (out_ip != 0) {
        out_ip[0] = ip[0];
        out_ip[1] = ip[1];
        out_ip[2] = ip[2];
        out_ip[3] = ip[3];
    }
    if (out_port != 0) {
        *out_port = port;
    }

    text_copy(out_page->title, sizeof(out_page->title), "WEVOA WEB");
    text_copy(out_page->line1, sizeof(out_page->line1), "CONNECTED");
    text_copy(out_page->line2, sizeof(out_page->line2), "HTTP PREVIEW RESPONSE");
    text_copy(out_page->line3, sizeof(out_page->line3), "TLS ENGINE PENDING");

    for (uint32_t i = 0u; i < sizeof(g_dns_records) / sizeof(g_dns_records[0]); ++i) {
        if (text_equal(host, g_dns_records[i].host)) {
            text_copy(out_page->title, sizeof(out_page->title), g_dns_records[i].title);
            text_copy(out_page->line1, sizeof(out_page->line1), g_dns_records[i].line1);
            text_copy(out_page->line2, sizeof(out_page->line2), g_dns_records[i].line2);
            text_copy(out_page->line3, sizeof(out_page->line3), g_dns_records[i].line3);
            if (text_equal(g_dns_records[i].host, "DEVDOPZ.COM")) {
                if (text_equal(path, "/") || text_equal(path, "/HOME")) {
                    text_copy(out_page->line2, sizeof(out_page->line2), "HOME PAGE PREVIEW");
                } else {
                    text_copy(out_page->line2, sizeof(out_page->line2), "PATH PREVIEW");
                }
            }
            return W_OK;
        }
    }

    return W_ERR_NOT_FOUND;
}
