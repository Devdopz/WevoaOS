#ifndef WEVOA_SHELL_H
#define WEVOA_SHELL_H

#include "wevoa/boot_info.h"

typedef int (*shell_cmd_fn)(int argc, char** argv);

void shell_init(const struct wevoa_boot_info* info);
int shell_register(const char* name, shell_cmd_fn fn, const char* help);
void shell_run(void);

#endif

