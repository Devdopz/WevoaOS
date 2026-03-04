bits 64

global default_interrupt_stub

section .text
default_interrupt_stub:
    cli
.loop:
    hlt
    jmp .loop

