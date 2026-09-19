# Milestone 81 --- Authoring Groups Foundation

**Status:** CLOSED\
**Branch:** `milestone/81-authoring-groups-foundation`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## 1. Goal

Introduce persistent **Authoring Groups** so a Level Designer can turn
an intentional multi-selection composition into a named authored group,
later select that group again, manipulate its complete membership using
the already established multi-selection transform/lifecycle operations,
and ungroup it without changing the member objects.

M81 is the persistence and semantic foundation for groups. M82 may
polish the Hierarchy/navigation workflow after real use.

An Authoring Group is **not** a gameplay object and **not** a Prefab.

## 2. Required user workflow

The minimum workflow is:

`multi-select objects -> Group Selected -> persistent group -> select group -> existing group-capable operations -> Ungroup`

Required group-capable operations reuse existing semantics: - Group
Translate from M78; - Duplicate Selected / Delete Selected from M79; -
Group Rotate from M80 when every member is Rotate-compatible.

Selecting a group must reconstruct a normal editor multi-selection: -
one deterministic PRIMARY; - remaining members as secondaries; - no
special parallel transform path.

This reuse is a core architectural requirement.

## 3. Persistent membership

Group membership must survive: - Save; - reload; - editor
restart/reopen; - Apply/rebuild; - unrelated authored object
additions/deletions where surviving members remain valid.

Before implementation, inspect the repository's current Level Format v1,
serialization, editor-only persistence, object lifecycle and
index-reference remapping.

Use the **smallest robust repository-consistent representation** for
persistent group membership.

M81 must not introduce GUIDs, a generic entity-ID system, scene graph,
parent transforms, or Level Format v2.

If typed `{kind,index}` membership can be persisted and safely remapped
using narrow group-specific lifecycle rules, prefer that approach.

If repository inspection proves that this cannot satisfy the required
persistence safely, STOP and report the concrete blocker before
introducing a broader identity system.

Do not silently add GUIDs.

## 4. Group identity and naming

Groups need deterministic authored identity sufficient for persistence
and editing, but M81 does not need a generalized object identity system.

Provide a simple user-visible group name/identifier using
repository-safe text rules. New groups receive a deterministic default
such as `Group 01`, `Group 02`, or the closest repository-consistent
equivalent.

The user must be able to rename a group in M81 through a narrow group
property/editor control.

Requirements: - non-empty valid name; - deterministic duplicate-name
handling according to the implemented rules; - rename affects group
metadata only; - renaming never mutates member objects.

Do not build folders, tags, collections, nested names, localization, or
asset identity.

## 5. Group Selected

`Group Selected` is enabled only when: - 2+ valid authored objects are
selected; - every selected member can be represented safely as authored
group membership; - the selection does not create an invalid group
state.

On success: - create exactly one persistent group containing each
selected object exactly once; - preserve deterministic member order; -
preserve current PRIMARY as the group's preferred/deterministic PRIMARY
where practical; - keep the composition selected; - mark authored state
Dirty; - do not Apply or Save automatically.

Do not clone, move, rotate, resize, delete, or otherwise mutate member
objects when grouping.

## 6. Group membership rules

M81 groups are flat.

An authored object may belong to **at most one Authoring Group** in M81.

This deliberately avoids overlapping membership ambiguity.

If the selection contains an object already belonging to another group,
refuse the complete Group Selected operation with compact feedback.

Do not: - create nested groups; - allow group-inside-group; - allow
overlapping group membership; - auto-remove objects from another
group; - partially group only eligible members.

## 7. Selecting a group

The editor must expose a clear way to select a group from the Hierarchy
or the narrowest existing equivalent UI.

Selecting a group reconstructs the existing M78 multi-selection model
rather than creating a second transform model.

The selected group should yield: - deterministic PRIMARY; -
deterministic secondary order; - existing primary/secondary viewport
feedback; - existing Inspector remains PRIMARY-only.

The group itself does not have a runtime transform component.

## 8. Group transforms

Do not implement new transform math.

A selected group reuses the existing selection operations.

### Translate

If all members satisfy current M78 Group Translate eligibility,
Translate behaves exactly as M78.

### Rotate

If all members satisfy current M80 Group Rotate eligibility, Rotate
behaves exactly as M80 using the reconstructed PRIMARY pivot.

If one member is not Rotate-compatible, existing atomic Group Rotate
refusal applies.

### Resize / Scale

Remain non-group operations according to current M78/M80 behavior.

Do not add Group Resize or Group Scale.

## 9. Duplicate semantics

M81 does **not** create Prefab-style group instances.

When a complete selected group is duplicated through M79 Duplicate
Selected: - duplicate the member objects using existing M79 semantics; -
preserve current object-reference behavior from M79; - create one new
Authoring Group for the copied composition; - originals remain in the
original group; - copied members belong to the new group; - copied
composition remains selected according to M79 semantics; - the copied
PRIMARY remains PRIMARY; - the new group gets a deterministic
non-conflicting name/identifier.

This is a one-time composition copy, not linked instances.

If duplication cannot create the complete copied group atomically,
refuse the complete operation.

Do not introduce Prefab propagation or relinking beyond current M79
authored-reference semantics.

## 10. Delete semantics

Deleting a complete selected group through `Delete Selected` deletes its
selected member objects using existing M79 atomic semantics and removes
the now-empty group metadata.

Deleting only an individual member object through a valid existing
single-object workflow must remove/remap that membership safely without
corrupting the group.

After member deletion: - no stale group membership; - no invalid
`{kind,index}` references; - remaining group membership stays
deterministic; - a group with fewer than 2 surviving members must be
dissolved automatically, because M81 groups represent compositions
rather than aliases for a single object.

Do not delete unrelated group members merely because one member is
individually deleted.

## 11. Ungroup

`Ungroup` removes group metadata only.

It must: - preserve every member object exactly; - preserve authored
transforms/properties; - keep the former members selected as the normal
M78 multi-selection where practical; - preserve PRIMARY
deterministically; - mark authored state Dirty; - not Apply/Save
automatically.

Ungroup is not Delete.

## 12. Lifecycle/index remapping

This is the critical correctness area.

If membership uses typed `{kind,index}` references, every authored
lifecycle operation that changes indices must reconcile group
membership.

At minimum inspect: - Add; - Duplicate; - Delete; - Duplicate
Selected; - Delete Selected; - New/Open/Switch; - Apply/Reload/Revert; -
Save/reload; - F2/Play roundtrip; - category-specific index remaps
already used for platform support references and Pressure Plate -\> Door
references.

Use narrow Authoring Group remap logic.

Do not create a generic reference graph.

Same-category deletion must not cause group membership to silently
retarget to the next object.

## 13. Persistence / Level Format

Groups must belong to authored Level state, not merely editor
layout/session state.

Use repository conventions to persist them in the authored Level
representation while keeping them gameplay-inert.

Requirements: - deterministic serialization; - deterministic
parse/roundtrip; - old Levels without groups continue to load; -
runtime/gameplay behavior is unchanged; - cooker/staging remain valid; -
limits remain safe under existing Level Format v1 constraints; -
malformed group data fails/diagnoses according to existing Level parsing
conventions.

Do not introduce Level Format v2 solely for groups.

Do not store group membership only in ImGui layout settings.

## 14. workingCopy authority

Preserve:

`authoring operation -> workingCopy -> Apply validates/promotes -> active`

Save writes authored state.

Group creation, rename, duplication metadata, membership reconciliation
and Ungroup operate on authored workingCopy state.

Do not directly mutate runtime gameplay state.

Dirty must reflect group metadata changes.

## 15. Hierarchy scope

M81 needs only enough Hierarchy/UI to make groups usable and testable.

Required: - groups are visible; - group can be selected; - membership
can be inspected/expanded in a straightforward way; - Group Selected; -
Rename; - Ungroup.

Do not turn M81 into the full Hierarchy UX polish milestone.

Defer advanced behavior such as: - drag/drop membership; - reorder; -
elaborate context menus; - search/filter specific to groups; -
lock/hide; - color tags; - nested tree authoring; - folders.

## 16. M76--M80 preservation

Preserve all closed behavior: - M76 Transform Snapping; - M77 Viewport
Grid; - M78 multi-selection + Group Translate; - M79 Duplicate/Delete
Selected and Item ID correction; - M80 Group Rotate.

A group must reuse these operations rather than fork them.

## 17. Testable semantic boundary

Keep group semantics testable outside ImGui where practical.

Preferred conceptual operations:

-   `CreateGroup(workingCopy, selection)`
-   `SelectGroup(group) -> EditorSelection + additionalSelections`
-   `RenameGroup(...)`
-   `DuplicateGroupedSelection(...)`
-   `ReconcileGroupsAfterLifecycle(...)`
-   `Ungroup(...)`

Names are illustrative; follow repository conventions.

Do not add a generic command framework, transaction system, scene graph,
ECS, GUID system, Prefab system, or Agent API.

## 18. Regression coverage

Add focused automated coverage for at least:

1.  create group from 2+ selected authored objects;
2.  membership contains every selected object exactly once;
3.  PRIMARY/member order deterministic;
4.  grouping does not mutate member authored properties/transforms;
5.  group metadata makes workingCopy Dirty;
6.  Save/parse roundtrip preserves group;
7.  old Level without groups remains valid;
8.  selecting persisted group reconstructs correct M78 multi-selection;
9.  group Translate reuses M78;
10. compatible group Rotate reuses M80;
11. mixed unsupported Rotate member refuses atomically;
12. object cannot belong to two groups;
13. overlapping/nested grouping is refused;
14. rename persists;
15. Ungroup preserves all member objects and removes only metadata;
16. Ungroup keeps a valid normal multi-selection;
17. Duplicate Selected on complete group duplicates members exactly
    once;
18. duplicated composition receives a distinct new group;
19. copied group selection/PRIMARY is correct;
20. Delete Selected on complete group removes members and group
    metadata;
21. deleting one member removes/remaps only that membership;
22. same-category index shifts do not retarget membership;
23. mixed-category delete reconciliation;
24. group auto-dissolves below 2 surviving members;
25. unrelated groups survive lifecycle changes;
26. Apply/Reload/Revert behavior is correct;
27. F2/Play roundtrip has no stale membership;
28. M79 Plate/Door and platform reference remaps remain correct;
29. Item ID editing regression remains green;
30. M76 snapping remains green;
31. M77 Grid remains green;
32. M78 Group Translate remains green;
33. M79 Duplicate/Delete remains green;
34. M80 Group Rotate remains green;
35. Gameplay/Release behavior unchanged;
36. canonical Levels unchanged.

Include explicit escaped-bug-style tests for same-category
deletion/index shifting and duplicate-group membership remapping.

## 19. Canonical Level safety

Do not intentionally modify: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

Use disposable test/editor data.

Do not normalize EOLs.

Final semantic diffs must be empty.

## 20. Validation

Inspect/run directly affected current C++ suites, especially: -
LevelFile; - LevelEditor/editor state; - EditorPicking; - EditorGizmo; -
EditorWorkspace; - AuthoredObjectLifecycle; -
AuthoredLifecycleIntegration; - EditorViewportGrid; - StaticProp; -
ItemPickup; - DynamicBox; - PressurePlate/Door; - PhysicsRebuild; -
newer M78--M80 tests.

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

## 21. Manual acceptance

Using disposable Development edits verify: - multi-select 2+ objects; -
Group Selected; - visible persistent group; - rename; - select another
object then reselect group; - full membership restored as
multi-selection; - PRIMARY deterministic; - Group Translate; -
compatible Group Rotate; - Rotate refusal with unsupported member; -
Duplicate Selected on a group creates copied composition + copied
group; - copied group remains independently editable; - Delete Selected
on copied group; - individual member deletion from a group; - group
dissolves below 2 members; - Ungroup preserves objects; - Save/reload
preserves group; - Apply/Dirty; - F2 roundtrip; - Snap/Grid; - Item ID
edit regression; - Gameplay/Release; - canonical Level safety.

Automated green is not sufficient.

## 22. Out of scope

Do not implement: - nested groups; - overlapping membership; - group
parenting; - scene graph; - folders/collections; - Prefabs or reusable
definitions/instances; - linked group instances; - group-level runtime
entity; - group-level gameplay behavior; - group pivot modes; - Group
Resize/Scale; - drag/drop group membership; - advanced Hierarchy
polish; - Undo/Redo; - alignment/distribution; - GUIDs/persistent
object-ID framework; - Level Format v2; - CLI/Agent API/MCP/scripting; -
generic command/transaction/reference graph; - 3D Player Character; -
Materials/Textures; - Lighting/Shadows; - Terrain; - M82 functionality.

## 23. Documentation

Canonical active document:

`docs/milestones/MILESTONE_81.md`

Preserve M76--M80 as CLOSED.

Keep `docs/MILESTONES.md` compact.

Update architecture/README/AGENTS only where actual implementation
requires it.

After implementation M81 is **implemented, awaiting manual acceptance**,
not CLOSED.

## 24. Cursor report and STOP

Report: 1. files changed; 2. repository identity/index architecture
discovered; 3. exact persistent group representation chosen and why; 4.
confirmation that no GUID system was introduced; 5. group naming rules;
6. Group Selected semantics; 7. one-group-per-object enforcement; 8.
group selection -\> M78 selection reconstruction; 9. Translate reuse;
10. Rotate reuse; 11. Duplicate Selected group-copy behavior; 12.
Delete/member deletion behavior; 13. index-remapping/reconciliation
algorithm; 14. Ungroup behavior; 15. persistence/Level Format v1
behavior; 16. workingCopy/Dirty/Apply/Save; 17. Hierarchy/UI scope; 18.
M76--M80 preservation; 19. testable non-ImGui boundary; 20. C++ tests;
21. Python tests; 22. Debug/Development/Release builds; 23.
`git diff --check`; 24. canonical Level diff confirmation; 25. explicit
out-of-scope confirmation.

If robust persistence cannot be achieved without introducing a broader
persistent identity/GUID architecture, STOP before implementing that
architecture and report the blocker for review.

Otherwise, after implementation STOP.

Do NOT commit. Do NOT push. Do NOT merge. Do NOT start M82. Do NOT mark
M81 CLOSED.
