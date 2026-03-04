#ifndef WEVOA_MM_H
#define WEVOA_MM_H

#include <stddef.h>
#include <stdint.h>

struct wevoa_boot_info;

void pmm_init(const struct wevoa_boot_info* info);
void* pmm_alloc_frame(void);
void pmm_free_frame(void* frame);
void* pmm_alloc_bytes(size_t size);

void kheap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

uint64_t pmm_total_kb(void);
uint64_t pmm_used_kb(void);
uint64_t kheap_bytes_used(void);
uint64_t kheap_bytes_capacity(void);

#endif

