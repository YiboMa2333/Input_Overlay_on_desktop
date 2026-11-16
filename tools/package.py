# SPDX-License-Identifier: GPL-2.0-only
"""Create private review staging; refuse distributable ZIPs without a matching audit.

No network/publishing operations. Python standard library and CMake only.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import struct
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SYSTEM_DLLS = set("kernel32.dll user32.dll gdi32.dll advapi32.dll shell32.dll comctl32.dll comdlg32.dll d2d1.dll dwrite.dll wtsapi32.dll ole32.dll oleaut32.dll ucrtbase.dll msvcrt.dll ntdll.dll bcrypt.dll shlwapi.dll rpcrt4.dll winmm.dll version.dll imm32.dll secur32.dll ws2_32.dll".split())


def imports(path):
    """Read PE32+ imports, rejecting a non-x64 or malformed executable."""
    data = Path(path).read_bytes()
    if data[:2] != b"MZ":
        raise ValueError("Not a PE executable")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe+4] != b"PE\0\0":
        raise ValueError("Missing PE signature")
    machine, count = struct.unpack_from("<HH", data, pe+4)
    optional_size = struct.unpack_from("<H", data, pe+20)[0]
    optional = pe+24
    if machine != 0x8664 or optional_size < 224 or struct.unpack_from("<H", data, optional)[0] != 0x20B:
        raise ValueError("Package requires Windows x64 PE32+")
    sections = optional+optional_size

    def offset(rva):
        for i in range(count):
            size, address, raw_size, raw = struct.unpack_from("<IIII", data, sections+i*40+8)
            if address <= rva < address+max(size, raw_size):
                result = raw+rva-address
                if result >= len(data):
                    break
                return result
        raise ValueError("Invalid PE RVA")

    rva, size = struct.unpack_from("<II", data, optional+112+8)
    if not rva:
        raise ValueError("Unsupported executable without a primary import table")
    result = []
    for i in range(min(size//20+1, 4096)):
        at = offset(rva+i*20)
        fields = struct.unpack_from("<IIIII", data, at)
        if not any(fields):
            break
        name = offset(fields[3])
        end = data.index(b"\0", name, min(name+260, len(data)))
        result.append(data[name:end].decode("ascii").lower())
    else:
        raise ValueError("Unterminated PE imports")
    delay_rva, delay_size = struct.unpack_from("<II", data, optional+112+13*8)
    if delay_rva:
        for i in range(min(delay_size//32+1, 4096)):
            fields = struct.unpack_from("<IIIIIIII", data, offset(delay_rva+i*32))
            if not any(fields):
                break
            if not fields[0] & 1:
                raise ValueError("Unsupported non-RVA delay import table")
            name = offset(fields[1])
            end = data.index(b"\0", name, min(name+260, len(data)))
            result.append(data[name:end].decode("ascii").lower())
        else:
            raise ValueError("Unterminated delay imports")
    return sorted(set(result))


def validate_imports(names):
    unexpected = [n for n in names if n not in SYSTEM_DLLS and not n.startswith(("api-ms-win-", "ext-ms-win-"))]
    if unexpected:
        raise ValueError("Unreviewed runtime DLLs (not silently omitted): " + ", ".join(unexpected))


def sources():
    paths = [ROOT/n for n in ("CMakeLists.txt", "README.md", "LICENSE", "NOTICE.md", ".gitignore")]
    for directory in ("src", "tests", "tools", "docs", "LICENSES", "release", ".github"):
        paths.extend(p for p in (ROOT/directory).rglob("*") if p.is_file() and "__pycache__" not in p.parts)
    paths = sorted(set(paths))
    for path in paths:
        if not path.is_file() or not path.resolve().is_relative_to(ROOT):
            raise ValueError("Missing or external source file: " + str(path))
    return paths


def digest(paths):
    checksum = hashlib.sha256()
    for path in paths:
        relative = path.relative_to(ROOT).as_posix()
        if relative == "release/license-review.json":
            continue  # Avoid a self-referencing approval hash.
        checksum.update(relative.encode()+b"\0"+path.read_bytes()+b"\0")
    return checksum.hexdigest()


def approved(toolchain, source_hash):
    review = json.loads((ROOT/"release/license-review.json").read_text(encoding="utf-8"))
    if not isinstance(review, dict) or not isinstance(review.get("reviewer", ""), str):
        return False
    evidence = (ROOT/review.get("evidence", "")).resolve()
    return (review.get("approved") is True and bool(review.get("reviewer", "").strip())
            and review.get("toolchain") == toolchain
            and review.get("reviewed_source_sha256") == source_hash
            and evidence.is_relative_to(ROOT) and evidence.is_file())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("stage", "release"))
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--toolchain", required=True, choices=("msvc-static", "llvm-mingw-static"))
    parser.add_argument("--cmake", default="cmake")
    args = parser.parse_args()
    if args.mode == "release" and args.config != "Release":
        raise ValueError("Only Release configuration may be distributed; Debug runtimes are not cleared")
    paths = sources()
    source_hash = digest(paths)
    cleared = approved(args.toolchain, source_hash)
    if args.mode == "release" and not cleared:
        raise ValueError("Release blocked: licence/runtime review is missing or does not match source/toolchain. Use stage for private review.")
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise ValueError("Output must be a new/empty directory (no stale files)")
    output.mkdir(parents=True, exist_ok=True)
    stage = output/"InputOverlayDesktop"
    subprocess.run([args.cmake, "--install", str(args.build.resolve()), "--config", args.config,
                    "--prefix", str(stage)], check=True)
    executable = stage/"InputOverlayDesktop.exe"
    dlls = imports(executable)
    validate_imports(dlls)
    manifest = {"architecture": "x64", "configuration": args.config, "toolchain": args.toolchain, "source_sha256": source_hash,
                "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
                "imports": dlls, "runtime_files": "Static C++ runtime; imported DLLs provided by Windows",
                "licence_review_approved": cleared}
    (stage/"BUILD_MANIFEST.json").write_text(json.dumps(manifest, indent=2)+"\n", encoding="utf-8")
    with zipfile.ZipFile(stage/"corresponding-source.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        for path in paths:
            archive.write(path, path.relative_to(ROOT).as_posix())
    if args.mode == "stage":
        (stage/"PRIVATE_REVIEW_ONLY.txt").write_text("Not approved for release. Do not upload/distribute this review staging.\n", encoding="utf-8")
    else:
        shutil.make_archive(str(output/"InputOverlayDesktop-windows-x64"), "zip", output, stage.name)
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, struct.error, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error))
