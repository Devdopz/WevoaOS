bits 16
org 0x8000

%ifndef STAGE2_SECTORS
%define STAGE2_SECTORS 32
%endif

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 64
%endif

%define KERNEL_LBA (1 + STAGE2_SECTORS)
%define KERNEL_LOAD_SEG 0x1000
%define KERNEL_DEST_PHYS 0x00100000
%define BOOT_INFO_ADDR 0x7000

%define BOOT_INFO_VERSION_OFF 0
%define BOOT_INFO_MEM_LOWER_OFF 8
%define BOOT_INFO_MEM_UPPER_OFF 16
%define BOOT_INFO_KERNEL_BASE_OFF 24
%define BOOT_INFO_BOOT_DRIVE_OFF 32
%define BOOT_INFO_MMAP_PTR_OFF 40
%define BOOT_INFO_MMAP_ENTRIES_OFF 48
%define BOOT_INFO_VBE_MODE_OFF 56
%define BOOT_INFO_FB_PHYS_OFF 64
%define BOOT_INFO_FB_WIDTH_OFF 72
%define BOOT_INFO_FB_HEIGHT_OFF 76
%define BOOT_INFO_FB_PITCH_OFF 80
%define BOOT_INFO_FB_BPP_OFF 84
%define BOOT_INFO_FB_TYPE_OFF 88

%define WEVOA_FB_TYPE_NONE 0
%define WEVOA_FB_TYPE_VGA13 1
%define WEVOA_FB_TYPE_LINEAR 2

%define VBE_CTRL_INFO_ADDR 0x7400
%define VBE_MODE_INFO_ADDR 0x7600
%define VBE_MODE_ATTRIBUTES 0
%define VBE_BYTES_PER_SCANLINE 16
%define VBE_X_RES 18
%define VBE_Y_RES 20
%define VBE_BPP 25
%define VBE_MEMORY_MODEL 27
%define VBE_PHYSBASE 40
%define VBE_MODE_LIST_PTR 14

%define GUI_MAX_WIDTH 2560
%define GUI_MAX_HEIGHT 1600

stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7A00
    sti

    mov [boot_drive], dl

    mov si, msg_stage2
    call print_string

    call setup_video_mode
    call build_boot_info
    call load_kernel
    call enable_a20

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp 0x08:protected_mode_start

disk_error:
    mov si, msg_disk_fail
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

build_boot_info:
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_VERSION_OFF], 2
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_VERSION_OFF + 4], 0

    int 0x12
    movzx eax, ax
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_MEM_LOWER_OFF], eax
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_MEM_LOWER_OFF + 4], 0

    mov ah, 0x88
    int 0x15
    jc .no_upper
    movzx eax, ax
    jmp .store_upper
.no_upper:
    xor eax, eax
.store_upper:
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_MEM_UPPER_OFF], eax
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_MEM_UPPER_OFF + 4], 0

    mov dword [BOOT_INFO_ADDR + BOOT_INFO_KERNEL_BASE_OFF], KERNEL_DEST_PHYS
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_KERNEL_BASE_OFF + 4], 0

    movzx eax, byte [boot_drive]
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_BOOT_DRIVE_OFF], eax
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_BOOT_DRIVE_OFF + 4], 0

    mov dword [BOOT_INFO_ADDR + BOOT_INFO_MMAP_PTR_OFF], 0
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_MMAP_PTR_OFF + 4], 0
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_MMAP_ENTRIES_OFF], 0
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_MMAP_ENTRIES_OFF + 4], 0

    movzx eax, word [video_mode]
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_VBE_MODE_OFF], eax
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_VBE_MODE_OFF + 4], 0

    mov eax, [fb_phys]
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_FB_PHYS_OFF], eax
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_FB_PHYS_OFF + 4], 0

    movzx eax, word [fb_width]
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_FB_WIDTH_OFF], eax

    movzx eax, word [fb_height]
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_FB_HEIGHT_OFF], eax

    movzx eax, word [fb_pitch]
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_FB_PITCH_OFF], eax

    movzx eax, byte [fb_bpp]
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_FB_BPP_OFF], eax

    movzx eax, byte [fb_type]
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_FB_TYPE_OFF], eax
    mov dword [BOOT_INFO_ADDR + BOOT_INFO_FB_TYPE_OFF + 4], 0
    ret

setup_video_mode:
    ; VirtualBox custom modes typically map to 0x160..0x16F.
    mov cx, 0x160
    call try_vbe_mode
    jnc .done

    mov cx, 0x161
    call try_vbe_mode
    jnc .done

    mov cx, 0x162
    call try_vbe_mode
    jnc .done

    mov cx, 0x163
    call try_vbe_mode
    jnc .done

    call select_best_vbe_mode
    jc .legacy_modes
    call try_vbe_mode
    jnc .done

.legacy_modes:
    mov cx, 0x11B
    call try_vbe_mode
    jnc .done

    mov cx, 0x11A
    call try_vbe_mode
    jnc .done

    mov cx, 0x118
    call try_vbe_mode
    jnc .done

    mov cx, 0x115
    call try_vbe_mode
    jnc .done

    mov cx, 0x112
    call try_vbe_mode
    jnc .done

    mov ax, 0x0013
    int 0x10
    mov word [video_mode], 0x0013
    mov byte [fb_type], WEVOA_FB_TYPE_VGA13
    mov dword [fb_phys], 0x000A0000
    mov word [fb_width], 320
    mov word [fb_height], 200
    mov word [fb_pitch], 320
    mov byte [fb_bpp], 8
.done:
    ret

try_vbe_mode:
    push ax
    push bx
    push cx
    push dx
    push di
    push es

    mov dx, cx
    xor ax, ax
    mov es, ax
    mov di, VBE_MODE_INFO_ADDR

    mov ax, 0x4F01
    int 0x10
    cmp ax, 0x004F
    jne .fail

    mov bx, [es:di + VBE_MODE_ATTRIBUTES]
    test bx, 0x0001
    jz .fail
    test bx, 0x0010
    jz .fail
    test bx, 0x0080
    jz .fail

    mov al, [es:di + VBE_MEMORY_MODEL]
    cmp al, 6
    je .check_bpp
    cmp al, 7
    jne .fail

.check_bpp:
    mov al, [es:di + VBE_BPP]
    cmp al, 32
    je .check_size
    cmp al, 24
    jne .fail

.check_size:
    mov ax, [es:di + VBE_X_RES]
    cmp ax, 640
    jb .fail
    cmp ax, GUI_MAX_WIDTH
    ja .fail

    mov ax, [es:di + VBE_Y_RES]
    cmp ax, 480
    jb .fail
    cmp ax, GUI_MAX_HEIGHT
    ja .fail

.set_mode:
    mov ax, 0x4F02
    mov bx, dx
    or bx, 0x4000
    int 0x10
    cmp ax, 0x004F
    jne .fail

    mov [video_mode], dx
    mov byte [fb_type], WEVOA_FB_TYPE_LINEAR
    mov eax, [es:di + VBE_PHYSBASE]
    mov [fb_phys], eax
    mov ax, [es:di + VBE_X_RES]
    mov [fb_width], ax
    mov ax, [es:di + VBE_Y_RES]
    mov [fb_height], ax
    mov ax, [es:di + VBE_BYTES_PER_SCANLINE]
    mov [fb_pitch], ax
    mov al, [es:di + VBE_BPP]
    mov [fb_bpp], al
    clc
    jmp .done

.fail:
    stc

.done:
    pop es
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

select_best_vbe_mode:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es

    mov word [best_mode], 0
    mov dword [best_area], 0
    mov byte [best_found], 0
    mov byte [best_wide], 0

    xor ax, ax
    mov es, ax
    mov di, VBE_CTRL_INFO_ADDR
    mov dword [es:di], 0x32454256
    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F
    jne .fail

    mov bx, [es:di + VBE_MODE_LIST_PTR]
    mov dx, [es:di + VBE_MODE_LIST_PTR + 2]
    mov es, dx

.scan_loop:
    mov cx, [es:bx]
    cmp cx, 0xFFFF
    je .scan_done

    push bx
    call probe_vbe_mode
    jc .next_mode

    cmp byte [best_found], 0
    je .select_mode

    mov al, [candidate_wide]
    cmp al, [best_wide]
    ja .select_mode
    jb .next_mode

    mov ax, [candidate_area + 2]
    cmp ax, [best_area + 2]
    ja .select_mode
    jb .next_mode
    mov ax, [candidate_area]
    cmp ax, [best_area]
    jbe .next_mode

.select_mode:
    mov [best_mode], cx
    mov ax, [candidate_area]
    mov [best_area], ax
    mov ax, [candidate_area + 2]
    mov [best_area + 2], ax
    mov al, [candidate_wide]
    mov [best_wide], al
    mov byte [best_found], 1

.next_mode:
    pop bx
    add bx, 2
    jmp .scan_loop

.scan_done:
    cmp byte [best_found], 1
    jne .fail
    mov cx, [best_mode]
    clc
    jmp .done

.fail:
    stc

.done:
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

probe_vbe_mode:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    push di
    push es

    xor ax, ax
    mov es, ax
    mov di, VBE_MODE_INFO_ADDR
    mov ax, 0x4F01
    int 0x10
    cmp ax, 0x004F
    jne .fail

    mov bx, [es:di + VBE_MODE_ATTRIBUTES]
    test bx, 0x0001
    jz .fail
    test bx, 0x0010
    jz .fail
    test bx, 0x0080
    jz .fail

    mov al, [es:di + VBE_MEMORY_MODEL]
    cmp al, 6
    je .check_bpp
    cmp al, 7
    jne .fail

.check_bpp:
    mov al, [es:di + VBE_BPP]
    cmp al, 32
    je .check_size
    cmp al, 24
    jne .fail

.check_size:
    mov ax, [es:di + VBE_X_RES]
    cmp ax, 640
    jb .fail
    cmp ax, GUI_MAX_WIDTH
    ja .fail

    mov dx, [es:di + VBE_Y_RES]
    cmp dx, 480
    jb .fail
    cmp dx, GUI_MAX_HEIGHT
    ja .fail

    mul dx
    mov [candidate_area], ax
    mov [candidate_area + 2], dx

    mov ax, [es:di + VBE_X_RES]
    mov bx, 10
    mul bx
    mov si, ax
    mov bp, dx

    mov ax, [es:di + VBE_Y_RES]
    mov bx, 14
    mul bx

    mov byte [candidate_wide], 0
    cmp bp, dx
    ja .mark_wide
    jb .ok
    cmp si, ax
    jb .ok

.mark_wide:
    mov byte [candidate_wide], 1

.ok:
    clc
    jmp .done

.fail:
    stc

.done:
    pop es
    pop di
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

load_kernel:
    mov byte [dap], 0x10
    mov byte [dap + 1], 0x00
    mov word [dap + 2], 1
    mov word [dap + 4], 0x0000
    mov word [dap + 6], KERNEL_LOAD_SEG
    mov dword [dap + 8], KERNEL_LBA
    mov dword [dap + 12], 0

    mov cx, KERNEL_SECTORS
.loop:
    cmp cx, 0
    je .done
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    add word [dap + 6], 0x20
    add dword [dap + 8], 1
    dec cx
    jmp .loop
.done:
    ret

enable_a20:
    in al, 0x92
    or al, 00000010b
    out 0x92, al
    ret

bits 32
protected_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x80000

    mov esi, 0x00010000
    mov edi, KERNEL_DEST_PHYS
    mov ecx, (KERNEL_SECTORS * 512) / 4
    rep movsd

    mov edi, 0x90000
    mov ecx, (4096 * 6) / 4
    xor eax, eax
    rep stosd

    mov dword [0x90000], 0x91003
    mov dword [0x90004], 0

    mov dword [0x91000], 0x92003
    mov dword [0x91004], 0
    mov dword [0x91008], 0x93003
    mov dword [0x9100C], 0
    mov dword [0x91010], 0x94003
    mov dword [0x91014], 0
    mov dword [0x91018], 0x95003
    mov dword [0x9101C], 0

    mov edi, 0x92000
    xor ebx, ebx
    mov ecx, 2048
.map_2mb_pages:
    mov eax, ebx
    or eax, 0x83
    mov [edi], eax
    mov dword [edi + 4], 0
    add ebx, 0x200000
    add edi, 8
    loop .map_2mb_pages

    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    mov eax, 0x90000
    mov cr3, eax

    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    jmp 0x18:long_mode_start

bits 64
long_mode_start:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov rsp, 0x80000
    mov rdi, BOOT_INFO_ADDR
    mov rax, KERNEL_DEST_PHYS
    jmp rax

boot_drive: db 0
video_mode: dw 0
fb_type: db WEVOA_FB_TYPE_NONE
fb_bpp: db 0
fb_width: dw 0
fb_height: dw 0
fb_pitch: dw 0
align 4
fb_phys: dd 0
best_mode: dw 0
best_area: dd 0
candidate_area: dd 0
best_found: db 0
best_wide: db 0
candidate_wide: db 0
msg_stage2: db "Wevoa stage2", 13, 10, 0
msg_disk_fail: db "Kernel load fail", 13, 10, 0
dap: times 16 db 0

align 8
gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00AF9A000000FFFF
    dq 0x00AF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
