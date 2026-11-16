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

Run the deterministic input-state tests (no third-party test framework):

```powershell
cmake --build build --config Release --target input_state_tests
ctest --test-dir build -C Release --output-on-failure
```

The state library and tests also build without Windows SDK dependencies on a
machine with CMake and a C++20 compiler:

```powershell
cmake -S . -B build/state -DOVERLAY_BUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build/state --config Release
ctest --test-dir build/state -C Release --output-on-failure
```

If working from the local checkout created during development, start directly
at the CMake commands. This branch is currently local; it has not been pushed.

Optional staging (not a source-compliant release archive by itself):

```powershell
cmake --install build --config Release --prefix build/package
```

For redistribution, include the matching complete source and build instructions
as described in [the licensing notes](docs/LICENSING.md).

## Settings and usage

Drag a visible key or the mouse body to move the borderless HUD. Right-click a
visible element and select **Close overlay** to exit. The window is always on
top by default and does not activate on ordinary clicks or input events. It has
no title bar or large background panel. Empty areas between elements are fully
transparent and pass clicks to windows beneath; the visible shapes remain
interactive for moving and closing the HUD.

Keys use thin off-white outlines, dark translucent fills and a warm yellow
`#D8CF45` pressed state. The mouse uses matching vector outlines. Dark edging
keeps white strokes visible on light desktops. All highlights read live state.
See [HUD rendering notes and previews](docs/HUD_DESIGN.md).

Window position is saved on close in
`%LOCALAPPDATA%\InputOverlayDesktop\settings.ini`. To change the defaults, create
or edit the file while the app is closed:

```ini
[window]
x=60
y=60
width=338
height=148
topmost=1
```

Width is clamped to 254–1352 and height to 111–592 logical pixels at 96 DPI.
The HUD preserves its aspect ratio inside that canvas, leaving any extra space
transparent. For 150% custom HUD size, use width=507 and height=222. Windows DPI
scaling is applied in addition; moving between monitors follows `WM_DPICHANGED`.
Existing INI sizes are respected: reset width/height to the example for the new
compact appearance. An entirely off-screen saved position falls back to (60,60).
If settings cannot be saved, defaults still work. No administrator rights are
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

No successful configure, compile or GUI launch has been performed in the current
development environment: CMake and a usable MSVC/Windows SDK were not found.
The launch acceptance criterion remains unverified. Follow
[VALIDATION.md](docs/VALIDATION.md) on a Windows development machine.

Remaining work includes a full click-through mode with a recovery
control, tray UI, configurable layouts, upstream preset
conversion with asset clearance and gamepad support. Automated state tests are
included but have not yet been executed in this environment. Very fast
press/release pairs between paints may not be visible.
Horizontal wheel input is not implemented. Devices already held when capture
starts are not seeded into state. A device disconnect while held can leave stale
state if Windows fails to report both device removal and a usable asynchronous
release state. Settings write failures are currently silent.
