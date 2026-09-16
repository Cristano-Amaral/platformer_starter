## Milestone 35 — Visual Level Editor v4: Scale/Resize Gizmo + Orientation Widget + Precision Navigation

### Status
Phase B implemented; Phase C functional validation passed. One placement
correction: orientation widget moves to the upper-right of the 3D viewport
(left of the default Inspector column). Awaiting final approval.
M35 is **not** complete. Milestone 36 has not started.

### Branch
`milestone/35-scale-gizmo-orientation-navigation`

## Recommended Cursor model
**Grok 4.6 High — Fast OFF**

### Goal
Build on M34 without changing the authored-level architecture. M35 adds three focused editor-quality improvements:

1. **Visual resize/scale handles** for the existing box-shaped editable objects.
2. **Persistent top-left orientation widget** showing world X/Y/Z and allowing canonical editor-camera views.
3. **Precision editor navigation**, including selected-object keyboard nudge and modifier + mouse-wheel camera dolly.

M35 does **not** add/remove/duplicate objects and does not change Level Format v1.

### Architectural contract
The M32–M34 split remains mandatory:

- Inspector edits `workingCopy`.
- Translate and resize gizmos edit `workingCopy`.
- Pending preview represents `workingCopy`.
- Normal render, physics, normal world picking and M33 highlight use the active/applied `LevelDefinition`.
- `Apply Preview` commits working → active and rebuilds physics.
- `Save Level Source` writes active/applied data only.
- Selection, editor camera, gizmo mode and UI state are editor-only and never serialized into `LevelDefinition`.

### Part A — Resize/Scale Gizmo

#### Scope
Visual resize is available only for box geometry whose size is already authored/editable:

- Ground
- Elevated Platform 0..5

Player Spawn remains **translation-only** in M35. Camera and M33 read-only categories receive no resize handles.

#### Behavior
Add an explicit editor transform mode:

- Translate
- Resize

A compact visible UI control is required. Optional editor-only shortcuts may be added only if they do not conflict with camera/game controls.

Resize handles operate in world X/Y/Z. M35 edits authored **box size**, not a generic Transform scale. Do not introduce scale matrices or parent transforms.

Dragging an axis handle changes only the corresponding size component. The default resize policy should preserve the box center and expand/contract symmetrically around it. This keeps M35 small and avoids introducing face-anchor/pivot semantics prematurely.

All size components must remain finite and strictly positive. Define a small project-owned minimum dimension and clamp safely; no negative or zero-sized boxes.

The pending ghost must immediately reflect working center + working size. Active render/highlight/picking/physics remain unchanged until Apply.

#### Resize tests
Add focused tests for:

- Ground/Platform supported; Spawn/read-only unsupported.
- X resize changes only size.x.
- Y resize changes only size.y.
- Z resize changes only size.z.
- center remains unchanged.
- minimum-size clamp.
- no NaN/inf.
- representative camera angles.
- near-parallel stability.
- active-vs-working preview semantics.

### Part B — Orientation / View Widget

Add a small **screen-space orientation triad** in the upper-left area of the 3D viewport, positioned so it does not collide with the existing TIME/BEST HUD or editor windows.

It must:

- display X=red, Y=green, Z=blue consistently with the world gizmo;
- rotate/orient according to the editor camera;
- be editor-only;
- not be authored level data;
- remain visible regardless of world geometry/depth;
- remain below ImGui windows.

Make the widget clickable for canonical views if the implementation stays focused and robust:

- +X / Right
- -X / Left
- +Y / Top
- -Y / Bottom (optional if awkward for current platformer workflow)
- +Z / Front
- -Z / Back

Clicking a view direction changes **editor camera orientation only**. Preserve editor-camera position unless a focused orbit-distance policy is needed; do not alter gameplay camera authoring.

If clickable faces/axes become disproportionate in scope, Phase A must report that and propose the smallest subset before enabling it live.

### Part C — Selected-object keyboard nudge

Add precision translation for the currently selected object that already supports the translation gizmo:

- Player Spawn
- Ground
- Elevated Platform 0..5

Do not reuse bare WASD/QE because those belong to editor-camera movement.

Preferred policy: require a modifier plus directional keys, chosen after inspecting existing bindings and ImGui capture. The controls must be visible/documented in the editor UI.

Nudge edits `workingCopy` only and follows the same Modified/Dirty/Apply/Revert semantics as the translate gizmo.

Provide a normal increment and, if clean, a smaller precision increment with an additional modifier. No grid/snapping system is introduced.

Keyboard nudge must respect current-frame ImGui keyboard capture: typing in `InputFloat` must never move an object.

### Part D — Modifier + mouse-wheel camera dolly

M34 wheel behavior changes editor navigation speed. Preserve that behavior for unmodified wheel input.

Add a modifier + wheel gesture for **editor-camera dolly** along its forward vector:

- ordinary wheel → adjust editor navigation speed (M34 behavior)
- modifier + wheel → move editor camera forward/backward

Choose the modifier after checking conflicts. Dolly must not mutate gameplay camera authoring or `workingCopy`.

ImGui mouse capture must block both speed-wheel and dolly-wheel when the pointer is over UI.

Clamp/guard movement to avoid non-finite camera state or extreme single-step jumps.

### Hierarchy inventory
M34 identified four visible cooker probes outside the 21-entry Hierarchy:

- `textures/test_checker.png` quad
- `models/test_static.glb`
- `models/test_authored.glb`
- `models/test_textured.glb`

They remain technical/demo/render probes in M35. Do not add them to the authored Hierarchy and do not make them editable.

Player runtime, grid, editor overlays and HUD also remain outside authored Hierarchy for their existing reasons.

### Renderer / input boundaries
Renderer receives read-only draw requests. It does not own selection, transform mode, working data or interaction state.

Editor interaction priority remains:

1. current-frame ImGui capture;
2. active transform interaction / orientation widget;
3. normal world picking;
4. camera navigation where applicable.

A single click/gesture must not trigger multiple systems.

### Layout persistence
M34 persistent window layout remains unchanged. M35 must not regress:

- `%LOCALAPPDATA%\\Platformer3D\\editor_layout.ini`
- Metrics / Hierarchy / Inspector / Level Editor persistence
- Reset Editor Layout
- off-screen recovery
- Debug/Development sharing
- Release isolation
- unrelated-CWD behavior

The orientation widget itself is viewport overlay state and does not need its own persisted position in M35.

### Configurations

#### Debug
- visual editor
- translate + resize tools
- orientation widget
- precision navigation
- persistent editor layout
- source authoring unavailable

#### Development
Same as Debug plus existing source authoring.

#### Release
No ImGui editor, transform tools, orientation widget, editor layout interaction or editor navigation controls.

### Explicitly out of scope

- object add/delete/duplicate
- dynamic authored collections
- stable authored object IDs
- Level 02
- New/Open/Save As
- scene browser
- rotation gizmo
- arbitrary/local Transform scale
- local/world transform toggle
- pivot editing
- face-anchored resize
- grid snapping
- generic snapping system
- undo/redo
- copy/paste
- multiselect
- box select
- docking/workspaces
- scene graph / ECS / reflection
- generic Inspector/serialization
- adding cooker probes to Hierarchy
- gameplay/campaign/enemies/bosses/combat
- Level Format v2
- save v2
- Web/mobile/Raspberry Pi editor work

### Phase plan

#### Phase A — architecture/math/scaffolding

Inspect M34 live implementation; define resize math/state, transform-mode ownership, orientation-widget math/hit testing, nudge bindings and dolly behavior. Add focused no-window tests. Do not enable the complete live feature set yet.

**Phase A outcome (complete):**

Resize. `EditorTransformMode` on `LevelEditorState` (default Translate, unused by Application). `GetEditableSize` / `IsResizeSelection` for Ground and Platform 0..5. `GizmoInteractionState` stores `dragStartSize` and `dragHandleSign`. Handles at `± gizmoLength` cubes. `newSize = start + 2 * sign * axisDelta`, clamp `kMinAuthoredBoxExtent = 0.12`. M34 translation solver unchanged. `UpdateResizeInteraction` exists for tests; Application still calls only `UpdateGizmoInteraction`.

Orientation. `ProjectOrientationWidgetAxes`, layout `(364, 28)`, canonical Front/Back/Right/Left/Top. Position-preserving yaw/pitch. 2D tip hit test. Not drawn.

Nudge. Ctrl+arrows/PageUp/PageDown, Ctrl+Shift precision 0.01 vs 0.10. Polled into `EditorInputState`, not applied.

Dolly. `ApplyEditorCameraDolly`; Alt+wheel fills `dollyWheelDelta`. `UpdateEditorCamera` still uses ordinary wheel for speed.

Tests: `EditorGizmoTest` resize/nudge cases; new `EditorOrientationTest`.

#### Phase B — live integration

**Phase B outcome (implemented, awaiting Phase C):**

Translate/Resize radios in Level Editor (locked during drag). Default Translate, session-only. `UpdateResizeInteraction` live for Resize; `UpdateGizmoInteraction` for Translate. Six cube handles, depth-independent overlay, ghost is true size. Spawn in Resize: no handles, UI hint. Orientation widget drawn screen-space before ImGui; clickable Front/Back/Left/Right/Top. Nudge live in Translate only (`NudgeAllowed`). `ResolveEditorWheel` makes Alt+wheel exclusive dolly. ImGui capture remains current-frame.

#### Phase C — manual validation
Manually validate resize from multiple camera angles, minimum-size safety, working-vs-active semantics, Apply/Revert/F2, orientation widget and canonical views, nudge/input capture, dolly/speed wheel separation, layout regression, Debug/Release boundaries, unrelated CWD and canonical level-source integrity.

**Phase C correction (placement only):** the orientation widget uses adaptive upper-right layout (`originX = viewportWidth - defaultInspectorColumn - radius - 24`, `originY = 64 + radius`) so it sits left of the default Inspector column, ~100 px from the top on a 1280×720 view. Hit-test, canonical views, and interaction priority are unchanged. Not persisted. Awaiting final approval.

### Acceptance criteria
M35 is complete only when:

- Ground and Platforms can be resized visually on X/Y/Z.
- Resize modifies authored size in `workingCopy`, with center preserved.
- Pending ghost shows resize before Apply.
- Active world/physics/pick/highlight remain applied until Apply.
- Minimum-size and finite-value safety work.
- Translate M34 remains correct.
- Orientation widget is readable and correctly follows editor-camera orientation.
- Approved canonical view clicks work without touching gameplay camera authoring.
- Keyboard nudge works only for translation-editable selections and respects ImGui capture.
- Modifier+wheel dolly and ordinary wheel speed adjustment coexist predictably.
- M34 persistent layout remains correct.
- Debug/Development/Release boundaries remain correct.
- Level Format v1, BEST and canonical source remain unchanged except intentional source authoring after explicit Save.
- All automated tests/builds/cooker regressions pass.
- Phase C manual validation passes.
- No M36 work has started.
