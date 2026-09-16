# Milestone 20 — Checkpoint + Fall/Respawn Loop [COMPLETE]

## Goal

Return focus to core platformer gameplay after the M15–M19 asset-pipeline milestones.

Milestone 20 adds the first complete failure/recovery loop:

1. Player starts at the initial spawn.
2. Player can activate exactly one checkpoint.
3. Falling below a kill plane triggers a respawn.
4. Before checkpoint activation, respawn returns to the initial spawn.
5. After checkpoint activation, respawn returns to the checkpoint.
6. Manual respawn is available through semantic input.
7. CharacterVirtual, movement state, moving-platform carry, and camera state are reset coherently.

No lives, savegame, death screen, scene reload, or generic checkpoint framework.

## Architectural Intent

Respawn must not be implemented as a raw visual position assignment.

The authoritative physical player representation is Jolt `CharacterVirtual`, while `Player` owns gameplay movement policy/state. A respawn therefore needs to reset the relevant state across the existing boundaries without leaking Jolt types.

Keep the existing ownership rules:

- `Player` owns gameplay movement policy and timers.
- `PhysicsWorld` owns Jolt and CharacterVirtual physical state.
- gameplay/world code owns checkpoint and respawn meaning.
- Renderer only visualizes state.
- no Jolt types outside `PhysicsWorld.cpp`.
- no raylib input constants in gameplay.
- no general-purpose checkpoint/trigger framework.

## World / Respawn Data

Add the smallest project-owned representation for:

- initial spawn;
- kill-plane Y;
- exactly one checkpoint trigger volume;
- checkpoint respawn position.

A focused file such as `game/source/world/RespawnWorld.h` is acceptable if it fits the repository conventions.

Conceptually:

```cpp
struct CheckpointSpec
{
    core::Vec3 center;
    core::Vec3 size;
    core::Vec3 respawnPosition;
};
```

Preserve the existing CharacterVirtual position convention established in M11. Verify it before choosing exact spawn/checkpoint coordinates.

Initial kill-plane target: `Y = -8.0`.

Place the checkpoint in an area requiring traversal, preferably near/on the existing elevated right platform. Inspect actual current geometry before selecting final coordinates.

Checkpoint activation may use a simple project-owned AABB/point-volume test. Do not add a Jolt sensor.

## Runtime Respawn State

Keep state minimal, conceptually:

```cpp
struct RespawnState
{
    bool checkpointActive;
    core::Vec3 respawnPosition;
    int deathCount;
};
```

Optional diagnostic: last respawn reason (`Fall` or `Manual`).

PhysicsWorld must not know what a checkpoint means. Renderer must not own checkpoint gameplay state.

## Manual Respawn

Add semantic input:

```cpp
bool respawnPressed;
```

Map `R -> respawnPressed` in the platform/input layer. Gameplay must not query `KEY_R` directly.

Before checkpoint activation, R returns to initial spawn. After activation, R returns to checkpoint. Manual respawn does not increment `deathCount`.

## CharacterVirtual Reset

Expose the smallest project-owned, Jolt-free PhysicsWorld API needed to safely reset the character, preferably a cohesive operation such as:

```cpp
void ResetCharacter(const core::Vec3& position, const core::Vec3& velocity);
```

On respawn, coherently reset:

1. CharacterVirtual position.
2. CharacterVirtual linear velocity to zero.
3. Player relative horizontal velocity to zero.
4. Player vertical velocity to zero.
5. Moving-platform horizontal carry to zero.
6. Coyote timer to zero.
7. Jump buffer timer to zero.
8. stale grounded/support state safely.
9. fixed gameplay Z.
10. project-facing Player position synchronized from CharacterVirtual.

The moving platform itself must not reset.

## Camera Snap

M08 smoothing must not interpolate across the teleport. Add the smallest camera operation needed to immediately synchronize desired and smoothed target state with the respawn position, e.g. `SnapToTarget(...)`.

After the snap, normal dead-zone and exponential smoothing resume.

## Checkpoint Visual

Use existing Renderer primitives only. Render a simple marker/beacon with clearly distinguishable inactive and active states.

No new asset, shader, particles, or animation system.

## Debug / Development Metrics

Extend the existing Dear ImGui metrics panel with read-only checkpoint/respawn data, such as:

- Checkpoint active
- Respawn position X/Y/Z
- Player Y
- Kill plane Y
- Death count
- Last respawn reason (optional)

No debug UI in Release.

## Existing Gameplay Constants

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

Preserve CharacterVirtual, moving platform, slopes, cyan Jolt box, camera behavior except respawn snap, and M15–M19 asset behavior.

## Manual Validation

| Test | Expected result |
|---|---|
| Fall before checkpoint | Respawn at initial spawn |
| Activate checkpoint | Marker changes to active |
| Fall after checkpoint | Respawn at checkpoint |
| R before checkpoint | Initial spawn |
| R after checkpoint | Checkpoint |
| Fall with horizontal velocity | Respawn stationary |
| Fall after jumping from moving platform | No residual platform carry |
| Jump input around death | No stale automatic jump |
| Respawn | Camera snaps immediately |
| Normal movement/jump after respawn | M07 behavior remains normal |
| Moving platform | Still works |
| Walkable/steep slopes | Still work |
| Cyan dynamic box | Still works |
| Development metrics | Correct state |
| Release | No debug UI |

## Explicitly Out of Scope

Do not implement multiple checkpoints, persistence/savegames, lives, health, damage, enemies, death animation/screen, fade transition, audio, particles, collectibles, score, full level restart, scene reload, Jolt checkpoint sensors, generic trigger/respawn framework, event bus, ECS, generic level manager/state machine, new art assets, new dependencies, asset-cooker changes, texture-pipeline changes, or Milestone 21.

## Completion Criteria

- [ ] Initial spawn explicitly represented.
- [ ] Kill plane is project-owned gameplay/world data.
- [ ] Exactly one checkpoint exists.
- [ ] Activation is independent of raylib/Jolt APIs.
- [ ] Checkpoint visual has inactive/active states.
- [ ] CharacterVirtual reset uses project-owned PhysicsWorld API.
- [ ] Physical and gameplay velocities reset.
- [ ] Moving-platform carry resets.
- [ ] Coyote and jump-buffer timers reset.
- [ ] Fixed gameplay Z remains correct.
- [ ] Camera snaps on respawn.
- [ ] R is semantic input.
- [ ] Fall increments death count.
- [ ] Manual respawn does not increment death count.
- [ ] Debug/Development metrics expose useful state.
- [ ] Release contains no debug UI.
- [ ] M01–M19 remain intact.
- [ ] No new dependency or generic framework.
- [ ] Milestone 21 not started.

## Recommended Phases

### Phase A — Architecture / State Integration

Inspect current Player, PhysicsWorld, Camera, Input, moving-platform and application update flow. Define ownership, reset APIs, semantic input, world data and exact update ordering. Stop for review before full feature implementation.

**Phase A:** `world/RespawnWorld.h` and `gameplay::RespawnState` exist. Semantic `respawnPressed` (`R`) is mapped. `ResetCharacter`, `ResetMovementState`, and `SnapToTarget` compile. Kill-plane, checkpoint activation, death count, checkpoint visuals, and the respawn update-order wiring were not active yet.

### Phase B — Gameplay Implementation
Implement checkpoint activation/visual, kill-plane and manual respawn, CharacterVirtual reset, transient-state cleanup, camera snap, death count and debug metrics.

**Phase B:** After `Player::Update`, Application evaluates Fall then Manual, performs at most one coherent CharacterVirtual/Player/camera reset, and skips checkpoint activation on a respawn frame. Checkpoint overlap uses visual-center vs `world::kCheckpoint`. Renderer draws a post+beacon from `checkpointActive` only. Debug metrics expose Respawn / Checkpoint. R and kill-plane are live.

### Phase C — Regression / Runtime Validation (current)
Validate all manual cases, all configurations, Release behavior, and M01–M19 regressions before Git closure.

**Phase C:** User-validated. Milestone 20 is complete. Checkpoint, kill-plane, manual/fall respawn, death count, camera snap, and marker visuals are live.

## Git Branch

```powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/20-checkpoint-respawn
git branch --show-current
```

Expected: `milestone/20-checkpoint-respawn`.

## Recommended Model

Use **Grok 4.6 High — Fast OFF** because this milestone touches interacting CharacterVirtual state, movement timers, moving-platform carry, input semantics, update ordering and camera state.

## Workflow

`main clean → define milestone → update docs/MILESTONES.md → create branch → Phase A → review → Phase B → review → Phase C/manual tests → approve → commit → push branch → merge main → push main → verify clean main`
