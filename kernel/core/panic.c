#include "wevoa/panic.h"
#include "wevoa/console.h"
#include "wevoa/ports.h"

void panic(const char* message) {
    console_set_color(0x0F, 0x04);
    console_write("PANIC: ");
    if (message != 0) {
        console_write(message);
    }
    console_putc('\n');

    for (;;) {
        cpu_halt();
    }
}

