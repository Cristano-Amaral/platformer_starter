## Milestone 42 --- Object Palette & Placement Workflow

**Branch:** `milestone/42-object-palette-placement`\
**Status:** CLOSED. Merged to main. Milestone 43 is Editor Quick Toolbar.\
**Prerequisite:** Milestone 41 --- Authored Object Lifecycle --- CLOSED\
**Scope:** Development editor only

### Goal

Turn the M41 authored-object lifecycle into a faster level-design
workflow by adding a compact **Object Palette** and an explicit
**placement mode** for the four repeatable authored categories:

-   Platform
-   Checkpoint
-   Hazard
-   Collectible

M42 improves *how* designers create and place objects. It does not
change Level Format v1, runtime authority, gameplay rules, or the
lifecycle semantics established in M41.

### User-facing outcome

In Development/F2 editor mode, the designer can open an Object Palette,
choose an authored object type, and place new objects directly in the 3D
viewport.

Expected workflow:

1.  Open **Object Palette**.
2.  Choose Platform, Checkpoint, Hazard, or Collectible.
3.  Enter placement mode.
4.  Move the placement preview through the viewport.
5.  Click to place the object into `workingCopy`.
6.  The placed object becomes selected.
7.  Continue placing the same type or exit placement mode.
8.  Use the existing M41 Apply/Revert/Save workflow normally.

The existing **Edit \> Add** commands remain valid and are not removed.

### Non-goals

M42 must not introduce:

-   ECS or scene graph architecture.
-   GUIDs or persistent object IDs.
-   Level Format v2.
-   New authored object categories.
-   Arbitrary asset browser/content browser.
-   Prefab system.
-   Drag-and-drop asset importing.
-   Runtime spawning system.
-   Physics simulation for placement previews.
-   General undo/redo stack.
-   Multi-selection.
-   Copy/paste framework.
-   Object grouping/parenting.
-   Grid/surface snapping framework beyond the narrow placement rules
    defined here.
-   Terrain editing.
-   Streaming/LOD/partitioning.
-   Changes to M39/M40 Cook/Stage/Reload semantics.

### Existing M41 contracts that remain authoritative

M42 must preserve:

-   `active` = currently applied runtime world.
-   `workingCopy` = pending authored state.
-   Add/Duplicate/Delete mutate `workingCopy` only.
-   Apply remains transactional.
-   Revert restores `workingCopy = active`.
-   Save writes active state only.
-   `StructuralIndexMap` remains the transient active↔working identity
    mechanism.
-   Pending Add/Duplicate/Modify visuals remain cyan.
-   Selected pending visuals use stronger emphasis.
-   Unselected pending visuals remain visible with softer emphasis.
-   Pending Delete remains faded/desaturated with delete outline.
-   Pending workingCopy objects remain viewport-pickable.
-   Pending Delete remains non-pickable.
-   Platform support-index deletion/remap rules remain unchanged.
-   Capacity policy from M41 remains unchanged.
-   Debug has no authoring palette/placement workflow.
-   Release has no editor.

### Object Palette

Add a Development-only **Object Palette** editor window.

Minimum contents:

-   Platform
-   Checkpoint
-   Hazard
-   Collectible

Each entry should be compact and immediately actionable.

The palette is a creation tool, not a hierarchy replacement.

#### Workspace integration

Add **Object Palette** to the existing `View` menu and workspace state.

Preferred menu:

``` text
View
├ Metrics
├ Hierarchy
├ Inspector
├ Level Editor
└ Object Palette
```

Requirements:

-   Window visibility participates in the existing editor workspace
    authority.
-   Closing the palette window updates the corresponding View menu
    state.
-   View menu toggling updates the window.
-   F2 editor visibility behavior remains consistent with other editor
    windows.
-   Persistent layout uses the existing
    `%LOCALAPPDATA%\Platformer3D\editor_layout.ini`.
-   Do not create CWD `imgui.ini`.
-   Reset Editor Layout resets palette window placement/visibility
    according to the established workspace policy.

### Placement mode

Selecting a palette entry enters an editor-only placement mode for that
category. Clicking the **same** category again returns to `None` (tool
toggle). Clicking a different category switches mode. Esc still exits.
Placement mode is transient editor state and must not modify the level
until the user confirms placement.

Conceptually:

``` text
PlacementMode
├ None
├ Platform
├ Checkpoint
├ Hazard
└ Collectible
```

No generalized tool framework is required.

### Placement preview

While placement mode is active, draw a preview of the object at the
candidate position.

The preview:

-   Uses the selected category's default authored geometry.
-   Uses a distinct placement-preview treatment.
-   Must be visually distinguishable from already-created pending
    objects.
-   Does not exist in `workingCopy` yet.
-   Has no physics.
-   Has no trigger/gameplay behavior.
-   Is not included in Hierarchy.
-   Is not included in Save/Apply.
-   Is not viewport-pickable as an authored object.

Preferred visual language:

-   Placement candidate: brighter/cleaner cyan preview, optionally with
    a simple placement marker.
-   Existing pending object: M41 cyan selected/unselected styles.
-   Pending Delete: M41 faded/delete style.

Do not introduce shaders or a new rendering framework.

### Candidate position

Placement should be driven by the editor viewport ray.

Preferred narrow rule:

1.  Cast the editor mouse ray.
2.  If it hits an eligible active-world placement surface, place the
    candidate at the hit point with category-specific vertical offset as
    needed.
3.  If there is no eligible hit, use a fallback point in front of the
    editor camera.
4.  Preserve the current level traversal-lane convention where
    appropriate rather than creating a new Level Format field.

Keep this deterministic and testable.

#### Eligible surfaces

For M42, surface placement only needs to consider authored/static world
geometry already available to editor picking/geometry helpers.

Do not implement arbitrary mesh triangle placement or physics raycasts
if existing editor CPU geometry is sufficient.

#### Fallback

If no surface is hit, use the existing editor-camera working-region
concept rather than silently placing near Spawn.

The fallback must remain visible/reachable from the current editor
camera.

### Category defaults

Use the same canonical default values already defined by M41 lifecycle
helpers.

Do not create a second set of defaults in the palette.

#### Platform

Placement creates a Platform using the M41 default size.

The candidate position should represent the Platform center
consistently.

#### Checkpoint

Placement creates the entire Checkpoint assembly.

The initial:

-   trigger center
-   respawn position

must preserve the M41 default relation.

After creation, normal checkpoint assembly translation behavior remains
authoritative.

#### Hazard

Placement creates the existing M41 default Hazard.

#### Collectible

Placement creates the existing M41 default Collectible using its
authored collection bounds.

### Confirm placement

Primary viewport click confirms placement when:

-   placement mode is active,
-   mouse is over the viewport,
-   ImGui does not capture the pointer,
-   no gizmo interaction consumes the pointer,
-   capacity permits the object.

Confirmation must call the same M41 lifecycle Add authority rather than
duplicating lifecycle mutation logic.

The helper should accept the resolved placement position.

After successful placement:

-   append to `workingCopy`,
-   update structural mapping/session state as required by M41,
-   select the new working object,
-   show the normal selected pending visual,
-   expose it in Hierarchy and Inspector.

Do not Apply automatically.

Do not Save automatically.

Do not create physics automatically.

### Repeated placement

After a successful placement, remain in the same placement mode by
default so multiple objects of one type can be authored efficiently.

Example:

``` text
Choose Collectible
click
click
click
Esc
```

Each click creates one Collectible in `workingCopy`.

Each newly created object becomes the current selection.

Previously created pending objects remain visible with M41 unselected
emphasis.

### Exit placement mode

Support at minimum:

-   `Esc` exits placement mode.
-   Selecting another palette category switches placement type.
-   Closing the Object Palette does **not** have to cancel placement
    mode unless the implementation makes that behavior clearer and is
    documented consistently.
-   F2 closing the editor must cancel placement mode.
-   Apply/Revert/Reload must cancel placement mode.
-   Entering a conflicting gizmo interaction/tool must not accidentally
    place an object.

No right-click context-menu system is required.

### Cursor/input safety

Placement must honor current-frame ImGui capture.

Do not place objects when clicking:

-   menu bar,
-   Object Palette,
-   Hierarchy,
-   Inspector,
-   Tool Output,
-   other ImGui windows.

Existing editor camera controls must remain usable.

RMB camera navigation must not place objects.

Gizmo interaction retains priority over placement.

### Capacity behavior

Reuse the M41 capacity policy.

-   Platform placement stops at actual physics capacity.
-   Checkpoint/Hazard/Collectible use Level Format v1 defensive
    record/file capacity rather than old arbitrary 8/16 limits.
-   Disabled/unavailable placement should communicate the existing
    capacity reason.
-   Do not add new small category caps.

### Selection and picking

After placement, the object is a normal M41 pending workingCopy object.

Therefore it must:

-   be selectable from Hierarchy,
-   be selectable from viewport using M41 pending picking,
-   receive strong pending emphasis when selected,
-   receive soft pending emphasis when deselected.

Do not create a separate selection model for palette-created objects.

### Apply / Revert / Reload

#### Apply success

-   Commit `workingCopy` to active using existing transactional rebuild.
-   Clear pending authoring state as M41 already defines.
-   Cancel placement mode.
-   New objects become normal active objects.

#### Apply failure

-   Preserve active world.
-   Preserve workingCopy.
-   Preserve pending objects.
-   Placement mode may be cancelled for safety, but behavior must be
    deterministic and documented.

#### Revert

-   Restore `workingCopy = active`.
-   Remove pending palette-created objects.
-   Clear transient placement preview.
-   Cancel placement mode.

#### Runtime Reload

-   Preserve M39 authority.
-   Successful Reload clears pending editor state and cancels placement
    mode.
-   Do not alter M40 Cook, Stage & Reload orchestration.

### Edit \> Add compatibility

Keep existing M41:

``` text
Edit
└ Add
   ├ Platform
   ├ Checkpoint
   ├ Hazard
   └ Collectible
```

These commands remain quick-add commands using their existing
placement-anchor semantics.

Do not silently change Edit \> Add into placement mode.

Object Palette placement is an additional workflow.

This keeps M41 behavior stable and gives the designer both:

-   quick Add,
-   deliberate viewport placement.

### Testability

Keep placement math and decisions outside direct ImGui/raylib UI code
where practical.

Preferred small pure/testable helpers include concepts such as:

-   placement mode state,
-   candidate position resolution,
-   placement surface hit selection,
-   category default placement transform,
-   placement confirmation eligibility.

Do not build a generalized editor command framework.

### Automated acceptance

Add focused tests for:

1.  Palette/workspace visibility state.
2.  Placement mode enter/switch/exit.
3.  Esc cancellation.
4.  F2/editor-close cancellation.
5.  Candidate surface-hit placement.
6.  Candidate fallback placement.
7.  Platform placement default.
8.  Checkpoint assembly placement default.
9.  Hazard placement default.
10. Collectible placement default.
11. Confirm adds exactly one object.
12. Repeated placement adds multiple objects.
13. New object becomes selected.
14. Previous pending objects remain visible.
15. M41 pending viewport picking works on palette-created objects.
16. ImGui capture prevents placement.
17. RMB camera interaction prevents placement.
18. Gizmo-consumed pointer prevents placement.
19. Capacity rejection.
20. Apply clears placement state.
21. Revert clears placement state.
22. Reload clears placement state.
23. Existing Edit \> Add behavior unchanged.
24. Pending Delete behavior unchanged.
25. StructuralIndexMap behavior unchanged.
26. Level Format v1 round-trip unchanged.

Run the established M41/editor/runtime regression suites in all
applicable configurations.

### Manual acceptance

In Development build:

#### Platform

-   Open F2.
-   Open Object Palette.
-   Choose Platform.
-   Move cursor over an eligible surface.
-   Verify placement preview.
-   Click.
-   Verify Platform appears in Hierarchy and Inspector.
-   Verify it is selected.
-   Click elsewhere.
-   Verify soft pending visual remains.
-   Click Platform again in viewport.
-   Verify selection returns.
-   Test Translate and Resize.
-   Revert.

#### Checkpoint

-   Choose Checkpoint.
-   Place it.
-   Verify trigger/beacon/respawn representation.
-   Verify checkpoint assembly semantics.
-   Deselect/reselect from viewport.
-   Revert.

#### Hazard

-   Choose Hazard.
-   Place it.
-   Deselect/reselect.
-   Move it.
-   Revert.

#### Collectible

-   Choose Collectible.
-   Place at least three with repeated clicks.
-   Verify each new object becomes selected.
-   Verify older pending Collectibles remain visible.
-   Click each from viewport.
-   Revert.

#### Input safety

-   While placement mode is active, interact with Object Palette,
    Inspector and Hierarchy.
-   Verify no accidental object creation.
-   RMB navigate camera.
-   Verify no accidental placement.
-   Press Esc.
-   Verify preview disappears and no object is created.

### Mixed lifecycle

Before Apply:

-   Place one new object.
-   Modify one existing object.
-   Delete another existing object.

Verify simultaneously:

-   placement-created/pending = cyan family,
-   modified pending = cyan family,
-   pending Delete = faded/delete style.

Apply and verify all three changes become the active world correctly.

Then restore the canonical Level 01 before closure.

### Regression suites

Run:

``` text
AuthoredObjectLifecycleTest
AuthoredLifecycleIntegrationTest
EditorWorkspaceTest
EditorGizmoTest
EditorOrientationTest
EditorPickingTest
LevelFileTest
PhysicsRebuildTest
CookStageReloadWorkflowTest
EditorToolRunnerTest
```

plus new M42 Object Palette / placement tests.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Then:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

### Canonical safety

Before M42 closure, restore/confirm canonical Level 01:

-   Platforms = 6
-   Checkpoints = 2
-   Hazards = 2
-   Collectibles = 3
-   Camera FOV = 40

Run:

``` text
git diff -- game/assets/source/levels/level_01.level
git diff --check
```

Expected:

-   canonical level has no unintended semantic diff,
-   `git diff --check` is clean.

Do not normalize line endings merely to remove the known EOL warning. If
necessary, restore the worktree copy from HEAD as established in
previous milestones.

### Documentation

Update as appropriate:

-   `README.md`
-   `AGENTS.md`
-   `docs/ARCHITECTURE.md`
-   `docs/MILESTONES.md`

Document:

-   Object Palette scope.
-   Placement mode state.
-   Surface-hit/fallback placement rule.
-   Repeated placement.
-   Input/capture rules.
-   Relationship to M41 lifecycle.
-   Edit \> Add remains available and unchanged.
-   No runtime/physics entity until Apply.
-   No Level Format change.

### Completion criteria

M42 is complete only when:

-   Object Palette is integrated into Development workspace.
-   All four M41 repeatable categories can enter placement mode.
-   Placement preview is clear and non-authoritative.
-   Viewport click creates the object at the resolved candidate
    position.
-   Repeated placement works.
-   Pending objects use M41 lifecycle/visual/picking semantics.
-   Input capture prevents accidental placement.
-   Apply/Revert/Reload cleanup is correct.
-   Existing Edit \> Add remains unchanged.
-   No Level Format v1 change.
-   Automated tests pass.
-   Manual acceptance passes.
-   Debug/Development/Release builds pass.
-   Canonical Level 01 is restored.
-   Git diff checks are clean.
-   No unrelated scope was introduced.

### Git closure

Do not commit/push/merge until manual acceptance is approved.

After approval:

``` text
test
→ approve
→ inspect diff/status
→ commit once
→ push milestone branch
→ merge normally into main
→ push main
→ confirm clean/synced main
```

Do not use `--no-ff` automatically.
