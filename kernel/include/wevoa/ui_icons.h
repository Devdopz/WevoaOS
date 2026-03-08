#ifndef WEVOA_UI_ICONS_H
#define WEVOA_UI_ICONS_H

#include "wevoa/ui_style.h"

#include <stdbool.h>
#include <stdint.h>

typedef void (*wevoa_ui_glass_rect_fn)(int32_t x,
                                       int32_t y,
                                       int32_t w,
                                       int32_t h,
                                       uint8_t base,
                                       uint8_t hi,
                                       uint8_t lo);

struct wevoa_ui_icon_painter {
    wevoa_ui_fill_rect_fn fill_rect;
    wevoa_ui_stroke_rect_fn stroke_rect;
    wevoa_ui_glass_rect_fn draw_glass_rect;
};

void wevoa_ui_icons_draw_app(uint8_t app,
                             int32_t x,
                             int32_t y,
                             int32_t s,
                             const struct wevoa_ui_icon_painter* painter);
void wevoa_ui_icons_draw_desktop_item(bool is_dir,
                                      int32_t x,
                                      int32_t y,
                                      int32_t s,
                                      const struct wevoa_ui_icon_painter* painter);

#endif
