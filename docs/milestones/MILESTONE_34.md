## Milestone 34 --- Visual Level Editor v3: Transform Gizmo + Persistent Editor Layout

### Status

Phase A complete: translation-gizmo math, pending-preview helpers, layout path
and ImGui ini ownership are implemented and tested. Phase B complete: live
axis drag, gizmo rendering, pending ghost, persistent layout, and Reset Editor
Layout. Phase C complete: overlay gizmo + canonical Level 01 restore. M34 is
**complete and merged**.

### Branch

`milestone/34-transform-gizmo-persistent-layout`

### Goal

Make the M33 visual editor materially faster to use by adding the first
direct-manipulation tool: a **world-space translation gizmo** for
supported authored objects.

M34 also adds **local persistence of the Dear ImGui editor window
layout**, so Metrics, Level Editor, Inspector and Hierarchy reopen where
the developer left them, with an explicit **Reset Editor Layout**
action.

The milestone preserves the established authoring contract:

``` text
Inspector / gizmo edits -> workingCopy
Viewport world          -> active/applied LevelDefinition
Apply Preview            -> commits workingCopy into visible/physical preview
Save Level Source        -> persists active/applied level source
```

### User experience target

``` text
F2
→ select Spawn, Ground or an Elevated Platform
→ X/Y/Z translation gizmo appears
→ drag one axis
→ Inspector updates immediately
→ Modified = true
→ active render/physics stay unchanged until Apply Preview
→ pending editor preview represents the working transform
→ Apply Preview moves render/physics/picking/highlight together
```

Window workflow:

``` text
Arrange Metrics / Level Editor / Inspector / Hierarchy
→ close application
→ reopen
→ positions/sizes restored

Reset Editor Layout
→ project default layout restored and persisted
```

### Translation Gizmo v1

#### Editable gizmo targets

Translation gizmo is available only for the existing positional
M33-editable world objects: - Player Spawn - Ground - Elevated Platform
0..5

Camera remains numerically editable in Inspector but has no world gizmo
because its authored data is gameplay-camera framing/offset rather than
a placed scene camera.

M33 read-only types remain read-only and receive no editable gizmo.

#### Modes

M34 implements **translation only**, world-space: - X - Y - Z

No rotation, scale, universal gizmo, snapping, or local/world mode
switching.

#### Working-copy contract

Gizmo manipulation edits the selected object's `workingCopy` position.

During drag: - workingCopy changes; - Inspector values change; -
Modified becomes true; - Dirty retains M32 semantics.

Normal active world rendering, physics, normal world picking and normal
M33 highlight do not move before Apply Preview.

#### Pending transform preview

Because active geometry remains unchanged before Apply, show a clear
editor-only pending/ghost representation at the working-copy transform
whenever the selected supported object's working transform differs from
active.

The active/applied object remains visible at its current position.

The gizmo follows the working-copy position.

The pending preview: - is editor-only; - does not affect physics; - is
visually distinguishable from active geometry; - does not replace the
active-world picking contract.

#### Selection semantics

Selection remains `EditorSelection { kind, index }`.

For a selected supported object: - Inspector → workingCopy; - gizmo
target → workingCopy; - pending preview → workingCopy; - normal world
picking/highlight → active/applied world.

No second selection system.

#### Interaction priority

Required conceptual priority:

``` text
ImGui interaction
    > active gizmo drag / gizmo handle
    > ordinary world picking
```

RMB editor-camera look must not manipulate the gizmo. A gizmo click must
not clear/change object selection. Dragging must remain constrained to
the chosen X/Y/Z axis.

Use focused project-owned ray/math helpers and robust behavior for
near-parallel camera/axis cases. Do not build a generic gizmo framework.

#### Inspector synchronization

Gizmo and Inspector are two interfaces over the same workingCopy.

Gizmo drag updates Inspector. Numeric Inspector edits reposition
gizmo/pending preview before Apply.

#### Apply/Revert/F2

Apply Preview: - commits/rebuilds as before; - active world moves to
working transform; - pending preview disappears when active ==
working; - selection preserved; - editor camera preserved; - gizmo
remains on selected object at applied position.

Revert Working Copy: - discards pending numeric/gizmo edits; - gizmo
returns to active position; - pending preview disappears; -
selection/editor camera preserved.

F2: - M33 rule remains: unapplied edits are discarded on reopen; - no
gizmo drag may remain latched across editor close.

### Persistent Editor Window Layout

#### Scope

Persist Dear ImGui layout for at least: - Metrics - Level Editor -
Inspector - Hierarchy

Use native Dear ImGui ini persistence where practical, including
position, size and naturally supported collapsed state.

#### Storage

Layout is developer-local tooling state.

Do not store it in: - LevelDefinition; - level_01.level; - BEST save; -
repository; - current working directory.

Preferred Windows path:

``` text
%LOCALAPPDATA%\Platformer3D\editor_layout.ini
```

Use existing platform/user-data boundaries where appropriate.

Development: persistence enabled. Debug: may use the same safe local
layout path. Release: no editor/layout persistence.

Do not allow Dear ImGui to create a default repository/CWD `imgui.ini`.

#### Default layout

Define a sensible project default arrangement for the current windows
while keeping the central 3D view useful.

Do not add docking.

#### Reset Editor Layout

Add a visible action:

`Reset Editor Layout`

It must restore the project-defined default positions/sizes and make
that layout the persisted layout for future launches.

An optional non-conflicting shortcut is allowed, but the visible UI
action is required.

It must not reset authored/gameplay data.

#### Robustness

Editor startup must remain safe when: - layout file is missing; - layout
file is malformed/unreadable; - application window size changes; -
stored positions are completely off-screen.

Use the smallest reasonable recovery behavior; no multi-monitor
workspace manager.

### Architecture constraints

Preserve: - C++20/CMake architecture; - gameplay never calls raylib
directly; - no editor state in LevelDefinition; - M33 single
selection; - M33 CPU world picking; - active-world pick/highlight
contract; - M32 Apply/Revert/Save semantics; - Development-only source
authoring; - Debug non-authoring; - Release editor-free; - BEST
independence; - portability boundaries.

Prefer a focused project-owned translation gizmo. Do not add a
third-party gizmo dependency without explicit approval.

### Phase A --- Architecture and math

Inspect the real M33 implementation and establish: 1. workingCopy
position access/mutation helpers; 2. exact gizmo-supported categories;
3. gizmo state ownership; 4. X/Y/Z handle geometry; 5. handle
hit-testing; 6. drag-start state; 7. constrained axis drag math; 8.
near-parallel stability policy; 9. ImGui/gizmo/world-pick/editor-camera
interaction priority; 10. pending-preview representation; 11.
active-vs-working rendering semantics; 12. Inspector/gizmo
synchronization; 13. Apply/Revert/F2 behavior; 14. focused gizmo math
tests; 15. ImGui ini persistence path; 16. default-layout mechanism; 17.
reset-layout mechanism; 18. missing/corrupt/off-screen recovery; 19.
Debug/Development/Release policy.

Phase A may implement focused math/state scaffolding and tests, but must
not enable the complete live gizmo workflow before review.

**Phase A outcome (complete):**

Gizmo. `editor::EditorAxis` / `GizmoInteractionState` on `LevelEditorState`.
`GetEditablePosition` mutates `workingCopy` for Spawn, Ground, Platform 0..5
only. Camera and M33 read-only kinds return null. Drag uses a camera-facing
plane containing the world axis; `|axis × viewForward| < 0.05` falls back to
closest-points; unusable rays keep the start pose. Position is
`start + (param - startParam) * axis` (no per-frame accumulation). Visual
length scales with distance/FOV and is clamped. Hit radius is 3× the visual
shaft fraction. `EditorGizmoTest` covers support, X/Y/Z isolation, continuity,
near-parallel finiteness, handle pick, pending preview vs active, and layout
path/defaults/clamp.

Layout. `IniFilename` is a `DebugUiBackend`-owned `std::string` at
`%LOCALAPPDATA%\Platformer3D\editor_layout.ini`. Debug and Development share it.
Release does not create or read it. First-run uses `ImGuiCond_FirstUseEver`
defaults (metrics/hierarchy left, inspector/level editor right). Reset UI is
Phase B (`ImGuiCond_Always` for one frame). No CWD `imgui.ini`.

Not live: Application does not hover/drag gizmo handles, Renderer does not
draw axes or a pending ghost, and there is no Reset Editor Layout button.

### Phase B --- Live implementation

**Phase B outcome (complete, awaiting Phase C):**

Live gizmo. After this-frame ImGui capture, `UpdateGizmoInteraction` hovers and
drags using Phase A `PickGizmoHandle` / constrained-axis math. Application
writes `workingCopy` only. Overlay order: active spawn/highlight, cyan pending
ghost, then X/Y/Z handles. RMB never starts a drag; active LMB drag suppresses
look. Apply/Revert/F2 clear gizmo state. Camera and M33 read-only kinds have no
gizmo and no pending world preview.

Layout. Reset Editor Layout in the Level Editor actions snaps Metrics,
Hierarchy, Inspector, and Level Editor (named `SetWindowPos` plus two frames of
`ImGuiCond_Always`) and `SaveIniSettingsToDisk`. One-shot off-screen clamp after
first Begin of Metrics and of the editor windows. Debug/Development share
`%LOCALAPPDATA%\Platformer3D\editor_layout.ini`. Release does not touch it.

### Phase C --- Manual validation

**Phase C correction:** gizmo X/Z handles were depth-tested `DrawLine3D`
geometry whose origin sits inside Ground/Platform/Spawn AABBs, so horizontal
axes disappeared. Renderer now draws the editor overlay *after* cooker probes,
uses a faint depth-tested pass then a depth-independent cylinder/cone overlay
(`rlDisableDepthTest` / `rlDisableDepthMask` / `rlDisableBackfaceCulling`), and
restores those states immediately. Handle picking is unchanged (world-space
shafts). Canonical `level_01.level` restored from M33 HEAD and recooked. M34
still awaits final approval. Milestone 35 has not started.

Validate at minimum:

#### Gizmo

-   Spawn, Ground and several Platforms show gizmo;
-   Camera has no world gizmo;
-   M33 read-only types have no editable gizmo;
-   X/Y/Z each change only their own coordinate;
-   multiple camera angles are stable;
-   gizmo clicks/drags do not world-pick another object;
-   RMB look does not drag gizmo;
-   ImGui interaction does not drag gizmo;
-   Inspector updates from gizmo;
-   numeric Inspector edits reposition gizmo/pending preview;
-   Modified/Dirty semantics remain correct;
-   active render/physics/picking/highlight remain unchanged before
    Apply;
-   pending preview represents working transform;
-   Apply moves render/physics/picking/highlight together;
-   pending preview disappears after Apply;
-   Apply preserves selection/editor camera;
-   Revert restores working transform;
-   F2 discards unapplied edits;
-   repeated drag/Apply cycles are stable.

#### Layout

-   arrange Metrics, Level Editor, Inspector and Hierarchy;
-   close and relaunch;
-   positions/sizes restore;
-   persistence path is outside repo/CWD;
-   unrelated-CWD launch restores layout;
-   Reset Editor Layout restores project defaults;
-   relaunch after reset keeps defaults;
-   missing layout file falls back safely;
-   malformed/unreadable layout does not crash;
-   resized window remains usable;
-   Debug policy works;
-   Release has no layout/editor behavior.

#### Regressions

-   M33 picking/navigation/selection;
-   Apply/Revert/Save;
-   source authoring;
-   BEST;
-   Level Format v1;
-   cooker;
-   unrelated CWD;
-   canonical Level 01 restored before closure.

### Acceptance criteria

M34 is complete only when: 1. Spawn, Ground and Elevated Platforms can
be translated with a world-space X/Y/Z gizmo. 2. Gizmo and Inspector
edit the same workingCopy. 3. Unapplied gizmo edits do not move active
render or physics. 4. Pending preview clearly represents working
transform. 5. Normal picking/highlight remain on active world before
Apply. 6. Apply moves render/physics/picking/highlight together. 7.
Revert/F2 preserve established semantics. 8. Gizmo does not conflict
with ImGui, world picking or editor camera. 9. M33 read-only types
remain read-only. 10. Camera remains numeric-only. 11. editor window
layout persists across launches. 12. persistence is developer-local and
CWD/repository independent. 13. Reset Editor Layout restores a project
default. 14. invalid/missing layout cannot prevent startup. 15. Release
remains editor-free. 16. Level Format v1 remains unchanged. 17.
LevelDefinition contains no editor state. 18. automated builds/tests
pass. 19. Phase C passes. 20. canonical gameplay values are restored
before Git closure. 21. no commit/push/merge occurs before approval.

### Explicitly out of scope

Do NOT implement: - rotation gizmo; - scale gizmo; - universal gizmo; -
snapping/grid snapping; - local/world mode switching; - pivot editing; -
object creation/deletion/duplication; - undo/redo; - copy/paste; -
multi-select; - box selection; - Level 02; - New/Open/Save As; - scene
graph/SceneManager/ECS/reflection; - generic Inspector/serialization; -
docking; - saved workspaces; - multi-monitor workspace management; -
enemies/bosses/combat; - asset browser; - Level Format v2; - save v2.

### Future direction

M34 establishes direct manipulation and local editor preferences. Later
milestones can build on it with rotation/scale where meaningful,
snapping, dynamic add/delete/duplicate, stable authored IDs when
required, and multiple levels/scenes.
