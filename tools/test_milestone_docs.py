#!/usr/bin/env python3
"""Narrow path checks for split milestone documentation (Milestone 60)."""

from __future__ import annotations

import re
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
REPO_ROOT = TOOLS_DIR.parent
INDEX = REPO_ROOT / "docs" / "MILESTONES.md"
MILESTONES_DIR = REPO_ROOT / "docs" / "milestones"

DECIMAL_MILESTONES = ("48.1", "48.2", "57.1", "58.1", "58.2", "58.3", "58.4", "64.1")
INTEGER_MILESTONES = tuple(range(0, 84))
HEADING = re.compile(r"^(#{1,2}) Milestone (\d+(?:\.\d+)?)(.*)$", re.M)
MD_LINK = re.compile(r"\[[^\]]+\]\(([^)]+)\)")


def canonical_filename(number: str) -> str:
    if "." in number:
        major, minor = number.split(".", 1)
        return f"MILESTONE_{int(major)}_{minor}.md"
    return f"MILESTONE_{int(number)}.md"


class MilestoneDocsTests(unittest.TestCase):
    def test_canonical_directory_is_lowercase_milestones(self) -> None:
        self.assertTrue(MILESTONES_DIR.is_dir())
        self.assertEqual(MILESTONES_DIR.name, "milestones")

    def test_expected_canonical_files_exist(self) -> None:
        expected = {canonical_filename(str(n)) for n in INTEGER_MILESTONES}
        expected.update(canonical_filename(n) for n in DECIMAL_MILESTONES)
        actual = {p.name for p in MILESTONES_DIR.glob("MILESTONE_*.md")}
        self.assertEqual(expected, actual)

    def test_no_dotted_decimal_filenames(self) -> None:
        dotted = list(MILESTONES_DIR.glob("MILESTONE_*.*.md"))
        self.assertEqual(dotted, [])

    def test_m59_and_m60_paths(self) -> None:
        m59 = MILESTONES_DIR / "MILESTONE_59.md"
        m60 = MILESTONES_DIR / "MILESTONE_60.md"
        self.assertTrue(m59.is_file())
        self.assertTrue(m60.is_file())
        self.assertIn("Milestone 59", m59.read_text(encoding="utf-8"))
        self.assertIn("Milestone 60", m60.read_text(encoding="utf-8"))
        self.assertNotIn("## Milestone 61", m60.read_text(encoding="utf-8"))

    def test_each_file_contains_only_its_milestone_heading(self) -> None:
        for path in sorted(MILESTONES_DIR.glob("MILESTONE_*.md")):
            text = path.read_text(encoding="utf-8")
            matches = HEADING.findall(text)
            self.assertEqual(len(matches), 1, msg=path.name)
            number = matches[0][1]
            self.assertEqual(canonical_filename(number), path.name)

    def test_index_is_compact_and_does_not_contain_corpus(self) -> None:
        text = INDEX.read_text(encoding="utf-8")
        self.assertLess(len(text.splitlines()), 250)
        self.assertNotIn("Integrate raylib and establish the minimum runtime foundation", text)
        self.assertNotIn("## Milestone 01 — Window and Game Loop", text)
        self.assertIn("docs/milestones/", text)
        self.assertIn("MILESTONE_<N>.md", text)
        self.assertIn("MILESTONE_58_4.md", text)

    def test_index_links_resolve(self) -> None:
        text = INDEX.read_text(encoding="utf-8")
        missing = []
        for target in MD_LINK.findall(text):
            if target.startswith(("http://", "https://", "#")):
                continue
            dest = (INDEX.parent / target).resolve()
            if not dest.exists():
                missing.append(target)
        self.assertEqual(missing, [])


if __name__ == "__main__":
    unittest.main()
