# Milestone 80 --- Multi-Selection Group Rotate

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/80-multi-selection-group-rotate`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Extend M78/M79 multi-selection with deterministic **Group Rotate** for
compatible authored objects. Use the PRIMARY object's authored
world-space anchor as the only group pivot. Persistent Authoring Groups
are deliberately deferred because durable membership/identity and
serialization should not be mixed into this transform milestone.

## Required behavior

With 2+ compatible selected authored objects and Rotate mode: - pivot =
PRIMARY's existing authored Rotate/transform anchor; - PRIMARY world
position stays fixed; - every secondary authored position/center orbits
the PRIMARY pivot by one shared signed world-axis rotation delta; -
every member's existing authored orientation receives that same rotation
delta; - derive results from drag-start authored state to prevent
cumulative drift; - preserve relative composition geometry; - mutate
workingCopy only.

If any selected member lacks safe existing authored Rotate semantics,
refuse the complete group operation. Never rotate only a supported
subset and never invent rotation support/fields.

## World-axis and snapping

Preserve the M58.1 world-axis Rotate gizmo. Use the PRIMARY gizmo anchor
and selected world axis.

Preserve M76 Rotate snapping exactly: snap the PRIMARY/group rotation
result once, derive one shared snapped angle delta, and apply it to all
members. Do not independently snap member rotations. Preserve default
15-degree increment, Snap toggle, Ctrl temporary inversion, dragStart,
half-away-from-zero behavior where applicable, and authored rotations
beyond 360 degrees.

## Authored transform authority

Repository code is authoritative. Inspect current Rotate support,
especially Static Prop and Item Pickup.

Group orbit uses the same authored position/center authority used by M78
Group Translate. Item Pickup must preserve its logical position versus
visual authored rotation semantics. Dynamic Box must never capture Jolt
runtime pose/orientation and remains unsupported if existing Rotate does
not support it. Do not mutate size, scale, bounds, runtime poses, or
unrelated fields.

## Atomicity and compatibility

Validate complete group eligibility before mutation. Failure preserves
workingCopy, Dirty, and selection. Single-selection Rotate remains
unchanged. Inspector remains PRIMARY-only. Group Translate and M79
Duplicate/Delete Selected remain unchanged. Resize/Scale remain
non-group operations.

Selection/lifecycle reconciliation from M78/M79 must remain safe across
Open/Switch/New, Apply/Reload/Revert, F2/Play, Add, Duplicate/Delete and
index shifts.

## Testable boundary

Keep core group-rotation math narrow/testable outside ImGui where
practical:

`drag-start transforms + primary pivot + world axis + requested angle + snap state -> one shared rotation delta -> rotated authored transforms`

No CLI, Agent API, MCP, scripting, AuthoringCommand, scene graph,
transform hierarchy, GUIDs, or persistent groups.

## Canonical safety

Do not intentionally modify `game/assets/source/levels/level_01.level`
or `level_02.level`. Use disposable edits, do not normalize EOLs, and
leave final semantic diffs empty.

## Regression coverage

Cover at minimum: - single Rotate unchanged; - 2+ compatible
eligibility; - unsupported member prevents partial rotation; - PRIMARY
pivot position fixed; - secondary orbit correctness; - shared signed
orientation delta; - preserved pivot distances; - deterministic
90-degree and negative-angle cases; - drag-start/no drift; - shared
Rotate Snap, no per-member snapping; - Ctrl inversion and Snap OFF; -
\>360-degree M76 behavior; - Item Pickup authority; - Dynamic Box no
runtime capture; - workingCopy/Dirty/Apply/Save; - selection
preserved; - M78 Group Translate; - M79 Duplicate/Delete Selected; -
Duplicate Selected -\> immediate Group Rotate when compatible; - M77
Grid; - Gameplay/Release unchanged; - canonical Levels unchanged.

Add explicit pivot/orbit math and drift-prevention regressions.

## Validation

Inspect/run relevant EditorGizmo, EditorSnap, selection/picking,
lifecycle, Static Prop, Item Pickup, Dynamic Box, Workspace,
QuickToolbar, EditorViewportGrid, PhysicsRebuild, LevelFile and newer
relevant tests.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Build all three configurations:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Then run `git diff --check` and inspect semantic diffs for both
canonical Levels.

## Manual acceptance

Using disposable Development edits verify single Rotate; compatible
multi-selection; PRIMARY pivot; world-axis Group Rotate; obvious
90-degree rotation; PRIMARY fixed; secondaries orbit; orientations
rotate together; Snap 15; Snap OFF; Ctrl inversion; repeated drags/no
drift; mixed compatible categories; unsupported-member refusal; Item
Pickup authority; no Dynamic Box runtime capture; Group Translate;
Duplicate Selected -\> Group Rotate; Delete Selected; Dirty/Apply/Save;
Grid; F2; Gameplay/Release; canonical safety.

## Out of scope

No persistent Authoring Groups, Group/Ungroup, group names/membership
serialization, scene graph/parenting, folders/collections/prefabs,
selection-center/bounding-box/custom pivot, local-space group
transforms, Group Resize/Scale, Undo/Redo, alignment/distribution,
GUIDs, Level Format v2, CLI/Agent API/MCP/scripting, generic
command/event/transaction framework, new gameplay/object types, or M81.

## Documentation and STOP

Canonical active document: `docs/milestones/MILESTONE_80.md`. Preserve
M76-M79 as CLOSED and keep `docs/MILESTONES.md` compact.

Report files changed; discovered Rotate architecture/categories;
eligibility; PRIMARY pivot; world-axis math; positional orbit;
orientation updates; no-drift; M76 snap/Ctrl; object authority; atomic
refusal; workingCopy authority; M78/M79/M77 preservation; testable
boundary; tests/builds/diffs; out-of-scope confirmation.

After implementation status is **implemented, awaiting manual
acceptance**.

Then STOP. Do NOT commit, push, merge, start M81, or mark M80 CLOSED.
