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

Player owns gameplay policy: semantic movement intent, horizontal acceleration/deceleration relative to supporting ground, vertical gameplay velocity, jump, coyote time, and jump buffer. PhysicsWorld owns Jolt runtime state, CharacterVirtual, static greybox bodies, the kinematic moving platform, and the dynamic test box.

Project-facing Player position is the **visual AABB center**. CharacterVirtual `GetPosition`/`SetPosition` is the **feet** (`visualCenter.y - visualSize.y * 0.5`). `PhysicsWorld::InitializePlayer` and `ResetCharacter` accept visual-center coordinates.

## Checkpoint / respawn (Milestone 20)

Checkpoint types and AABB helpers live in `world/RespawnWorld.h` (`CheckpointSpec`). Authored spawn, kill-plane Y, and checkpoint instances live in `world/Level01.cpp` inside the immutable `LevelDefinition`. Runtime state lives in `gameplay::RespawnState`, owned by `Application`. PhysicsWorld does not interpret checkpoints. Renderer does not own activation.

`input::InputState::respawnPressed` is edge-triggered (`R` mapped in the input backend only). Application owns the respawn decision after `Player::Update`: Fall if visual-center Y is below the active level's `killPlaneY`, else Hazard, else Manual if `respawnPressed`. Fall wins if both Fall and Hazard occur. At most one respawn per frame. Checkpoint activation runs only when no respawn happened that frame.

`PhysicsWorld::ResetCharacter` teleports CharacterVirtual (feet), zeros linear velocity and airborne platform carry (`carriedGroundVelocityX`), refreshes contacts, and enforces fixed gameplay Z. `Player::ResetMovementState` clears relative horizontal/vertical velocity, coyote, jump buffer, and carry-related Player fields. `PlatformerCamera::SnapToTarget` copies the player visual center into both desired and smoothed targets. On a respawn frame the camera is snapped and `camera.Update` is skipped.

Checkpoint overlap tests `Player::Position()` (visual center) against checkpoint AABBs. No Jolt sensor. See Milestone 24 for the live two-checkpoint progression and marker states.

## Level goal / completion (Milestone 21)

The `LevelGoalSpec` type lives in `world/LevelGoal.h`. The authored Level 01 goal instance lives in `world/Level01.cpp`. Runtime state lives in `gameplay::LevelCompletionState`, owned by `Application`. PhysicsWorld does not interpret goals. Renderer does not decide completion.

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

Status: complete (manually validated). Do not implement Milestone 24 in this section.

## Shared greybox geometry
`world::Box` in `GreyboxWorld.h` is the project-owned AABB type. Canonical Level 01 ground and elevated platforms live in `world/Level01.cpp` (`CreateLevel01Definition()`). Renderer and PhysicsWorld both derive from the active `LevelDefinition`. Ground and platform coordinates are not duplicated inside those systems.

The kinematic platform's immutable spec is `LevelDefinition::movingPlatform` (`MovingPlatformSpec`). PhysicsWorld owns runtime pose/direction/BodyID. Renderer draws authored size with the runtime pose.

Static test slopes are `LevelDefinition::slopes` (`SlopeSpec`: 30° walkable, 60° steep). CharacterVirtual max slope remains 50 degrees in PhysicsWorld, not level authoring. Walkable `OnGround` is valid gameplay support; `OnSteepGround` is not.

## Moving ground
The Player rides kinematic ground through CharacterVirtual: `UpdateGroundVelocity` then `GetGroundVelocity`, added to Player-relative horizontal speed. The Player is not parented to the platform and does not receive a manual position delta.

## Physics boundary
Jolt types stay inside `PhysicsWorld.cpp`. Public physics headers expose only project-owned types (`PlayerMoveCommand`, `PlayerPhysicsState`, `PlayerGroundSupport`, `DynamicTestBox`, `MovingPlatformState`). The project has one physics backend: Jolt v5.6.0. There is no abstract `IPhysicsEngine`.

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
Authored files live in `game/assets/source/`. The Python cooker writes runtime-ready copies to `game/assets/cooked/`. The game never loads from `source/`.

Cook from the repository root:

    python tools/cook_assets.py

The cooker uses the Python standard library plus cooker-only **Pillow 12.3.0** (`python -m pip install -r tools/requirements.txt`; not a CMake or game runtime dependency). It processes known authored assets by explicit `KNOWN_ASSETS` identity, identifies content with SHA-256, skips rewriting an identical cooked file, and writes `game/assets/cooked/manifest.json` with portable relative paths (no absolute paths, timestamps, or machine names). Assets are not auto-discovered: a PNG under `source/textures/` is a runtime texture only if it is listed. Blender authoring PNGs (for example `test_textured_basecolor.png`) stay out of the cooker.

Standalone runtime PNGs (`kind: runtime_png`) use recipe `runtime_png.max512.lanczos.v1`: maximum dimension **512 px**, aspect ratio preserved, no upscale, no crop, Pillow `LANCZOS` when downscaling. Sources already within the limit are copied byte-for-byte. GLBs remain opaque byte copies; images embedded in `models/test_textured.glb` are not resized. Changing the recipe (max dimension, filter, or encoding) recooks even if source bytes are unchanged. See `tools/README.md`.

CMake does not cook. After configure, a POST_BUILD step stages cooked files next to the executable:

    <executable directory>/assets/textures/test_checker.png
    <executable directory>/assets/models/test_static.glb
    <executable directory>/assets/models/test_authored.glb
    <executable directory>/assets/models/test_textured.glb

Runtime lookup uses `platform::RuntimeAssetPath`, which joins `<executable directory>/assets/` with the logical relative path. The executable directory is queried from the OS in the platform layer (`GetModuleFileNameW` on Windows, `/proc/self/exe` on Linux). The process current working directory is never used, and non-absolute results are rejected so raylib file loads cannot silently resolve against CWD. The renderer does not hard-code `game/assets/cooked`.

Logical identities:
- Milestone 15 test texture: `textures/test_checker.png`
- Milestone 16 test model: `models/test_static.glb`
- Milestone 17 authored model: `models/test_authored.glb`
- Milestone 18 textured model: `models/test_textured.glb`

The M18 Base Color PNG `game/assets/source/textures/test_textured_basecolor.png` is Blender authoring input only. It is not cooked or staged. The exported GLB must embed the image. See `docs/BLENDER_WORKFLOW.md`.

Editable Blender files live in `game/assets/source/blender/`. They are not cooked and are not runtime assets. See `docs/BLENDER_WORKFLOW.md`. The cooker copies exported GLBs unchanged. Blender is not a build or runtime dependency.

The checker appears on a dedicated visual quad at `(0, 1.5, 2.5)`. The Milestone 16 test GLB is drawn at `(2.5, 1.0, 2.5)`. The Blender-authored GLB is drawn at `(-2.5, 1.0, 2.5)`. The Milestone 18 textured GLB is drawn at `(4.0, 1.0, 2.5)`. None of these are greybox geometry and none have collision.

The checker, Milestone 16 pyramid, Blender-authored model, and textured model are visual-only technical tests: no GreyboxWorld entries, no Jolt bodies, no mesh collision.

If a required cooked file is missing at CMake configure time, configure fails and tells the developer to run the cooker. If a staged runtime file is missing at load time, the renderer logs the logical id and draws a simple visual fallback. Texture and models are each loaded once after the window/graphics context exists and unloaded before window shutdown.

## Extended traversal / two checkpoints (Milestone 24)

M24 is a longer hardcoded greybox plus **exactly two ordered checkpoints**. It is not a generic level, trigger, or checkpoint framework.

Live world data (canonical instances in `world/Level01.cpp`):

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

M25 adds the first explicit non-fall lethal volumes: **exactly two** compile-time `HazardSpec` AABBs authored in `world/Level01.cpp`. Identity is the array index. There is no health, no Jolt sensor, and no generic trigger type.

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

M26 adds the first non-lethal collectible loop: **exactly three** compile-time `CollectibleSpec` AABBs authored in `world/Level01.cpp`. Identity is the array index. Per-run flags live in `gameplay::CollectibleRunState` (`std::array<bool, world::kCollectibleCount>`); count is derived. Collection is optional and must not gate the goal. The array size remains compile-time coupled to Level 01's three collectibles.

- Collectible 1 (index 0): right-platform hop `{5.0, 2.5, 0}` size `{1.0, 1.2, 1.0}`
- Collectible 2 (index 1): left-landing hop `{-4.5, 4.0, 0}` size `{1.0, 1.2, 1.0}`
- Collectible 3 (index 2): middle-left-step hop `{-10.0, 3.75, 0}` size `{1.0, 1.2, 1.0}`

Standing on the support does not collect (AABB sits just above standing center). A normal hop does. No Jolt sensor. Ordinary R/Fall/Hazard preserve flags; only Enter `RestartRun` clears them. Collection runs in the no-respawn branch after checkpoint/goal and is skipped when `restartAvailableAtFrameStart && Enter`.

Renderer receives collected flags plus derived count, draws a gold 0.45 cube for available items only, and always draws `COLLECTED N / <level collectible count>` in the upper-right after `EndMode3D`. Debug/Development metrics show Available/Collected, Inside, and Collected this frame.

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

M30 introduces a project-owned immutable `world::LevelDefinition` for the current single playable level (`level_01`). Canonical authored values live in `world/Level01.cpp` (`CreateLevel01Definition()`). The factory populates literals directly; it does not copy legacy Level 01 globals.

Application constructs the definition once as a member (`levelDefinition = CreateLevel01Definition()`), validates it at Initialize, and passes it read-only to PhysicsWorld (`Initialize(level)`) and Renderer (`DrawWorld(..., level, ...)`). Gameplay meaning (checkpoint activation, hazard death, collectible pickup, goal completion, timer, BEST) stays in Application. Runtime pose for the moving platform and cyan box stays in PhysicsWorld. `collected[]`, `activeCheckpointIndex`, `LevelCompletionState`, timer, and BEST stay outside `LevelDefinition`.

Camera offset `{2, 3.5, 12}` and FOV 40 are level framing (`LevelCameraSpec`). Dead zone X/Y `1.5` / `0.75` and follow sharpness `8` remain `PlatformerCamera` controller policy. Player visual size `{0.8, 1.6, 0.8}` remains character/render configuration (`world::kPlayerVisualSize`), not level authoring. CharacterVirtual max slope remains 50° in PhysicsWorld.

Only BEST is persisted (`PLATFORMER_SAVE 1` / `best_seconds`). The save does not store `level_01`. No external level file, editor, LevelManager, or second playable level.

Phase B: live consumers migrated. World type headers keep reusable specs/helpers and no longer own Level 01 instance arrays. Debug/Development metrics include a read-only Level Data section sourced from the active definition.

Status: Phase B implementation complete, awaiting Phase C manual validation. Do not mark M30 complete. Do not implement Milestone 31.
