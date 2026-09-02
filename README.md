# Platformer3D

Starter repository for a C++ 3D platformer developed with Cursor.

## Windows bootstrap
Requirements:
- Cursor
- Visual Studio 2022 with Desktop development with C++
- CMake 3.25+
- Git
- Python 3 (cooker uses the standard library plus cooker-only Pillow; see `tools/requirements.txt`)

Cook runtime assets from the repository root (required before configure/build):
```powershell
python -m pip install -r tools/requirements.txt
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

The Development executable opens a resizable 1280x720 window titled `Platformer3D` with a 3D greybox scene. Use A/Left or D/Right to move along X, and Space or Up Arrow to jump. Press R to respawn at the current spawn (initial spawn, or the checkpoint after it activates). Falling below Y = -8 respawns and increments death count. A post+beacon marker on the right elevated platform shows the checkpoint: dull when inactive, green when active. Reaching the two-post goal marker on the left elevated platform completes the level (`LEVEL COMPLETE`); completion lasts for the rest of the run and is not cleared by R or a fall. The camera follows with dead zones and smoothing, and snaps immediately on respawn. Player collision and physical position come from Jolt CharacterVirtual. Greybox geometry in `GreyboxWorld` feeds both rendering and Jolt static bodies. A 30-degree static slope on the right is walkable; a 60-degree static slope on the left is steep/non-walkable. One Jolt kinematic platform moves back and forth on X; standing on it carries the Player. The cyan box is a Jolt dynamic integration test. There is no custom Player AABB collision backend. A magenta/dark checker quad in front of the spawn (`textures/test_checker.png`), an orange test pyramid to its right (`models/test_static.glb`), a Blender-authored static model to its left (`models/test_authored.glb`), and a textured Blender model further right (`models/test_textured.glb`) validate the cooked asset pipeline; all four are visual only and have no collision. Debug and Development builds show a read-only `Platformer3D Metrics` panel (F1 toggles it), including Respawn / Checkpoint and Level Goal. Close the window or press ESC to exit.

Release builds omit the metrics panel. Runtime assets still load from `assets/` next to the executable.

## Assets
- `game/assets/source/`: authored inputs (tracked).
- `game/assets/cooked/`: cooker output (generated; gitignored except `.gitkeep`).
- Logical test texture: `textures/test_checker.png`.
- Logical test models: `models/test_static.glb`, `models/test_authored.glb`, `models/test_textured.glb` (copied unchanged; no collision).
- Milestone 18: `models/test_textured.glb` embeds (or must embed) its Base Color; `test_textured_basecolor.png` is authoring-only and is not a runtime asset. See `docs/BLENDER_WORKFLOW.md`.
- Blender authoring: `docs/BLENDER_WORKFLOW.md`. `.blend` files are not cooked or loaded at runtime.
- Cooker: `python tools/cook_assets.py`. Standalone runtime PNGs (`runtime_png`) use recipe `runtime_png.max512.lanczos.v1` (max 512 px, LANCZOS, no upscale). Pillow `12.3.0` is cooker-only (`python -m pip install -r tools/requirements.txt`). Blender authoring PNGs are not cooker inputs. GLBs are opaque copies.
- Runtime: CMake stages cooked files to `<exe dir>/assets/...`. The game never loads from `source/`.

If CMake configure reports a missing cooked asset, run the cooker command above.

## Cursor
Open the repository root in Cursor. The agent will pick up `AGENTS.md`, `.cursor/rules/*.mdc`, and `.cursor/skills/*/SKILL.md`.
