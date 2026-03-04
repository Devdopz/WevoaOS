#ifndef WEVOA_CONFIG_H
#define WEVOA_CONFIG_H

#include <stdint.h>

enum wevoa_config_key {
    WEVOA_CFG_BRIGHT = 0,
    WEVOA_CFG_WALL = 1,
    WEVOA_CFG_WIFI = 2,
    WEVOA_CFG_NET_LINK = 3,
    WEVOA_CFG_CURSOR_SPEED = 4,
    WEVOA_CFG_KEY_REPEAT_DELAY = 5,
    WEVOA_CFG_KEY_REPEAT_RATE = 6,
    WEVOA_CFG_LOCK_ENABLED = 7,
    WEVOA_CFG_LOCK_PIN = 8,
    WEVOA_CFG_SHELL_LAYOUT = 9,
    WEVOA_CFG_UI_BLUR_LEVEL = 10,
    WEVOA_CFG_UI_MOTION_LEVEL = 11,
    WEVOA_CFG_UI_EFFECTS_QUALITY = 12,
};

struct wevoa_config {
    uint8_t bright;
    uint8_t wall;
    uint8_t wifi_on;
    uint8_t net_link;
    uint8_t cursor_speed;
    uint16_t key_repeat_delay_ms;
    uint16_t key_repeat_rate_hz;
    uint8_t lock_enabled;
    uint16_t lock_pin;
    uint8_t shell_layout;
    uint8_t ui_blur_level;
    uint8_t ui_motion_level;
    uint8_t ui_effects_quality;
};

void config_reset_defaults(void);
const struct wevoa_config* config_snapshot(void);
int config_apply(const struct wevoa_config* cfg);
int config_get(enum wevoa_config_key key, uint32_t* out_value);
int config_set(enum wevoa_config_key key, uint32_t value);

#endif
