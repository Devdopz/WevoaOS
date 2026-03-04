#include "wevoa/shell.h"

#include "wevoa/console.h"
#include "wevoa/keyboard.h"
#include "wevoa/mm.h"
#include "wevoa/ports.h"
#include "wevoa/print.h"
#include "wevoa/string.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SHELL_MAX_COMMANDS 32
#define SHELL_MAX_LINE 128
#define SHELL_MAX_ARGS 16

struct shell_command {
    const char* name;
    shell_cmd_fn fn;
    const char* help;
};

static struct shell_command g_commands[SHELL_MAX_COMMANDS];
static size_t g_command_count = 0;
static const struct wevoa_boot_info* g_boot_info = 0;
static uint64_t g_boot_tsc = 0;

static void shell_prompt(void) {
    print_write("wevoa> ");
}

static void shell_read_line(char* out, size_t max_len) {
    size_t idx = 0;
    for (;;) {
        char c = keyboard_read_char_blocking();
        if (c == '\r' || c == '\n') {
            console_putc('\n');
            out[idx] = '\0';
            return;
        }
        if (c == '\b') {
            if (idx > 0) {
                --idx;
                console_putc('\b');
            }
            continue;
        }
        if (idx + 1 < max_len) {
            out[idx++] = c;
            console_putc(c);
        }
    }
}

static int shell_tokenize(char* line, char** argv, int max_args) {
    int argc = 0;
    bool in_token = false;

    for (char* p = line; *p != '\0'; ++p) {
        if (*p == ' ' || *p == '\t') {
            *p = '\0';
            in_token = false;
            continue;
        }
        if (!in_token) {
            if (argc < max_args) {
                argv[argc++] = p;
            }
            in_token = true;
        }
    }
    return argc;
}

static int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    for (size_t i = 0; i < g_command_count; ++i) {
        print_write(g_commands[i].name);
        print_write(" - ");
        print_line(g_commands[i].help);
    }
    return 0;
}

static int cmd_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_clear();
    return 0;
}

static int cmd_mem(int argc, char** argv) {
    (void)argc;
    (void)argv;
    print_write("boot lower KB: ");
    print_u64_dec(g_boot_info != 0 ? g_boot_info->mem_lower_kb : 0);
    print_line("");
    print_write("boot upper KB: ");
    print_u64_dec(g_boot_info != 0 ? g_boot_info->mem_upper_kb : 0);
    print_line("");
    print_write("pmm total KB: ");
    print_u64_dec(pmm_total_kb());
    print_line("");
    print_write("pmm used KB: ");
    print_u64_dec(pmm_used_kb());
    print_line("");
    print_write("heap bytes used: ");
    print_u64_dec(kheap_bytes_used());
    print_line("");
    return 0;
}

static int cmd_uptime(int argc, char** argv) {
    (void)argc;
    (void)argv;
    uint64_t delta = rdtsc() - g_boot_tsc;
    print_write("uptime cycles: ");
    print_u64_dec(delta);
    print_line("");
    return 0;
}

static int cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        print_write(argv[i]);
        if (i + 1 < argc) {
            console_putc(' ');
        }
    }
    print_line("");
    return 0;
}

static int cmd_reboot(int argc, char** argv) {
    (void)argc;
    (void)argv;
    print_line("Rebooting...");
    while ((inb(0x64) & 0x02u) != 0u) {
    }
    outb(0x64, 0xFE);
    for (;;) {
        cpu_halt();
    }
    return 0;
}

void shell_init(const struct wevoa_boot_info* info) {
    g_boot_info = info;
    g_boot_tsc = rdtsc();
    g_command_count = 0;

    shell_register("help", cmd_help, "List available commands");
    shell_register("clear", cmd_clear, "Clear the screen");
    shell_register("mem", cmd_mem, "Show memory summary");
    shell_register("uptime", cmd_uptime, "Show cycles since shell init");
    shell_register("echo", cmd_echo, "Print arguments");
    shell_register("reboot", cmd_reboot, "Reboot machine");
}

int shell_register(const char* name, shell_cmd_fn fn, const char* help) {
    if (name == 0 || fn == 0 || help == 0) {
        return -1;
    }
    if (g_command_count >= SHELL_MAX_COMMANDS) {
        return -1;
    }
    g_commands[g_command_count].name = name;
    g_commands[g_command_count].fn = fn;
    g_commands[g_command_count].help = help;
    ++g_command_count;
    return 0;
}

void shell_run(void) {
    char line[SHELL_MAX_LINE];
    char* argv[SHELL_MAX_ARGS];

    for (;;) {
        shell_prompt();
        shell_read_line(line, sizeof(line));

        int argc = shell_tokenize(line, argv, SHELL_MAX_ARGS);
        if (argc == 0) {
            continue;
        }

        bool executed = false;
        for (size_t i = 0; i < g_command_count; ++i) {
            if (kstrcmp(argv[0], g_commands[i].name) == 0) {
                (void)g_commands[i].fn(argc, argv);
                executed = true;
                break;
            }
        }

        if (!executed) {
            print_write("unknown command: ");
            print_line(argv[0]);
        }
    }
}
