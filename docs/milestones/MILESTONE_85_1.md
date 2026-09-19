# Milestone 85.1 --- Environment & Directional Light Authoring

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/85-1-environment-light-authoring`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Turn M85's fixed project lighting defaults into explicit Level-authored
Environment and Directional Light data with Development Editor
authoring.

``` text
Level
└─ Environment
   ├─ Ambient
   │  ├─ Color
   │  └─ Intensity
   └─ Directional Light
      ├─ Enabled
      ├─ Direction
      ├─ Color
      ├─ Intensity
      └─ Shadows Enabled
```

The author can inspect/edit lighting, manipulate light direction
visually, preview changes live, Apply/Save/reload them, and use the
authored result in Release.

## Architecture and compatibility

Inspect M85 LightingEnvironment/WorldLighting, LevelDefinition/Level
Format v1, editor workingCopy/active/Dirty/Apply/Save authority, M82
Hierarchy, typed selection/groups, Inspector, Rotate gizmo, picking,
F2/reload/transition lifecycle and Release paths.

Add the smallest singleton Level environment representation. Existing
Levels without environment records must retain M85-compatible defaults:
ambient white 0.34, directional ray direction normalized
`(-0.42,-1,-0.38)`, warm color `(1,0.96,0.88)`, intensity 0.88,
directional enabled, shadows enabled.

Extend Level Format v1 narrowly and deterministically. Preserve limits
and backward compatibility. Reject/normalize invalid finite values
according to existing conventions. No generic component/property syntax.

## Editor authority

Preserve:

``` text
workingCopy → Apply → active
workingCopy → Save → Level file
```

Lighting edits set Dirty only on semantic change. Live editor preview
may reflect workingCopy using existing preview-authority conventions
without silently promoting unrelated authored data.

## Hierarchy and selection

Expose singleton authoring nodes conceptually as:

``` text
Environment
└─ Directional Light
```

Environment selects ambient settings. Directional Light selects
directional settings and viewport visualization.

These are not repeatable normal objects: no Duplicate/Delete/Group, no
Authoring Groups, and no invalid Ctrl multi-selection/group composition.

## Inspector

Environment: Ambient Color, Ambient Intensity.

Directional Light: Enabled, Direction, Color, Intensity, Shadows
Enabled.

Direction remains normalized/canonical. Numeric edits and gizmo edits
agree. `Enabled=false` disables directional contribution and directional
shadows while ambient remains. `Shadows Enabled=false` disables shadows
while directional illumination remains.

## Viewport visualization and gizmo

Add an editor-only sun/light representation plus direction indication.
Its displayed position is only an authoring anchor and does not affect
illumination.

Provide visual direction manipulation, preferably reusing existing
Rotate gizmo math where practical without pretending the light has a
gameplay transform. The gizmo edits direction only, produces normalized
deterministic results, updates Inspector and lighting/shadows live, and
sets Dirty correctly. No Translate/Scale/Resize lighting semantics.

The visualization is Development-only, does not cast shadows, has no
physics/gameplay interaction and never appears in Release gameplay.

## Runtime/lifecycle

Resolve M85 LightingEnvironment from the active Level-authored
environment; do not duplicate the renderer lighting system.

Transitions adopt destination lighting with no stale source-Level state.
Restart, Play Again, MainMenu→Play and reload use the appropriate Level
environment.

M85 shadow resources remain persistent; authoring changes update
configuration rather than recreating GPU resources per frame/edit.

## Future gameplay activation seam

Persist/authored `Directional Light Enabled` now and keep a clean future
seam for gameplay-driven activation/deactivation, conceptually allowing
later separation of authored enabled state and transient gameplay
override.

Do NOT implement Pressure Plate→Light, Door-style links, trigger
targets, receiver IDs, event bus, generic activation graph or any
gameplay-driven light link in M85.1.

## Canonical Level safety

Do not intentionally modify `level_01.level` or `level_02.level` to
demonstrate the feature. Old Levels must work through defaults. Use
disposable/reversible Level workflows for persistence testing. Do not
normalize EOLs.

## Regression coverage

Cover old-Level defaults; parse/write roundtrip; deterministic writer;
duplicate singleton handling; validation; Dirty/no-op semantics;
Apply/Save/reload; Hierarchy rows; singleton operation restrictions;
selection/Inspector sync; gizmo direction-only normalized edits; live
ambient/directional preview; Enabled and Shadows Enabled;
transitions/Restart/Play Again/MainMenu→Play/F2; no per-edit shadow
resource recreation; M83 Player; M84 materials; Item Pickup/highlight;
Static Props; M78--M82 groups; thumbnail/Preview isolation; Release;
absence of gameplay light-link syntax; canonical safety; relevant
M69--M85 regressions.

## Manual acceptance

Development: select Environment, edit Ambient Color/Intensity and
observe live result; select Directional Light, edit
Color/Intensity/Direction; manipulate direction with gizmo and observe
shadows move; verify Inspector sync; toggle shadows; toggle light and
confirm ambient remains; Apply; Save/reload on disposable Level; verify
Level switching/lifecycle; check grid/gizmos/groups, textured prop, Item
Pickup/highlight, Player and F2.

Release: old Levels still use deterministic defaults; lighting/shadows
work; gameplay/transitions/Restart work; editor light visualization is
absent; no source dependency.

Automated green is insufficient.

## Validation

Run directly affected C++
Level/editor/selection/gizmo/lifecycle/lighting/material/player/item/static-prop
tests and relevant M69--M85 regressions.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Build all three configurations, then `git diff --check` and canonical
Level diffs.

## Out of scope

No Point/Spot lights, multiple Directional Lights, arbitrary repeatable
lights, gameplay trigger/link/receiver behavior, Pressure Plate→Light
links, generic events/event bus/receiver system, day/night cycle,
animated sun, skybox/fog/HDR/bloom, CSM, PBR expansion, lighting
presets, environment asset files, undo/redo, player animation, Character
Definition, Terrain, ECS, scene graph, generic asset-manager rewrite, or
M86 functionality.

## Documentation and STOP

Canonical active document: `docs/milestones/MILESTONE_85_1.md`. Preserve
M76--M85 as CLOSED and keep `docs/MILESTONES.md` compact.

Document Level environment schema/defaults, editor authority,
hierarchy/selection, direction convention/gizmo, enabled vs
shadows-enabled, lifecycle, future gameplay activation seam, and
explicit absence of gameplay linking.

Cursor report must cover files, prior architecture, data
model/syntax/defaults/parser/writer, authority,
Hierarchy/selection/Inspectors, visualization/gizmo/live preview,
enabled semantics, future activation seam, M85 integration/lifecycle,
regressions, Release, tests/builds/diffs and out-of-scope confirmation.

After implementation M85.1 is **implemented, awaiting manual
acceptance**, not CLOSED.

Then STOP.

Do NOT commit. Do NOT push. Do NOT merge. Do NOT start M86. Do NOT mark
M85.1 CLOSED.
