# Milestone 86 --- Terrain Foundation

## Status

**IMPLEMENTED — awaiting manual acceptance, not CLOSED**

M85.4 is CLOSED.

-   Branch: `milestone/86-terrain-foundation`
-   Cursor: **Grok 4.6 High --- Fast OFF**

## Purpose

Introduce the first authored **Terrain** primitive as a small
deterministic foundation for later Terrain Editor milestones. M86 covers
the terrain data model, Level Format v1 persistence, runtime rendering,
Jolt collision, editor selection/inspection, lifecycle, staging and
Release. It does **not** implement terrain sculpting or painting.

## Core architecture

Support **at most one Terrain per Level**. Terrain is a Level/world
surface, not a repeatable prop category.

Conceptually:

``` text
TerrainSpec
├── enabled
├── origin
├── sizeX
├── sizeZ
├── resolutionX
├── resolutionZ
└── heights[]
```

Exact names follow repository conventions. Terrain is a regular XZ grid;
authored height is world Y. Define one exact origin convention and use
it consistently.

Heights must genuinely support non-flat terrain. The sample count is
exactly `resolutionX * resolutionZ`. Use strict finite-value, dimension,
resolution and allocation bounds. Choose exact defaults/bounds only
after inspecting Level Format limits, world scale, renderer and Jolt.

Adding Terrain creates a deterministic flat default. Old Levels do not
gain Terrain automatically.

## Level Format v1

Add deterministic singleton Terrain persistence. Select exact syntax
after repository inspection. Prefer a bounded header plus row/sample
records rather than one huge line, conceptually:

``` text
terrain <enabled> <originX> <originY> <originZ> <sizeX> <sizeZ> <resolutionX> <resolutionZ>
terrain_row <rowIndex> <h0> <h1> ... <hN>
```

This syntax is conceptual, not mandatory.

Requirements: old Levels remain valid; at most one Terrain; exact sample
count; deterministic ordering; no duplicate rows/samples; finite
numbers; strict bounds; malformed/incomplete Terrain rejected; Level v1
64 KiB/256-line/512-char limits preserved; deterministic roundtrip; no
version bump unless unavoidable. If unavoidable, STOP and report first.

## Rendering

Render Terrain as normal lit world geometry using existing
world-lighting architecture. Terrain receives Ambient, Directional and
Point/Spot lighting, receives Directional shadows, and casts Directional
shadows where consistent with the existing caster path.

Use deterministic triangle topology and normals derived from height
variation. Do not use constant +Y normals for non-flat terrain.

No Terrain-specific material system, splatting, layers, triplanar
mapping, PBR expansion or generic mesh framework in M86. Use a simple
deterministic default surface compatible with current rendering.

## Jolt collision

Terrain has real static gameplay collision. Prefer Jolt heightfield
support if it fits the regular authored grid/current integration;
otherwise use the narrowest safe static representation and document why.

Player and Dynamic Boxes must collide correctly. Ground/ray queries
should see Terrain where appropriate. Rendered and collision heights
must correspond. No runtime deformation. Apply/reload/transition must
not leak stale bodies/shapes.

## Editor

Show a singleton Terrain hierarchy entry only when Terrain exists.
Provide Add Terrain when absent. A second Terrain cannot be added.
Delete is supported; Duplicate is unavailable. Terrain cannot join
Authoring Groups.

Terrain is selectable through hierarchy and viewport. Prefer actual
surface intersection and preserve nearest-hit behavior so Terrain does
not steal nearer object selection.

Provide a focused Inspector for foundational properties such as Enabled
(if retained), Origin, horizontal dimensions and resolution diagnostics.
Do not expose raw height arrays as ordinary Inspector fields.

Be conservative with changing resolution: if it requires undefined
resampling semantics, make resolution creation-time/read-only in M86.

## Gizmos

Terrain is not a normal prop. Do not automatically grant
Translate/Rotate/Scale.

A narrow Translate of authored origin may be implemented if cleanly
supported, using existing snapping and Dirty semantics. No Terrain
Rotate. Avoid generic Scale if it ambiguously changes
dimensions/heights; prefer explicit Inspector dimensions.

## Authority and lifecycle

Preserve:

``` text
workingCopy
    ↓ Apply
active
    ↓
runtime rendering + Jolt
```

Terrain authoring edits workingCopy; semantic changes Dirty; no-op does
not. Apply validates/promotes and safely rebuilds derived
render/collision state. Save/reload preserves semantic authored data.

Add creates flat authored Terrain and marks Dirty. Delete removes
authored Terrain, safely clears selection, marks Dirty, and Apply
removes runtime resources. No Duplicate.

Restart/death/checkpoint do not mutate authored Terrain. Level
transitions must correctly handle Terrain→none, none→Terrain and Terrain
A→Terrain B without resource leakage. Play Again/Main Menu→Play use
fresh staged Level data. F2 must not duplicate GPU/Jolt resources or
leak authority.

## Release

Release renders and collides with Terrain using staged Level data only,
with no editor UI/gizmos and no source-directory dependency. Reuse
existing world shaders where possible.

## Canonical Level safety

Do not intentionally modify `game/assets/source/levels/level_01.level`
or `level_02.level`. Use fixtures/disposable Levels. Do not normalize
EOLs. If canonical migration is required, STOP and report first.

## Required automated coverage

Cover: absent/flat/non-flat Terrain; singleton enforcement; finite
values; exact sample count; bounds; malformed data; deterministic
equality/roundtrip; Level v1 limits; geometry vertex/index topology;
flat/non-flat normals; world positions; collision
creation/removal/rebuild; editor
Add/singleton/hierarchy/selection/picking/Inspector/Dirty/Delete;
Duplicate unavailable; no Group; Translate only if implemented;
lifecycle Apply/Save/reload/Restart/transitions/Play Again/Main
Menu/death/F2; lighting/shadows; Release.

Regress Ambient/Directional/Directional shadows, Point/Spot lighting,
M85.4 gameplay-linked lights, Static Props, Dynamic Boxes, Doors,
Pressure Plates, Item Pickups, Player, materials, gizmos/groups,
thumbnails/Preview.

## Manual acceptance

Use a disposable Level: 1. Add Terrain and confirm only one can exist.
2. Confirm flat Terrain renders. 3. Confirm Player stands/walks on it.
4. Confirm Dynamic Box collision. 5. Select from hierarchy and viewport.
6. Confirm nearer props remain pickable. 7. Edit supported foundational
properties. 8. Apply and verify render/collision rebuild. 9.
Save/reload. 10. Delete Terrain + Apply and verify render/collision
disappear. 11. Re-add Terrain. 12. Exercise a non-flat test fixture and
verify rendered shape matches collision. 13. Verify
Ambient/Directional/Point/Spot lighting. 14. Verify Directional shadows.
15. Verify M85.4 controlled local lights illuminate Terrain. 16.
Restart/death/checkpoint. 17. Transition Terrain↔no-Terrain. 18. Toggle
F2 repeatedly. 19. Test Release.

Manual acceptance is mandatory.

## Out of scope

No sculpt/raise/lower/smooth/flatten brushes; terrain paint; texture
splatting/layers; terrain PBR expansion; holes/caves/overhangs;
arbitrary mesh topology; multiple Terrains/tiles; chunk streaming;
quadtree/clipmaps/LOD; procedural generation/noise UI; erosion; foliage;
navmesh; water; biomes; runtime deformation; terrain gameplay triggers;
generic mesh editor; ECS/scene graph; GUIDs; Undo/Redo;
alignment/distribution; or M87 Terrain Editor features.

## Tuning backlog

Do not opportunistically implement unrelated Tuning items. If something
works correctly but has a cleaner non-urgent implementation, report it
as a Tuning candidate. Bugs, regressions, unsafe lifecycle/resource
behavior and spec mismatches remain Corrections.

## Validation

Run relevant C++ suites and standard Python validation including, as
applicable:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Build Debug, Development and Release with the standard Windows presets.

Before reporting run:

``` text
git branch --show-current
git status
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

## Cursor completion report

Report: files; authored representation; singleton representation;
defaults/bounds; exact Level syntax; parser/writer; sample organization;
geometry/topology; normals; rendering/lighting/shadows; Jolt
representation and correspondence;
hierarchy/selection/picking/Inspector/gizmos;
Add/Delete/Duplicate/Groups; workingCopy/Dirty/Apply/Save;
lifecycle/transitions/F2; Release/staging; regressions;
tests/builds/diff checks; Tuning candidates; deferred scope.

Then STOP. Do not commit, push, merge, start M87 or mark M86 CLOSED.
Wait for manual acceptance and separate Git closure.
