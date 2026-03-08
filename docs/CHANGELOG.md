# Wevoa Changelog

## v0.4.0-alpha (unreleased)
- Added `wstatus` foundation and normalized terminal result/error output (`OK ...` / `ERROR ...`).
- Added persistent storage layer:
  - ATA PIO driver (`kernel/dev/ata.c`)
  - persistent store block (`kernel/storage/pstore.c`)
  - save/load/reset commands: `WSAVE`, `WLOAD`, `WRESET`
- Added centralized runtime configuration model:
  - config API (`kernel/core/config.c`)
  - terminal settings commands: `WGET`, `WSET`, `WLIST`
- Added Wevoa UI core modules:
  - reusable style system (`ui_style`)
  - reusable icon pack (`ui_icons`)
  - shared design tokens (`ui_tokens`)
- Expanded desktop/apps:
  - notepad/code editor app
  - calculator app
  - image viewer and video player mock apps
  - browser/recon preview app with DNS/port/firewall-backed network simulation
  - lock screen and PIN controls
- Improved display and VM runtime path:
  - higher resolution VBE mode selection in stage2
  - improved VirtualBox runner defaults and custom video mode registration
  - updated build pipeline for new kernel subsystems
- Keyboard and coding input upgrade:
  - lowercase typing support
  - shift-aware symbols for coding (`() [] {} <> ; : ' " , . / \\ | = + - _ ! @ # $ % ^ & * ? ~`)

## v0.3.1-alpha (unreleased)
- Added first structured settings/config terminal commands:
  - `WGET`
  - `WSET BRIGHT <10-100>`
  - `WSET WALL <0-2>`
  - `WSET WIFI ON|OFF`
- Improved shell help output grouping and discoverability.
- Added clearer command error/ok messages for config operations.

## v0.3.0-alpha (unreleased)
- Fixed terminal command execution crash path in GUI mode by building kernel C code with `-mgeneral-regs-only` (prevents unintended SSE usage before SSE state management is implemented).
- Added richer GUI terminal command set and aliases:
  - `DIR`, `MD`, `TYPE`, `CLS`
  - `WEVOA`, `WVER`, `WHOAMI`
  - `DESKLS`, `DESKDIR`, `DESKFILE`, `WALL`
- Improved terminal prompt style and startup hints.
- Added in-memory desktop item integration with right-click menu actions:
  - `Refresh`
  - `New Folder`
  - `New File`
- Added architecture status and step-by-step upgrade planning docs:
  - `docs/ARCHITECTURE_STATUS.md`
  - `docs/UPGRADE_PLAN.md`

## v0.2.2-alpha (unreleased)
- Added custom VBE linear framebuffer boot path in stage2 (tries high-res truecolor modes first).
- Added framebuffer metadata handoff for VBE mode, pitch, bpp, and physical address.
- Expanded early paging setup to map the first 4GB with 2MB pages, enabling high physical framebuffer access.
- Updated GUI renderer to support 8bpp (VGA) and 24/32bpp (VBE) output paths.
- Increased GUI canvas limits and adaptive default window sizing for higher-resolution desktops.

## v0.2.1-alpha (unreleased)
- GUI render loop switched to event-driven redraw path.
- Removed fixed `rdtsc` frame delay in GUI loop to reduce VM-dependent stutter.
- Added idle pause path when no visual state changes are detected.
- Result: lower flicker perception and improved cursor/window responsiveness in VirtualBox.

## v0.2.0-alpha (unreleased)
- Added custom BIOS boot path (stage1 + stage2) to long mode kernel entry.
- Added bootable raw image + ISO build pipeline for Windows PowerShell and bash.
- Added GUI alpha desktop in VGA mode `320x200x8`.
- Added sample desktop icons and sample windows:
  - File Manager (mock)
  - Terminal (mock)
  - Settings (mock)
- Added keyboard controls for pointer/navigation/window actions.
- Added PS/2 mouse initialization and software cursor rendering.
- Improved VirtualBox ISO runner:
  - Enforces 64-bit flags.
  - Handles saved-state VMs by discarding state before boot.

### Known issues
- GUI frame pacing and cursor movement may feel low-FPS in VirtualBox.
- GUI uses full-frame software blits; optimization work is planned next.
