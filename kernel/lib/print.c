#include "wevoa/print.h"
#include "wevoa/console.h"

void print_write(const char* s) {
    console_write(s);
}

void print_line(const char* s) {
    console_write(s);
    console_putc('\n');
}

void print_u64_dec(uint64_t value) {
    char buf[21];
    int i = 0;
    if (value == 0) {
        console_putc('0');
        return;
    }
    while (value > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (i > 0) {
        console_putc(buf[--i]);
    }
}

void print_u64_hex(uint64_t value) {
    const char* hex = "0123456789ABCDEF";
    console_write("0x");
    for (int i = 15; i >= 0; --i) {
        uint8_t nibble = (uint8_t)((value >> (i * 4)) & 0xFu);
        console_putc(hex[nibble]);
    }
}

