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

The Development executable opens a resizable 1280x720 window titled `Platformer3D` with a 3D greybox scene. Use A/Left or D/Right to move along X, and Space or Up Arrow to jump. Press **Tab** to open or close the player Inventory (read-only list of owned `itemId` / quantity; Up/Left previous, Down/Right next; Esc also closes). While Inventory is open, gameplay simulation pauses so movement, jump, Grab/Drop, and Item Pickup collection do not fire; a carried Dynamic Box is not dropped. Press R to respawn at the current spawn (initial spawn, Checkpoint 1 after it activates, or Checkpoint 2 after that). Falling below the level kill plane (Y = -8 in Level 01) or touching a static hazard respawns and increments death count (Manual R does not). Two red/orange spike bars sit on the ground: one in the center ↔ Checkpoint 1 corridor, one in the Checkpoint 2 → goal gap. They are non-solid gameplay volumes, not physics bodies. Three gold cubes are optional hop collectibles (right platform, left landing, middle-left step). A `TIME MM:SS.mmm` readout sits in the upper-left in all configurations, with `BEST --:--.---` or `BEST MM:SS.mmm` directly below it, and a `COLLECTED N / <level collectible count>` counter sits in the upper-right (Level 01 still has 3). Time uses the gameplay frame delta, freezes on the first level completion, stays frozen through R / fall / hazard and leftover collectibles, and resets to `00:00.000` only when Enter starts a fresh run. Session BEST is the fastest completed run: first completion sets it, a faster later run replaces it, a slower run or exact tie leaves it, and Enter preserves it. A valid BEST is persisted under `%LOCALAPPDATA%\Platformer3D\best_time_v1.txt` and restored on the next launch; missing or invalid saves show `BEST --:--.---`. Collected items stay collected through R, fall, and hazard death; Enter after level complete restores them to 0 / 3. Runtime Inventory (M54) is likewise preserved through R / fall / hazard and cleared on Enter Restart Run; it is not a world collectible and is not saved with the level. Collectibles do not gate the goal. Two post+beacon markers show ordered checkpoints: steel when future, bright green when current, darker green when previously activated. Checkpoint 2 cannot activate before Checkpoint 1; backtracking does not downgrade progress. The far-left two-post gate is the level goal, not a third checkpoint. Reaching it completes the level (`LEVEL COMPLETE` / `PRESS ENTER TO RESTART`); completion lasts for the rest of the run and is not cleared by R, a fall, or a hazard death. After completion, Enter starts a new run (player, both checkpoints, death count, collectibles, timer, moving platform, and completion reset; BEST is kept). Enter before completion does nothing. The camera follows with dead zones and smoothing, and snaps immediately on respawn and restart. Player collision and physical position come from Jolt CharacterVirtual. Canonical Level 01 authored data lives in `game/assets/source/levels/level_01.level`. The cooker copies it to `game/assets/cooked/levels/level_01.level`; CMake stages `<exe>/assets/levels/level_01.level`. Application loads that staged file once in Initialize into `LevelDefinition`. Renderer and PhysicsWorld derive visuals and bodies from the loaded definition. There is no compiled Level 01 fallback. After Checkpoint 1 the Player returns toward the center on open ground. A 30-degree static slope past Checkpoint 1 is an optional walkable test; a 60-degree static slope further right is steep/non-walkable. One Jolt kinematic platform moves back and forth on X; standing on it carries the Player. The Level Format `dynamic_box` record is a repeatable authored Dynamic Box (center, size, mass in kg). Canonical Level 01 has zero Dynamic Boxes so the old cyan probe is not scene clutter. Place boxes in the Development editor, then Apply Preview. Jolt pose is runtime authority; Save writes authored centers. Full Enter restart restores authored poses and zeros velocities; checkpoint respawn does not. Press **E** to Grab / Drop a nearby Dynamic Box in front of the player (2.5 m max, blocked by solid Ground/Platform/Slope/Door geometry). If no box is carried and no grab target exists, the same **E** collects a nearby available Item Pickup (`E Pick Up <itemId> x<qty>`). If neither applies, **E** can unlock a nearby locked Door that requires a `key` (`E Unlock Door (key)` / `Requires key`). Carry, collection, and unlock are runtime-only and never mark the authored level Modified. A `pressure_plate` record is a repeatable authored trigger volume (center, size). It is Active while at least one runtime Dynamic Box overlaps that AABB, Inactive with none, and never serializes Active/Inactive. Pressure Plates do not consume Jolt body slots. Canonical Level 01 has zero Pressure Plates. If a live Dynamic Box center falls below the authored kill plane (Y = -8 in Level 01), only that box returns to its currently applied authored transform with identity orientation and zero velocity; other boxes keep their runtime state. A carried box that falls through the kill plane is released, then recovered the same way. Cooker test assets (`textures/test_checker.png`, `models/test_static.glb`, `models/test_authored.glb`, `models/test_textured.glb`) remain in cook/stage inventory and automated tests; Milestone 44 does not load or draw them in Level 01. Debug and Development builds show a `Platformer3D Metrics` panel (F1 toggles it), including Level Loading, Level Data, Run timer, Session best, Persistence, Respawn / Checkpoint, Hazards, Collectibles, and Level Goal. Development adds an **Inventory (Test)** section in that same F1 panel: inspect `itemId` / quantity and manually Add / Remove / Clear through the production Inventory API (not the player HUD; Tab opens the separate game Inventory UI). Close the window or press ESC to exit.

Debug and Development builds also have a visual level editor toggled by F2 (`Hierarchy`, `Inspector`, and `Level Editor` windows, plus a top Dear ImGui menu bar: View / Transform / Level). Development adds an Edit menu (Add Platform/Checkpoint/Hazard/Collectible/Dynamic Box/Pressure Plate/Door/Item Pickup/Static Prop, Duplicate Selected, Delete Selected; Delete key matches the menu and is ignored while ImGui owns the keyboard), an Object Palette with viewport placement for Platform/Checkpoint/Hazard/Collectible/Dynamic Box/Pressure Plate/Door/Item Pickup (not Static Prop), a compact Quick Toolbar below the menu bar (Translate, Resize, Scale, Rotate, Apply, Revert, Save, build selector, Run), an Assets menu (Import Static GLB), a Content Browser (View > Content Browser; catalog of registered static models, search, Refresh, Import Static GLB, Add Static Prop, confirmed Delete Asset), a Model Preview (View > Model Preview; real selected GLB with orbit/zoom/Reset View), a Build menu (Cook Assets, Stage Runtime Assets, Cook & Stage, Cook, Stage & Reload, Build Debug/Development/Release, Build All) and a Tool Output window. Opening the editor pauses the entire gameplay simulation: the run timer, Player, gravity, CharacterVirtual, moving platform, Jolt stepping, fall/hazard detection, R respawn, checkpoints, goal completion, BEST comparison, collectible pickup, and Enter restart all stop. Rendering and the UI keep running. The editor camera (RMB look, WASD, Q/E, Shift, wheel speed, Alt+wheel dolly) navigates independently of the gameplay follow camera. Level Editor, Transform menu, and Quick Toolbar share Translate, Resize, Scale, and Rotate. Translate: LMB on an X/Y/Z handle moves Spawn, Ground, an Elevated Platform, Checkpoint, Hazard, Collectible, Dynamic Box, Pressure Plate, Door, Item Pickup, or Static Prop in the working copy. Resize: cube handles change Ground/Platform/Dynamic Box/Pressure Plate/Door authored size; Spawn/Checkpoint/Hazard/Collectible/Item Pickup/Static Prop are not resizable. Scale: cube handles change Static Prop `scale` or Item Pickup `visualScale`. Rotate: world-axis X/Y/Z rings change Static Prop authored rotation or Item Pickup `visualRotationDegrees` (gizmo origin is `position + visualOffset` for pickups). Item Pickup Translate still edits gameplay `position`; visual offset stays Inspector-only. Ctrl+Arrows/PageUp/PageDown nudge in Translate mode only. A screen-space orientation widget sets canonical editor views (yaw/pitch only) and sits below the menu bar and Quick Toolbar. LMB elsewhere picks a world object. Hierarchy rows select the same identity. Unapplied gizmo/Inspector edits show a cyan ghost; the active object, physics, picking, and yellow highlight stay put until Apply Preview except for model-backed Static Props and Item Pickups, whose selected-model highlight follows `workingCopy` live. Pending deletes remain visible in the viewport with a distinct wireframe until Apply. F2 again resumes gameplay (follow camera snaps to the Player). F2 is ignored while an editor text field owns the keyboard, so typing a value cannot close the editor. Clicks, drags, and wheel on ImGui panels, the menu bar, or the Quick Toolbar do not pick, drag a gizmo, or navigate the world behind them. **Reset Editor Layout** (Level menu or Level Editor button) restores Metrics, Hierarchy, Inspector, Level Editor, Object Palette, Content Browser, Model Preview, Tool Output, and Quick Toolbar visibility to the project defaults, snaps those windows, and restores Content Browser **Thumbnails** view. It does not reset the last toolbar build configuration. F1 and View > Metrics toggle the same Metrics visibility; Metrics can stay open with F2 off.

A Development F2 **Assets > Import Static GLB** command copies a compatible self-contained static `.glb` into `game/assets/source/models/`, keeping the external filename. Identity is the project-relative path `models/<filename>.glb`. Import never overwrites an existing file, never edits Level 01, and never cooks or stages. After import, use `Build > Cook Assets` then `Stage Runtime Assets` (or Cook & Stage). The Development **Content Browser** lists that derived catalog in a **Thumbnails** grid by default (or **List**), filters by filename/path, refreshes, invokes the same import command, and can delete a selected registered asset (canonical source plus matching cooked/staged copies) after confirmation. Toolbar **Add Static Prop** is the primary direct-add path and creates one authored instance from the selected valid static model (working copy only; not placement). **Edit > Add > Static Prop** is retained and emits the same `LevelEditorRequest::AddStaticProp`; that menu row shows the selected asset filename, or `select asset` when nothing usable is selected. If the identity has no staged runtime model, the Content Browser and Inspector say so and point to `Build > Cook & Stage` (the viewport draws a placeholder cube until then; there is no source fallback). Authored `Scale (1,1,1)` keeps the model's raw GLB size. Thumbnails are generated offscreen from the source GLB and cached under `%LOCALAPPDATA%\Platformer3D\thumbnails\` (not source, not cooked, not staged, not Git). **Model Preview** renders the real selected source GLB (not the thumbnail PNG): LMB orbit, wheel zoom, Reset View. Browser selection is not a level object. Debug has no import/browser/preview UI. Release has no editor.

Release builds omit the metrics and editor panels, and F2 does nothing. The player-facing Inventory UI (Tab) remains available in Release; the Development Inventory (Test) harness does not. Runtime assets still load from `assets/` next to the executable, including the required `levels/level_01.level`. Milestones 30–58.4 are complete. Milestone 59 adds optional visual-only Item Pickup idle bob/Y-spin and a Target Highlight Gold Amount control distinct from M58.4 Intensity. Canonical Level 01 has 0 Item Pickups, 0 Pressure Plates, 0 Doors, 0 Dynamic Boxes, and 0 Static Props. Debug omits Build, Reload Runtime Level, the Edit lifecycle menu, Object Palette, Quick Toolbar, Assets import, Content Browser, Model Preview, and the Inventory test harness. Missing or invalid Level 01 fails initialization; BEST save missing/invalid remains nonfatal. There is no LevelManager and no SceneManager.

## Level authoring (Milestone 32)

Development can edit Level 01 in-game:

```powershell
cmake --build --preset windows-development
.\build\windows-vs2022\bin\Development\Platformer3D.exe
# F2                    open the editor (simulation pauses; editor camera active)
# Hierarchy / world     select an authored object (does not set Modified)
# Edit > Add / Duplicate / Delete   working copy only; "Modified" turns true
#                           Static Prop: Content Browser Add Static Prop or
#                           Edit > Add > Static Prop (selected identity;
#                           not Object Palette placement)
# Object Palette                    toggle placement; click viewport to Add
#                           (Platform/Checkpoint/Hazard/Collectible/Dynamic Box/Pressure Plate)
# Esc or click category again       exit placement mode (does not Apply)
# Quick Toolbar                     Translate/Resize, Apply, Revert, Save, Build+Run
# View > Quick Toolbar              hide/show the toolbar (viewport reclaims space)
# edit Inspector        working copy only; "Modified" turns true for editable types
# Apply Preview         validate, rebuild physics, preview; "Dirty" turns true
# Save Level Source     write game/assets/source/levels/level_01.level
#                       (not cook, not stage)
python tools/cook_assets.py                               # cooked repo copy only
# F2 Build > Cook & Stage   cook, then stage Development runtime assets
#                           (no C++ build). Or Stage Runtime Assets if cooked
#                           is already current.
# F2 Level > Reload Runtime Level  re-read staged level in-process (no Build, no restart).
# F2 Build > Cook, Stage & Reload  cook, stage, then Reload Runtime Level in-process.
#                           Does not Apply, Save, Build, or restart.
# F2 Assets > Import Static GLB    copy a compatible .glb into
#                           game/assets/source/models/<filename>.glb
#                           (not a level object; not cook; not stage)
# F2 View > Content Browser        Thumbnails grid (default) or List;
#                           search registered static models; Refresh;
#                           Import Static GLB; Add Static Prop (direct
#                           authored add from the selected identity);
#                           Delete Selected Asset (project asset only;
#                           not a level edit except Add Static Prop)
# F2 View > Model Preview          real selected GLB (LMB orbit, wheel zoom,
#                           Reset View; not a thumbnail; not a level object)
```

After an authored change, use `Build > Cook, Stage & Reload`, or Cook & Stage then `Level > Reload Runtime Level`. Save is not Cook. Cook is not runtime staging. Reload reads only `<exe>/assets/levels/level_01.level`. Development `Build > Stage Runtime Assets` copies cooked files into `build/windows-vs2022/bin/Development/assets/` via `cmake -P cmake/StageRuntimeAssets.cmake` (no `--build`, no cooker). `Build > Cook & Stage` runs Cook Assets then Stage; cook failure skips Stage. `Build > Cook Assets` remains only `python tools/cook_assets.py`. Build Development remains `cmake --build --preset windows-development` and still stages via POST_BUILD. Staging does not reload in-memory assets by itself. Reload and Cook, Stage & Reload are disabled while Modified, while a Build tool is Running, or while Cook, Stage & Reload is pending. Dirty does not block. Stale extra files already in the destination tree are not deleted.

Editable fields are the initial spawn, the camera offset and vertical FOV, the center/size of the ground box and of every elevated platform, the existing authored fields of checkpoints (trigger center/size and respawn position), hazards (center/size), collectibles (center/size), Dynamic Boxes (center/size/mass), Pressure Plates (center/size), and Static Props (canonical model identity, position, Euler XYZ degrees, visual scale). Lifecycle Add/Duplicate/Delete are Development Edit-menu only and mutate the working copy; Apply Preview, Save Level Source, and Cook, Stage & Reload remain explicit. Everything else in the file stays read-only: id, kill plane, slopes, moving platform, support indices, and goal.

The editor holds a working copy of the authored `LevelDefinition`. Two states are reported separately:

- **Modified** — the working copy differs from the applied level. Editing a field sets it; `Apply Preview` or `Revert Working Copy` clears it. `Save Level Source` is disabled while it is true, so only applied data can ever be written.
- **Dirty** — the applied level differs from the last source state this session saved. `Apply Preview` of changed data sets it; a successful save clears it. It survives closing and reopening the editor.

`Apply Preview` validates the working copy first. Invalid data (a size `<= 0`, an out-of-range FOV, a non-finite value) is rejected with no change to the world, and nothing is clamped silently. Valid data rebuilds PhysicsWorld from the same definition the Renderer draws, so visual and collision geometry cannot diverge, and starts a fresh preview run (Player at the authored spawn, timer, checkpoints, collectibles, completion, moving platform and cyan box reset). Session and persisted BEST are never touched by the editor.

Applied but unsaved edits live in memory only: gameplay and Enter restart use them, and closing the process loses them. Closing and reopening the editor discards *unapplied* working-copy edits.

Save writes `game/assets/source/levels/level_01.level` only. It never writes the cooked or staged copy, never auto-cooks, and never spawns Python or CMake from the game. The writer emits canonical formatting, so the first save may rewrite `25.60` as `25.6`; the values are unchanged. Only Development can author: the source root is a Development-only compile definition, so Debug and Release cannot resolve a project path at all. In Debug the panel reports `Authoring: Unavailable` and Save is disabled.

Known Development-tool limitations: the dirty baseline is the last successful Save in this session (seeded from the staged file at Initialize). Reload does not update that baseline. There is no filesystem watching. Cook, Stage & Reload does not auto Apply or auto Save; Dirty does not block it. Cook, Stage & Reload cooks whatever is currently in authored source on disk.

Milestone 33 is complete and merged. Milestone 34 is complete and merged. Milestone 35 is complete and merged. Milestone 36 is complete and merged. Milestone 37 is complete and merged. Milestone 38 is complete and merged. Milestone 39 is complete and merged. Milestone 40 is complete and merged. Milestone 41 is complete and merged. Milestone 42 is complete and merged. Milestone 43 is complete and merged. Milestone 44 is complete and merged. Milestone 45 is complete and merged. Milestone 46 is complete and merged. Milestone 47 adds Development `Assets > Import Static GLB` (canonical source copy + derived `models/*.glb` catalog). Milestone 48 adds the Development Content Browser over that catalog (search, refresh, same import, confirmed delete of source plus matching cooked/staged copies). Milestone 48.1 adds Thumbnails/List presentation and a local derived thumbnail cache. Milestone 48.2 adds a Development Model Preview of the selected static GLB (orbit/zoom/Reset View; independent of thumbnail cache). Milestone 49 adds Development-authored Static Props (complete transform; Delete Asset refuses referenced models). Milestone 50 adds Development Content Browser **Place Static Prop** (real-model viewport placement on Ground/Platform/Slope; Direct Add remains distinct). Milestone 51 adds gameplay Grab / Carry for authored Dynamic Boxes (`E`). Milestone 52 adds Development-authored Pressure Plates (primitive Translate/Resize; runtime Active from Dynamic Box overlap only). Milestone 53 adds Development-authored Doors and a Pressure Plate → one Door index (kinematic solid, +Y open, OR from linked M52 Active). Debug omits Build, Reload Runtime Level, the Edit lifecycle menu, Object Palette, Quick Toolbar, Assets import, Content Browser, and Model Preview. Awaiting manual acceptance. Do not implement Milestone 57.

Tests:
```powershell
.\build\windows-vs2022\Development\LevelFileTest.exe        # parser + writer + M39 reload core
.\build\windows-vs2022\Development\PhysicsRebuildTest.exe   # repeated editor Apply rebuild cycle
.\build\windows-vs2022\Development\DynamicBoxGrabTest.exe   # M51 Dynamic Box Grab / Carry
.\build\windows-vs2022\Development\PressurePlateTest.exe    # M52 Pressure Plate overlap / activation
.\build\windows-vs2022\Development\InventoryTest.exe        # M54 Inventory API / lifecycle
.\build\windows-vs2022\Development\InventoryUiTest.exe      # M56 player Inventory UI / focus
.\build\windows-vs2022\Development\ItemPickupTest.exe       # M55/M58 Item Pickup targeting / visual transform
.\build\windows-vs2022\Development\ItemPickupTargetHighlightTest.exe # M58.3/M58.4 target highlight presentation
.\build\windows-vs2022\Development\DoorTest.exe             # M53 Door / Pressure Plate link
.\build\windows-vs2022\Development\EditorPickingTest.exe    # M33 ray/AABB, slope, nearest-hit, selection
.\build\windows-vs2022\Development\EditorGizmoTest.exe      # M34 translation + M35 resize/nudge math
.\build\windows-vs2022\Development\EditorOrientationTest.exe # M35 orientation widget + dolly math
.\build\windows-vs2022\Development\EditorWorkspaceTest.exe   # M36 workspace visibility + action enable
.\build\windows-vs2022\Development\AuthoredObjectLifecycleTest.exe # M41 working-copy Add/Duplicate/Delete
.\build\windows-vs2022\Development\AuthoredLifecycleIntegrationTest.exe # M41 owner-side lifecycle
.\build\windows-vs2022\Development\EditorPlacementTest.exe   # M42 palette placement mode + candidate
.\build\windows-vs2022\Development\EditorQuickToolbarTest.exe # M43 toolbar mapping, chrome, build preference
.\build\windows-vs2022\Development\CookStageReloadWorkflowTest.exe # M40 cook/stage/reload orchestration
.\build\windows-vs2022\Development\EditorToolRunnerTest.exe # M37/M38 process capture + sequences
.\build\windows-vs2022\Development\StaticGlbImportTest.exe # M47 static GLB import + catalog
.\build\windows-vs2022\Development\ContentBrowserTest.exe # M48 browser query/filter/delete authority
.\build\windows-vs2022\Development\ContentBrowserThumbnailTest.exe # M48.1 thumbnail cache/view/framing
.\build\windows-vs2022\Development\StaticModelPreviewTest.exe # M48.2 preview selection/orbit/framing
.\build\windows-vs2022\Development\StaticPropRenderTransformTest.exe # M49 authored transform + staged diagnostic
.\build\windows-vs2022\Development\StaticPropLifecycleTest.exe # M49 Add/Apply/Delete active promotion
.\build\windows-vs2022\Development\StaticPropSceneIsolationTest.exe # M49 DrawModel isolation vs later world draws
.\build\windows-vs2022\Development\GreyboxImmediateStateTest.exe # M49 clip-plane leak vs Gameplay BeginMode3D
.\build\windows-vs2022\Development\StaticPropPlacementTest.exe # M50 Static Prop viewport placement
.\build\windows-vs2022\Development\ModelSelectionPreviewTest.exe # M58.2 selected-model ghost / picking
.\build\windows-vs2022\Development\SelectedModelHighlightStateTest.exe # M58.2/M58.3/M58.4 highlight rlgl restore
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_stage_runtime_assets.py
python tools/test_import_static_glb.py
```

## Assets
- `game/assets/source/`: authored inputs (tracked). Save Level Source writes here.
- `game/assets/cooked/`: cooker output (generated; gitignored except `.gitkeep`). Cook Assets writes here only.
- `build/windows-vs2022/bin/<Config>/assets/`: staged runtime copy the built game loads. CMake POST_BUILD copies from cooked; Cook Assets does not.
- Logical test texture: `textures/test_checker.png` (cooker/staging inventory; not drawn in Level 01).
- Logical test models: `models/test_static.glb`, `models/test_authored.glb`, `models/test_textured.glb` (copied unchanged; cooker/staging inventory; not drawn in Level 01). Development `Assets > Import Static GLB` and the Content Browser copy additional compatible `.glb` files into `game/assets/source/models/` using the same identity form. Extra `source/models/*.glb` files are cooked as `copy` and staged from cooked `models/*.glb`. Import never overwrites and is not a level edit. Content Browser Delete removes that canonical source file and matching cooked/staged copies after confirmation; it is not a level edit and does not instantiate objects. Content Browser thumbnails are derived editor cache under `%LOCALAPPDATA%\Platformer3D\thumbnails\` and are not source, cooked, or staged runtime content.
- Milestone 31 live Level 01: `levels/level_01.level`. Format: `docs/LEVEL_FORMAT_V1.md`. Edit `game/assets/source/levels/level_01.level`, then cook/build. Do not edit cooked or staged copies. Milestone 32 adds a `PLATFORMER_LEVEL 1` writer (`world/LevelWriter.h`) that targets the same source file.
- Milestone 18: `models/test_textured.glb` embeds (or must embed) its Base Color; `test_textured_basecolor.png` is authoring-only and is not a runtime asset. See `docs/BLENDER_WORKFLOW.md`.
- Blender authoring: `docs/BLENDER_WORKFLOW.md`. `.blend` files are not cooked or loaded at runtime.
- Cooker: `python tools/cook_assets.py`. Standalone runtime PNGs (`runtime_png`) use recipe `runtime_png.max512.lanczos.v1` (max 512 px, LANCZOS, no upscale). Pillow `12.3.0` is cooker-only (`python -m pip install -r tools/requirements.txt`). Blender authoring PNGs are not cooker inputs. Known GLBs plus extra valid `source/models/*.glb` files are opaque copies after static-GLB checks. PNGs under `source/textures/` remain explicit-list only.
- Runtime: CMake POST_BUILD and the M38 staging script (`cmake -P cmake/StageRuntimeAssets.cmake`) copy cooked files to `<exe dir>/assets/...`. Inventory: `cmake/RuntimeAssets.cmake`. The game never loads from `source/`.

If CMake configure reports a missing cooked asset, run the cooker command above.

## Cursor
Open the repository root in Cursor. The agent will pick up `AGENTS.md`, `.cursor/rules/*.mdc`, and `.cursor/skills/*/SKILL.md`.
