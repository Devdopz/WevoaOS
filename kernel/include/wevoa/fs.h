#ifndef WEVOA_FS_H
#define WEVOA_FS_H

#include <stdint.h>

int fs_mount(void);
int fs_open(const char* path, uint32_t flags);
int64_t fs_read(int fd, void* buf, uint64_t n);
int64_t fs_write(int fd, const void* buf, uint64_t n);
int fs_close(int fd);

#endif

