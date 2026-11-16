# SPDX-License-Identifier: GPL-2.0-only
import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("package", ROOT/"tools/package.py")
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)
EXE = Path(sys.argv[1]) if len(sys.argv)>1 else None


class PolicyTests(unittest.TestCase):
    def test_source_includes_attribution(self):
        names = {p.relative_to(ROOT).as_posix() for p in package.sources()}
        self.assertTrue({"LICENSE", "NOTICE.md", "LICENSES/MIT.txt", "src/input/input_state.cpp",
                         "resources/app.ico", "resources/app.manifest", "tools/make_icon.py"} <= names)

    def test_release_hold(self):
        with patch.object(package.json, "loads", return_value={"approved": False}):
            self.assertFalse(package.approved("msvc-static", package.digest(package.sources())))

    def test_review_binds_source_and_toolchain(self):
        record = {"approved": True, "reviewer": "test-only", "toolchain": "msvc-static",
                  "reviewed_source_sha256": "test-digest", "evidence": "docs/RELEASE_REVIEW.md"}
        with patch.object(package.json, "loads", return_value=record):
            self.assertTrue(package.approved("msvc-static", "test-digest"))
            self.assertFalse(package.approved("llvm-mingw-static", "test-digest"))
            self.assertFalse(package.approved("msvc-static", "changed"))

    def test_runtime_dlls_fail_closed(self):
        package.validate_imports(["user32.dll", "api-ms-win-crt-runtime-l1-1-0.dll"])
        with self.assertRaises(ValueError):
            package.validate_imports(["libc++.dll"])
        with self.assertRaises(ValueError):
            package.validate_imports(["obs.dll"])

    def test_real_x64_imports(self):
        self.assertIsNotNone(EXE)
        names = package.imports(EXE)
        self.assertIn("user32.dll", names)
        package.validate_imports(names)

    def test_malformed_binary_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"bad.exe"
            path.write_bytes(b"bad")
            with self.assertRaises((ValueError, struct.error)):
                package.imports(path)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
