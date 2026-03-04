#include "wevoa/mm.h"

#include <stddef.h>
#include <stdint.h>

static uint64_t g_heap_used = 0;
static uint64_t g_heap_capacity = 0;

void kheap_init(void) {
    g_heap_used = 0;
    g_heap_capacity = pmm_total_kb() * 1024u;
}

void* kmalloc(size_t size) {
    void* ptr = pmm_alloc_bytes(size);
    if (ptr != NULL) {
        g_heap_used += (uint64_t)size;
    }
    return ptr;
}

void kfree(void* ptr) {
    (void)ptr;
}

uint64_t kheap_bytes_used(void) {
    return g_heap_used;
}

uint64_t kheap_bytes_capacity(void) {
    return g_heap_capacity;
}

