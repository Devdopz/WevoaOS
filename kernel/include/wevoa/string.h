#ifndef WEVOA_STRING_H
#define WEVOA_STRING_H

#include <stddef.h>

size_t kstrlen(const char* s);
int kstrcmp(const char* a, const char* b);
int kstrncmp(const char* a, const char* b, size_t n);
void* kmemset(void* dst, int value, size_t n);
void* kmemcpy(void* dst, const void* src, size_t n);

#endif

