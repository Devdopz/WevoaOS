#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
IMG="${BUILD_DIR}/wevoa.img"
ISO="${BUILD_DIR}/wevoa.iso"
ISO_ROOT="${BUILD_DIR}/iso-root"
BOOT_IMG="${ISO_ROOT}/boot/wevoa.img"

if [[ ! -f "${IMG}" ]]; then
  CC="${CC:-x86_64-elf-gcc}" LD="${LD:-x86_64-elf-ld}" OBJCOPY="${OBJCOPY:-x86_64-elf-objcopy}" NASM="${NASM:-nasm}" \
    bash "${ROOT_DIR}/tools/build-image.sh"
fi

if ! command -v xorriso >/dev/null 2>&1; then
  echo "xorriso not found. Install xorriso first." >&2
  exit 1
fi

rm -rf "${ISO_ROOT}"
mkdir -p "${ISO_ROOT}/boot"
cp "${IMG}" "${BOOT_IMG}"
cat > "${ISO_ROOT}/README.TXT" <<'EOF'
Wevoa boot ISO.
Boot image: boot/wevoa.img
EOF

xorriso -as mkisofs \
  -V WEVOA_001 \
  -iso-level 3 \
  -R \
  -J \
  -b boot/wevoa.img \
  -hard-disk-boot \
  -o "${ISO}" \
  "${ISO_ROOT}"

echo "Built ${ISO}"
