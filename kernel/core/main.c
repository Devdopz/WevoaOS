#include "wevoa/boot_info.h"
#include "wevoa/console.h"
#include "wevoa/fs.h"
#include "wevoa/gui.h"
#include "wevoa/idt.h"
#include "wevoa/keyboard.h"
#include "wevoa/mm.h"
#include "wevoa/net.h"
#include "wevoa/panic.h"
#include "wevoa/print.h"
#include "wevoa/shell.h"

#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __bss_end;

static void clear_bss(void) {
    uint8_t* p = &__bss_start;
    while (p < &__bss_end) {
        *p++ = 0;
    }
}

void kernel_start(struct wevoa_boot_info* boot_info) {
    clear_bss();

    if (boot_info == 0 || boot_info->boot_info_version != WEVOA_BOOT_INFO_VERSION) {
        console_init();
        panic("invalid boot info");
    }

    (void)net_init();

    if ((boot_info->fb_type == WEVOA_FB_TYPE_VGA13 ||
         boot_info->fb_type == WEVOA_FB_TYPE_LINEAR) &&
        boot_info->fb_phys_addr != 0 &&
        boot_info->fb_width != 0 &&
        boot_info->fb_height != 0 &&
        boot_info->fb_pitch != 0) {
        gui_run(boot_info);
    }

    console_init();
    print_line("Wevoa kernel booting...");

    idt_init();
    keyboard_init();
    pmm_init(boot_info);
    kheap_init();
    (void)fs_mount();

    print_write("mem_lower_kb=");
    print_u64_dec(boot_info->mem_lower_kb);
    print_write(" mem_upper_kb=");
    print_u64_dec(boot_info->mem_upper_kb);
    print_line("");

    print_line("Entering shell.");
    shell_init(boot_info);
    shell_run();

    panic("shell exited");
}
