#include "wevoa/keyboard.h"
#include "wevoa/ports.h"

#include <stdbool.h>
#include <stdint.h>

static bool shift_down = false;

static const char keymap[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,
};

static const char keymap_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,
};

void keyboard_init(void) {
    shift_down = false;
}

char keyboard_read_char_blocking(void) {
    for (;;) {
        if ((inb(0x64) & 0x01u) == 0u) {
            continue;
        }

        uint8_t scancode = inb(0x60);
        if (scancode == 0x2A || scancode == 0x36) {
            shift_down = true;
            continue;
        }
        if (scancode == 0xAA || scancode == 0xB6) {
            shift_down = false;
            continue;
        }
        if ((scancode & 0x80u) != 0u) {
            continue;
        }

        char c = shift_down ? keymap_shift[scancode] : keymap[scancode];
        if (c != 0) {
            return c;
        }
    }
}

