#!/usr/bin/env python3
"""Header-gate tests for cooker level_v1 kind (Milestone 31 Phase A)."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
REPO_ROOT = TOOLS_DIR.parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import cook_assets as cooker  # noqa: E402
from test_import_static_glb import copy_known_sources  # noqa: E402
from test_stage_runtime_assets import run_stage  # noqa: E402


class LevelV1HeaderTests(unittest.TestCase):
    def test_canonical_source_header_is_accepted(self) -> None:
        source = cooker.source_root(cooker.repo_root()) / "levels" / "level_01.level"
        cooker.validate_level_v1_header(source.read_bytes())

    def test_level_02_source_header_is_accepted(self) -> None:
        source = cooker.source_root(cooker.repo_root()) / "levels" / "level_02.level"
        cooker.validate_level_v1_header(source.read_bytes())
        self.assertIn(b"id level_02", source.read_bytes())

    def test_wrong_magic_fails(self) -> None:
        with self.assertRaises(cooker.CookError):
            cooker.validate_level_v1_header(b"PLATFORMER_SAVE 1\n")

    def test_unsupported_version_fails(self) -> None:
        with self.assertRaises(cooker.CookError) as raised:
            cooker.validate_level_v1_header(b"PLATFORMER_LEVEL 2\n")
        self.assertIn("unsupported", str(raised.exception))

    def test_bom_fails(self) -> None:
        with self.assertRaises(cooker.CookError):
            cooker.validate_level_v1_header(b"\xef\xbb\xbfPLATFORMER_LEVEL 1\n")

    def test_static_prop_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"static_prop 1 2 3 0 45 0 1 1 1 models/test_static.glb\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"static_prop 1 2 3", payload)
        self.assertIn(b"models/test_static.glb", payload)

    def test_pressure_plate_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"pressure_plate 2 0.1 0 2 0.2 2\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"pressure_plate 2 0.1 0", payload)

    def test_door_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"door 4 1.5 0 1.2 3 2.4 3.2\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"door 4 1.5 0", payload)

    def test_door_requires_key_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"door 4 1.5 0 1.2 3 2.4 3.2 1\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"door 4 1.5 0", payload)
        self.assertIn(b"3.2 1", payload)

    def test_door_item_id_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"door 4 1.5 0 1.2 3 2.4 3.2 card\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"door 4 1.5 0", payload)
        self.assertIn(b"card", payload)

    def test_pressure_plate_mode_flags_do_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"pressure_plate 2 0.1 0 2 0.2 2 0 0 1 0\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"pressure_plate 2 0.1 0", payload)
        self.assertIn(b"0 1 0", payload)

    def test_item_pickup_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"item_pickup 2 1 0 1 key\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"item_pickup 2 1 0", payload)
        self.assertIn(b"key", payload)

    def test_item_pickup_visual_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"item_pickup 2 1 0 1 key visual 0 0.5 0 0 90 0 0.15 0.15 0.15 "
            b"models/Chest by Quaternius - O72u4Drp8k.glb\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"visual 0 0.5 0", payload)
        self.assertIn(b"models/Chest by Quaternius - O72u4Drp8k.glb", payload)


    def test_item_pickup_bounds_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"item_pickup 2 1 0 1 key visual 0 0.5 0 0 90 0 0.15 0.15 0.15 bounds 0 "
            b"models/Chest by Quaternius - O72u4Drp8k.glb\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"bounds 0", payload)
        self.assertIn(b"models/Chest by Quaternius - O72u4Drp8k.glb", payload)

    def test_item_pickup_highlight_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"item_pickup 2 1 0 1 key visual 0 0.5 0 0 90 0 0.15 0.15 0.15 bounds 1 "
            b"highlight 0.25 models/Chest by Quaternius - O72u4Drp8k.glb\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"highlight 0.25", payload)
        self.assertIn(b"models/Chest by Quaternius - O72u4Drp8k.glb", payload)

    def test_item_pickup_gold_idle_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"item_pickup 2 1 0 1 key visual 0 0.5 0 0 90 0 0.15 0.15 0.15 bounds 1 "
            b"highlight 0.25 gold 0.5 idle 1 0.2 2 90 "
            b"models/Chest by Quaternius - O72u4Drp8k.glb\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"gold 0.5", payload)
        self.assertIn(b"idle 1 0.2 2 90", payload)
        self.assertIn(b"models/Chest by Quaternius - O72u4Drp8k.glb", payload)


    def test_level_goal_record_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"level_goal -21 3.8 0 2 1.6 1.8\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"level_goal -21 3.8 0", payload)

    def test_destination_bearing_level_goal_does_not_fail_header_gate(self) -> None:
        payload = (
            b"PLATFORMER_LEVEL 1\n"
            b"id level_01\n"
            b"level_goal -21 3.8 0 2 1.6 1.8 level_02\n"
        )
        cooker.validate_level_v1_header(payload)
        self.assertIn(b"level_02", payload)


class ExtraLevelDiscoveryTests(unittest.TestCase):
    def test_discovery_skips_unsafe_names_and_is_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            sources = Path(tmp) / "game" / "assets" / "source"
            levels = sources / "levels"
            levels.mkdir(parents=True)
            (levels / "level_03.level").write_text("PLATFORMER_LEVEL 1\n", encoding="utf-8")
            (levels / "level_10.level").write_text("PLATFORMER_LEVEL 1\n", encoding="utf-8")
            (levels / "notes.txt").write_text("nope", encoding="utf-8")
            (levels / "bad-name.level").write_text("PLATFORMER_LEVEL 1\n", encoding="utf-8")
            extras = cooker.discover_extra_level_v1_assets(sources)
            ids = [item["id"] for item in extras]
            self.assertEqual(ids, ["levels/level_03.level", "levels/level_10.level"])
            self.assertTrue(all(item["kind"] == cooker.KIND_LEVEL_V1 for item in extras))

    def test_collect_does_not_duplicate_canonical_levels(self) -> None:
        sources = cooker.source_root(cooker.repo_root())
        collected = cooker.collect_cook_assets(sources)
        ids = [item["id"] for item in collected]
        self.assertEqual(ids.count("levels/level_01.level"), 1)
        self.assertEqual(ids.count("levels/level_02.level"), 1)

    def test_created_level_cooks_and_stages_without_hardcoded_enumeration(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            copy_known_sources(root)
            extra_identity = "levels/level_03.level"
            extra_path = cooker.source_root(root) / "levels" / "level_03.level"
            extra_path.write_text("PLATFORMER_LEVEL 1\nid level_03\n", encoding="utf-8")
            inventory = (REPO_ROOT / "cmake" / "RuntimeAssets.cmake").read_text(encoding="utf-8")
            self.assertNotIn("levels/level_03.level", inventory)
            collected = cooker.collect_cook_assets(cooker.source_root(root))
            ids = [item["id"] for item in collected]
            self.assertEqual(ids.count(extra_identity), 1)
            self.assertIn(extra_identity, ids)

            result = cooker.cook(root)
            self.assertEqual(result, 0)
            cooked_path = cooker.cooked_root(root) / "levels" / "level_03.level"
            self.assertTrue(cooked_path.is_file())
            dest = root / "staged" / "assets"
            stage = run_stage(cooker.cooked_root(root), dest, asset_list=None)
            self.assertEqual(stage.returncode, 0, stage.stderr + stage.stdout)
            staged_path = dest / "levels" / "level_03.level"
            self.assertTrue(staged_path.is_file())
            self.assertTrue(staged_path.as_posix().endswith("assets/levels/level_03.level"))

    def test_deleted_extra_level_is_removed_from_cooked_and_staged(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            copy_known_sources(root)
            extra_source = cooker.source_root(root) / "levels" / "level_tmp_extra.level"
            extra_source.write_text(
                "PLATFORMER_LEVEL 1\nid level_tmp_extra\n", encoding="utf-8"
            )

            first_cook = cooker.cook(root)
            self.assertEqual(first_cook, 0)
            cooked_extra = cooker.cooked_root(root) / "levels" / "level_tmp_extra.level"
            self.assertTrue(cooked_extra.is_file())

            dest = root / "staged" / "assets"
            first_stage = run_stage(cooker.cooked_root(root), dest, asset_list=None)
            self.assertEqual(first_stage.returncode, 0, first_stage.stderr + first_stage.stdout)
            staged_extra = dest / "levels" / "level_tmp_extra.level"
            self.assertTrue(staged_extra.is_file())

            unrelated_model = dest / "models" / "unrelated_probe.glb"
            unrelated_note = dest / "levels" / "notes.txt"
            unrelated_orphan = dest / "orphan.txt"
            unrelated_model.write_bytes(b"not-a-real-glb")
            unrelated_note.write_text("keep", encoding="utf-8")
            unrelated_orphan.write_text("keep", encoding="utf-8")

            extra_source.unlink()
            self.assertFalse(extra_source.exists())

            second_cook = cooker.cook(root)
            self.assertEqual(second_cook, 0)
            self.assertFalse(cooked_extra.exists())
            self.assertTrue(
                (cooker.cooked_root(root) / "levels" / "level_01.level").is_file()
            )
            self.assertTrue(
                (cooker.cooked_root(root) / "levels" / "level_02.level").is_file()
            )
            self.assertTrue(
                (cooker.cooked_root(root) / "models" / "test_static.glb").is_file()
            )

            second_stage = run_stage(cooker.cooked_root(root), dest, asset_list=None)
            self.assertEqual(
                second_stage.returncode, 0, second_stage.stderr + second_stage.stdout
            )
            self.assertFalse(staged_extra.exists())
            self.assertTrue((dest / "levels" / "level_01.level").is_file())
            self.assertTrue((dest / "levels" / "level_02.level").is_file())
            self.assertTrue(unrelated_model.is_file())
            self.assertEqual(unrelated_model.read_bytes(), b"not-a-real-glb")
            self.assertTrue(unrelated_note.is_file())
            self.assertTrue(unrelated_orphan.is_file())
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

    def test_unmanifested_cooked_level_is_removed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            copy_known_sources(root)
            self.assertEqual(cooker.cook(root), 0)
            ghost = cooker.cooked_root(root) / "levels" / "level_ghost.level"
            ghost.write_text("PLATFORMER_LEVEL 1\nid level_ghost\n", encoding="utf-8")
            leftover_note = cooker.cooked_root(root) / "levels" / "notes.txt"
            leftover_note.write_text("keep", encoding="utf-8")
            self.assertEqual(cooker.cook(root), 0)
            self.assertFalse(ghost.exists())
            self.assertTrue(leftover_note.is_file())
            self.assertTrue(
                (cooker.cooked_root(root) / "levels" / "level_01.level").is_file()
            )
            self.assertTrue(
                (cooker.cooked_root(root) / "levels" / "level_02.level").is_file()
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
