# Milestone 94 --- Terrain Vegetation Distribution & Rendering Improvements

## Status

**Implemented, awaiting manual acceptance**

## Objective

Consolidate the Terrain Vegetation system introduced in Milestone 93 by
improving two implementation-level aspects without changing its
established authoring model:

1.  reduce visible grid artifacts, gaps, lanes, and artificial clumping
    in generated vegetation;
2.  improve vegetation rendering scalability by replacing or
    substantially reducing the current per-instance CPU `DrawModel` path
    with an instanced/batched rendering path appropriate to the existing
    renderer.

Milestone 94 is an implementation and visual-quality milestone. It must
preserve the authored meaning and editor workflow established by M93.

------------------------------------------------------------------------

## Background

Milestone 93 established authored Terrain Vegetation with:

-   a vegetation palette backed by the existing static-model catalog;
-   up to 8 vegetation entries;
-   deterministic authored spatial distribution;
-   independent vegetation grid resolution;
-   per-entry coexistence in the same region;
-   Paint and selected-entry Erase;
-   per-cell/per-entry authored Density;
-   per-cell/per-entry authored Min Scale / Max Scale;
-   per-cell/per-entry authored Random Yaw;
-   per-cell/per-entry authored Align to Terrain Normal;
-   deterministic Save/reload;
-   Terrain-derived Y and optional current-normal alignment;
-   palette removal/remapping;
-   Level Format v1 serialization and migration of earlier M93
    representations;
-   cook/stage dependency extraction for vegetation-only GLB assets;
-   Content Browser delete protection;
-   workingCopy → Apply → active → Save lifecycle.

M93 deliberately left two items for later improvement:

-   the deterministic cell-based placement can reveal visible gaps,
    lanes, and clumping;
-   rendering currently issues CPU-side model draws per vegetation
    instance while reusing loaded model resources.

M94 addresses those two items while treating the M93 authored data model
as an established contract.

------------------------------------------------------------------------

## Scope

### 1. Improved Deterministic Spatial Distribution

Replace or improve the current per-cell placement sampling so vegetation
looks less like an underlying regular grid.

The result should reduce:

-   visible empty lanes between populated cells;
-   repeated cell-shaped gaps;
-   obvious aligned rows/columns;
-   unnatural clustering caused primarily by the sampling method.

The implementation must remain deterministic.

For identical authored vegetation data, seed, palette configuration, and
Terrain surface, repeated generation must produce the same placement.

The distribution algorithm may use deterministic techniques such as:

-   stratified jitter;
-   low-discrepancy sampling;
-   blue-noise-like deterministic sampling;
-   deterministic rejection/spacing;
-   neighboring-cell-aware sampling;

or another compact approach justified by the current architecture.

The milestone does **not** require mathematically exact Poisson-disk
sampling.

### 2. Preserve Authored Density Semantics

M93 Density remains spatially authored.

M94 may reinterpret how the requested density is spatially distributed
inside/around occupied cells, but must preserve the user-facing meaning:

-   low-density painted regions remain visibly less dense;
-   high-density painted regions remain visibly denser;
-   changing Next-Paint Density without painting does not affect
    existing authored vegetation;
-   repaint changes only the selected entry inside the brush;
-   different vegetation entries can coexist independently.

Do not convert authored vegetation into individual serialized instances.

### 3. Stable Terrain Following

Vegetation must continue to derive:

-   X/Z from deterministic vegetation distribution;
-   Y from the current Terrain surface;
-   normal alignment from the current Terrain surface when authored
    Align is enabled.

Terrain sculpting must continue to update vegetation height and aligned
orientation without rewriting authored vegetation data.

### 4. Rendering Scalability

Inspect the current vegetation rendering path and implement an
appropriate instanced or batched path that materially reduces
per-instance CPU draw overhead.

The preferred target is GPU instancing when it can be integrated
correctly with the existing rendering architecture.

The solution must preserve the visual behavior required by the existing
world lighting path, including the relevant transforms and
lighting/shadow behavior already supported for vegetation/static models.

Do not introduce an instanced path that silently bypasses established
world lighting semantics merely to reduce draw calls.

If the existing `world_lit` shader architecture requires a focused
extension for per-instance transforms, that extension is in scope.

### 5. Resource Reuse

Continue to reuse one loaded model/resource per vegetation model
identity.

Do not duplicate model resources per vegetation instance.

Instancing/batching should be organized around compatible
model/mesh/material resources as appropriate to the current renderer.

### 6. Mixed Vegetation Entries

The optimized rendering path must support:

-   multiple vegetation palette entries;
-   multiple entries occupying the same cells;
-   different spatial Density values;
-   different spatial Min/Max Scale values;
-   Random Yaw ON/OFF;
-   Align to Terrain Normal ON/OFF;
-   different model identities.

The renderer must not assume that all vegetation shares one transform
policy or one model.

### 7. Editor Preview and Lifecycle

Preserve the established authored lifecycle:

`workingCopy → Apply → active → Save`

Vegetation preview in the editor must continue to reflect workingCopy
changes without prematurely promoting them to active authored state.

No editor action introduced by M94 may bypass Apply/Save semantics.

### 8. Level Format v1 Compatibility

Keep:

`PLATFORMER_LEVEL 1`

M94 should preferably require **no authored format change** because the
milestone changes generated distribution/rendering rather than the
meaning of authored vegetation.

Existing M93 levels must continue to load.

Do not serialize generated instance transforms.

If a format change becomes genuinely necessary, it must be minimal,
backward-compatible, remain within all Level Format v1 limits, and be
explicitly justified. A format change must not be introduced merely for
rendering convenience.

Limits remain:

-   64 KiB maximum file size;
-   256 lines maximum;
-   512 characters maximum per line.

### 9. Determinism

Add regression coverage proving that the improved distribution is
deterministic.

For fixed authored data and seed, verify stable generated instance data
across repeated rebuilds/reloads.

The deterministic contract concerns the M94 algorithm itself. M94 is
allowed to improve the exact generated X/Z placement relative to M93; it
does not need to preserve the exact old M93 generated positions, because
those positions were derived rather than authored.

However:

-   authored occupancy and parameters must not be modified merely by
    loading;
-   Save without authoring changes must not bake generated positions
    into the level;
-   repeated M94 generation must be stable.

### 10. Practical Performance Validation

Add an automated or diagnostic validation path appropriate to the
repository that demonstrates the optimized renderer handles a high
vegetation instance count without falling back to one ordinary model
draw call per instance.

The milestone does not require a specific FPS target because performance
depends on hardware.

Validation should instead verify architectural properties such as:

-   number of generated instances;
-   number of instanced/batched submissions or groups;
-   resource reuse;
-   absence of per-instance model loading.

Where practical, expose debug/test counters rather than relying on
timing-sensitive tests.

------------------------------------------------------------------------

## Editor UX

M94 is not a general Terrain Vegetation Inspector redesign.

Small UI/debug additions are allowed only when they directly support
validation or understanding of the new distribution/rendering behavior.

Examples:

-   generated instance count;
-   batch/instancing group count;
-   optional development-only diagnostics.

Do not turn M94 into the broader Vegetation Editor UX milestone.

------------------------------------------------------------------------

## Required Regression Coverage

At minimum, tests must cover:

1.  deterministic generation for identical authored input and seed;
2.  different seeds produce a different deterministic distribution;
3.  low vs high authored density remains meaningfully distinct;
4.  changing Next-Paint controls without painting does not alter
    existing authored vegetation;
5.  coexistence of entries A and B remains intact;
6.  selected-entry Erase remains intact;
7.  scale/yaw/align authored semantics remain intact;
8.  Terrain Y following remains intact;
9.  Align-to-normal follows the current sculpted Terrain;
10. palette removal/remapping remains valid;
11. Save/reload preserves authored data and regenerates the same M94
    distribution;
12. existing M93 serialized vegetation loads successfully;
13. high-instance vegetation uses the optimized rendering grouping/path;
14. model resources remain shared by identity;
15. Level Format v1 size/line constraints remain valid.

Existing M93 regression tests must remain green unless a test explicitly
encoded the old derived placement algorithm. Such a test may be updated
to the new deterministic contract, but authored semantics must not be
weakened.

------------------------------------------------------------------------

## Canonical Level Safety

Do not use canonical gameplay levels as disposable test fixtures.

In particular:

-   do not mechanically rewrite
    `game/assets/source/levels/level_01.level`;
-   do not mechanically rewrite
    `game/assets/source/levels/level_02.level`;
-   do not normalize their line endings;
-   use dedicated/temporary fixtures for automated tests.

Before completion:

-   `git diff -- game/assets/source/levels/level_01.level`
-   `git diff -- game/assets/source/levels/level_02.level`

must show no unintended M94 changes.

------------------------------------------------------------------------

## Out of Scope

M94 must **not** introduce:

-   grass/ground-cover-specific rendering;
-   wind animation;
-   vegetation LOD authoring;
-   billboards/impostors;
-   fauna, NPCs, or AI;
-   PBR terrain/material redesign;
-   triplanar terrain mapping;
-   vegetation collision authoring;
-   harvesting/destruction gameplay;
-   GUIDs;
-   prefabs;
-   undo/redo;
-   generalized ECS work;
-   a broad Content Browser redesign;
-   a broad Terrain Vegetation Inspector redesign;
-   individual-instance authored transforms;
-   a new Level Format version.

These belong to later milestones unless separately reprioritized.

------------------------------------------------------------------------

## Implementation Guidance

Prefer adapting the existing M93 structures instead of replacing them.

Keep a clear separation between:

-   **authored vegetation data** --- palette identity plus spatially
    authored Paint parameters;
-   **derived vegetation instances** --- deterministic
    positions/transforms generated from authored data and Terrain;
-   **render resources** --- reusable loaded model/mesh/material
    resources;
-   **render submissions** --- instanced/batched groups generated from
    compatible derived instances.

Do not let renderer-specific representation become the authored data
model.

Distribution generation should remain CPU-testable without requiring the
renderer.

Rendering optimization should be testable independently from Level
serialization where practical.

------------------------------------------------------------------------

## Validation

Run the standard repository validation required by `AGENTS.md` and
`DEVELOPMENT_WORKFLOW.md`.

At minimum:

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

Run all affected C++ tests, including at least:

-   TerrainVegetationTest;
-   LevelFileTest;
-   ContentBrowserAssetLibraryTest;
-   AuthoredLifecycleIntegrationTest;
-   affected renderer/static-model tests;
-   affected Terrain geometry/sculpt tests.

Before reporting:

``` text
git branch --show-current
git status
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

------------------------------------------------------------------------

## Manual Acceptance

Manual acceptance should verify at least:

1.  paint a broad low-density vegetation region and inspect it from
    several camera angles;
2.  paint a broad high-density region and verify it remains visibly
    denser;
3.  verify the distribution has materially fewer obvious grid
    lanes/gaps/clumps than M93;
4.  paint multiple vegetation entries over the same area and verify
    coexistence;
5.  verify different authored scale/yaw/align regions remain correct;
6.  sculpt Terrain under vegetation and verify Y/normal behavior;
7.  Apply, Save, reload, and verify deterministic appearance;
8.  create a practically dense vegetation scene and verify
    editor/runtime behavior remains responsive enough for manual use;
9.  verify Development and Release rendering preserve the expected
    vegetation appearance and world lighting.

Automated tests are not a substitute for this manual acceptance.

------------------------------------------------------------------------

## Completion Criteria

M94 is complete only when:

-   improved deterministic distribution is implemented;
-   visible regular-grid artifacts are materially reduced;
-   vegetation rendering no longer relies on an ordinary CPU model draw
    for every generated instance, or an equivalent batching/instancing
    solution has been implemented and justified;
-   M93 authored semantics are preserved;
-   existing M93 levels remain compatible;
-   deterministic Save/reload behavior is preserved;
-   Terrain following remains correct;
-   all required automated validation is green;
-   canonical levels contain no unintended changes;
-   manual acceptance is explicitly approved by the user;
-   Git closure is performed only after approval.

------------------------------------------------------------------------

## Agent Stop Condition

The implementation agent must stop after implementation, validation, and
reporting.

It must **not**:

-   commit;
-   push;
-   merge;
-   close M94;
-   start M95.

Git closure remains a separate user-approved step.
