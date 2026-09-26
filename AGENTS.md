# Project Instructions — 3D Platformer

## Project goal
Build a C++ platform game with 3D visuals and a platformer-style camera. Windows is the first shipping target. Architecture must preserve a practical path to Linux/Raspberry Pi, Android, and iOS.

## Authority
The repository is authoritative. The coding agent is replaceable.

This file is the common entry point for Cursor, OpenAI Codex, Google Antigravity, and any other coding agent. The detailed lifecycle is [`DEVELOPMENT_WORKFLOW.md`](DEVELOPMENT_WORKFLOW.md). Read that file before implementing a milestone.

Tool-specific files are adapters. They must not contradict this file or `DEVELOPMENT_WORKFLOW.md`, and they must not carry a second copy of the development workflow.

## Source of truth
1. Current repository code.
2. Tests.
3. Current architecture/docs.
4. Active milestone file.
5. Latest relevant checkpoint/current documentation.
6. Older milestone files/history.

Historical milestone files do not override current implemented behavior.

Inspect the current repository before implementation. Do not assume chat history or an older milestone file matches the code.

## Active milestone
The user prompt names the single active milestone. Work on that milestone's branch only.

Canonical definition: `docs/milestones/MILESTONE_<N>.md`. Decimal milestones use an underscore in the filename (`58.4` → `MILESTONE_58_4.md`).

Use [`docs/MILESTONES.md`](docs/MILESTONES.md) only as a compact index. Do not load every historical milestone.

Current milestone: Milestone 105 — Reusable Animation Assets & Animation Library Foundation.

Canonical file: [`docs/milestones/MILESTONE_105.md`](docs/milestones/MILESTONE_105.md)

Branch: `milestone/105-reusable-animation-assets`

Status: implemented, awaiting manual acceptance. Do not start Milestone 106.

Earlier milestone status, including Milestone 99 awaiting manual acceptance, is recorded in [`docs/MILESTONES.md`](docs/MILESTONES.md).

Current architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)

## Responsibilities
- User: chooses direction, observes local behavior, performs manual acceptance, approves completion, and performs or authorizes Git closure.
- ChatGPT: discusses architecture, defines milestones and prompts, reviews reports, and provides Git closure only after approval.
- Coding agent: inspects, implements the active milestone, validates, reports, and stops.

Repository access does not grant the coding agent project authority.

## Milestone work
1. Read the named `docs/milestones/MILESTONE_<N>.md`. Use `docs/MILESTONES.md` only as an index.
2. Inspect current code, tests, and current docs. Read other milestone files only for a dependency, historical decision, or ambiguity.
3. Implement only that milestone. Do not implement a future milestone while completing the current one.
4. Run the validation required by the milestone and by [`DEVELOPMENT_WORKFLOW.md`](DEVELOPMENT_WORKFLOW.md).
5. Report and STOP.

Do not mark a milestone complete if the build is broken. Do not close a milestone during implementation.

## Canonical Level safety
Do not modify `game/assets/source/levels/level_01.level` or `game/assets/source/levels/level_02.level` unless the active milestone explicitly requires it.

Do not normalize line endings because Git reports CRLF/LF warnings. If either canonical Level already has a user change, stop and report it instead of overwriting it.

## STOP
After the completion report, STOP.

Do not commit, push, merge, close the milestone, or begin the next milestone as part of implementation.

Manual acceptance and Git closure are separate steps. They belong to the User. ChatGPT provides Git closure only after explicit approval.

## Non-negotiable engineering principles
1. Work in small, testable, compilable milestones.
2. Do not implement future milestones while completing the current one.
3. Keep gameplay code platform-agnostic.
4. Platform/library-specific APIs must stay behind adapters/interfaces.
5. CMake is the canonical build definition. IDE project files are generated artifacts and must not be hand-maintained.
6. Every meaningful change must leave the repository in a buildable state.
7. Prefer simple code over speculative abstractions, except at platform boundaries where abstraction is intentional.
8. Before adding a subsystem or dependency, check whether an equivalent already exists.

## Initial technology choices
- Language: C++20 for game code.
- Build: CMake 3.25+.
- Windows compiler: Visual Studio 2022 / MSVC.
- Linux/Raspberry Pi compiler: GCC or Clang.
- Graphics/window/input backend for initial Windows build: raylib.
- Math: GLM when needed.
- Physics: Jolt Physics when the physics milestone begins.
- Debug/editor UI: Dear ImGui, Development/Debug builds only.
- Tools/asset pipeline: Python 3.

## Architecture boundaries
Game code must not include platform OS headers or call OS APIs directly.

Use these layers:
- `game/source/core/`: application loop, timing, config, logging, shared infrastructure.
- `game/source/platform/`: platform and backend adapters (window, input, filesystem, timing, lifecycle).
- `game/source/gameplay/`: player, movement, camera behavior, levels, game rules.
- `game/source/render/`: renderer-facing abstractions and rendering systems.
- `game/source/physics/`: physics integration/adapters.
- `game/source/ui/`: HUD, menus, debug UI.

Avoid calling raylib directly from gameplay code. If a feature requires raylib, expose the smallest useful wrapper in the appropriate platform/render layer.

## Portability rules
- Paths must use `std::filesystem`.
- Do not assume drive letters, backslashes, current working directory, keyboard availability, mouse availability, or a desktop window.
- Input gameplay actions must be semantic (`MoveLeft`, `Jump`, `Pause`) rather than hardwired keys outside the input backend.
- Do not put Win32-specific code outside a clearly named Windows implementation file.
- File names and asset paths are case-sensitive by convention, even on Windows.
- Avoid undefined behavior and compiler-specific extensions unless isolated behind a portability layer.
- Do not allocate per-frame unless justified and measured.

## Code style
- Classes/structs/functions/methods/files: PascalCase.
- Local variables and data members: camelCase.
- Constants: kPascalCase.
- Macros: ALL_UPPER_CASE and only when necessary.
- Namespaces: short lowercase names.
- Prefer `#pragma once` for new headers.
- RAII is required for owning resources.
- Prefer value semantics and `std::unique_ptr` where ownership must be dynamic. Avoid shared ownership unless there is a concrete reason.
- Keep headers lean; use forward declarations where useful.
- Comments explain why, constraints, or non-obvious behavior — not obvious syntax.

## Build configurations
- Debug: diagnostics, assertions, debug tools, low optimization.
- Development: symbols + debug tools + representative optimization; default for gameplay/performance testing.
- Release: shipping configuration; no editor/debug UI, assertions may be reduced, optimized.

Performance conclusions must not be drawn from Debug builds.
