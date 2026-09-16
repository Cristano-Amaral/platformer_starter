## Milestone 45 --- Authored Dynamic Physics Objects

**Branch:** `milestone/45-authored-dynamic-physics-objects` **Status:**
Implemented on `milestone/45-authored-dynamic-physics-objects`. Awaiting
manual acceptance. Not complete. Milestone 46 has not started.
**Prerequisite:** M44 CLOSED

### Goal

Replace the useful behavior once demonstrated by the legacy Dynamic Cyan
Box with a real authored feature: repeatable box-shaped dynamic rigid
bodies that can be created in the Development editor, persisted in the
level, instantiated through Jolt, and physically pushed by the player.

Each Dynamic Box has authored center, size, and mass in kilograms. M45
is intentionally box-only and is not a general physics-object framework.

### Authoring

Add `Dynamic Box` as a repeatable authored category alongside Platform,
Checkpoint, Hazard and Collectible.

Support: - Object Palette placement - Edit \> Add - Duplicate Selected -
Delete Selected - Translate - Resize - Inspector center/size/mass
editing - Apply/Revert/Save - pending Add/Modify/Delete visuals - active
and pending viewport picking

Hierarchy uses a `Dynamic Boxes` group. Do not restore the legacy
singleton `Dynamic Cyan Box`.

Preferred defaults: size `{1,1,1}`, mass `30 kg`. Object Palette
placement follows M42 Ground/Platform/Slope surface-hit and fallback
behavior.

### Data and validation

Use a value-level definition equivalent to:

`DynamicBoxDefinition { center, size, massKg }`

Do not expose Jolt internals in LevelDefinition.

Size components must be finite and positive; reuse the established 0.12
minimum extent where appropriate. Mass must be finite, \> 0, and have
one centralized defensible safety maximum. Reject zero, negative, NaN
and infinity.

### Level Format v1 migration

M44 retained the legacy singleton `dynamic_box` record only for
compatibility. M45 should evolve the existing syntax into a repeatable
record:

`dynamic_box <cx> <cy> <cz> <sx> <sy> <sz> <massKg>`

Prefer keeping Level Format v1. Allow zero, one or multiple records in
encounter order; no count field. Migrate the in-memory singleton to a
vector/list.

If a safe v1-compatible migration is impossible, STOP and report before
introducing Level Format v2.

The canonical M44 legacy `dynamic_box` line must not silently resurrect
the old probe. Preferred canonical Level 01 migration is Dynamic Boxes =
0. Its removal is an intentional semantic source diff and must be
reported; do not erase it with the historical EOL restore.

Preserve file safety bounds: 64 KiB, 256 lines, 512 chars/line.

### Runtime physics

Each authored Dynamic Box creates one Jolt dynamic rigid body with: -
box collision matching authored size - authored mass - gravity -
collision with intentional world geometry - ordinary contact-based
interaction with CharacterVirtual

Do not fake pushing with manual player-input translation.

After instantiation, Jolt pose is authoritative for runtime rendering.
Authored transform remains authoritative for rebuild/reset/reload/save.
Never continuously overwrite the simulated body pose from authored data.

### Rendering

Render from current Jolt pose with a simple intentional
greybox/debug-gameplay appearance. Do not restore the hard-coded cyan
probe renderer or any M15--M23 legacy scene probe.

### Physics rebuild

Integrate Dynamic Boxes into transactional `PhysicsWorld::TryRebuild`.
Apply reconstructs bodies from active authored definitions. Failure
preserves the previous active/runtime world.

### Shared body capacity

M44 state: - `kPhysicsMaxBodies = 64` - fixed non-Platform usage = 5 -
Platform-only capacity = 59

M45 makes Dynamic Box count variable. Replace the independent
Platform-only assumption with a shared authored-body budget derived from
real body accounting.

At minimum enforce semantically:

`fixedBodies + platformCount + dynamicBoxCount <= kPhysicsMaxBodies`

Lifecycle Add/Duplicate and level validation must reject overflow
deterministically. Reuse `AtLimit` if semantically sufficient. Do not
create arbitrary independent quotas that can exceed Jolt capacity.

### Player interaction and reset

Player must be able to push Dynamic Boxes through ordinary collision.

Define deterministic reset behavior. Preferred: - full run restart
resets boxes to authored poses and zero velocities - Apply/reload
reconstruct from authored definitions - checkpoint respawn alone does
not reset all boxes unless existing run semantics require it

Audit current restart boundaries and document/test the chosen policy.

### Editor semantics

Inspector edits workingCopy only. Gizmo Translate/Resize edits
workingCopy only. Do not drag the live Jolt body with editor gizmos.

Pending ghost represents authored proposal while the active body may
continue simulating.

Active picking must use current runtime physics pose, not stale authored
center. Extend M41 structural mapping to Dynamic Boxes; no GUIDs.

Save serializes active authored definitions, never transient simulated
poses. A box authored at X=2 and pushed to X=8 still saves X=2 unless an
authored edit was applied.

### Reload / cook / stage

M39 reload reconstructs boxes from staged authored definitions and
resets their runtime poses/velocities. No special hot reload system.

No new external asset is required. Preserve M40 Cook → Stage → Reload
workflow.

### Build boundaries

Runtime Dynamic Boxes work in Debug, Development and Release. Authoring
UI remains Development-only.

### Tests

Add focused tests for: - zero/one/multiple records - encounter order and
deterministic round-trip - malformed record - invalid mass/size
including NaN/Inf - exact shared body-budget limit accepted and one-over
rejected - one definition creates one Jolt dynamic body - authored size
and mass mapping - gravity/Ground collision - deterministic contact/push
behavior where practical - rebuild/restart reset - transactional rebuild
failure - body removal after authored Delete + Apply -
Add/Duplicate/Delete/Translate/Resize - Inspector mass edit - pending
lifecycle visuals/identity - active moving-body picking - pending
picking - Object Palette placement - capacity rejection - Save authored
pose rather than simulated pose

Run all applicable configs for: `AuthoredObjectLifecycleTest`,
`AuthoredLifecycleIntegrationTest`, `CanonicalSceneCleanupTest`,
`EditorWorkspaceTest`, `EditorGizmoTest`, `EditorOrientationTest`,
`EditorPickingTest`, `EditorPlacementTest`, `EditorQuickToolbarTest`,
`LevelFileTest`, `PhysicsRebuildTest`, `CookStageReloadWorkflowTest`,
`EditorToolRunnerTest`, plus new M45 tests.

Run: `python tools/test_stage_runtime_assets.py`
`python tools/test_cook_level_v1.py`
`python tools/test_cook_runtime_png.py`

Then configure and build Debug, Development and Release with the
existing Windows presets.

### Manual acceptance

In Development/F2: 1. Place Dynamic Box from Object Palette. 2. Verify
pending visual and Hierarchy. 3. Apply and verify a real dynamic body.
4. Walk into it and verify physical pushing/gravity/Ground collision. 5.
Compare same-size boxes with meaningfully different masses such as 5 kg
and 100 kg. 6. Resize, Apply and verify render/collision dimensions. 7.
Translate workingCopy and verify live body does not teleport until
Apply. 8. Push a box away, Save, and verify simulated pose is not baked
into authored source. 9. Test Duplicate/Delete with Apply/Revert. 10.
Push boxes and test documented restart reset. 11. Test staged runtime
reload reconstruction. 12. Verify slopes, moving platform, checkpoints,
hazards, collectibles, goal, Object Palette and Quick Toolbar. 13.
Verify no legacy white/checker/textured/orange probes or old singleton
Dynamic Cyan Box return.

### Canonical target

After intentional migration: - Platforms = 6 - Checkpoints = 2 - Hazards
= 2 - Collectibles = 3 - Dynamic Boxes = 0 preferred - FOV = 40

Run `git diff --check` and inspect the exact `level_01.level` diff. The
intentional legacy dynamic_box removal must not be hidden by
`git restore`.

### Documentation

Update README, AGENTS, architecture, milestones and LEVEL_FORMAT_V1 docs
as appropriate. Document repeated Dynamic Boxes, mass units/bounds,
shared body budget, authored-vs-runtime pose, reset semantics,
lifecycle/editor support and legacy migration.

### Non-goals

No spheres/capsules/cylinders, arbitrary convex or dynamic GLB
collision, friction/restitution editor, density calculation, joints,
ropes, ragdolls, grab/carry mechanic, physics-layer UI, prefabs, GUIDs,
undo/redo or Level Format v2 without explicit review.

### Completion

M45 requires repeatable authored Dynamic Boxes, validated mass/size,
shared body capacity, editor lifecycle/placement, real Jolt bodies,
player pushing, runtime-pose rendering, authored-pose saving,
deterministic restart/reload, no legacy probe regression, passing
tests/builds and manual acceptance.

### Stop

After implementation, automated validation and report: STOP.

Do not commit, push, merge or start M46. M45 remains incomplete until
explicit manual approval.
