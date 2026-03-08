# Wevoa Setup (Recommended)

## 1. Install WSL Ubuntu
Use an Ubuntu distro (24.04 LTS recommended), not `docker-desktop`.

## 2. Install build dependencies
```bash
sudo apt update
sudo apt install -y build-essential nasm xorriso qemu-system-x86
```

## 3. Install x86_64-elf cross toolchain
You can either:
1. Build `x86_64-elf-gcc` and `x86_64-elf-binutils` manually, or
2. Use a known package/source if available in your environment.

Required binaries in `PATH`:
- `x86_64-elf-gcc`
- `x86_64-elf-ld`
- `x86_64-elf-objcopy`
- `nasm`

## 4. Build Wevoa
```bash
make
```

Output:
- `build/wevoa.img`

## 5. Run Wevoa
Optional QEMU:
```bash
make run-qemu
```

VirtualBox (Windows host):
```powershell
.\tools\run-virtualbox.ps1
```

## Windows-native build option (no WSL)
If you prefer not to use Ubuntu/WSL, use:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build-image.ps1
```

Required tools in Windows:
- `gcc`, `ld`, `objcopy` (MSYS2 UCRT64 is supported)
- `nasm`
