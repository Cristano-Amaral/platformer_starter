# Milestone 84 --- Materials & Textures Foundation

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/84-materials-textures-foundation`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Introduce the first engine-owned material/texture foundation for
model-backed rendering without redesigning the renderer, Level Format,
Content Browser, or asset pipeline.

Conceptually:

``` text
Renderable Model
├─ geometry
└─ material presentation
   ├─ base color
   └─ base color texture
```

Preserve Static Props, Item Pickups, M83 Player Character presentation,
editor previews/thumbnails, gameplay, staging, and Release behavior.

## Product outcome

After M84, model-backed objects can preserve/use imported GLB base-color
material presentation; base color and base-color texture have a clear
runtime material boundary; textured GLBs render consistently in gameplay
and relevant editor previews; missing textures fail safely; runtime
remains staged-only; untextured models remain correct.

This is foundation, not a Material Editor.

## Inspect first

Inspect current raylib Model/Material/Texture handling,
StaticModelSceneStore, Static Props, Item Pickups, M83 Player
loading/rendering, Content Browser thumbnails/previews, GLB/PNG
import/cook/stage paths, texture ownership/unload, Renderer
responsibilities, shader/lighting assumptions, and Development/Release
paths.

## Scope

Support only: - Base Color - Base Color Texture

No normal, roughness, metallic, AO, emissive, PBR, shader graph, custom
lighting, or transparency system.

Prefer imported GLB materials over a new authored material file format.
Inspect and explicitly document whether embedded textures, external
textures, or both are safely supported by the actual pipeline.

## Engine boundary and ownership

Create/clarify the narrowest engine-owned runtime
material/model-presentation boundary needed. Do not spread ad-hoc
material manipulation across gameplay/editor code and do not build a
generic asset manager.

Resources must have deterministic ownership: no per-frame loads, double
unloads, dangling texture/material references, or unsafe reload/shutdown
behavior. Follow actual raylib ownership semantics.

## Rendering semantics

Preserve imported base color/tint where supported. Untextured materials
use deterministic base color fallback. Textured GLBs use model UVs and
staged runtime data. Missing/broken texture data fails safely and does
not corrupt unrelated assets.

No Level-authored material overrides.

## Reference asset

Add or identify a small project-owned textured GLB reference asset.
Prefer deterministic project-generated geometry and a small
project-owned PNG if needed. Make UV/orientation visually obvious. Do
not modify canonical Levels to place it.

## Pipeline

Preserve the existing GLB import workflow and extend only the narrowest
pipeline pieces needed for the chosen texture mode. Runtime/Release must
not depend on source roots, Content Browser, editor catalogs, or
Development-only discovery.

## Existing systems

Static Props must preserve transforms, selection/picking, Content
Browser, thumbnails/previews, Save/Apply/lifecycle.

Item Pickups must preserve logical gameplay authority, visual
transforms, idle bob/spin, highlight and collection feedback.

M83 Player must preserve staged-only loading, authoritative-following
presentation, facing/offset/scale, fallback, lifecycle, and no
animation.

Where the same model data is rendered in Content Browser
thumbnails/previews, imported base-color presentation should be
consistent. Do not redesign Content Browser or create a Material
Browser.

Transient selection/target highlights must not permanently corrupt
imported material state.

## Renderer and Lighting boundary

Stay within the current raylib/renderer architecture. No deferred
renderer, render graph, PBR pipeline, shader framework, lighting
redesign, directional lights or shadows. M85 remains Lighting & Shadows
Foundation.

## Failure behavior

Missing/invalid model or texture must not crash, poison unrelated
assets, or spam every frame. Use deterministic fallback and staged-only
behavior.

## Regression coverage

Cover untextured GLB, base color, textured GLB recognition, chosen
texture dependency mode, missing texture fallback, non-per-frame
loading, ownership/reload safety, Static Prop, Item Pickup/highlight,
M83 Player, Content Browser thumbnail/preview, staged-only Release, no
new Level material syntax, canonical Level safety, and relevant M76--M83
regressions.

## Manual acceptance

In Development verify an untextured model and the reference textured
GLB, UV/orientation, Static Prop workflow where appropriate,
thumbnail/preview, selection/highlight, disposable-Level
Apply/Save/reload if needed, model-backed Item Pickup, M83 Player, F2,
and repeated reload stability.

In Release verify the existing run, Player rendering, staged
textured-model behavior through an appropriate non-canonical path if
available, and no source dependency.

Automated green is not sufficient.

## Canonical Level safety

Do not intentionally modify: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

Do not normalize EOLs. Final semantic diffs must be empty.

## Validation

Run directly affected model/store/lifetime, Static Prop, Item Pickup,
Content Browser/catalog/thumbnail/preview, runtime asset, Renderer, M83
PlayerPresentation, lifecycle/F2, and relevant editor tests.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Add/extend the narrowest Python regression needed for the chosen GLB
texture mode.

Build all:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Then run `git diff --check` and canonical Level diffs.

## Out of scope

No Material Editor/Browser, authored material format, per-instance
overrides, normal/roughness/metallic/AO/emissive, PBR, shader
graph/framework, lighting/shadows/environment maps, terrain
materials/painting, player animation, Character Definition, multiple
playable characters, ECS, scene graph, generic asset-manager rewrite, or
M85 functionality.

## Documentation and STOP

Canonical active document: `docs/milestones/MILESTONE_84.md`. Preserve
M76--M83 as CLOSED and keep `docs/MILESTONES.md` compact.

Cursor report must cover files, discovered pre-M84 architecture,
material boundary, supported properties, embedded/external texture
decision, reference asset/provenance, base color/texture behavior,
ownership/lifetime, reload/failure, Static Prop/Item
Pickup/highlight/Player/Content Browser integration, staging/Release
behavior, no Level syntax, regressions/tests/builds/diffs and
out-of-scope confirmation.

After implementation M84 is **implemented, awaiting manual acceptance**,
not CLOSED.

Then STOP.

Do NOT commit. Do NOT push. Do NOT merge. Do NOT start M85. Do NOT mark
M84 CLOSED.
