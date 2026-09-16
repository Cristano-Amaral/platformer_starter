## Milestone 33 --- Visual Level Editor v2: Viewport Navigation, Hierarchy & World Selection

### Status

Phase B implemented: live editor camera, Hierarchy, world picking, Inspector
routing and selection highlight. M33 is **not** complete. Phase C manual
validation remains. Milestone 34 has not started.

### Branch

`milestone/33-visual-level-editor-v2`

### Goal

Evolve the M32 property editor into the first genuinely visual
level-editing workflow.

M33 must let the developer enter editor mode, navigate the 3D level
independently of the gameplay camera, see an explicit hierarchy of
authored level objects, and select supported objects either from the
hierarchy or by clicking them in the 3D world. Selection must stay
synchronized with the Inspector.

This milestone deliberately does **not** add object creation/deletion or
transform gizmos yet. Its purpose is to establish the selection/viewport
architecture those features will build on.

### User experience target

``` text
Development
→ F2 opens editor and pauses gameplay
→ editor camera can navigate around the level
→ Hierarchy lists authored objects
→ click an object in Hierarchy OR click it in the world
→ same object becomes selected
→ Inspector shows that object's supported properties
→ existing M32 edit / Apply Preview / Save workflow continues
```

### Scene/level terminology

For the current project, the active external `LevelDefinition` is the
authored gameplay scene/level.

M33 must not introduce a generic `SceneManager` or engine-wide scene
graph merely to rename this concept. The editor may use the term
**Level** or **Scene/Level** in its UI, but runtime ownership remains
based on the existing `LevelDefinition`.

Multiple levels, New/Open/Save As, and level transitions remain future
work.

### In scope

#### Editor viewport/navigation

-   Development/Debug editor mode retains paused gameplay.
-   Independent editor/navigation camera while editor is active.
-   Mouse + keyboard navigation suitable for inspecting a 3D platformer
    level.
-   Gameplay camera state must be preserved/restored appropriately when
    leaving editor mode.
-   Navigation must not mutate authored camera offset/FOV unless the
    user edits those Inspector properties explicitly.

#### Hierarchy

A focused hierarchy/list of the currently authored LevelDefinition
objects, including at least: - Ground - Platform 0..5 - Slope 0..1 -
Moving Platform - Checkpoint 0..1 - Hazard 0..1 - Collectible 0..2 -
Goal - Dynamic Cyan Box - Player Spawn - Camera

Read-only/non-editable object types may still be selectable and
inspectable.

#### Stable editor selection

Introduce a small, explicit editor selection identity suitable for the
current fixed Level Format v1 categories.

It must identify object type/category plus index where required.

Examples conceptually: - Ground - ElevatedPlatform\[0\] - Slope\[1\] -
Hazard\[0\] - Collectible\[2\] - Goal - Spawn - Camera

Do not introduce a generic entity/component system, UUID database,
reflection system, or scene graph.

#### Hierarchy selection

Clicking an item in the hierarchy selects it.

The selected row/item is visibly highlighted.

#### World selection / picking

Clicking a selectable object in the 3D viewport selects the same object
represented in the hierarchy.

Use project-owned CPU-side ray construction/intersection or the smallest
renderer/input boundary consistent with the existing architecture.

Do not make gameplay code depend directly on raylib.

#### Selection synchronization

Selection is one state shared by: - world picking; - hierarchy; -
Inspector.

Selecting in any supported place updates the others.

#### Selection visualization

The selected world object must have a clear Development/Debug-only
visual indication such as: - wireframe bounds; - outline-like debug
bounds; - bounding box; - other simple non-production selection marker.

Prefer a simple robust debug visualization over a complex outline
shader.

#### Inspector integration

Reuse and improve the M32 Level Editor inspector.

For M32-editable selections: - Spawn - Camera - Ground - Platform 0..5

show the existing editable controls.

For newly selectable but not yet editable types: - show useful read-only
authored properties; - clearly label them read-only for M33.

Do not expand authoring scope merely because an object is selectable.

### Editor camera contract

Phase A must inspect the current camera/render/input abstractions before
choosing controls.

Preferred desktop editor navigation: - RMB held: mouse-look/orbit-style
free look; - WASD: horizontal/free movement; - Q/E or another simple
pair: vertical movement; - mouse wheel: speed adjustment or dolly if
useful; - optional Shift: faster movement.

Exact controls may be refined after repository inspection.

Requirements: - editor camera is not serialized into Level Format v1; -
editor navigation never changes authored gameplay camera framing; -
entering editor initializes the editor camera from a sensible current
view; - leaving editor returns to the gameplay camera without corrupting
its follow state; - no cursor trapping that makes normal window
interaction impossible.

### Picking contract

Phase A must determine the smallest correct picking strategy.

Preferred conceptual flow:

``` text
mouse screen position
→ ray through active editor camera
→ intersect authored selectable geometry/proxies
→ nearest valid hit
→ EditorSelection
```

Picking must account for the actual transforms of selectable objects.

For objects without obvious box geometry
(spawn/checkpoint/collectible/goal/camera), use focused editor-only
selection proxies/bounds consistent with their existing debug/render
representation.

Do not use physics BodyIDs as persistent authoring identity.

### UI capture contract

World navigation/picking must not fire while the mouse is interacting
with Dear ImGui.

Respect ImGui mouse/keyboard capture at the UI boundary.

Examples: - clicking an InputFloat must not select an object behind the
panel; - scrolling an ImGui panel must not zoom/change editor camera; -
typing in an editor field must not move the editor camera.

### M32 preservation

Preserve: - working copy; - Modified; - Dirty; - Apply Preview; - Revert
Working Copy; - Save Level Source; - deterministic Level Format v1
writer; - safe source replacement; - Development-only authoring; - Debug
no source write; - Release no editor; - BEST separation; - paused
simulation.

Selection should normally refer to the editor working copy while
editing.

Apply Preview must preserve a valid selection when the corresponding
authored object still exists.

### Explicitly out of scope

-   object creation;
-   object deletion;
-   duplicate object;
-   transform gizmos;
-   drag-to-move objects;
-   rotation gizmo;
-   scale gizmo;
-   undo/redo;
-   copy/paste;
-   multi-select;
-   box selection;
-   scene graph;
-   parent/child hierarchy;
-   generic entity system;
-   ECS;
-   reflection;
-   generic Inspector;
-   generic serialization;
-   Level 02;
-   New Level/Open Level/Save As;
-   level browser;
-   campaign/transitions;
-   enemies/bosses;
-   asset browser;
-   terrain/material/audio/animation editors;
-   runtime editor in Release;
-   Web/mobile/Pi editor.

### Phase A --- Inspection & selection/navigation architecture

Phase A must: 1. inspect M32 editor, camera, renderer, input, DebugUi
and LevelDefinition; 2. define exact editor camera ownership and
controls; 3. define cursor/mouse capture behavior; 4. define a focused
`EditorSelection` representation; 5. define the hierarchy item model
without generic scene/entity infrastructure; 6. inventory selectable
LevelDefinition objects and their picking proxies; 7. design screen-ray
construction and nearest-hit selection; 8. design selection
visualization; 9. define how selection maps into the existing M32
Inspector; 10. add focused math/picking tests where practical; 11. add
scaffolding only as appropriate; 12. not enable the full live visual
workflow until Phase B.

**Phase A outcome (complete):**

Camera. `editor::EditorCamera` is session tooling on `LevelEditorState`, not
`PlatformerCamera` and not `LevelDefinition.camera`. `render::CameraView` is
the project-owned view Renderer will consume in Phase B. First F2 seeds from
gameplay target+offset; later toggles keep the pose. Apply does not move the
editor camera. Exit will SnapToTarget the gameplay camera. Controls (not live):
RMB look, WASD, Q/E, Shift, wheel speed. Cursor stays visible.

Selection. `EditorSelection { EditorObjectKind, index }` is the only current
selection. Validated by `IsValidSelection`. Display names match the 21-entry
`kHierarchyEntries` table. Resolves against `workingCopy`. Apply/Revert/Save
keep identity. Selecting/navigating/picking do not mark Modified or Dirty.

Picking. Project-owned `Ray3`, slab AABB, inverse-rotated slope AABB,
`ScreenToWorldRay`, `PickNearest` (nearest positive t; exact ties keep earlier
hierarchy proxy). Moving platform and cyan box proxies take runtime pose via
`EditorPickingWorldState`. Camera is hierarchy-only. Runtime Player is not
selectable. Spawn uses `kPlayerVisualSize` (marker in Phase B). Empty click
clears. LMB picks, RMB looks. No Jolt raycasts.

Input. `editor::EditorInputState` polled by `PollEditorInput()` in the input
backend. `InputState` is unchanged. `DebugUi::WantsMouseCapture()` added
beside keyboard capture. `Window::Width/Height` expose the resizable viewport.

Not live: Application does not consume editor input, does not swap Renderer to
`CameraView`, does not world-pick, does not draw Hierarchy/highlight, and does
not route the Inspector by selection. `selectedPlatformIndex` remains the M32
geometry combo.

Tests. `EditorPickingTest` covers AABB (front/miss/inside/parallel/behind/
negative-dir), rotated slope hit/miss, nearer-candidate wins, hierarchy-order
ties, runtime-pose vs authored startX, Camera/Player exclusion, screen-to-world
center/right rays, seed-from-gameplay without mutating authored camera, and
selection validity/names/equality.

### Phase B --- Live visual editor

Implemented: editor navigation camera; Hierarchy; world picking; synchronized
selection; selection visualization; Inspector routing by selection; M32
editing for Spawn/Camera/Ground/Platforms; read-only inspection for other
selectable Level Format v1 objects; ImGui input capture; configuration
boundaries.

**Phase B outcome:** Development F2 opens Hierarchy + Inspector + Level Editor,
seeds the editor camera once per process, and renders from `CameraView`. RMB /
WASD / Q/E / Shift / wheel navigate without writing `LevelDefinition.camera`.
LMB world-picks via `ScreenToWorldRay` + `BuildPickingSet(active/applied
level, runtime poses)` + `PickNearest`. Unapplied working-copy transforms do
not move pick, highlight, or the spawn marker. Empty click clears. Camera is
hierarchy-only. Runtime Player is not selectable. Spawn uses a
Debug/Development `kPlayerVisualSize` marker matching the pick proxy.
Highlight uses `MakeHighlightRequest` from the same proxies.
`selectedPlatformIndex` is retired. Apply/Revert/Save preserve selection and
editor camera. Reopening F2 still resets `workingCopy = active`. Debug has
the visual editor but cannot save source. Release has none of this.

M33 remains incomplete until Phase C.

### Phase C --- Manual validation

Validate at minimum:

- F2 editor pause still works
- unapplied Inspector edits do not move pick/highlight/spawn marker
- clicking the visible (applied) location still selects; the unapplied location does not
- Apply then moves render, physics, pick and highlight together
- editor camera navigation (RMB, WASD, Q/E, Shift, wheel)
- editor camera session persistence across F2 close/reopen
- gameplay camera resume / SnapToTarget on exit
- Hierarchy content (21 authored selections)
- hierarchy selection (Ground, Platform 0, Platform 5, Slope 0, Hazard 0, Collectible 2, Goal, Camera)
- world picking (Ground, elevated platform, hazard, collectible, goal)
- nearest hit when proxies overlap
- empty-space click clears selection
- slope oriented picking
- moving platform runtime pick (frozen pose, not authored start)
- cyan box runtime pick
- spawn marker / pick
- Camera hierarchy-only (no world proxy)
- selection highlight follows the pick proxy
- hierarchy / world / Inspector sync
- UI mouse capture (click/scroll/type over panels)
- keyboard capture (InputFloat does not move camera; F2 ignored while typing)
- window resize picking alignment
- editable Inspector fields (Spawn, Camera, Ground, Platform)
- read-only Inspector fields (slope, moving platform, checkpoint, hazard, collectible, goal, dynamic box)
- Modified / Dirty semantics
- Apply preserves selection and editor camera
- Revert preserves selection and restores Inspector values
- Save preserves selection and editor camera
- BEST preservation
- Debug cannot save source
- Release has no editor
- unrelated-CWD Release/Development
- canonical source integrity (`game/assets/source/levels/level_01.level`)

### Acceptance criteria

M33 is complete only when: 1. editor mode provides useful independent 3D
navigation; 2. a visible hierarchy represents the authored objects of
Level 01; 3. one explicit editor selection state synchronizes hierarchy,
world and Inspector; 4. supported objects can be selected by clicking
them in the world; 5. nearest valid hit is selected; 6. selected objects
have a clear editor-only world highlight; 7. ImGui interaction cannot
accidentally navigate/select the world behind it; 8. M32 editable object
types remain editable through the selected-object Inspector; 9. other
M33 object types are inspectable read-only without expanding scope; 10.
Apply/Revert/Save semantics from M32 remain correct; 11. gameplay
simulation remains paused during editing; 12. gameplay camera authoring
is not modified by editor navigation; 13. Debug remains non-authoring;
14. Release remains editor-free; 15. Level Format v1 remains unchanged;
16. no generic scene/entity/reflection framework is introduced; 17. all
automated tests/builds pass; 18. Phase C manual validation passes; 19.
no commit/push/merge occurs before approval.

### Future direction

M33 establishes the selection and viewport foundation needed for later
milestones such as: - transform gizmos; - add/remove/duplicate
objects; - dynamic LevelDefinition collections; - New/Open/Save As; -
multiple levels/scenes; - richer gameplay object authoring; - enemy and
boss spawn authoring.

Those features are intentionally deferred until the M33
selection/navigation foundation is proven stable.
