# Milestone 92 --- Scalable Terrain Material Palette & Weight Maps

## Status

**IMPLEMENTED — awaiting manual acceptance**

## Branch

`milestone/92-scalable-terrain-material-palette`

## Purpose

Evolve the Terrain material system introduced by M88/M90 from a
topology-bound four-layer RGBA vertex-weight representation into a
scalable authored material palette backed by dedicated Terrain weight
maps.

The Level Designer should no longer be architecturally limited to four
Terrain materials. A Terrain may contain an expandable palette of
material layers such as Grass, Dirt, Rock, Sand, Mud, Cobblestone, and
future materials, while the renderer uses packed RGBA weight maps
internally.

M92 is an architectural migration of Terrain material authoring and
rendering. It must preserve the established M90 painting workflow and
existing Levels.

## Motivation

M90 intentionally established regional Terrain painting with at most
four layers: R/G/B/A map to layers 0/1/2/3, and weights are stored per
Terrain geometry sample/vertex.

M92 removes two long-term constraints:

1.  the visible material palette is capped at four layers;
2.  paint resolution is tied to Terrain geometry resolution.

Do not introduce vegetation, PBR expansion, procedural biome painting,
or unrelated Terrain features.

## Core Architecture

### Material Palette

A Terrain owns an ordered material palette. Each layer contains at
minimum texture identity and texture tiling. Layer 0 remains the
base/fallback material.

The palette must support more than four layers. Do not retain a
user-visible architectural four-layer limit. A practical implementation
limit may exist for memory/GPU safety, but it must be substantially
larger than four, documented, validated, and not be a consequence of one
RGBA vector.

Duplicate texture identities remain disallowed within the same Terrain
palette.

### Dedicated Weight Maps

Terrain paint weights are no longer stored as one RGBA tuple per
geometry sample.

Use dedicated authored weight/splat maps with RGBA packing:

-   weight map 0 stores layers 0--3;
-   weight map 1 stores layers 4--7;
-   weight map 2 stores layers 8--11;
-   continue as required by the palette.

The number of maps is derived from the palette/layer count. Weight-map
resolution is authored independently from Terrain geometry resolution.

### Weight Resolution

Choose a conservative default and validated bounds suitable for the
current engine and Level Format constraints.

The default should provide visibly finer painting than low-resolution
Terrain geometry where practical without excessive Level-file size,
upload cost, or editor latency.

Do not silently tie weight resolution back to Terrain
`resolutionX`/`resolutionZ`.

### Weight Invariants

At every weight texel, weights across all active layers must be finite,
non-negative, normalized, and deterministic. Zero/invalid total weight
resolves to layer 0 = 1 and all others = 0. Unused packed channels
behave as zero.

Quantized serialization must preserve a deterministic normalized
representation.

## Backward Compatibility

M86/M87/M88/M90 Levels must continue to load.

For old M90 Terrain, preserve its palette and deterministically
convert/interpolate old per-geometry-sample weights into the new
dedicated weight representation while preserving the visual result as
closely as practical. Terrain without explicit paint remains 100% layer
0.

Saving an old Level may migrate it to M92 representation, but migration
must be deterministic and documented.

Prefer a backward-compatible Level Format v1 extension rather than
changing format version solely for M92.

## Level Format

Add deterministic textual representation for scalable palette layers,
weight-map dimensions, and packed/quantized weight data.

Requirements:

-   remain Level Format v1 compatible where practical;
-   no opaque binary blobs or base64;
-   deterministic writer;
-   strict parser validation;
-   bounded file size;
-   clear malformed-input errors;
-   old `terrain_material`, `terrain_layer`, and `terrain_paint` remain
    readable.

Follow current parser/writer conventions discovered in the repository.

The existing 64 KiB / 256-line Level constraints remain authoritative.
If naive text representation is too large, design compact deterministic
textual encoding within those constraints rather than casually weakening
them.

## Painting Semantics

Preserve the M90 Paint Mode model: select Terrain; Materials & Paint;
select Paint Layer; Radius/Strength; LMB paints; Esc exits; footprint
preview; deterministic spatial stroke spacing; no stationary frame
accumulation; linear falloff; `workingCopy` preview; Apply promotes;
Save persists.

Painting a layer blends toward that layer while preserving normalized
weights across the full palette, not only within one RGBA map. Painting
layer \>= 4 must correctly reduce weights stored in other packed maps.

Sculpt preserves material weights. Paint preserves heights. Paint and
Sculpt remain mutually exclusive.

## Palette Editing

The Materials & Paint UI must support a palette beyond four entries.

Required operations: add layer from M89 texture catalog; change/assign
texture where allowed; edit tiling; remove non-base layer; select any
layer as Paint Layer.

Removing a used layer deterministically redistributes its weight,
preferring layer 0, then compacts palette indices and packed maps
without corrupting remaining painted regions.

Keep the UI compact and usable with scrolling where appropriate.

## Content Browser Integration

Continue using M89 texture identities and `SourceTextureCatalog`.

The loaded-Level physical texture delete guard must consider every
Terrain palette layer, including layers above index 3.

Do not duplicate texture identities to simulate channels.

## Rendering

Extend the world-lit Terrain path to render the scalable palette while
preserving per-layer XZ tiling, directional/local lighting, directional
shadows, M90's dedicated Terrain sampler strategy, and established
texture fallback behavior.

Do not reintroduce the raylib material-map sampler collision or
vertex-color tint leak.

Do not assume every palette texture can be permanently bound as a
traditional individual sampler. Choose a scalable GPU representation
compatible with the current raylib/OpenGL architecture, such as texture
arrays or another bounded strategy, after repository inspection.

The Level Designer palette may be larger than the number of
simultaneously bound traditional texture units. Document renderer-side
practical limits.

## Runtime / Asset Pipeline

Cook and stage every texture referenced by the Terrain palette using the
existing runtime PNG recipe and dependency-driven pipeline.

Development preserves established staged → cooked → source fallback
where applicable. Release remains staged-only.

Do not stage unreferenced textures merely because they exist in the
Content Browser.

## Editor Authority

Preserve:

-   `workingCopy` = pending authored edits/editor preview;
-   Apply validates/promotes to `active`;
-   Save writes authored state;
-   runtime state does not silently author data.

Palette edits and weight painting follow the M90 Dirty/Apply/Save
lifecycle.

## Physics

Terrain collision remains based on Terrain geometry/heights. Material
weight maps do not alter collision topology or physics.

## Required Regression Coverage

Add automated coverage for at least:

-   old M88 Terrain becomes 100% layer 0;
-   old M90 four-layer Terrain loads/migrates correctly;
-   more than four layers can be authored;
-   painting layer \>= 4 affects the correct layer;
-   normalization spans multiple RGBA maps;
-   unused packed channels remain zero;
-   deterministic quantized round-trip;
-   add/remove/remap above index 3;
-   painted-layer removal redistributes deterministically;
-   duplicate texture identity remains rejected/no-op;
-   sculpt preserves weights;
-   paint preserves heights;
-   delete guard checks high-index layers;
-   cook/stage discovers all referenced palette textures;
-   renderer distinguishes layers in different packed maps;
-   M90 shadow-map sampler regression remains covered;
-   Development fallback remains correct;
-   Release remains staged-only.

## Canonical Data Safety

Do not modify canonical authored Levels. Preserve:

-   `game/assets/source/levels/level_01.level`;
-   `game/assets/source/levels/level_02.level`.

Do not normalize their line endings. Use dedicated fixtures for
migration/serialization tests.

## Required Validation

Follow `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md`.

At minimum:

``` bash
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run relevant C++ tests and the established Python suite, including:

``` bash
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Run new M92-specific tests explicitly.

Before reporting:

``` bash
git branch --show-current
git status
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

## Manual Acceptance

Verify at minimum:

1.  an existing M90 Level retains its painted Terrain;
2.  palette can exceed four materials;
3.  layer above index 3 can be selected and painted;
4.  painting produces smooth regional blending;
5.  high-index painting does not corrupt low-index layers;
6.  removing a used high-index layer leaves a valid deterministic
    result;
7.  Sculpt still works without damaging paint;
8.  Apply/Save/reload preserves the result;
9.  referenced textures render correctly in Development;
10. Release renders saved Terrain from staged assets;
11. canonical Levels remain unchanged.

## Out of Scope

Do NOT add vegetation/foliage, trees/grass placement, fauna, traversal
paths, navmesh/pathfinding, PBR expansion, normal/roughness/metallic
Terrain layers, triplanar mapping, automatic slope/height materials,
biome generation, procedural distribution, undo/redo, ECS/GUID/prefab
work, generalized material graphs, external splat-map editing, or M93
features.

## Tuning vs Correction

Functional defects are not tuning. Brush feel, default weight
resolution, or measured performance optimization may enter Tuning
Backlog only when current behavior is already correct and safe.

Data corruption, wrong layer mapping, broken migration, wrong texture
sampling, Apply/Save lifecycle errors, delete-guard failures, or Release
dependency failures require correction before closure.

## Completion Report

Report:

1.  repository state inspected before implementation;
2.  scalable weight-map architecture and rationale;
3.  practical palette/render limits;
4.  weight-map resolution/default/bounds;
5.  Level Format extension and M90 migration;
6.  palette/editor changes;
7.  painting/remapping behavior;
8.  renderer/GPU representation;
9.  asset pipeline/delete-guard changes;
10. regression coverage;
11. Debug/Development/Release build results;
12. Python/C++ test results;
13. `git diff --check`;
14. canonical Level status;
15. remaining manual acceptance;
16. legitimate Tuning Backlog candidates;
17. confirmation of no commit/push/merge/M93 work.

## STOP

After implementation, validation, and completion report: **STOP**.

Do not commit. Do not push. Do not merge. Do not close M92. Do not begin
M93.
