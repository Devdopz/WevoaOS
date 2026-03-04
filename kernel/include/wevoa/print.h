#ifndef WEVOA_PRINT_H
#define WEVOA_PRINT_H

#include <stdint.h>

void print_write(const char* s);
void print_line(const char* s);
void print_u64_dec(uint64_t value);
void print_u64_hex(uint64_t value);

#endif

