#!/usr/bin/env python3
"""M47 import/catalog/cook-stage tests. Uses temporary trees; does not write
canonical game/assets/source content."""

from __future__ import annotations

import json
import shutil
import struct
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
REPO_ROOT = TOOLS_DIR.parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import cook_assets as cooker  # noqa: E402
from test_stage_runtime_assets import run_stage  # noqa: E402


def pack_glb(json_text: str, bin_payload: bytes = b"\x00\x00\x00\x00") -> bytes:
    json_bytes = json_text.encode("utf-8")
    while len(json_bytes) % 4:
        json_bytes += b" "
    bin_bytes = bin_payload
    while len(bin_bytes) % 4:
        bin_bytes += b"\x00"
    total = 12 + 8 + len(json_bytes) + 8 + len(bin_bytes)
    return b"".join(
        [
            b"glTF",
            struct.pack("<I", 2),
            struct.pack("<I", total),
            struct.pack("<I", len(json_bytes)),
            struct.pack("<I", 0x4E4F534A),
            json_bytes,
            struct.pack("<I", len(bin_bytes)),
            struct.pack("<I", 0x004E4942),
            bin_bytes,
        ]
    )


MINIMAL_STATIC_JSON = (
    '{"asset":{"version":"2.0"},"meshes":[{}],"buffers":[{"byteLength":4}]}'
)


def copy_known_sources(dest_root: Path) -> None:
    src = cooker.source_root(cooker.repo_root())
    dest = cooker.source_root(dest_root)
    for relative in (
        "models/test_static.glb",
        "models/test_authored.glb",
        "models/test_textured.glb",
        "textures/test_checker.png",
        "levels/level_01.level",
    ):
        target = dest / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src / relative, target)


class StaticGlbValidationTests(unittest.TestCase):
    def test_canonical_models_validate(self) -> None:
        models = cooker.source_root(cooker.repo_root()) / "models"
        for name in ("test_static.glb", "test_authored.glb", "test_textured.glb"):
            cooker.validate_static_glb((models / name).read_bytes())

    def test_invalid_magic_fails(self) -> None:
        with self.assertRaises(cooker.CookError):
            cooker.validate_static_glb(b"not a glb file!!!!")

    def test_animation_fails(self) -> None:
        payload = pack_glb(
            '{"asset":{"version":"2.0"},"meshes":[{}],"animations":[{}],'
            '"buffers":[{"byteLength":4}]}'
        )
        with self.assertRaises(cooker.CookError) as raised:
            cooker.validate_static_glb(payload)
        self.assertIn("animated", str(raised.exception))

    def test_external_buffer_uri_fails(self) -> None:
        payload = pack_glb(
            '{"asset":{"version":"2.0"},"meshes":[{}],'
            '"buffers":[{"byteLength":4,"uri":"mesh.bin"}]}'
        )
        with self.assertRaises(cooker.CookError):
            cooker.validate_static_glb(payload)


class CatalogDiscoveryTests(unittest.TestCase):
    def test_empty_models_dir_is_valid(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            sources = Path(tmp) / "game" / "assets" / "source"
            (sources / "models").mkdir(parents=True)
            extras = cooker.discover_extra_static_glb_assets(sources)
            self.assertEqual(extras, [])

    def test_discovery_is_deterministic_and_skips_unsupported(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            sources = Path(tmp) / "game" / "assets" / "source"
            models = sources / "models"
            models.mkdir(parents=True)
            (models / "zebra.glb").write_bytes(pack_glb(MINIMAL_STATIC_JSON))
            (models / "alpha.glb").write_bytes(pack_glb(MINIMAL_STATIC_JSON))
            (models / "notes.txt").write_text("nope", encoding="utf-8")
            (models / "mesh.gltf").write_text("{}", encoding="utf-8")
            extras = cooker.discover_extra_static_glb_assets(sources)
            ids = [item["id"] for item in extras]
            self.assertEqual(ids, ["models/alpha.glb", "models/zebra.glb"])
            self.assertTrue(all(item["kind"] == cooker.KIND_COPY for item in extras))
            self.assertNotIn("models/notes.txt", ids)
            self.assertNotIn("models/mesh.gltf", ids)

    def test_collect_merges_known_without_duplicate(self) -> None:
        sources = cooker.source_root(cooker.repo_root())
        collected = cooker.collect_cook_assets(sources)
        ids = [item["id"] for item in collected]
        self.assertEqual(ids, sorted(ids))
        self.assertEqual(ids.count("models/test_static.glb"), 1)
        self.assertIn("textures/test_checker.png", ids)
        self.assertIn("levels/level_01.level", ids)


class CookStageImportedGlbTests(unittest.TestCase):
    def test_imported_glb_cooks_and_stages_with_distinct_identity(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            copy_known_sources(root)
            imported_name = "imported_crate.glb"
            source_identity = f"models/{imported_name}"
            source_path = cooker.source_root(root) / "models" / imported_name
            source_path.write_bytes(pack_glb(MINIMAL_STATIC_JSON))
            collected = cooker.collect_cook_assets(cooker.source_root(root))
            ids = [item["id"] for item in collected]
            self.assertEqual(ids.count(source_identity), 1)
            self.assertIn(source_identity, ids)

            result = cooker.cook(root)
            self.assertEqual(result, 0)
            cooked_path = cooker.cooked_root(root) / "models" / imported_name
            self.assertTrue(cooked_path.is_file())
            self.assertEqual(cooked_path.read_bytes(), source_path.read_bytes())
            manifest = json.loads(
                (cooker.cooked_root(root) / "manifest.json").read_text(encoding="utf-8")
            )
            manifest_ids = [item["id"] for item in manifest["assets"]]
            self.assertEqual(manifest_ids.count(source_identity), 1)
            self.assertNotIn(str(source_path), json.dumps(manifest))

            dest = root / "staged" / "assets"
            stage = run_stage(cooker.cooked_root(root), dest, asset_list=None)
            self.assertEqual(stage.returncode, 0, stage.stderr + stage.stdout)
            staged_path = dest / "models" / imported_name
            self.assertTrue(staged_path.is_file())
            self.assertEqual(staged_path.read_bytes(), source_path.read_bytes())
            self.assertNotEqual(source_path, staged_path)
            self.assertEqual(source_identity, "models/imported_crate.glb")
            self.assertTrue(source_path.as_posix().endswith("source/models/imported_crate.glb"))
            self.assertTrue(staged_path.as_posix().endswith("assets/models/imported_crate.glb"))

    def test_invalid_extra_glb_fails_cook_without_writing_cooked_output(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            copy_known_sources(root)
            bad = cooker.source_root(root) / "models" / "broken.glb"
            bad.write_bytes(b"glTF-not-valid")
            result = cooker.cook(root)
            self.assertNotEqual(result, 0)
            self.assertFalse((cooker.cooked_root(root) / "models" / "broken.glb").exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
