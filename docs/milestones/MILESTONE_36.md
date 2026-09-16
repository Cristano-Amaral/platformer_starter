## Milestone 36 --- Editor Menu Bar & Workspace Controls

### Status

Phase B live integration implemented: F2 Dear ImGui main menu bar (View /
Transform / Level) shares workspace visibility, `transformMode`, and
`LevelEditorRequest` with the existing panels. Automated tests/builds are
part of Phase B. Milestone 36 is **not** complete until Phase C manual
validation. Cooker/build integration is deferred. Milestone 37 has not
started.

### Branch

`milestone/36-editor-menu-bar-workspace-controls`

### Recommended Cursor model

**Grok 4.6 High --- Fast OFF**

------------------------------------------------------------------------

### 1. Goal

Consolidate the Development/Debug visual editor controls introduced in
M32--M35 into a clear, persistent top editor menu bar without changing
the authored level model, gameplay, physics, level format, or existing
editor semantics.

M36 is an **editor UX consolidation milestone**, not a new
authoring-capability milestone.

When F2 editor mode is active, a top menu bar provides centralized
access to:

-   editor panel visibility;
-   transform mode selection;
-   existing level actions;
-   editor layout reset.

The existing M32--M35 behavior remains authoritative. The menu bar must
call the same state/actions rather than introducing parallel
implementations.

------------------------------------------------------------------------

### 2. Why M36 exists

The editor now has multiple independent surfaces:

-   Platformer3D Metrics;
-   Hierarchy;
-   Inspector;
-   Level Editor;
-   Translate / Resize;
-   Apply Preview;
-   Revert Working Copy;
-   Save Level Source;
-   Reset Editor Layout.

These controls are useful but increasingly distributed across panels.
Before adding structurally larger features such as object
creation/removal, multiple levels, rotation, undo/redo, or external
build tooling, M36 creates a stable editor workspace shell.

This keeps the editor evolution incremental and reduces future UI
duplication.

------------------------------------------------------------------------

### 3. In scope

#### 3.1 Top editor menu bar

Visible only while the F2 editor is active.

Required top-level menus:

-   `View`
-   `Transform`
-   `Level`

The menu bar is editor UI, not gameplay HUD.

#### 3.2 View menu

Provide checkable visibility controls for exactly these existing
windows:

-   `Metrics`
-   `Hierarchy`
-   `Inspector`
-   `Level Editor`

Each menu item manipulates the same authoritative visibility state used
to decide whether the corresponding window is drawn.

No duplicate visibility state is allowed.

#### 3.3 F1 integration

F1 remains the Metrics shortcut.

F1 and `View > Metrics` must modify the **same Metrics visibility
state**.

Examples:

-   Metrics visible → F1 hides it → menu becomes unchecked.
-   Metrics hidden → `View > Metrics` shows it → F1 state follows.

#### 3.4 Transform menu

Expose the existing M35 transform mode:

-   `Translate`
-   `Resize`

The menu must use the existing `EditorTransformMode`.

No second transform-mode state.

The existing transform controls inside the Level Editor panel may remain
during M36 unless removing them is clearly simpler and regression-free.
If both surfaces exist, they must stay synchronized.

#### 3.5 Level menu

Expose the existing actions:

-   `Apply Preview`
-   `Revert Working Copy`
-   `Save Level Source`
-   separator
-   `Reset Editor Layout`

These must invoke the existing M32--M35 action/request paths.

Do not duplicate Apply, Revert, Save, or Reset logic inside menu code.

#### 3.6 Enable/disable semantics

Menu actions must respect the same rules as their existing panel
equivalents.

At minimum:

-   `Apply Preview`: enabled only when the current existing Apply
    semantics allow it.
-   `Revert Working Copy`: enabled only when the current existing Revert
    semantics allow it.
-   `Save Level Source`: Development authoring only and subject to the
    existing Save rules.
-   Debug must not gain source authoring.
-   `Reset Editor Layout`: available in Debug and Development editor
    builds.

#### 3.7 Panel close buttons

Hierarchy, Inspector, Level Editor, and Metrics should support normal
ImGui close behavior where practical.

Closing a window through its close button must update the same
visibility state represented by `View`.

A panel hidden this way must be recoverable through the menu bar.

#### 3.8 Editor reopen behavior

F2 closes editor mode as before.

When reopening F2 during the same process:

-   existing editor camera session behavior remains unchanged;
-   selection behavior remains unchanged;
-   transform mode session behavior remains unchanged;
-   panel visibility should remain as the user last set it during that
    process.

Do not serialize panel visibility into `LevelDefinition`.

#### 3.9 Layout persistence

Continue using the existing M34 `editor_layout.ini` for ImGui window
geometry/collapse state.

M36 may persist panel open/closed visibility in the same
editor-preference domain **only if Dear ImGui naturally provides a
reliable solution without custom level/save data**.

Preferred M36 rule:

-   position/size/collapse: existing ImGui INI behavior;
-   panel visibility: session state.

Do not create a new general settings format merely for M36.

#### 3.10 Reset Editor Layout

`Reset Editor Layout` must continue to reset the four known editor
windows to the project defaults.

It must **not**:

-   mutate the active level;
-   mutate `workingCopy`;
-   change Modified/Dirty;
-   change BEST;
-   change selection;
-   change editor camera;
-   invoke Apply/Revert/Save;
-   invoke cooker/build tools.

Panel visibility after Reset should be deterministic. Preferred
behavior: restore all four editor panels visible.

#### 3.11 Menu bar layout impact

The top menu bar consumes screen space.

Update viewport/editor overlay placement where necessary so that:

-   the orientation widget does not collide with the menu bar;
-   editor 3D interaction remains aligned with the actual viewport;
-   mouse-ray picking remains correct;
-   gizmo hit testing remains correct;
-   ImGui capture remains authoritative.

Do not casually offset camera projection/picking without tracing the
actual raylib/ImGui coordinate relationship.

#### 3.12 Orientation widget

The M35 orientation widget remains viewport UI.

It must remain:

-   upper-right;
-   left of the default Inspector region;
-   below the menu bar with comfortable spacing;
-   adaptive to window resize;
-   non-persisted.

Its Front/Back/Left/Right/Top behavior is unchanged.

------------------------------------------------------------------------

### 4. Out of scope

M36 must **not** implement:

-   Cooker execution from the editor;
-   CMake/build execution from the editor;
-   Build Debug/Development/Release/All;
-   external process launching;
-   build console/log window;
-   Add Object;
-   Delete Object;
-   Duplicate Object;
-   dynamic object counts;
-   Level Format v2;
-   new level/scene creation;
-   Open/Save As;
-   multiple levels;
-   rotation gizmo;
-   generic scale system beyond M35 Resize;
-   snap;
-   undo/redo;
-   copy/paste;
-   docking;
-   SceneManager;
-   ECS/reflection;
-   asset browser;
-   runtime player authoring;
-   new gameplay systems.

External Cooker/build integration is intentionally deferred to a later
dedicated milestone because process execution/build orchestration is
architecturally different from editor workspace UI.

------------------------------------------------------------------------

### 5. Architecture rules

#### 5.1 One source of truth

Menu controls must route into existing editor state/actions.

Examples:

`View > Inspector` → existing Inspector visibility state.

`Transform > Resize` → existing `EditorTransformMode::Resize`.

`Level > Apply Preview` → existing Apply request path.

No menu-specific Apply implementation.

#### 5.2 UI requests, Application ownership

Preserve the existing M32 pattern where UI produces requests and
`Application` executes operations that own gameplay/physics/runtime
consequences after the frame at the appropriate point.

Do not let ImGui menu code directly rebuild physics or replace the
active level.

#### 5.3 Renderer boundaries

Renderer remains unaware of editor menu semantics.

Renderer may receive only the same read-only overlay requests/camera
data needed for rendering.

#### 5.4 Gameplay isolation

Gameplay input/state must not depend on menu bar state.

F2 remains the semantic editor toggle boundary.

Release gameplay remains unchanged.

#### 5.5 Configurations

**Debug** - F2 editor available; - menu bar available; - visual editor
tools available; - source authoring Save unavailable.

**Development** - full M36 menu bar; - existing source authoring Save
available under existing rules.

**Release** - no editor menu bar; - no ImGui editor; - F2 remains
no-op; - no editor layout behavior added.

------------------------------------------------------------------------

### 6. Preferred menu structure

``` text
View            Transform          Level
├─ [✓] Metrics  ├─ ● Translate     ├─ Apply Preview
├─ [✓] Hierarchy└─ ○ Resize        ├─ Revert Working Copy
├─ [✓] Inspector                   ├─ Save Level Source
└─ [✓] Level Editor                ├────────────────────
                                    └─ Reset Editor Layout
```

Exact spacing is not important. Semantics are.

------------------------------------------------------------------------

### 7. Menu bar behavior

-   Menu bar appears when F2 editor mode is active.
-   Menu bar disappears when F2 editor mode is inactive.
-   It should span the usable top area naturally.
-   It must not create a second OS window.
-   It must not require docking.
-   ImGui interaction with the menu bar must block world picking/gizmo
    interaction beneath it.
-   Clicking a menu item must never also select a world object.
-   Keyboard navigation/focus inside ImGui must continue to respect
    capture.

------------------------------------------------------------------------

### 8. Panel visibility model

Introduce the minimum project-owned editor workspace state needed, for
example conceptually:

``` cpp
struct EditorWorkspaceState
{
    bool showMetrics = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showLevelEditor = true;
};
```

The exact type/location should follow the existing code ownership
discovered in Phase A.

Do not blindly add this exact struct if the project already has a
cleaner authoritative location.

The important invariant is one visibility value per panel.

------------------------------------------------------------------------

### 9. Existing panel compatibility

#### Metrics

-   F1 still works.
-   `View > Metrics` stays synchronized.
-   Reset Layout can restore visibility.

#### Hierarchy

-   hidden panel must not affect selection ownership.
-   selected object remains selected while Hierarchy is hidden.

#### Inspector

-   hiding Inspector must not mutate `workingCopy`.
-   selection remains valid.

#### Level Editor

-   hiding it must not change transform mode.
-   hiding it must not Apply/Revert/Save.
-   menu actions remain available while the panel is hidden.

------------------------------------------------------------------------

### 10. Transform compatibility

Switching Translate/Resize from the menu must behave exactly like
switching from the existing M35 controls.

It must not:

-   Apply;
-   Revert;
-   clear Modified;
-   change Dirty;
-   change selection;
-   change editor camera.

If a gizmo drag is active when a transform-mode change is requested, use
the existing safe interaction policy. Do not redirect an active drag
into another mode.

------------------------------------------------------------------------

### 11. Level action compatibility

#### Apply Preview

Preserve: - validation; - physics rebuild; - active/working semantics; -
selection; - editor camera; - BEST isolation.

#### Revert Working Copy

Preserve: - working = active; - gizmo interaction cleanup; -
selection/camera preservation.

#### Save Level Source

Preserve: - Development-only authoring path; - active/applied definition
only; - disabled while current existing Save semantics disallow it; -
deterministic writer; - safe replacement; - no automatic
cooker/build/restart.

#### Reset Editor Layout

Preserve M34 behavior plus deterministic panel visibility restoration.

------------------------------------------------------------------------

### 12. Tests

Add focused no-window tests where useful for project-owned
workspace/menu decision logic.

At minimum preserve and rerun:

-   `EditorGizmoTest`
-   `EditorOrientationTest`
-   `EditorPickingTest`
-   `LevelFileTest`
-   `PhysicsRebuildTest`
-   cooker tests

Do not weaken M34/M35 tests.

If workspace visibility/action-enable logic is extracted into
project-owned pure helpers, add a focused test for:

-   default panel visibility;
-   F1/menu Metrics synchronization;
-   transform mode synchronization;
-   Debug vs Development Save availability;
-   Reset restoring default visibility without changing authored state.

Do not attempt to unit-test Dear ImGui itself.

------------------------------------------------------------------------

### 13. Manual validation

Phase C must manually validate at least:

1.  F2 shows menu bar.
2.  F2 hides menu bar.
3.  View toggles each of four panels independently.
4.  Closing a panel updates View check state.
5.  Hidden panel can be restored from View.
6.  F1 and View \> Metrics remain synchronized.
7.  Selection survives hiding/showing Hierarchy.
8.  Working edits survive hiding/showing Inspector.
9.  Transform mode survives hiding/showing Level Editor.
10. Translate menu selection works.
11. Resize menu selection works.
12. Existing panel transform controls, if retained, synchronize with
    menu.
13. Apply from menu works.
14. Revert from menu works.
15. Save from menu works in Development.
16. Save is unavailable/disabled in Debug.
17. Reset Layout from menu resets all four windows.
18. Reset restores deterministic panel visibility.
19. Reset does not change level/editor gameplay state.
20. Menu clicks do not world-pick.
21. Menu interaction does not start gizmo drag.
22. Orientation widget remains correctly placed below/right of menu bar.
23. Orientation widget remains clickable.
24. Resize/Translate gizmos remain correctly pickable.
25. Window resize preserves menu/widget/picking alignment.
26. layout persistence regression.
27. unrelated-CWD regression.
28. Debug editor regression.
29. Release has no menu/editor.
30. canonical `level_01.level` remains clean.

------------------------------------------------------------------------

### 14. Phase plan

#### Phase A --- Audit & workspace architecture

-   inspect current DebugUi/LevelEditor/Metrics ownership;
-   trace F1/F2 state;
-   trace M35 transform state;
-   trace LevelEditorRequest Apply/Revert/Save/Reset;
-   design one-source-of-truth workspace visibility;
-   design menu request routing;
-   identify menu-bar impact on viewport/orientation-widget coordinates;
-   add pure helpers/tests where useful;
-   do not wire the live menu yet.

#### Phase B --- Live menu integration

-   render top menu bar in Debug/Development F2 mode;
-   wire View;
-   wire F1 synchronization;
-   wire Transform;
-   wire Level actions through existing requests;
-   support panel close/reopen;
-   update Reset behavior;
-   adjust orientation widget placement for menu bar;
-   preserve capture/picking/gizmo behavior;
-   builds/tests/smokes.

#### Phase C --- Manual validation

-   full UX validation checklist above;
-   correct only M36 regressions;
-   restore canonical level source;
-   final builds/tests;
-   no commit until approval.

------------------------------------------------------------------------

### 15. Completion criteria

M36 is complete only when:

-   top menu bar is live in Debug/Development editor mode;
-   all four panel visibility toggles work from one authoritative state;
-   F1 Metrics synchronization works;
-   Translate/Resize menu controls use the existing transform mode;
-   Apply/Revert/Save/Reset use existing action paths;
-   Debug/Development authoring boundaries remain correct;
-   menu interaction cannot leak into world picking/gizmos;
-   orientation widget and picking remain correct after menu-bar
    introduction;
-   Release remains editor-free;
-   all tests/builds/cooker regressions pass;
-   manual Phase C passes;
-   canonical Level 01 source is clean;
-   M37 has not started.

------------------------------------------------------------------------

### 16. Deferred backlog

Not part of M36, but preserved for future milestones:

-   editor tool runner;
-   Cook Assets;
-   Build Debug;
-   Build Development;
-   Build Release;
-   Build All;
-   build/cooker output console;
-   object Add/Delete/Duplicate;
-   dynamic authored collections;
-   multiple levels/scenes;
-   New/Open/Save As;
-   rotation;
-   snap;
-   undo/redo;
-   focus/frame selected.

These must not be opportunistically implemented during M36.
