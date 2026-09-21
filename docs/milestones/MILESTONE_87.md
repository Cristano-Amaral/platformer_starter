# Milestone 87 --- Terrain Sculpting

## Status

**CLOSED**

M87 begins only after M86 is CLOSED and `main` is clean and
synchronized.

## Branch

`milestone/87-terrain-sculpting`

## Recommended Cursor model

**Grok 4.6 High --- Fast OFF**

## Purpose

Turn the M86 Terrain foundation into the first directly editable
terrain-shaping workflow in the Development Editor. Add focused viewport
sculpting that edits the existing authored Terrain height samples with
Raise, Lower, Smooth, and Flatten brushes while preserving editor
authority, deterministic persistence, renderer/Jolt correspondence,
lifecycle safety, and Release behavior.

This milestone is height sculpting only. It does not add Terrain
materials, texture painting, vegetation, foliage, LOD, streaming,
procedural Terrain, fauna, or a generalized mesh editor.

## Existing M86 foundation to preserve

Reuse the existing singleton optional Terrain and its `TerrainSpec`,
regular XZ grid, `heights[]`, Level Format v1 `terrain`/`terrain_row`
persistence, CPU geometry/normals, world-lit rendering, directional
shadows, static Jolt collision, hierarchy/selection/picking/Inspector,
Translate support, and `workingCopy -> Apply -> active` authority.

Do not create a second Terrain representation or another authoritative
height source.

## User-facing result

With Terrain present, the user can enter Terrain Sculpt mode and drag a
brush directly on the Terrain surface.

Required operations: - **Raise** --- increases affected authored
samples. - **Lower** --- decreases affected authored samples. -
**Smooth** --- blends affected samples toward deterministic
neighborhood-smoothed values. - **Flatten** --- blends affected samples
toward one target height captured at stroke begin.

Expose at minimum: - Sculpt mode on/off; - brush operation; - brush
radius; - brush strength; - clear viewport brush footprint/preview.

Expected workflow:
`Terrain -> Sculpt -> workingCopy heights change -> Dirty -> Apply -> active render/collision rebuild -> Save -> Reload -> same relief`

## Terrain-only scope

Sculpting applies only to the existing singleton Terrain. If no Terrain
exists, sculpt controls are unavailable/inactive.

Terrain remains a singleton, non-groupable, non-duplicable authored
world surface. Do not introduce IDs, GUIDs, ECS migration, generic
editable meshes, or multiple Terrain tiles.

## Authored data

Brushes modify only `workingCopy.terrain.heights`. Do not add sculpt
maps, runtime deformation buffers, hidden authoritative files, or
editor-only authoritative height storage.

All resulting samples must obey the existing M86 validation/bounds.

Brush parameters are transient editor/tool state and are not serialized
into the Level.

## Resolution

Keep M86 resolution semantics and limits unless a narrowly scoped
correctness issue is proven. Resolution remains creation-time/read-only
in M87.

Do not add resampling, subdivision, adaptive tessellation, or topology
editing.

## Ground compatibility

Preserve M86 Ground/Terrain behavior exactly. Do not remove Ground,
migrate existing Levels, or introduce a new Ground/Terrain exclusivity
policy. Sculpt affects Terrain only.

## Sculpt hit testing

Derive the brush cursor from the Terrain surface under the viewport ray.

During sculpting, hit testing must use the **current working-copy
Terrain geometry**, so the brush follows unapplied relief edits. Do not
depend on active Jolt collision for sculpt placement.

Unrelated props/Ground/physics bodies must not become the authoritative
sculpt surface. Normal editor picking outside Sculpt mode remains
unchanged.

## Brush footprint and falloff

Operate in world-space XZ around the Terrain hit position. Radius is in
world units and remains meaningful when Terrain size changes. Samples
outside the radius are unchanged.

Use a simple deterministic distance-based falloff, preferably linear
unless repository conventions justify another simple rule. Document the
exact formula.

## Stroke semantics

A stroke begins on viewport press, continues while dragging, and ends on
release.

Avoid frame-rate-dependent authoring. Prefer deterministic spatial brush
stamping based on cursor travel rather than accumulating deformation
every rendered frame while the mouse is stationary.

Inspect current input/editor architecture and implement the smallest
robust stroke-spacing rule. Document and test the exact rule.

Do not add Undo/Redo.

## Raise

Increase each affected sample according to strength and falloff. Clamp
to existing Terrain height bounds.

## Lower

Symmetric to Raise, decreasing each affected sample according to
strength and falloff. Clamp to existing Terrain height bounds.

## Smooth

Use deterministic neighborhood data from the **pre-stamp** height state,
so mutation order cannot change the result.

A small fixed neighborhood appropriate to the regular grid is
sufficient. Strength/falloff blends the current sample toward the
computed smooth target. Document the exact neighborhood/math.

## Flatten

At stroke begin, capture one target **world-space Terrain Y** from the
initial hit point/surface. Keep that target fixed for the whole stroke.

Affected samples blend toward that target according to strength/falloff
rather than snapping unconditionally. Convert consistently to the
existing origin-relative height representation.

A new stroke may capture a new target. Do not persist a flatten-height
property.

## Brush parameters

Choose practical defaults/bounds after inspecting current world scale
and UI conventions.

At minimum: - radius is finite, positive, and bounded; - strength is
finite, positive, and bounded; - invalid values never reach authored
Terrain data.

Changing Sculpt mode, operation, radius, or strength does not Dirty the
Level.

## Working-copy preview and authority

Mandatory authority: `workingCopy -> Apply -> active -> runtime`

While sculpting in edit mode: - visually preview working-copy relief; -
preview normals reflect working-copy heights; - active authored Terrain
remains unchanged until Apply; - active Jolt/runtime collision must not
silently mutate from unapplied strokes.

On Apply: - validate working copy; - promote to active; -
synchronize/rebuild active Terrain GPU resources as needed; -
rebuild/synchronize Jolt Terrain collision from the same promoted
heights.

On Save, persist according to existing editor semantics and
deterministic `terrain_row` output.

Runtime state never writes back into authored data.

## GPU/resource behavior

Development preview may refresh while working-copy heights change, but
must remain bounded and lifecycle-safe: - no resource leak across
strokes; - no duplicate Terrain mesh; - no unbounded allocation each
frame; - no rebuild when nothing changed; - no Release dependency on
editor preview code.

Reuse M86 Terrain geometry generation. If needed, add only a small
Terrain-specific preview cache/revision mechanism.

## Jolt

Unapplied sculpt edits do not replace active runtime collision. After
Apply, collision must match the promoted sculpted geometry.

Validate Player and Dynamic Box collision, repeated Apply,
delete/reload/transitions, and no accumulated Terrain bodies.

No runtime Terrain deformation.

## Normals, lighting, shadows

Sculpted relief regenerates correct normals and continues to work with
Ambient, Directional, Point, Spot, M85.4 effective local-light behavior,
and directional shadow casting/receiving.

Do not add Terrain-specific lighting or a new Terrain shader unless
proven strictly necessary.

## Selection, hierarchy, gizmos

Preserve M86 selection/hierarchy behavior. Sculpt mode must avoid
accidental selection churn during strokes.

Terrain remains translatable through existing supported behavior, not
rotatable, not generically scalable/resizable, non-groupable, and
non-duplicable.

Sculpt is not a generic object gizmo.

## Dirty/no-op semantics

These do **not** Dirty: - enter/exit Sculpt mode; - change operation; -
change radius/strength; - move brush preview; - stamp/stroke that
produces no semantic height change.

Any operation that changes at least one `workingCopy` height marks
Dirty.

Avoid redundant Dirty transitions/resource rebuilds for semantic no-ops.

## Persistence

Sculpted Terrain must survive Apply, Save, reload, normal editor restart
workflow, staging, and Release.

Level Format remains v1. Existing M86 Terrain files remain valid. Do not
change Terrain syntax unless unavoidable for correctness.

If a Level Format version bump appears necessary, **STOP and report
instead of implementing it**.

## Lifecycle

Validate: - Add Terrain -\> sculpt -\> Apply; - multiple strokes -\>
Apply; - sculpt -\> Save -\> reload; - sculpt -\> Apply -\> Delete -\>
Apply; - delete -\> re-add -\> sculpt -\> Apply; -
Restart/death/checkpoint; - repeated F2 cycles; - Terrain -\> no-Terrain
Level; - no-Terrain -\> Terrain; - Terrain A -\> Terrain B; - Play
Again/Main Menu where applicable; - Development shutdown/reload; -
staged Release.

No stale mesh, preview, collision body, brush state, or cross-Level
authored leakage.

## Canonical Level safety

Do not intentionally modify: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

Do not normalize their line endings. Use disposable/test fixtures for
sculpted Terrain validation.

## Automated tests

Add focused regression coverage for: - Raise only affects in-radius
samples and increases them; - Lower only affects in-radius samples and
decreases them; - deterministic falloff; - height-bound clamping; -
Smooth determinism and independence from mutation order; - Flatten fixed
target per stroke; - no-op stamp semantics; - brush parameters not
authored/persisted; - working-copy sculpt does not mutate active before
Apply; - Apply promotes sculpted heights; - geometry/normals reflect
sculpted heights; - Jolt collision after Apply reflects sculpted
heights; - save/parse roundtrip preserves sculpted samples; - existing
M86 Terrain format compatibility; - editor selection/picking
regression; - repeated Apply/F2/lifecycle does not duplicate
resources/bodies; - M85/M85.4 lighting regressions remain green; -
existing M86 Terrain tests remain green.

If UI interaction is difficult to unit-test, extract only the smallest
pure Terrain sculpt math/state helpers needed for deterministic tests.
Do not create a generalized command framework.

## Manual acceptance

Before closure manually verify: 1. Add/select Terrain and enter Sculpt
mode. 2. Brush preview follows current Terrain surface. 3. Raise creates
visible relief. 4. Lower creates a visible depression. 5. Smooth softens
an irregular area. 6. Flatten moves an area toward one consistent stroke
target. 7. Radius changes affected area. 8. Strength changes deformation
magnitude. 9. Dragging is continuous/predictable without obvious
frame-rate accumulation. 10. Preview movement alone does not Dirty; real
sculpt does. 11. Working-copy preview works before Apply without
corrupting active/runtime authority. 12. Apply updates active render and
Jolt collision. 13. Player walks/stands on sculpted relief. 14. Dynamic
Box collides/rests on sculpted relief. 15. Lighting, normals, shadows,
Point/Spot, and Pressure Plate-controlled local lights remain correct.
16. Save/reload preserves relief. 17. Delete/Apply removes visual
Terrain and collision. 18. Re-add/sculpt/Apply works without duplicates.
19. F2/restart/transitions do not leak/reset/duplicate Terrain state.
20. Release renders and collides with saved sculpted Terrain.

## Required validation

Run:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run all directly relevant C++ tests, including M86 Terrain
geometry/collision/parser/editor/lifecycle and M85/M85.4 lighting
regressions.

Run applicable Python tests, including:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Run existing shader/staging tests relevant to changed paths.

Before reporting:

``` text
git branch --show-current
git status
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

## Out of scope

Do not implement: - Terrain material painting; - splat/weight maps or
material layers; - Terrain texture authoring; -
grass/trees/vegetation/foliage/scatter painting; - fauna/animals; -
resolution resampling/subdivision; - multiple Terrains/tiles; -
chunks/streaming/quadtree/clipmaps/LOD; -
holes/caves/overhangs/arbitrary topology; - procedural noise/erosion; -
navmesh/pathfinding; - water/biomes; - runtime Terrain deformation; -
Undo/Redo; - generic mesh editing; - ECS/GUID migration; - generalized
command framework; - M88+ features.

## Implementation discipline

Repository code, tests, current docs, and this active milestone are the
source of truth.

Inspect the repository before choosing exact names, UI placement, input
handling, defaults, and helper boundaries. Prefer the smallest
implementation that cleanly extends M86.

Do not opportunistically refactor unrelated systems or start Terrain
Materials/Vegetation work.

If implementation reveals a conflict with the M86 contract or requires a
Level Format version bump, STOP and report.

## Cursor completion report

Report: - files added/changed; - exact Sculpt UI/workflow; - brush
defaults/bounds; - falloff formula; - stroke/stamp spacing and
frame-rate-independence strategy; - Raise/Lower/Smooth/Flatten math; -
working-copy preview behavior; - Apply/Jolt/GPU synchronization; -
Dirty/no-op behavior; - lifecycle handling; - tests and results; -
Debug/Development/Release build results; - Python results; - canonical
Level diff status; - legitimate tuning candidates; - intentionally
deferred items.

Then **STOP**.

Do not commit, push, merge, close M87, or begin M88. Manual acceptance
and Git closure remain separate user/ChatGPT steps.
