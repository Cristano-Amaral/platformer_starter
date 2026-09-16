# Milestone 21 — Level Goal + Completion Loop [COMPLETE]

## Goal

Build the first explicit level-completion condition for the platformer.

Milestone 20 established the failure/recovery loop. Milestone 21 adds the successful end of that loop:

```text
spawn → traversal → checkpoint/platforming → goal volume → levelCompleted = true → completion feedback
```

This milestone adds exactly one level goal, a project-owned completion state, a simple visual goal marker, and minimal player-facing completion feedback.

It does **not** add multiple levels, scene transitions, menus, save data, scoring, collectibles, or a generic game-state framework.

## Core Design

- `Application` owns the current run-level completion state.
- Project-owned world data defines the goal volume and marker placement.
- `PhysicsWorld` does not know what a goal means.
- `Player` does not own level completion.
- `Renderer` visualizes the goal and completion feedback but does not decide completion.
- Jolt is not required for goal detection.
- Do not generalize checkpoint + goal into a trigger framework.

## Level Goal World Data

Add a focused project-owned goal specification, e.g. `game/source/world/LevelGoal.h`.

Conceptually:

```cpp
struct LevelGoalSpec
{
    core::Vec3 center;
    core::Vec3 size;
};
```

The exact final coordinates must be chosen only after inspecting the current greybox, slopes, moving platform, and checkpoint layout. The goal must require meaningful traversal, must not overlap the M20 checkpoint trigger, and must be reachable with the existing movement constants.

## Goal Detection

Use a simple project-owned test against the player's current visual-center convention, conceptually `PointInsideGoal(player.Position())`.

Do not use Jolt sensors, collision callbacks, raylib collision helpers in gameplay, generic trigger volumes, or event buses.

## Completion State

Keep state minimal:

```cpp
struct LevelCompletionState
{
    bool completed = false;
};
```

Own it alongside other run-level state in `Application` unless inspection reveals an already-existing better project-owned location.

## Completion Semantics

On first entry into the goal:

```text
levelCompleted = true
```

Completion is one-way for the current application run. After completion, the player may continue moving; physics, moving platform, checkpoint and respawn remain functional; `R` remains manual respawn; falling still increments `deathCount`; respawning does not clear completion.

Do not introduce a pause/game-state machine in M21.

## Goal vs Checkpoint

Checkpoint and goal remain separate concepts. Do not merge them into a generic trigger abstraction.

The goal must not change `respawnPosition`, activate the checkpoint, reset `deathCount`, reset Player movement, reset CharacterVirtual, or snap the camera.

## Goal Visual

Render a clear marker using existing primitives only, for example two posts plus a top bar/beacon. It must have obvious not-completed and completed visual states.

No model assets, textures, shaders, particles, animation systems, or new dependencies.

## Player-Facing Completion Feedback

When completed, show minimal visible feedback such as:

```text
LEVEL COMPLETE
```

Keep raylib rendering details behind the existing render/UI boundary. Gameplay must not call raylib text functions directly.

No menu, buttons, transition, score screen, animation sequence, fade, or input lock.

The message may remain visible for the rest of the run.

## Input

Add no new input. Preserve movement, jump, and manual respawn (`R`). Do not overload `R` with level restart.

## Physics

No new Jolt body/sensor/callback/layer. `PhysicsWorld` remains unaware of level completion. CharacterVirtual behavior remains unchanged.

## Update Ordering

Inspect the actual M20 loop first. Intended relationship:

```text
Poll input
↓
UpdateMovingPlatform
↓
Player::Update
↓
Fall / Manual respawn decision
↓
if respawn:
    PerformRespawn
else:
    checkpoint activation
    goal completion test
↓
PhysicsWorld::Update
↓
camera update/snap behavior
↓
render
```

Rules:

1. A respawn frame must not complete the goal because of teleport.
2. Goal detection occurs only when no respawn happened that frame.
3. Checkpoint behavior remains unchanged.
4. Completion does not interrupt physics.
5. Completion does not teleport CharacterVirtual.
6. Completion does not change camera behavior.

Do not reorder the M13/M14 physics-sensitive pipeline unnecessarily.

## Respawn After Completion

Completion remains true after later manual/fall respawns:

```text
reach goal → completed = true → R/fall → normal M20 respawn → completed remains true
```

Do not implement level-reset semantics yet.

## Debug / Development Metrics

Add a small read-only Level Goal section:

```text
Level completed: true/false
Goal center: X/Y/Z
Goal size: X/Y/Z
Player inside goal: true/false
```

No Dear ImGui metrics in Release.

## Release Behavior

Release must include the gameplay goal marker and `LEVEL COMPLETE` feedback, while Dear ImGui diagnostics remain absent.

## Existing Systems to Preserve

Do not change:

```text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Preserve M08 camera, M11 CharacterVirtual, M13 moving-platform carry, M14 slopes, M15–M19 assets, M20 checkpoint/kill plane/respawn/death count/camera snap, and the cyan dynamic Jolt box.

## Manual Validation

| Test | Expected |
|---|---|
| Start fresh | Level not completed |
| Approach goal without entering | No completion |
| Enter goal | `levelCompleted = true` |
| Goal marker | Changes to completed visual |
| Completion feedback | `LEVEL COMPLETE` visible |
| Stay inside goal | No repeated completion behavior |
| Leave goal | Completion remains true |
| R after completion | Normal M20 respawn; completion remains true |
| Fall after completion | Normal M20 fall/deathCount; completion remains true |
| Checkpoint | Still activates and controls respawn |
| Moving platform | Still carries player correctly |
| Slopes | 30°/60° behavior unchanged |
| Camera | Normal follow and M20 respawn snap unchanged |
| Cyan box | Still behaves normally |
| Release | Goal/message present, no debug UI |

## Explicitly Out of Scope

Do not implement Milestone 22, multiple levels, level loading, scene transitions/reload, level restart, next-level button, menus, savegames, persistence, times/leaderboards, score/stars/medals, collectibles, enemies, health/lives, audio, particles, completion animation system, fades, Jolt goal sensor, generic trigger/event/game-state frameworks, ECS, generic level manager, new assets/dependencies, or asset-pipeline changes.

## Completion Criteria

- [ ] Exactly one project-owned level goal exists.
- [ ] Goal is reachable and does not overlap checkpoint trigger.
- [ ] Goal detection uses project-owned logic.
- [ ] No Jolt sensor/body is used for the goal.
- [ ] Run-level owner owns completion state.
- [ ] Player/PhysicsWorld/Renderer ownership boundaries remain correct.
- [ ] Goal completes only once per run.
- [ ] Completion survives later M20 respawns.
- [ ] Checkpoint/death count/moving platform/camera remain unchanged.
- [ ] Goal has inactive/completed visual states.
- [ ] `LEVEL COMPLETE` is visible to the player.
- [ ] Release includes completion feedback but no Dear ImGui metrics.
- [ ] Gameplay constants remain unchanged.
- [ ] M15–M19 asset behavior remains unchanged.
- [ ] No new dependency or generic framework is added.
- [ ] Milestone 22 is not started.

## Recommended Phases

### Phase A — Goal Architecture and Placement

Inspect M20 world/update/render architecture. Define exact goal location/dimensions, world-data representation, completion-state ownership, overlap helper, update insertion point, goal visual, minimal completion-message rendering path, and debug metrics. Small foundational APIs/data may be added, but do not wire the complete loop yet.

**Phase A:** `world/LevelGoal.h` (`kLevelGoal` center `{-4.5, 3.3, 0}`, size `{2.0, 1.6, 1.8}`) and `gameplay::LevelCompletionState` exist and are owned by `Application`. `PointInsideGoal` tests visual center. Renderer draws an incomplete two-post + bar marker on the left platform. Goal detection, `completed = true`, `LEVEL COMPLETE`, and Level Goal ImGui metrics were not wired yet.

### Phase B — Completion Gameplay

Implement goal detection, one-way completion state, visual states, `LEVEL COMPLETE`, metrics, and persistence of completion across M20 respawns.

**Phase B:** On non-respawn frames, Application sets `completed` once via `PointInsideGoal` after checkpoint evaluation. Renderer consumes `levelCompleted` for marker colors and draws `LEVEL COMPLETE` after `EndMode3D` in all configurations. Debug/Development metrics expose the Level Goal section. Completion is not cleared by `PerformRespawn`.

### Phase C — Regression and Manual Validation (current)

Validate goal completion, Release behavior, checkpoint/respawn, moving platform, slopes, camera, cyan box, and M15–M19 runtime assets.

**Phase C:** User-validated. Milestone 21 is complete. Goal detection, marker states, `LEVEL COMPLETE`, and completion persistence across M20 respawns are live.

Do not commit/push/merge until final approval.

## Git Branch

```powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/21-level-completion
git branch --show-current
```

Expected: `milestone/21-level-completion`.

## Recommended Model

Use **Grok 4.6 High — Fast OFF** for Phase A. If Phase A confirms a straightforward integration, Phase B may use Grok 4.6 High Fast.

## Workflow

```text
main clean → define M21 → update docs/MILESTONES.md → create branch → Phase A → review → Phase B → review → Phase C/manual tests → approve → commit → push → merge main → push main → verify clean main
```
