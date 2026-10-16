# discord-overlay-input-hijack

A minimal C++ DLL that intercepts input routed through Discord's overlay trap window using Win32 window subclassing. Gives you a clean hook point to inspect, modify, or suppress any keyboard or mouse message before Discord's overlay handler processes it.

---

## Background

When Discord's in-game overlay is active, it creates a hidden top-level window with the class name `DiscordDesktopOverlayInputTrap`. All input intended for the overlay — key presses, raw input, mouse events — arrives at that window's message procedure first. Replacing that procedure with `SetWindowLongPtrW` from inside the same process is the standard Win32 subclassing pattern and is sufficient to sit in front of every message Discord will ever receive while the overlay is open.

---

## How it works

**On DLL attach** (`DllMain` → `DLL_PROCESS_ATTACH`):

- `DisableThreadLibraryCalls` is called to skip unnecessary thread notifications.
- A background thread (`InitThread`) is created immediately.
- `InitThread` calls `InstallOverlayHijack()` in a loop, once every 500 ms for up to 60 seconds, because Discord creates the trap window lazily after the overlay renderer initializes. The retry loop means you can inject at any point and it will self-install when the window appears.

**`InstallOverlayHijack`**:

- Calls `EnumWindows` with a callback that checks every top-level window's class name against `DISCORD_TRAP_CLASS` (`L"DiscordDesktopOverlayInputTrap"`).
- When found, calls `SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)HijackWndProc)` to swap the window procedure, storing the original pointer in `g_origWndProc`.

**`HijackWndProc`**:

- Called by Windows for every message sent to the trap window.
- Passes the message to `OnInput`. If `OnInput` returns `TRUE`, the message is swallowed (returns 0 to Windows, Discord never sees it). If it returns `FALSE`, `CallWindowProcW` forwards the message to Discord's original handler so overlay behavior is preserved.

**`OnInput`** (the part you edit):

- A labeled skeleton covering the relevant message types:
  - `WM_KEYDOWN` / `WM_SYSKEYDOWN` — `wParam` is the virtual key code. The F8 case is included as a working example of a swallow. Replace or extend it.
  - `WM_INPUT` — Reads the `RAWINPUT` struct via `GetRawInputData`. Splits into `RIM_TYPEKEYBOARD` (virtual key in `data.keyboard.VKey`) and `RIM_TYPEMOUSE` (deltas and button flags in `data.mouse`). Discord reads keystroke state through raw input when the overlay is active, so this path matters.
  - `WM_MOUSEMOVE`, `WM_LBUTTONDOWN/UP`, `WM_RBUTTONDOWN/UP`, `WM_MBUTTONDOWN/UP`, `WM_MOUSEWHEEL` — stubs showing where cursor position and button state are available.
- Returns `FALSE` by default (pass-through). Only return `TRUE` when you want Discord to never see the message.

**On DLL detach** (`DLL_PROCESS_DETACH`):

- `RemoveOverlayHijack` calls `SetWindowLongPtrW` again to restore `g_origWndProc`, then clears both globals.

---

## Files

| File | Description |
|---|---|
| `dllmain.cpp` | Entry point — attach/detach routing, retry thread |
| `overlay_hijack.cpp` | Window search, subclass install/remove, `HijackWndProc`, `OnInput` skeleton |
| `overlay_hijack.h` | Shared types, `DISCORD_TRAP_CLASS` constant, function declarations |

---

## Requirements

- Windows 10/11 x64
- MSVC with C++17 or later
- No third-party libraries

---

## Build

**Command line (MSVC):**

```
cl /std:c++17 /O2 /LD dllmain.cpp overlay_hijack.cpp /link user32.lib kernel32.lib /Fe:overlay.dll
```

**Visual Studio:**

Create a new DLL project, add the three source files, set the platform to x64, and build. No additional settings required.

The output must be x64 to match the target process.

---

## Usage

1. Build `overlay.dll`.
2. Inject the DLL into the process that owns the `DiscordDesktopOverlayInputTrap` window using any standard injection method (LoadLibrary-based, manual map, etc.).
3. The retry thread handles timing automatically. There is no need to wait for the overlay to be visible before injecting.
4. Open `overlay_hijack.cpp` and fill in `OnInput`. Return `TRUE` to eat a message, `FALSE` to let Discord handle it normally.

If Discord renames the trap window class in a future update, change `DISCORD_TRAP_CLASS` in `overlay_hijack.h` and rebuild.

---

## Owner

GitHub: [@dwcmethod](https://github.com/dwcmethod)

---

## Disclaimer

This repository is published for **educational and research purposes only**. It demonstrates standard Win32 window subclassing and input interception techniques at the API level. The author — [@dwcmethod](https://github.com/dwcmethod) — accepts **no responsibility** for any use of this code outside of personal research and study. Using this software in a manner that violates any application's Terms of Service, End User License Agreement, or applicable law is solely the responsibility of the person doing so. The author provides no warranty of any kind and is not liable for any damages, account actions, or legal consequences arising from the use or misuse of this project. By cloning, compiling, or running any part of this code you acknowledge that you bear full and sole responsibility for how it is used.
