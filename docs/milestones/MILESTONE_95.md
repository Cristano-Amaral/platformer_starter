# Milestone 95 --- Terrain Vegetation Editor UX

## Status

**Implemented, awaiting manual acceptance**

## Objective

Improve the Terrain Vegetation authoring experience established by M93
and technically consolidated by M94. M95 focuses on editor usability,
clarity, visual feedback, and efficient vegetation-palette workflows
without changing the underlying authored vegetation semantics,
deterministic distribution, GPU-instanced rendering, or Level Format v1
contract.

## Background

M93 established the vegetation palette, independent vegetation grid,
coexistence, Paint/Erase, spatial Next-Paint Density/Scale/Yaw/Align
parameters, workingCopy → Apply → active → Save, deterministic derived
vegetation, serialization, and asset integration.

M94 improved deterministic spatial distribution, added GPU-instanced
rendering/resource reuse and diagnostics, and corrected
directional-shadow focus for larger traversable worlds.

The architecture is established. M95 improves authoring UX rather than
redesigning it.

## Scope

### 1. Vegetation Palette UX

Present the existing palette as a clearer asset-oriented palette. Each
entry should expose a model thumbnail, useful name/identity, selected
state, selection for Paint/Erase, and clear removal action. Reuse
existing Content Browser/static-model thumbnail infrastructure. Do not
create a parallel thumbnail system. Preserve the maximum of 8 entries.

### 2. Add Vegetation Model Workflow

Provide a practical way to select an existing compatible static `.glb`
asset and add it to the palette without manually typing identities.
Reuse StaticModelCatalog/Content Browser concepts through an asset
picker, filtered popup, or equivalent architecture-consistent UI.

Only compatible models may be selected. Preserve duplicate rules,
clearly handle the palette limit, and modify workingCopy rather than
active.

### 3. Selected Vegetation Entry

Make the selected entry visually unambiguous. The UI must clearly show
which model Paint/Erase affects and which Next-Paint values are active.
Changing selection must not mutate painted vegetation.

### 4. Next-Paint Parameter Presentation

Clearly group Density, Min Scale, Max Scale, Random Yaw, and Align to
Terrain Normal under a section such as `Next Paint`. These controls
affect future Paint/Repaint only. Changing them alone must not author
vegetation.

### 5. Brush Controls

Clearly expose Paint/Erase mode, Radius, and selected entry. Preserve
existing brush semantics unless a genuine bug is found.

### 6. Viewport Brush Preview

When vegetation authoring is active and the cursor has a valid Terrain
hit, show a clear viewport brush footprint reflecting Radius. It must
follow the Terrain hit, remain visible against common materials,
disappear without a valid hit/inactive painting, and never become
authored content. Reuse existing Terrain brush-preview conventions where
suitable.

### 7. Paint / Erase Feedback

WorkingCopy preview must immediately and clearly reflect Paint/Erase.
Erase affects only the selected entry and coexisting entries remain
intact. Do not add individual-instance selection/manipulation.

### 8. Empty / Limit / Invalid States

Explicitly handle empty palette, no selected entry, full palette,
missing model references, and no compatible picker/search results. Avoid
silent failure.

### 9. Diagnostics Placement

Keep M94 diagnostics such as Generated instances and Render groups
available but subordinate to normal authoring controls. Do not turn M95
into a renderer profiler.

### 10. Lifecycle Authority

Preserve exactly: `workingCopy → Apply → active → Save`

Adding/removing palette entries and Paint/Erase modify workingCopy.
Changing Next-Paint controls without painting does not author
vegetation. Apply promotes workingCopy; Save persists authored state.

### 11. Content Browser / Identity Integration

Reuse StaticModelCatalog, existing identities, thumbnail/cache behavior,
and Content Browser delete guards. References from workingCopy, active
state, or saved baseline must remain protected according to established
behavior.

### 12. Input Safety

Inspector controls, popups, asset pickers, search fields, and other
captured editor UI must not accidentally trigger Terrain vegetation
Paint/Erase. Reuse existing editor input-capture conventions.

## Target Workflow

1.  Select Terrain.
2.  Open Vegetation.
3.  Add one or more models from existing assets.
4.  Select a thumbnail/card as the active entry.
5.  Select Paint or Erase.
6.  Adjust Radius.
7.  Adjust clearly labeled Next-Paint Density / Scale / Yaw / Align.
8.  See the viewport brush footprint.
9.  Paint/erase and see immediate workingCopy preview.
10. Apply.
11. Save.

The exact layout should follow existing editor conventions and available
space.

## Data and Serialization

No Level Format v1 change is expected. Keep `PLATFORMER_LEVEL 1`.

Do not serialize UI-only state such as thumbnail state, popup state,
brush hover position, temporary preview, generated instances, or
arbitrary UI selection. Editor-only preferences may use an existing
appropriate layout/preferences mechanism only if justified.

Authored vegetation remains the M93/M94 representation.

## Required Regression Coverage

At minimum cover applicable non-visual behavior: 1. add valid model; 2.
duplicate handling; 3. palette maximum; 4. removal/remapping; 5.
selection does not mutate authored vegetation; 6. Next-Paint controls
without painting do not mutate existing vegetation; 7. Paint authors
selected entry/current Next-Paint values; 8. Erase removes only selected
entry; 9. coexistence remains intact; 10. workingCopy does not affect
active before Apply; 11. Apply promotes correct state; 12. Save/reload
preserves authored vegetation; 13. missing model references are safe;
14. delete guards remain correct; 15. M94 deterministic distribution
remains unchanged by editor-only interactions; 16. M94 instanced
rendering remains active; 17. UI capture does not trigger painting where
deterministic input testing is practical.

Visual thumbnail/brush correctness remains subject to manual acceptance
where automation is impractical.

## Manual Acceptance

Verify: 1. palette entries are easy to identify from thumbnails/names;
2. adding models is straightforward and requires no manual identity
entry; 3. selected entry is obvious; 4. Next-Paint semantics are
obvious; 5. changing Next-Paint controls without painting leaves
existing vegetation unchanged; 6. Radius is easy to find and viewport
preview matches the affected area; 7. Paint gives immediate workingCopy
feedback; 8. Erase affects only selected entry; 9. coexistence remains
understandable; 10. empty/full states are clear; 11. missing-model
behavior is safe; 12. UI interaction does not accidentally paint; 13.
Apply → Save → reload remains correct; 14. M94 distribution remains
visually correct; 15. dense vegetation still uses M94 instancing and
remains usable; 16. directional shadows continue following traversal;
17. Development and Release are correct.

## Out of Scope

Do not introduce distribution retuning unrelated to a bug, new
vegetation serialization, individual-instance selection/transforms, LOD
authoring, billboards/impostors, wind, grass/ground-cover, fauna/NPC/AI,
vegetation gameplay/collision/harvesting/destruction, generalized
drag-and-drop/property/reflection frameworks, GUIDs, prefabs, undo/redo,
ECS redesign, broad Content Browser redesign, renderer redesign, or
Level Format v2.

## Implementation Guidance

Inspect the existing editor and Content Browser first. Prefer reuse for
thumbnails, StaticModelCatalog lookup, display names, popup/search
behavior, layout persistence, input capture, and Terrain brush
visualization.

Keep authored state separate from editor-only transient state. Do not
put UI-only concepts into TerrainSpec or Level Format structures.
Preserve M94 distribution/rendering unless a regression is discovered.

## Canonical Level Safety

Do not use canonical levels as disposable fixtures and do not
mechanically modify/normalize: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

Use dedicated/temporary fixtures.

## Validation

Follow `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md`.

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

Run affected C++ tests including TerrainVegetationTest,
ContentBrowserAssetLibraryTest, AuthoredLifecycleIntegrationTest,
LevelFileTest, LoadedModelMaterialsTest, LightingEnvironmentTest,
affected editor/input tests, and affected Terrain tests.

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

M95 is complete only when palette authoring is materially
clearer/faster; models are visually identifiable; adding compatible
assets is practical; selected-entry and Next-Paint semantics are
unambiguous; brush mode/radius/viewport footprint are understandable;
empty/full/missing states are handled; UI interaction does not
accidentally author vegetation; M93/M94 semantics, deterministic
distribution, instancing, lifecycle, serialization, and delete guards
remain intact; automated validation passes; canonical levels have no
unintended changes; manual acceptance is explicitly approved; and Git
closure occurs only afterward.

## Agent Stop Condition

After implementation, automated validation, and reporting, STOP.

Do not commit, push, merge, close M95, or start M96.
