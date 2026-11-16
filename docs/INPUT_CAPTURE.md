# Windows input adapter

The upstream GPL input-state dispatch adaptation remains attributed in
`input_state.cpp`. The capture implementation is a separate Windows adapter;
it does not reuse the OBS/libuiohook integration because of its OBS coupling and
the unresolved dependency licensing issue documented in `UPSTREAM_AUDIT.md`.
No new third-party dependency or asset is introduced. WTS session notifications
add the Windows system library `wtsapi32` to the executable's existing SDK links.

## Capture and state boundaries

`RawInput` registers keyboard and mouse usage collections with
`RIDEV_INPUTSINK | RIDEV_DEVNOTIFY`. Windows delivers `WM_INPUT` to the window even
when an ordinary application has focus. It does not install a low-level hook,
use `BlockInput`, suppress messages or request `RIDEV_NOLEGACY`, `RIDEV_NOHOTKEYS`
or `RIDEV_CAPTUREMOUSE`. Foreground packets are passed to `DefWindowProc` for
the required cleanup. Registration is process-wide for each device collection;
this application owns its registrations exclusively.

Keyboard events use physical Set-1 make codes, not text or layout-dependent
letters. W/A/S/D/Q/E mean the positions with those labels on a US keyboard.
Both Shift sides, both Ctrl sides and Space are recognized. E0 distinguishes
right Ctrl; E1 and unsupported keys are ignored. Typematic down events are
idempotent. Mouse indices 0–4 mean physical left/right/middle/X1/X2; mouse button
swapping does not change the labels. Wheel delta is signed (up positive, down
negative), preserves high-resolution deltas and expires after 180 ms. It is a
single transient indicator, not a scroll history or accumulated log.

`InputState` has no Windows or graphics dependency. It holds bounded current
key/button states per device and ORs them into the public rendering snapshot.
Two keyboards holding the same key remain highlighted until both release it.
Events contain no character/string data. Only the tracked controls are retained.
Transition timestamps serve release recovery; no sequence of events, typed text,
keystroke history, disk recording or network transmission is implemented.

The window thread owns input, state and repaint scheduling. `RawInput` only
changes the state; it never calls the renderer. `Renderer` consumes state without
registering devices or modifying held controls. Each input invalidates the window;
the 16 ms timer handles wheel expiry and recovery even when no new input arrives.

## Recovery and lifecycle

- Ordinary focus changes preserve held keys. Background Raw Input continues;
  focus-change and timer checks repair missed releases instead of clearing chords.
- Device removal clears only that device's current holds. Keyboard overrun clears
  that device; malformed/unreadable packets conservatively clear all current state.
- A release-only `GetAsyncKeyState` high-bit check repairs missed key/button ups
  after a 100 ms grace period. This avoids resetting brand-new raw events against
  asynchronously updated Windows state. Polling never creates presses. Physical
  scan codes map through the foreground thread's layout for this safety check.
- Session lock/disconnect and suspend unregister capture and clear state;
  session connect/unlock and resume clear and re-register. A registration failure
  produces an error and closes the app. Startup requires session notification
  registration, capture registration and refresh timer creation to succeed.
- Shutdown unregisters Raw Input, session notifications and the timer and clears
  state. `RawInput` is noncopyable and also unregisters in its destructor. Error
  exits from the message loop destroy the window as well.

Windows may return zero from `GetAsyncKeyState` when the active desktop or
foreground-thread permissions prevent querying it. In that case stale highlights
are conservatively released; this can temporarily under-report a real hold.
There is no timeout that releases an otherwise observable held key merely because
the user holds it for a long time. Already-held inputs at startup or after a reset
require a new make event (keyboard repeat can provide one). Secure desktops,
elevated/isolated apps, exclusive games, injected/remapped input and remote/virtual
devices need separate integration validation; Raw Input is not a security guarantee
of hardware origin. No attempt is made to bypass Windows input isolation.

References: [registration flags](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawinputdevice),
[device notifications](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-input-device-change),
[asynchronous key state and limitations](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate).
