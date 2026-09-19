# Milestone 82 --- Hierarchy & Group Workflow Polish

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/82-hierarchy-group-workflow-polish`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Polish the Development Editor Hierarchy and Authoring Group workflow
introduced in M81 so persistent groups are comfortable, predictable, and
efficient during real Level Design.

M82 is a UX/workflow milestone built on M78--M81. Preserve the M81
persistent data model and semantics unless a concrete correctness defect
is discovered.

## Product outcome

The Level Designer can: - clearly distinguish persistent groups from
ungrouped authored objects; - expand/collapse groups; - click a group to
select its complete composition; - click a member to select only that
authored object; - create a group from multi-selection; - rename and
ungroup naturally from the Hierarchy; - use existing Duplicate/Delete
Selected on complete groups; - retain clear PRIMARY/secondary feedback
and stable selection after lifecycle operations.

The Hierarchy should become a practical authoring surface, not merely a
diagnostic list.

## Preserve M81 semantics

M81 is CLOSED and authoritative. Preserve: - `AuthoringGroup` authored
metadata; - typed `{kind,index}` membership; - `members[0]` as
preferred/deterministic PRIMARY; - one group per object; - flat groups
only; - no nested/overlapping membership; - deterministic delete/index
reconciliation; - auto-dissolve below 2 members; - Save/reload
persistence; - workingCopy authority; - complete-group duplication; -
partial-group duplicate refusal; - existing Group Selected / Rename /
Ungroup semantics.

## Hierarchy organization

Inspect the current M81 Hierarchy before changing it. Implement the
smallest coherent polish.

Clearly distinguish persistent groups from ungrouped authored objects.
Grouped members should not also appear as confusing duplicate top-level
editable entries.

Conceptually:

``` text
Hierarchy
▾ Groups
  ▾ Door_Puzzle
      Door 0
      Pressure Plate 0
      Pressure Plate 1
▾ Objects
    Ground 0
    Spawn
    Static Prop 0
    Goal 0
```

Exact labels must follow repository conventions. This is not a scene
graph.

## Expand / collapse

Groups support normal tree expand/collapse: - expand reveals members; -
collapse hides only member rows; - expansion is UI state, not Level
authored data; - expand/collapse never marks Dirty; - no Save is
required; - selection remains valid when collapsed; - no runtime effect.

Persist expansion only if an existing layout convention makes it
natural; otherwise transient state is acceptable. Do not add a generic
hierarchy-state persistence framework.

## Selection semantics

Click group row: - reconstruct M81/M78 selection; - `members[0]` becomes
PRIMARY; - remaining members become `additionalSelections`; - group row
gets clear selected feedback; - no separate transform authority.

Click member row: - select only that authored object; - do not select
the whole group; - do not alter membership; - do not mark Dirty.

Clicking the group again restores the complete group selection.

Preserve M78 Ctrl-toggle semantics for authored member rows. For group
rows choose the narrowest predictable behavior after inspecting current
code; prefer whole-group selection rather than adding multi-group
selection semantics.

## Synchronization

Hierarchy, viewport picking, Inspector, and groups must remain
synchronized: - viewport-select grouped member -\> corresponding member
is identifiable/highlighted; - member row -\> normal individual viewport
selection; - group row -\> PRIMARY + secondaries; -
duplicate/delete/ungroup -\> no stale rows/selections; -
Apply/Reload/Open/Switch -\> no stale group/member state; -
auto-dissolve -\> safe selection reconciliation.

Do not mutate membership to simplify UI state.

## Rename workflow

Keep M81 name validation but make Rename discoverable/natural from
Hierarchy using the narrowest existing editor pattern: context action,
explicit action, or safe inline rename if an existing pattern supports
it.

Valid rename marks Dirty and preserves selection. Invalid/duplicate
rename changes nothing. Member objects remain untouched.

## Contextual group actions

Make these discoverable near the Hierarchy: - Group Selected for a valid
ungrouped multi-selection; - Rename selected group; - Ungroup selected
group.

Existing Duplicate Selected / Delete Selected remain authoritative. Do
not create divergent implementations or a generic context-menu
framework.

## Workflow feedback

After Group Selected: - new group appears immediately; - complete group
remains selected; - membership is immediately understandable; - no
member looks lost/duplicated.

After Ungroup: - group row disappears; - former members become normal
ungrouped objects; - members remain selected where practical; - PRIMARY
deterministic; - no stale UI state.

After complete-group Duplicate Selected: - copied group appears
immediately; - copied composition is selected per M81/M79; - original
remains intact.

After complete-group Delete Selected: - group and members disappear; -
unrelated content remains; - selection reconciles safely.

After individual member delete: - membership/index display updates; -
group survives with 2+ members; - group disappears when M81
auto-dissolves below 2.

## Ordering

Use deterministic display ordering. Prefer authored group order unless
an existing editor convention is stronger. Members follow persistent M81
order; `members[0]` remains preferred PRIMARY.

No drag/drop reorder and no authored membership reorder for
presentation.

## Inspector and input safety

Preserve PRIMARY-only Inspector authority. Do not add generic
multi-object editing.

Group metadata controls may stay in the existing narrow location or move
closer to Hierarchy if cleaner.

Respect ImGui input capture and existing shortcut conventions while
renaming. Text entry must not accidentally trigger destructive
editor/gameplay shortcuts.

## UI state vs authored state

Hierarchy expansion and similar presentation preferences are editor
UI/layout state.

Group name and membership are authored Level state.

Never mix these authorities.

## Testable boundary

UI code should orchestrate existing M78--M81 helpers rather than
reimplementing group selection, membership, rename validation,
lifecycle, or reconciliation.

Add only narrow testable helpers where necessary. No generic UI command
system.

## Regression coverage

Cover at least: 1. group row reconstructs complete M81 selection; 2.
member row selects only that object; 3. member selection does not alter
membership/Dirty; 4. reselecting group restores full selection; 5.
grouped object is clearly identifiable; 6. no confusing duplicate
top-level presentation; 7. deterministic group/member ordering and
PRIMARY; 8. Group Selected immediately appears correctly; 9. Rename
preserves selection; invalid/duplicate rename is atomic; 10. Ungroup
preserves objects/selection; 11. complete-group duplicate presents
copied group correctly; 12. complete-group delete removes correct
presentation; 13. individual member delete updates membership; 14.
auto-dissolve removes stale group UI; 15. same-category remap still
represents the same logical object; 16.
Apply/Reload/Revert/Open/Switch/New reconcile selection; 17. F2
roundtrip has no stale group UI; 18. expand/collapse does not mark Dirty
or mutate LevelDefinition; 19. M81 persistence and partial-duplicate
refusal remain green; 20. M80 Rotate, M79 lifecycle/Item ID, M78
Translate, M77 Grid, M76 Snap remain green; 21. Gameplay/Release
unchanged; 22. canonical Levels unchanged.

## Manual acceptance

With disposable Development edits verify: - create ungrouped objects and
Group Selected; - new group is clear; - expand/collapse and confirm no
Dirty; - group click -\> full selection; - member click -\> individual
selection; - group click again -\> full selection; - viewport-select
grouped member and inspect Hierarchy; - Ctrl-select members from
Hierarchy; - rename; invalid/duplicate rename; - Group Translate; -
compatible Group Rotate and incompatible refusal; - Duplicate Selected
complete group; - copied group appears/selected; - Delete Selected
copied group; - delete one member; verify membership/auto-dissolve; -
Ungroup; members remain; - Save/reload; Apply/Dirty; F2 roundtrip; -
Snap/Grid/Item ID regression; - Gameplay/Release; - canonical Level
safety.

Automated green is not sufficient.

## Canonical Level safety

Do not intentionally modify: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

Use disposable data. Do not normalize EOLs. Final semantic diffs must be
empty.

## Validation

Inspect/run affected C++ suites, especially AuthoringGroupsTest,
LevelFileTest, EditorPicking, EditorWorkspace, EditorQuickToolbar,
EditorGizmo, AuthoredObjectLifecycle, AuthoredLifecycleIntegration,
EditorViewportGrid, StaticProp, ItemPickup, DynamicBox,
PressurePlate/Door, PhysicsRebuild, and M78--M81 coverage.

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

## Out of scope

Do not implement nested/overlapping groups, group parenting, scene
graph, folders/collections, drag/drop membership/reorder, new
multi-group semantics, lock/hide, group colors/tags, generic icon
system, generic multi-object Inspector, bulk property editing, Group
Resize/Scale, pivot modes, Prefabs, linked instances, Undo/Redo,
Alignment/Distribution, GUIDs/generalized IDs, Level Format v2,
CLI/Agent API/MCP/scripting, generic command/transaction/reference
graph, 3D Player Character, Materials/Textures, Lighting/Shadows,
Terrain, or M83 functionality.

## Documentation and STOP

Canonical active document: `docs/milestones/MILESTONE_82.md`.

Preserve M76--M81 as CLOSED. Keep `docs/MILESTONES.md` compact. Update
architecture/README/AGENTS only where implementation requires it.

After implementation M82 is **implemented, awaiting manual acceptance**,
not CLOSED.

Cursor report must include: 1. files changed; 2. pre-change M81
Hierarchy behavior; 3. final organization; 4. grouped-vs-ungrouped
presentation; 5. expand/collapse behavior; 6. group/member/Ctrl
selection behavior; 7. viewport/Hierarchy/Inspector synchronization; 8.
rename UX; 9. contextual actions; 10. Group/Ungroup/Duplicate/Delete
feedback; 11. ordering; 12. keyboard/focus safeguards; 13. UI-state vs
authored-state boundary; 14. non-ImGui semantic reuse; 15. confirmation
M81 data model preserved unless a documented correctness fix was
necessary; 16. C++/Python/build results; 17. `git diff --check`; 18.
canonical Level diff confirmation; 19. explicit out-of-scope
confirmation.

Then STOP.

Do NOT commit. Do NOT push. Do NOT merge. Do NOT start M83. Do NOT mark
M82 CLOSED.
