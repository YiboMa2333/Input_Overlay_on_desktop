# Installation and use

Binary release is currently on hold pending licence/runtime review. Build from
source using README, or privately inspect review staging; do not redistribute
unapproved staging as an installer or release.

When an approved Windows x64 ZIP is available, extract the **entire** folder to
a permanent writable location and run `InputOverlayDesktop.exe`. No OBS, admin
installation or separate runtime installer is intended: compiler C++ runtime is
linked statically, and supported Windows 10/11 supplies the imported system DLLs.
The packager verifies that assumption for each executable. Keep LICENSE,
LICENSES, NOTICE and the matching corresponding-source ZIP alongside it.

- The tray menu opens **Settings > Customize overlay**. The Appearance, Layout
  and Behavior tabs use Apply to save changes. Closing without Apply discards
  unapplied changes. Use Exit in the tray menu to quit completely.
- Locked mode passes mouse clicks through. Ctrl+Alt+F8 toggles Edit mode by
  default; resize handles appear only in Edit. Reset Position recovers the HUD.
- Settings are in `%LOCALAPPDATA%\InputOverlayDesktop\settings.ini`; files beside
  the executable do not need to be writable. Back up this file to migrate settings.
- Start with Windows registers only this application's value in the current
  user's Run key, with a quoted executable path and `--tray`. Keep the executable
  at that path. Disable the toggle and Apply before moving/removing the app.
- Launch-to-tray hides the HUD initially; tray Edit/Lock or the shortcut reveals
  it. The tray remains available. Do not configure a conflicting shortcut.
- Uninstall: disable Start with Windows, Apply, Exit, then remove the extracted
  folder. Optionally remove only the application's configuration directory.

Windows signing/SmartScreen approval and a native installer are not supplied.
Actual logon startup, mixed-DPI monitor moves and physical monitor unplugging
remain manual checks; see VALIDATION and CUSTOMIZATION documentation.
