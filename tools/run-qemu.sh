#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMG="${ROOT_DIR}/build/wevoa.img"

if [[ ! -f "${IMG}" ]]; then
  echo "Missing image: ${IMG}" >&2
  echo "Run make first." >&2
  exit 1
fi

exec qemu-system-x86_64 \
  -drive format=raw,file="${IMG}" \
  -m 128M \
  -serial stdio \
  -no-reboot \
  -no-shutdown

