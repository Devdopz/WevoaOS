# Wevoa OS

Wevoa is a custom x86_64 desktop OS project built from scratch.
Current product direction: cybersecurity operations, secure development workflows, and server-first tooling.

Current implemented baseline:
- Custom BIOS MBR stage1 bootloader (`boot/stage1`).
- Custom stage2 loader (`boot/stage2`) that enters long mode.
- Freestanding 64-bit kernel (`kernel/`).
- VGA text console and interactive shell fallback.
- Graphics mode boot with custom VBE linear framebuffer driver path (high-resolution truecolor, VGA fallback).
- Raw disk image build pipeline (`build/wevoa.img`).

## Project Principles
- No Linux/BSD kernel/boot/filesystem code reuse.
- Freestanding C + NASM.
- BIOS + MBR first, ISO support later.
- VirtualBox as primary VM target.
- Security-first and ethical-use-first design (authorized testing and defense only).

## Build Prerequisites (WSL/Linux)
- `x86_64-elf-gcc`
- `x86_64-elf-ld`
- `x86_64-elf-objcopy`
- `nasm`
- `bash`, `dd`, `truncate`, `sha256sum`

See setup details in `docs/SETUP.md`.

## Commands
- Build image: `make`
- Build image (Windows-native, no WSL): `powershell -ExecutionPolicy Bypass -File .\tools\build-image.ps1`
- Build ISO (Windows-native): `powershell -ExecutionPolicy Bypass -File .\tools\build-iso.ps1`
- Build from Windows via WSL helper: `tools/build-wsl.ps1`
- Run in QEMU (optional): `make run-qemu`
- Hash determinism check: `make hash-check`
- VirtualBox helper (PowerShell, persistent disk default): `tools/run-virtualbox.ps1`
- Rebuild VirtualBox disk from latest `build/wevoa.img`: `tools/run-virtualbox.ps1 -RecreateDisk`
- VirtualBox ISO boot helper: `tools/run-virtualbox-iso.ps1`
- ISO helper (bash): `tools/build-iso.sh`

## GUI Controls (Current Alpha)
- Move cursor: mouse or `W/A/S/D`
- Faster cursor move: hold `Shift`
- Select/open icon under cursor: left-click or `Enter`
- Open Settings quickly: click the Settings button on the right side of the taskbar
- Open apps quickly: `1` = Lab Files, `2` = Secure Shell, `3` = Ops Center, `4` = Wevoa Code, `5` = Port Scan, `6` = Log Viewer, `7` = Packet Analyzer, `8` = Web Recon
- Right-click desktop free space: opens menu with `Refresh`, `New Folder`, `New File`
- In Settings window: `F` toggle Wi-Fi, `C` connect/disconnect mock network, `B/N` brightness down/up, `M` cycle wallpaper
- In Settings window: `U/O/P` scale down/up/auto-fit (display sizing), `L` lock toggle, `H/J` lock PIN down/up, `R` lock now
- In Settings window (Network Advanced card): click `UP/OFF`, `DEV/OAI` DNS test buttons, and `80/443` firewall toggles
- Wevoa Code app: type directly, `Backspace` delete, `Enter` new line, `Tab` inserts indentation, use `LOAD/SAVE/NEW` buttons
- Coding keyboard support: lowercase letters + full code punctuation (`() [] {} <> ; : ' " , . / \\ | = + - _ ! @ # $ % ^ & * ? ~`)
- Calculator app: use on-screen buttons or keyboard (`0-9`, `-`, `/`, `x`, `=`, `Enter`, `C`)
- Image Viewer app: opens sample desktop image panel
- Video Player app: demo playback with `PLAY`, `PAUSE`, `STOP` controls
- Browser app: URL bar + `GO/HOME/REF` navigation with DNS/firewall-backed website preview (`WEB DEVDOPZ.COM`; `https://` auto-added)
- Lock screen: appears at boot when lock is enabled; enter PIN and press `Enter`
- Cycle focused window: `Tab`
- Drag focused window from title bar: hold left-click (or `Space`) while moving cursor
- Nudge focused window directly: `I/J/K/L`
- Window buttons: title bar has `Minimize`, `Maximize/Restore`, and `Close` (mouse-clickable)
- Close focused window: `Q`
- Reset layout: `Esc`

### Terminal Commands (GUI Terminal)
- `HELP`
- `LS` or `DIR`
- `PWD`
- `CD /`, `CD ..`, `CD <DIR>`
- `MKDIR <NAME>` (aliases: `MKIDR`, `MD`)
- `TOUCH <NAME>`
- `CAT <FILE>` or `TYPE <FILE>`
- `WRITE <FILE> <TEXT>`
- `EDIT <FILE> <TEXT>`
- `APPEND <FILE> <TEXT>`
- `COPY <NAME>` or `CP <NAME>`
- `CUT <NAME>` or `MV <NAME>`
- `PASTE [NEWNAME]`
- `DEL <NAME>` (aliases: `DELETE`, `RM`) -> moves item to `WASTEBIN`
- `BIN` or `WASTEBIN`
- `RESTORE <NAME> [NEWNAME]`
- `EMPTYBIN`
- `ECHO <TEXT>`
- `CLEAR` or `CLS`
- `WEVOA` (about/version)
- `WVER`
- `WHOAMI`
- `WALL` (next wallpaper)
- `CODE`, `NOTE`, `SCAN`, `LOGS`, `PACKET`
- `RECON`, `BROWSER`, `WEB [URL]` (`WEB DEVDOPZ.COM` works, scheme optional)
- `DNS <HOST>`
- `PORT <PORT> ALLOW|BLOCK|STATUS`
- `NETUP`, `NETDOWN`
- `LOCK`, `LOCK ON`, `LOCK OFF`
- `WGET` or `WGET <KEY>`
- `WGET RES` (show detected framebuffer resolution)
- `WSET <KEY> <VALUE>`
- `WLIST`
- `WSAVE`, `WLOAD`, `WRESET`
- `DESKLS`
- `DESKDIR <NAME>`
- `DESKFILE <NAME>`

Config keys:
- `BRIGHT`, `WALL`, `WIFI`, `NET`, `CURSOR_SPEED`, `KEY_REPEAT_DELAY`, `KEY_REPEAT_RATE`, `LOCK`, `LOCK_PIN`, `SHELL_LAYOUT`, `UI_BLUR_LEVEL`, `UI_MOTION_LEVEL`, `UI_EFFECTS_QUALITY`, `SCALE`

`CURSOR_SPEED` accepts DPI-like values in terminal commands (for example `WSET CURSOR_SPEED 1600`).

## UI Core Library (New)
Wevoa now has a native reusable UI core (instead of hardcoding everything in one file):
- Tokens: `kernel/include/wevoa/ui_tokens.h`
- Style system: `kernel/include/wevoa/ui_style.h` + `kernel/gui/ui_style.c`
- Icon pack: `kernel/include/wevoa/ui_icons.h` + `kernel/gui/ui_icons.c`

This is the OS-native equivalent of a utility-style system: centralized colors, shared panel styles, and reusable icon drawing.

## VirtualBox Note
If you see a Guru Meditation / triple-fault on boot, ensure the VM is 64-bit with:
- `PAE/NX = ON`
- `Long Mode = ON`

The provided `run-virtualbox*.ps1` scripts now enforce these flags automatically.
For full-screen VM view, use VirtualBox Host key + `F`.
`tools/run-virtualbox*.ps1` now registers host-sized and widescreen custom VBE modes, and Wevoa auto-selects the best available one (up to `2560x1600`).
If the VM still shows the old size, rebuild and run with `tools/run-virtualbox.ps1 -RecreateDisk`, then reopen in full-screen (`Host+F`).

## Status
Current baseline: `v0.4.0-alpha` development state.

Architecture tracking:
- `docs/ARCHITECTURE_STATUS.md`
- `docs/UPGRADE_PLAN.md`

## Known Limitations (Current Alpha)
- GUI is software-rendered on top of VBE linear framebuffer (no hardware acceleration yet).
- Cursor and window movement can feel low-FPS in VirtualBox due to full-frame CPU blits.
- No hardware acceleration, no dirty-rectangle compositor, no PIT-vsynced frame pacing yet.
- Persistence in this milestone is fixed-block `pstore v1`, not full WevoaFS yet.
- Current desktop tools are still early mock implementations, not production-grade security modules.

## Next Performance Work
- Add PIT-based frame tick and target stable frame pacing.
- Switch to dirty-region redraw instead of full-screen copy every frame.
- Use direct mouse IRQ path and event queue instead of polling loop.
- Add lightweight profiling counters for frame time and input latency.
