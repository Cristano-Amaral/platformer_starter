#!/usr/bin/env python3
"""Narrow consistency checks for repository agent instructions (Milestone 91)."""

from __future__ import annotations

import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
REPO_ROOT = TOOLS_DIR.parent

AGENTS = REPO_ROOT / "AGENTS.md"
WORKFLOW = REPO_ROOT / "DEVELOPMENT_WORKFLOW.md"

CANONICAL_FILES = (
    "AGENTS.md",
    "DEVELOPMENT_WORKFLOW.md",
    "README.md",
    "CMakePresets.json",
    "docs/MILESTONES.md",
    "docs/ARCHITECTURE.md",
    "docs/milestones/MILESTONE_91.md",
    "docs/milestones/MILESTONE_92.md",
    "game/assets/source/levels/level_01.level",
    "game/assets/source/levels/level_02.level",
)

ADAPTERS = (
    ".cursor/rules/00-project-core.mdc",
    ".cursor/skills/implement-milestone/SKILL.md",
    "GEMINI.md",
    ".agents/rules/repository-workflow.md",
)

FORBIDDEN_WORKFLOW_COPIES = (
    "CURSOR_WORKFLOW.md",
    "CODEX_WORKFLOW.md",
    "ANTIGRAVITY_WORKFLOW.md",
)

SOURCE_OF_TRUTH = (
    "1. Current repository code.",
    "2. Tests.",
    "3. Current architecture/docs.",
    "4. Active milestone file.",
    "5. Latest relevant checkpoint/current documentation.",
    "6. Older milestone files/history.",
)

BUILD_COMMANDS = (
    "cmake --preset windows-vs2022",
    "cmake --build --preset windows-debug",
    "cmake --build --preset windows-development",
    "cmake --build --preset windows-release",
)

PYTHON_CHECKS = (
    "python tools/test_milestone_docs.py",
    "python tools/test_agent_instructions.py",
    "python tools/test_stage_runtime_assets.py",
    "python tools/test_cook_level_v1.py",
    "python tools/test_cook_runtime_png.py",
    "python tools/test_import_static_glb.py",
    "python tools/test_stage_world_shaders.py",
)

PORTABLE_PROMPT_LINES = (
    "Implement Milestone XX as defined in:",
    "docs/milestones/MILESTONE_XX.md",
    "Follow AGENTS.md and the repository development workflow as authoritative instructions.",
    "STOP after reporting.",
    "Do not commit, push, merge, close the milestone, or begin the next milestone.",
)

WORKFLOW_MARKER = "Canonical workflow " + "marker:"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class AgentInstructionTests(unittest.TestCase):
    def test_canonical_files_exist(self) -> None:
        missing = [rel for rel in CANONICAL_FILES if not (REPO_ROOT / rel).is_file()]
        self.assertEqual(missing, [])

    def test_agents_routes_to_workflow_and_milestone(self) -> None:
        text = read(AGENTS)
        self.assertIn("DEVELOPMENT_WORKFLOW.md", text)
        self.assertIn("docs/MILESTONES.md", text)
        self.assertIn("docs/ARCHITECTURE.md", text)
        self.assertIn("docs/milestones/MILESTONE_", text)
        self.assertIn("docs/milestones/MILESTONE_92.md", text)
        self.assertIn("milestone/92-scalable-terrain-material-palette", text)
        self.assertIn("level_01.level", text)
        self.assertIn("level_02.level", text)
        self.assertIn("manual acceptance", text.lower())
        self.assertIn("STOP", text)
        self.assertIn("Do not commit", text)
        self.assertIn("push", text)
        self.assertIn("merge", text)

    def test_source_of_truth_matches_agents_and_workflow(self) -> None:
        agents = read(AGENTS)
        workflow = read(WORKFLOW)
        for line in SOURCE_OF_TRUTH:
            self.assertIn(line, agents)
            self.assertIn(line, workflow)

    def test_workflow_owns_commands_prompt_and_smoke_test(self) -> None:
        text = read(WORKFLOW)
        for command in BUILD_COMMANDS + PYTHON_CHECKS:
            self.assertIn(command, text)
        for line in PORTABLE_PROMPT_LINES:
            self.assertIn(line, text)
        self.assertIn("game/assets/source/levels/level_01.level", text)
        self.assertIn("game/assets/source/levels/level_02.level", text)
        self.assertIn("git diff --check", text)
        self.assertIn("The User", text)
        self.assertIn("Do not modify, create, or delete any file.", text)
        self.assertIn("Cursor", text)
        self.assertIn("Codex", text)
        self.assertIn("Antigravity", text)
        self.assertEqual(text.count(WORKFLOW_MARKER), 1)
        self.assertEqual(text.count("Implement Milestone XX as defined in:"), 1)

    def test_workflow_marker_is_not_copied(self) -> None:
        copies = []
        candidates = [REPO_ROOT / name for name in CANONICAL_FILES]
        candidates.extend(REPO_ROOT / rel for rel in ADAPTERS)
        candidates.extend((REPO_ROOT / "docs").rglob("*.md"))
        candidates.extend((REPO_ROOT / "tools").glob("*.py"))
        candidates.extend((REPO_ROOT / "tools").glob("*.md"))
        seen: set[Path] = set()
        for path in candidates:
            if not path.is_file() or path.resolve() in seen:
                continue
            seen.add(path.resolve())
            if path.resolve() == WORKFLOW.resolve() or path.resolve() == Path(__file__).resolve():
                continue
            if WORKFLOW_MARKER in read(path):
                copies.append(str(path.relative_to(REPO_ROOT)))
        self.assertEqual(copies, [])

    def test_provider_workflow_files_are_absent(self) -> None:
        present = [name for name in FORBIDDEN_WORKFLOW_COPIES if (REPO_ROOT / name).exists()]
        self.assertEqual(present, [])

    def test_adapters_reference_canonical_docs_and_stay_thin(self) -> None:
        for rel in ADAPTERS:
            path = REPO_ROOT / rel
            self.assertTrue(path.is_file(), msg=rel)
            text = read(path)
            self.assertIn("AGENTS.md", text, msg=rel)
            self.assertIn("DEVELOPMENT_WORKFLOW.md", text, msg=rel)
            self.assertLessEqual(len(text.splitlines()), 60, msg=rel)
            self.assertNotIn("cmake --build --preset", text, msg=rel)
            self.assertNotIn("Implement Milestone XX", text, msg=rel)
            self.assertNotIn(WORKFLOW_MARKER, text, msg=rel)
            self.assertIn("STOP", text, msg=rel)
            self.assertIn("commit", text, msg=rel)

    def test_antigravity_rule_uses_supported_trigger(self) -> None:
        text = read(REPO_ROOT / ".agents/rules/repository-workflow.md")
        self.assertIn("trigger: always_on", text)
        self.assertIn("@/AGENTS.md", text)
        self.assertIn("@/DEVELOPMENT_WORKFLOW.md", text)

    def test_milestone_91_file_is_in_the_canonical_directory(self) -> None:
        path = REPO_ROOT / "docs/milestones/MILESTONE_91.md"
        text = read(path)
        self.assertIn("Milestone 91", text)
        self.assertNotIn("MILESTONE_91.1.md", text)


if __name__ == "__main__":
    unittest.main()
