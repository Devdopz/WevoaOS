#include "wevoa/ui_style.h"

void wevoa_ui_style_load_default_theme(uint8_t theme_rgb6[COLOR_COUNT][3]) {
    theme_rgb6[COLOR_BLACK][0] = 0u;
    theme_rgb6[COLOR_BLACK][1] = 0u;
    theme_rgb6[COLOR_BLACK][2] = 0u;

    theme_rgb6[COLOR_DESKTOP][0] = 22u;
    theme_rgb6[COLOR_DESKTOP][1] = 36u;
    theme_rgb6[COLOR_DESKTOP][2] = 60u;

    theme_rgb6[COLOR_TASKBAR][0] = 4u;
    theme_rgb6[COLOR_TASKBAR][1] = 8u;
    theme_rgb6[COLOR_TASKBAR][2] = 24u;

    theme_rgb6[COLOR_TASKBAR_HI][0] = 20u;
    theme_rgb6[COLOR_TASKBAR_HI][1] = 30u;
    theme_rgb6[COLOR_TASKBAR_HI][2] = 52u;

    theme_rgb6[COLOR_BORDER][0] = 8u;
    theme_rgb6[COLOR_BORDER][1] = 12u;
    theme_rgb6[COLOR_BORDER][2] = 26u;

    theme_rgb6[COLOR_WINDOW_BG][0] = 60u;
    theme_rgb6[COLOR_WINDOW_BG][1] = 61u;
    theme_rgb6[COLOR_WINDOW_BG][2] = 63u;

    theme_rgb6[COLOR_PANEL_BG][0] = 50u;
    theme_rgb6[COLOR_PANEL_BG][1] = 54u;
    theme_rgb6[COLOR_PANEL_BG][2] = 61u;

    theme_rgb6[COLOR_TITLE_ACTIVE][0] = 8u;
    theme_rgb6[COLOR_TITLE_ACTIVE][1] = 36u;
    theme_rgb6[COLOR_TITLE_ACTIVE][2] = 63u;

    theme_rgb6[COLOR_TITLE_INACTIVE][0] = 28u;
    theme_rgb6[COLOR_TITLE_INACTIVE][1] = 24u;
    theme_rgb6[COLOR_TITLE_INACTIVE][2] = 46u;

    theme_rgb6[COLOR_ACCENT][0] = 63u;
    theme_rgb6[COLOR_ACCENT][1] = 24u;
    theme_rgb6[COLOR_ACCENT][2] = 14u;

    theme_rgb6[COLOR_ICON_FOLDER][0] = 62u;
    theme_rgb6[COLOR_ICON_FOLDER][1] = 50u;
    theme_rgb6[COLOR_ICON_FOLDER][2] = 10u;

    theme_rgb6[COLOR_ICON_TERMINAL][0] = 12u;
    theme_rgb6[COLOR_ICON_TERMINAL][1] = 56u;
    theme_rgb6[COLOR_ICON_TERMINAL][2] = 26u;

    theme_rgb6[COLOR_ICON_SETTINGS][0] = 12u;
    theme_rgb6[COLOR_ICON_SETTINGS][1] = 46u;
    theme_rgb6[COLOR_ICON_SETTINGS][2] = 62u;

    theme_rgb6[COLOR_CURSOR_OUT][0] = 0u;
    theme_rgb6[COLOR_CURSOR_OUT][1] = 0u;
    theme_rgb6[COLOR_CURSOR_OUT][2] = 0u;

    theme_rgb6[COLOR_CURSOR_IN][0] = 63u;
    theme_rgb6[COLOR_CURSOR_IN][1] = 63u;
    theme_rgb6[COLOR_CURSOR_IN][2] = 60u;

    theme_rgb6[COLOR_SHADOW][0] = 3u;
    theme_rgb6[COLOR_SHADOW][1] = 4u;
    theme_rgb6[COLOR_SHADOW][2] = 9u;

    theme_rgb6[COLOR_WALL_SKY][0] = 20u;
    theme_rgb6[COLOR_WALL_SKY][1] = 40u;
    theme_rgb6[COLOR_WALL_SKY][2] = 63u;

    theme_rgb6[COLOR_WALL_HORIZON][0] = 45u;
    theme_rgb6[COLOR_WALL_HORIZON][1] = 22u;
    theme_rgb6[COLOR_WALL_HORIZON][2] = 55u;

    theme_rgb6[COLOR_WALL_SUN][0] = 63u;
    theme_rgb6[COLOR_WALL_SUN][1] = 56u;
    theme_rgb6[COLOR_WALL_SUN][2] = 10u;

    theme_rgb6[COLOR_WALL_GROUND][0] = 10u;
    theme_rgb6[COLOR_WALL_GROUND][1] = 28u;
    theme_rgb6[COLOR_WALL_GROUND][2] = 18u;
}

void wevoa_ui_style_draw_glass_rect(int32_t x,
                                    int32_t y,
                                    int32_t w,
                                    int32_t h,
                                    uint8_t base,
                                    uint8_t hi,
                                    uint8_t lo,
                                    uint8_t border,
                                    wevoa_ui_fill_rect_fn fill_rect,
                                    wevoa_ui_stroke_rect_fn stroke_rect) {
    if (fill_rect == 0 || stroke_rect == 0) {
        return;
    }
    if (w <= 2 || h <= 2) {
        return;
    }

    fill_rect(x, y, w, h, base);
    fill_rect(x + 1, y + 1, w - 2, 1, hi);
    if (h > 6) {
        fill_rect(x + 1, y + 2, w - 2, 1, hi);
    }
    fill_rect(x + 1, y + h - 2, w - 2, 1, lo);
    stroke_rect(x, y, w, h, border);
}
