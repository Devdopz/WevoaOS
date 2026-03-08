#include "wevoa/ui_shell.h"

#define WEVOA_UI_SHELL_DOCK 0u

static int32_t clamp_i32_local(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

bool wevoa_ui_shell_is_dock(uint8_t shell_layout) {
    return shell_layout == WEVOA_UI_SHELL_DOCK;
}

int32_t wevoa_ui_shell_top_h(uint8_t shell_layout, int32_t scale, int32_t taskbar_h) {
    if (wevoa_ui_shell_is_dock(shell_layout)) {
        return clamp_i32_local(16 + scale * 4, 18, 34);
    }
    return taskbar_h;
}

int32_t wevoa_ui_shell_bottom_h(uint8_t shell_layout, int32_t scale) {
    if (wevoa_ui_shell_is_dock(shell_layout)) {
        return clamp_i32_local(22 + scale * 7, 32, 52);
    }
    return 0;
}
