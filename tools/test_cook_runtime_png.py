#!/usr/bin/env python3
"""Stdlib unittest coverage for runtime PNG cooking (Milestone 19)."""

from __future__ import annotations

import sys
import unittest
from io import BytesIO
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import cook_assets as cooker  # noqa: E402


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
        self.assertEqual(kinds["models/test_authored.glb"], cooker.KIND_COPY)
        self.assertEqual(kinds["models/test_textured.glb"], cooker.KIND_COPY)
        self.assertEqual(kinds["textures/test_checker.png"], cooker.KIND_RUNTIME_PNG)
        self.assertNotIn("textures/test_textured_basecolor.png", kinds)
        glb_path = cooker.source_root(cooker.repo_root()) / "models" / "test_static.glb"
        source_data = glb_path.read_bytes()
        self.assertEqual(source_data[:4], b"glTF")
        self.assertNotEqual(source_data[:8], cooker.PNG_SIGNATURE)


if __name__ == "__main__":
    unittest.main()
