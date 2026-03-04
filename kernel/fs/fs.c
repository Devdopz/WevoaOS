#include "wevoa/fs.h"

#include <stdint.h>

int fs_mount(void) {
    return 0;
}

int fs_open(const char* path, uint32_t flags) {
    (void)path;
    (void)flags;
    return -1;
}

int64_t fs_read(int fd, void* buf, uint64_t n) {
    (void)fd;
    (void)buf;
    (void)n;
    return -1;
}

int64_t fs_write(int fd, const void* buf, uint64_t n) {
    (void)fd;
    (void)buf;
    (void)n;
    return -1;
}

int fs_close(int fd) {
    (void)fd;
    return -1;
}

