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

## Required verification on a provisioned machine

Run the README's exact configure and Release build commands, then launch the
resulting executable. Record the Windows version, compiler, SDK, CMake version,
commit and output before claiming the launch criterion is satisfied.

1. Confirm a normal desktop window appears with no OBS installation/process.
2. Focus a different application. Press/release letters, left/right Shift,
   Ctrl and Alt, arrows and a multi-key chord. Labels must reflect held keys
   and clear on release; typing in the other app must continue normally.
3. Press/release all five mouse buttons. Check matching highlighting, cursor
   coordinates and signed vertical wheel ticks. Wheel display should clear
   approximately 180 ms after the last scroll event.
4. Drag the window; verify topmost behavior, minimize/restore and close. Confirm
   closing exits the process and stops capture. Relaunch and verify position.
5. Edit INI dimensions and topmost while closed. Check bounds and off-screen
   position fallback. Test a non-writable settings directory.
6. Check display-scale changes and target recreation where practical; document
   clipping and DPI limitations. Test rapid clicks and held-device disconnects
   against the known limitations in README.
7. Inspect executable dependencies with `dumpbin /dependents`; no OBS, Qt, SDL,
   uiohook or Mongoose DLL should be required. Verify package includes notices
   and a matching complete source archive before release.

Future automated state tests should cover out-of-range codes, press/release
transitions, wheel expiry and simultaneous keys. Runtime/GUI and device tests
remain necessary beyond those unit tests.
