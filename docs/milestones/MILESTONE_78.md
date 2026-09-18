# Milestone 78 --- Multi-Selection & Group Translate

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/78-multi-selection-group-translate`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Increase Level-authoring throughput by allowing the Development editor
to select multiple existing authored objects and move them together with
the existing world-axis Translate gizmo. M76 snapping and M77 grid
remain authoritative foundations.

M78 is deliberately not a generalized group-transform, hierarchy,
prefab, collection, or Undo/Redo milestone.

## Selection model

-   Ordinary click replaces selection with one authored object.
-   Ctrl+click unselected authored object adds it.
-   Ctrl+click selected authored object removes it.
-   Empty click without modifier clears selection according to current
    conventions.
-   Keep a clear primary selection; most recently directly
    selected/added object becomes primary.
-   Removing primary chooses a deterministic remaining primary.
-   Single-selection behavior remains unchanged.
-   Preserve M76 Ctrl inversion during gizmo manipulation; inspect input
    conflicts.

## Primary selection

Primary remains authority for Inspector, transform-mode availability,
gizmo anchor/pivot, and existing single-object operations not explicitly
extended by M78. No implicit bulk Inspector editing and no mixed-value
Inspector.

## Visual feedback

All selected authored objects must be identifiable; primary must be
distinguishable from secondary selections. Preserve gizmo/grid
readability. No authored data.

## Group Translate

With 2+ compatible selected objects and Translate active: - gizmo anchor
is the primary object's existing Translate anchor; - every member
receives the exact same world-space translation delta; - relative
offsets remain unchanged; - only fields already edited by each object's
current Translate behavior may change; - M76 snapping quantizes the
primary result, then one shared snapped delta is applied to the whole
group; - do not independently snap each member; - Ctrl snap inversion
and drag-start authority remain correct.

## Unsupported members

Do not expand Translate support. If any selected member cannot
Translate, do not partially move a subset. Refuse/disable group
Translate safely, preserve selection, and provide compact feedback.

## Object authority

Preserve repository semantics, including Item Pickup logical `position`,
Dynamic Box authored center without runtime Jolt capture, Checkpoint
assembly semantics, Static Prop position, etc. Size/scale/rotation
remain untouched.

## Hierarchy

Ctrl+click in Hierarchy should follow the same add/remove semantics
where practical; ordinary click replaces selection. Viewport and
Hierarchy remain synchronized. No parenting, folders, collections,
reordering, or drag/drop.

## Existing commands

Resize/Scale/Rotate, Inspector edits, Duplicate, Delete, Palette
placement, etc. do not gain implicit bulk semantics. Choose the safest
narrow behavior per existing command: primary/single-object only where
unambiguous, or disable while multi-selected if primary-only execution
would mislead. Document choices. Delete and Duplicate are NOT bulk
operations in M78.

## Lifecycle safety

Multi-selection is transient editor state. Clear/reconcile safely across
Open/Switch Level, Apply/Reload where indexing can change, deletion, New
Level, and existing reconstruction boundaries. Do not persist selection.
Do not introduce GUIDs/persistent IDs.

## Authority

Group Translate edits `workingCopy` only. Apply validates/promotes to
active; Save writes authored state. Dirty semantics reflect successful
authored edits. Runtime state must never become authored accidentally.

## M76 preservation

Preserve all snapping semantics, increments, half-away-from-zero, Ctrl
inversion, dragStart, persistence, accumulated Rotate. Group Translate
snaps only primary result and derives one shared delta.

## M77 preservation

Preserve editor-only grid, Translate-derived spacing, axes, density,
persistence, and no Grid in Gameplay/Release.

## Future automation readiness

Keep group-translation calculation/application narrow and testable
outside ImGui where practical. No CLI, Agent API, MCP, scripting,
AuthoringCommand framework, generalized Authoring Operations, or GUIDs.

## Canonical safety

Do not intentionally modify `level_01.level` or `level_02.level`. Use
disposable manual edits. Do not normalize EOLs. Canonical diffs must be
semantically empty.

## Regression coverage

Prove ordinary/Ctrl selection semantics, deterministic primary,
clearing, single-selection preservation, viewport/Hierarchy sync,
primary/secondary feedback, shared-delta group Translate,
relative-offset preservation, primary-only M76 snapping, Ctrl inversion,
no drift, Item Pickup and Dynamic Box authority, safe refusal with
unsupported member, no accidental group
Resize/Scale/Rotate/Delete/Duplicate, lifecycle safety,
workingCopy/Dirty/Apply/Save authority, M76/M77 regressions, unchanged
gameplay/Release, and unchanged canonical Levels.

## Validation

Run affected C++ tests for picking, gizmos/snap, workspace, quick
toolbar, Hierarchy/selection, authored lifecycle, Dynamic Box, Item
Pickup, Static Prop, EditorViewportGrid, Physics rebuild, Level file
behavior, plus relevant discovered tests.

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

Then run `git diff --check` and verify canonical Level diffs are
semantically empty.

## Manual acceptance

Verify viewport and Hierarchy multi-selection, primary distinction,
clear selection, group Translate with Snap on/off and Ctrl inversion,
preserved relative spacing, mixed translatable categories, safe refusal
with unsupported member, Item Pickup/Dynamic Box authority, unchanged
single-object transforms, non-bulk Duplicate/Delete, Apply/Save/Dirty,
M77 Grid, F2, Gameplay/Release, and canonical Level safety.

## Out of scope

No group Resize/Scale/Rotate/Delete/Duplicate, arbitrary pivots,
local-space group transforms, parenting, folders, collections, prefabs,
Undo/Redo, smart alignment/distribution, object/surface/vertex snapping,
hierarchy reordering, GUIDs, CLI, Agent API, MCP, scripting, generic
command/event framework, new gameplay, new object types, Level Format
v2, or M79.

## Documentation

Canonical: `docs/milestones/MILESTONE_78.md`. Preserve M76/M77 as
CLOSED. Keep milestone index compact. After implementation M78 is
**implemented, awaiting manual acceptance**, not CLOSED.

## Cursor report and STOP

Report files changed; discovered selection architecture; multi-selection
representation/primary rule; viewport and Hierarchy semantics; visual
feedback; group Translate/pivot/shared snapping; unsupported behavior;
object authority; behavior of other commands; lifecycle safety;
workingCopy/Dirty/Apply/Save; M76/M77 preservation; automation-readiness
boundary; tests/builds; diff checks; canonical safety; and explicit
out-of-scope confirmation.

Then STOP. Do NOT commit, push, merge, start M79, or mark M78 CLOSED.
