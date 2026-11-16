# Release review: HOLD

No public binary release is approved. `release/license-review.json` is deliberately
`approved: false`. Do not mark it approved merely to obtain a ZIP or CI artifact.

| Item | Finding / remaining review |
| --- | --- |
| Application source | GPL-2.0-only; adapted upstream state header and attribution retained |
| Original repository MIT notice | Preserved separately, not asserted as the combined application's licence |
| Upstream dependencies/assets | No OBS/Qt/libuiohook/SDL/Mongoose or upstream texture assets imported |
| New UI | Win32 common controls/dialogs and system fonts; original generated keyboard/mouse icon, generator and ICO source included; no imported image assets or UI framework |
| Windows libraries | user32, gdi32, d2d1, dwrite, wtsapi32, shell32, comctl32, comdlg32, advapi32; imported DLLs audited from the built PE |
| CI target | Windows x64, MSVC static CRT (/MT); static runtime avoids a missing VC runtime DLL, but does not by itself establish licence compatibility |
| MSVC runtime | Review exact CRT/STL/SDK notices, redistribution terms and applicable GPL system-library treatment before approving this route |
| Local LLVM test build | LLVM-MinGW static runtime; LLVM exception text preserved, full MinGW-w64/runtime notice inventory still pending; not approved for redistribution |
| Corresponding source | Packager snapshots exact source/build scripts/notices with the binary; manifest records source and executable SHA-256 |
| Dynamic dependencies | Packager rejects non-system DLL imports rather than silently omitting runtimes; no automatic download/copy of unreviewed DLLs |
| Publication | No GitHub Release publishing step; workflow artifact upload is manual, main-branch-only, and must first pass the review gate |

## Approving a particular source snapshot

1. Complete the runtime/licence review for the exact compiler and linked libraries.
   Record versions, notices, compatibility reasoning and any required source or
   additional licence files here. Keep GPL obligations and upstream attribution.
2. Run the Release build, tests and `tools/package.py stage`. Inspect the manifest's
   DLL imports and source snapshot, including the corresponding-source ZIP.
3. Set `release/license-review.json` only after review: named reviewer, approved
   toolchain, evidence path and `reviewed_source_sha256` from the manifest.
   That file alone is excluded from the source digest to avoid self-reference.
   Any other source/build/notice/documentation change invalidates the approval.
4. Run `tools/package.py release` with the reviewed toolchain. It refuses missing,
   mismatched or unapproved review records. The workflow has the same check.

Private review staging contains a `PRIVATE_REVIEW_ONLY.txt` marker. It is not
uploaded by CI and must not be presented as a release. The release gate is a
maintainer process and fail-closed check, not an automated legal opinion.

References: [CRT linkage](https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-170),
[upstream audit](UPSTREAM_AUDIT.md), [GPL obligations](LICENSING.md).
