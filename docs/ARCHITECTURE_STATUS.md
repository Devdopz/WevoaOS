# Wevoa Software Architecture Status

This tracks the current state of Wevoa against your 8-part software structure.

## 1. Program Logic
Status: `implemented (alpha)`

What exists now:
- GUI desktop/window management logic
- Terminal command parsing/execution
- In-memory VFS node logic (`DIR`/`FILE`)
- Settings toggles (Wi-Fi mock, brightness, wallpaper)

Gaps:
- Strong module boundaries (logic still concentrated in `kernel/gui/gui.c`)
- Formal internal APIs between UI, terminal, and storage

## 2. User Interface (UI)
Status: `implemented (alpha)`

What exists now:
- Desktop/taskbar/start area
- App windows (File Manager, Terminal, Settings)
- Window controls (minimize/maximize/close)
- Right-click desktop menu (`Refresh`, `New Folder`, `New File`)

Gaps:
- Rich text rendering
- UI scaling/layout system
- Widget library reuse

## 3. Input System
Status: `implemented (alpha)`

What exists now:
- PS/2 keyboard input
- PS/2 mouse input
- Click + right-click handling
- Keyboard shortcuts

Gaps:
- Dedicated input event queue
- Repeat-rate and keymap profiles
- Better focus model for text input widgets

## 4. Output System
Status: `implemented (alpha)`

What exists now:
- Software-rendered framebuffer output
- Text and icon rendering
- Terminal output history

Gaps:
- Partial redraw (dirty rectangles)
- Higher quality font/text
- Notification subsystem

## 5. Data Storage
Status: `implemented (v1)`

What exists now:
- ATA PIO disk read/write driver (`kernel/dev/ata.c`)
- Persistent store block (`pstore v1`) in reserved end-of-disk region
- Save/load/reset for settings + in-memory VFS state across reboot (`WSAVE`, `WLOAD`, `WRESET`)

Gaps:
- Persistent disk filesystem
- Journaling/recovery beyond checksum fallback

## 6. Processing Engine
Status: `implemented (alpha)`

Flow exists:
- Input -> command parsing -> logic -> render output

Gaps:
- Task/job queue
- Performance profiling hooks per subsystem

## 7. Error Handling
Status: `implemented (v1)`

What exists now:
- Structured status model (`W_OK`, `W_ERR_*`) in `wstatus`
- Normalized terminal error output format (`ERROR <STATUS>`)
- Panic-level crash handling in kernel paths

Gaps:
- Recoverable error flows in GUI commands
- Richer context/error codes for advanced subsystems

## 8. Settings / Configuration
Status: `implemented (v1)`

What exists now:
- Runtime settings panel (Wi-Fi mock, link mock, brightness, wallpaper)
- Central config model/API (`config_get`, `config_set`, `config_apply`)
- Terminal config commands (`WGET`, `WSET`, `WLIST`)
- Persistent settings via `pstore`

Gaps:
- More categories (input speed, theme variants, language)

## Summary
- Strong already: `Program Logic`, `UI`, `Input`, `Output`, `Processing Engine`
- Next priority: `Input queue`, `output notifications/dirty rects`, `module split`
