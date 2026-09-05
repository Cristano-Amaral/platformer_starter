#!/usr/bin/env python3
"""Tests for cmake/StageRuntimeAssets.cmake (Milestone 38 Phase A)."""

from __future__ import annotations

import shutil
import subprocess
import tempfile
import time
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
STAGE_SCRIPT = REPO_ROOT / "cmake" / "StageRuntimeAssets.cmake"


def cmake_executable() -> str:
    found = shutil.which("cmake")
    if found is None:
        raise RuntimeError("cmake not found on PATH")
    return found


def run_stage(
    cooked: Path,
    dest: Path,
    *,
    cwd: Path | None = None,
    asset_list: str | None = "nested/a.txt;other/b.txt",
) -> subprocess.CompletedProcess[str]:
    args = [
        cmake_executable(),
        f"-DPLATFORMER_STAGE_ASSET_LIST={asset_list}" if asset_list is not None else "",
        f"-DPLATFORMER_COOKED_DIR={cooked.as_posix()}",
        f"-DPLATFORMER_STAGE_DEST={dest.as_posix()}",
        "-P",
        str(STAGE_SCRIPT),
    ]
    args = [arg for arg in args if arg]
    return subprocess.run(
        args,
        cwd=str(cwd if cwd is not None else REPO_ROOT),
        text=True,
        capture_output=True,
        check=False,
    )


class StageRuntimeAssetsTests(unittest.TestCase):
    def test_valid_staging_creates_nested_files(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cooked = root / "cooked"
            dest = root / "dest"
            (cooked / "nested").mkdir(parents=True)
            (cooked / "other").mkdir(parents=True)
            (cooked / "nested" / "a.txt").write_text("alpha", encoding="utf-8")
            (cooked / "other" / "b.txt").write_text("beta", encoding="utf-8")
            result = run_stage(cooked, dest)
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertTrue((dest / "nested" / "a.txt").is_file())
            self.assertEqual((dest / "nested" / "a.txt").read_text(encoding="utf-8"), "alpha")
            self.assertEqual((dest / "other" / "b.txt").read_text(encoding="utf-8"), "beta")
            combined = result.stdout + result.stderr
            self.assertIn("cooked:", combined)
            self.assertIn("destination:", combined)
            self.assertIn("Staging complete", combined)
            self.assertNotIn("--build", " ".join(result.args[1:]))

    def test_unchanged_file_keeps_timestamp(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cooked = root / "cooked"
            dest = root / "dest"
            (cooked / "nested").mkdir(parents=True)
            (cooked / "other").mkdir(parents=True)
            (cooked / "nested" / "a.txt").write_text("same", encoding="utf-8")
            (cooked / "other" / "b.txt").write_text("same-b", encoding="utf-8")
            first = run_stage(cooked, dest)
            self.assertEqual(first.returncode, 0, first.stderr)
            target = dest / "nested" / "a.txt"
            before = target.stat().st_mtime_ns
            time.sleep(0.05)
            second = run_stage(cooked, dest)
            self.assertEqual(second.returncode, 0, second.stderr)
            after = target.stat().st_mtime_ns
            self.assertEqual(before, after)

    def test_changed_file_is_copied(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cooked = root / "cooked"
            dest = root / "dest"
            (cooked / "nested").mkdir(parents=True)
            (cooked / "other").mkdir(parents=True)
            (cooked / "nested" / "a.txt").write_text("v1", encoding="utf-8")
            (cooked / "other" / "b.txt").write_text("b", encoding="utf-8")
            self.assertEqual(run_stage(cooked, dest).returncode, 0)
            (cooked / "nested" / "a.txt").write_text("v2", encoding="utf-8")
            self.assertEqual(run_stage(cooked, dest).returncode, 0)
            self.assertEqual((dest / "nested" / "a.txt").read_text(encoding="utf-8"), "v2")

    def test_missing_source_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cooked = root / "cooked"
            dest = root / "dest"
            cooked.mkdir()
            result = run_stage(cooked, dest)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("missing cooked asset", result.stderr + result.stdout)

    def test_missing_cooked_directory_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            result = run_stage(root / "no-cooked", root / "dest")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("cooked root is not a directory", result.stderr + result.stdout)

    def test_unrelated_cwd(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cooked = root / "cooked"
            dest = root / "dest"
            cwd = root / "unrelated cwd"
            cwd.mkdir()
            (cooked / "nested").mkdir(parents=True)
            (cooked / "other").mkdir(parents=True)
            (cooked / "nested" / "a.txt").write_text("x", encoding="utf-8")
            (cooked / "other" / "b.txt").write_text("y", encoding="utf-8")
            result = run_stage(cooked, dest, cwd=cwd)
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertTrue((dest / "nested" / "a.txt").is_file())
            self.assertFalse((cwd / "assets").exists())
            self.assertFalse((cwd / "imgui.ini").exists())

    def test_paths_with_spaces(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "repo root"
            cooked = root / "cooked files"
            dest = root / "dest files"
            (cooked / "nested").mkdir(parents=True)
            (cooked / "other").mkdir(parents=True)
            (cooked / "nested" / "a.txt").write_text("space", encoding="utf-8")
            (cooked / "other" / "b.txt").write_text("ok", encoding="utf-8")
            result = run_stage(cooked, dest)
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertEqual((dest / "nested" / "a.txt").read_text(encoding="utf-8"), "space")

    def test_stale_destination_file_is_left_in_place(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cooked = root / "cooked"
            dest = root / "dest"
            (cooked / "nested").mkdir(parents=True)
            (cooked / "other").mkdir(parents=True)
            (cooked / "nested" / "a.txt").write_text("a", encoding="utf-8")
            (cooked / "other" / "b.txt").write_text("b", encoding="utf-8")
            stale = dest / "obsolete" / "gone.txt"
            stale.parent.mkdir(parents=True)
            stale.write_text("stale", encoding="utf-8")
            result = run_stage(cooked, dest)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(stale.is_file())
            self.assertEqual(stale.read_text(encoding="utf-8"), "stale")

    def test_missing_define_fails(self) -> None:
        result = subprocess.run(
            [cmake_executable(), "-P", str(STAGE_SCRIPT)],
            cwd=str(REPO_ROOT),
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("PLATFORMER_COOKED_DIR is required", result.stderr + result.stdout)


if __name__ == "__main__":
    unittest.main()
