# Attribution

Input Overlay Desktop includes an adaptation of `deps/common/input_data.cpp`
from [univrsal/input-overlay](https://github.com/univrsal/input-overlay), inspected
at commit `7d97ae9e66d9e4dd968f759fcafd447969510f23`.

Copyright 2023 Alex <uni@vrsal.xyz>. The original GPL version 2 notice and the
standalone modification notice remain in `src/input/input_state.cpp`.
The combined application is GPL-2.0-only. Its corresponding source and build
scripts must accompany a distributed binary under the intended release process.

The original repository's MIT notice (Copyright 2026 Yibo Ma) is retained in
`LICENSES/MIT.txt`; this does not make the combined application MIT-only.
No upstream presets, fonts, OBS, Qt, SDL, Mongoose or libuiohook are shipped.
Windows system fonts/icons and DLLs are requested from the installed OS.

Compiler runtime terms depend on the selected toolchain. The LLVM exception
notice is retained in `LICENSES/LLVM-exception.txt`; full runtime compatibility
review remains pending. See `docs/RELEASE_REVIEW.md` before any binary release.
