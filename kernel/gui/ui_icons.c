#include "wevoa/ui_icons.h"

static bool icon_painter_ready(const struct wevoa_ui_icon_painter* painter) {
    if (painter == 0) {
        return false;
    }
    if (painter->fill_rect == 0 || painter->stroke_rect == 0 || painter->draw_glass_rect == 0) {
        return false;
    }
    return true;
}

void wevoa_ui_icons_draw_app(uint8_t app,
                             int32_t x,
                             int32_t y,
                             int32_t s,
                             const struct wevoa_ui_icon_painter* painter) {
    if (!icon_painter_ready(painter)) {
        return;
    }

    int32_t pad = s / 8;
    if (pad < 2) {
        pad = 2;
    }
    int32_t bx = x + pad;
    int32_t by = y + pad;
    int32_t bw = s - pad * 2;
    int32_t bh = s - pad * 2;
    if (bw < 8 || bh < 8) {
        return;
    }

    painter->draw_glass_rect(bx, by, bw, bh, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);

    if (app == APP_FILE_MANAGER) {
        int32_t fy = by + bh / 3;
        painter->fill_rect(bx + 3, fy, bw - 6, bh - (fy - by) - 3, COLOR_ICON_FOLDER);
        painter->fill_rect(bx + 3, fy - 3, bw / 3, 3, COLOR_ICON_FOLDER);
        painter->stroke_rect(bx + 3, fy, bw - 6, bh - (fy - by) - 3, COLOR_BORDER);
        return;
    }

    if (app == APP_TERMINAL) {
        painter->fill_rect(bx + 2, by + 2, bw - 4, bh - 4, COLOR_BLACK);
        painter->stroke_rect(bx + 2, by + 2, bw - 4, bh - 4, COLOR_BORDER);
        painter->fill_rect(bx + 4, by + 5, bw - 8, 2, COLOR_ICON_TERMINAL);
        painter->fill_rect(bx + 5, by + bh - 7, bw / 3, 1, COLOR_ICON_TERMINAL);
        painter->fill_rect(bx + 5, by + bh - 5, 2, 1, COLOR_ICON_TERMINAL);
        return;
    }

    if (app == APP_SETTINGS) {
        int32_t cx = bx + bw / 2;
        int32_t cy = by + bh / 2;
        painter->fill_rect(cx - 4, by + 2, 8, bh - 4, COLOR_ICON_SETTINGS);
        painter->fill_rect(bx + 2, cy - 4, bw - 4, 8, COLOR_ICON_SETTINGS);
        painter->fill_rect(cx - 6, cy - 1, 12, 2, COLOR_ICON_SETTINGS);
        painter->fill_rect(cx - 1, cy - 6, 2, 12, COLOR_ICON_SETTINGS);
        painter->fill_rect(cx - 3, cy - 3, 6, 6, COLOR_WINDOW_BG);
        painter->stroke_rect(cx - 3, cy - 3, 6, 6, COLOR_BORDER);
        return;
    }

    if (app == APP_NOTEPAD) {
        painter->fill_rect(bx + 2, by + 2, bw - 4, bh - 4, COLOR_BLACK);
        painter->stroke_rect(bx + 2, by + 2, bw - 4, bh - 4, COLOR_BORDER);
        painter->fill_rect(bx + 4, by + 4, bw - 8, 2, COLOR_PANEL_BG);
        painter->fill_rect(bx + 5, by + 9, bw - 10, 1, COLOR_WINDOW_BG);
        painter->fill_rect(bx + 6, by + 12, bw - 13, 1, COLOR_WINDOW_BG);
        painter->fill_rect(bx + 7, by + 15, bw - 16, 1, COLOR_WINDOW_BG);
        painter->fill_rect(bx + bw - 10, by + 7, 1, 3, COLOR_PANEL_BG);
        painter->fill_rect(bx + bw - 9, by + 8, 2, 1, COLOR_PANEL_BG);
        return;
    }

    if (app == APP_CALCULATOR) {
        painter->fill_rect(bx + 3, by + 3, bw - 6, 4, COLOR_PANEL_BG);
        painter->stroke_rect(bx + 3, by + 3, bw - 6, 4, COLOR_BORDER);
        int32_t ky = by + 9;
        int32_t cw = (bw - 10) / 3;
        int32_t ch = (bh - 12) / 3;
        for (int32_t r = 0; r < 3; ++r) {
            for (int32_t c = 0; c < 3; ++c) {
                int32_t kx = bx + 3 + c * (cw + 1);
                int32_t sy = ky + r * (ch + 1);
                painter->fill_rect(kx, sy, cw, ch, (r + c) % 2 == 0 ? COLOR_ACCENT : COLOR_PANEL_BG);
            }
        }
        return;
    }

    if (app == APP_IMAGE_VIEWER) {
        painter->fill_rect(bx + 2, by + 2, bw - 4, bh - 4, COLOR_WINDOW_BG);
        painter->stroke_rect(bx + 2, by + 2, bw - 4, bh - 4, COLOR_BORDER);
        painter->fill_rect(bx + 4, by + 4, bw - 8, bh / 3, COLOR_DESKTOP);
        painter->fill_rect(bx + 6, by + bh / 3 + 3, bw / 2, 2, COLOR_TITLE_INACTIVE);
        painter->fill_rect(bx + bw / 2, by + bh / 3 + 1, bw / 3, 4, COLOR_TITLE_INACTIVE);
        painter->fill_rect(bx + 5, by + 5, 3, 3, COLOR_ICON_FOLDER);
        return;
    }

    if (app == APP_VIDEO_PLAYER) {
        painter->fill_rect(bx + 2, by + 2, bw - 4, bh - 7, COLOR_BLACK);
        painter->stroke_rect(bx + 2, by + 2, bw - 4, bh - 7, COLOR_BORDER);
        painter->fill_rect(bx + bw / 2 - 2, by + bh / 2 - 5, 5, 10, COLOR_ACCENT);
        painter->fill_rect(bx + 4, by + bh - 4, bw - 8, 2, COLOR_ACCENT);
        return;
    }

    painter->fill_rect(bx + 2, by + 2, bw - 4, bh - 4, COLOR_PANEL_BG);
    painter->stroke_rect(bx + 2, by + 2, bw - 4, bh - 4, COLOR_BORDER);
    painter->fill_rect(bx + 3, by + 3, bw - 6, 3, COLOR_ACCENT);
    painter->fill_rect(bx + bw / 2 - 3, by + 8, 6, 6, COLOR_WINDOW_BG);
    painter->stroke_rect(bx + bw / 2 - 3, by + 8, 6, 6, COLOR_BORDER);
    painter->fill_rect(bx + bw / 2 - 8, by + 10, 16, 1, COLOR_BORDER);
    painter->fill_rect(bx + bw / 2 - 1, by + 6, 1, 10, COLOR_BORDER);
}

void wevoa_ui_icons_draw_desktop_item(bool is_dir,
                                      int32_t x,
                                      int32_t y,
                                      int32_t s,
                                      const struct wevoa_ui_icon_painter* painter) {
    if (!icon_painter_ready(painter)) {
        return;
    }

    if (is_dir) {
        painter->fill_rect(x + 2, y + 4, s - 4, s - 7, COLOR_ICON_FOLDER);
        painter->fill_rect(x + 2, y + 2, s / 2, 3, COLOR_ICON_FOLDER);
        painter->stroke_rect(x + 2, y + 4, s - 4, s - 7, COLOR_BORDER);
        return;
    }

    painter->fill_rect(x + 3, y + 2, s - 6, s - 4, COLOR_WINDOW_BG);
    painter->stroke_rect(x + 3, y + 2, s - 6, s - 4, COLOR_BORDER);
    painter->fill_rect(x + 6, y + 6, s - 12, 1, COLOR_BORDER);
    painter->fill_rect(x + 6, y + 9, s - 14, 1, COLOR_BORDER);
}
