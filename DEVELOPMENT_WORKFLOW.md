# Development Workflow

This file is the detailed lifecycle authority for milestone implementation.

[`AGENTS.md`](AGENTS.md) is the common entry point. It states the project rules and points here. Cursor rules, Cursor skills, `GEMINI.md`, and `.agents/rules/` are thin adapters. They discover these two documents. They are not additional copies of this workflow.

Canonical workflow marker: project policy, milestone lifecycle, validation commands, and STOP conditions have one authority, this file, together with `AGENTS.md` as the entry point.

If an adapter disagrees with `AGENTS.md` or this file, stop and report the conflict.

## Orientation

A new coding agent should answer these questions from the repository before editing:

1. What project is this? A C++ 3D platformer. See [`AGENTS.md`](AGENTS.md) and [`README.md`](README.md).
2. What is the source-of-truth order? The list in the next section, also stated in `AGENTS.md`.
3. What milestone is active? The milestone named by the current task. The repository pointer is the Current milestone section of `AGENTS.md`.
4. Which branch should be used? The branch named in that milestone file. Confirm it with `git branch --show-current` before editing.
5. Where is the milestone definition? `docs/milestones/MILESTONE_<N>.md`. Decimals use underscores: Milestone 58.4 is `docs/milestones/MILESTONE_58_4.md`.
6. How is the project configured? `cmake --preset windows-vs2022`, from [`CMakePresets.json`](CMakePresets.json). CMake is the build definition. Do not hand-author `.sln` or `.vcxproj` files.
7. How is it built? The build presets below.
8. Which tests must be run? The default validation below, plus every test the active milestone requires.
9. Which canonical authored files require special protection? `game/assets/source/levels/level_01.level` and `game/assets/source/levels/level_02.level`.
10. Who performs manual acceptance? The User.
11. Is the coding agent allowed to commit? No, not as part of implementation.
12. Is it allowed to push? No, not as part of implementation.
13. Is it allowed to merge? No, not as part of implementation.
14. When must it STOP? After the completion report. Do not close the milestone or begin the next one.

[`docs/MILESTONES.md`](docs/MILESTONES.md) is a compact index. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) describes current architecture. Historical milestone files do not override current code, tests, or current docs.

## Source of truth

1. Current repository code.
2. Tests.
3. Current architecture/docs.
4. Active milestone file.
5. Latest relevant checkpoint/current documentation.
6. Older milestone files/history.

Historical milestone files do not override current implemented behavior.

Inspect current code, tests, and current docs before implementation. Read another milestone file only for a dependency, historical decision, or ambiguity. Do not load the historical corpus by default.

## Responsibilities

### User

The User:

- chooses project direction;
- runs and observes local game behavior when required;
- performs manual acceptance;
- explicitly approves milestone completion;
- performs or authorizes Git closure.

### ChatGPT

ChatGPT:

- discusses architecture and scope;
- defines milestones;
- prepares implementation and correction prompts;
- reviews coding-agent reports;
- evaluates manual acceptance feedback;
- provides Git closure only after approval.

### Coding agent

Cursor, Codex, Antigravity, or another implementation agent:

- inspects the repository;
- follows `AGENTS.md`, this file, and the active milestone;
- implements only the current scope;
- validates;
- reports;
- stops.

The coding agent does not become project authority because it can run commands or edit files.

## Milestone lifecycle

1. Identify the single active milestone from the user prompt (`docs/milestones/MILESTONE_<N>.md`).
2. Read that file. Use `docs/MILESTONES.md` only as an index when the prompt did not name a file.
3. Check out or confirm the branch named in the milestone. Do not mix unrelated milestone work on that branch.
4. Inspect the current repository. State a short plan limited to that milestone.
5. Implement only that milestone. Do not take on later milestones, drive-by refactors, or opportunistic features.
6. Keep the tree buildable. Do not mark the milestone complete if the build is broken.
7. Run default validation and the milestone's extra checks.
8. Before the report, run the pre-report Git safety commands below.
9. Write the completion report.
10. STOP.

## Required validation

Run these commands from the repository root unless the active milestone explicitly narrows the set. A narrower set must be written in that milestone file. When the milestone adds checks, run those too.

Configure and build:

```bash
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Python checks that protect milestone docs, agent instructions, cooking, and staging:

```bash
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

C++ test executables are described in [`README.md`](README.md). Run the ones that cover the code the milestone touches. Performance conclusions must not be drawn from Debug builds.

Cook runtime assets with `python tools/cook_assets.py` before configure when cooked outputs are missing. See [`README.md`](README.md) and [`tools/README.md`](tools/README.md).

## Canonical Level safety

Do not modify these authored files unless the active milestone explicitly requires a Level change:

- `game/assets/source/levels/level_01.level`
- `game/assets/source/levels/level_02.level`

Do not edit cooked or staged copies as if they were source. Do not normalize line endings because Git reports CRLF/LF warnings.

If `git diff` shows an existing change in either canonical Level that this milestone did not intend, STOP and report it. Do not overwrite it.

## Pre-report Git safety

```bash
git branch --show-current
git status
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

Both Level diffs should be empty unless a pre-existing user change was found, in which case stop and report that change.

Do not repair unrelated user changes.

## Completion report

Report at least:

- files changed and the behavior or documentation implemented;
- commands used to configure, build, and test, with results;
- canonical Level diff status for `level_01.level` and `level_02.level`;
- `git diff --check` result;
- remaining manual acceptance;
- known limitations;
- confirmation that implementation stopped before commit, push, merge, milestone closure, and the next milestone.

The active milestone may require additional report items. Include those.

## Portable milestone prompt

Future implementation prompts can stay short because repository-wide policy lives here and in `AGENTS.md`. Complex corrections may still need detailed instructions. This template does not replace milestone-specific requirements.

```text
Implement Milestone XX as defined in:

docs/milestones/MILESTONE_XX.md

Follow AGENTS.md and the repository development workflow as authoritative instructions.

Inspect the current repository before implementation.
Preserve existing architecture and milestone boundaries.
Run all validation required by the milestone and repository workflow.

Report implementation, tests, builds, canonical-data status, and remaining manual acceptance.

STOP after reporting.
Do not commit, push, merge, close the milestone, or begin the next milestone.
```

## Coding-agent entry points

| Role | File | What it is |
| --- | --- | --- |
| Common entry point | `AGENTS.md` | Project rules and routes into this workflow. Codex loads it automatically. Antigravity and Cursor load it as repository instructions. |
| Lifecycle authority | `DEVELOPMENT_WORKFLOW.md` | This file. One detailed workflow. |
| Active work | `docs/milestones/MILESTONE_<N>.md` | Scope, branch, validation, and acceptance for one milestone. |
| Index | `docs/MILESTONES.md` | Navigation and status only. |
| Current architecture | `docs/ARCHITECTURE.md` | Current behavior. Historical milestones do not override it. |
| Cursor adapter | `.cursor/rules/` and `.cursor/skills/` | Editor rules and skills. `.cursor/rules/00-project-core.mdc` and `.cursor/skills/implement-milestone/SKILL.md` point here. |
| Codex | `AGENTS.md` | No Codex-only workflow file. Codex discovers repository instructions through `AGENTS.md`. |
| Antigravity adapter | `GEMINI.md` and `.agents/rules/repository-workflow.md` | Pointers only. Antigravity also reads `AGENTS.md`. |

Do not add `CURSOR_WORKFLOW.md`, `CODEX_WORKFLOW.md`, or `ANTIGRAVITY_WORKFLOW.md`.

Nested `AGENTS.md` files are appropriate only when a directory has genuinely local instructions. Do not add them for symmetry.

### Cursor

Open the repository root. Cursor loads `AGENTS.md`, `.cursor/rules/*.mdc`, and `.cursor/skills/*/SKILL.md`.

The implementation skill tells Cursor to follow `AGENTS.md` and this file: inspect, implement the active milestone, validate, report, and STOP without commit, push, or merge.

### Codex

Codex reads `AGENTS.md` from the repository root before it works. No extra Codex configuration is required for this project.

A Codex session still needs to open `DEVELOPMENT_WORKFLOW.md` and the active milestone file. `AGENTS.md` requires that.

### Antigravity

Antigravity's documented workspace instructions include `AGENTS.md` and `GEMINI.md` at the repository root, plus workspace rules in `.agents/rules/`.

`GEMINI.md` is a short pointer. `.agents/rules/repository-workflow.md` is an always-on rule whose trigger is `always_on`. It references `@/AGENTS.md` and `@/DEVELOPMENT_WORKFLOW.md` instead of copying them. The supported rule location is `.agents/rules/` (`.agent/rules/` remains a legacy path and is not used here).

## Cross-agent bootstrap smoke test

This check is read-only. The agent must not modify, create, or delete repository files, and must not commit, push, or merge.

Give a fresh agent only this prompt:

```text
Inspect this repository and report only.
Do not modify, create, or delete any file.
Do not commit, push, or merge.

Report:
- authoritative instruction files
- current branch
- active milestone
- required build commands
- relevant test commands
- canonical Level protection rules
- whether you may commit, push, or merge
- manual acceptance owner
- STOP condition
```

Expected discoveries:

- authoritative files include `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md`;
- the active milestone and branch match `AGENTS.md` and `docs/milestones/MILESTONE_<N>.md`;
- build commands are the CMake presets in this file;
- test commands include `python tools/test_agent_instructions.py` and the other default Python checks;
- `level_01.level` and `level_02.level` under `game/assets/source/levels/` are protected;
- the agent may not commit, push, or merge;
- the User performs manual acceptance;
- the agent stops after the report.

How to run it:

- Cursor: open the repository root and send the prompt, or invoke the agent with that prompt alone.
- Codex: open the repository root so Codex loads `AGENTS.md`, then send the prompt.
- Antigravity: open the repository root so it loads `AGENTS.md`, `GEMINI.md`, and `.agents/rules/repository-workflow.md`, then send the prompt.

Record which providers actually ran. Do not treat an unread procedure as a passed provider test.

## Out of scope for this workflow

This workflow does not authorize gameplay, renderer, physics, editor, Level Format, or asset changes by itself. Those belong to an active milestone that explicitly includes them.

Do not add provider routing, cloud-agent infrastructure, autonomous merging, API keys, or a vendor SDK whose only purpose is instruction compatibility.
