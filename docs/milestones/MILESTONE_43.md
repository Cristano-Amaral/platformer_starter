## Milestone 43 --- Editor Quick Toolbar

**Branch:** `milestone/43-editor-quick-toolbar`\
**Status:** CLOSED\
**Prerequisite:** Milestone 42 --- Object Palette & Placement Workflow
--- CLOSED\
**Scope:** Development editor UX only

### Goal

Add a compact, fixed **Quick Toolbar** directly below the existing menu
bar so the most frequently used editor actions are available without
keeping multiple windows open or repeatedly navigating menus.

The toolbar is an alternate presentation of existing editor commands. It
must reuse the same state and command authorities already established by
M36--M42 rather than introducing duplicate behavior.

Initial toolbar actions:

-   Translate
-   Resize
-   Apply Preview
-   Revert Working Copy
-   Save Level Source
-   Build configuration selector:
    -   Debug
    -   Development
    -   Release
    -   All
-   Run Build button

The toolbar can be shown or hidden through **View \> Quick Toolbar**.

### Design principles

1.  **One authority per action.** Toolbar buttons call the same commands
    as menus/windows.
2.  **Compact.** The toolbar should reduce window/menu friction, not
    become another large panel.
3.  **Stateful where useful.** Transform mode and selected build
    configuration must be visible.
4.  **Extensible, not generic.** Future milestones may add
    high-frequency shortcuts, but M43 must not create a generalized
    command registry/plugin toolbar framework.
5.  **Development-only authoring UI.** Do not expose authoring controls
    in Debug or Release.

### Layout

Preferred structure:

``` text
Menu Bar
File  Edit  View  Transform  Level  Build ...

Quick Toolbar
[Translate] [Resize] | [Apply] [Revert] [Save] | Build: [Development v] [Run]
```

The toolbar is fixed immediately below the menu bar and spans the
available editor width.

It is not an ordinary floating ImGui window.

When hidden, the viewport/editor content should reclaim its vertical
space.

### View menu integration

Add:

``` text
View
├ Metrics
├ Hierarchy
├ Inspector
├ Level Editor
├ Object Palette
├ Quick Toolbar
└ Tool Output
```

Exact ordering may follow the current project convention.

Requirements:

-   `View > Quick Toolbar` toggles visibility.
-   Visibility participates in the existing workspace authority.
-   Default visibility: **on**.
-   Reset Editor Layout restores the toolbar to its default visible
    state.
-   F2 hide/show behavior remains consistent with the editor.
-   No CWD `imgui.ini`.

### Transform shortcuts

Provide two mutually exclusive controls:

-   Translate
-   Resize

They must use the exact existing `TransformMode` authority.

Expected:

``` text
Translate active -> Translate appears pressed/active
Resize active    -> Resize appears pressed/active
```

Clicking Translate selects Translate mode.

Clicking Resize selects Resize mode.

The toolbar, `Transform` menu, and any existing mode UI must always
reflect the same state.

Do not create a second toolbar-specific transform state.

#### Availability

Respect existing selection/category rules.

If Resize is not valid for the current selection, the toolbar must
reflect the same availability policy as the existing editor behavior.

Do not enable an operation through the toolbar that the canonical
command path would reject.

### Apply Preview shortcut

Add a compact Apply action.

It must invoke the exact same Apply Preview command/handler used by:

-   Level menu
-   Level Editor window

Preserve all existing rules:

-   transactional physics rebuild,
-   working-copy validation,
-   Development/Debug policy already established,
-   pending lifecycle cleanup,
-   placement-mode cleanup from M42,
-   failure preservation semantics.

Do not create a toolbar-specific Apply implementation.

### Revert Working Copy shortcut

Add Revert.

It must call the existing Revert authority.

Preserve:

-   `workingCopy = active`,
-   pending lifecycle cleanup,
-   gizmo/transient cleanup,
-   M42 placement cancellation,
-   structural-map reset semantics.

### Save Level Source shortcut

Add Save.

It must call the existing Save Level Source authority.

Preserve:

-   Development-only authoring boundary,
-   Save writes active state only,
-   no implicit Apply,
-   existing validation/error handling,
-   deterministic Level Format v1 writer,
-   existing canonical source path behavior.

The toolbar must reflect whether Save is currently enabled.

### Toolbar iconography

Use compact icon-like controls where practical.

M43 does **not** require importing a new icon font, texture atlas, SVG
library, or asset dependency.

Preferred implementation order:

1.  Existing built-in/project icon resources, if already available.
2.  Small text/symbol labels that render reliably with the existing
    ImGui font.
3.  Compact labeled buttons if symbols would be ambiguous.

Tooltips are required for icon-only or abbreviated controls.

Examples:

-   Translate
-   Resize
-   Apply Preview
-   Revert Working Copy
-   Save Level Source
-   Run selected build

Do not use obscure Unicode glyphs if font coverage is uncertain.

### Build selector

Add a build configuration combo with:

-   Debug
-   Development
-   Release
-   All

Default on first use:

-   **Development**

The selected option must persist and be restored in later editor
sessions.

This selection is editor preference state, not level data.

It must never be serialized into Level Format v1.

### Build selection persistence

Persist the last selected build option using the existing editor
preference/layout persistence mechanism where appropriate.

Requirements:

-   First run/no saved value -\> Development.
-   User selects Debug -\> later editor session restores Debug.
-   User selects All -\> later editor session restores All.
-   Invalid/unknown persisted value -\> safely fall back to Development.
-   Reset Editor Layout behavior must be explicitly defined and tested.

Preferred M43 policy:

-   Reset Editor Layout resets window/workspace layout and toolbar
    visibility.
-   It does **not** need to reset the user's last build selection unless
    the existing preference architecture treats all such editor
    preferences as resettable together.

Choose one deterministic policy and document it.

Do not introduce a general settings database.

### Run Build button

The Run button executes the build currently selected in the combo.

Mapping:

``` text
Debug       -> existing Build Debug command
Development -> existing Build Development command
Release     -> existing Build Release command
All         -> existing Build All command
```

Use the existing `EditorToolRunner` command authority from M37.

Do not invoke CMake directly from toolbar code.

Do not duplicate process-launch logic.

Do not invent a new build orchestration layer.

### Build busy state

Preserve the existing one-job-at-a-time policy.

While a tool/build job is running:

-   Run Build must be disabled or otherwise prevented from starting a
    second job.
-   Existing Tool Output behavior remains authoritative.
-   Existing menu build commands must remain consistent.

The combo may remain changeable while a job is running only if doing so
does not alter the running job; otherwise disable it. Choose and test a
deterministic behavior.

### Tool Output interaction

Starting a build from the toolbar should behave like starting it from
the Build menu.

Do not force Tool Output open unless the existing command path already
does so.

Do not create separate toolbar logs.

### Toolbar geometry and editor viewport

The toolbar consumes vertical editor space below the menu bar.

Update any hard-coded editor geometry assumptions introduced by earlier
milestones, including where relevant:

-   orientation widget position,
-   viewport bounds,
-   mouse-to-viewport interpretation,
-   placement ray coordinates,
-   picking,
-   gizmo interaction,
-   camera navigation,
-   overlays.

Do not merely draw the toolbar over the existing viewport.

The viewport must start below the toolbar when the toolbar is visible.

When hidden, geometry must return to the menu-only layout.

This is a critical M43 acceptance requirement.

### M42 placement integration

Object Palette placement must remain correct with the toolbar visible or
hidden.

Verify:

-   candidate follows the correct viewport ray,
-   toolbar clicks never place objects,
-   toolbar does not shift ray/picking incorrectly,
-   placement preview remains aligned with the mouse,
-   gizmo interaction remains correct,
-   toolbar ImGui capture prevents accidental viewport actions.

### Toolbar and Object Palette

These are complementary:

-   Object Palette = object creation workflow.
-   Quick Toolbar = frequent global/editor commands.

Do not merge them into one window.

The user should be able to hide Object Palette and still use the
toolbar.

### Workspace persistence

Add toolbar visibility to the established editor workspace state.

Requirements:

-   default visible,
-   View menu sync,
-   persistence consistent with existing workspace policy,
-   Reset Editor Layout restores default,
-   no CWD `imgui.ini`.

Build selection persistence is separate from level data and should use
the smallest existing editor preference mechanism.

### Keyboard/input capture

Toolbar interactions must be treated as ImGui UI interactions.

Clicking toolbar controls must not:

-   place M42 objects,
-   viewport-pick,
-   begin gizmo drag,
-   rotate/move editor camera,
-   trigger Delete-selected behavior.

Preserve current-frame ImGui capture rules.

### Debug and Release boundaries

Development:

-   full Quick Toolbar.

Debug:

-   do not expose Development authoring toolbar controls unless the
    existing compile-time/editor policy explicitly supports the
    underlying action.
-   Preferred M43 behavior: Quick Toolbar is part of Development
    authoring workspace only.

Release:

-   no editor toolbar.

Do not broaden build-configuration feature boundaries accidentally.

### Non-goals

M43 must not add:

-   undo/redo,
-   copy/paste,
-   play/pause simulation controls,
-   prefab controls,
-   Object Palette category buttons inside toolbar,
-   Save All,
-   Cook/Stage/Reload toolbar buttons,
-   customizable toolbar ordering,
-   user-defined toolbar actions,
-   command palette,
-   keyboard shortcut editor,
-   icon/font dependency,
-   docking framework rewrite,
-   generic action registry.

Those may be evaluated later if actual usage justifies them.

### Automated acceptance

Add/update focused tests for:

1.  Toolbar default visible.
2.  View toggle hides/shows toolbar.
3.  Reset restores default visibility.
4.  Translate toolbar action changes canonical TransformMode.
5.  Resize toolbar action changes canonical TransformMode.
6.  Transform menu reflects toolbar change.
7.  Toolbar reflects menu transform change.
8.  Resize availability matches canonical selection rules.
9.  Apply toolbar request uses existing Apply action.
10. Revert toolbar request uses existing Revert action.
11. Save toolbar request uses existing Save action.
12. Apply enabled/disabled state matches canonical command.
13. Save enabled/disabled state matches canonical command.
14. Build selector defaults to Development.
15. Debug selection persists.
16. Development selection persists.
17. Release selection persists.
18. All selection persists.
19. Invalid persisted selection falls back safely.
20. Build Debug maps to existing command.
21. Build Development maps to existing command.
22. Build Release maps to existing command.
23. Build All maps to existing command.
24. Busy runner prevents second build.
25. Toolbar interaction consumes UI pointer.
26. Toolbar visible viewport top/bounds are correct.
27. Toolbar hidden viewport top/bounds are correct.
28. Orientation widget position accounts for toolbar height.
29. M42 placement ray remains correct with toolbar visible.
30. M42 placement ray remains correct with toolbar hidden.
31. Picking remains correct with toolbar visible/hidden.
32. Gizmo remains correct with toolbar visible/hidden.
33. F2 lifecycle does not leave stale toolbar interaction state.
34. Existing Build menu behavior remains unchanged.
35. Existing Level menu behavior remains unchanged.
36. Existing Transform menu behavior remains unchanged.
37. Level Format v1 unchanged.

### Manual acceptance

#### Toolbar visibility

1.  Run Development.
2.  Open F2.
3.  Confirm Quick Toolbar is visible below menu bar.
4.  Toggle `View > Quick Toolbar`.
5.  Confirm toolbar disappears and viewport reclaims space.
6.  Toggle it on again.

#### Transform

1.  Select a Platform.
2.  Click Translate toolbar control.
3.  Confirm Translate active and Resize inactive.
4.  Click Resize.
5.  Confirm Resize active and Translate inactive.
6.  Change mode through Transform menu.
7.  Confirm toolbar updates immediately.

#### Apply / Revert

1.  Create or modify a pending object.
2.  Use toolbar Apply.
3.  Confirm same behavior as Level \> Apply Preview.
4.  Create another pending modification.
5.  Use toolbar Revert.
6.  Confirm same behavior as Level \> Revert Working Copy.

Restore canonical state as necessary.

#### Save

Use only after ensuring active state is canonical or with a deliberate
temporary test that will be restored.

Verify:

-   Save availability is correct.
-   Toolbar Save invokes the existing Save behavior.
-   No implicit Apply occurs.

#### Build selector

1.  Confirm initial/default selection is Development when no preference
    exists.
2.  Select Debug and run.
3.  Verify Debug build command runs through Tool Runner.
4.  Select Development and run.
5.  Select Release and run.
6.  Select All and run.
7.  Confirm one-job-at-a-time behavior.

No need to repeat expensive builds manually if automated command-mapping
tests plus one or two real toolbar launches sufficiently validate the UI
path.

#### Build persistence

1.  Choose a non-default build option.
2.  Close/reopen the Development editor/application as appropriate.
3.  Confirm the last selection is restored.

#### M42 regression

With toolbar visible:

1.  Open Object Palette.
2.  Enter Collectible placement.
3.  Move mouse across Ground/Platform/empty space.
4.  Confirm candidate alignment remains correct.
5.  Click toolbar controls while placement mode is active.
6.  Confirm toolbar click does not create an object.
7.  Return to viewport and place normally.
8.  Test pending viewport picking.
9.  Revert.

Repeat the key alignment check with toolbar hidden.

#### Input safety

Verify toolbar clicks do not:

-   select world objects,
-   place objects,
-   manipulate gizmo,
-   move/rotate camera.

#### Layout

Verify:

-   toolbar does not overlap menu bar,
-   toolbar does not overlap viewport content,
-   orientation widget remains correctly positioned,
-   no CWD `imgui.ini`.

### Regression suites

Run all applicable configurations for:

``` text
AuthoredObjectLifecycleTest
AuthoredLifecycleIntegrationTest
EditorWorkspaceTest
EditorGizmoTest
EditorOrientationTest
EditorPickingTest
EditorPlacementTest
LevelFileTest
PhysicsRebuildTest
CookStageReloadWorkflowTest
EditorToolRunnerTest
```

plus new M43 toolbar tests.

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

Before M43 closure confirm canonical Level 01:

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

-   no intended canonical level diff,
-   diff check clean.

If only the known EOL/LF-CRLF artifact appears and there is no intended
semantic level change, use the established safe worktree restore:

``` text
git restore --worktree --source=HEAD -- game/assets/source/levels/level_01.level
```

Do not normalize EOL.

### Documentation

Update as appropriate:

-   `README.md`
-   `AGENTS.md`
-   `docs/ARCHITECTURE.md`
-   `docs/MILESTONES.md`

Document:

-   Quick Toolbar scope.
-   View toggle.
-   Shared command/state authority.
-   Transform mutual exclusivity.
-   Apply/Revert/Save mappings.
-   Build selector and persistence.
-   Build runner mapping.
-   Toolbar impact on viewport geometry.
-   Development-only boundary.
-   No Level Format change.

### Completion criteria

M43 is complete only when:

-   Quick Toolbar is fixed below the menu bar.
-   View toggle works.
-   Toolbar visibility participates in workspace state.
-   Translate/Resize share canonical TransformMode.
-   Apply/Revert/Save share canonical actions.
-   Build selector supports Debug/Development/Release/All.
-   Development is the first-run default.
-   Last build selection persists.
-   Run invokes existing EditorToolRunner build commands.
-   Busy-state behavior is safe.
-   Toolbar geometry correctly adjusts the viewport.
-   M42 placement/picking/gizmo behavior remains correct with toolbar
    visible/hidden.
-   Automated tests pass.
-   Manual acceptance passes.
-   Debug/Development/Release builds pass.
-   Canonical level is restored.
-   Git checks are clean.
-   No unrelated scope is introduced.

### Git closure

Do not commit/push/merge until manual acceptance is approved.

After approval:

``` text
test
-> approve
-> inspect diff/status
-> commit once
-> push milestone branch
-> merge normally into main
-> push main
-> confirm clean/synced main
```

Do not use `--no-ff` automatically.
