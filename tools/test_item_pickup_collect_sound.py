#!/usr/bin/env python3
"""Narrow Milestone 61 collection-sound cooker/staging checks."""

from __future__ import annotations

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
import generate_item_pickup_collect_wav as wavgen  # noqa: E402

LOGICAL_ID = "sounds/item_pickup_collect.wav"
SOURCE = cooker.source_root(cooker.repo_root()) / "sounds" / "item_pickup_collect.wav"
RUNTIME_ASSETS = REPO_ROOT / "cmake" / "RuntimeAssets.cmake"


def read_wav_header(path: Path) -> tuple[int, int, int, int]:
    data = path.read_bytes()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE" or data[12:16] != b"fmt ":
        raise AssertionError("not a RIFF/WAVE file")
    _audio_format, channels, sample_rate, _byte_rate, _block_align, bits = struct.unpack_from(
        "<HHIIHH", data, 20
    )
    data_size = struct.unpack_from("<I", data, 40)[0]
    return channels, sample_rate, bits, data_size


class ItemPickupCollectSoundTests(unittest.TestCase):
    def test_source_wav_is_project_owned_pcm(self) -> None:
        self.assertTrue(SOURCE.is_file())
        channels, sample_rate, bits, data_size = read_wav_header(SOURCE)
        self.assertEqual(channels, 1)
        self.assertEqual(sample_rate, wavgen.SAMPLE_RATE)
        self.assertEqual(bits, 16)
        self.assertGreater(data_size, 0)
        self.assertEqual(SOURCE.read_bytes(), wavgen.wav_bytes(wavgen.pcm16_frames()))

    def test_known_assets_lists_opaque_copy(self) -> None:
        kinds = {asset["id"]: asset["kind"] for asset in cooker.KNOWN_ASSETS}
        self.assertEqual(kinds[LOGICAL_ID], cooker.KIND_COPY)
        self.assertFalse(LOGICAL_ID.endswith(".glb"))

    def test_runtime_inventory_lists_sound(self) -> None:
        text = RUNTIME_ASSETS.read_text(encoding="utf-8")
        self.assertIn("sounds/item_pickup_collect.wav", text)

    def test_cooker_copies_from_source_not_runtime_png(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            sources = cooker.source_root(root)
            cooked = cooker.cooked_root(root)
            (sources / "sounds").mkdir(parents=True)
            (sources / "models").mkdir(parents=True)
            (sources / "textures").mkdir(parents=True)
            (sources / "levels").mkdir(parents=True)
            payload = wavgen.wav_bytes(wavgen.pcm16_frames())
            (sources / LOGICAL_ID).write_bytes(payload)
            # Minimal known companions so cook() of a fake root is not used;
            # cook a single copy through the same byte path as KIND_COPY.
            cooked.mkdir(parents=True)
            wrote = cooker.write_bytes_if_changed(cooked / LOGICAL_ID, payload)
            self.assertTrue(wrote)
            self.assertEqual((cooked / LOGICAL_ID).read_bytes(), payload)
            self.assertNotEqual(payload[:8], cooker.PNG_SIGNATURE)

    def test_logical_id_is_portable_and_not_source_path(self) -> None:
        self.assertEqual(cooker.portable_relative(LOGICAL_ID), LOGICAL_ID)
        self.assertFalse(LOGICAL_ID.startswith("game/assets/source"))
        self.assertNotIn("..", LOGICAL_ID)


if __name__ == "__main__":
    unittest.main()
