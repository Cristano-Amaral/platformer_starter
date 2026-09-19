#!/usr/bin/env python3
"""Milestone 85: lighting shaders cook/stage as required runtime assets."""

from __future__ import annotations

import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
REPO_ROOT = TOOLS_DIR.parent
if str(TOOLS_DIR) not in __import__("sys").path:
    __import__("sys").path.insert(0, str(TOOLS_DIR))

import cook_assets as cooker  # noqa: E402


SHADER_IDS = (
    "shaders/world_lit.vs",
    "shaders/world_lit.fs",
    "shaders/shadow_depth.vs",
    "shaders/shadow_depth.fs",
)


class WorldShaderStagingTests(unittest.TestCase):
    def test_shaders_are_required_copy_assets(self) -> None:
        kinds = {asset["id"]: asset["kind"] for asset in cooker.KNOWN_ASSETS}
        for identity in SHADER_IDS:
            self.assertEqual(kinds[identity], cooker.KIND_COPY, identity)

    def test_runtime_assets_cmake_lists_shaders(self) -> None:
        text = (REPO_ROOT / "cmake" / "RuntimeAssets.cmake").read_text(encoding="utf-8")
        for identity in SHADER_IDS:
            self.assertIn(identity, text)

    def test_cooked_shaders_exist_and_match_source(self) -> None:
        source = cooker.source_root(cooker.repo_root())
        cooked = cooker.cooked_root(cooker.repo_root())
        for identity in SHADER_IDS:
            src = source / identity
            dst = cooked / identity
            self.assertTrue(src.is_file(), f"missing source {identity}")
            self.assertTrue(dst.is_file(), f"missing cooked {identity}")
            self.assertEqual(src.read_bytes(), dst.read_bytes(), identity)

    def test_cpp_has_no_source_shader_fallback(self) -> None:
        render_dir = REPO_ROOT / "game" / "source" / "render"
        blobs = []
        for path in render_dir.glob("WorldLighting*.cpp"):
            blobs.append(path.read_text(encoding="utf-8"))
        combined = "\n".join(blobs)
        self.assertIn("RuntimeAssetPath", combined)
        self.assertNotIn("game/assets/source/shaders", combined)
        self.assertNotIn("AuthoringSourceRoot", combined)


if __name__ == "__main__":
    unittest.main()
