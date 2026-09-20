# Milestone 85.2 --- Directional Light Gameplay Activation & Authoring Polish

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/85-2-directional-light-activation`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Build on M85.1 with two narrow capabilities:

1.  Directional Light authoring-visualization polish:
    -   Translate the editor-only visualization anchor.
    -   Scale the editor-only visualization.
    -   Rotate continues to change the real authored light direction.
2.  Directional Light gameplay activation:
    -   Allow Pressure Plates to control the singleton Level Directional
        Light.
    -   Separate authored enablement from transient runtime activation.
    -   Do not create a generic event/receiver framework.

## Core semantics

Directional Light remains infinite/direction-only. Translate and Scale
affect only its editor representation; they must not alter illumination,
range, intensity, attenuation, shadow coverage, or light physics. Rotate
edits `directionalRayDirection`.

Prefer editor-layout/settings persistence for visualization anchor/scale
if a suitable repository convention exists. Otherwise keep deterministic
transient defaults. Do not expand Level gameplay syntax merely for
cosmetic visualization state.

## Gameplay activation

Extend Pressure Plate narrowly to optionally control the singleton
Directional Light while preserving existing Door behavior and
`linkedDoorIndex`.

Preferred effective rule:

`effectiveEnabled = authoredEnabled && (noLinkedPlates || anyLinkedPlateActive)`

Thus: - authored Enabled=false is a master OFF; - no linked plate
preserves M85.1 behavior; - linked plates use OR semantics; - active
linked plate enables the directional light; - all linked plates inactive
disable it; - ambient remains when directional light is effectively
off; - Shadows Enabled remains an independent authored setting when the
light is effectively on.

Reuse the existing Pressure Plate active state (Box/Player overlap
flags). Do not create another overlap system.

Expose a simple Pressure Plate Inspector control such as
`Controls Directional Light`, rather than an index or generic receiver
selector.

Choose the exact Level Format extension only after inspecting current
M57.1 Pressure Plate syntax/parser/writer. Existing syntax must remain
backward compatible and deterministic.

## Lifecycle

Transient activation must rebuild/reset correctly on fresh load,
Restart, Apply/reload, Level transition, Play Again, MainMenu→Play,
death/checkpoint overlap changes, and F2. No source-Level activation
state may leak into a destination Level. Do not recreate M85 shadow GPU
resources per activation/edit.

## Editor polish

Translate: - moves only the sun/icon/arrow visualization anchor and
picking proxy; - follows translation snapping where applicable; - does
not change ray direction or lighting.

Scale: - changes only visualization size, preferably uniformly; -
remains positive/bounded; - follows existing scale snapping where
applicable; - does not change intensity, direction, range, or shadow
coverage.

Rotate: - preserves M85.1 semantics; - edits normalized authored ray
direction; - updates Inspector, lighting and shadows live; - marks
authored Level data Dirty on semantic change.

If visualization transform is layout/transient state, Translate/Scale
must not Dirty authored Level gameplay data.

## Compatibility

Preserve: - Pressure Plate → Door behavior; - Door
required-item/obstruction semantics; - M78--M82 selection/groups; - M83
Player; - M84 materials; - M85 lighting/shadows; - M85.1
Environment/Directional authoring; - Item Pickup/highlight; - Static
Props; - thumbnail/Model Preview isolation; - Development/Release
separation.

Directional Light remains singleton and cannot Duplicate/Delete/Group.

Do not intentionally modify canonical `level_01.level` or
`level_02.level`; use test fixtures/disposable Levels. Do not normalize
EOLs.

## Required regression coverage

Cover old/new Pressure Plate syntax, roundtrip/writer validation,
Inspector Dirty/no-op, unlinked behavior, linked inactive/active
behavior, authored master OFF, multiple plate OR semantics, Door-only
and Door+Light compatibility, Box/Player activation,
Restart/Apply/reload/transition/Play
Again/MainMenu→Play/death/checkpoint/F2, shadow-pass behavior,
Translate-only visualization, Scale-only visualization, Rotate
regression, picking after Translate/Scale, snapping, singleton lifecycle
exclusions, M78--M85.1 regressions, Release isolation, no generic event
framework, and canonical Level safety.

## Manual acceptance

Development: - select Directional Light; - Translate: representation
moves but lighting/shadows do not; - Scale: representation size changes
but lighting/shadows/intensity do not; - Rotate: actual light/shadow
direction changes; - verify picking and snapping; - configure a
disposable/test Pressure Plate to control Directional Light; - authored
Enabled ON + plate inactive =\> directional OFF, ambient remains; -
activate plate with configured Player/Box =\> directional ON; -
deactivate =\> OFF; - verify shadows track effective state; - Shadows
Enabled OFF still permits directional illumination when effectively
ON; - authored Enabled OFF cannot be overridden by plate; - test
multiple linked plates if practical; - existing Door-linked plate
remains correct; - Restart, Apply/reload, F2 and transition lifecycle.

Release: - linked plate controls light; - ambient remains while
directional OFF; - shadows behave correctly; - Door/Plate gameplay
remains correct; - no editor sun/arrow; - no source dependency.

Automated green is insufficient.

## Validation

Run directly affected C++ Level Format, Pressure Plate, Door, editor
selection/gizmo/picking/layout/lifecycle, lighting/shadow,
Player/material/item/static-prop and game-flow tests.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Build Debug, Development and Release using the standard Windows presets.

Then run:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
```

## Out of scope

No multiple Directional Lights, Point/Spot Lights, arbitrary Light
objects, generic Source/Receiver/Event/Action framework, event bus,
receiver IDs, arbitrary target lists, connection graph editor, AND/XOR
logic, Switch/Trigger objects, Item/Goal/Door → Light, light
animation/flicker, day/night cycle, skybox/fog/HDR/bloom, CSM, PBR
expansion, Terrain, Undo/Redo, player animation, ECS, scene graph, or
M86 functionality.

## Documentation and STOP

Canonical active document: `docs/milestones/MILESTONE_85_2.md`.

Preserve M85 and M85.1 as CLOSED. Keep `docs/MILESTONES.md` compact.
Document visualization Translate/Scale vs real Rotate, persistence
choice, exact Pressure Plate syntax, effective enable rule, OR
semantics, lifecycle, Door compatibility, and absence of a generic
event/receiver system.

After implementation M85.2 is **implemented, awaiting manual
acceptance**, not CLOSED.

Cursor must report files, inspected architecture, visualization
state/persistence, Translate/Scale/Rotate, exact Level syntax,
parser/writer compatibility, Inspector, runtime activation/effective
formula/multiple plates, Door compatibility, activation sources,
shadows/lifecycle, regressions, Release, tests/builds/diffs and explicit
out-of-scope confirmation.

Then STOP.

Do NOT commit. Do NOT push. Do NOT merge. Do NOT start M86. Do NOT mark
M85.2 CLOSED.

## Implemented behavior

Visualization Translate moves only the editor sun/icon/arrow/picking
proxy. Visualization Scale changes only representation size (uniform,
positive, clamped to `[0.25, 8]`, default `1`). Rotate still edits
normalized `directionalRayDirection` and Dirties authored Level data.

The visualization transform is **transient session state**
(`LevelEditorState::directionalLightVisualization`). It is not Level
Format and not `editor_layout.ini`. The layout file is global, not
per-Level; storing cosmetic light gizmos there would leak across
Levels. Defaults `{0, 8, 0}` / scale `1` preserve M85.1. Translate/Scale
do not Dirty authored gameplay data. The state resets on authored Level
switch, runtime reload, Level transition, Play Again, and Main Menu →
Play. It survives F2 close/reopen.

Pressure Plate Level syntax is backward compatible:

```
pressure_plate <cx> <cy> <cz> <sx> <sy> <sz>
    [<doorIndex> [<activateByDynamicBox> <activateByPlayer> <visibleInGameplay>
    [<controlsDirectionalLight>]]]
```

7 / 8 / 11-token records remain valid (`controlsDirectionalLight = 0`).
The writer always emits 12 tokens. Malformed `0`/`1` values are
rejected. `linkedDoorIndex` is unchanged. One plate may control its Door
and the singleton Directional Light at the same time.

Effective enablement:

`effectiveEnabled = authoredEnabled && (noLinkedPlates || anyLinkedPlateActive)`

Multiple linked plates use OR. Authored Enabled is a master OFF. No
AND/XOR/inversion. Activation reuses M57.1 Box/Player overlap; there is
no second collision system and no event/receiver framework.

Editor preview (F2) continues to use authored Enabled only. Gameplay and
Release resolve effective state from live plate overlap. Effective OFF
skips directional diffuse and the shadow pass; ambient remains. Shadows
Enabled stays independent while the light is effectively ON. M85 shadow
GPU resources are not recreated per activation.

Restart/Apply/reload/transition/Play Again/Main Menu → Play rebuild from
destination authored data plus current overlap. Death/checkpoint follow
existing overlap authority (no stored plate-active bool).
