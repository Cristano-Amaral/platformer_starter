# Milestone 79 --- Multi-Selection Lifecycle Operations

**Status:** CLOSED\
**Branch:** `milestone/79-multi-selection-lifecycle-operations`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Build directly on M78 multi-selection with two selection-aware authored
lifecycle operations: **Duplicate Selected** and **Delete Selected**.

Primary workflow:
`select composition -> duplicate selected -> copies remain selected -> group translate`.

M79 extends existing lifecycle behavior only. No generalized command
system, prefab/collection system, or Undo/Redo.

## Duplicate Selected

With 2+ selected authored objects, Duplicate duplicates every selected
supported object exactly once using existing object-specific duplication
authority. Mixed supported categories must work. Unsupported members
refuse the whole operation; no partial duplication.

On success originals become unselected, all copies become selected, and
the copy corresponding to the original PRIMARY becomes the new PRIMARY.
This must allow immediate M78 Group Translate.

Preserve the existing single-object duplicate positional offset
convention where applicable, using composition-safe semantics so
relative spacing is not distorted. Do not Apply or Save automatically.

Prefer atomic planning/validation: failure leaves workingCopy, Dirty,
and selection unchanged.

## Delete Selected

With 2+ selected authored objects, Delete removes every selected object
exactly once and no unselected object.

M78 identity remains `{kind,index}`. Handle index shifting
deterministically, typically descending indices within each category or
an equivalent repository-safe plan. Do not introduce GUIDs.

Preserve current authored referential-integrity semantics. Inspect
especially Pressure Plate -\> Door index links. Multi-delete must safely
remap/clear links as existing single-delete authority requires,
including when referrer and referenced objects are both selected.

Prefer atomic planning/validation. Failure leaves workingCopy, Dirty,
and selection unchanged. Successful deletion safely clears/reconciles
selection. Do not Apply or Save automatically.

## Single-selection compatibility

Exactly one selected object keeps established Duplicate/Delete behavior
unless routed through an equivalent unified implementation. Preserve
keyboard/menu/Hierarchy/Inspector behavior.

## UI

With multi-selection, communicate Duplicate Selected / Delete Selected
(or concise equivalent). Enable only when the complete selection
supports the operation. Existing shortcuts invoke selection-aware
behavior. Compact refusal feedback; no new confirmation modal unless
existing behavior already has one.

## Preserve M78

Keep Ctrl viewport/Hierarchy add/remove, ordinary replace, empty clear,
PRIMARY/secondary distinction, deterministic promotion, synchronization,
transient selection, lifecycle reconciliation, Group Translate, shared
M76 snap delta, and unsupported Translate refusal.

## Preserve M76/M77

All snapping and editor-only Grid behavior remain unchanged.
Gameplay/Release unchanged.

## Authority

Mutate `workingCopy` only. Apply validates/promotes to active; Save
writes authored state. No automatic Apply/Save and no runtime/Jolt
capture.

## Future automation readiness

Where practical keep multi-object lifecycle planning testable outside
ImGui:
`selection + workingCopy -> validated duplication/deletion plan -> new workingCopy + resulting selection`.

No CLI, Agent API, MCP, scripting, generic AuthoringCommand, command
stack, transaction framework, GUIDs, or generic reference graph.

## Canonical safety

Do not intentionally modify `game/assets/source/levels/level_01.level`
or `level_02.level`. Use disposable edits. Do not normalize EOLs. Final
semantic diffs must be empty.

## Regression coverage

Cover single Duplicate/Delete compatibility; all-selected exactly-once
duplication/deletion; mixed categories; composition spacing; copies
selected with copied PRIMARY; unsupported/failed operations atomic;
same-category deletion index shifts; mixed-category deterministic
deletion; Pressure Plate/Door and discovered reference remapping;
referrer+referenced simultaneous deletion; no stale selection;
Dirty/Apply/Save; M78 Group Translate; M76 Snap; M77 Grid; unchanged
Gameplay/Release; canonical safety.

Include explicit regression tests for same-category index shifting and
authored reference remapping.

## Validation

Run affected lifecycle, selection, gizmo, Hierarchy, Dynamic Box, Item
Pickup, Static Prop, Pressure Plate/Door, LevelFile, PhysicsRebuild,
Workspace and EditorViewportGrid tests plus relevant discovered tests.

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

Then run `git diff --check` and verify both canonical Level diffs are
semantically empty.

## Manual acceptance

Verify single Duplicate/Delete; same-category and mixed-category
Duplicate Selected; copies remain selected with copied PRIMARY;
immediate Group Translate; preserved spacing; same-category and
mixed-category Delete Selected; Door/Pressure Plate reference behavior;
no unrelated deletion; atomic unsupported refusal; Dirty/Apply/Save;
selection synchronization; M78 Group Translate; M76 Snap; M77 Grid; F2;
Gameplay/Release; canonical safety.

## Out of scope

No Group Resize/Scale/Rotate, arbitrary pivots, parenting, folders,
collections, prefabs, Undo/Redo, clipboard/cut/paste,
alignment/distribution, advanced snapping, GUIDs, generic reference
graph/transaction/command framework, CLI, Agent API, MCP, scripting, new
gameplay, new object types, Level Format v2, or M80.

## Documentation

Canonical active document: `docs/milestones/MILESTONE_79.md`. Preserve
M76/M77/M78 as CLOSED. Keep milestone index compact. After
implementation M79 is **implemented, awaiting correction manual acceptance**, not
CLOSED.

## Item ID Inspector correction

Development Inspector Item ID for an Item Pickup now uses a session-local
edit buffer bound to the PRIMARY selection. Valid M54 `itemId` values
commit to `workingCopy` on InputText change and on focus loss. Invalid
text is rejected and the buffer is restored from the last committed
value. Duplicate `itemId` values remain legal. Secondary multi-selection
members are not bulk-edited.

## Cursor report and STOP

Report files changed; discovered lifecycle architecture; selection-aware
routing; Duplicate Selected algorithm/offset/post-selection/atomicity;
Delete Selected plan/index safety/reference remapping/atomicity;
single-selection compatibility; UI/shortcuts; workingCopy authority;
M78/M76/M77 preservation; automation-readiness boundary;
tests/builds/diff checks/canonical safety; and out-of-scope
confirmation.

Then STOP. Do NOT commit, push, merge, start M80, or mark M79 CLOSED.
