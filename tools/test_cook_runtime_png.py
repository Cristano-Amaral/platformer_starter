#!/usr/bin/env python3
"""Stdlib unittest coverage for runtime PNG cooking (Milestone 19)."""

from __future__ import annotations

import sys
import tempfile
import unittest
from io import BytesIO
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import cook_assets as cooker  # noqa: E402
from test_import_static_glb import copy_known_sources  # noqa: E402
from test_stage_runtime_assets import run_stage  # noqa: E402


def rgba_png_bytes(width: int, height: int, pixel) -> bytes:
    from PIL import Image

    image = Image.new("RGBA", (width, height), pixel)
    buffer = BytesIO()
    image.save(buffer, format="PNG", optimize=False, compress_level=6)
    return buffer.getvalue()


class ScaledDimensionsTests(unittest.TestCase):
    def test_within_limit_is_unchanged(self) -> None:
        self.assertEqual(cooker.scaled_png_dimensions(16, 16), (16, 16))
        self.assertEqual(cooker.scaled_png_dimensions(256, 256), (256, 256))
        self.assertEqual(cooker.scaled_png_dimensions(512, 256), (512, 256))

    def test_policy_examples(self) -> None:
        self.assertEqual(cooker.scaled_png_dimensions(1024, 1024), (512, 512))
        self.assertEqual(cooker.scaled_png_dimensions(1024, 512), (512, 256))
        self.assertEqual(cooker.scaled_png_dimensions(800, 600), (512, 384))

    def test_never_upscales(self) -> None:
        self.assertEqual(cooker.scaled_png_dimensions(16, 16, max_dimension=512), (16, 16))


class RuntimePngCookTests(unittest.TestCase):
    def test_checker_16x16_is_byte_identical(self) -> None:
        source_path = cooker.source_root(cooker.repo_root()) / "textures" / "test_checker.png"
        source_data = source_path.read_bytes()
        result = cooker.cook_runtime_png_bytes(source_data)
        self.assertEqual((result.source_width, result.source_height), (16, 16))
        self.assertEqual((result.cooked_width, result.cooked_height), (16, 16))
        self.assertFalse(result.resized)
        self.assertEqual(result.recipe, cooker.RUNTIME_PNG_RECIPE)
        self.assertEqual(result.cooked_data, source_data)

    def test_fixture_1024x512_becomes_512x256(self) -> None:
        fixture = TOOLS_DIR / "fixtures" / "textures" / "test_large_checker.png"
        source_data = fixture.read_bytes()
        self.assertEqual(cooker.read_png_dimensions(source_data), (1024, 512))
        result = cooker.cook_runtime_png_bytes(source_data)
        self.assertTrue(result.resized)
        self.assertEqual((result.cooked_width, result.cooked_height), (512, 256))
        self.assertEqual(
            cooker.read_png_dimensions(result.cooked_data),
            (512, 256),
        )
        self.assertEqual(
            result.cooked_width / result.cooked_height,
            result.source_width / result.source_height,
        )

    def test_synthetic_800x600_becomes_512x384(self) -> None:
        source_data = rgba_png_bytes(800, 600, (40, 80, 160, 255))
        result = cooker.cook_runtime_png_bytes(source_data)
        self.assertEqual((result.cooked_width, result.cooked_height), (512, 384))
        self.assertTrue(result.resized)

    def test_malformed_png_fails_clearly(self) -> None:
        with self.assertRaises(cooker.CookError) as truncated:
            cooker.cook_runtime_png_bytes(b"\x89PNG\r\n\x1a\n")
        self.assertIn("truncated PNG", str(truncated.exception))
        with self.assertRaises(cooker.CookError) as invalid:
            cooker.cook_runtime_png_bytes(b"not a png")
        self.assertIn("invalid signature", str(invalid.exception))

    def test_recipe_policy_change_changes_oversize_output(self) -> None:
        fixture = TOOLS_DIR / "fixtures" / "textures" / "test_large_checker.png"
        source_data = fixture.read_bytes()
        current = cooker.cook_runtime_png_bytes(source_data)
        tighter = cooker.cook_runtime_png_bytes(
            source_data,
            max_dimension=256,
            recipe="runtime_png.max256.lanczos.v2",
        )
        self.assertEqual(current.recipe, "runtime_png.max512.lanczos.v1")
        self.assertEqual(tighter.recipe, "runtime_png.max256.lanczos.v2")
        self.assertEqual((tighter.cooked_width, tighter.cooked_height), (256, 128))
        self.assertNotEqual(current.cooked_data, tighter.cooked_data)
        source_hash = cooker.sha256_bytes(source_data)
        previous = {
            "sourceSha256": source_hash,
            "recipe": current.recipe,
        }
        self.assertTrue(
            cooker.runtime_png_output_is_current(previous, source_hash, current.recipe)
        )
        self.assertFalse(
            cooker.runtime_png_output_is_current(previous, source_hash, tighter.recipe)
        )

    def test_copy_glbs_are_not_runtime_png(self) -> None:
        kinds = {asset["id"]: asset["kind"] for asset in cooker.KNOWN_ASSETS}
        self.assertEqual(kinds["models/test_static.glb"], cooker.KIND_COPY)
        self.assertEqual(kinds["models/player.glb"], cooker.KIND_COPY)
        self.assertEqual(kinds["models/test_authored.glb"], cooker.KIND_COPY)
        self.assertEqual(kinds["models/test_textured.glb"], cooker.KIND_COPY)
        self.assertEqual(kinds["textures/test_checker.png"], cooker.KIND_RUNTIME_PNG)
        self.assertNotIn("textures/test_textured_basecolor.png", kinds)
        glb_path = cooker.source_root(cooker.repo_root()) / "models" / "test_static.glb"
        source_data = glb_path.read_bytes()
        self.assertEqual(source_data[:4], b"glTF")
        self.assertNotEqual(source_data[:8], cooker.PNG_SIGNATURE)


class TerrainTextureDependencyTests(unittest.TestCase):
    def test_extracts_valid_terrain_texture_identity(self) -> None:
        text = (
            "PLATFORMER_LEVEL 1\n"
            "terrain_material textures/test_checker.png 0.25\n"
            "terrain_material - 1\n"
        )
        identities = cooker.extract_terrain_texture_identities(text)
        self.assertEqual(identities, ["textures/test_checker.png"])

    def test_rejects_absolute_and_model_identities(self) -> None:
        text = (
            "terrain_material C:/abs/grass.png 0.25\n"
            "terrain_material models/test_static.glb 0.25\n"
            "terrain_material textures/../x.png 0.25\n"
        )
        self.assertEqual(cooker.extract_terrain_texture_identities(text), [])

    def test_unrelated_source_png_is_not_discovered(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            copy_known_sources(root)
            sources = cooker.source_root(root)
            unrelated = sources / "textures" / "unrelated_dirt.png"
            unrelated.write_bytes(
                (cooker.source_root(cooker.repo_root()) / "textures" / "test_checker.png").read_bytes()
            )
            extras = cooker.discover_level_referenced_runtime_png_assets(sources)
            ids = [item["id"] for item in extras]
            self.assertNotIn("textures/unrelated_dirt.png", ids)
            collected = cooker.collect_cook_assets(sources)
            collected_ids = [item["id"] for item in collected]
            self.assertNotIn("textures/unrelated_dirt.png", collected_ids)
            self.assertIn("textures/test_checker.png", collected_ids)

    def test_level_referenced_png_cooks_and_stages(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            copy_known_sources(root)
            sources = cooker.source_root(root)
            extra_png = sources / "textures" / "terrain_detail.png"
            extra_png.write_bytes(
                (cooker.source_root(cooker.repo_root()) / "textures" / "test_checker.png").read_bytes()
            )
            extra_level = sources / "levels" / "level_terrain_tex.level"
            extra_level.write_text(
                "PLATFORMER_LEVEL 1\n"
                "id level_terrain_tex\n"
                "terrain_material textures/terrain_detail.png 0.25\n",
                encoding="utf-8",
            )
            collected = cooker.collect_cook_assets(sources)
            ids = [item["id"] for item in collected]
            self.assertIn("textures/terrain_detail.png", ids)
            self.assertEqual(ids.count("textures/terrain_detail.png"), 1)
            kinds = {item["id"]: item["kind"] for item in collected}
            self.assertEqual(kinds["textures/terrain_detail.png"], cooker.KIND_RUNTIME_PNG)

            self.assertEqual(cooker.cook(root), 0)
            cooked_png = cooker.cooked_root(root) / "textures" / "terrain_detail.png"
            self.assertTrue(cooked_png.is_file())
            dest = root / "staged" / "assets"
            stage = run_stage(cooker.cooked_root(root), dest, asset_list=None)
            self.assertEqual(stage.returncode, 0, stage.stderr + stage.stdout)
            self.assertTrue((dest / "textures" / "terrain_detail.png").is_file())
            self.assertTrue((dest / "textures" / "test_checker.png").is_file())
            self.assertFalse((dest / "textures" / "test_textured_basecolor.png").exists())

    def test_canonical_collect_does_not_require_authoring_png(self) -> None:
        collected = cooker.collect_cook_assets(cooker.source_root(cooker.repo_root()))
        ids = [item["id"] for item in collected]
        self.assertIn("textures/test_checker.png", ids)
        self.assertNotIn("textures/test_textured_basecolor.png", ids)

    def test_cook_imported_runtime_png_reuses_recipe(self) -> None:
        source = cooker.source_root(cooker.repo_root()) / "textures" / "test_checker.png"
        with tempfile.TemporaryDirectory() as tmp:
            cooked = Path(tmp) / "textures" / "imported_checker.png"
            result = cooker.cook_imported_runtime_png(source, cooked)
            self.assertEqual(result.recipe, "runtime_png.max512.lanczos.v1")
            self.assertFalse(result.resized)
            self.assertTrue(cooked.is_file())
            self.assertEqual(cooked.read_bytes(), source.read_bytes())

    def test_cook_runtime_png_cli_does_not_glob_source_textures(self) -> None:
        source = cooker.source_root(cooker.repo_root()) / "textures" / "test_checker.png"
        with tempfile.TemporaryDirectory() as tmp:
            cooked = Path(tmp) / "out.png"
            code = cooker.main(["--cook-runtime-png", str(source), str(cooked)])
            self.assertEqual(code, 0)
            self.assertTrue(cooked.is_file())
            self.assertEqual(cooker.main(["--cook-runtime-png"]), 2)

    def test_organization_metadata_is_not_cooked_or_staged(self) -> None:
        collected = cooker.collect_cook_assets(cooker.source_root(cooker.repo_root()))
        ids = [item["id"] for item in collected]
        self.assertNotIn("content_browser_organization.v1.txt", ids)
        self.assertTrue(
            (cooker.repo_root() / "game" / "assets" / "source" / "content_browser_organization.v1.txt").is_file()
        )


if __name__ == "__main__":
    unittest.main()
