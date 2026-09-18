# Milestone 76 --- Transform Snapping & Authoring Productivity

**Status:** CLOSED\
**Branch:** `milestone/76-transform-snapping-authoring-productivity`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Improve practical Level-building speed and precision in the Development
editor with deterministic snapping for the existing transform gizmos.
M76 directly addresses the authoring bottleneck observed after M74/M75
without changing gameplay or Level Format v1.

Support snapping for existing Translate, Resize, Scale, and Rotate
operations only where those operations are already supported.

## Defaults

-   Translate: `0.25` world units.
-   Resize: `0.25` world units.
-   Scale: `0.10`.
-   Rotate: `15` degrees.

Add a compact global Snap toggle and contextual increment control near
the existing transform/gizmo controls. Validate/clamp invalid
increments.

## Temporary inversion

Provide one non-conflicting conventional modifier which temporarily
inverts snapping during manipulation: enabled -\> temporarily unsnapped;
disabled -\> temporarily snapped. It must not change the persisted
toggle. Inspect current controls before choosing the key and report the
conflict analysis.

## Deterministic semantics

Implement snapping through narrow testable authoring/math helpers rather
than embedding quantization arithmetic in ImGui. Define/test positive,
negative, boundary, half-increment, repeated-manipulation, and
floating-point-drift behavior.

Preserve current drag authority. Quantize the intended authored result;
do not accumulate error by repeatedly quantizing runtime/transient
transforms.

## Authority

Preserve `workingCopy -> Apply -> active -> Save`. Runtime Jolt motion
stays transient unless an explicit authoring operation changes authored
data. Dynamic Box snapping must not capture runtime motion. Item Pickup
keeps logical-position versus visual-transform authority; Rotate
continues editing `visualRotationDegrees`.

## Persistence

Persist Snap enabled state and Translate/Resize/Scale/Rotate increments
using the existing editor layout/preferences mechanism. Older layouts
load M76 defaults. Invalid persisted values safely fall back/clamp. No
new settings file or format version.

## Object coverage

Support all and only object/mode combinations already supported by the
current gizmos. Do not expand transform capabilities as scope creep.
Report the actual coverage matrix.

## Visual scope

Keep the current gizmo visually authoritative. Add only compact
controls/state feedback. No viewport grid, rulers, smart guides,
magnetic object snapping, surface snapping, or vertex snapping.

## Canonical Level safety

Do not intentionally modify `game/assets/source/levels/level_01.level`
or `level_02.level`. Manual tests use disposable edits. Canonical diffs
must be semantically empty before closure. Do not normalize EOLs.

## Future agent-authoring readiness

The snapping/quantization and authored-transform application logic
introduced by M76 must be callable/testable independently of ImGui. This
is deliberate preparation for future human+agent authoring.

This does NOT authorize CLI, JSON command protocols, MCP, scripting,
remote control, generic AuthoringCommand, or generalized authoring
framework work. CLI readiness is an architectural consideration, not a
requirement of every milestone.

## Regression coverage

Prove at minimum: 1. Translate 0.25 snapping for positive/negative
values. 2. Stable boundary and half-increment behavior. 3. Resize 0.25
while preserving minimum dimensions. 4. Scale 0.10 while preserving
constraints. 5. Rotate 15 degrees. 6. Item Pickup Rotate still edits
`visualRotationDegrees`. 7. Snap disabled preserves unsnapped behavior.
8. Modifier inversion works without mutating persisted toggle. 9.
Repeated snapping avoids meaningful drift. 10. workingCopy/Apply/Save
authority remains correct. 11. Dynamic Box runtime motion is not
captured. 12. layout round-trip persists M76 preferences. 13. old layout
loads defaults. 14. invalid persisted increments are safe. 15.
unsupported object/mode combinations remain unsupported. 16. runtime
gameplay behavior is unchanged. 17. canonical Levels remain unchanged.

## Validation

Run directly affected current C++ tests for transform gizmos, Rotate
Gizmo, EditorWorkspace, authored lifecycle, editor layout persistence,
Level validation, Physics rebuild, Item Pickup visual transforms, and
Dynamic Box authored/runtime separation. Run newer relevant repository
tests discovered during inspection.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Then:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
```

## Manual acceptance

Verify Snap UI; Translate default/custom/negative coordinates; Resize;
Scale on currently supported objects; Rotate on Static Prop and Item
Pickup; modifier inversion; contextual increments; persistence;
Apply/Save; Dynamic Box transient-motion safety; normal gameplay/F2
roundtrip; and no canonical Level changes.

Automated green is insufficient.

## Out of scope

No CLI, agent API, MCP, scripting, generalized Authoring Operations
framework, Undo/Redo, multi-selection, group transforms, viewport grid,
smart alignment, object/surface/vertex snapping, drag-and-drop
placement, new object types, new gameplay mechanics, enemies, Level
Format v2, menu/audio/settings work, or M77 functionality.

## Documentation

Canonical active document: `docs/milestones/MILESTONE_76.md`. Preserve
M74 and M75 as CLOSED. Keep `docs/MILESTONES.md` compact. Update
architecture/README/AGENTS only when implementation requires it. After
implementation M76 is **implemented, awaiting manual acceptance**, not
CLOSED.

## Cursor report and STOP

Report files changed; discovered gizmo architecture; snapping
helper/math and exact rounding/tie behavior;
Translate/Resize/Scale/Rotate behavior; actual coverage matrix;
modifier/conflict analysis; drift behavior; authority preservation;
Dynamic Box and Item Pickup semantics; layout persistence/backward
compatibility; automation-readiness boundary; C++/Python/build results;
`git diff --check`; canonical Level diffs; and confirmation that no
CLI/agent API/MCP/Undo/multi-select/grid/smart snapping/new gameplay/M77
work was added.

Then STOP.

Do NOT commit.\
Do NOT push.\
Do NOT merge.\
Do NOT start M77.\
Do NOT mark M76 CLOSED.
