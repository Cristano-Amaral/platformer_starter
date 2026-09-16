## Milestone 58 — Item Pickup Visual Transform & Asset Presentation

### Status

**Implemented — awaiting manual acceptance**

Do not mark CLOSED until user manual acceptance and the separate Git closure workflow.

### Branch

`milestone/58-item-pickup-visual-transform`

### Recommended Cursor Model

**Grok 4.6 High — Fast OFF**

### Goal

Improve the production usability and visual fidelity of M55 Item Pickups that use imported static GLB assets.

M58 adds authored visual transform controls to Item Pickups so a pickup model can be positioned visually, rotated, and scaled appropriately without changing the gameplay pickup location or interaction rules.

Target workflow:

**Select imported model → assign to Item Pickup → visually fit/orient it in the world → keep pickup interaction behavior unchanged**

This milestone addresses the current limitation where an Item Pickup can reference a GLB model but has only one authored world position and no independent visual rotation/scale.

M58 is not an item-definition, asset-metadata, animation, or generic component milestone.

### Preserve Current Architecture

Inspect the current repository first and preserve the actual authorities for:

- M55 `world::ItemPickupSpec`;
- M55 pickup runtime collected state;
- M55 targeting/range/facing/LOS;
- M55 Content Browser model assignment;
- M54 Inventory;
- M56 Inventory UI;
- M57/M57.1 Door item requirements;
- M49 Static Prop model loading/rendering/picking conventions where reusable;
- current staged static model runtime pipeline;
- current missing-model placeholder behavior;
- current Editor workingCopy/active/savedSourceBaseline authority;
- Hierarchy/Inspector/Object Palette;
- current Translate gizmo;
- M49 Static Prop Scale gizmo patterns;
- current Level Format v1 limits;
- Restart/checkpoint/Apply/reload semantics;
- Debug/Development/Release separation.

Do not replace working systems or introduce speculative abstractions.

### Item Pickup Authored Visual Transform

Extend `world::ItemPickupSpec` narrowly with visual-only authored transform data.

Preferred fields:

- `visualOffset`
- `visualRotationDegrees`
- `visualScale`

Conceptual types:

- `visualOffset`: Vector3
- `visualRotationDegrees`: Vector3 Euler XYZ degrees
- `visualScale`: Vector3

Defaults:

- `visualOffset = {0, 0, 0}`
- `visualRotationDegrees = {0, 0, 0}`
- `visualScale = {1, 1, 1}`

The existing `position` remains the authoritative gameplay pickup position.

The visual model transform becomes:

`worldVisualPosition = position + visualOffset`

with authored visual rotation and scale applied to the rendered GLB.

Do not replace `position` with a generic transform object.

### Gameplay Position vs Visual Position

This distinction is critical.

`ItemPickupSpec::position` remains authoritative for:

- pickup targeting;
- pickup interaction distance;
- facing test;
- LOS target point unless current code uses a better stable equivalent;
- Object Palette placement;
- Duplicate +1 X behavior;
- lifecycle/gameplay semantics.

The new visual fields affect only:

- model rendering;
- visual bounds used for editor visualization/picking where appropriate.

Changing visual offset/rotation/scale must not silently move the gameplay interaction point.

No hidden coupling between rendered mesh pivot and gameplay pickup position.

### Level Format v1

Keep Level Format v1.

Current M55 conceptual syntax:

`item_pickup <px> <py> <pz> <quantity> <itemId> [<modelIdentity...>]`

M58 must extend this backward-compatibly.

Because `modelIdentity` is trailing and may contain spaces, do not use an ambiguous token extension that makes parsing unreliable.

Choose the narrowest deterministic representation compatible with the current parser architecture.

Preferred direction:

- keep old M55 records valid with identity-only behavior and default visual transform;
- add an explicit unambiguous marker/segment for visual transform fields if needed;
- writer emits one deterministic canonical representation;
- model identity with spaces must continue to round-trip;
- empty model identity must remain valid;
- no Level Format v2.

Do not casually break the current identity reassembly behavior.

Report the exact final grammar.

### Validation

Use narrow validation:

#### Position
Preserve current M55 finite position validation.

#### Visual Offset
- finite XYZ.

#### Visual Rotation
- finite XYZ;
- do not impose artificial angle clamping if existing Static Prop rotation accepts arbitrary finite degrees.

#### Visual Scale
- finite XYZ;
- every axis strictly positive;
- use the same practical minimum/floor as the existing Static Prop scale path where appropriate.

#### Model Identity
Preserve existing rule:

- empty, or
- valid `StaticPropIdentityIsValid`.

No disk existence requirement during parse/apply/save.

### Authored Lifecycle

The new visual fields participate in:

- workingCopy;
- active;
- savedSourceBaseline;
- authored equality;
- Modified/Dirty;
- Add;
- Duplicate;
- Delete;
- Apply;
- Revert;
- Save.

Add Item Pickup defaults to identity empty and neutral visual transform.

Duplicate preserves:

- itemId;
- quantity;
- modelIdentity;
- visual offset;
- visual rotation;
- visual scale;

while preserving the existing gameplay position duplication rule (+1 X).

Runtime collection must never mutate authored visual transform.

### Inspector

Extend Item Pickup Inspector with a clear separation between gameplay and visual authoring.

Preserve current:

- Position;
- Item ID;
- Quantity;
- Model assignment from selected Content Browser asset;
- Clear Model.

Add a `Visual` section with:

- Offset
- Rotation
- Scale

Use current editor numeric-edit conventions.

Do not add gameplay radius, interaction range, collider size, animation, rarity, icon, or item-definition fields.

### Gizmos

Preserve Item Pickup Translate behavior for the gameplay `position`.

Add narrow visual-scale authoring support only if consistent with the current M49 scale-gizmo architecture.

Preferred behavior:

- normal Translate gizmo edits `ItemPickupSpec::position`;
- Item Pickup may use the existing Scale transform mode to edit `visualScale`;
- visual offset and visual rotation remain Inspector-authored in M58 unless a very narrow existing gizmo path can be reused without introducing a Rotate gizmo framework.

Do not add a generalized Rotate gizmo in M58.

Do not reinterpret primitive Resize as model scale.

If adding pickup Scale gizmo would require broad editor redesign, keep scale Inspector-only and report the limitation.

### Rendering

For an uncollected Item Pickup with a valid staged model identity:

render the real staged model using:

- `position + visualOffset`;
- `visualRotationDegrees`;
- `visualScale`.

Reuse the existing static model resource/cache path where appropriate.

Preserve:

- staged runtime authority;
- no Development source fallback during Gameplay;
- existing placeholder behavior when staged model is missing.

For identity empty, preserve the existing fallback pickup cube.

The fallback cube may remain fixed-size and ignore visual transform except where current architecture makes applying scale trivial and deterministic. Do not turn fallback geometry into a new authored asset system.

### Imported Model Pivot Independence

Imported GLB pivots may differ.

M58 must not attempt to rewrite GLB files, bake pivots, recenter meshes, or introduce asset import metadata.

The designer uses `visualOffset`, `visualRotationDegrees`, and `visualScale` to compensate per pickup instance.

This keeps the milestone local to authored Item Pickup presentation.

### Editor Picking

Item Pickups must remain practical to select after visual transform changes.

Preferred behavior:

- if a real model is assigned, editor picking should use the transformed model/local bounds path where current renderer/model cache makes this narrow and reliable;
- fallback pickup continues using its existing proxy;
- gameplay targeting remains based on M55 gameplay position/rules, not editor mesh bounds.

If exact mesh picking would require a broad new system, use a deterministic transformed bounds proxy derived from existing model bounds.

Do not change Static Prop picking behavior.

### Selection Feedback

Preserve M55 runtime pickup-target feedback.

If target highlight currently renders around the pickup visual, update it so it remains aligned with the transformed visual model when practical.

The prompt/HUD remains:

`E Pick Up <itemId> x<quantity>`

No item display-name metadata.

No new interaction UI framework.

### Content Browser Integration

Preserve existing M55 model assignment workflow.

When a static GLB is selected in Content Browser and assigned to an Item Pickup:

- store only existing canonical `modelIdentity`;
- do not copy source path;
- do not create an Item asset;
- do not auto-import or auto-stage;
- do not auto-create metadata.

Model assignment must not overwrite existing visual transform.

### Runtime Collection Semantics

Preserve M55 exactly:

- one E arbitration;
- Inventory `TryAdd` is atomic authority;
- successful add marks collected;
- failed/overflow add leaves pickup available;
- collected pickup is hidden and untargetable;
- checkpoint respawn preserves collection state + Inventory;
- full Restart restores pickups and clears Inventory;
- Apply/reload resets according to current authority;
- PhysicsWorld rebuild alone preserves pickup run state.

New visual fields do not affect collection state.

### Door / Pressure Plate Isolation

M58 must not change M57/M57.1 gameplay logic.

Specifically:

- Door required item checks continue using `itemId`;
- Item Pickup visual model/transform does not affect logical item identity;
- Item Pickups do not activate Pressure Plates;
- Player/Dynamic Box Pressure Plate modes remain unchanged;
- invisible plates remain unchanged.

No cross-feature coupling.

### Asset Delete Protection

Preserve M55 delete protection.

If an Item Pickup references a model identity in any relevant authored authority:

- workingCopy;
- active;
- savedSourceBaseline;

asset deletion remains protected.

The new visual fields do not affect dependency identity.

Runtime collected state must not weaken delete protection.

### Explicitly Out of Scope

Do not implement:

- ItemDefinition;
- ItemCatalog;
- `.item` files;
- icons;
- rarity;
- descriptions;
- pickup animation;
- bobbing;
- spinning;
- emissive effects;
- particles;
- sounds;
- automatic mesh recentering;
- GLB pivot rewriting;
- import-time scale metadata;
- asset metadata sidecars;
- generic RenderComponent;
- generic TransformComponent;
- ECS;
- generic visual override framework;
- Rotate gizmo framework;
- physics collider for Item Pickup;
- Item Pickup activating Pressure Plates;
- model-based gameplay targeting;
- inventory item 3D preview;
- hotbar/equipment/use;
- drag/drop from Content Browser into world;
- Static Prop visual redesign;
- Content Browser redesign;
- thumbnail redesign;
- material editor;
- animation system;
- prefabs;
- GUIDs;
- undo/redo;
- Level Format v2;
- M59 functionality.

### Focused Tests

Cover actual equivalents of:

#### Parsing / authored data
1. old M55 item pickup record still parses;
2. old record gets neutral visual transform defaults;
3. new visual transform syntax parses;
4. model identity with spaces still round-trips;
5. empty model identity remains valid;
6. finite visual offset validation;
7. finite visual rotation validation;
8. positive visual scale validation;
9. zero/negative scale rejected;
10. canonical writer round-trip;
11. Level Format limits preserved.

#### Lifecycle
12. Add defaults neutral;
13. Duplicate preserves visual transform and existing +1 X gameplay position rule;
14. equality includes visual fields;
15. Dirty/Modified behavior;
16. Apply/Revert/Save preserve fields;
17. runtime collection does not mutate authored visual state.

#### Rendering / editor
18. transformed model render uses gameplay position + visual offset;
19. rotation is applied;
20. scale is applied;
21. staged missing model preserves placeholder behavior;
22. no source fallback;
23. editor picking remains aligned/useful after transform;
24. Translate still edits gameplay position only;
25. visual editing does not move gameplay target point;
26. Scale gizmo behavior if implemented;
27. Content Browser assignment preserves canonical identity;
28. assigning/clearing model does not corrupt visual transform.

#### Gameplay regressions
29. M55 range/facing/LOS unchanged;
30. pickup prompt unchanged;
31. E arbitration M51/M55/M57 unchanged;
32. Inventory collection atomicity unchanged;
33. checkpoint behavior unchanged;
34. Restart behavior unchanged;
35. Apply/reload behavior unchanged;
36. PhysicsWorld rebuild behavior unchanged;
37. Door item requirements unchanged;
38. Pressure Plate modes unchanged;
39. asset delete protection unchanged;
40. collected pickup hidden/untargetable;
41. canonical cleanup;
42. no generic component/asset metadata framework introduced.

Do not expose public production APIs solely for tests.

### Manual Acceptance

Use disposable Development fixtures.

#### A. Assign imported models
1. Add at least two Item Pickups.
2. Assign different static GLB models from Content Browser.
3. Confirm real staged models render.

#### B. Visual scale
4. Change visual scale of one pickup.
5. Confirm model visibly scales.
6. Confirm gameplay pickup position remains unchanged.
7. Confirm E targeting still works from the expected gameplay point.

#### C. Visual rotation
8. Change visual rotation.
9. Confirm model orientation changes.
10. Confirm Inventory item identity/quantity is unaffected.

#### D. Visual offset
11. Change visual offset to compensate for an inconvenient GLB pivot.
12. Confirm rendered model moves relative to the gameplay pickup point.
13. Confirm collection still occurs according to the original authored gameplay position.

#### E. Editor selection
14. Select transformed pickup visually.
15. Confirm Hierarchy/Inspector selection remains reliable.
16. Translate pickup and confirm gameplay position + visual model move together.
17. If Scale gizmo is implemented, confirm it edits only visual scale.

#### F. Lifecycle
18. Duplicate transformed pickup.
19. Confirm item/model/visual transform preserved and gameplay position shifts +1 X.
20. Apply/Revert/Save and confirm values persist.
21. Restart and confirm visual transform remains authored while runtime collection resets normally.

#### G. Missing model
22. Temporarily test a valid identity with missing staged asset using existing safe workflow.
23. Confirm placeholder behavior remains and no Development source fallback appears.

#### H. Regression
24. Collect transformed pickup.
25. Confirm M54 Inventory receives correct `itemId`/quantity.
26. Confirm M56 displays it.
27. Confirm Door requiring that `itemId` still works.
28. Confirm Item Pickup still does not activate Pressure Plate.

Remove all disposable fixtures before closure.

### Validation

Run relevant current C++ tests including M58 plus regressions for M49, M51–M57.1, Level Format, renderer, picking, editor lifecycle, asset deletion, input arbitration and runtime lifecycle.

Run:

```text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Build:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:

`git diff --check`

### Canonical Data Safety

M58 changes Item Pickup authored serialization but must leave canonical Level 01 without test fixtures.

Expected closure:

- Item Pickups: 0
- Doors: 0
- Pressure Plates: 0
- Dynamic Boxes: 0
- Static Props: 0

Keep absent:

`dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

### Completion Criteria

M58 is ready for manual acceptance when:

- Item Pickups support authored visual offset, rotation, and scale;
- gameplay `position` remains the interaction authority;
- imported staged GLB models render with the authored visual transform;
- model identity with spaces still round-trips safely;
- old M55 records remain compatible;
- editor picking remains practical;
- Content Browser assignment remains canonical and narrow;
- runtime collection/lifecycle behavior remains unchanged;
- Door/Pressure Plate systems remain isolated;
- asset delete protection remains correct;
- Debug/Development/Release builds and tests pass;
- canonical Level 01 is clean;
- no generic component, item asset, asset metadata, animation, or Level Format v2 system is introduced.

### STOP Rule

After implementation, validation, and report, Cursor must STOP.

Do not commit, push, merge, start M59, or declare M58 CLOSED.

Closure occurs only after user manual acceptance and the separate Git closure workflow.
