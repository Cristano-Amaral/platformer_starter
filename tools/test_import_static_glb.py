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
    for asset in cooker.KNOWN_ASSETS:
        relative = cooker.portable_relative(asset["source"])
        target = dest / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src / relative, target)


class StaticGlbValidationTests(unittest.TestCase):
    def test_canonical_models_validate(self) -> None:
        models = cooker.source_root(cooker.repo_root()) / "models"
        for name in ("player.glb", "test_static.glb", "test_authored.glb", "test_textured.glb"):
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

    def test_external_image_uri_fails(self) -> None:
        payload = pack_glb(
            '{"asset":{"version":"2.0"},"meshes":[{}],'
            '"buffers":[{"byteLength":4}],'
            '"images":[{"uri":"sidecar.png"}]}'
        )
        with self.assertRaises(cooker.CookError) as raised:
            cooker.validate_static_glb(payload)
        self.assertIn("embedded", str(raised.exception))


def glb_json(data: bytes) -> dict:
    offset = 12
    while offset + 8 <= len(data):
        chunk_length, chunk_type = struct.unpack_from("<II", data, offset)
        offset += 8
        payload = data[offset : offset + chunk_length]
        offset += chunk_length
        if chunk_type == 0x4E4F534A:
            return json.loads(payload.decode("utf-8").rstrip(" \0"))
    raise AssertionError("GLB JSON chunk missing")


class EmbeddedGlbTextureTests(unittest.TestCase):
    def test_test_textured_embeds_base_color_image(self) -> None:
        path = cooker.source_root(cooker.repo_root()) / "models" / "test_textured.glb"
        gltf = glb_json(path.read_bytes())
        images = gltf.get("images") or []
        self.assertTrue(images)
        for image in images:
            self.assertIsInstance(image, dict)
            uri = image.get("uri")
            if uri:
                self.assertTrue(str(uri).startswith("data:"))
            else:
                self.assertIn("bufferView", image)
        materials = gltf.get("materials") or []
        self.assertEqual(len(materials), 1)
        texture = materials[0]["pbrMetallicRoughness"]["baseColorTexture"]
        self.assertEqual(texture["index"], 0)
        cooker.validate_static_glb(path.read_bytes())

    def test_player_and_untextured_models_have_no_images(self) -> None:
        models = cooker.source_root(cooker.repo_root()) / "models"
        for name in ("player.glb", "test_static.glb", "test_authored.glb"):
            gltf = glb_json((models / name).read_bytes())
            self.assertFalse(gltf.get("images"))
            cooker.validate_static_glb((models / name).read_bytes())

    def test_textured_glb_cooks_and_stages_self_contained(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            copy_known_sources(root)
            source_path = cooker.source_root(root) / "models" / "test_textured.glb"
            source_bytes = source_path.read_bytes()
            self.assertEqual(cooker.cook(root), 0)
            cooked_path = cooker.cooked_root(root) / "models" / "test_textured.glb"
            self.assertEqual(cooked_path.read_bytes(), source_bytes)
            dest = root / "staged" / "assets"
            stage = run_stage(cooker.cooked_root(root), dest, asset_list=None)
            self.assertEqual(stage.returncode, 0, stage.stderr + stage.stdout)
            staged_path = dest / "models" / "test_textured.glb"
            self.assertTrue(staged_path.is_file())
            self.assertEqual(staged_path.read_bytes(), source_bytes)
            self.assertFalse((dest / "textures" / "test_textured_basecolor.png").exists())
            staged_json = glb_json(staged_path.read_bytes())
            for image in staged_json.get("images") or []:
                uri = image.get("uri")
                if uri:
                    self.assertTrue(str(uri).startswith("data:"))
                else:
                    self.assertIn("bufferView", image)


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
        self.assertEqual(ids.count("models/player.glb"), 1)
        self.assertIn("models/player.glb", ids)
        self.assertIn("textures/test_checker.png", ids)
        self.assertIn("textures/test_ground_cover_tuft.png", ids)
        self.assertNotIn("textures/test_textured_basecolor.png", ids)
        self.assertIn("levels/level_01.level", ids)
        self.assertIn("levels/level_02.level", ids)
        self.assertIn("gameplay/definitions.gameplay", ids)
        self.assertIn("sounds/item_pickup_collect.wav", ids)
        self.assertIn("sounds/player_damage.wav", ids)
        self.assertIn("sounds/player_death.wav", ids)
        self.assertIn("sounds/player_respawn.wav", ids)
        self.assertIn("sounds/player_footstep.wav", ids)
        self.assertIn("sounds/player_jump.wav", ids)
        self.assertIn("sounds/player_land.wav", ids)
        self.assertIn("sounds/checkpoint_activate.wav", ids)
        self.assertIn("sounds/pressure_plate_activate.wav", ids)
        self.assertIn("sounds/pressure_plate_deactivate.wav", ids)
        self.assertIn("sounds/door_unlock.wav", ids)
        self.assertIn("sounds/level_goal_complete.wav", ids)
        self.assertIn("sounds/collectible_collect.wav", ids)
        self.assertIn("sounds/ui_navigate.wav", ids)
        self.assertIn("sounds/ui_confirm.wav", ids)
        self.assertIn("sounds/pause_open.wav", ids)
        self.assertIn("sounds/pause_close.wav", ids)
        self.assertIn("sounds/inventory_open.wav", ids)
        self.assertIn("sounds/inventory_close.wav", ids)
        self.assertIn("shaders/world_lit.vs", ids)
        self.assertIn("shaders/world_lit.fs", ids)
        self.assertIn("shaders/shadow_depth.vs", ids)
        self.assertIn("shaders/shadow_depth.fs", ids)


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
