# Input Overlay on Desktop

A standalone Windows desktop adaptation of [univrsal/input-overlay](https://github.com/univrsal/input-overlay).
This scaffold uses C++20, CMake, Win32 Raw Input, Direct2D and DirectWrite. OBS Studio
is neither needed nor linked. It is an early prototype, not a feature-complete port.

The compact vector HUD highlights physical Q/W/E, Shift/A/S/D, Ctrl and Space
keys, left/right/middle mouse regions and brief directional scroll indicators.
X1/X2 and cursor position remain in the input model but are not drawn. Raw Input continues to arrive
while another normal desktop application has focus. Input is processed in memory;
there is no input log, network listener or telemetry. Close the window to stop capture.

## Build on Windows

Prerequisites: Windows 10 version 1703 or newer / Windows 11, Visual Studio 2022 with **Desktop development with C++**
(MSVC v143 and a Windows 10/11 SDK), and CMake 3.25 or newer on PATH. Use an
x64 Native Tools Command Prompt or Developer PowerShell. No third-party packages,
OBS SDK, submodules or network downloads are required by CMake.

```powershell
git clone https://github.com/YiboMa2333/Input_Overlay_on_desktop.git
cd Input_Overlay_on_desktop
# For this scaffold, use the development branch once published:
git switch develop/standalone-win32
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\InputOverlayDesktop.exe
```

Run the automated state, geometry and settings tests (no third-party test framework):

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The state library and tests also build without Windows SDK dependencies on a
machine with CMake and a C++20 compiler:

```powershell
cmake -S . -B build/state -DOVERLAY_BUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build/state --config Release
ctest --test-dir build/state -C Release --output-on-failure
```

The native build and tests have also been run using portable
[LLVM-MinGW 20261104](https://github.com/mstorsjo/llvm-mingw/releases/tag/20261104)
(Clang 23.1.5) and CMake 4.5.0. With those tools extracted and CMake on PATH:

```powershell
$llvm = "C:/tools/llvm-mingw-20261104-ucrt-x86_64"
$env:PATH = "$llvm/bin;" + $env:PATH
cmake -S . -B build/llvm -G "MinGW Makefiles" `
  "-DCMAKE_CXX_COMPILER=$llvm/bin/clang++.exe" `
  "-DCMAKE_MAKE_PROGRAM=$llvm/bin/mingw32-make.exe" `
  -DCMAKE_BUILD_TYPE=Release "-DCMAKE_EXE_LINKER_FLAGS=-static"
cmake --build build/llvm -j 4
ctest --test-dir build/llvm --output-on-failure
.\build\llvm\InputOverlayDesktop.exe
```

Interactive desktop tests are opt-in: they open a temporary test backdrop and
move/click the pointer. They use an isolated settings directory and restart only
their own child application. Run while you are not interacting with the desktop:

```powershell
cmake -S . -B build -DOVERLAY_ENABLE_DESKTOP_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Use `-B build/llvm` for the portable build. See [validation details](docs/VALIDATION.md).

If working from the local checkout created during development, start directly
at the CMake commands. This branch is currently local; it has not been pushed.

Optional staging (not a source-compliant release archive by itself):

```powershell
cmake --install build --config Release --prefix build/package
```

For redistribution, include the matching complete source and build instructions
as described in [the licensing notes](docs/LICENSING.md).

## Settings and usage

The HUD starts **Locked**: always on top of ordinary windows, transparent, fully
click-through (including visible key/mouse shapes), with no editing controls and
no keyboard focus activation from ordinary clicks. Visualization remains active.

Use the system tray menu **Edit Overlay** or **Ctrl+Alt+F8** to edit. A selection
rectangle with eight handles surrounds the entire keyboard/mouse group. Drag
anywhere inside to move it. Drag a corner to resize proportionally; labels,
spacing, outlines and the mouse all scale together. Handles show directional
cursors and resizing displays logical width/height. Escape cancels a drag.

Aspect ratio is locked to the original HUD by default. Uncheck **Settings > Lock
aspect ratio** for single-axis edge resizing; corner resizing still preserves
the current proportions. Hold Shift to force the original aspect ratio. Lock
again from the tray or shortcut to remove all selection controls and pass clicks
through. Edit mode may take focus when clicked so drag capture works reliably;
locking restores the prior foreground application when it remains available.

The tray menu provides **Edit Overlay**, **Lock Overlay**, **Reset Position**,
**Settings**, and **Exit**. Reset returns the HUD to the primary work area at
its default size; it works even when the overlay is offscreen or locked. Explorer
restart is handled by recreating the tray icon. Startup fails visibly if the
recovery tray icon cannot be registered.

Keys use thin off-white outlines, dark translucent fills and a warm yellow
`#D8CF45` pressed state. The mouse uses matching vector outlines. Dark edging
keeps white strokes visible on light desktops. All highlights read live state.
See [HUD rendering notes and previews](docs/HUD_DESIGN.md).

Position, logical size and mode are saved after completed edits, mode changes,
recovery and shutdown to `%LOCALAPPDATA%\InputOverlayDesktop\settings.ini`.
Use **Settings > Open configuration** and then **Reload configuration** after
saving, or edit while the app is closed:

```ini
[window]
x=60
y=60
width=338
height=148
edit=0
aspect_lock=1
hotkey_enabled=1
hotkey_modifiers=3
hotkey_vk=119
```

Width accepts 169–2028 and height 74–888 logical pixels at 96 DPI. Invalid values
fall back to defaults. Screen x/y are signed physical coordinates, so monitors
left/above the primary are supported. For 150% custom HUD size use width=507,
height=222; Windows DPI scaling is applied in addition. Unlocked edge resizing
uses nonuniform vector scaling. The legacy `topmost` setting is ignored: both
modes stay above ordinary desktop windows.

The default shortcut uses modifier bitmask 3 (Ctrl=2, Alt=1, Shift=4) and key 119
(F8). Configuration accepts F1 through F11 with at least two Ctrl/Alt/Shift
modifiers; no bare gaming keys, Windows combinations or F12. Set
`hotkey_enabled=0` to disable it. Registration uses `MOD_NOREPEAT`; on conflict
the existing owner is retained, the tray reports shortcut unavailable, and tray
controls remain usable. A global shortcut reserves its configured combination;
choose another if your game/application uses it.

Partially offscreen positions are allowed while a useful portion remains
reachable. Restore and display/work-area changes validate against current monitor
work areas; inaccessible placements move onto the nearest available screen and
oversize geometry is reduced to fit. Reset Position is always available from the
tray. Mixed-DPI transitions end the current drag before applying the recommended
scaled bounds; start a new drag to continue on the new monitor.

Configuration write failures are reported in the tray tooltip/menu. No administrator rights are
required for ordinary desktop capture. Secure desktop, other sessions and
exclusive-input applications have not been validated.

## Architecture and upstream relationship

| Module | Responsibility |
| --- | --- |
| `src/input/raw_input.*` | Register keyboard/mouse Raw Input and normalize Windows events |
| `src/input/input_state.*` | Bounded held-state arrays, mouse position and wheel expiry |
| `src/render/renderer.*`, `hud_layout.hpp` | Vector HUD geometry, DirectWrite labels, alpha surface and visual state |
| `src/settings/settings.*` | Validated INI settings and position persistence |
| `src/desktop/window.*` | HWND lifecycle, message loop, topmost policy and refresh timer |
| `src/desktop/geometry.hpp` | Handle hit testing, anchored resizing and monitor recovery without Win32 dependencies |
| `src/main.cpp` | DPI setup and application entry point |

All state mutation and drawing happen on the window thread; capture does not call
the renderer. A 16 ms timer requests repaint; no background polling thread is needed.
Rendering recreates a lost Direct2D target. Capture unregisters on shutdown.
The adapter tracks physical scan-code locations, separately tracks each device,
and repairs missed releases. Shift and Ctrl combine left/right states visually.
See [INPUT_CAPTURE.md](docs/INPUT_CAPTURE.md) for interruption handling, privacy,
non-blocking registration and recovery limits.

`input_state.cpp` adapts upstream's `deps/common/input_data.cpp` event-to-state
dispatch logic. Its original copyright and GPL notice are retained. The remaining
modules are new Windows-specific scaffolding. Upstream's OBS graphics, Qt UI,
networking, libuiohook, SDL and preset assets are excluded. See the pinned
[repository and reuse audit](docs/UPSTREAM_AUDIT.md).

## Licence

The standalone application is **GPL-2.0-only**, not MIT-only. See [LICENSE](LICENSE),
[LICENSES](LICENSES) and [licensing obligations](docs/LICENSING.md). The MIT notice
from the original repository is preserved in `LICENSES/MIT.txt`; it does not
relicense the GPL adaptation. This is an independent adaptation, not an official
upstream release.

## Validation and remaining work

The Release application was built and launched on Windows using portable
LLVM-MinGW/CMake. State, placement, settings and real desktop integration tests
passed, including cross-process click-through, drag/resize, restart persistence,
hotkeys and recovery notifications. See [VALIDATION.md](docs/VALIDATION.md) for
the exact scope, tool versions and remaining hardware checks.

Remaining work includes configurable layouts, upstream preset
conversion with asset clearance and gamepad support. Physical monitor unplugging,
mixed-DPI monitor transitions, exclusive full-screen games and MSVC compilation
still require separate validation. Very fast
press/release pairs between paints may not be visible.
Horizontal wheel input is not implemented. Devices already held when capture
starts are not seeded into state. A device disconnect while held can leave stale
state if Windows fails to report both device removal and a usable asynchronous
release state.
