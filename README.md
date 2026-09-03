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

The Development executable opens a resizable 1280x720 window titled `Platformer3D` with a 3D greybox scene. Use A/Left or D/Right to move along X, and Space or Up Arrow to jump. Press R to respawn at the current spawn (initial spawn, Checkpoint 1 after it activates, or Checkpoint 2 after that). Falling below Y = -8 or touching a static hazard respawns and increments death count (Manual R does not). Two red/orange spike bars sit on the ground: one in the center ↔ Checkpoint 1 corridor, one in the Checkpoint 2 → goal gap. They are non-solid gameplay volumes, not physics bodies. Three gold cubes are optional hop collectibles (right platform, left landing, middle-left step). A `TIME MM:SS.mmm` readout sits in the upper-left in all configurations, with `BEST --:--.---` or `BEST MM:SS.mmm` directly below it, and a `COLLECTED N / 3` counter sits in the upper-right. Time uses the gameplay frame delta, freezes on the first level completion, stays frozen through R / fall / hazard and leftover collectibles, and resets to `00:00.000` only when Enter starts a fresh run. Session BEST is the fastest completed run: first completion sets it, a faster later run replaces it, a slower run or exact tie leaves it, and Enter preserves it. A valid BEST is persisted under `%LOCALAPPDATA%\Platformer3D\best_time_v1.txt` and restored on the next launch; missing or invalid saves show `BEST --:--.---`. Collected items stay collected through R, fall, and hazard death; Enter after level complete restores them to 0 / 3. Collectibles do not gate the goal. Two post+beacon markers show ordered checkpoints: steel when future, bright green when current, darker green when previously activated. Checkpoint 2 cannot activate before Checkpoint 1; backtracking does not downgrade progress. The far-left two-post gate is the level goal, not a third checkpoint. Reaching it completes the level (`LEVEL COMPLETE` / `PRESS ENTER TO RESTART`); completion lasts for the rest of the run and is not cleared by R, a fall, or a hazard death. After completion, Enter starts a new run (player, both checkpoints, death count, collectibles, timer, moving platform, cyan box, and completion reset; BEST is kept). Enter before completion does nothing. The camera follows with dead zones and smoothing, and snaps immediately on respawn and restart. Player collision and physical position come from Jolt CharacterVirtual. Greybox geometry in `GreyboxWorld` feeds both rendering and Jolt static bodies. After Checkpoint 1 the Player returns toward the center on open ground. A 30-degree static slope past Checkpoint 1 is an optional walkable test; a 60-degree static slope further right is steep/non-walkable. One Jolt kinematic platform moves back and forth on X; standing on it carries the Player. The cyan box is a Jolt dynamic 30 kg crate: the Player is a physical barrier and can push it; temporary blocking is valid, a permanent wedge is not. There is no custom Player AABB collision backend. A magenta/dark checker quad in front of the spawn (`textures/test_checker.png`), an orange test pyramid to its right (`models/test_static.glb`), a Blender-authored static model to its left (`models/test_authored.glb`), and a textured Blender model further right (`models/test_textured.glb`) validate the cooked asset pipeline; all four are visual only and have no collision. Debug and Development builds show a read-only `Platformer3D Metrics` panel (F1 toggles it), including Run timer, Session best, Persistence, Respawn / Checkpoint, Hazards, Collectibles, and Level Goal. Close the window or press ESC to exit.

Release builds omit the metrics panel. Runtime assets still load from `assets/` next to the executable. Milestone 28 (session best time) is complete and manually approved. Milestone 29 (persistent best time / save file v1) implementation is complete and awaiting Phase C manual validation: only BEST is persisted as `%LOCALAPPDATA%\Platformer3D\best_time_v1.txt`, with sibling `best_time_v1.tmp` promoted through `platform::ReplaceFileWithTemporary` (no delete-final-then-rename). There is no SaveManager or generic save system.

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
