#!/usr/bin/env python3
"""Header-gate tests for cooker level_v1 kind (Milestone 31 Phase A)."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import cook_assets as cooker  # noqa: E402


class LevelV1HeaderTests(unittest.TestCase):
    def test_canonical_source_header_is_accepted(self) -> None:
        source = cooker.source_root(cooker.repo_root()) / "levels" / "level_01.level"
        cooker.validate_level_v1_header(source.read_bytes())

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


if __name__ == "__main__":
    unittest.main(verbosity=2)
