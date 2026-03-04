bits 64

global _start
extern kernel_start

section .text
_start:
    cli
    mov rsp, 0x8F000
    and rsp, -16
    mov rcx, rdi
    call kernel_start

.hang:
    hlt
    jmp .hang
