# Milestone 24 --- Extended Traversal + Multiple Checkpoints

## Goal

Expand the current greybox into a longer platforming route and evolve
the M20 single-checkpoint implementation into a small ordered system
with **exactly two checkpoints**.

Target run:

``` text
Initial Spawn
  -> Checkpoint 1
  -> Extended Traversal / Moving Platform
  -> Checkpoint 2
  -> Final Traversal
  -> Goal
  -> LEVEL COMPLETE
  -> Enter
  -> Fresh Run
```

M24 implements the previously deferred experiment: a more extended
traversal area and more than one checkpoint. It is deliberately **not**
a generic level/checkpoint framework.

## Core Requirements

-   Exactly two checkpoints total.
-   Checkpoints activate in order.
-   Checkpoint 2 cannot activate before Checkpoint 1.
-   The furthest checkpoint reached is the current respawn destination.
-   Backtracking through an earlier checkpoint never downgrades
    progress.
-   Manual R and Fall use the latest activated checkpoint.
-   M22 Enter restart clears all checkpoint progress.
-   Both checkpoint respawn volumes must be physically safe.
-   The moving platform must never occupy a checkpoint respawn volume.
-   M23 CharacterVirtual inner-body behavior and 30 kg cyan dynamic box
    are preserved.
-   No new gameplay movement tuning.
-   No assets or dependencies.

## World Expansion

Phase A must inspect the exact M23 geometry before choosing coordinates.
Expand the greybox horizontally enough to make the route meaningfully
longer while remaining readable with the existing M08 camera.

Prefer a small number of additional/extended static greybox platforms.
Reuse the existing moving platform and slopes where useful.

Do not blindly scale all existing coordinates and do not create a large
level.

## Player Reach

Design geometry around the existing movement:

``` text
max speed     = 6
acceleration  = 40
deceleration  = 50
jump speed    = 8
gravity       = 20
coyote        = 0.10 s
jump buffer   = 0.10 s
```

Ideal maximum jump rise is approximately 1.6 world units. Phase A must
check horizontal and vertical gaps before approving geometry.

## Checkpoint Data

Replace the single-checkpoint assumption with a fixed project-owned
collection containing exactly two `CheckpointSpec` values.

Preferred concept:

``` cpp
std::array<CheckpointSpec, 2>
```

No heap allocation, runtime registration, checkpoint manager, trigger
framework, ECS, or level manager.

Each checkpoint has a stable index: 0 and 1.

## Ordered Progression

Conceptual state:

``` text
activeCheckpointIndex = none
respawnPosition = initial spawn
```

Entering Checkpoint 1:

``` text
activeCheckpointIndex = 0
respawnPosition = checkpoint 1 respawn
```

Entering Checkpoint 2 after Checkpoint 1:

``` text
activeCheckpointIndex = 1
respawnPosition = checkpoint 2 respawn
```

Entering Checkpoint 1 again after Checkpoint 2 does nothing.

Checkpoint 2 must not activate while `activeCheckpointIndex` is none.

## RespawnState Migration

Phase A must inspect all consumers of the current:

``` text
checkpointActive
respawnPosition
deathCount
lastRespawnReason
```

Prefer replacing redundant `checkpointActive` with an optional/sentinel
checkpoint index rather than keeping contradictory state.

Keep the representation simple and project-owned.

## Detection

Continue using Player visual-center point vs checkpoint AABB.

No Jolt sensor/body.

No checkpoint meaning in PhysicsWorld.

Checkpoint evaluation occurs only on frames without Fall/Manual respawn,
preserving M20 same-frame rules.

## Safe Respawns

For both checkpoints verify:

-   visual AABB is non-penetrating;
-   CharacterVirtual feet position is valid;
-   M23 inner body synchronizes safely;
-   fixed Z is valid;
-   no static geometry occupies the player volume;
-   no moving platform can sweep through the respawn volume;
-   checkpoint is not inside the goal;
-   checkpoint is not at the cyan-box canonical spawn.

Phase A must explicitly compare both respawn volumes against the
moving-platform swept volume.

## Checkpoint Placement

The existing M20 checkpoint at approximately `{5, 1.8, 0}` may remain
Checkpoint 1 only if it is still useful and safe in the expanded route.

Do not preserve it blindly.

Checkpoint 2 must be later in traversal, safely reachable, and useful
before the final goal section.

## Goal

Preserve one M21 goal and its semantics. Phase A may relocate it if
necessary so it becomes the actual endpoint of the longer route.

Preserve:

-   `LevelGoalSpec`;
-   point/AABB detection;
-   one-way completion per run;
-   `LEVEL COMPLETE`;
-   `PRESS ENTER TO RESTART`;
-   completion surviving R/Fall;
-   Enter restart clearing completion.

No generic goal system.

## Visual States

Render both checkpoint markers with three semantic states:

``` text
Future
Current
PreviouslyActivated
```

Use existing primitives only.

Example visual intent:

-   Future: muted/steel.
-   Current: bright green.
-   Previously activated: subdued green.

Renderer consumes simple project-owned visual state only. It does not
perform overlap/progression logic.

## Manual R

Expected destinations:

``` text
no checkpoint -> initial spawn
checkpoint 1  -> checkpoint 1
checkpoint 2  -> checkpoint 2
```

R does not increment `deathCount`.

## Fall

Fall increments `deathCount` exactly once and respawns at the latest
checkpoint, or initial spawn if none is active.

## Restart

M22 Enter restart resets:

``` text
active checkpoint = none
respawn position  = initial spawn
deathCount        = 0
last reason       = None
completed         = false
```

It must continue resetting Player/CharacterVirtual/inner body, moving
platform, cyan box and camera.

After restart both checkpoint markers are Future again.

## M23 Preservation

Preserve:

``` text
CharacterVirtual:
  inner body enabled
  mass = 70 kg
  maxStrength = 100 N

cyan box:
  Dynamic
  size = 1x1x1
  mass = 30 kg
  spawn = {0,5,0}
```

The cyan box remains a technical physics object, not a required
traversal puzzle.

Do not introduce a new compression/wedge trap through level geometry.

## Moving Platform

Preserve M13 physics semantics: kinematic `MoveKinematic`, carry,
airborne carry, reversal and restart reset.

Prefer keeping size `{4,0.4,3}`, speed `2.5`, and current path. If Phase
A proposes relocation, it must justify coordinates without changing
physics semantics.

## Slopes

Preserve max slope 50°, 30° walkable and 60° steep.

## Camera

Preserve M08 dead zones 1.5/0.75 and smoothing sharpness 8.

The longer level is a camera stress test, not a camera-redesign
milestone.

## Debug Metrics

Debug/Development should show at minimum:

``` text
active checkpoint index / none
respawn position
death count
last respawn reason

checkpoint 1:
  inside
  visual/progression state

checkpoint 2:
  inside
  visual/progression state
```

Keep M23 physics diagnostics. Release remains free of Dear ImGui.

## Architecture

Preserve ownership:

``` text
Application  -> run/checkpoint/goal meaning
Player       -> movement policy
PhysicsWorld -> physical simulation
Renderer     -> visual consumption only
world/*      -> project world specifications
```

No Jolt checkpoint sensor, Renderer mutation, or generic trigger/level
framework.

## Asset Pipeline

No changes to cooker, Pillow, Blender workflow, source/cooked assets,
runtime PNG recipe, or CMake staging. No new dependency.

## Phase A --- Inspect + Design

Phase A must:

1.  inspect the exact M23 world;
2.  inventory all static geometry;
3.  map moving-platform swept volume;
4.  map cyan-box spawn/interaction region;
5.  map current checkpoint and goal;
6.  calculate Player reach;
7.  propose the longer greybox;
8.  propose Checkpoint 1 and Checkpoint 2;
9.  verify both safe respawn volumes;
10. define minimal `RespawnState` migration;
11. define checkpoint visual states;
12. define final update ordering;
13. create only minimal scaffolding if useful;
14. build Debug/Development/Release;
15. stop before full multi-checkpoint wiring.

**Phase A:** Complete. Design and scaffolding recorded and approved. The live M23 single checkpoint at `{5, 1.8, 0}` was rejected because its respawn player AABB overlaps the moving-platform swept volume.

### Phase A design record

Movement envelope (unchanged): jump rise 1.6, apex 0.4 s, same-height airtime 0.8 s, max-speed air distance 4.8.

Live M23 checkpoint `{5, 1.8, 0}` is **not** reused: its respawn player AABB overlaps the moving-platform swept volume (X [-8, 8], Y [1.1, 1.5]).

Proposed route (Phase B):

``` text
Spawn {0, 0.8, 0}
  -> early right / optional 30-degree slope
  -> Checkpoint 1 {16.5, 1.8, 0} on new platform top Y = 1.0
  -> back to moving platform (unchanged path)
  -> left mid landing (existing)
  -> new mid-left step
  -> Checkpoint 2 {-15.5, 2.8, 0} on new platform top Y = 2.0
  -> Goal {-21, 3.8, 0} on new platform top Y = 3.0
```

Ground resized to `{48, 0.5, 8}` at `{0, -0.25, 0}`. Steep 60-degree slope relocated to `{22, 0.966, 0}` (classification dead-end). Walkable 30-degree slope stays. Kill plane stays Y = -8.

`RespawnState` Phase B: `int activeCheckpointIndex = -1` replaces `checkpointActive`. Activation: `expectedIndex = active + 1` against `kCheckpoints[expectedIndex]` on the existing no-respawn branch.

Renderer Phase B: two `CheckpointVisualState` values (Future / Current / PreviouslyActivated). Do not pass `RespawnState` wholesale.

See `docs/ARCHITECTURE.md` and `world/RespawnWorld.h`.

## Phase B --- Implement

After Phase A approval:

-   implement approved greybox expansion;
-   implement exactly two ordered checkpoints;
-   update respawn progression;
-   render both marker states;
-   update diagnostics;
-   preserve M20--M23;
-   build all configs;
-   stop before final approval.

**Phase B:** Implementation recorded. Live world used the approved M24 greybox. `kCheckpoints` has exactly two ordered specs. `RespawnState::activeCheckpointIndex` replaced `checkpointActive`. Renderer consumes two `CheckpointVisualState` values.

Manual play found a **traversal geometry** defect, not a checkpoint-order defect: CP2 correctly refuses to activate before CP1; CP1 on the right activates. After CP1 the 30-degree ramp blocked return to the center.

## Phase B.1 --- Traversal Geometry Correction

The M14 30-degree box at `{10.90, 1.6732, 0}` is rotated **+30 degrees about Z**. Approximate world ends:

``` text
low  (left)  top:    X ~ 8.20,  Y ~ 0.35
high (right) top:    X ~ 13.40, Y ~ 3.35
```

The high end sat immediately left of CP1 (platform X [14.5, 18.5], top Y = 1.0). Players could walk up and drop onto CP1. Return failed: max jump rise 1.6 cannot clear a ~2.35 m face, and the underside wedges anyone walking back on the ground. The 60-degree slope at X=22 did not cause this trap.

Correction (one intent: clear the center <-> CP1 corridor):

- 30-degree test moved to `{21.70, 1.6732, 0}` (optional dead-end past CP1).
- 60-degree test moved to `{25.60, 0.966, 0}` so it does not overlap the 30-degree box.
- Ground expanded to size `{56, 0.5, 8}` (X [-28, 28]) so both tests rest on support.

CP1/CP2/goal specs and ordered activation are unchanged.

**Phase B.1:** Complete. The user manually verified spawn -> CP1 activation and CP1 -> central/moving-platform return without R, death, clipping, or frame-perfect play. Center -> CP1 remains comfortable.

## Phase C --- Manual Validation (current)

Required manual cases:

A. Fresh run: R -\> initial spawn.

B. Activate Checkpoint 1: R -\> CP1.

C. Attempt CP2 before CP1 if physically possible: CP2 must not activate.

D. Activate CP2 after CP1: R -\> CP2.

E. Backtrack through CP1: active remains CP2.

F. Fall after CP1: death +1, respawn CP1.

G. Fall after CP2: death +1, respawn CP2.

H. Reach goal: completion UI appears.

I. R after completion: latest checkpoint, completion remains true.

J. Fall after completion: death +1, latest checkpoint, completion
remains true.

K. Enter restart: initial spawn, checkpoint progress cleared, deaths 0,
goal incomplete, platform/box reset, camera snap.

L. Second run: both checkpoints activate again in order.

M. M23 cyan box: pushable, Player solid, no persistent wedge.

N. Moving platform: standing carry, airborne carry, reversal.

O. Slopes: 30° walkable, 60° steep.

P. Camera: longer traversal remains readable; respawn/restart snap
correct.

Q. Release: full gameplay works without ImGui.

**Phase C:** Complete and manually validated. Longer greybox, two ordered checkpoints, spawn → CP1 → center return → moving platform → left landing → mid-left step → CP2 → goal, plus R/Fall/Enter and M23 regressions, are accepted. Milestone 25 is now the active milestone.

## Completion Criteria

-   [x] Traversal is meaningfully longer.
-   [x] Exactly two checkpoints exist.
-   [x] CP2 requires CP1.
-   [x] Latest checkpoint controls respawn.
-   [x] Backtracking cannot downgrade progression.
-   [x] Both respawn volumes are safe.
-   [x] Moving platform cannot occupy either respawn volume.
-   [x] R/Fall semantics are correct at none/CP1/CP2.
-   [x] Goal remains reachable.
-   [x] Completion survives R/Fall.
-   [x] Enter restart clears checkpoint progression.
-   [x] Second run works.
-   [x] Markers communicate Future/Current/Previous.
-   [x] Renderer owns no checkpoint logic.
-   [x] PhysicsWorld owns no checkpoint meaning.
-   [x] No Jolt checkpoint sensor/body.
-   [x] M23 inner body and 30 kg box remain correct.
-   [x] No persistent wedge regression.
-   [x] Moving platform behavior remains correct.
-   [x] Slopes remain correct.
-   [x] Camera behavior remains M08.
-   [x] Gameplay constants remain unchanged.
-   [x] Debug metrics support two checkpoints.
-   [x] Release has no ImGui.
-   [x] Asset pipeline remains unchanged.
-   [x] No dependency added.
-   [x] No generic level/checkpoint framework.
-   [x] Milestone 25 is not started. (superseded: M25 Phase A is active)

## Explicitly Out of Scope

Do not add Milestone 25, more than two checkpoints, multiple levels,
save/load, serialization, JSON level data, LevelManager, SceneManager,
generic trigger manager, checkpoint registry, ECS/event bus, enemies,
combat, health, lives, collectibles, score, timer, audio, particles, new
assets, dependencies, or cooker changes.

## Git Branch

``` powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/24-extended-traversal-checkpoints
git branch --show-current
```

## Recommended Model

Use **Grok 4.6 High --- Fast OFF** for Phase A and preferably Phase B.
Geometry reachability, safe CharacterVirtual/inner-body respawns,
moving-platform swept volume, checkpoint-state migration, and existing
M20--M23 ordering interact here.

## Workflow

``` text
main clean
 -> M24 branch
 -> Phase A inspect/design
 -> review
 -> Phase B implementation
 -> review
 -> Phase C manual traversal/regression
 -> approve
 -> commit
 -> push branch
 -> merge main
 -> push main
 -> verify clean main
```

Do not commit, push, or merge before manual Phase C approval.
