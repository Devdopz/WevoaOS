#include "wevoa/console.h"

#include <stddef.h>
#include <stdint.h>

static volatile uint16_t* const VGA = (volatile uint16_t*)0xB8000;
static uint8_t g_row = 0;
static uint8_t g_col = 0;
static uint8_t g_color = 0x0F;

static void scroll_if_needed(void) {
    if (g_row < 25) {
        return;
    }

    for (size_t row = 1; row < 25; ++row) {
        for (size_t col = 0; col < 80; ++col) {
            VGA[(row - 1) * 80 + col] = VGA[row * 80 + col];
        }
    }

    for (size_t col = 0; col < 80; ++col) {
        VGA[(24 * 80) + col] = ((uint16_t)g_color << 8) | ' ';
    }
    g_row = 24;
}

void console_set_color(uint8_t fg, uint8_t bg) {
    g_color = (uint8_t)((bg << 4) | (fg & 0x0F));
}

void console_clear(void) {
    for (size_t i = 0; i < 80 * 25; ++i) {
        VGA[i] = ((uint16_t)g_color << 8) | ' ';
    }
    g_row = 0;
    g_col = 0;
}

void console_init(void) {
    console_set_color(0x0F, 0x00);
    console_clear();
}

void console_putc(char c) {
    if (c == '\n') {
        g_col = 0;
        ++g_row;
        scroll_if_needed();
        return;
    }

    if (c == '\r') {
        g_col = 0;
        return;
    }

    if (c == '\b') {
        if (g_col > 0) {
            --g_col;
            VGA[g_row * 80 + g_col] = ((uint16_t)g_color << 8) | ' ';
        }
        return;
    }

    VGA[g_row * 80 + g_col] = ((uint16_t)g_color << 8) | (uint8_t)c;
    ++g_col;
    if (g_col >= 80) {
        g_col = 0;
        ++g_row;
        scroll_if_needed();
    }
}

void console_write(const char* s) {
    if (s == NULL) {
        return;
    }
    while (*s != '\0') {
        console_putc(*s++);
    }
}

