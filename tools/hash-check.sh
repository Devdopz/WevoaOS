#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

CC="${CC:-x86_64-elf-gcc}"
LD="${LD:-x86_64-elf-ld}"
OBJCOPY="${OBJCOPY:-x86_64-elf-objcopy}"
NASM="${NASM:-nasm}"

rm -rf "${BUILD_DIR}"
CC="${CC}" LD="${LD}" OBJCOPY="${OBJCOPY}" NASM="${NASM}" bash "${ROOT_DIR}/tools/build-image.sh"
hash1="$(sha256sum "${BUILD_DIR}/wevoa.img" | awk '{print $1}')"

CC="${CC}" LD="${LD}" OBJCOPY="${OBJCOPY}" NASM="${NASM}" bash "${ROOT_DIR}/tools/build-image.sh"
hash2="$(sha256sum "${BUILD_DIR}/wevoa.img" | awk '{print $1}')"

if [[ "${hash1}" != "${hash2}" ]]; then
  echo "Hash mismatch: ${hash1} != ${hash2}" >&2
  exit 1
fi

echo "Deterministic build hash: ${hash1}"
