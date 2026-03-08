# Wevoa Step-by-Step Upgrade Plan

This plan follows your requested structure and focuses on practical incremental upgrades.

## Step 1 (Done): Terminal Stability + Base UX
Implemented:
- Fixed terminal command crash path (SSE-disabled kernel C build via `-mgeneral-regs-only`)
- Added friendlier terminal prompt and command aliases
- Added Wevoa custom commands: `WEVOA`, `WVER`, `WHOAMI`, `WALL`, `DESKLS`, `DESKDIR`, `DESKFILE`

Result:
- Terminal no longer triggers VM triple-fault when executing commands.

## Step 2: Split GUI Monolith Into Modules
Goal:
- Separate `gui.c` into:
  - `ui/` (windows, drawing, menu)
  - `term/` (command parser/executor)
  - `vfs/` (in-memory file model)
  - `input/` (keyboard/mouse dispatch)

Why:
- Better maintainability and safer feature growth.

## Step 3 (Done): Strong Error Handling Layer
Goal:
- Introduce consistent error model:
  - `W_OK`, `W_ERR_NOT_FOUND`, `W_ERR_EXISTS`, `W_ERR_INVALID_ARG`, `W_ERR_NO_SPACE`
- Convert terminal and desktop actions to return/status codes.
- Render user-facing errors as readable messages.

Implemented:
- Added `wstatus` API and normalized `ERROR <STATUS>` terminal output
- Converted terminal command path to status-based handling

## Step 4 (Done): Persistent Data Storage (First Real Persistence)
Goal:
- Persist settings + small file table across reboot.

Phase A:
- Write/read fixed metadata block near end of disk image.

Phase B:
- Replace fixed block with first `WevoaFS v1` superblock + entries.

Implemented:
- Added ATA PIO disk driver
- Added `pstore v1` fixed persistence block with checksum
- Added `WSAVE`, `WLOAD`, `WRESET`
- Added `config` model and `WGET`, `WSET`, `WLIST`

## Step 5: Terminal UX Upgrade
Goal:
- Improve terminal to feel closer to Linux/PowerShell usability:
  - command history navigation (up/down)
  - tab completion
  - command usage hints
  - consistent prompt and path behavior

## Step 6: Settings Upgrade
Goal:
- Add user configuration options:
  - cursor speed
  - key repeat delay/rate
  - default wallpaper/theme
- Store all in persistent config file/record.

## Step 7: Performance Path
Goal:
- Reduce render/input latency and stutter:
  - dirty-region redraw
  - frame pacing tick
  - optional mouse event buffering

## Step 8: Window/Desktop Usability
Goal:
- Make desktop interaction more production-like:
  - rename file/folder
  - delete item
  - open file viewer app window
  - improved context menus

## Release Labels
- `v0.3.0-alpha`: stable terminal baseline + custom commands + no command crash
- `v0.3.1-alpha`: module split start
- `v0.4.0-alpha`: first persistence layer
- `v0.5.0-alpha`: upgraded terminal UX + settings persistence
