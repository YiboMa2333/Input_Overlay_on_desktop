# Repository and reuse audit

Inspected 2026-11-16. These are source observations, not a blanket clearance of
every upstream file or asset.

## Starting repository

- Repository: https://github.com/YiboMa2333/Input_Overlay_on_desktop
- Default branch: `main` (local `origin/HEAD`).
- Starting commit: `6b544c859f577bfed8bd3c064e394a6c50ec8d11` (`Initial commit`).
- The tree contained only `LICENSE`: MIT, Copyright (c) 2026 Yibo Ma.
- No source, build system or README existed. Development branch:
  `develop/standalone-win32`.

## Upstream source snapshot

Repository: https://github.com/univrsal/input-overlay

Inspected `master` at **7d97ae9e66d9e4dd968f759fcafd447969510f23**.
All paths below refer to that commit, not a floating latest release:
https://github.com/univrsal/input-overlay/tree/7d97ae9e66d9e4dd968f759fcafd447969510f23

| Area | Source and coupling | Reuse decision |
| --- | --- | --- |
| Plugin startup | `src/input_overlay.cpp`: OBS module registration, Qt actions, hooks and networking | Replace with `wWinMain`/HWND message loop |
| Capture | `src/hook/uiohook_helper_win.cpp`: libuiohook lifecycle, Win32 thread/events, OBS logging; `sdl_gamepad.*` uses SDL3 | Use Windows Raw Input for keyboard/mouse; defer gamepads |
| State | `deps/common/input_data.*`: key/button maps, motion/wheel dispatch, mutex/atomic timestamps; includes libuiohook/SDL and OBS clock | Adapt dispatch logic into `InputState`, remove unrelated dependencies and remote/gamepad state |
| Rendering | `src/sources/input_source.*`, `src/util/overlay.*`, `src/util/element/*`: OBS source callbacks, texture/effect drawing, JSON elements | Replace OBS graphics with Direct2D/DirectWrite; preserve conceptual element/state split |
| Configuration | `src/util/config.*`, `settings.h`, GUI dialog: OBS config and Qt; overlay layout JSON and texture mappings | New INI window settings; layout compatibility deferred |
| Shared schema | `deps/common/layout_constants.h`, `keycodes.h` | Not copied: no preset parser yet; uiohook codes are not interchangeable with Win32 VKs |
| Network/client | `src/network/*`, `client/src/*`: Mongoose and capture sender | Exclude; client is not a desktop renderer |
| Build | Root CMake builds a MODULE linking libobs, frontend API, Qt6, SDL3 and uiohook | Independent executable target; no OBS build template |
| Data | `data`, `presets`, `docs` | No images, shaders or fonts imported |

## Dependency and asset screening

| Component | Evidence at inspected snapshot | Status |
| --- | --- | --- |
| Input Overlay source | `LICENSE` GPL v2; adapted file header explicitly specifies version 2, Copyright 2023 Alex `<uni@vrsal.xyz>` | GPL-2.0-only application; original header retained |
| libuiohook fork | `.gitmodules` points to TolikPylypchuk/libuiohook; gitlink `98b8c0bb536c9987989c6363493bb4b5da6e9ba7`; upstream README describes GPL v3 | Excluded. Exact pinned licence could not be retrieved; do not assume compatibility with v2-only code. Resolve with rightsholders and pinned licence before adoption |
| Mongoose 7.6 | `deps/mongoose/mongoose.h` header: GPL version 2 or commercial licence, Cesanta/Sergey Lyubka notices | GPL option potentially suitable for a GPLv2 application, not an MIT-only binary; excluded because networking is unnecessary |
| SDL3 | `deps/sdl3/include/SDL3/SDL_copying.h`: zlib terms, Sam Lantinga notice | Permissive candidate with notice/alteration requirements; excluded from scaffold |
| json11 | `deps/json11/json11.cpp`: MIT, Copyright 2013 Dropbox | Permissive candidate with notice requirement; excluded (no JSON parser needed yet) |
| argparse | `client/src/argparse.h`: Copyright 2012–2015 Yecheng Fu, refers to MIT-style LICENSE | Excluded; licence file provenance must be resolved before copying |
| Qt/OBS | Referenced by root CMake; not vendored as standalone dependencies | Excluded; do not pull the plugin's build graph into this executable |
| SIMDe | Optional CMake FetchContent, v0.8.2; not fetched or audited | Excluded |
| Presets | `presets/xbox-controller/README.md` states CC0; `gamecube/CREDITS.md` credits borrowed m-overlay textures; `inphic/README.md` describes vendor images and permission correspondence | No asset imported. One preset's CC0 label does not clear the entire collection; audit each asset's origin/licence and redistribution rights before reuse |

The standalone target links only Windows system/SDK libraries (`user32`, `gdi32`,
`d2d1`, `dwrite`, `wtsapi32`, `shell32`, `comctl32`, `comdlg32`, `advapi32`) and the compiler C++ runtime. Windows SDK and runtime use and
redistribution follow Microsoft's applicable terms; no Microsoft SDK binaries or
font files are copied into this repository. Segoe UI is requested as an installed
system font. There are no bundled third-party dependency binaries or assets.

The inspection checkout and previously downloaded plugin/presets live outside
this repository and are not build inputs or distribution contents.

The local test build uses portable LLVM-MinGW 20261104 and its statically linked
compiler runtime rather than MSVC. The toolchain's Apache-2.0-with-LLVM-exceptions
text is retained in `LICENSES/LLVM-exception.txt`; the LLVM exception specifically
addresses embedded object code and GPLv2 combinations. The application remains
GPL-2.0-only. This does not grant permission to ignore other runtime components'
individual notices: audit the exact MinGW-w64 runtime notices before a public
LLVM-built binary release. The source repository contains no toolchain binaries
or packaged binary release. `com_ptr.hpp` is a small newly written owning wrapper,
replacing the optional Microsoft WRL header for toolchain portability.
