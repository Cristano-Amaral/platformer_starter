## Milestone 58.3 --- Item Pickup Gameplay Target Highlight

### Status

**Implemented.** Awaiting manual acceptance. Do not mark CLOSED. Milestone
58.2 is CLOSED. Do not start Milestone 59.

### Branch

`milestone/58.3-item-pickup-target-highlight`

### Cursor Model

**Grok 4.6 High --- Fast OFF**

### Goal

Improve Gameplay targeting feedback for Item Pickups. When a
model-backed pickup becomes the current M55 target, the actual rendered
GLB must receive a clear golden highlight. The existing yellow
interaction-bounds wire becomes secondary, must follow the M58 visual
transform correctly, and can be disabled per Item Pickup in the
Inspector.

Target experience:

**current pickup target → highlighted actual model → optional
transformed interaction bounds → existing
`E Pick Up <itemId> x<quantity>` HUD**

This is presentation only. Do not change how pickups are targeted or
collected.

### Preserve Existing Authorities

Inspect the repository first. Preserve M55 `FindItemPickupTargetIndex`,
targeting distance/facing/LOS/nearest/tie rules, E arbitration,
collection/Inventory integration, `ItemPickupRunState`, M58 visual
transforms, M58.2 cached model highlight/tinted drawing where safely
reusable, `StaticModelSceneStore`, staged runtime asset authority,
fallback rendering, HUD, workingCopy/active/savedSourceBaseline,
Apply/Revert/Save, Level Format v1, renderer/rlgl state restoration,
Release behavior, and tests.

Rendering must consume the existing target result. Do not duplicate
target search logic.

### Model-backed Gameplay Target Highlight

When an uncollected Item Pickup with an assigned available GLB is the
current gameplay target, render a clear golden highlight using: -
`position + visualOffset` - `visualRotationDegrees` - `visualScale`

Prefer narrowly reusing the safe M58.2 tinted model drawing path. Reuse
the cached model resource. No per-frame loading, filesystem I/O, source
fallback, GLB parsing, or mesh duplication.

A tinted second pass is sufficient. True bloom/post-processing is not
required. Do not create a generic gameplay glow/outline framework.

A subtle occluded/x-ray pass may be reused only if it is already safe
and appropriate. It is not mandatory. Restore every changed render
state.

### Authored Interaction Bounds Toggle

Add exactly one narrow Item Pickup presentation field:

`bool showInteractionBounds = true`

Meaning: - `true`: when this pickup is the current gameplay target, draw
the interaction-bounds wire in addition to model/fallback highlight. -
`false`: suppress that wire.

This field controls presentation only. It must never affect targeting,
LOS, collection, Inventory, Door unlocking, editor picking, or gameplay
interaction position.

Default `true` preserves prior behavior.

### Inspector

Add Item Pickup Inspector checkbox:

**Show Interaction Bounds**

It edits workingCopy and participates in normal authored lifecycle: -
Add defaults true - Duplicate preserves - equality / Modified / Dirty -
Apply - Revert - Save / reload

No global preference.

### Level Format v1

Extend the existing M58 Item Pickup record narrowly and
backward-compatibly.

Because `modelIdentity` may contain spaces, do not append an ambiguous
bare boolean after arbitrary model identity text.

Preferred explicit marker:

`bounds <0|1>`

Inspect the actual current M58 parser/writer before choosing exact
placement. Use an unambiguous marker placement that preserves all legacy
model identities with spaces.

Requirements: - old M55/M58 records without marker remain valid; -
omitted flag defaults true; - canonical writer emits the explicit flag
deterministically; - boolean accepts exact `0` or `1` only; - model
identity round-trips unchanged, including spaces; - no Level Format v2.

If `bounds` placement conflicts with actual grammar, use the narrowest
unambiguous equivalent and report it.

### Correct Model-backed Interaction Bounds

When `showInteractionBounds == true`, the targeted model-backed pickup
wire must follow the actual M58 visual transform rather than a
disconnected world-axis cube around gameplay `position`.

Use: - `position + visualOffset` - `visualRotationDegrees` -
`visualScale`

Prefer M58.2 model-local bounds: 1. loaded model-local bounds from
existing cache; 2. otherwise existing local fallback/proxy bounds.

Transform the local corners through the actual visual transform and draw
a quiet golden wire. This wire is visualization only and must not become
targeting geometry.

### Model Highlight vs Bounds

For model-backed pickups, the model highlight is the primary cue. Bounds
are secondary.

Therefore `showInteractionBounds = false` must still preserve: -
targeted model highlight; - existing HUD prompt; - target eligibility; -
collection.

### Fallback / Missing Model

Pickup with no model: preserve safe fallback cube behavior and clear
targeting feedback. The toggle controls the additional interaction wire
consistently.

Missing staged model: preserve existing placeholder and safe targeting.
No source fallback.

### HUD and E Arbitration

Preserve `E Pick Up <itemId> x<quantity>`.

Preserve current arbitration: 1. carrying box → Drop 2. else Dynamic Box
target → Grab 3. else Item Pickup target → Collect 4. else locked Door
target → Unlock 5. else nothing

One E = one action.

### Editor Isolation

The new checkbox controls Gameplay Item Pickup target bounds. It must
not disable M58.2 Editor model-selection ghost, editor picking, or
gizmos.

M58.2 selection behavior remains intact.

### Gameplay / Release

This highlight is player-facing and must work in normal gameplay,
including Release. No ImGui dependency. Do not accidentally depend on
Development-only editor code.

### Render-state Safety

If target highlighting changes depth, depth mask, blend, culling,
shader/material state, line width, or rlgl matrix stack, restore
expected state afterward. Reuse M58.2 helpers only if their
scope/compile-time availability is valid for Gameplay/Release.

### Gameplay Isolation

Do not change: - logical `ItemPickupSpec::position` targeting
authority; - M55 range/facing/LOS/nearest/tie behavior; - collection
atomicity; - Inventory; - checkpoint collected-state preservation; -
Restart/Apply/reload semantics; - PhysicsWorld rebuild semantics; - Door
`requiredItemId`; - Pressure Plate behavior; - Dynamic Box Grab/Carry.

Visual Offset/Rotation/Scale remain presentation-only for gameplay
targeting.

### Explicitly Out of Scope

No bloom pipeline, generic gameplay outline/glow system, generic
Interactable highlighting, Door/Dynamic Box/Static Prop gameplay
highlight configuration, per-item colors, pulsing/bobbing/spinning
animation, particles, sounds, interaction-radius authoring, target
position from mesh, triangle targeting, ItemDefinition/catalog, generic
presentation/Transform/Render component, ECS, undo/redo, Level Format
v2, or M59 functionality.

### Focused Tests

Cover: 1. `showInteractionBounds` defaults true. 2. Add defaults true.
3. Duplicate preserves. 4. equality detects change. 5. Revert restores.
6. Apply promotes. 7. Save persists. 8. legacy record without marker
defaults true. 9. canonical writer emits marker deterministically. 10.
exact `0` parses false. 11. exact `1` parses true. 12. invalid boolean
token rejected. 13. model identity with spaces round-trips. 14.
non-targeted model pickup has no target highlight. 15. targeted model
pickup receives model highlight. 16. highlight uses visual
position/rotation/scale. 17. highlight reuses cached model. 18.
collected pickup has no highlight. 19. bounds true draws wire. 20.
bounds false suppresses wire. 21. bounds false preserves model
highlight. 22. bounds false preserves HUD. 23. transformed bounds align
with visual transform. 24. bounds do not affect target eligibility. 25.
no-model fallback safe. 26. missing staged model safe. 27. no source
fallback. 28. target search remains position-based. 29.
range/facing/LOS/nearest/tie unchanged. 30. collection and Inventory
unchanged. 31. E arbitration unchanged. 32. Door/Pressure Plate behavior
unchanged. 33. M58.2 Editor ghost/picking unchanged. 34.
matrix/depth/blend/cull/material state restored. 35. subsequent
rendering unaffected.

Do not expose broad public production APIs solely for tests.

### Manual Acceptance

Use disposable Development fixtures.

#### Model-backed pickup

Add an irregular GLB Item Pickup, clear itemId, non-zero Visual
Offset/Rotation and non-uniform Scale. Apply/run and approach it.
Confirm the actual model receives clear golden target feedback. With
bounds enabled, confirm the wire follows the transformed model rather
than logical `position`. Confirm HUD remains correct.

#### Toggle

Disable **Show Interaction Bounds**, Apply/run, target the pickup, and
confirm model highlight + HUD remain while the wire disappears.
Re-enable and confirm the wire returns.

#### Targeting invariance

Use a large Visual Offset. Confirm targeting remains based on logical
`position`, not highlighted mesh. Rotation/scale must not change
range/facing/LOS. Collect and verify correct Inventory item/quantity.

#### Multiple pickups

Place at least two pickups. Only the current deterministic M55 target
should highlight. Move/turn so target changes and confirm highlight
transfers. Collected pickup loses highlight.

#### Fallback

Test no-model pickup and, if practical, missing staged model. Confirm
safe feedback and no source fallback.

#### Editor regression

Return to F2 and confirm M58.2 selection ghost plus
Translate/Scale/Rotate still work.

#### Lifecycle

Toggle the field, Revert, Apply, Save/reload and verify expected
authored lifecycle.

Remove all disposable fixtures before closure.

### Validation

Run relevant C++ tests for LevelFile/LevelWriter, Item Pickup, renderer,
StaticModelSceneStore, M55 targeting/collection, M56 Inventory UI,
M57/M57.1 Doors, M58 visual transform, M58.1 Rotate, M58.2 selection
preview, Pressure Plates, authored lifecycle, render-state restoration,
and canonical cleanup.

Run:

    python tools/test_stage_runtime_assets.py
    python tools/test_cook_level_v1.py
    python tools/test_cook_runtime_png.py
    python tools/test_import_static_glb.py

Build:

    cmake --preset windows-vs2022
    cmake --build --preset windows-debug
    cmake --build --preset windows-development
    cmake --build --preset windows-release

Run `git diff --check`.

### Canonical Data Safety

Before closure, Level 01 remains fixture-free: - Item Pickups: 0 -
Doors: 0 - Pressure Plates: 0 - Dynamic Boxes: 0 - Static Props: 0

Keep absent: `dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

### Completion Criteria

Ready for manual acceptance when the current model-backed Item Pickup
target highlights the actual model; highlight follows M58 visual
transform; optional bounds align with transformed model;
`Show Interaction Bounds` is authored per pickup and defaults true;
disabling it leaves highlight/HUD intact; legacy records and model
identities with spaces remain valid; M55 targeting is unchanged;
fallback/missing-model behavior is safe; M58.2 Editor selection remains
intact; render state is restored; all builds/tests pass; canonical Level
01 is clean; and no generic highlighting/postprocess/Interactable
framework or M59 functionality was introduced.

### STOP

After implementation, validation and report: STOP.

Do not commit, push, merge, start M59, or declare M58.3 CLOSED. Wait for
user manual acceptance and separate Git closure.
