## Milestone 58.1 — Editor Rotate Gizmo v1

### Status
Implemented — awaiting manual acceptance.

Do not mark CLOSED until user manual acceptance and the separate Git closure workflow.

Milestone 58 visual-transform data is the closed prerequisite. This milestone does not change Level Format.

### Branch
`milestone/58.1-editor-rotate-gizmo`

### Cursor Model
**Grok 4.6 High — Fast OFF**

### Goal
Add a focused Development Editor Rotate Gizmo v1 for authored objects that already have meaningful rotation data:
- Static Props: edit their existing authored rotation.
- Item Pickups: edit M58 `visualRotationDegrees`.

Complete the practical viewport workflow without introducing a generic Transform component, ECS, physics rotation authoring, or arbitrary rotation for other authored types.

### Existing authorities to preserve
Inspect the repository first. Reuse the current `EditorGizmo`, central transform-mode state, Quick Toolbar, picking/selection arbitration, editor camera/input ownership, M49 Static Prop authored rotation/Scale gizmo, M58 Item Pickup visual transform, workingCopy/active/savedSourceBaseline, Dirty/Modified, Apply/Revert/Save, renderer, Level Format v1, and current tests.

Do not rewrite Translate/Resize/Scale unless a minimal shared helper is clearly justified.

### Supported objects
Rotate supports only:
1. Static Prop — edits its existing authored rotation.
2. Item Pickup — edits `visualRotationDegrees`.

For Item Pickup, never alter gameplay `position`, `visualOffset`, or `visualScale` while rotating.

Unsupported: Ground, Platforms, Slopes, Checkpoints, Hazards, Collectibles, Dynamic Boxes, Pressure Plates, Doors, moving platform, Player, runtime physics bodies. Rotate mode on unsupported selection must be safely inert.

### Transform mode and toolbar
Extend the existing central transform mode with `Rotate`. Integrate it into the existing Quick Toolbar and active-mode presentation. Do not create a second rotate-only mode authority.

Keyboard shortcut is optional. Add one only if the repository already has a safe transform-shortcut convention; otherwise omit it and report the decision.

### Gizmo visual
Render conventional X/Y/Z rotation rings at the selected object's gizmo origin. Reuse existing axis color conventions. Provide hover and active feedback consistent with existing gizmos. Rings must be distinguishable and usable from normal oblique editor camera angles.

No free rotate, trackball, screen-space rotate, local/world toggle, quaternion UI, or snapping system.

### Rotation space
Use world-axis rings:
- X ring changes authored X Euler degrees.
- Y ring changes authored Y Euler degrees.
- Z ring changes authored Z Euler degrees.

Inspector remains the exact numeric authority/view of the same workingCopy values. Preserve current Euler-degree representation; do not migrate to quaternions or invent a new normalization policy.

### Gizmo origin
Static Prop: use the same authored/model transform origin already used by its current transform path.

Item Pickup: use `position + visualOffset`, matching M58 visual model origin.

### Drag interaction
Implement stable one-axis-at-a-time click-drag:
1. hover ring;
2. mouse-down captures axis;
3. establish rotation plane from axis;
4. intersect mouse ray with plane;
5. capture initial radial vector;
6. compare current radial vector to initial;
7. compute signed angular delta around axis;
8. update only corresponding authored Euler component;
9. mouse-up ends drag.

Prefer robust world-space ray/plane geometry over raw horizontal mouse-pixel deltas if practical with current architecture.

Handle degeneracy safely: no NaN/Inf, no initial jump, no mutation if valid intersection cannot be established, mouse release clears drag, selection/mode change cancels drag.

### Static Prop integration
Rotate edits the same authored rotation already edited by Inspector. Rendering updates immediately from workingCopy. Picking remains useful. Translate and Scale remain unchanged. Do not create a second visual rotation field.

### Item Pickup integration
Rotate edits only `visualRotationDegrees`. Rendering updates immediately. Inspector stays synchronized. M58 transformed editor picking remains useful. Scale continues editing `visualScale`; Translate continues editing gameplay `position`.

Critical invariant: M55 gameplay targeting remains based on logical `position`; viewport rotation must not affect range/facing/LOS/collection semantics.

### Dirty and lifecycle
Rotate drag edits workingCopy only and participates in existing semantic equality/Dirty authority. Revert restores; Apply promotes; Save persists. Do not directly mutate active before Apply, savedSourceBaseline before Save, or runtime-only state.

### Picking, selection and camera
When Rotate is active, ring hit-testing receives the same appropriate priority over world selection as current gizmos. Dragging a ring must not select objects behind it. Active rotation drag must suppress conflicting camera orbit/pan; camera behavior resumes after release. Reuse current input ownership conventions.

### Inspector synchronization
Inspector and gizmo are two views of the same authored values:
- Inspector edit updates gizmo/model.
- Gizmo edit updates Inspector/model.
- Revert updates both.
- Apply/Save preserve both.
No cached second rotation authority.

### Level Format
No Level Format change. Static Props already serialize rotation; M58 Item Pickups already serialize `visualRotationDegrees`. If a format change seems necessary, stop and reassess rather than expanding scope.

### Runtime/physics isolation
Development editor authoring only. No new Jolt bodies and no changes to Dynamic Boxes, Doors, Pressure Plates, Player physics, Inventory, pickup runtime collection, Door required-item semantics, or Release gameplay behavior.

### Explicitly out of scope
No Rotate for primitives/Dynamic Boxes/Doors/Pressure Plates/Player; no physics-body rotation authoring; no local/world toggle; no local-space rings; no free/trackball/screen-space rotation; no quaternion Inspector/migration; no rotation snapping/preferences; no pivot editing/modes; no parenting/hierarchical transforms; no multi-selection/group rotation; no undo/redo; no generic Transform component; no ECS; no prefab transform system; no animation/keyframes; no asset pivot rewrite; no Level Format v2; no M59 functionality.

### Focused tests
Cover:
- Rotate in central transform mode and Quick Toolbar.
- Static Prop and Item Pickup applicability; unsupported types inert.
- X/Y/Z ring changes only matching authored component.
- Static Prop render/Inspector/picking synchronization.
- Item Pickup edits only `visualRotationDegrees`; position/offset/scale unchanged.
- Item Pickup render follows rotation and gameplay targeting remains based on `position`.
- Translate/Resize/Scale unchanged.
- mouse-down captures one axis; mouse-up ends; mode/selection change cancels.
- degenerate geometry cannot write NaN/Inf or cause initial jump.
- ring interaction does not select behind or manipulate camera simultaneously.
- workingCopy/Dirty/Revert/Apply/Save semantics.
- existing Level Format output unchanged.
- regressions for Static Props, M58 Item Pickups, M55 gameplay, M57/M57.1 Doors, Pressure Plates, Inventory/UI, editor picking/rendering and canonical cleanup.
Do not expose public production APIs solely for tests.

### Manual acceptance
Use disposable Development fixtures.

#### Static Prop
Add/assign a Static Prop model. Select Rotate. Confirm X/Y/Z rings. Drag each axis and verify only the corresponding Inspector rotation changes and the model rotates live. Switch among Translate, Scale and Rotate and verify coherence.

#### Item Pickup
Add an Item Pickup with GLB and non-zero Visual Offset. Select Rotate and confirm origin is `position + visualOffset`. Rotate X/Y/Z and verify Inspector Visual Rotation changes while Position, Visual Offset and Visual Scale remain unchanged. Scale must still edit Visual Scale; Translate must still edit gameplay Position.

#### Gameplay invariance
Apply/run. Confirm Item Pickup rotation does not change targeting position/range/facing/LOS. Collect successfully and verify correct Inventory item/quantity; optionally verify a Door requiring that item still unlocks.

#### Lifecycle
Rotate Static Prop and Item Pickup. Revert and verify restoration. Rotate again, Apply, Save/reload and verify persistence with no new Level Format migration.

#### Robustness
Test several camera angles. Verify no initial jump, camera conflict, behind-object selection, or unstable state after mode/selection changes. Select unsupported objects in Rotate mode and confirm no mutation.

Remove all disposable fixtures before closure.

### Validation
Run relevant C++ tests for M58.1, editor gizmos, Static Props, Item Pickups, authored lifecycle, picking, renderer, M51–M58, and canonical cleanup.

Run:
```
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Build:
```
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check`.

### Canonical data safety
Level 01 must remain fixture-free at closure:
- Item Pickups: 0
- Doors: 0
- Pressure Plates: 0
- Dynamic Boxes: 0
- Static Props: 0

Keep absent:
`dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

### Completion criteria
Ready for manual acceptance when Rotate is a real Editor transform mode; Static Props and Item Pickup visual rotation work on X/Y/Z; Item Pickup gameplay position is untouched; gizmo origins are correct; drag is stable and isolated from camera/selection; Inspector/lifecycle stay synchronized; Translate/Resize/Scale regressions are green; Level Format is unchanged; all builds/tests pass; canonical Level 01 is clean; and no generic Transform/ECS/local-space/snapping/undo system was introduced.

### STOP
After implementation, validation and report: STOP.

Do not commit, push, merge, start M59, or declare M58.1 CLOSED. Wait for user manual acceptance and separate Git closure.
