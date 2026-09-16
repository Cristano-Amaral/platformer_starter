## Milestone 58.2 — Model-backed Selection & Transform Preview

### Status
Implemented, awaiting manual acceptance. Milestone 58.1 is CLOSED. Do not start Milestone 59.

### Branch
`milestone/58.2-model-selection-transform-preview`

### Cursor Model
**Grok 4.6 High — Fast OFF**

### Goal
Improve Development Editor selection and transform feedback for authored objects backed by imported static GLB models.

Supported:
- Static Props
- Item Pickups with assigned GLB models

Target experience:
**Select model-backed object → clearly see the selected model itself highlighted/ghosted → manipulate Translate/Scale/Rotate → feedback follows the model transform directly**

This addresses the current generic selection box being visually disconnected from the model, poorly centered, or hidden behind geometry.

### Preserve Existing Architecture
Inspect and reuse current authorities for M49 Static Prop transform/render/picking, M58 Item Pickup visual transform/render/picking, M58.1 Rotate gizmo, Translate/Resize/Scale/Rotate modes, EditorPicking, EditorGizmo, StaticModelSceneStore, staged runtime model authority, workingCopy/active/savedSourceBaseline, Hierarchy/Inspector, Quick Toolbar, editor camera/input arbitration, Development overlays, renderer/rlgl state restoration, Level Format v1, and current tests.

Do not replace stable systems.

### Core UX Requirement
The box proxy may remain useful internally, but it must no longer be the primary designer-facing selection/transform feedback for model-backed objects.

Selection and transform feedback should visually follow the actual transformed model.

### Selected Model Ghost / Highlight
When a supported model-backed object is selected in the Development Editor, render an editor-only second visual pass of the same model as a selection ghost/highlight.

Requirements:
- reuse the same model resource;
- use the same current workingCopy transform;
- no duplicate authored transform authority;
- no persistent runtime object;
- no asset mutation;
- Development editor only.

Conceptual authority:

`workingCopy transform -> normal model + selection ghost + gizmo`

The ghost is visual feedback only.

### Ghost Appearance
Use a simple readable editor-only treatment compatible with the current renderer.

Preferred:
- clear tint/highlight;
- semi-transparent or emissive-like overlay if supported narrowly;
- readable on dark materials;
- clearly visible during Translate/Scale/Rotate.

Do not require a new full post-processing pipeline. Avoid a complex silhouette/stencil framework unless current renderer support makes it trivial.

Prefer a safe second model pass.

### Depth / Occlusion
Selection feedback should remain useful even when parts of the model are occluded.

Preferred:
- normal depth-respecting ghost/highlight;
- optionally a subtle x-ray/through-geometry second pass if current rlgl state handling supports it safely.

If that would be too invasive, implement the narrowest robust single-pass improvement.

All modified render state must be restored.

### Transform Preview
During Translate, Scale and Rotate, the selected model highlight must update immediately from workingCopy.

No lagging box-only preview.
No separate preview transform.
No delayed commit-only visualization.

### Static Prop
Ghost/highlight uses existing authored:
- position
- rotation
- scale

Translate/Scale/Rotate must reflect immediately.

Do not change Static Prop schema or Level Format.

### Item Pickup
For model-backed Item Pickup, ghost/highlight uses:
- `position + visualOffset`
- `visualRotationDegrees`
- `visualScale`

Translate edits gameplay `position`.
Scale edits `visualScale`.
Rotate edits `visualRotationDegrees`.

Ghost must follow all three.

Gameplay targeting remains based on logical `position`.

### Selection Bounds Visualization
For model-backed Static Props and Item Pickups, remove or visually demote the generic box proxy as the primary selection indicator.

A bounds wireframe may remain as a secondary diagnostic if useful, but:
- it must follow the actual transformed bounds;
- it must not imply the box itself is the object;
- it must not dominate or obscure the model highlight.

Prefer transformed oriented bounds where narrow existing math allows it.

### Model-space Bounds
Reuse trustworthy model-local bounds if already available in the cache.

If current paths rely on normalized local ±0.5 bounds, preserve compatibility but transform all 8 corners through the actual authored transform before visualization.

No per-frame triangle scanning.
No GLB reimport.

### Editor Picking
Improve model-backed picking coherence with the visible transformed model.

Preferred priority:
1. existing transformed model-local bounds if reliable;
2. transformed oriented proxy from current local bounds;
3. avoid disconnected world-axis AABB when better transformed proxy is already available.

Exact triangle picking is out of scope unless already essentially available.

Picking must remain deterministic.

### Gizmo Origin
Do not change mathematical gizmo origin.

Static Prop: existing origin.
Item Pickup: `position + visualOffset`.

Ghost does not redefine pivot.

### Selection State
Only the currently selected supported object receives ghost/highlight.

No multi-selection.
No persistent material mutation.
Selection change immediately moves highlight to the new selected object.

### Missing / Fallback Models
Static Prop missing staged model: preserve current safe placeholder/missing behavior.
Item Pickup missing model: preserve M58 placeholder.
Item Pickup with empty modelIdentity: retain current fallback cube selection behavior.

Do not fabricate model data.

### Editor-only Scope
Ghost/highlight affects only Development editor visualization according to current F2/editor authority.

Do not add runtime gameplay selection highlighting.
Release must not gain editor-only rendering behavior.

### Render-state Safety
Any ghost/highlight pass that changes depth, blend, culling, material/shader state, line width, or rlgl matrices must restore the expected state before subsequent drawing.

This is mandatory.

### Performance
Reuse cached GLB resources.
At most an extra draw for the selected model-backed object.
No per-frame filesystem I/O.
No source fallback.
No dynamic mesh duplication.

### No Authored Data Change
M58.2 adds no serialized fields and no Level Format changes.

If implementation appears to require authored schema changes, stop and reassess.

### Explicitly Out of Scope
No full post-process outline framework, generic editor selection renderer for every object, multi-selection, group transform, pivot editing, Local/World toggle, snapping, undo/redo, triangle mesh picking framework, asset import metadata, GLB pivot rewrite, material/shader editor, generic Transform/Render component, ECS, prefabs, parenting, Item Pickup gameplay targeting by visual mesh, runtime gameplay selection, Level Format v2, or M59 functionality.

### Focused Tests
Cover:
1. selected Static Prop gets model-based highlight;
2. unselected Static Prop does not;
3. selected model-backed Item Pickup gets highlight;
4. unselected Item Pickup does not;
5. fallback Item Pickup remains safe;
6. missing model remains safe;
7. highlight reuses current model resource;
8. Static Prop Translate updates highlight live;
9. Static Prop Scale updates live;
10. Static Prop Rotate updates live;
11. Item Pickup Translate updates live;
12. Item Pickup Scale updates live;
13. Item Pickup Rotate updates live;
14. Visual Offset reflected in highlight origin;
15. no second transform authority;
16. transformed Static Prop remains pickable;
17. transformed Item Pickup remains pickable;
18. visible feedback aligns with transformed model;
19. retained bounds diagnostic follows transformed corners if retained;
20. generic disconnected box no longer primary cue;
21. Item Pickup gameplay targeting still uses logical `position`;
22. collection unchanged;
23. Door required-item behavior unchanged;
24. Pressure Plate behavior unchanged;
25. no Level Format change;
26. no authored schema change;
27. matrix state restored;
28. depth/blend/cull state restored as applicable;
29. later rendering unaffected;
30. no persistent material mutation;
31. Translate regression;
32. Resize regression;
33. Scale regression;
34. Rotate regression;
35. Static Prop render/picking regression;
36. Item Pickup render/picking regression;
37. M55 targeting regression;
38. M57/M57.1 Door regression;
39. Inventory/UI regression;
40. canonical cleanup;
41. no generic selection/postprocess/transform framework.

Do not expose public production APIs solely for tests.

### Manual Acceptance
Use disposable Development fixtures.

#### Static Prop
- Add an irregularly shaped GLB as Static Prop.
- Select it.
- Confirm the model itself receives clear selection feedback.
- Confirm selection is no longer communicated primarily by a disconnected cube.
- Translate/Scale/Rotate it and confirm highlight follows live.

#### Item Pickup
- Add Item Pickup with GLB.
- Set non-zero Visual Offset, non-uniform Scale and non-zero Rotation.
- Select it.
- Confirm highlight is aligned with the transformed model.
- Translate/Scale/Rotate and confirm feedback follows live.

#### Occlusion
- Place part of selected object behind other geometry.
- Confirm selection remains readable enough to identify the selected model.
- Confirm no depth/render corruption elsewhere.

#### Picking
- Rotate/scale model-backed object.
- Click its visible transformed region.
- Confirm reliable selection.
- Confirm clicking away follows existing selection rules.

#### Gameplay isolation
- Apply/run.
- Confirm Item Pickup targeting still uses logical gameplay position.
- Collect it.
- Confirm Inventory receives correct item.
- Confirm Door/Pressure Plate behavior remains unchanged.

#### Missing/fallback
- Verify Item Pickup without model stays safe.
- Verify existing missing-staged-model placeholder stays safe.

#### Lifecycle / format
- Revert/Apply/Save as appropriate.
- Confirm no authored fields or Level Format changes were introduced.

Remove all disposable fixtures before closure.

### Validation
Run relevant C++ tests including M58.2 plus regressions for Static Prop rendering, Item Pickup rendering, EditorPicking, EditorGizmo, Translate/Resize/Scale/Rotate, authored lifecycle, renderer state restoration, M49, M55-M58.1, and canonical cleanup.

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

### Canonical Data Safety
Before closure, Level 01 remains fixture-free:
- Item Pickups: 0
- Doors: 0
- Pressure Plates: 0
- Dynamic Boxes: 0
- Static Props: 0

Keep absent:
`dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

### Completion Criteria
Ready for manual acceptance when:
- selected model-backed Static Props use model-based ghost/highlight feedback;
- selected model-backed Item Pickups use model-based ghost/highlight feedback;
- feedback follows workingCopy Translate/Scale/Rotate live;
- Item Pickup Visual Offset/Rotation/Scale are reflected correctly;
- disconnected generic box is no longer the primary visual selection cue for model-backed objects;
- model-backed picking is coherent with transformed visual bounds;
- render state is restored safely;
- no authored schema or Level Format changes occur;
- gameplay targeting/collection remains unchanged;
- Debug/Development/Release builds and tests pass;
- canonical Level 01 is clean;
- no generic postprocess/selection/ECS/mesh-picking framework or M59 functionality is introduced.

### STOP
After implementation, validation and report: STOP.

Do not commit, push, merge, start M59, or declare M58.2 CLOSED. Wait for user manual acceptance and separate Git closure.
