#ifndef WEVOA_IDT_H
#define WEVOA_IDT_H

#include <stdint.h>

typedef void (*irq_handler_t)(void);

void idt_init(void);
void irq_install_handler(uint8_t irq, irq_handler_t fn);
void irq_ack(uint8_t irq);

#endif

