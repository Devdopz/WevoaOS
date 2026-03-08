#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
OBJ_DIR="${BUILD_DIR}/obj"

CC="${CC:-x86_64-elf-gcc}"
LD="${LD:-x86_64-elf-ld}"
OBJCOPY="${OBJCOPY:-x86_64-elf-objcopy}"
NASM="${NASM:-nasm}"

STAGE2_SECTORS=32
KERNEL_MAX_SECTORS=512
IMAGE_SIZE_MB=64

mkdir -p "${OBJ_DIR}"
mkdir -p "${BUILD_DIR}/boot/stage1" "${BUILD_DIR}/boot/stage2" "${BUILD_DIR}/kernel"

KERNEL_CFLAGS=(
  -ffreestanding
  -fno-builtin
  -fno-stack-protector
  -fno-pic
  -m64
  -mno-red-zone
  -mgeneral-regs-only
  -mcmodel=kernel
  -Wall
  -Wextra
  -Werror
  -std=c11
  -I"${ROOT_DIR}/kernel/include"
)

KERNEL_SOURCES=(
  "${ROOT_DIR}/kernel/arch/x86_64/idt.c"
  "${ROOT_DIR}/kernel/dev/ata.c"
  "${ROOT_DIR}/kernel/dev/console.c"
  "${ROOT_DIR}/kernel/dev/keyboard.c"
  "${ROOT_DIR}/kernel/mm/pmm.c"
  "${ROOT_DIR}/kernel/mm/kheap.c"
  "${ROOT_DIR}/kernel/fs/fs.c"
  "${ROOT_DIR}/kernel/storage/pstore.c"
  "${ROOT_DIR}/kernel/net/net.c"
  "${ROOT_DIR}/kernel/gui/ui_compositor.c"
  "${ROOT_DIR}/kernel/gui/ui_shell.c"
  "${ROOT_DIR}/kernel/gui/ui_style.c"
  "${ROOT_DIR}/kernel/gui/ui_icons.c"
  "${ROOT_DIR}/kernel/gui/gui.c"
  "${ROOT_DIR}/kernel/lib/string.c"
  "${ROOT_DIR}/kernel/lib/print.c"
  "${ROOT_DIR}/kernel/core/config.c"
  "${ROOT_DIR}/kernel/core/panic.c"
  "${ROOT_DIR}/kernel/core/wstatus.c"
  "${ROOT_DIR}/kernel/core/main.c"
  "${ROOT_DIR}/kernel/shell/shell.c"
)

KERNEL_OBJECTS=()
for src in "${KERNEL_SOURCES[@]}"; do
  obj="${OBJ_DIR}/$(basename "${src%.c}").o"
  "${CC}" "${KERNEL_CFLAGS[@]}" -c "${src}" -o "${obj}"
  KERNEL_OBJECTS+=("${obj}")
done

"${NASM}" -f elf64 "${ROOT_DIR}/kernel/arch/x86_64/entry.asm" -o "${OBJ_DIR}/entry.o"
"${NASM}" -f elf64 "${ROOT_DIR}/kernel/arch/x86_64/idt.asm" -o "${OBJ_DIR}/idt_asm.o"
KERNEL_OBJECTS+=("${OBJ_DIR}/entry.o" "${OBJ_DIR}/idt_asm.o")

"${LD}" -nostdlib -z max-page-size=0x1000 -T "${ROOT_DIR}/kernel/linker.ld" -o "${BUILD_DIR}/kernel/kernel.elf" "${KERNEL_OBJECTS[@]}"
"${OBJCOPY}" -O binary "${BUILD_DIR}/kernel/kernel.elf" "${BUILD_DIR}/kernel/kernel.bin"

kernel_size_bytes="$(stat -c%s "${BUILD_DIR}/kernel/kernel.bin")"
kernel_sectors="$(( (kernel_size_bytes + 511) / 512 ))"
if (( kernel_sectors == 0 )); then
  echo "Kernel binary is empty" >&2
  exit 1
fi
if (( kernel_sectors > KERNEL_MAX_SECTORS )); then
  echo "Kernel is too large (${kernel_sectors} sectors > ${KERNEL_MAX_SECTORS})" >&2
  exit 1
fi

"${NASM}" -f bin "${ROOT_DIR}/boot/stage2/stage2.asm" \
  -D STAGE2_SECTORS="${STAGE2_SECTORS}" \
  -D KERNEL_SECTORS="${kernel_sectors}" \
  -o "${BUILD_DIR}/boot/stage2/stage2.bin"

stage2_size_bytes="$(stat -c%s "${BUILD_DIR}/boot/stage2/stage2.bin")"
stage2_max_bytes="$(( STAGE2_SECTORS * 512 ))"
if (( stage2_size_bytes > stage2_max_bytes )); then
  echo "Stage2 is too large (${stage2_size_bytes} bytes > ${stage2_max_bytes})" >&2
  exit 1
fi

"${NASM}" -f bin "${ROOT_DIR}/boot/stage1/stage1.asm" \
  -D STAGE2_SECTORS="${STAGE2_SECTORS}" \
  -o "${BUILD_DIR}/boot/stage1/stage1.bin"

stage1_size_bytes="$(stat -c%s "${BUILD_DIR}/boot/stage1/stage1.bin")"
if (( stage1_size_bytes != 512 )); then
  echo "Stage1 must be exactly 512 bytes, got ${stage1_size_bytes}" >&2
  exit 1
fi

truncate -s "${IMAGE_SIZE_MB}M" "${BUILD_DIR}/wevoa.img"
dd if="${BUILD_DIR}/boot/stage1/stage1.bin" of="${BUILD_DIR}/wevoa.img" bs=512 seek=0 conv=notrunc status=none
dd if="${BUILD_DIR}/boot/stage2/stage2.bin" of="${BUILD_DIR}/wevoa.img" bs=512 seek=1 conv=notrunc status=none
dd if="${BUILD_DIR}/kernel/kernel.bin" of="${BUILD_DIR}/wevoa.img" bs=512 seek="$((1 + STAGE2_SECTORS))" conv=notrunc status=none

printf "Built %s\n" "${BUILD_DIR}/wevoa.img"
printf "Stage2 sectors: %d\n" "${STAGE2_SECTORS}"
printf "Kernel sectors: %d\n" "${kernel_sectors}"
