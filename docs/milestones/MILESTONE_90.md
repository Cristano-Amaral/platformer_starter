# Milestone 90 --- Terrain Material Painting

## Status

**IMPLEMENTED — awaiting manual acceptance**

Milestone 89 --- Content Browser Asset Library & Texture Import is
CLOSED. `main` is clean and synchronized.

## Branch

`milestone/90-terrain-material-painting`

## Recommended Cursor model

**Grok 4.6 High --- Fast OFF**

## Purpose

Add authored region-based material painting to Terrain. M90 evolves the
M88 single base Terrain texture into a bounded multi-material workflow
that consumes the M89 shared Texture catalog. The user selects imported
textures, adds them to a Terrain palette, and paints where they appear
using a deterministic brush similar to M87 Sculpt Mode.

Preserve working-copy preview, Apply/Save/reload, staging/Release,
Sculpt, collision, lighting and shadows. This is not a generic material
editor or vegetation milestone.

## User workflow

1.  Import textures through M89.
2.  Select Terrain.
3.  Add/select textures from the shared catalog as Terrain layers.
4.  Enter Paint Mode.
5.  Select Grass/Dirt/Sand/Rock/Cobblestone or any other texture (names
    have no semantic meaning).
6.  Adjust Radius and Strength.
7.  Paint directly on Terrain with LMB.
8.  See blended working-copy result immediately.
9.  Apply, Save/reload and run Release.

## Existing architecture

Preserve M87 Sculpt, M88 Terrain Materials, M89 Texture
catalog/import/Content Browser, Terrain geometry/normals/Jolt,
`workingCopy -> Apply -> active`, Level Format v1 limits, world-lit
rendering, Ambient/Directional/Point/Spot lights, M85.4 local-light
behavior, directional shadows, lifecycle, staging and canonical safety.
Do not duplicate SourceTextureCatalog or Runtime PNG infrastructure.

## Material layers

Introduce the smallest authored layer representation needed for
painting. Target up to **4 layers** unless repository/shader inspection
demonstrates a materially cleaner bounded limit. Layer 0 is the base
layer. Each layer references an existing `textures/<file>.png` identity
and the minimum mapping/tiling data needed to preserve M88 behavior.

No semantic Grass/Dirt/Rock enums. No generic project Material asset
system.

## M88 backward compatibility

Existing M88 Terrain with one texture/tiling remains valid and visually
equivalent, conceptually as layer 0 at full weight. Old Levels must load
without manual migration. No painted data means the M88 single-material
appearance. Do not rewrite canonical Levels to materialize defaults.

## Painted weights

Persist deterministic spatial material weights independently of heights.
Prefer weights tied to the existing Terrain regular sample/vertex
topology: one normalized material mixture per sample, interpolated
across Terrain geometry. Weights remain stable when heights change.
Terrain resolution remains read-only.

Avoid arbitrary-resolution splat textures unless current renderer
architecture strictly requires them. Weights must be finite, bounded,
deterministic and normalized.

Defaults: layer 0 = 1, others = 0. Adding a layer must not change
appearance until painted. Removing a used layer must redistribute
deterministically; prefer folding removed contribution into layer 0
unless inspection identifies a cleaner rule. Reordering is not required.

## Paint brush

Paint Mode intentionally resembles M87 Sculpt Mode: LMB paints, Esc
exits, Radius, Strength, visible Terrain footprint and clearly selected
layer. Reuse M87 ray-to-working-copy Terrain and deterministic
spatial-stroke concepts where appropriate.

Painting must be spatial, not frame-count based. Holding LMB stationary
must not continuously accumulate paint because FPS is high. Reuse
comparable deterministic stamp spacing after inspecting M87.

## Paint operation

Painting layer L increases its local contribution according to
Radius/falloff/Strength while decreasing other layers and preserving a
valid normalized mixture. Repeated strokes converge predictably toward
L. Strength controls rate; Radius controls extent. No NaN/Inf.

Document and test the exact formula. Reuse M87 linear falloff initially
unless a shared canonical brush helper already exists. Do not add
multiple falloff curves.

## Palette UI

When Terrain is selected, expose a compact Terrain Materials/Paint
section with: - assigned layers; - selected paint layer; - Add Layer
from M89 shared Texture catalog; - safe Remove Layer; - texture
identity/display name; - tiling where compatible with M88; - Paint
Mode; - Radius; - Strength.

Newly imported M89 textures must be available without restart. Prevent
duplicate identical layers unless there is a demonstrated need.

## Fallback

Terrain remains valid without optional painted layers. Preserve M88
deterministic solid fallback when no valid base texture exists. Missing
optional layer textures fail safely without corrupting weights or
substituting unrelated textures. Follow existing log-once/fallback
conventions.

## Working-copy authority

Layer assignment, tiling and painting modify `workingCopy`. Before
Apply, active Terrain is unchanged while Development previews current
geometry, palette and blends. Apply promotes through the established
path. Save semantics remain unchanged.

Paint Mode state, selected layer, Radius and Strength are transient and
do not Dirty authored data. Actual layer/weight changes do.

## Sculpt coexistence

Sculpt Mode and Paint Mode are mutually exclusive. Entering one exits
the other. Painting changes weights only; Sculpt changes heights only.
Painted regions remain associated with the same XZ/sample locations
while Raise/Lower/Smooth/Flatten change Y. Apply preserves both.

## Rendering

Extend the existing Terrain world-lit path minimally. Final Terrain
albedo is the normalized blend of assigned layer textures by painted
weights. Preserve Ambient, Directional, Point, Spot, M85.4 effective
local lights and directional shadow cast/receive. Do not create an unlit
or second lighting model.

## UVs

Preserve M88 XZ mapping, independent of Terrain Y. If tiling is per
layer: `u = (worldX - origin.x) * layerTiling`
`v = (worldZ - origin.z) * layerTiling` or exact equivalent required by
current architecture. Document final formula. Triplanar remains out of
scope unless already directly available and strictly required.

## GPU/resources

Reuse existing Texture/resource conventions. No per-frame
reload/recreation, leaks or stale resources. Layer resource count is
bounded by the layer limit. F2/Level transitions are safe. Release uses
staged runtime assets only. No new global asset manager.

## Level Format v1

Keep version 1 if possible. Persist assigned layers/identities, tiling
as required, and painted weights with deterministic strict
backward-compatible grammar respecting 64 KiB / 256 lines / 512 chars
per line.

Because Terrain max resolution is currently bounded, prefer compact
textual row records tied to Terrain samples rather than opaque
binary/base64. Validate exact counts, finite/bounded weights, layer
indices/counts, duplicate/missing records and
normalization/canonicalization rules.

Document exact grammar in `docs/LEVEL_FORMAT_V1.md`. If a version bump
is genuinely required, STOP and report instead of implementing it.

## Layer removal/remap

Removing a layer must leave no dangling index, preserve valid normalized
weights and compact/remap remaining indices deterministically. Deleting
a used layer follows the documented redistribution rule. Heights are
untouched.

## Dirty/no-op

No Dirty for: entering mode, selecting layer, changing Radius/Strength,
brush movement, painting outside Terrain, a no-effective-change stamp,
rejected duplicate layer, or same tiling. Effective layer add/remove,
tiling or weight changes modify workingCopy.

## Input

In Paint Mode, LMB paints working-copy Terrain and does not select/place
other objects. Outside Terrain = no paint. Esc exits. Pointer
capture/loss of selection/Terrain deletion/Apply lifecycle must be safe.
Outside Paint Mode, existing editor behavior remains.

## Cooker/staging/Release

All valid textures referenced by assigned Terrain layers participate in
existing dependency discovery/cooking/staging. Do not stage all Content
Browser textures, Favorites or folders. Release renders the same painted
Terrain from staged assets without Development metadata/source paths.

## M89 integration

Consume the M89 shared Texture catalog; do not add another Texture
browser. Logical folders/Favorites do not alter identities. Update the
loaded-Level physical Texture delete guard so it recognizes every
Terrain layer reference, not only the old M88 base texture. Do not build
a project-wide dependency graph.

## Canonical safety

Do not intentionally modify or normalize: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

Use disposable fixtures. Both diffs must be empty before completion.

## Automated tests

Cover at minimum: - default weights; - add layer no visual/weight
change; - duplicate rejection; - paint selected layer; -
Radius/Strength/falloff; - normalization; -
deterministic/repeated/spatial strokes; - stationary held input not
FPS-dependent; - no NaN/Inf; - outside Terrain no-op; - paint then
Sculpt preserves weights; - Sculpt then paint preserves heights; - Apply
preserves both; - remove unused/used layer and remap; - missing layer
texture fallback; - GPU add/remove/change lifecycle and
F2/transitions; - old M86/M87 Terrain parsing; - old M88 single-material
parsing/equivalence; - new layers/weights roundtrip and deterministic
writer; - invalid counts/indices/weights/rows; - workingCopy does not
mutate active before Apply; - no-op Dirty behavior; - mode mutual
exclusion/lifecycle; - bounded renderer resources; - M88 UV behavior; -
lighting/shadow regressions; - dependency discovery for all layers; -
Texture delete guard for layers; - dependency-driven staging/Release; -
M87/M88/M89 regressions.

## Manual acceptance

Verify: 1. Use at least four distinct imported textures. 2. Create
Terrain palette. 3. Base-only appearance matches M88. 4. Add second
layer; appearance unchanged before painting. 5. Paint a small region. 6.
Increase Radius and paint larger region. 7. Change Strength and verify
blending rate. 8. Verify blended rather than unexpected hard edges. 9.
Paint third material elsewhere. 10. Paint over an existing blend. 11.
Hold LMB stationary; no FPS-driven accumulation. 12. Paint at different
mouse speeds; spatial coverage remains consistent. 13. Esc exits. 14.
Sculpt painted regions with Raise/Lower/Smooth/Flatten; materials remain
attached. 15. Return to Paint and continue. 16. Sculpt/Paint are
mutually exclusive. 17. Working-copy preview before Apply. 18. Apply.
19. Player/Dynamic Box collision unchanged. 20. Lighting and directional
shadows remain correct. 21. Save/reload preserves
layers/tiling/weights/heights. 22. Remove unused layer. 23. Remove used
layer and verify redistribution. 24. Duplicate layer is safely
rejected/no-op. 25. Content Browser refuses deletion of a texture
referenced by any Terrain layer. 26. Content Browser folders/Favorites
still work. 27. F2/Level transitions have no stale resources. 28.
Release renders same painted Terrain. 29. Old M88-only Terrain remains
valid. 30. No-texture Terrain fallback remains valid.

## Validation

Run:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run relevant C++ suites across M85, M87, M88, M89 and new M90 coverage.

Run applicable Python tests:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Also run relevant shader/staging tests.

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

Automatic slope/height material rules; semantic material enums;
unbounded layers; arbitrary high-resolution splat maps unless strictly
required; multiple falloff types; decals; generalized eraser;
speculative triplanar; PBR expansion; normal/roughness/metalness/AO
Terrain layers; vegetation/foliage; grass/tree scattering; fauna;
Terrain resolution editing/resampling; Terrain
tiles/LOD/chunks/streaming; procedural biomes; runtime painting;
Undo/Redo; generic material editor; GUID asset database; ECS migration;
generalized command framework; Content Browser drag-and-drop;
editor-wide help-icon cleanup; Player Traversal Path; M91+ features.

## Implementation discipline

Inspect repository code/tests/docs before choosing exact layer structs,
bounded layer limit, weight storage, shader bindings, Level Format
grammar, palette UI, brush reuse, removal redistribution and resource
ownership.

Prefer the smallest coherent extension of M87/M88/M89. Do not implement
speculative future Terrain systems or a generalized asset/material
framework. If four layers are impractical because of established shader
limits, choose the smallest justified bounded alternative and report it.
If Level Format v1 cannot remain within hard limits, STOP and report.

## Cursor completion report

Report: 1. files added/changed; 2. exact Terrain layer representation;
3. layer limit and rationale; 4. M88 backward compatibility; 5. weight
representation/defaults/normalization; 6. paint formula; 7.
Radius/Strength/falloff/stamp spacing and bounds; 8. palette UI; 9.
Sculpt/Paint exclusion; 10. workingCopy/Apply/Dirty; 11. exact Level
Format grammar; 12. remove/remap/redistribution; 13. shader/render
integration; 14. UV/tiling; 15. GPU resources; 16. M89 Texture catalog
integration; 17. Texture delete guard; 18. cooker/staging/Release; 19.
lifecycle/F2/transitions; 20. tests/results; 21.
Debug/Development/Release builds; 22. Python results; 23. canonical
Level diffs; 24. legitimate Tuning Backlog candidates; 25. intentionally
deferred items.

Then STOP. Do not commit, push, merge, close M90, or begin M91. Manual
acceptance and Git closure remain separate steps.

## Correction 2 (functional, not closed)

Manual acceptance after Correction 1 still failed because extra-layer
albedo sampled the directional shadow depth map. raylib `DrawMesh`
binds `MATERIAL_MAP` index `i` to texture unit `i` and writes sampler
uniforms `texture0..N`, so `texture1` aliased `shadowMap` on unit 1.
Extra layers now use dedicated `terrainAlbedo1..3` samplers on units
5/6/7. Layer 0 catalog identities that are not yet cooked/staged may
preview from source in Development only. Materials/Paint Inspector
separates the material palette from the Paint Tool. M90 remains
implemented, awaiting manual acceptance. Do not begin M91.
