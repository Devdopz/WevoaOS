#include "wevoa/idt.h"
#include "wevoa/ports.h"
#include "wevoa/string.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern void default_interrupt_stub(void);

static struct idt_entry idt[256];
static irq_handler_t irq_handlers[16];

static void idt_set_gate(uint8_t vector, void* handler, uint8_t type_attr) {
    uint64_t addr = (uint64_t)handler;
    idt[vector].offset_low = (uint16_t)(addr & 0xFFFFu);
    idt[vector].selector = 0x18;
    idt[vector].ist = 0;
    idt[vector].type_attr = type_attr;
    idt[vector].offset_mid = (uint16_t)((addr >> 16) & 0xFFFFu);
    idt[vector].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFFu);
    idt[vector].zero = 0;
}

void idt_init(void) {
    kmemset(idt, 0, sizeof(idt));
    kmemset(irq_handlers, 0, sizeof(irq_handlers));

    for (uint32_t i = 0; i < 256; ++i) {
        idt_set_gate((uint8_t)i, (void*)default_interrupt_stub, 0x8Eu);
    }

    struct idt_ptr ptr;
    ptr.limit = (uint16_t)(sizeof(idt) - 1u);
    ptr.base = (uint64_t)&idt[0];
    __asm__ volatile ("lidt %0" : : "m"(ptr));

    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void irq_install_handler(uint8_t irq, irq_handler_t fn) {
    if (irq < 16u) {
        irq_handlers[irq] = fn;
    }
}

void irq_ack(uint8_t irq) {
    if (irq >= 8u) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

