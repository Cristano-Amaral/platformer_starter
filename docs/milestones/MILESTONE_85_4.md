# Milestone 85.4 --- Local Light Gameplay Linking

## Status

**IMPLEMENTED --- awaiting manual acceptance, not CLOSED**

M85 through M85.3 are CLOSED.

-   Branch: `milestone/85-4-local-light-gameplay-linking`
-   Cursor: **Grok 4.6 High --- Fast OFF**

## Purpose

Extend the existing Pressure Plate gameplay-authoring path so a Pressure
Plate can control **specific repeatable Point Lights and Spot Lights**
introduced by M85.3, while preserving the M85.2 singleton Directional
Light control and existing Door behavior.

``` text
Pressure Plate
├── linkedDoorIndex                 ← existing
├── controlsDirectionalLight        ← existing
└── controlledLocalLights           ← M85.4
    ├── Point Light N
    ├── Point Light M
    └── Spot Light K
```

A plate may control a Door, the singleton Directional Light, and zero or
more local lights simultaneously.

## Effective activation

For each local light:

``` text
effectiveEnabled =
    authoredEnabled &&
    (noControllingPlates || anyControllingPlateActive)
```

Thus authored Enabled=false is master OFF; unlinked lights retain M85.3
behavior; multiple plates use OR semantics. Runtime activation never
writes back to authored Enabled. Reuse the existing Pressure Plate
overlap/active state.

## Authored references

Inspect current Door, lifecycle, Authoring Group and M85.3 index/remap
conventions first. Prefer the smallest typed reference sufficient for
local lights:

``` text
LocalLightTarget
├── kind: Point | Spot
└── index
```

A Pressure Plate owns a deterministic ordered list of targets. No
duplicate identical target, stale reference or cross-kind index
ambiguity.

Do not introduce GUIDs preemptively. If typed index references cannot be
made safe under existing lifecycle/remap conventions without broad
fragile work, STOP and report before introducing a new identity system.

## Level Format v1

Extend Pressure Plate persistence narrowly and backward-compatibly.
Preserve all existing valid 7/8/11/12-token forms. Because targets are
variable-length and typed, prefer an explicit marker-based suffix after
repository inspection, conceptually:

``` text
pressure_plate ... <existing fields...> [lights <count> <kind> <index> ...]
```

The repository determines the exact syntax. Require deterministic writer
output, explicit Point/Spot kinds, strict count/token/bool/index
validation, target range validation, explicit duplicate policy,
old-Level compatibility and existing Level v1 limits. No version bump
unless unavoidable; if unavoidable, STOP and report.

## Pressure Plate Inspector

Add a focused `Controlled Local Lights` section using existing ImGui
conventions. It must allow adding/removing multiple specific Point/Spot
targets and clearly identify kind/index. Preserve Door, Box, Player,
Visible and Controls Directional Light controls. Edit workingCopy;
semantic changes Dirty; no-op does not. No graph editor or generic
receiver picker.

## Runtime

Derive local-light effective activation from authored Enabled, current
Pressure Plate active state and authored target relationships. Do not
persist independent plate-active booleans or mutate authored light
specs. Prefer a small CPU-side resolved activation seam analogous to
M85.2. Renderer consumes effective state but does not own gameplay
logic.

Only effectively enabled valid local lights count toward the existing
M85.3 active-light cap. Preserve its deterministic ordering/overflow and
no-local-shadow behavior.

## Multiple relationships

One plate may control many Point/Spot Lights. Multiple plates may
control the same light using OR semantics. A single plate may
simultaneously control its Door, Directional Light and local lights.
These relationships are independent.

## Duplicate semantics

Duplicating a Pressure Plate alone preserves its references to the
original Door/local lights, matching the conservative existing Door-link
precedent. Do not duplicate targets.

Duplicating a targeted Point/Spot Light creates an independent light;
existing plates continue targeting only the original.

Complete Authoring Group duplication must not invent gameplay-reference
cloning or retargeting.

## Delete/remap

Deleting a targeted Point/Spot removes references to that object and
remaps later indices of the same kind. Point and Spot namespaces remap
independently. No stale/out-of-range references.

Deleting a Pressure Plate changes no authored light data; runtime
control resolves from remaining plates or authored Enabled when none
remain.

Preserve Door and Authoring Group remapping.

## Authoring Groups

M85.3 grouping remains intact. Gameplay links target actual Point/Spot
objects, never Authoring Groups. No group-as-gameplay-receiver
semantics.

## Editor/F2/lifecycle

Preserve authored editor preview versus gameplay runtime semantics. No
transient activation leakage or GPU duplication through F2.

Verify fresh load, Restart, Apply, Save/reload, transition, Play Again,
Main Menu→Play, death/checkpoint and F2. Activation is always derived
from current authored links plus real current plate overlap.

## Canonical safety

Do not intentionally modify `level_01.level` or `level_02.level`; do not
normalize EOLs. Use fixtures/disposable Levels. If migration is
necessary, STOP and report first.

## Required tests

Cover old/new Pressure Plate syntax; Point/Spot/mixed/multiple targets;
malformed marker/count/kind/index; out-of-range and duplicate policy;
deterministic roundtrip.

Cover unlinked authored behavior; inactive/active plate; authored master
OFF; one plate→many lights; many plates→one light OR;
Door+Directional+local simultaneous control; Box/Player activation;
visible presentation-only; effective-OFF lights excluded from renderer
cap.

Cover Inspector add/remove and Dirty/no-op.

Cover duplicate plate, duplicate targeted light, delete/remap for Point
and Spot independently, Authoring Group duplication non-retargeting,
Apply/Save/reload, Restart, transition, Play Again, Main Menu,
death/checkpoint and F2.

Regress M85.2 Directional control, Door behavior, M85.3 Point/Spot
rendering and Spot Rotate/groups, Player, materials, Static Props, Item
Pickups/highlights, thumbnails/Preview and Release.

## Manual acceptance

In a disposable Level: 1. Create multiple Point/Spot Lights and a
Pressure Plate. 2. Link several local lights to one plate. 3. Authored
Enabled ON + inactive plate =\> targeted lights OFF. 4. Activate by
Dynamic Box =\> ON; deactivate =\> OFF. 5. Test Player activation. 6.
Authored Enabled OFF cannot be overridden. 7. Two plates targeting one
light use OR. 8. One plate controls several Point/Spot Lights. 9. Same
plate also controls Directional Light and a Door. 10. Duplicate plate
preserves original targets. 11. Duplicate targeted light is not
automatically targeted. 12. Delete/remap targeted lights and inspect
references. 13. Verify grouped fixtures still move/rotate. 14. Apply,
Save/reload, Restart, transition and F2. 15. Verify renderer-cap
behavior and no local-light shadows. 16. Verify Release gameplay
activation, staged-only behavior and existing Directional shadows.

Manual acceptance is mandatory before Git closure.

## Out of scope

No generic Event/Receiver/Action framework, event bus, connection graph,
generic receiver IDs, Pressure Plate→Authoring Group,
Door/Item/Goal→lights, Switch/Trigger, toggle/latch, AND/XOR/inversion,
timers/delays/sequencers, gameplay modulation of intensity/color/range,
multiple Directional Lights, local-light shadows, area lights,
deferred/Forward+/clustered renderer, HDR/bloom/fog/skybox/day-night,
Terrain/M86, Undo/Redo, Player animation, ECS/scene graph rewrite, or
GUID system unless index references are proven unsafe and work is
stopped for review.

## Validation

Run relevant C++ suites plus:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Build Debug/Development/Release using the standard Windows presets. Run
`git diff --check`, canonical Level diffs and `git diff --stat`.

## Cursor completion report

Report exact files, reference architecture, final Level syntax,
parser/writer compatibility, Inspector UI, runtime resolution,
effectiveEnabled rule, multiple-target/controller behavior, simultaneous
Door/Directional/local behavior, renderer-cap interaction,
Duplicate/Delete/remap, groups, lifecycle/F2/Release, regressions,
tests/builds, diff checks and deferred scope.

Then STOP. Do not commit, push, merge, start M85.5/M86 or mark M85.4
CLOSED. Wait for manual acceptance and separate Git closure.
