# Milestones

## Milestone 00 — Repository/Foundation Setup [ACTIVE]
Goal: create a clean Cursor/CMake repository before gameplay implementation.

Acceptance criteria:
- Repository structure exists.
- `AGENTS.md`, Cursor Rules, and project Skills exist and are discoverable.
- CMake config has Debug, Development, and Release conventions.
- A minimal executable target exists.
- Windows Development build can be configured with Visual Studio 2022.
- No gameplay feature is implemented yet.

## Milestone 01 — Window + Foundation
- Bring in/pin raylib.
- Open a window and clear the frame.
- Establish app loop, timing, logging, clean shutdown.
- Smoke test launch/exit.

## Milestone 02 — Platformer Prototype
- Greybox player represented by primitive geometry.
- Left/right movement, jump, gravity, floor collision.
- Platform-style camera with 3D perspective/orthographic decision documented after testing.
- Input is expressed as semantic actions, not raw keys in gameplay.

## Milestone 03 — Level Greybox
- Platforms, hazards, spawn, checkpoint, goal.
- Reset/death loop.
- First simple test level loaded from data rather than hard-coded placement where practical.

## Milestone 04 — Physics Integration
- Introduce Jolt only if the prototype shows it is justified.
- Preserve gameplay-facing collision/movement abstraction.

## Milestone 05 — Menus + Debug Tools
- Main menu, pause menu, Development-only diagnostics.
- Basic frame-time/FPS display.

## Milestone 06 — Asset Pipeline
- Python environment and cooker skeleton.
- Source/cooked asset separation.
- Incremental cooking foundation.

## Later milestones
Animation, enemies, collectibles, level editor, audio, save system, profiling/optimization, Raspberry Pi validation, Android port, iOS feasibility/backend work.
