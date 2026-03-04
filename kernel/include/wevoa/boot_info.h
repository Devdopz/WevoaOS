#ifndef WEVOA_BOOT_INFO_H
#define WEVOA_BOOT_INFO_H

#include <stdint.h>

#define WEVOA_BOOT_INFO_VERSION 2u
#define WEVOA_FB_TYPE_NONE 0u
#define WEVOA_FB_TYPE_VGA13 1u
#define WEVOA_FB_TYPE_LINEAR 2u

struct wevoa_boot_info {
    uint32_t boot_info_version;
    uint32_t reserved0;
    uint64_t mem_lower_kb;
    uint64_t mem_upper_kb;
    uint64_t kernel_phys_base;
    uint32_t boot_drive;
    uint32_t reserved1;
    uint64_t mmap_ptr;
    uint32_t mmap_entries;
    uint32_t reserved2;
    uint64_t vbe_mode;
    uint64_t fb_phys_addr;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
    uint32_t fb_bpp;
    uint32_t fb_type;
    uint32_t reserved3;
} __attribute__((packed));

#endif
