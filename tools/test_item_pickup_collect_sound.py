#!/usr/bin/env python3
"""Narrow Milestone 61/71 gameplay-SFX cooker/staging checks."""

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
import generate_gameplay_sfx_wav as sfxgen  # noqa: E402
import generate_item_pickup_collect_wav as wavgen  # noqa: E402

PICKUP_ID = "sounds/item_pickup_collect.wav"
DAMAGE_ID = "sounds/player_damage.wav"
DEATH_ID = "sounds/player_death.wav"
RESPAWN_ID = "sounds/player_respawn.wav"
ALL_SOUND_IDS = (PICKUP_ID, DAMAGE_ID, DEATH_ID, RESPAWN_ID)
SOURCE_SOUNDS = cooker.source_root(cooker.repo_root()) / "sounds"
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
        source = SOURCE_SOUNDS / "item_pickup_collect.wav"
        self.assertTrue(source.is_file())
        channels, sample_rate, bits, data_size = read_wav_header(source)
        self.assertEqual(channels, 1)
        self.assertEqual(sample_rate, wavgen.SAMPLE_RATE)
        self.assertEqual(bits, 16)
        self.assertGreater(data_size, 0)
        self.assertEqual(source.read_bytes(), wavgen.wav_bytes(wavgen.pcm16_frames()))

    def test_m71_source_wavs_are_project_owned_pcm(self) -> None:
        expected = {
            DAMAGE_ID: sfxgen.cue_wav_bytes(sfxgen.DAMAGE_NAME),
            DEATH_ID: sfxgen.cue_wav_bytes(sfxgen.DEATH_NAME),
            RESPAWN_ID: sfxgen.cue_wav_bytes(sfxgen.RESPAWN_NAME),
        }
        for logical_id, payload in expected.items():
            source = cooker.source_root(cooker.repo_root()) / logical_id
            self.assertTrue(source.is_file(), logical_id)
            channels, sample_rate, bits, data_size = read_wav_header(source)
            self.assertEqual(channels, 1, logical_id)
            self.assertEqual(sample_rate, sfxgen.SAMPLE_RATE, logical_id)
            self.assertEqual(bits, 16, logical_id)
            self.assertGreater(data_size, 0, logical_id)
            self.assertEqual(source.read_bytes(), payload, logical_id)
            self.assertNotEqual(payload, wavgen.wav_bytes(wavgen.pcm16_frames()), logical_id)

    def test_known_assets_lists_opaque_copy(self) -> None:
        kinds = {asset["id"]: asset["kind"] for asset in cooker.KNOWN_ASSETS}
        for logical_id in ALL_SOUND_IDS:
            self.assertEqual(kinds[logical_id], cooker.KIND_COPY)
            self.assertFalse(logical_id.endswith(".glb"))

    def test_runtime_inventory_lists_sound(self) -> None:
        text = RUNTIME_ASSETS.read_text(encoding="utf-8")
        for logical_id in ALL_SOUND_IDS:
            self.assertIn(logical_id, text)
        self.assertNotIn("game/assets/source", text)

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
            (sources / PICKUP_ID).write_bytes(payload)
            cooked.mkdir(parents=True)
            wrote = cooker.write_bytes_if_changed(cooked / PICKUP_ID, payload)
            self.assertTrue(wrote)
            self.assertEqual((cooked / PICKUP_ID).read_bytes(), payload)
            self.assertNotEqual(payload[:8], cooker.PNG_SIGNATURE)

    def test_logical_id_is_portable_and_not_source_path(self) -> None:
        for logical_id in ALL_SOUND_IDS:
            self.assertEqual(cooker.portable_relative(logical_id), logical_id)
            self.assertFalse(logical_id.startswith("game/assets/source"))
            self.assertNotIn("..", logical_id)


if __name__ == "__main__":
    unittest.main()
