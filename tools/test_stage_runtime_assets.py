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
        # Unrelated staged files stay. Discovered Level leftovers are a
        # separate ownership-aware cleanup, covered below.
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

    def test_canonical_runtime_inventory_includes_level_02(self) -> None:
        inventory = (REPO_ROOT / "cmake" / "RuntimeAssets.cmake").read_text(encoding="utf-8")
        self.assertIn("levels/level_01.level", inventory)
        self.assertIn("levels/level_02.level", inventory)
        self.assertNotIn("levels/level_03.level", inventory)

    def test_extra_cooked_level_is_staged_without_required_inventory_edit(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cooked = root / "cooked"
            dest = root / "dest"
            required = [
                "textures/test_checker.png",
                "models/test_static.glb",
                "models/test_authored.glb",
                "models/test_textured.glb",
                "levels/level_01.level",
                "levels/level_02.level",
                "sounds/item_pickup_collect.wav",
                "sounds/player_damage.wav",
                "sounds/player_death.wav",
                "sounds/player_respawn.wav",
                "sounds/player_footstep.wav",
                "sounds/player_jump.wav",
                "sounds/player_land.wav",
                "sounds/checkpoint_activate.wav",
                "sounds/pressure_plate_activate.wav",
                "sounds/pressure_plate_deactivate.wav",
                "sounds/door_unlock.wav",
                "sounds/level_goal_complete.wav",
                "sounds/collectible_collect.wav",
                "sounds/ui_navigate.wav",
                "sounds/ui_confirm.wav",
                "sounds/pause_open.wav",
                "sounds/pause_close.wav",
                "sounds/inventory_open.wav",
                "sounds/inventory_close.wav",
            ]
            repo_cooked = REPO_ROOT / "game" / "assets" / "cooked"
            for relative in required:
                target = cooked / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(repo_cooked / relative, target)
            extra = cooked / "levels" / "level_03.level"
            extra.write_text("PLATFORMER_LEVEL 1\nid level_03\n", encoding="utf-8")
            result = run_stage(cooked, dest, asset_list=None)
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertTrue((dest / "levels" / "level_03.level").is_file())
            self.assertTrue((dest / "levels" / "level_01.level").is_file())

    def test_stale_staged_level_is_removed_without_touching_other_categories(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cooked = root / "cooked"
            dest = root / "dest"
            required = [
                "textures/test_checker.png",
                "models/test_static.glb",
                "models/test_authored.glb",
                "models/test_textured.glb",
                "levels/level_01.level",
                "levels/level_02.level",
                "sounds/item_pickup_collect.wav",
                "sounds/player_damage.wav",
                "sounds/player_death.wav",
                "sounds/player_respawn.wav",
                "sounds/player_footstep.wav",
                "sounds/player_jump.wav",
                "sounds/player_land.wav",
                "sounds/checkpoint_activate.wav",
                "sounds/pressure_plate_activate.wav",
                "sounds/pressure_plate_deactivate.wav",
                "sounds/door_unlock.wav",
                "sounds/level_goal_complete.wav",
                "sounds/collectible_collect.wav",
                "sounds/ui_navigate.wav",
                "sounds/ui_confirm.wav",
                "sounds/pause_open.wav",
                "sounds/pause_close.wav",
                "sounds/inventory_open.wav",
                "sounds/inventory_close.wav",
            ]
            repo_cooked = REPO_ROOT / "game" / "assets" / "cooked"
            for relative in required:
                target = cooked / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(repo_cooked / relative, target)
            extra = cooked / "levels" / "level_tmp_extra.level"
            extra.write_text("PLATFORMER_LEVEL 1\nid level_tmp_extra\n", encoding="utf-8")
            first = run_stage(cooked, dest, asset_list=None)
            self.assertEqual(first.returncode, 0, first.stderr + first.stdout)
            self.assertTrue((dest / "levels" / "level_tmp_extra.level").is_file())

            extra.unlink()
            unrelated_model = dest / "models" / "unrelated_probe.glb"
            unrelated_orphan = dest / "orphan.txt"
            unrelated_note = dest / "levels" / "notes.txt"
            unrelated_model.write_bytes(b"keep-model")
            unrelated_orphan.write_text("keep-orphan", encoding="utf-8")
            unrelated_note.write_text("keep-note", encoding="utf-8")

            second = run_stage(cooked, dest, asset_list=None)
            self.assertEqual(second.returncode, 0, second.stderr + second.stdout)
            combined = second.stdout + second.stderr
            self.assertIn("removed stale staged levels/level_tmp_extra.level", combined)
            self.assertFalse((dest / "levels" / "level_tmp_extra.level").exists())
            self.assertTrue((dest / "levels" / "level_01.level").is_file())
            self.assertTrue((dest / "levels" / "level_02.level").is_file())
            self.assertTrue(unrelated_model.is_file())
            self.assertTrue(unrelated_orphan.is_file())
            self.assertTrue(unrelated_note.is_file())
            self.assertTrue((dest / "textures" / "test_checker.png").is_file())
            self.assertTrue((dest / "sounds" / "item_pickup_collect.wav").is_file())
            self.assertTrue((dest / "sounds" / "player_damage.wav").is_file())
            self.assertTrue((dest / "sounds" / "player_death.wav").is_file())
            self.assertTrue((dest / "sounds" / "player_respawn.wav").is_file())
            self.assertTrue((dest / "sounds" / "player_footstep.wav").is_file())
            self.assertTrue((dest / "sounds" / "player_jump.wav").is_file())
            self.assertTrue((dest / "sounds" / "player_land.wav").is_file())
            self.assertTrue((dest / "sounds" / "checkpoint_activate.wav").is_file())
            self.assertTrue((dest / "sounds" / "pressure_plate_activate.wav").is_file())
            self.assertTrue((dest / "sounds" / "pressure_plate_deactivate.wav").is_file())
            self.assertTrue((dest / "sounds" / "door_unlock.wav").is_file())
            self.assertTrue((dest / "sounds" / "level_goal_complete.wav").is_file())
            self.assertTrue((dest / "sounds" / "collectible_collect.wav").is_file())
            self.assertTrue((dest / "sounds" / "ui_navigate.wav").is_file())
            self.assertTrue((dest / "sounds" / "ui_confirm.wav").is_file())
            self.assertTrue((dest / "sounds" / "pause_open.wav").is_file())
            self.assertTrue((dest / "sounds" / "pause_close.wav").is_file())
            self.assertTrue((dest / "sounds" / "inventory_open.wav").is_file())
            self.assertTrue((dest / "sounds" / "inventory_close.wav").is_file())


if __name__ == "__main__":
    unittest.main()
