# Validation record

## Executed on Windows, 2026-11-16

Environment: Windows build 26200 (x64), portable LLVM-MinGW 20261104 / Clang
23.1.5, CMake 4.5.0, MinGW Makefiles, C++20, Release, statically linked compiler
runtime. No OBS runtime or SDK was used. Tools were extracted outside the Git
repository into the workspace; no global toolchain installation was required.

The documented LLVM configure/build completed successfully. The final idle-desktop
CTest run passed all four targets (0 failures, 4.78 seconds). Results are retained
locally in `build/validation-junit.xml` (generated/ignored, not a source artifact):

| Test | Evidence/scope |
| --- | --- |
| `input_state_transitions` | W+D/selective release, simultaneous keyboard/mouse, repeats, device removal, modifiers, wheel expiry, release recovery and state-to-HUD mapping |
| `placement_geometry` | All eight handles, anchored corners, single-axis edges, Shift lock, bounds/no-flip, DPI hit-test scaling, negative coordinates, partial offscreen placement, simulated monitor removal and resolution shrink |
| `settings_persistence` | Position/size/mode/shortcut roundtrip, signed coordinates, overflow/corrupt values, unsafe shortcut rejection and observable write failure |
| `windows_desktop_integration` | Real standalone HWND/tray, edit-gap hit testing, pointer-driven movement/resizing, click-through/focus in another process, persistence across restarts, real shortcut toggles/conflict/cleanup and recovery notifications |

The native test launches only its own overlay children with isolated LOCALAPPDATA
profiles inside the build directory. It opens a dedicated backdrop, uses
`SetCursorPos`/`SendInput` for gestures, checks the foreground window, restores
the pointer and closes its children. It first verifies that an opaque key is
hit in Edit mode, then that the same point reaches the backdrop in Locked mode
with a real click. Backdrop/overlay Z-order is explicitly isolated from unrelated
topmost desktop windows; the two tested windows belong to different processes.

The test invokes public tray command handlers through WM_COMMAND. It verifies
the tray icon through `Shell_NotifyIconGetRect`, but does not automate choosing
every Shell menu item by pointer. Native edit/locked screenshots were captured,
and inspected; see [PLACEMENT.md](PLACEMENT.md).

The expanded interaction test initially stopped at a backdrop hit-test mismatch:
locking restored another foreground desktop window above the test backdrop.
The harness now explicitly orders its backdrop below the overlay and validates
the opaque-key hit before the locked probe. The corrected interaction suite
passed. No production click-forwarding workaround was introduced.

Further shared-desktop reruns intermittently lost capture or saw unexpected
pointer movement. Diagnostics observed the actual pointer leave the requested
test location. The harness now uses atomic absolute pointer events and checks
capture before proceeding; drag handling uses queued screen-event positions
instead of asynchronous cursor queries, avoiding geometry drift from later input.
The final run passed, including checking that a painted locked key remained
visibly rendered while that same point delivered a real click underneath.
Interactive tests still require an idle desktop: concurrent user/automation input
can invalidate them. Testing paused when the user requested that this limitation
be documented, then resumed for the follow-up below when the user was available.

## Follow-up verification, 2026-11-16

Windows reported one connected monitor, 2240 by 1400 pixels, effective DPI 144
(150% scaling), with a 2240 by 1328 work area. The user confirmed a second monitor
is unavailable on this PC and agreed to leave physical removal and mixed-DPI
cross-monitor checks untested.

The expanded build succeeded and the final CTest run passed all four targets
(0 failures, 6.20 seconds). The generated result is retained locally as
`build/verification-followup.xml`. Additional checks in the desktop suite passed:

- Pixel probes over white, black and dark backdrops confirmed that the gap
  between keyboard and mouse exactly matched the backdrop, while a painted key
  remained visually distinct. This tests alpha composition; it does not replace
  a full readability review on every background or DPI.
- Eighty alternating native surface resizes completed, with the child's GDI
  object count staying within four objects of its initial count. This checks
  GDI ownership, not every category of memory or graphics-driver allocation.
- The test removed only its own overlay's tray icon, sent `TaskbarCreated`, and
  verified that the icon was re-added. Explorer itself was not restarted.
- Moving the overlay on the available monitor preserved its logical size at
  DPI 144. Cross-monitor/mixed-DPI coverage was explicitly reported unexercised.
- Real tray-popup activation was attempted, but Windows kept this icon in a
  hidden/overflow area; the popup selection subcheck was explicitly unexercised.
  Tray registration, recovery, shortcut fallback and command handlers passed.

The earlier movement, corner/edge resizing, Shift locking, click-through/focus,
restart persistence, offscreen recovery and hotkey-conflict checks also passed
again without the previous pointer-interference failures. Test children exited
cleanly. No production application code changed in this follow-up.

## Reproduce

Use the README build commands. Ordinary CTest runs the state, geometry and
settings suites. To include interactive tests:

```powershell
cmake -S . -B build -DOVERLAY_ENABLE_DESKTOP_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For LLVM use the existing `build/llvm` directory instead of `build`. Do not
interact with the desktop during the native test; it temporarily controls the
pointer. Run in an interactive unlocked Windows session. A noninteractive/headless
session cannot verify desktop input or Shell behavior. MSVC build commands are
provided but have not been run in this environment.

## User acceptance sequence exercised

1. Opened Edit via its actual command handler and confirmed interactive styling.
2. Used real pointer gestures to drag to the top-right of the work area.
3. Pulled a corner and verified the opposite anchor and original ratio.
4. Tested unlocked single-axis edge resize and Shift restoration of original ratio.
5. Locked via the actual handler and probed a painted key region.
6. Sent a real click; the separate-process backdrop received it and held focus.
7. Exited and relaunched; compared saved/recovered position and size.
8. Verified both Locked and Edit modes persist across restarts.
9. Moved the HWND completely offscreen and delivered WM_DISPLAYCHANGE; verified
   recovery. Invoked Reset Position and checked accessible work-area bounds.
10. Sent real Ctrl+Alt+F8 shortcuts in both directions, checked cleanup, then
    launched with a competing registration and verified that owner still received
    the shortcut while the overlay remained usable through its tray.

## Remaining manual/hardware checks

- Physical multi-monitor removal and mixed-DPI transitions are explicitly deferred
  at the user's request because this PC has one available monitor. With suitable
  hardware, disconnect/reconnect a monitor containing the HUD; change resolution
  and work-area/taskbar layout and confirm accessible placement. The automated
  recovery notification and monitor topology model do not replace cable testing.
- Move between monitors at 100%, 125%, 150% and 200% DPI, including negative
  screen coordinates. The gesture ends on DPI transition; verify a new gesture
  continues normally and logical size is stable.
- Open the tray menu by pointer, select every command, edit/reload a shortcut
  using Notepad and restart Explorer to verify icon re-add/retry behavior.
- Verify keyboard/mouse visualization against physical hardware in ordinary apps,
  long chords, device unplugging, session lock/unlock and suspend/resume. Unit
  state tests and UI gesture injection do not establish every raw-device behavior.
- White/black/dark backdrop alpha probes and repeated-resize GDI counts passed in
  the follow-up. Patterned-background readability, physical pressed-state capture,
  mixed-DPI text sharpness, other memory allocations and recovery after a graphics
  device reset remain separate checks.
- Exclusive full-screen games, elevated/isolated applications, secure desktop,
  remote sessions and simultaneous external topmost windows need separate testing.
- No public binary package has been produced. Audit the exact compiler runtime
  notices and include corresponding source/notices before a public release.

The earlier scaffold lacked a usable compiler. That limitation has now been
resolved with the portable toolchain; previous unbuilt/unrun statements describe
the earlier state, not the current validation results.
