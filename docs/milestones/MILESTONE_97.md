# Milestone 97 --- Terrain Rendering / Material Visual Upgrade

## Status

**Implemented, awaiting manual acceptance**

## Objective

Upgrade Terrain material visual quality while preserving the authored
Terrain, scalable material palette/weight maps, Vegetation, Ground
Cover, lighting, physics, and workingCopy lifecycle established through
M96.

M97 extends Terrain layers from albedo-only appearance to a bounded
material channel set with optional Normal and Roughness maps, correct
multi-layer blending, meaningful lighting response, practical editor
assignment, runtime asset handling, and backward-compatible Level Format
v1 serialization.

This is a Terrain-specific visual upgrade, not an engine-wide PBR
rewrite.

## Background

The Terrain stack now includes geometry/collision (M86), sculpting
(M87), albedo materials (M88), material painting (M90), scalable
palette/weight maps (M92), discrete Vegetation (M93--M95), and
Grass/Ground Cover (M96).

Terrain supports up to 16 material layers and four RGBA weight maps, but
its material appearance is primarily albedo-driven. The surrounding
authored scene now justifies improving the Terrain surface itself.

## Scope

### Material Channels

Each Terrain material layer supports: - existing Albedo/Base Color; -
optional Normal; - optional Roughness.

Missing channels use deterministic neutral defaults: flat tangent-space
normal and a documented default roughness.

Do not add metallic, AO, emissive, height/displacement, clearcoat, or
other PBR channels in M97.

### Existing Palette and Weight Maps

Preserve the ordered M92 palette and weight maps. Normal/Roughness
identities belong to the same layer as its albedo and use the same paint
weights. Do not create independent paint layers for channels.

### Level Format v1

Keep `PLATFORMER_LEVEL 1`. Old albedo-only levels load unchanged. Add
only minimal deterministic optional-channel records/fields. Preserve 64
KiB, 256-line, and 512-character-per-line limits. Do not rewrite
weight-map records or introduce v2.

### Asset Integration

Reuse SourceTextureCatalog, RuntimePng, Content Browser thumbnails,
cook/stage dependency discovery, and Development/Release resolution.
Assignment must use asset pickers rather than manual path typing.

### Editor UX

For the selected Terrain material layer expose Albedo, Normal,
Roughness, and existing Tiling. Optional channels support
Assign/Replace, Clear, identity/name, useful preview, and missing-asset
state. Reuse M95/M96 picker quality: clickable cards/thumbnails, search,
and input-capture safety.

### Compatibility

Do not claim every PNG is semantically a Normal or Roughness map merely
because it is a PNG. Inspect current assets/metadata and use the
smallest Terrain-specific compatibility convention needed. Do not create
a generalized tagging/material database.

### Terrain Shading

Blend albedo, tangent-space normal, and roughness using the existing M92
layer weights. Normal results must be normalized and stable. Add only
the minimum Terrain tangent-space basis required by the current XZ UV
mapping.

### Lighting

Preserve ambient, directional, point, spot, and directional-shadow
receiving. Roughness must have a meaningful bounded effect on Terrain
light response. A small Terrain/world-lit specular response is in scope
if required. Do not perform an engine-wide PBR conversion.

### Multi-Layer Correctness

Preserve up to 16 layers and four RGBA weight maps. Layer0 fallback,
no-paint 100% layer0, layer removal/remapping, and existing duplicate
policy remain correct. Do not reduce M92 capacity.

### GPU Resources

Preserve scalable array-based resources where practical.
Normal/Roughness must not require one sampler per material layer.
Document texture units/samplers and verify no collisions with shadows,
vegetation, Ground Cover, or other world_lit resources.

### Resolution Policy

Define deterministic runtime/cook behavior for channel textures with
differing source resolutions, including target dimensions/filtering and
neutral fallback representation.

### Lifecycle

Preserve `workingCopy → Apply → active → Save`. Assign/Clear/Replace
modifies workingCopy; Apply promotes; Save persists identities/settings.

### Delete Guards

Normal/Roughness references in workingCopy, active, or saved baseline
block physical deletion according to established behavior.

### Cook / Stage

All channel dependencies are discovered, cooked, and staged. Development
follows established resolution policy. Release uses staged/cooked
runtime assets only.

### Existing Systems

Do not regress Terrain geometry/sculpt/collision/painting, M92
palette/weights, M93--M95 Vegetation, M96 Ground Cover, directional
shadow follow, local lights, editor selection, or lifecycle.

## Required Regression Coverage

Cover old albedo-only compatibility; neutral fallbacks; Normal/Roughness
assign/clear/save/reload; missing assets; lifecycle; delete guards;
cook/stage; Development and Release resolution; multi-layer channel
blending; layer0/no-paint behavior; removal/remap; full palette; sampler
collisions; stable normalized normal blending; default roughness;
directional shadows; local lights; Vegetation instancing; Ground Cover
instancing/cutout; Level Format v1 limits; and picker input capture.

Use robust graphics/readback tests where an existing OpenGL seam is
appropriate; avoid fragile screenshots.

## Manual Acceptance

Verify an old albedo-only level remains sensible; Normal maps visibly
add lighting detail without geometry change; Clear restores neutral
behavior; Roughness produces visible response differences; multiple
painted layers blend all channels coherently; no invalid seams/normals;
picker UX/input capture; missing maps; Apply/Save/reload; sculpting;
Vegetation; Ground Cover; directional shadows; local lights;
Development; and Release staged/cooked resolution.

## Out of Scope

No displacement/height mapping, parallax, tessellation, triplanar
mapping, metallic/AO/emissive/clearcoat/subsurface, full engine-wide
PBR, static-model material-editor redesign, generalized automatic
texture-set discovery, procedural biomes, macro/micro texture system,
virtual texturing, streaming, Terrain LOD redesign, unrelated
vegetation/wind changes, GUIDs, prefabs, undo/redo, ECS redesign, or
Level Format v2.

## Implementation Guidance

Inspect current Terrain shaders/resources, M92 texture arrays/sampler
units, world_lit lighting, RuntimePng cook/stage, Content Browser picker
patterns, M96 texture compatibility work, and Level Format serialization
before selecting the exact representation.

Prefer the smallest Terrain-specific extension that makes Normal +
Roughness useful. Keep material/resource planning CPU-testable
independently from GL submission where practical.

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

Run affected C++ tests including TerrainMaterialTest, TerrainPaintTest,
TerrainGeometryTest, TerrainSculptTest, TerrainVegetationTest,
TerrainGroundCoverTest, ContentBrowserAssetLibraryTest,
AuthoredLifecycleIntegrationTest, LevelFileTest,
LoadedModelMaterialsTest, LightingEnvironmentTest,
WorldLightingResourcesTest, and affected graphics/resource tests.

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

M97 is complete only when optional Normal/Roughness channels work per
Terrain layer; old content remains valid; multi-layer blending and
lighting are correct; GPU resource usage scales to the M92 palette;
editor assignment is practical; cook/stage/delete guards are correct;
existing systems remain intact; Level Format v1 remains valid; automated
validation passes; canonical levels have no unintended changes; manual
acceptance is approved; and Git closure occurs afterward.

## Agent Stop Condition

After implementation, validation, and reporting, STOP. Do not commit,
push, merge, close M97, or start M98.
