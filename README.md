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

The Development executable opens a resizable 1280x720 window titled `Platformer3D` with a 3D greybox scene. Use A/Left or D/Right to move along X, and Space or Up Arrow to jump. Press R to respawn at the current spawn (initial spawn, Checkpoint 1 after it activates, or Checkpoint 2 after that). Falling below the level kill plane (Y = -8 in Level 01) or touching a static hazard respawns and increments death count (Manual R does not). Two red/orange spike bars sit on the ground: one in the center ↔ Checkpoint 1 corridor, one in the Checkpoint 2 → goal gap. They are non-solid gameplay volumes, not physics bodies. Three gold cubes are optional hop collectibles (right platform, left landing, middle-left step). A `TIME MM:SS.mmm` readout sits in the upper-left in all configurations, with `BEST --:--.---` or `BEST MM:SS.mmm` directly below it, and a `COLLECTED N / 3` counter sits in the upper-right (the 3 is the Level 01 collectible count). Time uses the gameplay frame delta, freezes on the first level completion, stays frozen through R / fall / hazard and leftover collectibles, and resets to `00:00.000` only when Enter starts a fresh run. Session BEST is the fastest completed run: first completion sets it, a faster later run replaces it, a slower run or exact tie leaves it, and Enter preserves it. A valid BEST is persisted under `%LOCALAPPDATA%\Platformer3D\best_time_v1.txt` and restored on the next launch; missing or invalid saves show `BEST --:--.---`. Collected items stay collected through R, fall, and hazard death; Enter after level complete restores them to 0 / 3. Collectibles do not gate the goal. Two post+beacon markers show ordered checkpoints: steel when future, bright green when current, darker green when previously activated. Checkpoint 2 cannot activate before Checkpoint 1; backtracking does not downgrade progress. The far-left two-post gate is the level goal, not a third checkpoint. Reaching it completes the level (`LEVEL COMPLETE` / `PRESS ENTER TO RESTART`); completion lasts for the rest of the run and is not cleared by R, a fall, or a hazard death. After completion, Enter starts a new run (player, both checkpoints, death count, collectibles, timer, moving platform, cyan box, and completion reset; BEST is kept). Enter before completion does nothing. The camera follows with dead zones and smoothing, and snaps immediately on respawn and restart. Player collision and physical position come from Jolt CharacterVirtual. Canonical Level 01 authored data lives in `game/assets/source/levels/level_01.level`. The cooker copies it to `game/assets/cooked/levels/level_01.level`; CMake stages `<exe>/assets/levels/level_01.level`. Application loads that staged file once in Initialize into `LevelDefinition`. Renderer and PhysicsWorld derive visuals and bodies from the loaded definition. There is no compiled Level 01 fallback. After Checkpoint 1 the Player returns toward the center on open ground. A 30-degree static slope past Checkpoint 1 is an optional walkable test; a 60-degree static slope further right is steep/non-walkable. One Jolt kinematic platform moves back and forth on X; standing on it carries the Player. The cyan box is a Jolt dynamic 30 kg crate: the Player is a physical barrier and can push it; temporary blocking is valid, a permanent wedge is not. There is no custom Player AABB collision backend. A magenta/dark checker quad in front of the spawn (`textures/test_checker.png`), an orange test pyramid to its right (`models/test_static.glb`), a Blender-authored static model to its left (`models/test_authored.glb`), and a textured Blender model further right (`models/test_textured.glb`) validate the cooked asset pipeline; all four are visual only and have no collision. Debug and Development builds show a read-only `Platformer3D Metrics` panel (F1 toggles it), including Level Loading, Level Data, Run timer, Session best, Persistence, Respawn / Checkpoint, Hazards, Collectibles, and Level Goal. Close the window or press ESC to exit.

Debug and Development builds also have a `Level Editor` panel toggled by F2. Opening it pauses the entire gameplay simulation: the run timer, Player, gravity, CharacterVirtual, moving platform, cyan box, Jolt stepping, fall/hazard detection, R respawn, checkpoints, goal completion, BEST comparison, collectible pickup, and Enter restart all stop. Rendering and the UI keep running, so the paused world stays visible behind the panel. F2 again resumes exactly where it left off. F2 is ignored while an editor text field owns the keyboard, so typing a value cannot close the editor.

Release builds omit the metrics and editor panels, and F2 does nothing. Runtime assets still load from `assets/` next to the executable, including the required `levels/level_01.level`. Milestones 30 and 31 are complete and merged. Milestone 32 Phase B (live editor) is implemented; Phase C (manual validation) remains. Missing or invalid Level 01 fails initialization; BEST save missing/invalid remains nonfatal. There is no LevelManager. Do not treat M32 as complete.

## Level authoring (Milestone 32)

Development can edit Level 01 in-game:

```powershell
cmake --build --preset windows-development
.\build\windows-vs2022\bin\Development\Platformer3D.exe
# F2                    open the editor (simulation pauses)
# edit fields           working copy only; "Modified" turns true
# Apply Preview         validate, rebuild physics, preview; "Dirty" turns true
# Save Level Source     write game/assets/source/levels/level_01.level
python tools/cook_assets.py
cmake --build --preset windows-development
.\build\windows-vs2022\bin\Development\Platformer3D.exe   # relaunch to load it
```

Editable fields are the initial spawn, the camera offset and vertical FOV, and the center/size of the ground box and of `platform 0`..`platform 5`. Everything else in the file is read-only in M32: id, kill plane, slopes, moving platform, support indices, checkpoints, hazards, collectibles, goal, and the cyan box.

The editor holds a working copy of the authored `LevelDefinition`. Two states are reported separately:

- **Modified** — the working copy differs from the applied level. Editing a field sets it; `Apply Preview` or `Revert Working Copy` clears it. `Save Level Source` is disabled while it is true, so only applied data can ever be written.
- **Dirty** — the applied level differs from the last source state this session saved. `Apply Preview` of changed data sets it; a successful save clears it. It survives closing and reopening the editor.

`Apply Preview` validates the working copy first. Invalid data (a size `<= 0`, an out-of-range FOV, a non-finite value) is rejected with no change to the world, and nothing is clamped silently. Valid data rebuilds PhysicsWorld from the same definition the Renderer draws, so visual and collision geometry cannot diverge, and starts a fresh preview run (Player at the authored spawn, timer, checkpoints, collectibles, completion, moving platform and cyan box reset). Session and persisted BEST are never touched by the editor.

Applied but unsaved edits live in memory only: gameplay and Enter restart use them, and closing the process loses them. Closing and reopening the editor discards *unapplied* working-copy edits.

Save writes `game/assets/source/levels/level_01.level` only. It never writes the cooked or staged copy, never auto-cooks, and never spawns Python or CMake from the game. The writer emits canonical formatting, so the first save may rewrite `25.60` as `25.6`; the values are unchanged. Only Development can author: the source root is a Development-only compile definition, so Debug and Release cannot resolve a project path at all. In Debug the panel reports `Authoring: Unavailable` and Save is disabled.

Known Development-tool limitations: the dirty baseline is the staged file loaded at startup, so a source edited without cooking is not reconciled; there is no filesystem watching, no conflict detection, and no `Reload Runtime Level`.

Tests:
```powershell
.\build\windows-vs2022\Development\LevelFileTest.exe        # parser + writer + authored equality
.\build\windows-vs2022\Development\PhysicsRebuildTest.exe   # repeated editor Apply rebuild cycle
python tools/test_cook_level_v1.py
```

## Assets
- `game/assets/source/`: authored inputs (tracked).
- `game/assets/cooked/`: cooker output (generated; gitignored except `.gitkeep`).
- Logical test texture: `textures/test_checker.png`.
- Logical test models: `models/test_static.glb`, `models/test_authored.glb`, `models/test_textured.glb` (copied unchanged; no collision).
- Milestone 31 live Level 01: `levels/level_01.level`. Format: `docs/LEVEL_FORMAT_V1.md`. Edit `game/assets/source/levels/level_01.level`, then cook/build. Do not edit cooked or staged copies. Milestone 32 adds a `PLATFORMER_LEVEL 1` writer (`world/LevelWriter.h`) that targets the same source file.
- Milestone 18: `models/test_textured.glb` embeds (or must embed) its Base Color; `test_textured_basecolor.png` is authoring-only and is not a runtime asset. See `docs/BLENDER_WORKFLOW.md`.
- Blender authoring: `docs/BLENDER_WORKFLOW.md`. `.blend` files are not cooked or loaded at runtime.
- Cooker: `python tools/cook_assets.py`. Standalone runtime PNGs (`runtime_png`) use recipe `runtime_png.max512.lanczos.v1` (max 512 px, LANCZOS, no upscale). Pillow `12.3.0` is cooker-only (`python -m pip install -r tools/requirements.txt`). Blender authoring PNGs are not cooker inputs. GLBs are opaque copies.
- Runtime: CMake stages cooked files to `<exe dir>/assets/...`. The game never loads from `source/`.

If CMake configure reports a missing cooked asset, run the cooker command above.

## Cursor
Open the repository root in Cursor. The agent will pick up `AGENTS.md`, `.cursor/rules/*.mdc`, and `.cursor/skills/*/SKILL.md`.
