# Dependency and licence review: pinned LLVM-MinGW portable build

Reviewed 16 November 2026 by Codex (automated dependency/licence audit). This is
an engineering assessment of this distribution, not legal advice or blanket
approval of future changes or every compiler supported by the source.

**Finding:** no incompatible dependency was identified in the scoped LLVM-MinGW
Release application. Distribution is supported with its GPL notices, application
source and runtime source supplement. Approval binds the source snapshot and
selected compiler/runtime file hashes. MSVC binaries are outside this approval.

## Components and evidence

| Component | Finding and required materials |
| --- | --- |
| Application | GPL-2.0-only. Adapted upstream input_data.cpp at 7d97ae9e66d9e4dd968f759fcafd447969510f23; Alex's original copyright/GPL header and modification notices retained. Full source and build scripts accompany the EXE. |
| Original MIT project | Yibo Ma's original notice preserved in LICENSES/MIT.txt; it does not relicense the combined application. |
| Icon/UI | Original code-generated icon, generator and assets included. No imported presets, images or fonts. Windows fonts are requested from the OS. |
| Toolchain | LLVM-MinGW 20261104 UCRT x86_64, Clang 23.1.5; LLVM commit 392dfff97d29b59d77e5cac6d4b6ef2585fe7117; MinGW commit ee9bd87644156c41072a11ad957d1e11831a72df. Exact archive and selected file hashes in release/runtime-audit.json. |
| LLVM runtimes | libc++, libc++abi, libunwind and compiler-rt: Apache-2.0 WITH LLVM-exception plus retained legacy notices. Exact component licence texts are in LICENSES. The LLVM exception expressly addresses compiled LLVM code combined with GPLv2, including conflicting Apache conditions. Exact runtime sources, credits and notices accompany the binary. |
| MinGW runtime | ZPL-2.1/public-domain/BSD/MIT portions. Complete aggregate notices copied unmodified to LICENSES/COPYING.MinGW-w64-runtime.txt. Winpthreads BSD/MIT notices included conservatively. |
| Imported headers | Wine-derived LGPL-2.1-or-later interface headers; source/notices and LGPL text included. Definitions, macros and interface declarations are used; LGPL 2.1 section 5 covers declarations and small inlines. Application remains GPLv2. |
| Historical math notice | MinGW's aggregate notice has a Cephes FIXME. Actual linked MinGW object names were compared to the 15 math source files containing Cephes/Moshier markers in the pinned tree: no matches. Linked gdtoa conversion/formatting notices are preserved. This is not clearance for every optional MinGW math function. |
| Windows libraries | UCRT, user32, gdi32, d2d1, dwrite, wtsapi32, shell32, comctl32, comdlg32, advapi32 and other imported system DLLs are supplied by Windows, not copied into the release. PE import audit rejects unknown non-system DLLs, non-x64 and non-GUI executables. |
| Excluded | OBS, Qt, SDL, libuiohook, Mongoose, upstream textures, OpenMP, sanitizer/profiling runtimes, compiler binaries, Python and CMake are not bundled. Some build-tool source files in the source supplement have GPLv3 terms; they are a separate source aggregation, not linked application code. GPLv3 text is included for those source files. |
| MSVC route | Compilation, headless tests and staging passed in GitHub Actions. Static MSVC runtime distribution has not been cleared; the distributable workflow uses LLVM-MinGW instead. This is not a finding that MSVC distribution is prohibited. |

release/linked-objects.txt records the application's link-map inputs inspected.
CMake now generates InputOverlayDesktop.map for LLVM builds. BUILD_MANIFEST.json
records the actual EXE hash, imports, normalized application-source hash and
runtime audit. The release packager checks selected compiler/runtime hashes, not
just a caller-supplied toolchain name. Text source hashes normalize CRLF to LF;
binary assets are unchanged. .gitattributes pins LF in future checkouts.

## Materials accompanying every reviewed release

- corresponding-source.zip: exact application sources, resources, tests, build
  and packaging scripts, workflow, docs, audit and retained licences.
- runtime-sources.zip: exact libc++, libc++abi, libunwind, compiler-rt, runtime
  CMake infrastructure, full pinned MinGW-w64 tree and LLVM-MinGW build recipes.
  All file-level notices/credits remain present. Archive URLs/checksums and a
  source-tree hash identify the collected source.
- LICENSE, LICENSES, NOTICE.md and reproducible application build instructions.

This accompanies binaries with source under GPLv2 section 3(a), rather than
relying only on a moving branch link or a written source offer. Recipients retain
GPL rights without an added EULA. The compiler and Windows OS are not bundled.
Distribute the complete portable ZIP, not a naked EXE without these materials.
Older private review ZIPs remain unapproved.

## Reproduce the package

Use the pinned LLVM-MinGW build instructions in README, then:

```powershell
python tools/runtime_sources.py --cache build/audit-downloads --output build/runtime-sources.zip
python tools/package.py release --build build/llvm --output dist/release `
  --toolchain llvm-mingw-static --toolchain-root C:/tools/llvm-mingw-20261104-ucrt-x86_64 `
  --runtime-sources build/runtime-sources.zip
```

Use a new/empty output directory; supply --cmake if it is not on PATH. Source
archives are checked before reading and never executed by the source assembler.
Packaging rejects missing/mismatched approval, runtime sources or toolchain files.

On GitHub, merge these changes to main. Open **Actions > Windows x64 build and
reviewed packaging > Run workflow**, choose main and enable **package**. After
both jobs pass, download the **InputOverlayDesktop-windows-x64** artifact. Extract
the outer Actions artifact ZIP and attach its contained portable ZIP to your
GitHub Release. Use a preview tag such as **v0.1.0-preview.1** and include the test
limitations from TRAY_RELEASE.md. The workflow does not create a tag or publish
a Release automatically.

## Primary references

- [GPLv2 sections 2 and 3](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)
- [Exact libc++ licence and LLVM exception](https://github.com/llvm/llvm-project/blob/llvmorg-23.1.5/libcxx/LICENSE.TXT)
- [Exact MinGW runtime notices](https://github.com/mingw-w64/mingw-w64/blob/ee9bd87644156c41072a11ad957d1e11831a72df/COPYING.MinGW-w64-runtime/COPYING.MinGW-w64-runtime.txt)
- [MinGW tool versus runtime notice scope](https://github.com/mingw-w64/mingw-w64/blob/ee9bd87644156c41072a11ad957d1e11831a72df/COPYING.MinGW-w64/COPYING.MinGW-w64.txt)
- [Pinned compiler release](https://github.com/mstorsjo/llvm-mingw/releases/tag/20261104)
- [Pinned MinGW recipe](https://github.com/mstorsjo/llvm-mingw/blob/20261104/build-mingw-w64.sh)

## Validation limits

The prior local five-suite Windows pass and actual Explorer-restart recovery test
are real. A separate packaged GUI run successfully opened the tray menu and
invoked Lock, then failed to reopen notification overflow for Edit. That complete
run failed; repeated tray interaction still needs investigation. Physical mixed-DPI
and monitor disconnect tests, and actual next-logon startup remain unverified.
Binaries are unsigned. Licence clearance does not remove these product limits.
