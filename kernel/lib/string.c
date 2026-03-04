#include "wevoa/string.h"

#include <stddef.h>

size_t kstrlen(const char* s) {
    size_t n = 0;
    if (s == NULL) {
        return 0;
    }
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

int kstrcmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return (int)((unsigned char)a[i] - (unsigned char)b[i]);
        }
        ++i;
    }
    return (int)((unsigned char)a[i] - (unsigned char)b[i]);
}

int kstrncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i] || a[i] == '\0' || b[i] == '\0') {
            return (int)((unsigned char)a[i] - (unsigned char)b[i]);
        }
    }
    return 0;
}

void* kmemset(void* dst, int value, size_t n) {
    unsigned char* out = (unsigned char*)dst;
    for (size_t i = 0; i < n; ++i) {
        out[i] = (unsigned char)value;
    }
    return dst;
}

void* kmemcpy(void* dst, const void* src, size_t n) {
    unsigned char* out = (unsigned char*)dst;
    const unsigned char* in = (const unsigned char*)src;
    for (size_t i = 0; i < n; ++i) {
        out[i] = in[i];
    }
    return dst;
}

