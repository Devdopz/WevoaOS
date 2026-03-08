#ifndef WEVOA_UI_STYLE_H
#define WEVOA_UI_STYLE_H

#include "wevoa/ui_tokens.h"

#include <stdint.h>

typedef void (*wevoa_ui_fill_rect_fn)(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t c);
typedef void (*wevoa_ui_stroke_rect_fn)(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t c);

void wevoa_ui_style_load_default_theme(uint8_t theme_rgb6[COLOR_COUNT][3]);
void wevoa_ui_style_draw_glass_rect(int32_t x,
                                    int32_t y,
                                    int32_t w,
                                    int32_t h,
                                    uint8_t base,
                                    uint8_t hi,
                                    uint8_t lo,
                                    uint8_t border,
                                    wevoa_ui_fill_rect_fn fill_rect,
                                    wevoa_ui_stroke_rect_fn stroke_rect);

#endif
