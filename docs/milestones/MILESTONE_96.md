# Milestone 96 --- Terrain Grass / Ground Cover Foundation

## Status

**Implemented, awaiting manual acceptance**

## Objective

Introduce the first dedicated high-density Terrain grass / ground-cover
authoring and rendering system. Grass/ground cover is a separate Terrain
domain rather than ordinary M93--M95 vegetation. M96 establishes compact
spatial authoring, deterministic placement, scalable rendering, Terrain
following, editor workflow, asset integration, and Level Format v1
compatibility.

## Background

M93--M95 established discrete static-model vegetation, deterministic
distribution, GPU instancing, and a practical editor workflow. Grass has
fundamentally higher density and should not become thousands of ordinary
vegetation-model instances. M96 therefore adds a purpose-built domain
while reusing proven Terrain, brush, lifecycle, asset, shader, and
instancing concepts where appropriate.

## Scope

### Separate Domain

Add a Terrain category dedicated to Grass / Ground Cover. Keep: -
Vegetation = discrete static-model placement. - Grass/Ground Cover =
dense Terrain surface detail.

Do not encode grass as M93 vegetation occupancy.

### Ground-Cover Palette

Support a small bounded ordered palette. Entries have stable authored
identity and editor-facing identification/preview. Choose/document the
smallest practical limit justified by Level Format v1 and renderer
constraints.

### Foundation Representation

Implement an inexpensive representation suitable for high counts, such
as crossed cards, simple blades/tufts, or equivalent reusable geometry.
Support at least appearance asset/texture or equivalent identity,
size/width and height variation, and deterministic yaw/variation. Do not
implement full procedural grass simulation.

### Compact Spatial Authoring

Use a dedicated density/weight mask independent from the M93 vegetation
grid. Never serialize individual blades/tufts. Stay within Level Format
v1 limits: 64 KiB, 256 lines, 512 chars/line.

### Paint / Erase

Provide selected-entry Paint/Erase with Radius and appropriate
Density/Strength plus necessary future-paint size/variation controls.
Preserve Next-Paint semantics: changing controls without painting must
not reinterpret existing authored regions.

### Brush Preview and Input Safety

Reuse Terrain brush-preview conventions. Preview follows valid Terrain
hit and Radius, remains transient, and disappears appropriately.
Inspector/picker interaction must not paint through UI.

### Deterministic Distribution

Generate instances deterministically from authored masks/settings.
Identical authored data + seed + Terrain produces stable output across
reload. Do not serialize transforms. Avoid distracting grid
rows/columns, applying M94 lessons.

### Terrain Following

Derived ground cover follows current Terrain height and appropriate
surface orientation. Sculpting updates derived placement without
rewriting authored masks.

### Scalable Rendering

Use a high-instance-count rendering path. GPU instancing/batching is
expected unless repository inspection supports a better existing path.
Never use one ordinary CPU model draw per blade/tuft. Reuse compatible
resources and provide diagnostic/test seams for instance/submission
counts.

### Lighting

Integrate coherently with current world lighting without regressing
Terrain, vegetation, props, Player, directional shadows, or local
lights. A foundation may intentionally omit grass shadow casting if
appropriate for the current renderer, but that must be an explicit
documented policy, not accidental behavior.

### Editor UX

Follow M95 quality: clear palette, selected state, Add/Remove, Brush,
Next Paint, subordinate diagnostics, empty/full/missing states. Reuse
Content Browser/catalog/thumbnail infrastructure where applicable. The
Add Ground Cover Texture picker lists cutout-compatible runtime PNGs
only and uses one selectable card covering the thumbnail and name.

### Lifecycle

Preserve `workingCopy → Apply → active → Save`. Palette and Paint/Erase
modify workingCopy. Apply promotes; Save persists. Derived transforms
remain transient.

### Cook / Stage

All referenced runtime assets participate in dependency discovery and
staging/cooking. Release must not depend on editor-only source
resolution.

### Delete Guard

Referenced Content Browser assets must remain protected from physical
deletion across workingCopy, active, and saved baseline according to
established semantics.

### Level Format v1

Keep `PLATFORMER_LEVEL 1`. Add only minimal deterministic
backward-compatible records for palette/settings/masks. Old levels
without ground cover load unchanged. Do not introduce Level Format v2.

## Required Regression Coverage

Cover at minimum: 1. old levels load unchanged; 2. palette add/remove;
3. palette limit and duplicate policy; 4. Paint/Erase; 5. coexistence
where supported; 6. Next-Paint non-retroactivity; 7. workingCopy vs
active; 8. Apply; 9. Save/reload determinism; 10. Level Format v1
limits; 11. deterministic placement; 12. different seeds differ
deterministically; 13. Terrain height following; 14. sculpt updates
derived placement without rewriting masks; 15. high-density rendering
uses instancing/batching; 16. resource reuse; 17. cook/stage
dependencies; 18. delete guards; 19. M93--M95 vegetation remains
unchanged; 20. directional-shadow traversal remains correct; 21. UI
capture prevents accidental painting.

## Manual Acceptance

Verify separation from Vegetation; understandable palette; Paint/Erase;
Radius footprint; visibly different low/high density; no distracting
regular grid; Next-Paint behavior; Terrain sculpt following; practical
dense area; coexistence with M95 vegetation; coherent lighting;
directional shadows; deterministic Apply/Save/reload; Development; and
Release asset resolution.

## Out of Scope

No wind, interactive bending, grass physics/collision, per-blade
authoring, individual-instance selection, authored LOD tiers,
billboards/impostors, fauna/NPC/AI, biome procedural generation,
automatic slope/height/texture rules, terrain PBR redesign, triplanar
mapping, generalized foliage framework, GUIDs, prefabs, undo/redo, ECS
redesign, or Level Format v2.

## Implementation Guidance

Inspect current Terrain material/weight maps, M93--M95 vegetation, M94
instancing, M95 editor UX, renderer/shaders, Content Browser, cook/stage
code, and Level Format limits before choosing exact representation.
Prefer a dedicated compact authored mask plus deterministic derived
instances. Keep generation CPU-testable independently from rendering.

## Canonical Level Safety

Do not use or mechanically modify/normalize
`game/assets/source/levels/level_01.level` or `level_02.level` as test
fixtures. Use dedicated/temporary fixtures.

## Validation

Follow `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md`.

Run:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release

python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Run affected C++ tests including Terrain tests, TerrainVegetationTest,
ContentBrowserAssetLibraryTest, AuthoredLifecycleIntegrationTest,
LevelFileTest, LoadedModelMaterialsTest, LightingEnvironmentTest, and
new ground-cover tests.

Before reporting:

``` text
git branch --show-current
git status
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

## Completion Criteria

M96 is complete only when Grass/Ground Cover is a separate authoring
domain; compact Paint/Erase works; derived placement is deterministic;
dense rendering is scalable; Terrain following works; editor UX is
practical; lifecycle/cook/stage/delete guards are correct; old levels
remain compatible; Level Format v1 limits remain satisfied; existing
vegetation remains intact; automated validation passes; canonical levels
have no unintended changes; manual acceptance is approved; and Git
closure occurs afterward.

## Agent Stop Condition

After implementation, validation, and reporting, STOP. Do not commit,
push, merge, close M96, or start M97.
