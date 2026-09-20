# Milestone 85.3 --- Local Lights Foundation

## Status

**DEFINED --- implemented, awaiting manual acceptance, not CLOSED**

M85, M85.1 and M85.2 are CLOSED.

-   Branch: `milestone/85-3-local-lights-foundation`
-   Cursor: **Grok 4.6 High --- Fast OFF**

## Purpose

Introduce the first repeatable spatial local lights while preserving the
existing M85--M85.2 global Environment/Directional Light architecture.

M85.3 adds repeatable **Point Lights** and **Spot Lights**, Level Format
v1 persistence, Development Editor authoring, runtime rendering,
lifecycle operations, deterministic renderer limits, tests and Release
support.

Gameplay linking of local lights is deliberately deferred. M85.2
Pressure Plate → singleton Directional Light behavior remains unchanged.

## Architecture

``` text
World Lighting
├── Environment
│   ├── Ambient Light
│   └── Directional Light        ← singleton/global
└── Local Lights                 ← repeatable
    ├── Point Light
    │   ├── position
    │   ├── color
    │   ├── intensity
    │   ├── range
    │   └── enabled
    └── Spot Light
        ├── position
        ├── direction
        ├── color
        ├── intensity
        ├── range
        ├── inner cone angle
        ├── outer cone angle
        └── enabled
```

Directional Light remains infinite/direction-only and singleton. Do not
turn it into a positional or repeatable light.

## Point Light

Authored data: - world-space position; - RGB color, finite and
`[0,1]`; - finite, non-negative, bounded intensity; - finite, positive,
bounded range; - exact authored boolean `enabled`.

Illumination attenuates with distance and has no meaningful contribution
outside authored range. Choose deterministic defaults/bounds after
repository inspection and document them.

No physical units, temperature, IES, cookies, volumetrics or local-light
shadows.

## Spot Light

Authored data: - world-space position; - finite, non-zero normalized
direction; - RGB color; - intensity; - range; - inner cone angle; -
outer cone angle; - enabled.

Require a valid deterministic cone relationship such as
`0 <= inner < outer`, with explicit practical bounds selected after
repository inspection. Illumination attenuates by distance and cone
angle and contributes nothing meaningfully outside range/outer cone.

No Spot shadows in this milestone.

## Level Format v1

Extend the existing format narrowly with repeatable records. Select the
final exact syntax only after inspecting current parser/writer
conventions. Preferred conceptual shape:

``` text
point_light <px> <py> <pz> <r> <g> <b> <intensity> <range> <enabled>
spot_light <px> <py> <pz> <dx> <dy> <dz> <r> <g> <b> <intensity> <range> <innerConeDegrees> <outerConeDegrees> <enabled>
```

Requirements: - repeatable records and deterministic writer output; -
strict token counts and exact bool parsing; - finite validation and
documented bounds; - normalized Spot direction; - malformed records
rejected; - old Levels remain valid and semantically unchanged; -
existing Level v1 size/line constraints remain; - no format-version bump
unless inspection proves unavoidable; if unavoidable, STOP and report
first.

Do not intentionally modify canonical `level_01.level` or
`level_02.level`.

## Renderer

Integrate Point/Spot Lights into the existing M84/M85 world-lit
material/rendering path, not a parallel renderer.

Preserve Ambient, Directional Light and directional shadows exactly.
Point/Spot Lights affect appropriate existing world-lit
geometry/materials. Preserve Base Color/Base Color Texture, highlights
and material restoration.

Use a **small fixed deterministic maximum active local-light count**
appropriate for the current forward renderer and shader/uniform
constraints. Inspect first, select/document/test the cap and
deterministic overflow behavior.

Prefer a simple forward uniform-array/light-loop solution if consistent
with the repository. Do not introduce deferred, Forward+, clustered
rendering or unbounded dynamic-light infrastructure.

Local lights do **not** cast shadows in M85.3. Do not add cubemap
shadows, local depth maps, shadow atlases or local shadow toggles.

## Editor hierarchy and creation

Expose repeatable local lights using existing hierarchy and creation
conventions, conceptually:

``` text
Environment
├── Environment
└── Directional Light

Lights
├── Point Light
├── Point Light
├── Spot Light
└── Spot Light
```

Point/Spot Lights are normal authored repeatable objects. Creation must
use deterministic defaults, select using existing conventions, mark
workingCopy Dirty and obey Apply/Save authority.

No separate Lighting Editor window.

## Inspector

Point Light: - Enabled - Position - Color - Intensity - Range

Spot Light: - Enabled - Position - Direction - Color - Intensity -
Range - Inner Cone Angle - Outer Cone Angle

Edits target `workingCopy`; semantic changes Dirty; no-op does not.
Validation/clamping follows established patterns.

No gameplay-link controls in M85.3.

## Gizmos

Point Light: - Translate edits real authored position. - Rotate must not
invent orientation. - Scale must not mean intensity. It may map to
authored Range only if current gizmo conventions make this clean and
unambiguous; otherwise Range remains Inspector-only.

Spot Light: - Translate edits real authored position. - Rotate edits
real authored direction, preferably reusing the proven normalized
world-axis signed-angle approach from M85.1/M85.2. - Scale must not mean
intensity. It may map to Range only if clean/unambiguous; otherwise
Inspector-only.

M76 snapping applies to supported operations. Do not add fake authored
position/scale semantics to Directional Light.

## Visualization and picking

Development-only Point visualization communicates origin and approximate
range. Spot visualization communicates origin, direction, outer
cone/range and optionally inner cone if uncluttered.

Visualization: - follows authored state; - is pickable through existing
editor conventions; - has no physics/gameplay role; - does not itself
cast/receive world lighting as gameplay geometry; - never appears in
Release; - requires no source asset.

Prefer simple editor/raylib primitives.

## Duplicate/Delete, selection and groups

Integrate Point/Spot Lights with existing typed selection and M79
lifecycle: - independent Duplicate; - Delete; - deterministic
index/order remap; - no stale selection/runtime state; - Ctrl
multi-selection consistent with M78.

For M81/M82 authoring groups, inspect current typed-member architecture.
Add local lights only if narrow integration is clean. If supported,
group Translate moves positions; Spot group Rotate may update both
pivoted position and direction only when consistent with M80. Point
Light has no orientation. Do not invent group Scale.

M85.3 uses that narrow typed-member extension: Point/Spot Lights are
valid Authoring Group members. Environment and Directional Light remain
ungroupable. Do not introduce a scene graph, parent-child transforms,
or GUIDs.

No GUID/reference framework is needed because M85.3 introduces no
gameplay references to local lights.

## Authored lifecycle

Preserve: `workingCopy -> Apply -> active authored Level -> runtime`.

Inspector/gizmo edits workingCopy. Apply validates/promotes. Save
persists. Reload reconstructs. Restart uses active authored state. Level
transition loads destination lights without source leakage. Play
Again/Main Menu → Play starts fresh staged `level_01`. F2 must not
duplicate lights or leak renderer state.

## Compatibility

Preserve M83--M85.2 behavior including Player, M84 materials, Static
Props, Item Pickups/highlight, Ambient, singleton Directional Light,
directional shadows, M85.1 authoring, M85.2 Directional visualization,
M85.2 Pressure Plate activation, Doors, Pressure Plates,
thumbnails/Preview, transitions and Release staging.

The M85.2 Directional effective rule remains exactly:

``` text
effectiveEnabled =
    authoredEnabled &&
    (noLinkedPlates || anyLinkedPlateActive)
```

## Explicitly out of scope

Do not implement: - multiple Directional Lights; - Pressure Plate →
Point/Spot linking; - Linked Light / Controlled Lights / target lists /
light receiver IDs; - generic Source/Receiver/Event/Action framework or
event bus; - connection graph; - AND/XOR/inversion; - Switch/Trigger; -
Door/Item/Goal → Light activation; - Point/Spot shadows or shadow
atlas/cubemap shadows; - area/rect/tube lights; - emissive-as-light,
cookies, IES, volumetrics, physical units/temperature; -
flicker/animation/day-night; - skybox/fog/HDR/exposure/bloom/CSM; -
deferred/Forward+/clustered rendering; - PBR expansion; - Terrain/M86; -
Undo/Redo; - Player animation/Character Definition; - ECS/scene
graph/general AssetManager rewrite; - GUID/reference infrastructure for
future links.

## Required automated coverage

Add focused coverage for: - valid Point/Spot records, repeated lights
and deterministic roundtrip; - malformed token counts, bools, non-finite
values, range/intensity/cone validation and zero Spot direction; - old
Levels with no local lights; - Point distance attenuation/range; - Spot
distance/angular attenuation; - disabled lights; - deterministic
active-light cap/overflow; -
create/hierarchy/selection/picking/Inspector Dirty/no-op; - Point
Translate/snapping; - Spot Translate/Rotate/snapping; - visualization; -
Duplicate/Delete; - multi-selection/group supported behavior or explicit
restriction; - Apply/Save/reload/Restart/transition/Play Again/Main
Menu/F2; - no stale local-light state; - regressions for Player,
materials, M85 shadows, M85.1, M85.2 activation, Door/Pressure Plate,
pickups/highlights, Static Props, thumbnails/Preview and Release.

## Validation

Run relevant C++ tests and:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Pre-report safety:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

Canonical Level diffs must remain empty unless an unavoidable migration
is discovered; if so, STOP and report before changing them.

## Manual acceptance

Development: 1. Create several Point Lights; verify independent
hierarchy/view objects. 2. Translate and confirm real illumination
origin moves. 3. Change color/intensity/range/enabled. 4.
Duplicate/delete and verify independence. 5. Create several Spot Lights.
6. Translate and Rotate; verify real origin/direction. 7. Change
color/intensity/range/inner/outer cone. 8. Verify visualization/picking
follows authored values. 9. Verify Ambient/Directional and directional
shadows remain correct. 10. Verify M85.2 Pressure Plate still controls
only singleton Directional Light. 11. Apply, Save/reload, Restart,
transitions, Play Again and F2. 12. Exercise Player, Static Props,
materials and Item Pickups under local lights. 13. Confirm local lights
do not incorrectly cast shadows.

Release: 1. Point/Spot lighting works. 2. No editor light
icons/cones/gizmos. 3. Staged-only operation. 4. Existing Directional +
Pressure Plate behavior remains correct. 5. No source-directory
dependency.

Manual acceptance is mandatory before Git closure.

## Cursor completion report and STOP

Report exact files, final Level syntax, defaults/bounds, renderer
architecture, active-light cap/overflow policy, editor behavior,
lifecycle/group behavior, regressions, tests/builds, `git diff --check`,
canonical Level diffs and limitations.

Then STOP.

Do not commit. Do not push. Do not merge. Do not start M85.4 or M86. Do
not mark M85.3 CLOSED. Wait for manual acceptance and separate Git
closure.

## Implementation notes

Implemented on `milestone/85-3-local-lights-foundation` without a format
version bump. Canonical `level_01` / `level_02` are unchanged.

Exact Level Format v1 records (after `directional_light`, omitted when empty):

``` text
point_light <px> <py> <pz> <r> <g> <b> <intensity> <range> <enabled>
spot_light <px> <py> <pz> <dx> <dy> <dz> <r> <g> <b> <intensity> <range> <innerConeDegrees> <outerConeDegrees> <enabled>
```

Defaults: Point/Spot color `{1, 0.95, 0.85}`, intensity `1.5`, range `8`,
enabled `1`. Spot direction `{0, -1, 0}`, inner cone `20°`, outer cone `35°`.
Bounds: color `[0,1]`, intensity `[0, 4]`, range `[0.1, 64]`, cones
`0 <= inner`, `outer <= 89`, `outer - inner >= 0.5`. Spot direction is
normalized at the parser/writer/gizmo boundary.

Attenuation: distance `(1 - d/range)^2` with hard cutoff at range. Spot
angular lerp from inner cosine (full) to outer cosine (zero). Active cap is
`8` enabled lights, Points then Spots in authored order; extras are dropped.
Local lights do not cast shadows. Scale is not mapped to Range. Point/Spot
Lights are valid M81/M82 Authoring Group members (`point_light` /
`spot_light` typed tokens). A selected Spot Light is native Rotate-capable:
the Rotate gizmo appears at authored position and edits
`SpotLightSpec::direction` (Rodrigues, M76 snap, no Euler field). Spot may
be Group Rotate PRIMARY. Point Light has no orientation and cannot start
Rotate. Group Translate uses the existing M78 shared
delta. Group Rotate orbits Point position only; Spot position orbits and
Spot direction receives the same world-axis Rodrigues delta. Environment
and Directional Light remain ungroupable. Ctrl multi-select remains.
Picking uses a `0.7` cube at the origin, not the range volume.
