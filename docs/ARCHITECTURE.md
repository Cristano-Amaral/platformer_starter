# Architecture

## Direction
The game is a 3D platformer with a side/platform-style presentation. The player moves in a constrained gameplay plane/track while the world may use full 3D geometry. Camera behavior belongs to gameplay, while camera/input/window implementation details stay behind engine/backend boundaries.

## Dependency direction
`gameplay -> core abstractions`
`ui -> core/gameplay public state`
`render backend -> raylib initially`
`physics backend -> Jolt CharacterVirtual / Jolt world`
`platform backend -> OS/raylib platform services`

Gameplay must not depend on backend implementation headers.

## Runtime path
```
semantic input
    ->
Player gameplay movement policy (Player-relative)
    ->
PhysicsWorld project-owned API
    ->
Jolt kinematic moving platform (MoveKinematic)
    ->
CharacterVirtual ground velocity + Player-relative velocity
    ->
Jolt CharacterVirtual / Jolt world
    ->
project-owned Player state
    ->
Renderer / PlatformerCamera / DebugMetrics
```

Jolt CharacterVirtual is the authoritative Player collision and physical-position backend. There is no second custom Player AABB collision path.

Player owns gameplay policy: semantic movement intent, horizontal acceleration/deceleration relative to supporting ground, vertical gameplay velocity, jump, coyote time, and jump buffer. PhysicsWorld owns Jolt runtime state, CharacterVirtual, static greybox bodies, the kinematic moving platform, authored Dynamic Box bodies, and individual Dynamic Box kill-plane recovery.

Project-facing Player position is the **visual AABB center**. CharacterVirtual `GetPosition`/`SetPosition` is the **feet** (`visualCenter.y - visualSize.y * 0.5`). `PhysicsWorld::InitializePlayer` and `ResetCharacter` accept visual-center coordinates.

## Checkpoint / respawn (Milestone 20)

Checkpoint types and AABB helpers live in `world/RespawnWorld.h` (`CheckpointSpec`). Authored spawn, kill-plane Y, and checkpoint instances live in the immutable `LevelDefinition` loaded from the staged Level 01 file. Runtime state lives in `gameplay::RespawnState`, owned by `Application`. PhysicsWorld does not interpret checkpoints. Renderer does not own activation.

`input::InputState::respawnPressed` is edge-triggered (`R` mapped in the input backend only). Application owns the respawn decision after `Player::Update`: Fall if visual-center Y is below the active level's `killPlaneY`, else Hazard, else Manual if `respawnPressed`. Fall wins if both Fall and Hazard occur. At most one respawn per frame. Checkpoint activation runs only when no respawn happened that frame. Dynamic Box recovery uses the same authored `killPlaneY` inside `PhysicsWorld::Update` and does not respawn the player or reset other boxes.

`PhysicsWorld::ResetCharacter` teleports CharacterVirtual (feet), zeros linear velocity and airborne platform carry (`carriedGroundVelocityX`), refreshes contacts, and enforces fixed gameplay Z. `Player::ResetMovementState` clears relative horizontal/vertical velocity, coyote, jump buffer, and carry-related Player fields. `PlatformerCamera::SnapToTarget` copies the player visual center into both desired and smoothed targets. On a respawn frame the camera is snapped and `camera.Update` is skipped.

Checkpoint overlap tests `Player::Position()` (visual center) against checkpoint AABBs. No Jolt sensor. See Milestone 24 for the live two-checkpoint progression and marker states.

## Level goal / completion (Milestone 21)

The `LevelGoalSpec` type lives in `world/LevelGoal.h`. The authored Level 01 goal instance lives in the loaded `LevelDefinition`. Runtime state lives in `gameplay::LevelCompletionState`, owned by `Application`. PhysicsWorld does not interpret goals. Renderer does not decide completion.

Goal overlap tests `Player::Position()` (visual center) via `PointInsideGoal` against the active `LevelDefinition` goal. No Jolt sensor. Application sets `completed = true` once on first entry, only on a non-respawn frame after checkpoint evaluation. `PerformRespawn` does not clear completion. Renderer draws the two-post + bar marker from a `levelCompleted` bool and, after `EndMode3D`, draws `LEVEL COMPLETE` with raylib text in all configurations including Release. Dear ImGui Level Goal metrics remain Debug/Development only.

## Level restart (Milestone 22)

`input::InputState::restartPressed` is edge-triggered (`Enter` mapped in the input backend only). Application captures `restartAvailableAtFrameStart` from `levelCompletionState.completed` immediately after poll, before goal evaluation can mutate completion. After Fall/Manual and checkpoint/goal, if there was no respawn this frame and restart was already available at frame start and `restartPressed`, Application calls `RestartRun()`. Same-frame goal entry + Enter completes the level and does not restart.

`RestartRun()` resets physical Player/platform/cyan-box state, M20 `RespawnState` to defaults, M21 completion, and camera snap. On a restart frame `camera.Update` is skipped. Renderer draws `LEVEL COMPLETE` and `PRESS ENTER TO RESTART` after `EndMode3D` while `levelCompleted` is true, in all configurations including Release.

M20 respawn still does not reset the moving platform or cyan box. M22 restart does, via `PhysicsWorld::ResetMovingPlatform` and `ResetDynamicTestBox` (project-owned; no Jolt in public headers). Fall/Manual win over restart. Enter is inert before completion.

On a restart frame, `UpdateMovingPlatform` still runs before `RestartRun` so the physics-sensitive order stays intact. Restart then teleports the platform to the active `LevelDefinition` moving-platform start (`startX` / `centerY` / `centerZ`) with direction `+1`. The next frame resumes toward +X.

Status: complete (manually validated). Do not implement Milestone 23 in this section.

## Dynamic body interaction (Milestone 23)

The cyan box is a Jolt `EMotionType::Dynamic` 1 m cube. Mass is overridden to **30 kg** via `EOverrideMassProperties::CalculateInertia` so CharacterVirtual `maxStrength` 100 N can produce a meaningful impulse. Friction/restitution/damping remain Jolt defaults.

CharacterVirtual remains the movement authority (`mMass = 70`, `mMaxStrength = 100`). It now creates a **kinematic inner body** (`CharacterVirtualSettings::mInnerBodyShape`) so `PhysicsSystem::Update` cannot freely integrate the box into the character volume. The inner shape is the same translated capsule as the CharacterVirtual, scaled by **0.9** (Jolt sample `cInnerShapeFraction`), on object layer `Moving`. CharacterVirtual ignores its own inner body through Jolt's `IgnoreSingleBodyFilterChained`. `SetPosition` / `CharacterVirtual::Update` call `UpdateInnerBodyTransform`. No `CharacterContactListener`. Update order is unchanged.

Temporary blocking with no free space is valid. Manual validation: the Player is a physical barrier, can push/drag the 30 kg box, and is no longer permanently trapped.

Status: complete (manually validated). Milestone 45 restores the useful **behavior** (pushable dynamic box, gravity, world collision) as a repeatable authored Dynamic Box. It does not resurrect the hard-coded cyan probe. Canonical Level 01 has zero Dynamic Boxes. Do not implement Milestone 24 in this section.

## Shared greybox geometry
`world::Box` in `GreyboxWorld.h` is the project-owned AABB type. Canonical Level 01 ground and elevated platforms live in `game/assets/source/levels/level_01.level` and are loaded into `LevelDefinition`. Renderer and PhysicsWorld both derive from the active `LevelDefinition`. Ground and platform coordinates are not duplicated inside those systems.

The kinematic platform's immutable spec is `LevelDefinition::movingPlatform` (`MovingPlatformSpec`). PhysicsWorld owns runtime pose/direction/BodyID. Renderer draws authored size with the runtime pose.

Static test slopes are `LevelDefinition::slopes` (`SlopeSpec`: 30° walkable, 60° steep). CharacterVirtual max slope remains 50 degrees in PhysicsWorld, not level authoring. Walkable `OnGround` is valid gameplay support; `OnSteepGround` is not.

## Moving ground
The Player rides kinematic ground through CharacterVirtual: `UpdateGroundVelocity` then `GetGroundVelocity`, added to Player-relative horizontal speed. The Player is not parented to the platform and does not receive a manual position delta.

## Physics boundary
Jolt types stay inside `PhysicsWorld.cpp`. Public physics headers expose only project-owned types (`PlayerMoveCommand`, `PlayerPhysicsState`, `PlayerGroundSupport`, `DynamicBoxRuntimeState`, `MovingPlatformState`). The project has one physics backend: Jolt v5.6.0. There is no abstract `IPhysicsEngine`.

Applied authored Dynamic Box specs and `killPlaneY` are copied at `Initialize` / `TryRebuild`. Runtime pose lives on the Jolt body. `RecoverFallenDynamicBoxes` compares that body's center Y to the copied kill plane and resets only the fallen body (`SetPositionAndRotation` to the authored center and `Quat::sIdentity()`, then zero linear/angular velocity). It is not a second transform authority for rendering or picking.

## Early abstraction points
Only abstract boundaries that are known to vary by target:
- application/platform lifecycle;
- input device mapping to semantic actions;
- filesystem/save location;
- timing;
- graphics/window backend exposure;
- physics integration boundary;
- asset location/loading.

Do not build a general-purpose engine before the game needs it.

## Asset pipeline
Three locations are distinct:

- `game/assets/source/` — authored source (Save Level Source writes here).
- `game/assets/cooked/` — canonical cooked repository copies (`python tools/cook_assets.py` writes here).
- `build/windows-vs2022/bin/<Config>/assets/` — staged runtime copy the built game loads.

Save Level Source is not Cook Assets. Cook Assets is not runtime staging. The cooker never copies into `<exe>/assets/`. CMake does not cook. Staging is `cmake -P cmake/StageRuntimeAssets.cmake` with `-DPLATFORMER_COOKED_DIR` and `-DPLATFORMER_STAGE_DEST`, sharing `cmake/RuntimeAssets.cmake`. `Platformer3D` POST_BUILD invokes that script into `$<TARGET_FILE_DIR:Platformer3D>/assets` for Debug, Development, and Release. The same script is the M38 Stage Runtime Assets child process (Development dest: `build/windows-vs2022/bin/Development/assets`). Staging does not compile or link. `copy_if_different` / `file(COPY_FILE ... ONLY_IF_DIFFERENT)` skips identical bytes. Stale destination files are not deleted. Build Development remains `cmake --build --preset windows-development`.

The game never loads from `source/` or `cooked/`. After authored level edits, a fresh-restart test uses Save → Cook & Stage → Restart, or Development `Level > Reload Runtime Level` / `Build > Cook, Stage & Reload` from the staged file without closing the process. `Build > Stage Runtime Assets` always targets the Development runtime tree. Reload does not Save, Cook, Stage, or Build. Cook, Stage & Reload does not auto Apply or auto Save.

Cook from the repository root:

    python tools/cook_assets.py

The cooker uses the Python standard library plus cooker-only **Pillow 12.3.0** (`python -m pip install -r tools/requirements.txt`; not a CMake or game runtime dependency). Required assets remain the explicit `KNOWN_ASSETS` list. Milestone 47 additionally discovers valid `source/models/*.glb` files, cooks them as opaque `copy` after the same static-GLB compatibility checks used by Development import, and writes portable relative paths into `game/assets/cooked/manifest.json` (no absolute paths, timestamps, or machine names). PNGs under `source/textures/` stay explicit-list only. Blender authoring PNGs (for example `test_textured_basecolor.png`) stay out of the cooker.

Standalone runtime PNGs (`kind: runtime_png`) use recipe `runtime_png.max512.lanczos.v1`: maximum dimension **512 px**, aspect ratio preserved, no upscale, no crop, Pillow `LANCZOS` when downscaling. Sources already within the limit are copied byte-for-byte. GLBs remain opaque byte copies; images embedded in `models/test_textured.glb` are not resized. Changing the recipe (max dimension, filter, or encoding) recooks even if source bytes are unchanged. See `tools/README.md`.

Staged runtime files (POST_BUILD `copy_if_different` from cooked):

    <executable directory>/assets/textures/test_checker.png
    <executable directory>/assets/models/test_static.glb
    <executable directory>/assets/models/test_authored.glb
    <executable directory>/assets/models/test_textured.glb
    <executable directory>/assets/levels/level_01.level

Runtime lookup uses `platform::RuntimeAssetPath`, which joins `<executable directory>/assets/` with the logical relative path. The executable directory is queried from the OS in the platform layer (`GetModuleFileNameW` on Windows, `/proc/self/exe` on Linux). The process current working directory is never used, and non-absolute results are rejected so raylib file loads cannot silently resolve against CWD. The renderer does not hard-code `game/assets/cooked`.

Logical identities:
- Milestone 15 test texture: `textures/test_checker.png`
- Milestone 16 test model: `models/test_static.glb`
- Milestone 17 authored model: `models/test_authored.glb`
- Milestone 18 textured model: `models/test_textured.glb`
- Milestone 31 Level 01: `levels/level_01.level` (required; missing/invalid is fatal)

The M18 Base Color PNG `game/assets/source/textures/test_textured_basecolor.png` is Blender authoring input only. It is not cooked or staged. The exported GLB must embed the image. See `docs/BLENDER_WORKFLOW.md`.

Development `Assets > Import Static GLB` copies a compatible self-contained static `.glb` into `game/assets/source/models/<filename>.glb`. Canonical identity is the project-relative path `models/<filename>.glb`. The original external absolute path is import input only. Collision never overwrites. Import does not cook, stage, or mutate `workingCopy` / `active` / `savedSourceBaseline`. A derived `assets::StaticModelCatalog` discovers valid `source/models/*.glb` files (non-recursive, sorted by identity). It is not persisted and is not a level/scene object list. After import, the existing Cook Assets then Stage Runtime Assets path processes extra cooked `models/*.glb` files. Staging's required inventory remains `cmake/RuntimeAssets.cmake`; extra cooked models are discovered at staging time. Debug has no import UI. Release consumes only staged runtime assets.

Editable Blender files live in `game/assets/source/blender/`. They are not cooked and are not runtime assets. See `docs/BLENDER_WORKFLOW.md`. The cooker copies exported GLBs unchanged. Blender is not a build or runtime dependency.

The checker, Milestone 16 pyramid, Blender-authored model, and textured model remain cooker/staging inventory (`cmake/RuntimeAssets.cmake` and `python tools/cook_assets.py`). Milestone 44 does not load or draw them in the canonical Level 01 scene. They have no GreyboxWorld entries and no Jolt bodies.

If a required cooked file is missing at CMake configure time, configure fails and tells the developer to run the cooker. Milestone 44 no longer loads those test GLBs/PNG into the renderer, so a missing staged probe is a cooker/staging concern rather than a magenta/orange fallback cube in Level 01.

## Extended traversal / two checkpoints (Milestone 24)

M24 is a longer hardcoded greybox plus **exactly two ordered checkpoints**. It is not a generic level, trigger, or checkpoint framework.

Live world data (canonical instances in `game/assets/source/levels/level_01.level`, loaded into `LevelDefinition`):

- Ground `{0, -0.25, 0}` size `{56, 0.5, 8}` (X [-28, 28], top Y = 0).
- Elevated platforms in `LevelDefinition::elevatedPlatforms` (right early, left landing, CP1 support, mid-left step, CP2 support, goal support).
- Checkpoint 1 `{16.5, 1.8, 0}` size `{2.4, 1.6, 2.0}` respawn `{16.5, 1.8, 0}`.
- Checkpoint 2 `{-15.5, 2.8, 0}` size `{2.4, 1.6, 2.0}` respawn `{-15.5, 2.8, 0}`.
- Goal `{ -21.0, 3.8, 0 }` size `{2.0, 1.6, 1.8}` (two-post gate; not a checkpoint).
- Walkable 30° slope at `{21.70, 1.6732, 0}` (optional dead-end past CP1). 60° slope at `{25.60, 0.966, 0}` (classification dead-end past the 30° test).
- Moving platform path unchanged. Swept AABB X [-8, 8], Y [1.1, 1.5], Z [-1.5, 1.5].

`LevelDefinition::checkpoints` is `std::array<CheckpointSpec, 2>`. Identity 0 = Checkpoint 1, 1 = Checkpoint 2. `RespawnState::activeCheckpointIndex` is `-1` (none), `0`, or `1`. Activation on a no-respawn frame is `expectedIndex = active + 1` against `level.checkpoints[expectedIndex]` only. CP2 cannot activate before CP1. Backtracking cannot downgrade. Enter restart restores `activeCheckpointIndex = -1` and the level initial spawn.

Application derives `CheckpointVisualState` (Future / Current / PreviouslyActivated) and passes `std::array<CheckpointVisualState, 2>` to Renderer. Renderer draws two post+beacon primitives from those states and checkpoint specs; it does not test overlap or mutate respawn. Debug/Development metrics show Active checkpoint None/1/2 plus per-checkpoint inside/state.

Intended route: spawn → Checkpoint 1 (open ground; 30° slope is optional and past CP1) → back to moving platform → left landing → mid-left step → Checkpoint 2 → goal.

Phase B.1: the M14 30° slope at `{10.90, 1.6732, 0}` rose with +X and ended ~3.35 high immediately left of CP1. Players could walk up and drop onto CP1 but could not return: jump rise 1.6 cannot clear the high end, and the underside wedges anyone walking back on the ground. The 30° test was moved past CP1; the 60° test moved just beyond it; ground expanded to X [-28, 28]. Checkpoint order was not a defect (CP2 before CP1 must stay inactive). The user manually approved spawn -> CP1 -> center return.

Status: complete (manually validated). Do not implement Milestone 25 in this section.

## Static hazards / hazard respawn (Milestone 25)

M25 adds the first explicit non-fall lethal volumes: **exactly two** `HazardSpec` AABBs authored in the Level 01 file. Identity is the array index. There is no health, no Jolt sensor, and no generic trigger type.

- Hazard 1 (index 0): corridor spikes `{11.5, 0.5, 0}` size `{1.4, 1.0, 2.0}` AABB X [10.8, 12.2], Y [0, 1.0], Z [-1, 1]
- Hazard 2 (index 1): goal-gap spikes `{-18.5, 0.5, 0}` size `{1.2, 1.0, 2.0}` AABB X [-19.1, -17.9], Y [0, 1.0], Z [-1, 1]

Application tests `Player::Position()` (visual center) with `FindHazardIndexContaining`. Multiple overlapping volumes still produce **one** Hazard death. Priority:

```
Fall > Hazard > Manual R > checkpoint / goal > Enter (M22 restartAvailableAtFrameStart)
```

`PerformRespawn` increments `deathCount` for Fall or Hazard, never Manual. Destination is `respawnState.respawnPosition`. Ordinary Hazard respawn does not reset the moving platform or cyan box. After completion, Hazard death preserves `completed`. Enter restart does not need a hazard reset API (static world specs). Renderer reads `LevelDefinition::hazards` and draws a red/orange bar matching the AABB plus three cube teeth on the top face; it does not detect contact. Debug/Development metrics show Inside hazard None/1/2 and Hazard contact this frame. Release draws hazards and runs the same death logic without ImGui.

User-confirmed Phase C evidence: hazard death +1; respawn at initial spawn before any checkpoint, CP1 after CP1, CP2 after CP2; Enter after LEVEL COMPLETE starts a fresh run and resets deathCount to 0.

Status: complete (manually approved). Do not implement Milestone 26 in this section.

## Collectibles / run counter (Milestone 26)

M26 adds the first non-lethal collectible loop. Canonical Level 01 still authors **three** `CollectibleSpec` AABBs. Identity remains the container index. Per-run flags live in `gameplay::CollectibleRunState` (`std::vector<std::uint8_t>` sized to the active level). Milestone 41 made the count variable; there is no 16-object design cap. Collection is still optional and must not gate the goal.

- Collectible 1 (index 0): right-platform hop `{5.0, 2.5, 0}` size `{1.0, 1.2, 1.0}`
- Collectible 2 (index 1): left-landing hop `{-4.5, 4.0, 0}` size `{1.0, 1.2, 1.0}`
- Collectible 3 (index 2): middle-left-step hop `{-10.0, 3.75, 0}` size `{1.0, 1.2, 1.0}`

Standing on the support does not collect (AABB sits just above standing center). A normal hop does. No Jolt sensor. Ordinary R/Fall/Hazard preserve flags; only Enter `RestartRun` clears them. Collection runs in the no-respawn branch after checkpoint/goal and is skipped when `restartAvailableAtFrameStart && Enter`.

Renderer receives collected flags plus derived count, draws a gold 0.45 cube for available items only, and always draws `COLLECTED N / <level collectible count>` in the upper-right after `EndMode3D`. That runtime skip is correct for gameplay. Opening the Development/Debug editor does not reset `CollectibleRunState`. Collected authored items stay in `LevelDefinition` and remain selectable; F2 draws a gold wireframe cube at the **active** authored center only when the runtime cube is hidden. Uncollected items keep the single runtime cube (no second opaque overlay). Debug/Development metrics show Available/Collected, Inside, and Collected this frame.

Status: complete (manually approved). Do not implement Milestone 27 in this section.

## Run timer / completion time (Milestone 27)

M27 adds a current-run elapsed timer owned by Application as `gameplay::RunTimerState` (`double elapsedSeconds`, `bool frozen`). There is one source of truth: `elapsedSeconds` itself; when frozen it is the completion time. No `bestTime`, previous-run time, persistence, TimerManager, or RunManager.

Accumulation uses the existing once-per-frame gameplay delta from `platform::DeltaSeconds()` (`float` seconds via raylib `GetFrameTime()`), stored as `deltaSeconds` in `Application::Run`. Do not introduce a wall clock or a second timing source. Advance at most once per frame after `restartAvailableAtFrameStart` and before moving-platform/Player update: `if (!frozen) elapsedSeconds += dt`. Freeze on the first `completed` false → true inside the existing goal branch. That frame's dt is included. Ordinary Manual/Fall/Hazard respawns must not clear timer state. Enter `RestartRun` resets `elapsedSeconds = 0` and `frozen = false`.

Display formatting is a pure helper in `core/RunTimeFormat.h` (`MM:SS.mmm`, floor to whole milliseconds). Renderer receives read-only `elapsedSeconds` and draws Release-visible `TIME MM:SS.mmm` in the upper-left (~20 px) after `EndMode3D`. `COLLECTED N / 3` stays upper-right; completion UI stays centered. Renderer must not own or advance time. PhysicsWorld stays unaware of the timer. Debug/Development metrics show run time seconds, Running/Frozen, and the same formatted `MM:SS.mmm`.

Status: complete (manually approved). Do not implement Milestone 28 in this section.

## Session best time (Milestone 28)

M28 adds a session-only best completion time owned by Application as `gameplay::SessionBestTimeState` (`bool hasBestTime`, `double bestSeconds`). Run state (`RunTimerState`) lasts one run and resets on Enter. Session BEST lasts the process and is **not** reset by Enter; only executable relaunch returns to no-best.

Comparison uses the raw frozen `runTimerState.elapsedSeconds` at the existing first-completion branch (`!completed && PointInsideGoal`). Update when `IsBetterSessionCompletion` (`!hasBestTime || elapsedSeconds < bestSeconds`). Ties and slower runs keep the previous record. Store the raw double; format with `core/RunTimeFormat.h` / `FormatSessionBestTime`. No-best HUD is `BEST --:--.---` directly below `TIME` at upper-left (y = 46); `COLLECTED N / 3` stays upper-right; completion stays centered. Renderer receives read-only `hasBestTime` + `bestSeconds` and must not compare or mutate records. `RestartRun` resets the current-run timer and does not reset session BEST. Debug/Development metrics show Has session best, Session best seconds (N/A when none), and formatted BEST.

Status: complete (manually approved). Do not implement Milestone 29 in this section.

## Persistent best time (Milestone 29)

M29 persists exactly one gameplay datum: the M28 session BEST. Application still owns whether a completion is better (`IsBetterSessionCompletion`). Persistence only serializes/parses a versioned text file:

```
PLATFORMER_SAVE 1
best_seconds <double>
```

Magic `PLATFORMER_SAVE`, version `1`, `std::numeric_limits<double>::max_digits10`. Load once at Initialize; save only when a new in-memory BEST is established. Missing/invalid/unsupported/IO errors leave no-best (`BEST --:--.---`) without crashing. Save failure keeps the in-memory record.

Writable path: `platform::UserDataDirectory()` (`FOLDERID_LocalAppData` on Windows) / `Platformer3D` / `best_time_v1.txt`. Sibling temp: `best_time_v1.tmp` (never loaded). Not CWD, not exe directory, not `assets/`. POSIX user-data discovery is not implemented in M29.

Final-file promotion is a tiny platform primitive, `platform::ReplaceFileWithTemporary(temporaryPath, finalPath)`. Persistence writes the complete v1 text to the temp sibling, flush/closes it, then asks the platform to promote. There is no standalone `remove(final)` before promotion. Windows (validated for M29): `ReplaceFileW` when the canonical file already exists; `MoveFileExW` with `MOVEFILE_WRITE_THROUGH` for the first-ever save. POSIX: `std::filesystem::rename` for compile compatibility only. Application/gameplay/parser must not include `Windows.h` or call Win32 replacement APIs.

This avoids a delete-then-rename window (old valid save gone, temp not yet promoted). It is not a transactional, journaled, fsync, or power-loss-proof store. Guarantees are: avoid obvious partial final writes; do not delete the old final as a separate step; flush/close the temp before promote. A leftover `.tmp` may remain; load ignores it.

Application zeros `sessionBestTimeState`, then loads once during Initialize. Only `Loaded` populates BEST. Missing/invalid/unsupported/IO leave `BEST --:--.---`. Save runs only inside the existing M28 new-record branch after the in-memory update. Save failure keeps the new in-memory BEST. Load does not create the user-data directory. Directory creation belongs to save only. Enter/R/Fall/Hazard/checkpoints/collectibles do not save. Only BEST persists; current TIME, completion, and other run state do not.

Status: complete (manually approved). Save v1 remains compatible with Milestone 30's single playable level. Do not implement save v2.

## Level data v1 (Milestone 30)

M30 introduces a project-owned immutable `world::LevelDefinition` for the current single playable level (`level_01`). At M30 the canonical authored values lived in `world/Level01.cpp` (`CreateLevel01Definition()`). Milestone 31 replaced that compiled factory as the live authored source; `LevelDefinition` remains the runtime data model.

Application owns one `LevelDefinition` that is empty until Initialize successfully loads the staged Level 01 file, then passes it read-only to PhysicsWorld (`Initialize(level)`) and Renderer (`DrawWorld(..., level, ...)`). Gameplay meaning (checkpoint activation, hazard death, collectible pickup, goal completion, timer, BEST) stays in Application. Runtime pose for the moving platform stays in PhysicsWorld. `collected[]`, `activeCheckpointIndex`, `LevelCompletionState`, timer, and BEST stay outside `LevelDefinition`.

Camera offset `{2, 3.5, 12}` and FOV 40 are level framing (`LevelCameraSpec`). Dead zone X/Y `1.5` / `0.75` and follow sharpness `8` remain `PlatformerCamera` controller policy. Player visual size `{0.8, 1.6, 0.8}` remains character/render configuration (`world::kPlayerVisualSize`), not level authoring. CharacterVirtual max slope remains 50° in PhysicsWorld.

Only BEST is persisted (`PLATFORMER_SAVE 1` / `best_seconds`). The save does not store `level_01`. No editor, LevelManager, or second playable level.

Phase B: live consumers migrated. World type headers keep reusable specs/helpers and no longer own Level 01 instance arrays. Debug/Development metrics include a read-only Level Data section sourced from the active definition.

Status: complete (manually approved). Live authored coordinates moved to the external Level 01 file in Milestone 31. Do not implement Milestone 32 in this section.

## External level file v1 (Milestone 31)

M31 introduces a project-owned text format `PLATFORMER_LEVEL 1` and parser `world::ParseLevelText` / `LoadLevelFile` (`world/LevelFile.h`). Canonical authored source: `game/assets/source/levels/level_01.level`. Cooker kind `level_v1` copies bytes after a UTF-8/header check. CMake stages `<exe>/assets/levels/level_01.level`. Grammar: `docs/LEVEL_FORMAT_V1.md`. `LevelDefinition.id` is an owning `std::string`.

**Phase B:** the staged runtime file is the **only** live authored source. Application owns `levelDefinition{}` until Initialize. It resolves `platform::RuntimeAssetPath("levels/level_01.level")`, calls `LoadLevelFile` once, requires status `Loaded`, requires `id == "level_01"`, validates authored content, stores the definition, applies camera framing, initializes respawn from the loaded spawn, initializes PhysicsWorld from the definition, then initializes Player at `initialSpawnVisualCenter`. PhysicsWorld and Renderer still consume `const LevelDefinition&` only; they do not open or parse the file. RestartRun and ordinary respawns do not reread the file.

There is no `CreateLevel01Definition()` and no `Level01.cpp`. Missing, invalid, unsupported-version, I/O error, or wrong Level ID is a fatal initialization failure (stderr diagnostics; no compiled fallback; no empty world). BEST save missing/invalid remains nonfatal. Debug/Development expose a Level Loading section (runtime path, load status, format version, loaded ID) plus the M30 Level Data section. Release has no ImGui and follows the same required-level policy.

Save v1 is unchanged. Camera file fields are offset + FOV only. Player feel and CharacterVirtual policy stay out of the file. No editor, writer, LevelManager, or second playable level.

Status: complete (manually approved and merged).

## Development level editor v1 (Milestone 32, Phase A)

Phase A is architecture, the Level Format v1 writer, and inert scaffolding. Nothing in Phase A lets a running build modify or save the canonical level.

### Writer

`world/LevelWriter.h` is the inverse companion to the M31 parser and lives beside it in `world/`, so one module pair owns the whole v1 contract. `SerializeLevelText` emits the canonical record order documented in `docs/LEVEL_FORMAT_V1.md`, deterministically, using `std::to_chars` shortest round-trip form. `IsWritableLevelDefinition` (`IsValidLevelIdToken` + `LevelDefinitionHasRequiredAuthoredContent`) gates both serialization and saving, so an invalid definition produces no text and reaches no file. `WriteLevelFileStatus` is `Saved`/`Invalid`/`Error` — a focused status, not a generic engine result type.

`SaveLevelFile` requires an absolute path, writes the sibling temp `<target>.tmp`, flush/closes it, then promotes through the M29 boundary `platform::ReplaceFileWithTemporary`. The writer contains no Win32. `ReplaceFileWithTemporary` moved from `RuntimePaths{Windows,Posix}.cpp` into its own `FileReplace{Windows,Posix}.cpp` TU so the parser/writer test target can link the boundary without pulling in `SHGetKnownFolderPath`/ole32/shell32.

### Authoring path boundary

The runtime reads `<exe>/assets/levels/level_01.level` through `platform::RuntimeAssetPath`. The editor must instead write `game/assets/source/levels/level_01.level`. These are intentionally different files and the staged copy is never treated as canonical source.

`editor/AuthoringPaths.h` resolves the source path. The root arrives as the compile definition `PLATFORMER_AUTHORING_SOURCE_ROOT`, injected by CMake from `${CMAKE_SOURCE_DIR}/game/assets/source` for the **Development** configuration only, alongside `PLATFORMER_ENABLE_LEVEL_AUTHORING`. There is no CWD use and no parent-directory searching for `.git`/`CMakeLists.txt`/`game/assets/source`. Release receives neither macro, so the preprocessor removes the path literal entirely and the shipped binary stays relocatable and carries no developer repository path. Authoring paths never enter `LevelDefinition`, gameplay, or the runtime parser, so runtime portability is unaffected.

Configuration policy: Release has no ImGui and no editor. Debug and Development both compile the editor scaffolding (both define `PLATFORMER_ENABLE_DEBUG_UI`), but only Development can author. `PLATFORMER_ENABLE_DEBUG_UI` means "Dear ImGui is present"; `PLATFORMER_ENABLE_LEVEL_AUTHORING` means "this build may write project source". Keeping them separate avoids widening write exposure just because tooling code compiles.

### Editor ownership

`editor/LevelEditor.h` holds `LevelEditorState` (`active`, `modified`, `dirty`, `selection`, `editorCamera`, apply/save status). Application remains the sole owner of the active `LevelDefinition` and of all gameplay state; it holds the editor state next to `DebugUi` under `PLATFORMER_ENABLE_DEBUG_UI`. There is no EditorManager, LevelManager, SceneManager, EntityManager, ECS, event bus, property/value model, or reflection.

## Development level editor v1 (Milestone 32, Phase B)

Phase B makes the editor live in Development while leaving M31 gameplay and Release untouched.

### Toggle and keyboard capture

`input::InputState::toggleLevelEditorPressed` is the semantic action, mapped to the F2 edge press inside `input/Input.cpp`. Application consumes only the flag and calls no backend input API. `DebugUiBackend::WantsKeyboardCapture()` returns `ImGui::GetIO().WantCaptureKeyboard` (and `false` where ImGui is not compiled in), surfaced through `DebugUi::WantsKeyboardCapture()`. Application ignores the toggle while that is true, so typing a value into an editor field cannot close the editor. ImGui knowledge stays in the debug-UI backend rather than leaking into input or gameplay.

### Simulation pause

One guard in the frame loop. Input polling, the editor toggle, rendering, the debug UI, the editor panel and window-close handling stay outside it; the entire M31 simulation and mutation block — run timer, moving platform, Player, hazard detection, fall/hazard/manual respawn, checkpoint activation, goal completion, BEST comparison and save, collectible pickup, Enter restart, `PhysicsWorld::Update`, and camera follow — sits inside `if (!simulationPaused)`. The block's internal order and content are unchanged from M31, so closing the editor restores exact M31 behaviour. There is no time-scale system and no per-system pause flag.

### Ownership and the working copy

Application still owns the active `world::LevelDefinition` and all gameplay state. `editor::LevelEditorState` owns only authored data: a `workingCopy` the panel edits and a `savedSourceBaseline` recording what was last written to source. `Modified` and `Dirty` are derived every draw from `world::AuthoredLevelDataEqual` rather than from widget return values, so a field that reports "edited" without changing its value does not mark the level modified. That helper was lifted from the writer test into `world/LevelDefinition.h`, so the same comparison the editor trusts is the one the round-trip tests prove. It is an explicit field list, not reflection.

The panel never mutates gameplay. `DrawLevelEditor` returns an `editor::LevelEditorRequest` (`None` / `ApplyPreview` / `RevertWorkingCopy` / `SaveLevelSource`) and Application executes it **after** `renderer.EndFrame()`. That is the commit point: the swap of active definition plus PhysicsWorld happens between frames, so no frame can draw geometry that disagrees with the physics it was drawn from.

### Apply Preview

Validate `world::IsWritableLevelDefinition(workingCopy)` and the `level_01` identity first, while the live world is still intact — an invalid working copy therefore cannot shut physics down, move the camera, or reset gameplay. Then `PhysicsWorld::Shutdown()`, `Initialize(candidate)`, `InitializePlayer(candidate spawn)`, and only afterwards `levelDefinition = candidate`. Committing last means the active definition never describes a world that failed to build.

If `Initialize` or `InitializePlayer` fails after `Shutdown`, M32 does not attempt a rollback transaction: it reports the failure on stderr, sets the editor's Apply status to `Error`, and raises `Application::fatalError`, which exits the frame loop through the normal `Shutdown()` path and makes `Run()` return 1. Running on partial physics is not an option.

A successful Apply starts a fresh editor preview run: Player at the authored spawn with movement state cleared, `RespawnState`, `LevelCompletionState`, `CollectibleRunState` and `RunTimerState` reset, moving platform reset through the rebuild, and `camera.ApplyLevelFraming` plus `camera.Initialize` so offset and FOV preview immediately. `SessionBestTimeState`, the persisted BEST, the persistence statuses, the loaded level id and the level-loading diagnostics are deliberately untouched — the editor can never write BEST.

`Revert Working Copy` assigns `workingCopy = levelDefinition` and nothing else: no physics rebuild, no camera change, no effect on Dirty, no file access. There is no `Reload Runtime Level` in M32.

### Saving

`editor::SaveLevelSource` is compiled only under `PLATFORMER_ENABLE_LEVEL_AUTHORING`; Debug and Release link a stub that returns `Error` and contains no path and no write call. It resolves `editor::AuthoringLevel01SourcePath()` and delegates to the Phase A `world::SaveLevelFile`, keeping the safe temp-plus-`ReplaceFileWithTemporary` promotion and the absolute-path requirement.

Save always serializes the **active** definition, and the panel disables the button while `Modified` is true, so the workflow is unambiguous: edit, Apply Preview, Save. A successful save updates `savedSourceBaseline`, which clears `Dirty`; a failure leaves the baseline and `Dirty` alone and the previous source contents intact. Nothing cooks, rebuilds, restarts, or spawns a process.

### Editor session semantics

Activating the editor copies the active definition into the working copy and clears `Modified`; it touches no gameplay state and no file. Closing does not auto-apply or auto-save, and reopening re-seeds the working copy, so unapplied edits are discarded — F2 stays deterministic and M32 needs no unsaved-changes modal. `Dirty` is derived from the active definition, so it survives the toggle for the life of the process.

The dirty baseline is seeded in `Initialize` from the staged level that was just loaded, so `Dirty` starts false and tracks only this session's applied edits. M32 does not reconcile a staged copy that disagrees with the repository source, does not watch the filesystem, and does not detect external edits; the next explicit save overwrites them.

Status: complete (manually approved and merged).

## Visual level editor v2 (Milestone 33, Phase B)

Phase B makes the visual editor live on top of the M32 contract. F2 still pauses the whole simulation, edits a working copy, and Apply/Revert/Save keep the same Modified/Dirty/authoring rules. M33 adds navigation and selection only.

### The four questions (now live)

**A. Editor camera.** A dedicated `editor::EditorCamera` (position, yaw, pitch, speed, FOV) owned by `LevelEditorState`, not `PlatformerCamera`. Navigation never writes `LevelDefinition.camera`. `Renderer::DrawWorld` consumes a project-owned `render::CameraView` and does not own camera state: gameplay builds it from `PlatformerCamera`; the editor builds it from `EditorCamera`. First F2 seeds from `gameplayTarget + offset` / current FOV; later F2 toggles in the same process keep the pose. Apply Preview may reset the gameplay camera; it does not move the editor camera. On editor exit, `SnapToTarget(Player)` so follow state does not interpolate from a stale pose. Disk persistence: none. RMB look hides the cursor only while held, via `input::SetMouseLookActive` in the input backend.

**B. Selection.** One `editor::EditorSelection { EditorObjectKind kind; size_t index; }` on `LevelEditorState`. Type + index is still the identity; M41 reconciles it after structural working-copy edits instead of introducing GUIDs. No UUID. Hierarchy, picking, Inspector and highlight share this identity. Inspector resolves fields against `workingCopy` each frame by index. Viewport picking and highlight resolve transforms against the **active/applied** `LevelDefinition` plus runtime poses for moving objects. While a lifecycle category has pending Add/Duplicate/Delete, active-world picks of that category are ignored so an applied index cannot select the wrong workingCopy slot. Apply keeps the selection if it is still valid; Revert reconciles against active; Reload still clears it.

**C. Mouse to object.** `editor::PollEditorInput()` (input backend, not Application/raylib) supplies mouse position, LMB press and RMB hold. `platform::Window::Width/Height` supply the resizable viewport. `ScreenToWorldRay(CameraView, mouse, viewport)` builds a project-owned `Ray3`. `BuildPickingSet(appliedLevel, runtime poses)` emits CPU proxies from the visible active world. Visible pending Add/Duplicate/Modify ghosts emit editor-only `PendingPickProxy` volumes from `CollectPendingAuthoringVisuals` (workingCopy index, no physics). `TryResolveEditorViewportPick` prefers the nearest pending hit, then `PickNearest` on the active set mapped through `StructuralIndexMap`. Ties on the active set keep the earlier proxy (hierarchy order); pending ties keep the nearer hit. Unapplied singleton Inspector edits still do not move active pick/highlight. Jolt raycasts are not used.

**D. Synchronization.** Hierarchy click and world pick both assign `state.selection`. Inspector routes on that value. Highlight is an `EditorHighlightRequest` built from the same proxy the picker used; Renderer draws it read-only from `DebugWorldOverlay`.

### Live UI

Three ImGui windows, no docking: `Hierarchy` (built from `workingCopy` via `BuildHierarchyEntries`, collapsible groups for Platforms/Slopes/Checkpoints/Hazards/Collectibles), `Inspector` (selected object only; stale type+index shows no object), `Level Editor` (status, Apply/Revert/Save). None shows "No object selected." Read-only kinds use `Text`, never `InputFloat`.

### Picking proxies

| Kind | World pick | Proxy |
|---|---|---|
| Player Spawn | yes | `kPlayerVisualSize` AABB at **applied** spawn (Debug/Development marker) |
| Camera | no | framing spec, not a placed object |
| Ground / Platform N | yes | **applied** AABB |
| Slope | yes | **applied** oriented local AABB |
| Moving Platform | yes | **runtime** center/size (visible frozen pose), Inspector stays authored read-only |
| Checkpoint / Hazard / Goal | yes | **applied** trigger AABB |
| Collectible | yes | `world::kCollectibleVisualSize` cube at **applied** center |
| Dynamic Box | yes | **runtime** Jolt center/size (active body). Pending Add/Modify uses authored workingCopy. |
| Runtime Player | no | not an authored object |

Empty LMB click (not captured by ImGui) clears selection. RMB look never picks. Keyboard move, world pick and wheel run after ImGui so this frame's `WantsKeyboardCapture()` / `WantsMouseCapture()` block them. RMB look still uses the previous-frame mouse-capture flag (same one-frame lag as M32 F2). Inspector continues to edit `workingCopy`; the spawn marker and static pick/highlight proxies use the active definition until Apply Preview.

### Controls

RMB held: mouse-look. WASD: move on look XZ. Q/E: world down/up. Shift: 2× speed. Wheel: movement speed (`1..40`). Ignored while ImGui wants keyboard or mouse.

Status: complete (manually approved and merged).

## Visual level editor v3 (Milestone 34)

M34 is complete and merged. Translation gizmo, pending ghost, persistent layout, and depth-independent overlay remain the live editor path.

### Translation gizmo

**Targets.** Spawn, Ground, Elevated Platform 0..5. Camera stays numeric-only. M33 read-only kinds stay read-only. Runtime Player is not selectable.

**Ownership.** `editor::GizmoInteractionState` lives on `LevelEditorState`. `GetEditablePosition` writes `workingCopy`. Application copies `GizmoDrawRequest` / `EditorPendingTransformPreview` into `render::DebugWorldOverlay` PODs. Renderer does not own selection, workingCopy, or drag state.

**Draw order (inside `BeginMode3D`).** Normal world → active spawn marker → active M33 highlight → Development pending object ghost (checkpoint post/beacon, full hazard, collectible cube) → cyan **wireframe** pending bounds (working geometry; no filled cube, so the object's own bounds do not hide its ghost) → Checkpoint respawn marker → X/Y/Z gizmo. The gizmo uses a depth-tested faint pass then a depth-independent overlay (`rlDisableDepthTest` + `rlDisableDepthMask` + `rlDisableBackfaceCulling`); those three states are restored immediately after the overlay batch flush. ImGui draws after `EndMode3D`, so panels sit over the gizmo.

**Size.** Axis length = `distance * tan(fovY/2) * 0.22`, clamped to `[0.75, 24]`. Visual shaft/head use cylinders/cones (`max(0.038 * length, 0.045)` shaft radius); hit radius remains `0.09 * length` on the world-space shaft (hub skip unchanged).

**Live tick.** After this-frame ImGui capture: `editor::UpdateGizmoInteraction` uses `PickGizmoHandle` / `BeginGizmoDrag` / `GizmoDragPosition`. Priority: ImGui > active drag/handle > M33 world pick. RMB never starts a drag; active LMB drag suppresses editor-camera look until release. Drag target identity is captured at press; Hierarchy clicks are ignored while dragging.

**Drag.** Camera-facing plane containing the selected world axis; if `|axis × viewForward| < 0.05`, closest-points between mouse ray and axis. Position = `dragStart + (param - startParam) * axis`. X/Y/Z each leave the other two coordinates unchanged. Unusable rays keep the start pose (no NaN). Apply/Revert/F2 call `ClearGizmoInteraction` so a drag cannot survive a physics rebuild.

**Sync.** Gizmo origin and pending preview use working-copy geometry. Active pick/highlight stay on the applied world until Apply Preview. Inspector reads the same `workingCopy` (one-frame latency after drag is acceptable).

### Persistent layout

Dear ImGui `IniFilename` is a backend-owned `std::string` pointing at `%LOCALAPPDATA%\Platformer3D\editor_layout.ini` (via `platform::UserDataDirectory()`, same project folder as BEST, not BEST serialization). It is set after `rlImGuiSetup` and before the first NewFrame. Null if the user-data path cannot be resolved. Debug and Development share the file. Release never touches it.

First-run / missing file: `ImGuiCond_FirstUseEver` applies `ComputeDefaultEditorLayout` (metrics/hierarchy left, inspector/level editor right). Persisted ini wins afterwards. **Reset Editor Layout** (Level Editor actions) snaps all four named windows with `ImGuiCond_Always` / `SetWindowPos` by name and `SaveIniSettingsToDisk`. Off-screen recovery is one-shot after first `Begin` of Metrics and of the editor windows, using `ClampEditorWindowPlacement`. No docking. Window names are stable: `Platformer3D Metrics`, `Hierarchy`, `Inspector`, `Level Editor`.

Status: complete and merged.

### Future editor notes (scheduled as Milestone 35)

M35 Phase A implements math/tests for:

- visual resize handles (Ground / Platform 0..5, center-preserving box size);
- a screen-space orientation/view widget;
- keyboard object nudge (Ctrl+arrows / PageUp / PageDown);
- Alt + mouse-wheel editor-camera dolly.

Live wiring is Phase B. Still out of M35: add/delete/duplicate, rotation, Hierarchy coverage of cooker probes.

### Hierarchy inventory (M34 / M35)

The Hierarchy lists the canonical scene objects (20 rows for Level 01: Spawn, Camera, Ground, 6 Platforms, 2 Slopes, Moving Platform, 2 Checkpoints, 2 Hazards, 3 Collectibles, Goal). Visible 3D things that are **not** rows:

| Visible thing | Classification | Why absent |
|---|---|---|
| Runtime Player mesh | runtime-only | Gameplay CharacterVirtual; only **Player Spawn** is authored |
| XZ `DrawGrid` | technical/demo/render | Renderer debug ground grid, not LevelDefinition |
| Editor spawn marker / highlight / pending ghost / gizmo | editor visualization | Overlay while F2 is active; not extra authored objects |
| TIME / BEST / COLLECTED HUD | other | 2D overlay, not scene objects |

Milestone 44 removes the four cooker-probe **scene instances** (checker quad, `test_static.glb`, `test_authored.glb`, `test_textured.glb`). Those assets remain in cook/stage inventory and automated tests. They were never Hierarchy rows and must not be added to M41 lifecycle. Milestone 45 turns `dynamic_box` into a repeatable authored Dynamic Box (`std::vector<DynamicBoxSpec>`). Canonical Level 01 has zero Dynamic Boxes so the old cyan probe does not return as scene clutter.

## Visual level editor v4 (Milestone 35, Phase B)

Phase B wires the approved Phase A math into the live Debug/Development editor. `EditorTransformMode` is shown as Translate/Resize radios (disabled during an active drag). Application still never serializes mode, widget, nudge, or editor camera.

**Resize.** Ground and Elevated Platform 0..5 only. Handles are cubes at `origin ± axis * gizmoLength` (camera-scaled triad, not box faces). Drag uses the M34 axis-parameter solver. `newSize.axis = start + 2 * handleSign * axisDelta`, then clamp to `kMinAuthoredBoxExtent` (0.12). Center is unchanged. The pending ghost is the true size feedback. Spawn in Resize mode shows no handles and the hint "Selected object is not resizable". Same depth-independent overlay as M34 translation. Hover/active identify axis **and** sign.

**Orientation widget.** Screen-space after EndMode3D, before ImGui. Adaptive upper-right placement: `originX = viewportWidth - (defaultInspectorColumn) - radius - 24`, `originY = 64 + radius` (center ~100 px from the top on a 1280 view), left of the default Inspector column, not bound to the live ImGui pose and not persisted. Canonical views change yaw/pitch only: Front 0/0, Back 180/0, Right −90/0, Left 90/0, Top keeps yaw and sets pitch −89. Bottom is omitted. Priority: ImGui > widget > gizmo > world pick. Does not dirty Modified/Dirty/BEST or authored camera.

**Nudge.** Ctrl+Left/Right = ∓/+X, Ctrl+PageDown/PageUp = ∓/+Y, Ctrl+Down/Up = ∓/+Z. Ctrl+Shift = 0.01 precision, else 0.10. **Translate mode only.** ImGui keyboard capture and active gizmo drag suppress it.

**Dolly.** `ResolveEditorWheel`: Alt+wheel is dolly (`clamp(wheel * speed * 0.35, ±8)` along look-forward) and does not change `movementSpeed`. Ordinary wheel still changes speed. ImGui mouse capture and active transform drag suppress both.

Status: Milestone 35 is complete and merged.

## Editor menu bar and workspace (Milestone 36, Phase B)

Phase B wires the approved Phase A workspace into the live Debug/Development editor. F2 shows `ImGui::BeginMainMenuBar()` after `rlImGuiBegin`, before Metrics/Hierarchy/Inspector/Level Editor. M36 originally overlaid the bar on the raylib framebuffer. Milestone 43 owns shared content-viewport chrome: while F2 is on, 3D rendering, rays, and picking use the region below the menu bar (and below the Quick Toolbar when it is visible).

**View.** Checkable items bind `workspace.showMetrics`, `showHierarchy`, `showInspector`, `showLevelEditor`. Development also binds `showObjectPalette` and `showQuickToolbar`. F1, View > Metrics, and the Metrics close button share `showMetrics`. Hierarchy/Inspector/Level Editor/Object Palette follow F2; Metrics does not. Closing a panel sets only its visibility bool (selection, workingCopy, transformMode, and pending edits remain). Hidden panels reopen from View. F2 off/on preserves session visibility and transform mode. Closing Object Palette does not cancel placement mode.

**Transform / Level.** Translate and Resize call `TrySetEditorTransformMode` on the same `transformMode` as the Level Editor radios (disabled while `gizmo.dragging`). Apply Preview / Revert Working Copy / Save Level Source emit the existing `LevelEditorRequest` values; Application still executes them after the UI frame. Enable policy is `CanApplyPreview` / `CanRevertWorkingCopy` / `CanSaveLevelSource`. `CanApplyPreview` uses in-memory working-copy validity (`IsWritableLevelDefinition`), not source authoring — Debug can Apply and cannot Save. Reset Editor Layout is `ResetEditorWorkspaceLayout` (menu and panel share it): default poses plus default visibility (including Object Palette and Quick Toolbar). It does not Apply/Revert/Save, does not reset the last build configuration, and does not touch authored/camera/selection/mode flags.

**Orientation widget.** Live `extraTopInset` is `OrientationWidgetLiveExtraTopInset(active, menuBarHeight)` (`GetFrameHeight()` stored from the bar, or 24 px until the first F2 frame, plus 8 px gap). Upper-right placement is otherwise unchanged. Widget math, hit radius, and canonical views are unchanged.

Cooker/build integration, Add/Delete/Duplicate, docking, and Milestone 37 were out of scope for M36.

Status: Milestone 36 is complete and merged.

## Editor tool runner (Milestone 37)

The Development editor launches the project's existing cooker and CMake presets as external child processes. Debug keeps the M36 editor and does not show Build or Tool Output. Release has no editor.

**Build menu** (F2, Development only): Cook Assets; Stage Runtime Assets; Cook & Stage; Build Debug / Development / Release / Build All; Tool Output. Start items call `EditorToolRunner::TryStart` with `RepositoryRoot()` and `IsEditorToolExecutionAvailable()`. Starting a job sets `workspace.showToolOutput = true`. While Running, start items are disabled; View/Transform/Level stay usable.

**Commands.** `Build > Cook Assets` is only `python tools/cook_assets.py` (cwd = repository root). It does not stage runtime assets, build any configuration, or restart the game. `Build Development` is still `cmake --build --preset windows-development`. `Build > Stage Runtime Assets` is `cmake -P cmake/StageRuntimeAssets.cmake` into the Development runtime `assets/` directory and is not a C++ build.

**Tool Output.** `DrawEditorToolOutput` reads the runner snapshot (label, Idle/Running/Succeeded/Failed, elapsed, exit code, generic sequence step, bounded log). Multi-step jobs display `{displayLabel}: {index}/{count} {step}` (Build All and Cook & Stage). `p_open` is `showToolOutput`. View > Tool Output and Build > Tool Output share that bool. Clear is disabled while Running. Auto-scroll sticks to the bottom when the user is already near the bottom.

**Frame loop.** `Application` owns the runner. `Poll()` runs every debug-UI frame, even if Tool Output is hidden. `Shutdown()` terminates a running child before ImGui teardown.

**Policy.** No auto configure, Save, Apply, cook-before-build, cook-before-stage, or restart. Self-build Policy A: Build Development stays enabled; LNK1168 while this exe is running is expected Failed when a relink is actually required. Incremental no-op builds can succeed with the exe locked because they do not relink.

**Layout.** Default Tool Output is a bottom strip (~180 px on 1280×720). Object Palette defaults to the inspector column below Level Editor. Reset Editor Layout shows the known panels and snaps those windows, including Object Palette and Tool Output.

Status: Milestone 37 is complete and merged.

## Runtime asset staging (Milestone 38)

Phase A introduced one staging implementation used by CMake POST_BUILD and by Development editor jobs. Phase B wires the live Development Build menu. Debug still has no Build menu. Release has no editor.

**Stage Runtime Assets.** Concise menu label for staging **Development** runtime assets only (`build/windows-vs2022/bin/Development/assets`). Command: `cmake -P cmake/StageRuntimeAssets.cmake` with cooked root `game/assets/cooked`. Does not cook, `--build`, MSBuild, or restart. Allowed while Development is running (asset files are not the exe lock). Updates disk only; no hot reload. Requires a configured CMake tree and a cooked directory. Exit 0 is success.

**Cook & Stage.** Sequence owned by `EditorToolRunner`: Cook Assets, then Stage Runtime Assets. Cook failure skips Stage. Stage failure is Failed at Stage Runtime Assets. Markers: `=== Cook & Stage: Step N/2 - ... ===`. One active job at a time.

**Inventory / stale files / no auto Save.** Unchanged from Phase A. Staging never deletes extra destination files. Neither job Apply/Revert/Saves or restarts the game.

Status: Milestone 38 is complete and merged.

## Development runtime level reload (Milestone 39, Phase B)

Phase A added in-process reload of Level Format v1 from the **staged** runtime file. Phase B wires the live Development `Level > Reload Runtime Level` item. It is not general hot reload and not an `EditorToolRunner` job.

**Authority.** Same path as startup: `platform::RuntimeAssetPath(world::kLevel01RuntimeLogicalId)` → `<exe>/assets/levels/level_01.level`. Development dest is `build/windows-vs2022/bin/Development/assets/levels/level_01.level`. Reload never reads `game/assets/source` or `game/assets/cooked`.

**Live routing.** ImGui emits only `LevelEditorRequest::ReloadRuntimeLevel`. `Application::HandleLevelEditorRequest` calls `ReloadRuntimeLevelFromStaged` → `PrepareRuntimeLevelReload` → `PhysicsWorld::TryRebuild` → commit/reconcile/`ResetGameplayAfterCommittedLevel`. Drawing code does not call Prepare/TryRebuild.

**Transaction.** `PrepareRuntimeLevelReload` parse/validates a candidate. `PhysicsWorld::TryRebuild` builds a replacement world and swaps only on success. Failure leaves the active world, `workingCopy`, and BEST intact. Apply Preview uses the same rebuild primitive.

**Enable.** `CanReloadRuntimeLevel(authoringAvailable, modified, toolRunnerRunning)`: Development-only, rejected while `Modified`, rejected while `EditorToolRunner::IsRunning()` (Policy A: do not read staged files mid Cook & Stage / Stage / Build). Dirty-but-not-Modified does not block. `savedSourceBaseline` is not updated. After success: `workingCopy = active`, Modified false, Dirty iff staged active differs from saved source.

**Session.** Same reset as Apply Preview (`ResetGameplayAfterCommittedLevel`): player at spawn, checkpoints/collectibles/goal/timer/platform/box reset. Persistent BEST is not touched. Editor navigation camera and transform mode stay. Selection and gizmo clear. Tool Output is unrelated (not opened, cleared, or cancelled).

**Status.** One current Level-action message in the Level Editor panel (`lastApplyStatus` / `lastSaveStatus` / `lastReloadStatus` plus `lastMessage`). A newer Apply/Save/Revert/Reload clears the other status enums so stale lines are not shown together.

**Menus.** Development Level: Apply Preview, Revert Working Copy, Save Level Source, separator, Reload Runtime Level, separator, Reset Editor Layout. Debug omits Reload (and Build). Release has no editor.

Status: Milestone 39 is complete and merged.

## Cook, Stage & Reload workflow (Milestone 40, Phase B)

Phase B wires the live Development `Build > Cook, Stage & Reload` item. It is cross-boundary orchestration, not a third Cook/Stage implementation and not an `EditorToolKind` for Reload.

**Live menu.** After `Cook & Stage`, before the C++ Build separator. The item emits only `LevelEditorRequest::CookStageAndReload`. Enable: `CanStartCookStageReload(authoring, modified, runner.IsRunning(), workflow.IsPending())`. Other Build jobs are also disabled while the workflow is pending (ReloadPending window after CookAndStage Succeeded). Debug has no Build menu. Release has no editor.

**Owner.** `Application` owns `CookStageReloadWorkflow` beside the existing `EditorToolRunner`. `DrawEditorMenuBar` does not poll jobs or call reload.

**Bridge.** One `EditorToolRunner::Poll` per frame in `Application::Run` (independent of F2), then `Observe`. On structured `Succeeded` + `CookAndStage`, the workflow arms `ReloadPending`. After the frame is presented, `TakeReloadRequest` fires **exactly one** `ReloadRuntimeLevelFromStaged`. No busy-wait. Shutdown: `Cancel()` then `EditorToolRunner::Shutdown()`.

**Enable.** Dirty does not block. No automatic Apply or Save. Manual `Level > Reload Runtime Level` stays disabled while pending.

**Status.** Tool Output keeps Cook & Stage 1/2–2/2. Level `lastMessage`: running / failed before Reload / Cook & Stage succeeded but Reload failed + M39 reason / completed. Starting the workflow clears stale Apply/Save/Reload status enums.

Status: complete and merged. Milestone 41 Phase B is in progress.

## Authored object lifecycle (Milestone 41, Phase B)

Phase A generalizes repeatable Level Format v1 categories to variable-length `std::vector` storage and adds **pure** working-copy Add / Duplicate / Delete.

Phase B exposes that helper in the live **Development** editor. There is **no Phase C yet**. Milestone 41 is **not** complete.

**Repeatable (lifecycle):** Platform, Checkpoint, Hazard, Collectible, Dynamic Box. Stored as `std::vector` in `LevelDefinition`. There is no small design cap of 16/8/8/16. Checkpoint / Hazard / Collectible share the v1 64 KiB / 256-line parser guards. Platform count plus Dynamic Box count is limited by leftover Jolt bodies (`kMaxAuthoredPhysicsBodies` = 59). Writer emits one record per element in container order. Still Level Format v1: no count header, no v2.

**Singletons unchanged:** Spawn, Ground, Camera, Goal, slopes (`std::array` of 2), moving platform. The three `support_index_*` fields remain authored singletons; they are 0-based indices into `elevatedPlatforms`, not independently addable objects.

**Lifecycle helper:** `editor::AuthoredObjectLifecycle` mutates only the supplied `LevelDefinition` (the editor's `workingCopy`). Add/Duplicate are append-only, so existing platform-index references do not shift. Duplicate copies values and adds world +X `1.0`. Delete validates first, then mutates atomically. Platform delete remaps every authored `support_index_*` with `R > D` to `R - 1` (same semantic platform after compaction), leaves `R < D` unchanged, and **rejects** the whole delete when any `R == D` (`ReferencedPlatform`) — no silent retarget to Platform 0 and no nearest-platform guess. Deleting the last platform is rejected (`MinimumCount`; a valid/saveable v1 level needs at least one platform because the three support indices must stay in range). Success clears selection. Failures leave `workingCopy` and the input selection unchanged. `LifecycleEditStatus` distinguishes Success / InvalidSelection / UnsupportedType / AtLimit / ReferencedPlatform / MinimumCount so Phase B can show a reason; `CanDeleteSelected` is a secondary enable guard and must not be the only integrity check. No Apply, Save, or physics.

**Live Edit menu (Development only).** `PLATFORMER_ENABLE_LEVEL_AUTHORING`. Menu-only; no Ctrl+D / Delete key.

```
View / Transform / Edit / Level / Build
Edit
├ Add
│  ├ Platform
│  ├ Checkpoint
│  ├ Hazard
│  ├ Collectible
│  └ Dynamic Box
├ Duplicate Selected
└ Delete Selected
```

ImGui emits `LevelEditorRequest` only. `HandleAuthoredLifecycleRequest` (Application after present) calls the Phase A helper, updates selection, marks per-category `structuralPending`, clears gizmo transients, and writes `lastMessage`. Debug compiles the visual editor without this menu. Release has no editor.

**Platform-index references.** The only authored fields whose meaning is “index into `LevelDefinition.elevatedPlatforms`” are `checkpoint1PlatformIndex`, `checkpoint2PlatformIndex`, and `goalPlatformIndex` (`support_index_cp1` / `support_index_cp2` / `support_index_goal`). They are validation metadata, not gameplay runtime. Renderer `platformIndex` is a draw-loop color counter. `EditorSelection.index` is editor identity, not a LevelDefinition field. There is no slope / moving-platform / cached gameplay platform index. Remapping those three ints does not change checkpoint/goal object identity, so only the Platform category needs `CategoryStructuralPending` after a successful platform delete. Apply / Save / Cook / Stage / Reload do not repair indices; the lifecycle mutation must already leave a semantically valid `LevelDefinition`. Still Level Format v1: writer emits the remapped ints; parser range-checks the final values.

**Selection.** Type + index stays. After Add/Duplicate the new index is selected. After Delete, selection is cleared and gizmo transients are cleared (`ClearGizmoInteraction`). Apply keeps the selection if it is still in range. Revert assigns `workingCopy = active` then reconciles. Reload still clears selection (M39).

**Pending structural picks.** A session-local `StructuralIndexMap` tracks active index → workingCopy index for Platform / Checkpoint / Hazard / Collectible / Dynamic Box. It is not a GUID and is not serialized. Add/Duplicate append: existing active objects stay mapped; the new working-only object has no active counterpart until Apply. Delete marks that active object as pending-deleted (`kNoStructuralIndex`) and shifts later working indices. Viewport picking priority is: (1) nearest visible pending workingCopy ghost from `CollectPendingAuthoringVisuals` / `PendingPickProxy` using the working index directly, (2) active-world proxy mapped through `StructuralIndexMap`, (3) empty click clears. Pending Add/Duplicate/Modify Platform, Checkpoint (trigger AABB), Hazard, and Collectible (authored collection bounds) are viewport-pickable. Picks of pending-deleted objects are ignored (no resurrection). Empty clicks still clear. The map resets to identity on Apply, Revert, Reload, and editor open. `CategoryStructuralPending` remains a per-category dirty flag; it no longer blocks the whole category. Authored `support_index_*` remapping is separate from this session map. No GUID framework.

**Hierarchy** is built from `workingCopy`, so pending adds appear and pending deletes disappear before Apply. Labels stay `Platform N` / `Checkpoint N` / … — not editable names. Hierarchy is not filtered by `CollectibleRunState`. The active world and picking set stay on the applied definition. Editor Collectible proxies come from `active` `LevelDefinition` even when the runtime cube is collected/hidden. Newly added platforms, checkpoints, hazards, and collectibles use cyan **wireframe** pending bounds plus, in Development, pending **object** ghosts from `workingCopy`. `CollectPendingAuthoringVisuals` enumerates **all** pending Add/Duplicate/Modify geometry in those four categories, not only the current selection: selected pending uses stronger cyan; unselected pending uses the same geometry at lower intensity so the complete authoring delta stays visible until Apply, Revert, or successful Reload. Checkpoint / Hazard / Collectible also get a Development-only pending object ghost (checkpoint post/beacon, full hazard bar+teeth, collectible visual cube), drawn before the wires so the authored volume does not hide the object. Platform does not get a second cube: its visual is the authored box, so one cyan wire AABB is enough. Pending object ghosts are editor rendering only: not physics, not gameplay. Visible pending Add/Duplicate/Modify ghosts are viewport-pickable via `PendingPickProxy` volumes derived from the same preview records; selection uses the workingCopy index directly. Viewport picking stays ImGui-capture-gated and below gizmo drag. Pending deletes stay visible in the active world until Apply. Identity is the session map (active index with no working counterpart), not geometry matching. Development draws the same authored geometry faded/desaturated with reduced alpha (world depth test on; not a through-wall overlay) plus a subtle dusty-red delete outline. Platform uses the same box; Checkpoint uses a faded editor post/beacon plus trigger outline (gameplay marker is skipped for that active index so it does not stay fully lit); Hazard uses the full bar+teeth; Collectible uses the full visual cube. Pending-delete style wins over cyan pending Add/Modify and over collected authored gold wire. Pending-deleted objects are not pickable into workingCopy. No tombstones. Object Palette is Milestone 42.

**Editor visual precedence** for one active object: (1) pending delete (faded + delete outline), (2) pending workingCopy transform/add cyan ghost (selected stronger, unselected softer; persists after deselection), (3) collected authored-only gold wire, (4) normal active runtime visual. Cyan Add/Duplicate/Translate ghosts do not use the delete fade. Non-deleted objects keep their current appearance. No runtime mutation before Apply.

**Authored capacity.** Design capacity is not the same as a parser safety guard. Checkpoint, Hazard, and Collectible have no small gameplay-facing cap; they share the v1 64 KiB / 256-line defensive file bounds. Elevated Platform count and Dynamic Box count share leftover Jolt bodies (`kPhysicsMaxBodies` = 64, five fixed bodies, `kMaxAuthoredPhysicsBodies` = 59). `platformCount + dynamicBoxCount <= 59`. Edit > Add / Duplicate disable when that combined budget is exhausted. There is no separate quota such as 40 platforms / 10 boxes.

**Add placement.** `AuthoredObjectLifecycle::Add*` takes a camera-region `placementAnchor` and does not read EditorCamera or raylib. Development `HandleAuthoredLifecycleRequest` for Edit > Add receives `EditorAddPlacementAnchor(editorCamera)`: camera position + look-forward * `kEditorAddPlacementDistance` (10). `EditorCameraTarget` is only the 1-unit view look-at and is not used for placement. Authored X/Y come from that camera region. Authored Z is the current Level Format v1 gameplay-lane proxy: `workingCopy.initialSpawnVisualCenter.z` (`spawn.z`). Camera-derived Z is ignored. Spawn X/Y do not affect Add. Category offsets from the hybrid placement are currently `{0,0,0}`; Checkpoint `respawnPosition` stays `center + kDefaultAddedCheckpointRespawnOffset` (`{0,0,0}`). Duplicate remains original +1 world X, preserves the source object's Z, and does not snap to the lane. This is not "place near Spawn." Object Palette confirm uses `Add*At` / `worldCenterPlacement` at the resolved candidate center and does not snap to spawn.z.

**Inspector.** Lookup-by-index each frame. Platform keeps Translate/Resize numerics. Checkpoint edits Trigger Center, Trigger Size, and Respawn Position as independent authored fields. Hazard and Collectible edit `center` and `size`. Dynamic Box edits Center, Size, and Mass (kg) on `workingCopy` only. A collected authored Collectible remains a valid selection; Inspector still shows `workingCopy` center/size. No runtime uncollect.

**Gizmo.** Translate: Spawn, Ground, Platform, Checkpoint, Hazard, Collectible, Dynamic Box. Resize: Ground, Platform, and Dynamic Box. Checkpoint Translate is an assembly move: `SetCheckpointAssemblyCenter` applies one delta to `center` and `respawnPosition` so `respawnPosition - center` is preserved. Inspector field edits do not use that helper. No respawn gizmo. Dynamic Box gizmo edits `workingCopy` only; the live Jolt body keeps simulating until Apply.

**Checkpoint editor overlay.** A selected Checkpoint draws the existing trigger AABB (highlight/pending ghost from trigger `center`/`size`) plus an editor-only magenta wire marker at `workingCopy.respawnPosition`. A thin connector is drawn when the two points differ. When the checkpoint assembly differs from `active`, Development also draws a translucent pending post/beacon from `workingCopy` using `MakeCheckpointMarkerLayout` (same shape as the runtime marker). The active runtime marker stays at `active` until Apply. Both overlay values come from `workingCopy`, never mixed with `active`. Hierarchy/picking still treat Checkpoint as one object. Add Checkpoint defaults `respawnPosition = center` (canonical Level 01 identity offset). Duplicate already offsets both by +1 X.

**Runtime.** `PhysicsWorld::TryRebuild` remains the only Apply/reload rebuild. Checkpoint progression uses container order. Collectible flags resize on load / Apply / Reload / RestartRun. HUD `COLLECTED N / count` uses the active level size. F2 editor visualization does not reset collected flags, collected count, timer, checkpoint progression, or BEST.

**Gating.** Enable helpers require `PLATFORMER_ENABLE_LEVEL_AUTHORING` (Development). Dirty and Modified do not disable lifecycle. No automatic Apply/Save. Pending object ghosts and pending-delete faded visuals are Development-only (`PLATFORMER_ENABLE_LEVEL_AUTHORING`); Debug keeps the cyan bounds overlay without those extras. Release has no editor overlay.

**Delete key.** Development Delete (not Backspace, not Ctrl+D) emits the same `DeleteSelected` request as Edit > Delete Selected, after ImGui `WantCaptureKeyboard` / `WantTextInput`. Disabled cases do nothing: invalid/unsupported selection, gizmo drag, referenced Platform, last remaining Platform. Platforms are deletable when count > 1 and the selected Platform is not referenced by `support_index_cp1` / `support_index_cp2` / `support_index_goal`.

Status: Milestone 41 is complete and merged. Milestone 42 is complete and merged.

## Object Palette and placement (Milestone 42)

Development-only authoring UX on top of M41. No Level Format change, no second lifecycle authority, no tool framework, no physics raycast.

**Object Palette.** View menu + `workspace.showObjectPalette` (default true). Window close and View stay synchronized. Layout persists in `%LOCALAPPDATA%\Platformer3D\editor_layout.ini`. Reset Editor Layout includes the palette. Debug does not show the window. Palette entries are tool toggles on `PlacementMode` (the single authority): click Collectible to enter, click Collectible again to exit, click Platform while Collectible is active to switch. Closing the palette does not cancel mode. Edit > Add remains the M41 camera-region + spawn.z command and does not enter placement mode. Palette clicks never Add.

**Placement mode.** Transient enum `None | Platform | Checkpoint | Hazard | Collectible` on `LevelEditorState`. Mode itself does not mutate `workingCopy`. Esc (when ImGui does not want keyboard) exits. The active category is highlighted; the palette shows `Placement active: <Category>` and `Esc or click again to stop`. A small top-center HUD repeats `Placing: <Category>` / `LMB place | Esc cancel` (plus `No surface hit` in fallback). Selection does not cancel mode. F2 close/open, successful Apply, Revert, and successful M39 Reload set mode to None. While mode is active, raylib Esc-to-quit is disabled so Esc cancels placement without closing the game.

**Candidate.** `EditorPlacement` resolves a preview from the editor mouse ray against active-world Ground / ElevatedPlatform / Slope AABBs (oriented AABB for slopes). Hit: authored center sits on the contact (`+ size.y * 0.5`), `PlacementCandidateSource::SurfaceHit`, bright cyan preview. Miss: camera-forward fallback (`EditorAddPlacementAnchor`, distance 10) as the center, `CameraFallback`, quieter wire-only preview. Not Spawn. Category defaults reuse M41 `kDefaultAdded*`. The preview is editor-only: not in workingCopy/active, no physics, no Hierarchy, not pickable. Pending Delete stays faded. Source is not Level Format and is not persisted.

**Confirm.** Primary LMB when mode is active, pointer is in the viewport, and the click was not claimed by ImGui, RMB look, the orientation widget, or the gizmo. A claimed LMB press (gizmo hover/press/drag/release, widget, ImGui, look) sets `placementPointerBlocked` until release, so gizmo manipulation of a just-placed object cannot confirm a second Add. Application calls `HandleAuthoredLifecycleRequest` with `worldCenterPlacement=true` so Add reuses M41 append/capacity/StructuralIndexMap/selection. One click appends one object. Mode stays active for repeated placement. The new object is selected and uses normal M41 selected-pending visuals. After the gizmo gesture ends, the next clean viewport click still places. No auto Apply/Save/physics. Capacity rejection uses the existing M41 reasons. Viewport pick is skipped while placing; Esc then pick uses the M41 pending-pick path.

**Cleanup.** Successful Apply, Revert, and successful Reload clear the candidate and set mode to None. Palette-created pending objects follow M41: Revert drops them with `workingCopy = active`.

Status: Milestone 43 is complete and merged. Milestone 44 is Legacy Prototype Scene Cleanup.

## Editor Quick Toolbar (Milestone 43)

Development-only compact Quick Toolbar fixed immediately below the F2 menu bar. Alternate UI for existing commands. No second command authority, no icon font, no docking framework, no Level Format change.

**Chrome.** `EditorContentViewport` / `LiveEditorChromeHeight` is the shared geometry authority. While F2 is on, menu bar, optional toolbar, and 3D content occupy separate vertical regions. `Renderer::DrawWorld` uses a sub-viewport so the world is not drawn under the chrome. `ScreenToWorldRayFromWindow` maps window mouse into that rectangle. Hidden toolbar returns to menu-only bounds. F2 off is the full window.

**View.** `workspace.showQuickToolbar` (default true). `View > Quick Toolbar` shares that bool. Reset Editor Layout restores visibility on. Not a floating ImGui layout window (`NoSavedSettings`). Debug has no authoring toolbar. Release has no editor.

**Transform.** Translate/Resize call `TrySetEditorTransformMode` on the same `transformMode` as the Transform menu and Level Editor radios. Active button uses `ImGuiCol_ButtonActive`. Resize availability remains `IsResizeSelection` (mode can still be selected; handles follow canonical rules).

**Level actions.** Apply / Revert / Save emit `LevelEditorRequest::ApplyPreview` / `RevertWorkingCopy` / `SaveLevelSource` through `QuickToolbar*Request()` helpers. Enable policy is the existing `CanApplyPreview` / `CanRevertWorkingCopy` / `CanSaveLevelSource`.

**Build selector.** `EditorBuildTarget` on `LevelEditorState` (Debug / Development / Release / All). First-run default Development. Persisted as `%LOCALAPPDATA%\Platformer3D\editor_build_selection.txt`, not Level Format and not `editor_layout.ini`. Invalid values fall back to Development. Reset Editor Layout does **not** reset the last selection. Run maps through `EditorToolKindForBuildTarget` into `RequestEditorToolStart` / `EditorToolRunner`. Combo stays editable while a job runs (affects the next Run only). Run is disabled while the runner is busy or Cook, Stage & Reload is pending. Tool Output behavior is unchanged.

**Input.** Toolbar is ImGui; Application uses this-frame `WantCaptureMouse` plus `EditorViewportPointerBlocked` so chrome clicks cannot pick, place, drag a gizmo, or move the camera.

**Orientation / M42.** Widget `extraTopInset` includes toolbar height when visible. Placement rays use the same content viewport. Object Palette is not merged into the toolbar.

Status: Milestone 43 is complete and merged.

## Legacy prototype scene cleanup (Milestone 44)

M44 separates cooker/physics **test coverage** from **canonical scene visibility**. It is cleanup, not a level redesign or Level Format v2.

**Removed scene instances (assets kept):** `textures/test_checker.png` quad, `models/test_static.glb` (the spawn-area orange pyramid), `models/test_authored.glb` (plain white/grey cube), `models/test_textured.glb` (larger textured cube). Renderer no longer loads or draws them. `cmake/RuntimeAssets.cmake`, cooker, staging, and Python PNG/GLB tests still require the files.

**Kept in the scene:** slope 0 (30° walkable, `kLevel01WalkableSlopeIndex`), slope 1 (60° steep, `kLevel01SteepSlopeIndex`), kinematic moving platform. M42 placement surfaces remain Ground / Platform / Slope.

**Authored `dynamic_box` (M45/M46):** repeatable `std::vector<world::DynamicBoxSpec>` with `center`, `size`, `massKg`. Zero, one, or many records. Each applied box creates one Jolt dynamic `BoxShape` with authored mass, gravity, world collision, and CharacterVirtual push. Runtime pose authority is Jolt; rendering and active picking follow the live body. Save / Reload / Apply use authored definitions. Full `RestartRun` restores every Dynamic Box to its authored pose and zeros velocities. Checkpoint respawn does not reset Dynamic Boxes. If an active runtime body center Y is strictly below the currently applied `killPlaneY`, `PhysicsWorld` restores **only that body** to its applied authored center, identity orientation (`Quat::sIdentity()`, matching body creation), and zero linear/angular velocity. Recovery does not rebuild PhysicsWorld, does not reset other boxes, and does not mutate `workingCopy`, active authored definitions, or `savedSourceBaseline`. Canonical Level 01 has 0 Dynamic Boxes (intentional M45 migration of the leftover probe line). Not the old hard-coded cyan crate.

**Body budget.** `kPhysicsMaxBodies` remains 64. Fixed bodies are 5 (ground, 2 slopes, kinematic moving platform, CharacterVirtual inner body). Authored leftover is **59**, shared by elevated Platforms and Dynamic Boxes: `fixed + platforms + dynamicBoxes <= 64`. Canonical Level 01 static bodies stay 9 (ground + 6 platforms + 2 slopes) with 0 Dynamic Boxes.

Status: Milestone 44 is CLOSED and merged. Milestone 45 is CLOSED and merged. Milestone 46 is CLOSED and merged. Milestone 47 is implemented, awaiting manual acceptance. Milestone 48 has not started.
