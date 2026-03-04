bits 16
org 0x7C00

%ifndef STAGE2_SECTORS
%define STAGE2_SECTORS 32
%endif

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, msg_boot
    call print_string

    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]
    int 0x13
    jc disk_fail
    cmp bx, 0xAA55
    jne disk_fail

    mov byte [dap], 0x10
    mov byte [dap + 1], 0x00
    mov word [dap + 2], STAGE2_SECTORS
    mov word [dap + 4], 0x8000
    mov word [dap + 6], 0x0000
    mov dword [dap + 8], 1
    mov dword [dap + 12], 0

    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc disk_fail

    mov dl, [boot_drive]
    jmp 0x0000:0x8000

disk_fail:
    mov si, msg_fail
    call print_string
    cli
    hlt
    jmp $

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    int 0x10
    jmp print_string
.done:
    ret

boot_drive: db 0
msg_boot: db "Wevoa stage1", 13, 10, 0
msg_fail: db "Disk load fail", 13, 10, 0

dap: times 16 db 0

times 510 - ($ - $$) db 0
dw 0xAA55

