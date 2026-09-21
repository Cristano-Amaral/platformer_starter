# Milestone 88 --- Terrain Materials Foundation

## Status

**CLOSED**

Milestone 87 — Terrain Sculpting is CLOSED. `main` was clean and
synchronized before M88. Milestone 89 is the next Development asset-library
milestone.

## Branch

`milestone/88-terrain-materials-foundation`

## Recommended Cursor model

**Grok 4.6 High --- Fast OFF**

## Purpose

Replace the M86/M87 solid-color Terrain presentation with the first
authored textured Terrain surface-material workflow. The immediate goal
is to make sculpted relief substantially easier to read by adding
visible surface detail and scale reference. Architecturally, establish
the smallest deterministic Terrain material foundation that can later
support material/layer painting without prematurely implementing
painting, splat maps, weight maps, vegetation, PBR expansion, or a
generalized material framework.

## Existing behavior to preserve

Preserve M84--M87 behavior: existing PNG/material asset conventions and
staging, singleton Terrain, Level Format v1, deterministic
geometry/normals, Terrain Sculpt, working-copy preview,
`workingCopy -> Apply -> active -> runtime`, Jolt collision, world
lighting/local-light behavior, directional shadows,
selection/picking/Inspector/lifecycle, and Release.

Reuse existing systems; do not create a second generic texture catalog
or PNG loader.

## User-facing outcome

Terrain can have one authored base surface texture selected in
Development using existing asset/catalog conventions plus an authored
texture tiling/scale parameter. The working-copy material previews
immediately, survives Sculpt, Apply, Save/reload, staging, and Release.
Grass, dirt, sand, rock, or other user-imported textures are valid
content examples, but their names have no hard-coded gameplay meaning.

## Scope: one base Terrain material

M88 supports one base Terrain texture/material assignment per Terrain.
It does not yet support painted multi-layer Terrain, per-sample weights,
splat maps, blend masks, automatic slope/height rules, or multiple
simultaneous material layers. Keep the representation minimal while
avoiding choices that unnecessarily block a later painting milestone.

## Reuse M84

Inspect M84 and reuse its established authored texture identity, texture
discovery/catalog UI, runtime PNG support, GPU texture caching/loading,
missing/invalid texture behavior, cooker/staging, and
Development/Release resource conventions. Do not duplicate
infrastructure. Report the exact reuse decision.

## Authored data

Extend Terrain with only the minimum authored base-material information:
existing-style texture identity/reference plus finite positive bounded
texture tiling/scale. Use no absolute paths or Development-only runtime
dependency. Choose defaults/bounds after repository inspection. Old
M86/M87 Terrain data without material data remains valid and uses
deterministic fallback. Follow existing validation conventions. Do not
expand Terrain PBR unless already directly required by an existing
reusable material abstraction.

## Level Format v1

Keep version 1. Extend Terrain persistence backward-compatibly using the
smallest grammar consistent with current parser/writer conventions. Old
Terrain syntax remains valid. Texture identity and tiling roundtrip
deterministically. Preserve 64 KiB / 256 lines / 512 chars-per-line
constraints. Do not materialize defaults into canonical Levels. Prefer
an optional Terrain record/suffix if appropriate after inspection. If a
version bump appears necessary, STOP and report. Document final grammar
in `docs/LEVEL_FORMAT_V1.md`.

## Texture mapping

Define deterministic continuous Terrain UVs. Prefer the simplest mapping
appropriate to the regular XZ grid and existing M86 UV support. Mapping
must remain stable when heights are sculpted, work for rectangular
Terrain, use authored tiling predictably, and not depend on camera
orientation. Prefer XZ/world-oriented planar mapping if it fits current
geometry/shader conventions. Do not implement triplanar unless existing
architecture already supports it cleanly and it is strictly necessary.
Steep-slope stretching may be reported as a foundation limitation.
Document the exact formula.

## Inspector

When Terrain is selected, provide a focused Terrain Material section
with current assignment, selection/assignment using existing catalog
conventions, clear/reset to fallback, and tiling/scale control. Do not
require opaque absolute paths. Assignment/tiling changes are authored
working-copy changes. Merely browsing a selector is not.

## Working-copy preview

Development must preview working-copy Terrain geometry, texture
assignment, and tiling before Apply. Sculpt and material editing
coexist: sculpt does not reset material; material changes do not reset
heights. Active/runtime authority remains unchanged until Apply.

## Rendering and lighting

Terrain remains on the established world-lit path, extended minimally to
consume existing texture resources. Textured Terrain must retain
Ambient, Directional, Point, Spot, M85.4 effective local lights, and
directional shadow cast/receive behavior. Do not replace lighting with
unlit rendering or create a second lighting model. No valid assignment
must produce deterministic usable fallback; preserving the existing
solid Terrain color is acceptable.

## GPU/resource lifecycle

Reuse existing texture caches/ownership. No per-frame texture loading,
GPU leaks, stale texture after Apply/reload/transition, or unnecessary
duplicate texture resources. F2 must not duplicate resources. Release
uses staged runtime assets only. Do not create a new global resource
manager.

## Sculpt compatibility

Raise, Lower, Smooth, Flatten, brush preview, working-copy preview,
Apply, and Jolt behavior from M87 remain functional. Texture mapping
remains stable as heights change. Sculpt never changes material data;
material editing never changes heights.

## Dirty/no-op

Semantic texture assignment/tiling changes participate in existing
workingCopy Modified/Dirty behavior. Reassigning the same texture,
setting the same effective tiling, opening/closing a selector,
selection, or brush movement must not create authored changes.

## Missing texture/failure behavior

Follow M84 conventions. No assignment renders fallback. Invalid
references must not silently become arbitrary textures. Development
diagnostics should be actionable. Release cannot depend on source-only
files. Staging/cooking must include valid referenced runtime textures.

## Cooker/staging/Release

Extend dependency discovery only as required. A valid Terrain texture
referenced by a staged Level must be staged through the existing asset
pipeline. Unrelated source textures must not become runtime dependencies
merely because they exist. Terrain without assignment remains valid.

## Ground

Ground remains unchanged. Do not remove/migrate it or introduce
Ground/Terrain exclusivity. M88 Terrain Materials apply only to Terrain.

## Canonical safety

Do not intentionally modify or normalize line endings of: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

Use disposable/test fixtures. Both canonical diffs must be empty before
completion.

## Automated tests

Cover at minimum: old Terrain syntax; material/tiling roundtrip and
deterministic writer; tiling validation; M84-style texture identity
validation; fallback; flat/rectangular UV mapping; UV stability when
only heights change; workingCopy material does not mutate active before
Apply; Apply promotion; same-value no-op; independence of
heights/material state; GPU sync/caching; texture change/clear/reload;
Level transitions and F2; staged dependency discovery; Release runtime
lookup; M84 regressions; M85/M85.4 lighting regressions; M86 Terrain
regressions; M87 Sculpt regressions.

## Manual acceptance

Verify: assign a visibly detailed grass-like texture; relief becomes
easier to read; tiling changes detail scale; clear restores fallback;
reassign; Raise/Lower/Smooth/Flatten preserve material; working-copy
preview before Apply; Apply; Player/Dynamic Box collision unaffected;
lighting/shadows/local lights correct; Save/reload persists
texture+tiling+relief; switching texture has no stale resource;
transitions/F2 clean; Release uses staged texture; unassigned Terrain
still renders fallback.

## Required validation

Run:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run directly relevant C++ tests across M84--M87 and M88.

Run applicable Python tests including:

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

Terrain material painting; multiple blended Terrain layers; splat/weight
maps; brush texture painting; automatic slope/height rules; speculative
triplanar work; new PBR pipeline; Terrain normal/roughness/metalness/AO
authoring expansion; grass geometry; trees; vegetation/foliage/scatter;
fauna; resolution resampling; multiple Terrains/tiles;
chunks/streaming/LOD; procedural noise/erosion; holes/caves/overhangs;
navmesh/pathfinding; water/biomes; runtime deformation; Undo/Redo;
generic material/mesh editor; ECS/GUID migration; generalized command
framework; Player Traversal Path; M89+ features.

## Implementation discipline

Inspect repository code/tests/docs before choosing exact names, grammar,
texture-reference representation, UI, defaults/bounds, UV formula,
shader/resource integration, and staging changes. Prefer the smallest
implementation that reuses M84 and extends M86/M87. Do not
opportunistically refactor unrelated systems or implement future
painting/vegetation architecture. If M84 conflicts with the conceptual
representation, adapt to existing architecture and report why. If a
Level Format version bump is necessary, STOP and report.

## Cursor completion report

Report: files changed; exact authored representation; exact Level Format
grammar/backward compatibility; reused M84 texture identity; tiling
defaults/bounds; UV formula; Inspector workflow; working-copy preview;
renderer/shader integration; lighting/shadows; GPU ownership/cache;
Sculpt compatibility; Dirty/no-op; cooker/staging/Release;
missing/invalid texture behavior; lifecycle; tests/results;
Debug/Development/Release builds; Python results; canonical Level diff
status; legitimate Tuning Backlog candidates; intentionally deferred
items.

Then STOP. Do not commit, push, merge, close M88, or begin M89. Manual
acceptance and Git closure are separate steps.
