# Platformer3D

Starter repository for a C++ 3D platformer developed with Cursor.

## Windows bootstrap
Requirements:
- Cursor
- Visual Studio 2022 with Desktop development with C++
- CMake 3.25+
- Git
- Python 3 (standard library only; no pip packages)

Cook runtime assets from the repository root (required before configure/build):
```powershell
python tools/cook_assets.py
```

Configure:
```powershell
cmake --preset windows-vs2022
```

Build Development:
```powershell
cmake --build --preset windows-development
```

Run:
```powershell
.\build\windows-vs2022\bin\Development\Platformer3D.exe
```

The Development executable opens a resizable 1280x720 window titled `Platformer3D` with a 3D greybox scene. Use A/Left or D/Right to move along X, and Space or Up Arrow to jump. The camera follows with dead zones and smoothing. Player collision and physical position come from Jolt CharacterVirtual. Greybox geometry in `GreyboxWorld` feeds both rendering and Jolt static bodies. A 30-degree static slope on the right is walkable; a 60-degree static slope on the left is steep/non-walkable. One Jolt kinematic platform moves back and forth on X; standing on it carries the Player. The cyan box is a Jolt dynamic integration test. There is no custom Player AABB collision backend. A magenta/dark checker quad in front of the spawn (`textures/test_checker.png`) and an orange test pyramid to its right (`models/test_static.glb`) validate the cooked asset pipeline; both are visual only and have no collision. Debug and Development builds show a read-only `Platformer3D Metrics` panel (F1 toggles it). Close the window or press ESC to exit.

Release builds omit the metrics panel. Runtime assets still load from `assets/` next to the executable.

## Assets
- `game/assets/source/`: authored inputs (tracked).
- `game/assets/cooked/`: cooker output (generated; gitignored except `.gitkeep`).
- Logical test texture: `textures/test_checker.png`.
- Logical test model: `models/test_static.glb` (copied unchanged; no collision).
- Cooker: `python tools/cook_assets.py` (SHA-256 incremental copy + `cooked/manifest.json`).
- Runtime: CMake stages cooked files to `<exe dir>/assets/...`. The game never loads from `source/`.

If CMake configure reports a missing cooked asset, run the cooker command above.

## Cursor
Open the repository root in Cursor. The agent will pick up `AGENTS.md`, `.cursor/rules/*.mdc`, and `.cursor/skills/*/SKILL.md`.
