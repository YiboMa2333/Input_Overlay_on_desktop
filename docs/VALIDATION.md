# Validation record and manual checks

## Current record (2026-11-16)

- Repository trees, pinned upstream sources, copyright/licence headers and
  CMake dependency declarations were inspected.
- CMake was invoked in PowerShell but is not installed/on PATH. No usable
  `cl`, `clang-cl`, `clang++`, `g++` or Ninja command was found.
- `vswhere -latest` returned no registered Visual Studio installation. The
  Visual Studio directory exists but no `cl.exe` was found beneath it. The
  standard Windows SDK include path was absent.
- No configure, build, automated test or executable launch has succeeded or
  been claimed. Source review cannot establish Windows compilation or behavior.
- An existing WSL Ubuntu distribution was also checked with `command -v` for
  `c++`, `cmake` and `x86_64-w64-mingw32-g++`; none was available there.
- Git whitespace checks passed for these changes. This is not a compilation
  or behavioral test result.

## Required verification on a provisioned machine

Run the README's exact configure and Release build commands, then launch the
resulting executable. Record the Windows version, compiler, SDK, CMake version,
commit and output before claiming the launch criterion is satisfied.

1. Confirm a normal desktop window appears with no OBS installation/process.
2. Focus Notepad or another ordinary app. Hold W+D: both cells highlight.
   Release W: only D remains highlighted. Release D: both clear. Repeat with
   Q/E/A/S/Space, left/right Shift and Ctrl. Hold both modifier sides and release
   one: the shared modifier cell stays lit. Alt-Tab between ordinary apps while
   holding W: W remains highlighted and clears on release in the other app.
   Repeat under another keyboard layout; labels represent physical US positions.
   Type untracked letters into Notepad: input remains normal and no overlay
   state for them is retained.
3. Press/release all five mouse buttons. Check matching highlighting, cursor
   coordinates and signed vertical wheel ticks. Wheel display should clear
   approximately 180 ms after the last scroll event. A single physical mouse
   button must light only its own region; hold W+D simultaneously with left
   mouse, then release controls individually. Test both wheel directions and
   normal foreground clicking, dragging, scrolling, shortcuts and typing.
4. Drag the window; verify topmost behavior, minimize/restore and close. Confirm
   closing exits the process and stops capture. Relaunch and verify position.
5. Edit INI dimensions and topmost while closed. Check bounds and off-screen
   position fallback. Test a non-writable settings directory.
6. Check display-scale changes and target recreation where practical; document
   clipping and DPI limitations. Test rapid clicks and held-device disconnects
   against the known limitations in README. Hold a key/button and unplug its
   device: that device's highlights must clear. With two keyboards holding W,
   releasing/removing one must preserve W until the other releases. Repeat with
   two mice. Lock/unlock Windows and suspend/resume while holding inputs: old
   highlights must clear; new presses/releases must work after recovery. Use a
   debugger to skip a release packet, then resume without restarting: the safety
   check should clear it after the 100 ms grace interval and next timer tick.
7. Inspect executable dependencies with `dumpbin /dependents`; no OBS, Qt, SDL,
   uiohook or Mongoose DLL should be required. Verify package includes notices
   and a matching complete source archive before release.

## Automated transition tests

`tests/input_state_tests.cpp` is a standalone CTest executable, using explicit
checks that remain enabled in Release builds and injected clock times (no sleeps).
It covers W+D and selective release, all supported keys, repeat downs, invalid and
untracked codes, independent mouse regions, simultaneous keyboard/mouse, multiple
devices holding one control, device removal, modifiers, wheel directions/expiry,
motion independence, release-recovery grace, no synthetic presses and reset/restart.
Build/run commands are in README. Tests have been added but have not been run
here because no C++ toolchain/CMake is available. These tests do not validate
Windows registrations, scan-code delivery, session messages or graphics; the
manual checks above are still required.
