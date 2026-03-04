#include "wevoa/mm.h"
#include "wevoa/boot_info.h"

#include <stddef.h>
#include <stdint.h>

extern uint8_t __kernel_end;

static uint64_t g_region_start = 0;
static uint64_t g_region_end = 0;
static uint64_t g_next_free = 0;

static uint64_t align_up_u64(uint64_t value, uint64_t alignment) {
    return (value + (alignment - 1u)) & ~(alignment - 1u);
}

void pmm_init(const struct wevoa_boot_info* info) {
    uint64_t kernel_end = (uint64_t)(uintptr_t)&__kernel_end;
    g_region_start = align_up_u64(kernel_end, 4096u);
    g_next_free = g_region_start;

    uint64_t upper = 0;
    if (info != NULL) {
        upper = info->mem_upper_kb;
    }
    g_region_end = 0x00100000ull + upper * 1024ull;

    if (g_region_end < g_region_start) {
        g_region_end = g_region_start;
    }
}

void* pmm_alloc_frame(void) {
    return pmm_alloc_bytes(4096u);
}

void pmm_free_frame(void* frame) {
    (void)frame;
}

void* pmm_alloc_bytes(size_t size) {
    if (size == 0u) {
        return NULL;
    }

    uint64_t aligned_size = align_up_u64((uint64_t)size, 16u);
    uint64_t candidate = align_up_u64(g_next_free, 16u);

    if (candidate + aligned_size > g_region_end) {
        return NULL;
    }

    void* out = (void*)(uintptr_t)candidate;
    g_next_free = candidate + aligned_size;
    return out;
}

uint64_t pmm_total_kb(void) {
    if (g_region_end <= g_region_start) {
        return 0;
    }
    return (g_region_end - g_region_start) / 1024u;
}

uint64_t pmm_used_kb(void) {
    if (g_next_free <= g_region_start) {
        return 0;
    }
    return (g_next_free - g_region_start) / 1024u;
}

