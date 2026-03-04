CC ?= x86_64-elf-gcc
LD ?= x86_64-elf-ld
OBJCOPY ?= x86_64-elf-objcopy
NASM ?= nasm

.PHONY: all clean run-qemu hash-check

all:
	@CC="$(CC)" LD="$(LD)" OBJCOPY="$(OBJCOPY)" NASM="$(NASM)" bash ./tools/build-image.sh

run-qemu: all
	@bash ./tools/run-qemu.sh

hash-check:
	@CC="$(CC)" LD="$(LD)" OBJCOPY="$(OBJCOPY)" NASM="$(NASM)" bash ./tools/hash-check.sh

clean:
	@rm -rf build
