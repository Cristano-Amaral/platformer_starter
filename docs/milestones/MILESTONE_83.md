# Milestone 83 --- 3D Player Character Foundation

**Status:** CLOSED\
**Branch:** `milestone/83-3d-player-character-foundation`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Introduce the first proper 3D visual presentation layer for the player
while preserving the existing controller, Jolt collision authority,
gameplay rules, camera, interactions, health/damage/death/respawn,
movement audio, transitions and UI flows.

Architecture:

``` text
Player
├─ Gameplay / Controller
├─ Physics collision authority
└─ Presentation
   ├─ 3D model
   ├─ visual scale
   ├─ visual offset
   └─ facing/orientation
```

The model follows gameplay/physics; it never becomes gameplay authority.
Animation is deferred.

## Product outcome

During Gameplay, represent the player with a staged `.glb` model rather
than only the current primitive/debug-style visual. The model follows
the authoritative player position, faces horizontal movement, preserves
last meaningful facing while stationary, and supports presentation-only
offset/scale independent of the collider.

The presentation must survive/reset correctly through spawn, checkpoint
respawn, death, Restart, level transitions, Play Again, Main Menu/Play
and F2 transitions.

## Inspect first

Before implementation inspect the current player controller, Jolt
body/collider, player rendering, camera target, movement/grounded state,
interaction/LOS origins, death/respawn, movement SFX, staged GLB loading
conventions, Static Prop/Item Pickup model rendering, runtime staging,
Release path and F2 lifecycle.

Do not assume the player is an authored Level object.

## Presentation state

Add the smallest explicit state/config needed, conceptually:

``` text
PlayerPresentation
├─ staged model identity
├─ visual offset
├─ visual scale
├─ facing/yaw
└─ model lifetime/readiness
```

Follow repository conventions. No ECS, Actor/Character framework, scene
graph, generic Renderable system or animation state machine.

## Player model asset

Use an existing suitable project-owned model if present; otherwise add a
simple project-owned `.glb` placeholder suitable for redistribution.
Avoid licensing ambiguity and external character marketplace
dependencies.

The asset must be staged deterministically and loaded from staged
runtime assets only. No source fallback in runtime/Release.

## Loading/lifetime

Reuse existing model/runtime conventions where appropriate. Load once
per appropriate lifetime, not per frame; unload safely; missing/invalid
model must not corrupt controller/physics/gameplay or spam logs. A
narrow old-primitive fallback on load failure is acceptable, but it must
not render simultaneously with a successfully loaded model.

Do not turn M83 into an asset-manager rewrite.

## Authority

Hard rule: existing player controller/collider remains authoritative.

Do not change collider dimensions, movement speed, acceleration, jump,
grounded detection, hazards, checkpoint behavior, interactions, goal
overlap or camera merely to fit the visual model. Fit visuals with
presentation offset/scale/orientation.

## Visual transform and facing

Derive presentation every frame from authoritative player state. Visual
transform must never write back into Level data or move the physics
body.

Facing: - derive from meaningful horizontal accepted/authoritative
movement; - update deterministically while moving; - preserve last
meaningful facing while stationary; - ignore near-zero movement; -
vertical velocity must not change facing; - use a fixed presentation
forward-axis correction if the GLB requires it; - do not rotate physics
solely for visuals.

Choose/document whether visual offset is world-space or yaw-relative
based on existing coordinate conventions.

## Lifecycle facing

Define deterministic initial/reset facing for Main Menu -\> Play, Play
Again, Restart, level transition, checkpoint/death respawn and gameplay
reconstruction. Do not add authored player-facing Level syntax.

## Preserve camera and interactions

Camera continues to follow authoritative player/controller state, not
model-specific visual points.

The model must not become authority for Item Pickup targeting, Door
interaction, LOS, Checkpoint, Pressure Plate overlap, Hazards,
Collectibles or Level Goals.

## Rendering

When the model loads successfully, normal Gameplay/Release must not also
show the old gameplay-facing primitive player representation.
Development-only debug collider visualization may remain behind an
existing appropriate debug path.

Preserve Main Menu, Gameplay, Pause, Inventory, Level Complete hold, Run
Complete, death delay and F2 behavior.

## Death, respawn and transitions

Preserve M70 semantics. No death animation. Respawn must snap
presentation to authoritative respawn position with no stale model at
the death location.

Verify level_01 -\> level_02, destination spawn, Restart, Play Again,
Main Menu -\> Play and no stale transform/facing across transitions.
Existing Inventory/Health lifecycle semantics remain unchanged.

## Movement audio

Preserve M72 Footstep/Jump/Landing exactly. Audio remains
gameplay/controller-driven, not animation-driven.

## Editor boundary

Player is not a new authored Level object. No Level Format player entry,
model selector, placement gizmo, Hierarchy player object or per-Level
player visual settings.

## Runtime staging/failure

Player model must be included in staged runtime assets for
Debug/Development/Release. Release must not depend on source
directories, Content Browser, authoring roots or Development-only
discovery.

Load failure must fail safely, log once by repository convention,
preserve gameplay and transitions, and use only a narrow safe fallback
if necessary.

## Testable boundary

Keep facing/visual-transform/lifecycle derivation testable without GPU
where practical. Do not add a generic transform/animation framework.

## Regression coverage

Cover meaningful +X/-X/+Z/-Z/diagonal facing, near-zero/stationary
preservation, vertical-only movement, model-forward correction,
presentation offset/scale independence from gameplay/physics,
authoritative position following, lifecycle reset,
respawn/transition/Restart/Play Again, safe model-load failure,
non-per-frame loading, no duplicate primitive on successful model load,
staged-only asset path, no Level Format player syntax,
camera/interaction authority, M69--M75 gameplay/audio regressions,
M76--M82 editor regressions, all builds, and canonical Level safety.

## Manual acceptance

In Development and Release verify: - Main Menu -\> Play; - visible 3D
player model and no duplicate old primitive; - movement in
cardinal/diagonal directions and correct facing; - stable facing when
stopped; - jump/fall without facing jitter; - unchanged camera; -
Collectible, Item Pickup, Door, Pressure Plate, Checkpoint and Hazard; -
death and checkpoint respawn; - movement SFX; - Pause/Resume and
Inventory; - level_01 -\> level_02 and destination spawn; - Restart,
finish run, Play Again, Main Menu/Play; - F2 roundtrip; - M82
Groups/Hierarchy regression; - Release staged-only execution.

Automated green is not sufficient.

## Canonical Level safety

Do not intentionally modify: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

No player presentation data belongs in these Levels. Do not normalize
EOLs. Final semantic diffs must be empty.

## Validation

Run directly affected current C++ suites for player movement/controller,
physics rebuild, checkpoints, hazards/health/death/respawn,
interactions, Item Pickup, Pressure Plate/Door, Level Goal/transitions,
gameplay/movement audio, model/runtime assets, F2/lifecycle and
appropriate M76--M82 editor regressions.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Add/extend the narrowest staging regression if current tests do not
prove the player model is staged.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Then:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
```

## Out of scope

No skeletal animation, Idle/Run/Jump/Fall/Land clips, blending, root
motion, animation-driven footsteps, IK, ragdoll, customization, multiple
playable characters, authored per-Level player model, skin selector,
controller/physics redesign, collider redesign merely for visuals,
camera redesign, combat/weapons, generic Actor/Character framework, ECS,
scene graph, asset-manager rewrite, Materials/Textures foundation,
Lighting/Shadows, Terrain or M84 functionality.

## Documentation / report / STOP

Canonical active document: `docs/milestones/MILESTONE_83.md`.

Preserve M76--M82 as CLOSED. Keep `docs/MILESTONES.md` compact. Document
clearly that gameplay/controller/physics is authority and the 3D model
is a presentation follower.

Cursor report must include: files changed; discovered player
architecture; authoritative transform; presentation state/config; GLB
identity/provenance; staging/loading/lifetime/fallback;
offset/scale/facing and forward-axis rules; lifecycle facing;
confirmation controller/physics/camera/interaction authority unchanged;
old primitive behavior; death/respawn/transitions; movement SFX; F2;
Release staged-only behavior; test helpers/regressions; C++/Python/build
results; `git diff --check`; canonical Level diffs; out-of-scope
confirmation.

After implementation M83 is **implemented, awaiting manual acceptance**,
not CLOSED.

Then STOP.

Do NOT commit. Do NOT push. Do NOT merge. Do NOT start M84. Do NOT mark
M83 CLOSED.
