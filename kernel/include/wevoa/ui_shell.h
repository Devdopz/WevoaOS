#ifndef WEVOA_UI_SHELL_H
#define WEVOA_UI_SHELL_H

#include <stdbool.h>
#include <stdint.h>

bool wevoa_ui_shell_is_dock(uint8_t shell_layout);
int32_t wevoa_ui_shell_top_h(uint8_t shell_layout, int32_t scale, int32_t taskbar_h);
int32_t wevoa_ui_shell_bottom_h(uint8_t shell_layout, int32_t scale);

#endif
