#include "wevoa/config.h"

#include "wevoa/wstatus.h"

static struct wevoa_config g_cfg;

void config_reset_defaults(void) {
    g_cfg.bright = 92u;
    g_cfg.wall = 2u;
    g_cfg.wifi_on = 1u;
    g_cfg.net_link = 1u;
    g_cfg.cursor_speed = 160u; /* 1600 DPI-like feel */
    g_cfg.key_repeat_delay_ms = 300u;
    g_cfg.key_repeat_rate_hz = 25u;
    g_cfg.lock_enabled = 0u;
    g_cfg.lock_pin = 1234u;
    g_cfg.shell_layout = 0u;       /* 0=dock+topbar, 1=taskbar */
    g_cfg.ui_blur_level = 2u;      /* 0..3 */
    g_cfg.ui_motion_level = 2u;    /* 0..3 */
    g_cfg.ui_effects_quality = 2u; /* 0..3 */
}

const struct wevoa_config* config_snapshot(void) {
    return &g_cfg;
}

int config_apply(const struct wevoa_config* cfg) {
    if (cfg == 0) {
        return W_ERR_INVALID_ARG;
    }
    g_cfg = *cfg;
    if (g_cfg.bright < 10u) {
        g_cfg.bright = 10u;
    }
    if (g_cfg.bright > 100u) {
        g_cfg.bright = 100u;
    }
    g_cfg.wall = (uint8_t)(g_cfg.wall % 3u);
    g_cfg.wifi_on = g_cfg.wifi_on ? 1u : 0u;
    g_cfg.net_link = (g_cfg.wifi_on && g_cfg.net_link) ? 1u : 0u;
    if (g_cfg.cursor_speed < 40u) {
        g_cfg.cursor_speed = 40u;
    }
    if (g_cfg.cursor_speed > 240u) {
        g_cfg.cursor_speed = 240u;
    }
    if (g_cfg.key_repeat_delay_ms < 100u) {
        g_cfg.key_repeat_delay_ms = 100u;
    }
    if (g_cfg.key_repeat_delay_ms > 1000u) {
        g_cfg.key_repeat_delay_ms = 1000u;
    }
    if (g_cfg.key_repeat_rate_hz < 5u) {
        g_cfg.key_repeat_rate_hz = 5u;
    }
    if (g_cfg.key_repeat_rate_hz > 60u) {
        g_cfg.key_repeat_rate_hz = 60u;
    }
    g_cfg.lock_enabled = g_cfg.lock_enabled ? 1u : 0u;
    if (g_cfg.lock_pin > 9999u) {
        g_cfg.lock_pin = 1234u;
    }
    if (g_cfg.shell_layout > 1u) {
        g_cfg.shell_layout = 0u;
    }
    if (g_cfg.ui_blur_level > 3u) {
        g_cfg.ui_blur_level = 3u;
    }
    if (g_cfg.ui_motion_level > 3u) {
        g_cfg.ui_motion_level = 3u;
    }
    if (g_cfg.ui_effects_quality > 3u) {
        g_cfg.ui_effects_quality = 3u;
    }
    return W_OK;
}

int config_get(enum wevoa_config_key key, uint32_t* out_value) {
    if (out_value == 0) {
        return W_ERR_INVALID_ARG;
    }

    switch (key) {
        case WEVOA_CFG_BRIGHT:
            *out_value = g_cfg.bright;
            return W_OK;
        case WEVOA_CFG_WALL:
            *out_value = g_cfg.wall;
            return W_OK;
        case WEVOA_CFG_WIFI:
            *out_value = g_cfg.wifi_on;
            return W_OK;
        case WEVOA_CFG_NET_LINK:
            *out_value = g_cfg.net_link;
            return W_OK;
        case WEVOA_CFG_CURSOR_SPEED:
            *out_value = g_cfg.cursor_speed;
            return W_OK;
        case WEVOA_CFG_KEY_REPEAT_DELAY:
            *out_value = g_cfg.key_repeat_delay_ms;
            return W_OK;
        case WEVOA_CFG_KEY_REPEAT_RATE:
            *out_value = g_cfg.key_repeat_rate_hz;
            return W_OK;
        case WEVOA_CFG_LOCK_ENABLED:
            *out_value = g_cfg.lock_enabled;
            return W_OK;
        case WEVOA_CFG_LOCK_PIN:
            *out_value = g_cfg.lock_pin;
            return W_OK;
        case WEVOA_CFG_SHELL_LAYOUT:
            *out_value = g_cfg.shell_layout;
            return W_OK;
        case WEVOA_CFG_UI_BLUR_LEVEL:
            *out_value = g_cfg.ui_blur_level;
            return W_OK;
        case WEVOA_CFG_UI_MOTION_LEVEL:
            *out_value = g_cfg.ui_motion_level;
            return W_OK;
        case WEVOA_CFG_UI_EFFECTS_QUALITY:
            *out_value = g_cfg.ui_effects_quality;
            return W_OK;
        default:
            return W_ERR_INVALID_ARG;
    }
}

int config_set(enum wevoa_config_key key, uint32_t value) {
    switch (key) {
        case WEVOA_CFG_BRIGHT:
            if (value < 10u || value > 100u) {
                return W_ERR_INVALID_ARG;
            }
            g_cfg.bright = (uint8_t)value;
            return W_OK;
        case WEVOA_CFG_WALL:
            g_cfg.wall = (uint8_t)(value % 3u);
            return W_OK;
        case WEVOA_CFG_WIFI:
            g_cfg.wifi_on = value ? 1u : 0u;
            if (!g_cfg.wifi_on) {
                g_cfg.net_link = 0u;
            }
            return W_OK;
        case WEVOA_CFG_NET_LINK:
            if (!g_cfg.wifi_on) {
                g_cfg.net_link = 0u;
                return value ? W_ERR_UNSUPPORTED : W_OK;
            }
            g_cfg.net_link = value ? 1u : 0u;
            return W_OK;
        case WEVOA_CFG_CURSOR_SPEED:
            if (value < 40u || value > 240u) {
                return W_ERR_INVALID_ARG;
            }
            g_cfg.cursor_speed = (uint8_t)value;
            return W_OK;
        case WEVOA_CFG_KEY_REPEAT_DELAY:
            if (value < 100u || value > 1000u) {
                return W_ERR_INVALID_ARG;
            }
            g_cfg.key_repeat_delay_ms = (uint16_t)value;
            return W_OK;
        case WEVOA_CFG_KEY_REPEAT_RATE:
            if (value < 5u || value > 60u) {
                return W_ERR_INVALID_ARG;
            }
            g_cfg.key_repeat_rate_hz = (uint16_t)value;
            return W_OK;
        case WEVOA_CFG_LOCK_ENABLED:
            g_cfg.lock_enabled = value ? 1u : 0u;
            return W_OK;
        case WEVOA_CFG_LOCK_PIN:
            if (value > 9999u) {
                return W_ERR_INVALID_ARG;
            }
            g_cfg.lock_pin = (uint16_t)value;
            return W_OK;
        case WEVOA_CFG_SHELL_LAYOUT:
            if (value > 1u) {
                return W_ERR_INVALID_ARG;
            }
            g_cfg.shell_layout = (uint8_t)value;
            return W_OK;
        case WEVOA_CFG_UI_BLUR_LEVEL:
            if (value > 3u) {
                return W_ERR_INVALID_ARG;
            }
            g_cfg.ui_blur_level = (uint8_t)value;
            return W_OK;
        case WEVOA_CFG_UI_MOTION_LEVEL:
            if (value > 3u) {
                return W_ERR_INVALID_ARG;
            }
            g_cfg.ui_motion_level = (uint8_t)value;
            return W_OK;
        case WEVOA_CFG_UI_EFFECTS_QUALITY:
            if (value > 3u) {
                return W_ERR_INVALID_ARG;
            }
            g_cfg.ui_effects_quality = (uint8_t)value;
            return W_OK;
        default:
            return W_ERR_INVALID_ARG;
    }
}
