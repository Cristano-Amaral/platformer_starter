# Milestone 85 --- Lighting & Shadows Foundation

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/85-lighting-shadows-foundation`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Introduce the first engine-owned world lighting and directional-shadow
foundation for the current raylib renderer, building on M84 Materials &
Textures without redesigning the renderer or Level Format.

``` text
World Rendering
├─ Ambient Light
└─ Directional Light
   ├─ direction
   ├─ color
   ├─ intensity
   └─ directional shadows
```

## Product outcome

After M85, supported world/model geometry responds consistently to
ambient + directional illumination and appropriate gameplay-world
geometry casts/receives directional shadows. M84 Base Color/Texture
remains intact. Development and Release share deterministic runtime
behavior. Lighting remains presentation only.

## Inspect first

Inspect Renderer, existing shaders, cameras/projection, M84 material
preparation, Static Props, Item Pickups/highlights, M83 Player,
greybox/world primitives, Dynamic Boxes, Doors, Goal visuals, editor
viewport, thumbnails/Preview, render-target lifecycle, resize/F2
lifecycle, runtime staging, and coordinate/world-up conventions.

## Lighting environment

Create/clarify the narrowest engine-owned environment containing
deterministic ambient illumination and one directional light.
Directional light has normalized direction, color and intensity. No
generic light list, ECS light components, Point or Spot lights.

## Material integration

Lighting modulates M84 Base Color and Base Color Texture without
replacing them. Textures remain recognizable, untextured models retain
color, fallbacks remain valid, and transient highlights do not
permanently mutate materials. No full PBR.

## Shader boundary

If required, add the smallest engine-owned shader path for
lighting/shadows. Centralize deterministic shader ownership/load/unload.
Runtime shader files, if any, must be staged in
Debug/Development/Release with no source fallback. No shader
framework/graph, render graph, deferred renderer, or renderer rewrite.

## Directional shadows

Implement the smallest robust single directional-shadow-map solution
compatible with the current renderer:

``` text
Directional Light
      ↓
Shadow Depth Pass
      ↓
Shadow Map
      ↓
Main Lit Pass
├─ Base Color/Texture
├─ Ambient
├─ Directional Light
└─ Shadow Visibility
```

No cascaded shadow maps.

Use deterministic orthographic shadow coverage suitable for current
small platformer scenes, with a narrow bias strategy against severe
acne/detachment. Basic filtering is acceptable.

## Casters and receivers

Deliberately inspect/integrate appropriate gameplay-world categories:
greybox/platform geometry, Static Props, Dynamic Boxes, Item Pickups,
Player, Doors, Pressure Plates where applicable, and Goal visual.
Editor-only grid/gizmos/ghost overlays must not accidentally pollute the
shadow map. Document exclusions.

## Existing systems

M83 Player authority/controller/physics/camera/interactions remain
unchanged; no animation.

Item Pickup bob/spin controls its visual transform and lighting/shadow
follows that transform. Highlight remains transient/readable.

Static Props preserve transforms, picking, gizmos, groups, lifecycle and
M84 textures.

Do not replace greybox geometry systems merely to make them lit.

## Editor / thumbnails / Preview

Development world viewport should show useful world lighting/shadows
while preserving grid, selection, gizmos, hierarchy/groups and F2. Do
not add a Lighting panel/editor.

Thumbnail/Preview rendering must remain isolated from world shadow
state. Preserve their existing deterministic preview presentation where
appropriate.

## Configuration

Use the smallest deterministic project-level/default lighting
configuration. Do not add Level syntax for ambient light, directional
light or shadow settings. Leave a clean future extension point for an
Environment/Lighting editor.

## Resource lifecycle

Shader, shadow-map/depth resources and uniform state have deterministic
ownership. No per-frame resource creation/destruction. Resize, F2,
Restart, level transition, Play Again and MainMenu→Play must not leave
stale resources.

## Failure

Shader/shadow failures must not crash or spam logs. Preserve usable
rendering/gameplay where practical. No source fallback.

## Tests

Add focused CPU-side tests where practical for light
normalization/defaults, invalid direction fallback, color/intensity
validation, light-view/projection math, shadow coverage/bias config,
resource lifecycle and caster eligibility. Preserve relevant M69--M84
regressions.

## Manual acceptance

Development: - Main Menu → Play; - visibly directional illumination; -
lit/shadowed sides readable; - cast shadows visible on world surfaces; -
Player shadow follows Player; - Static Prop shadow; -
`test_textured.glb` texture remains recognizable; - model-backed Item
Pickup lighting/shadow follows bob/spin; - target highlight remains
correct; - Dynamic Box/Door/world geometry; - move through scene and
inspect clipping/swimming/acne; - death/respawn; - level_01 →
level_02; - Restart / Play Again; - F2 roundtrip; - editor
grid/gizmos/selection/groups readable; - thumbnail/Preview correct and
isolated.

Release: - lighting/shadows visible; - Player/materials correct; -
normal gameplay/interactions/transitions; - no source dependency.

Automated green is insufficient.

## Canonical Level safety

Do not intentionally modify: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

Do not normalize EOLs. Final semantic diffs must be empty.

## Validation

Run directly affected C++
renderer/model/material/player/item/editor/lifecycle/runtime-asset
suites and relevant M69--M84 regressions.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Add the narrowest shader/runtime staging regression if shader files are
introduced.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Then run `git diff --check` and both canonical Level diffs.

## Out of scope

No Point/Spot lights, multiple authored lights, per-Level lighting
syntax, Lighting Editor, cascaded shadows, baked lightmaps, GI, SSAO,
HDR/bloom/tone-map redesign, environment maps/IBL, skybox, full PBR,
richer material channels, shader graph/framework, render graph, deferred
renderer, terrain, player animation, Character Definition, multiple
playable characters, ECS, scene graph, generic asset-manager rewrite, or
M86 functionality.

## Documentation and STOP

Canonical active document: `docs/milestones/MILESTONE_85.md`. Preserve
M76--M84 as CLOSED and keep `docs/MILESTONES.md` compact.

Document environment ownership, ambient/directional semantics,
shader/shadow ownership, caster/receiver categories, coverage/bias,
staging, failure behavior and future Lighting Editor extension point.

Cursor report must cover files, discovered architecture, lighting
boundary/defaults, shader architecture, shadow technique/map/lifetime,
projection/coverage, bias/filter, casters/receivers, world/Static
Prop/M84/Item Pickup/Player/editor integration, thumbnail isolation,
lifecycle/failure/staging, no Level lighting syntax/editor,
tests/builds/diffs and out-of-scope confirmation.

After implementation M85 is **implemented, awaiting manual acceptance**,
not CLOSED.

Then STOP.

Do NOT commit. Do NOT push. Do NOT merge. Do NOT start M86. Do NOT mark
M85 CLOSED.
