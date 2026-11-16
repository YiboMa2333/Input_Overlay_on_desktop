# SPDX-License-Identifier: GPL-2.0-only
"""Fetch pinned runtime source archives and assemble the release source supplement.

Never executes downloaded code. Downloads are SHA-256 checked before reading;
tar members are copied into a ZIP without extracting paths onto the filesystem.
"""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import tarfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def tree_digest(archive):
    digest = hashlib.sha256()
    with zipfile.ZipFile(archive) as source:
        names = source.namelist()
        if len(names) != len(set(names)):
            raise ValueError("Duplicate source ZIP entries")
        for name in sorted(names):
            path = PurePosixPath(name)
            if path.is_absolute() or ".." in path.parts:
                raise ValueError("Unsafe source ZIP path")
            digest.update(name.encode()+b"\0"+source.read(name)+b"\0")
    return digest.hexdigest()


def prepare(cache, output):
    record = json.loads((ROOT/"release/runtime-audit.json").read_text())
    cache.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as bundle:
        for item in record["source_archives"]:
            path = cache/(item["name"]+".tar.gz")
            if not path.exists():
                print("Fetching", item["url"], flush=True)
                urllib.request.urlretrieve(item["url"], path)
            checksum = hashlib.sha256()
            with path.open("rb") as download:
                for chunk in iter(lambda: download.read(1024*1024), b""):
                    checksum.update(chunk)
            if checksum.hexdigest() != item["sha256"]:
                raise ValueError("Source archive checksum mismatch: "+str(path))
            with tarfile.open(path) as source:
                for member in source:
                    parts = PurePosixPath(member.name).parts[1:]
                    if not parts or ".." in parts:
                        continue
                    name = "/".join(parts)
                    if item["name"] == "llvm" and not any(
                        name == p or name.startswith(p+"/") for p in item["include"]
                    ):
                        continue
                    if not (member.isfile() or member.issym() or member.islnk()):
                        continue
                    stream = source.extractfile(member)
                    if stream is None:
                        raise ValueError("Unreadable source member: "+member.name)
                    info = zipfile.ZipInfo(item["name"]+"/"+name, (2026, 11, 16, 0, 0, 0))
                    info.compress_type = zipfile.ZIP_DEFLATED
                    info.external_attr = (0o100755 if member.mode & 0o111 else 0o100644) << 16
                    bundle.writestr(info, stream.read())
        bundle.writestr("SOURCE_MANIFEST.json", json.dumps(record["source_archives"], indent=2)+"\n")
    actual = tree_digest(output)
    print("Runtime source tree SHA-256:", actual)
    expected = record.get("runtime_source_tree_sha256")
    if expected and actual != expected:
        raise ValueError("Runtime source tree differs from reviewed snapshot")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    prepare(args.cache, args.output)
