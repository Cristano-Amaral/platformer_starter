# Project Instructions — 3D Platformer

## Project goal
Build a C++ platform game with 3D visuals and a platformer-style camera. Windows is the first shipping target. Architecture must preserve a practical path to Linux/Raspberry Pi, Android, and iOS.

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

## Milestone workflow
For each milestone:
1. Read `docs/MILESTONES.md` and identify the single active milestone.
2. State a short implementation plan.
3. Implement only that milestone.
4. Configure/build the project.
5. Run available tests/smoke checks.
6. Summarize changed files, architectural decisions, how to test, and remaining known limitations.
7. Do not mark a milestone complete if the build is broken.

## Current milestone
Milestone 37 — Editor Tool Runner: Asset Cooker & Build Integration
(implementation and Phase C manual validation complete; awaiting Git
closure — do not commit, push, or merge until the user requests it).
Milestone 36 is complete and merged. Milestone 38 has not started. See
`docs/MILESTONES.md`, `docs/ARCHITECTURE.md`, and `docs/LEVEL_FORMAT_V1.md`.

Milestone 33 is complete and merged. F2 still pauses simulation, edits a
working copy, and uses Apply Preview / Revert / Save Level Source. Viewport
pick/highlight use the active/applied world; Inspector and the translation
gizmo edit `workingCopy`. Editable set is unchanged. Debug compiles the visual
editor but cannot author. Release has no editor.

M34 added world-space X/Y/Z translation for Spawn, Ground, and Elevated
Platform 0..5, a pending ghost, persistent Dear ImGui layout, and a
depth-independent gizmo overlay. M35 is complete (resize, orientation widget,
Translate-only nudge, Alt+wheel dolly). M36 is complete (F2 Dear ImGui menu
bar: View / Transform / Level). M37 adds a Development-only Build menu and
Tool Output. Debug has no Build menu and does not launch tools.
`Build > Cook Assets` runs only `python tools/cook_assets.py` (repository
cooked assets). It does not stage runtime assets, build, or restart. CMake
POST_BUILD still stages cooked files into
`build/windows-vs2022/bin/<Config>/assets/`. After authored edits, a
fresh-restart test is Save → Cook → Build (may be a C++ no-op) → Restart.
Do not add automatic cook/build/stage chains, Add/Delete, docking, or
Milestone 38.

Milestone 31 is complete and merged. One playable level (`level_01`). The sole
live authored source is `game/assets/source/levels/level_01.level` → cooker →
cooked → staged `<exe>/assets/levels/level_01.level` → `LoadLevelFile` →
`LevelDefinition`. Application loads that staged file once in `Initialize` and
owns it. There is no compiled Level 01 fallback. Missing/invalid/unsupported
Level 01 is a fatal init error. M29 BEST save remains nonfatal. The M32 writer,
authoring boundary, F2 editor, Apply Preview and source Save remain the live
authoring path. Save Level Source is not Cook Assets; Cook Assets is not
runtime staging.
