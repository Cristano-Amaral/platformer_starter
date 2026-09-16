## Milestone 49 — Authored Static Props

### Status
CLOSED. Milestone 50 is implemented and awaits manual acceptance.

### Branch
`milestone/49-authored-static-props`

### Goal
Introduce authored Static Prop instances that reference imported static GLB assets, allowing reusable project assets to become persistent visual objects in a level without yet implementing the Content Browser placement workflow.

### Core distinction
- **Static Model Asset**: reusable project asset discovered through the existing `StaticModelCatalog`, identified by its canonical project-relative asset identity.
- **Static Prop**: authored level instance that references one Static Model Asset and owns its authored transform/state.

M49 must preserve this distinction. An asset is not itself a scene object, and selecting an asset in the Content Browser is not scene selection.

### Repository-first architecture
Before implementation, inspect the real post-M48.2 repository and current conventions for `LevelDefinition`, Level Format v1, `workingCopy` / `active` / `savedSourceBaseline`, authored lifecycle requests, Hierarchy, Inspector, rendering, picking, gizmos, physics rebuild boundaries, cook/stage/reload, Content Browser Delete Asset, StaticModelCatalog/model loading, and all M47–M48.2 asset workflows.

Use the narrowest architecture that fits the current engine. Do not introduce ECS, GUIDs, prefabs, generic components, a generic scene-object framework, a generalized asset-instance system, generic dependency graph, generic asset database, or generic reference manager unless the existing repository already requires such an abstraction.

### Authored data
Add a repeatable authored Static Prop category to the current level-authoring model.

Each Static Prop must reference a canonical static model asset identity and must own a complete authored visual transform:

- `position`
- `rotation`
- `scale`

All three are part of the functional target of M49.

The exact Level Format v1 syntax and exact in-memory transform representation must be chosen only after inspecting the current parser/writer, rendering, Inspector, picking and gizmo architecture. Prefer the smallest backward-compatible Level Format v1 extension and the smallest representation consistent with the current repository.

Do not create a generic `Transform` component/framework merely to satisfy Static Props.

If `rotation` or `scale` would require a disproportionate architectural expansion that cannot be kept narrow and consistent with the actual repository, Cursor must **STOP and report the blocker to the user before omitting that capability or inventing a large framework**.

M49 must not silently degrade into a position-only Static Prop milestone without explicit user approval.

Static Prop transform is authored state. Runtime rendering must not mutate it.

### Authored transform vs gizmo scope
Authored transform data and manipulation tools are separate concerns.

M49 must own, persist and round-trip:

- `position`
- `rotation`
- `scale`

All three must be editable at minimum through the Inspector.

#### Gizmo requirements
- **Translate**: integrate with the existing translation gizmo when applicable.
- **Rotate**: implement a Rotate gizmo only if it can be added narrowly and coherently to the current gizmo architecture without redesigning the entire gizmo system.
- **Scale**: implement a Scale gizmo only if it can be added narrowly and coherently to the current gizmo architecture without redesigning the entire gizmo system.

Do not semantically reuse primitive `Resize` as if it were automatically equivalent to visual model `Scale`.

A justified absence of Rotate and/or Scale gizmos does **not** remove `rotation` or `scale` from the authored Static Prop data model. Those fields must still exist, persist, validate, render correctly and be editable through the Inspector.

The final Cursor report must state exactly which Static Prop transform operations are available through:
- Inspector;
- Translate gizmo;
- Rotate gizmo;
- Scale gizmo.

### Asset references and validation
Static Props reference assets by the existing canonical project-relative identity, for example:

`models/example.glb`

Do not add GUIDs.

On load/apply, validate references using the existing static-model/catalog/path-safety authority. A missing/invalid referenced asset must fail safely and diagnostically without arbitrary filesystem access or corrupting authored state.

Do not copy GLB data into the level file.

Do not serialize:
- absolute filesystem paths;
- cooked absolute paths;
- staged absolute paths;
- thumbnail hashes or PNGs;
- Model Preview state;
- runtime resource handles.

### M48 Delete Asset compatibility / reference safety
M49 introduces the first authored level references to Static Model Assets. Therefore the existing M48 **Delete Asset** operation must no longer be allowed to silently remove a Static Model Asset that is known to be referenced by authored Static Props in the currently relevant authored level context.

Before implementing the rule, inspect the real post-M48.2 authority model and determine which authored states are relevant for reference safety. The implementation must explicitly decide whether deletion checks:
- `workingCopy`;
- `active`;
- `savedSourceBaseline`;
- a narrow combination of these;
- or another existing authored authority that is actually canonical for the current editor flow.

Use the smallest rule consistent with the repository's real authority semantics.

At minimum, attempting to delete a Static Model Asset that the relevant authored authority knows is referenced by one or more Static Props must:
- fail safely;
- preserve the asset;
- preserve the referencing Static Props;
- preserve thumbnail/model-preview behavior;
- produce a clear diagnostic explaining that the asset is referenced.

Do not create:
- generic dependency graph;
- asset database;
- reference manager framework;
- force-delete;
- automatic retargeting;
- automatic deletion of referencing Static Props;
- automatic reference repair.

The check must be narrow and authored-level aware, not a speculative cross-project dependency system.

The final Cursor report must document precisely:
- which authored authority or authorities are checked;
- why that scope was chosen;
- whether unsaved pending references are protected;
- whether applied-but-unsaved references are protected;
- whether saved-baseline references are protected;
- how Delete Asset behaves on rejection.

### Editor integration
Integrate Static Props into the existing Development editor:
- Hierarchy category/entries;
- scene selection;
- Inspector;
- Add/Duplicate/Delete authored lifecycle;
- Apply/Revert/Save authority;
- Modified/Dirty semantics;
- editor viewport rendering;
- CPU/editor picking as appropriate;
- existing transform workflow where applicable.

Asset selection in Content Browser remains independent from Static Prop scene selection.

M49 may provide a narrow direct-add path needed to create a Static Prop for testing/authoring, but it must **not** implement the M50 Content Browser-to-viewport placement workflow. No drag/drop, ghost placement, click-to-place loop, repeated placement or double-click instantiate.

If a direct-add action needs an asset reference, use the smallest explicit UI consistent with current editor patterns; do not turn this into the M50 placement workflow.

### Rendering and resource lifetime
Render Static Props using the referenced real static GLB asset.

Reuse post-M48.2 model loading/rendering primitives where architecturally appropriate, but do not make level rendering depend on thumbnail cache or Model Preview state.

Avoid loading the same GLB independently every frame or once per instance if a narrow shared model-resource cache is clearly warranted by the actual repository. Any such cache must remain static-model resource infrastructure, not a speculative generic asset manager.

Multiple Static Props may reference the same model resource while owning independent authored transforms.

Static Props are visual authored objects in M49. They do not automatically become physics bodies, collision surfaces, gameplay interactables, placement surfaces, hazards, collectibles, triggers, inventory items, or grab/carry objects.

### Transform rendering behavior
The rendered Static Prop must apply its authored:
- position;
- rotation;
- scale.

The exact rotation representation may be Euler angles, quaternion-backed storage, matrix decomposition, or another narrow form only if it matches the actual repository architecture. Serialization must remain deterministic.

Scale must represent authored visual model scale, not primitive dimensions. Validate against non-finite, zero, negative or otherwise pathological values according to the narrowest rule consistent with the engine.

Picking must account for all implemented authored transform fields. If exact mesh picking is unnecessary, use the narrowest reliable transformed-bounds approach.

### Physics boundary
Do not add Static Props to Jolt merely because they are visible models.

Preserve existing physics body accounting and `PhysicsWorld::TryRebuild` behavior unless the current implementation genuinely requires an unrelated compatibility adjustment.

Static Props are non-physical by default in M49.

### Lifecycle and authority
Preserve the editor authority model:
- `workingCopy` contains pending authored edits;
- Apply validates/promotes to `active`;
- Revert restores the appropriate authored state;
- Save writes authored source state;
- runtime/transient rendering state is not authored state.

Add/Duplicate/Delete/transform changes to Static Props must follow the same authored lifecycle conventions as existing repeatable categories.

Expected behavior includes:
- Add Static Prop -> `workingCopy` changes;
- Duplicate -> `workingCopy` changes;
- Delete instance -> `workingCopy` changes;
- Inspector transform edit -> `workingCopy` changes;
- Apply -> validated props promote to `active`;
- Revert -> pending edits roll back according to current semantics;
- Save -> Static Props serialize;
- reload -> saved Static Props reconstruct.

### Hierarchy
Integrate Static Props into current Hierarchy conventions.

Use a repeatable category such as `Static Props` when consistent with existing category behavior.

Each instance must be individually selectable.

Reuse `StructuralIndexMap` or the actual current session-local mapping mechanism where appropriate.

Do not introduce folders, parenting, scene graph, or GUID-based hierarchy identity.

### Inspector
When a Static Prop is scene-selected, the Inspector must expose the minimum useful authored instance data:
- referenced static-model asset identity;
- `position`;
- `rotation`;
- `scale`.

All three transform fields must be editable through the Inspector in M49.

Use current Inspector patterns.

Do not create a generic property system, component inspector, material inspector, mesh inspector, shader inspector, or gameplay property system.

### Add / Duplicate / Delete instance semantics
#### Add
Creates one `workingCopy` Static Prop using a valid Static Model Asset reference and deterministic/default transform values. Does not change `active` until Apply.

#### Duplicate
Duplicates the selected Static Prop, preserving asset reference and complete authored transform, except for any already-established deterministic duplicate offset convention in the repository.

#### Delete instance
Removes only the selected authored Static Prop from `workingCopy`.

It must **not**:
- delete the underlying GLB asset;
- delete thumbnail cache;
- modify the StaticModelCatalog;
- alter Content Browser asset selection unless current scene-selection rules independently require it.

This is distinct from M48 Delete Asset.

### Content Browser selection vs scene selection
Preserve M48/M48.1/M48.2 boundaries.

Content Browser selection represents:
- reusable project asset selection.

Hierarchy/viewport selection represents:
- authored scene instance selection.

Creating a Static Prop may consume the currently selected asset identity if that is the chosen narrow direct-add workflow.

After creation, the Static Prop becomes a normal authored scene object. Content Browser selection remains independent.

Selecting a Static Prop must not silently change Content Browser selection.

Selecting an asset must not silently change scene selection.

Model Preview continues to follow Content Browser asset selection, not Static Prop scene selection.

### Runtime / reload
Applied Static Props must render from `active` authored data in the appropriate runtime/editor path.

Save and staged-runtime reload must preserve:
- asset reference;
- position;
- rotation;
- scale.

Do not introduce Development source fallback where staged runtime authority currently forbids it.

Inspect the cooker/stager and add only the minimum required handling so referenced static model assets are available through the correct runtime path.

### Cook / stage dependency handling
Inspect the post-M48.2 asset pipeline before deciding behavior.

If a level references a Static Prop asset, the existing cook/stage workflow must produce a runnable staged result without relying on Development source paths at runtime.

Implement only the narrow dependency mapping required for Static Props and existing static GLB assets.

Do not create a general dependency graph, package manager, registry database, GUID system, generic reference manager, or Asset Manager framework.

### Model Preview independence
M48.2 Model Preview remains an asset-inspection tool.

Static Prop scene rendering must not depend on:
- Model Preview window visibility;
- Model Preview current selected asset;
- Model Preview loaded model resource;
- Model Preview camera;
- Model Preview render target.

Selecting a Static Prop must not hijack Model Preview selection.

### Thumbnail independence
Static Props must not depend on:
- thumbnail PNG;
- thumbnail metadata;
- thumbnail cache hit;
- thumbnail GPU texture.

Thumbnail cache remains a derived Content Browser representation.

### Canonical data safety
Do not make semantic changes to:

`game/assets/source/levels/level_01.level`

merely to demonstrate M49.

Tests should use fixtures/disposable levels/assets.

The canonical Level 01 should remain unchanged unless the user explicitly approves a content change later.

Expected canonical state remains:
- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The legacy line:

`dynamic_box 0 5 0 1 1 1 30`

must remain absent.

### Development / Debug / Release
Development owns:
- Static Prop authoring UI;
- Hierarchy integration;
- Inspector transform editing;
- Add/Duplicate/Delete authoring;
- authoring-specific asset-selection bridge;
- gizmo manipulation where implemented.

Debug and Release must continue to build correctly and remain free of Development-only editor UI.

Runtime rendering/data code required by staged levels may exist outside Development-only code as appropriate.

### Required focused tests
Add focused tests for the actual architecture, including:
- Level Format v1 parse/write/round-trip for Static Props;
- multiple/repeatable Static Props;
- old level compatibility with zero Static Props;
- canonical asset identity/path validation;
- missing/invalid asset reference handling;
- position round-trip;
- rotation round-trip;
- scale round-trip;
- finite/valid transform validation;
- Add/Duplicate/Delete lifecycle;
- `workingCopy` / `active` / `savedSourceBaseline` authority;
- Apply/Revert/Save;
- Modified/Dirty semantics;
- Hierarchy/scene selection integration;
- Inspector integration for position/rotation/scale;
- viewport rendering/resource mapping at a testable boundary;
- resource reuse for multiple instances of the same GLB;
- picking with transformed Static Props;
- Translate gizmo behavior where applicable;
- Rotate gizmo behavior if implemented;
- Scale gizmo behavior if implemented;
- explicit confirmation that absence of Rotate/Scale gizmos does not remove authored rotation/scale;
- asset selection vs scene selection isolation;
- Model Preview isolation;
- no physics-body creation by default;
- cook/stage/reload of referenced static models;
- missing staged dependency behavior;
- thumbnail independence;
- M48 Delete Asset rejection when a referenced model is protected by the chosen authored authority;
- Delete Asset success for genuinely unreferenced assets;
- Delete Asset cancellation unchanged;
- reference-safety behavior across whichever of `workingCopy`, `active`, `savedSourceBaseline` are chosen as relevant;
- Debug/Development/Release boundaries;
- regressions for M47, M48, M48.1, M48.2 and existing authored object lifecycle.

Do not expose public production APIs solely for tests. Reuse existing test-access patterns.

### Manual acceptance targets
The user must be able to create at least one Static Prop through the narrow M49 authoring path, select it in Hierarchy/viewport as implemented, inspect its asset reference and complete authored transform, edit position/rotation/scale through the Inspector, Duplicate/Delete it, Apply/Revert/Save, reload it, and run the level with the real referenced GLB rendered at the authored transform.

Manual acceptance must also confirm:
- Content Browser asset selection remains separate;
- Model Preview remains functional and independent;
- position, rotation and scale persist;
- Translate gizmo integrates where appropriate;
- Rotate/Scale gizmo availability matches the documented narrow-scope decision;
- primitive Resize was not misrepresented as model Scale;
- no automatic physics/collision/gameplay behavior;
- no M50 placement workflow has appeared;
- Delete Asset refuses to delete a model that is referenced by Static Props according to the documented authored authority scope;
- Delete Asset still works for unreferenced disposable assets;
- canonical Level 01 remains unchanged after cleanup.

### Explicit non-goals
Do not implement:
- M50 viewport placement workflow;
- drag-and-drop to level;
- ghost placement;
- repeated click placement;
- double-click instantiate;
- automatic physics/collision for props;
- Grab/Carry;
- Pressure Plates;
- Inventory;
- generic interaction/component systems;
- ECS;
- GUIDs;
- prefabs;
- parenting/folders/scene graph;
- undo/redo;
- generic Transform/component framework;
- generic dependency graph;
- generic asset database;
- generic reference manager;
- force-delete;
- automatic retargeting;
- automatic deletion of referencing Static Props;
- material/mesh/texture/shader editing;
- generic Asset Editor;
- new asset formats;
- Level Format v2 unless absolutely forced by existing repository constraints and explicitly reported before implementation.

### Validation
Run all focused tests and relevant existing regressions, current Python asset/cook/stage tests, and:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
git diff --check
```

Verify canonical Level 01 has no unintended semantic diff.

### Cursor final report
Report:
1. repository infrastructure discovered before architecture choice;
2. root implementation approach;
3. exact files changed;
4. Static Model Asset vs Static Prop data model;
5. exact Level Format v1 syntax;
6. parser/writer/round-trip behavior;
7. asset identity/path validation;
8. missing/invalid asset policy;
9. complete authored transform representation;
10. position implementation;
11. rotation implementation;
12. scale implementation;
13. Inspector editability for position/rotation/scale;
14. Translate gizmo behavior;
15. Rotate gizmo behavior/decision;
16. Scale gizmo behavior/decision;
17. explanation proving primitive Resize was not incorrectly reused as model Scale;
18. `workingCopy` / `active` / `savedSourceBaseline` integration;
19. Modified/Dirty semantics;
20. direct-add workflow;
21. Hierarchy integration;
22. Inspector integration;
23. Duplicate/Delete instance semantics;
24. Content Browser asset selection isolation;
25. Model Preview isolation;
26. viewport rendering path;
27. model resource cache/lifetime/reuse;
28. thumbnail-cache independence;
29. viewport picking;
30. physics isolation;
31. cook mapping;
32. stage mapping;
33. runtime lookup/reload;
34. missing staged dependency behavior;
35. M48 Delete Asset compatibility;
36. exact authored authority/authorities checked for reference safety;
37. Delete Asset rejection behavior and diagnostic;
38. tests added/changed;
39. focused Static Prop test results;
40. M47–M48.2/editor/physics regressions;
41. Python results;
42. Debug/Development/Release builds;
43. canonical Level 01 verification;
44. asset/cache/repository safety;
45. `git diff --check`;
46. remaining risks/limitations.

### Stop rule
After implementation, automated validation, documentation and report: STOP.

Do not commit, push, merge, start M50, or declare M49 CLOSED.

If complete authored `position + rotation + scale` cannot be implemented without a disproportionate architectural expansion, STOP and report the blocker before reducing scope or inventing a large framework.

Manual acceptance and Git closure remain mandatory.

### Implementation notes (post-inspection)
- Data: `world::StaticPropSpec` (`modelIdentity`, `position`, Euler XYZ `rotationDegrees`, `scale`).
- Level Format v1: `static_prop <px> <py> <pz> <rx> <ry> <rz> <sx> <sy> <sz> <identity...>`. Identity last. No v2.
- Parse/Apply: grammar + finite transform + scale `> 0` + `TryParseStaticModelIdentity`. No on-disk existence check.
- Missing staged GLB: `StaticModelSceneStore` marks failed; renderer draws a fallback cube. No source fallback.
- Direct-add: Content Browser toolbar **Add Static Prop** and `Edit > Add > Static Prop` share `LevelEditorRequest::AddStaticProp` using Content Browser `selectedIdentity`. Not Object Palette / M50 placement.
- Correction 1: the menu item existed but was disabled unless `contentBrowser.selectedIdentity` already parsed as a valid Static Model identity, with no hover reason and no Content Browser action. Manual acceptance could not discover the workflow. The toolbar button uses the same `CanIssueAuthoredLifecycleRequest` / `HandleAuthoredLifecycleRequest` path; both surfaces now share disable-reason copy.
- Correction 2 (Edit menu): `Edit > Add > Static Prop` is wired correctly and routes the same `LevelEditorRequest::AddStaticProp` as the toolbar button; there is no enablement, submenu, routing, or gizmo-state defect. It is the only `Add` row whose enablement comes from another window, and that dependency was invisible in the menu bar, so a greyed row read as "the command does not exist". Kept (Option A) and made self-describing: the row's right-hand column shows the selected asset filename or `select asset` (`AddStaticPropMenuHint`), and the `Add` submenu carries a persistent `Static Prop uses the Content Browser selection.` note. The Content Browser button remains the primary direct-add UI.
- Correction 2 (staged diagnostic): the Inspector Static Prop panel and the Content Browser selected-asset line now report `Static model is not available in staged runtime assets, so the viewport draws a placeholder cube. Run Build > Cook & Stage.` Policy lives in `ClassifyStaticPropAsset` / `StaticPropAssetStateMessage`; the ImGui caller supplies the staged-file answer. No automatic cook/stage and no canonical-source fallback.
- Correction 2 (rendering): reproduced with all four models staged and applied. No renderer defect found. `DrawProp` pushes and pops its own matrix, `DrawModel` takes `Model` by value so no per-instance transform is written back to the shared resource, and the authored spec is read-only, so nothing accumulates across frames or contaminates another instance, identity, or later world/HUD drawing.
- Correction 2 (measured raw world bounds, Scale `(1,1,1)`): Barrel `3.43 x 3.01 x 3.48` (center `0.00, 0.87, 0.00`), `models/test_textured.glb` `2.00 x 2.00 x 2.00`, `models/test_authored.glb` `2.00 x 1.00 x 2.00`, `models/test_static.glb` `1.10 x 1.20 x 1.10`. The three test models are the *smallest* assets, so "unexpectedly large geometry" is not raw model size. Apparent size comes from proximity to the follow camera. Direct-add is unchanged and remains deterministic: camera-region X/Y with authored Z snapped to `spawn.z`; with the editor camera seeded from the gameplay pose that is the lane between the follow camera and the player, so repeated adds without moving the camera stack in one visible spot.
- Correction 2 (pre-Apply boxes): the pre-Apply representation is the M34/M41 cyan pending-Add ghost, sized from `kStaticPropDefaultLocalMin/Max` scaled by authored `scale`, which is why every pending prop looks like a similar unit box. `render::DebugWorldOverlay` is a per-frame local populated only while the editor is active, so it cannot reach Gameplay or mutate `active`. Not accidental fallback geometry, and no M50 ghost model rendering was added.
- Correction 2 (known limitation): picking and the selection highlight still use the unit local AABB proxy, not real GLB bounds, so a prop whose model is much larger or smaller than one unit has a pick box that does not match its silhouette.
- Correction 3 (Chest Gameplay): reproduced `models/Chest by Quaternius - O72u4Drp8k.glb` through source → cook → stage → LoadModel. The Chest **does load** (5 meshes, 4 materials, 0 textures, ~1.18×0.90×0.82 after raylib bakes the FBX `scale (100,100,100)` node transforms). It is smaller than the Barrel (~3.43×3.01×3.48). Default-add at camera-region X/Y and `spawn.z` (~`0.42, 1.54, 0`) does **not** put the gameplay camera `(2, 4.3, 12)` inside the Chest, and the look ray toward spawn does not immediately hit it. A hidden-window isolation pass showed that drawing Chest/Barrel cannot erase later greybox pixels or leave the next frame blank; HUD remaining visible matches ordinary 3D occlusion/sky, not a lost framebuffer. `DrawProp` now flushes the immediate batch before `DrawModel` and restores default shader/blend/depth/cull/texture afterward so a GLB without texcoords cannot leave `rlBegin` greybox draws unbound. Inspector reports loaded staged size at Scale `(1,1,1)`, current visual size, and warns when the gameplay camera/target is inside the loaded AABB. Scale is not auto-normalized. Direct-add placement is unchanged (not M50).
- Correction 4 (post-delete persistence): the user's Delete Chest → Apply → Gameplay still broken is not explained by a leftover instance. Apply copies `workingCopy` onto `active`; `Sync` already prunes unreferenced models with `UnloadModel`; draw submission is `active.staticProps` only. DrawMesh writes the last material `colDiffuse` onto the **default** shader (Chest Metal ≈ sky). Correction 3 restored state only after `DrawProp`, so a later zero-prop frame never healed. `RestoreGreyboxImmediateState` now runs at the start of every `DrawWorld` 3D pass (and after each prop): default shader, `colDiffuse` white, texture0. **Superseded:** user captures showed `colDiffuse = 1 1 1 1` in broken frames; leftover tint is same-pass hygiene, not the persistent blank world.
- Correction 5 (temporary diagnostic capture, removed): Development `Debug > Capture Next Gameplay Render Frame` was a one-shot framebuffer capture used to locate the blank Gameplay frame. It is not a shipped editor feature and was deleted in Correction 7 after the user confirmed the clip-plane fix.
- Correction 6 (persistent 3D clip planes): the divergent Gameplay state is `rlCullDistanceNear/Far`. Model Preview and thumbnails call `rlSetClipPlanes` for a tight model frame. Gameplay `BeginMode3D` builds the frustum from those getters; editor `BeginMode3DInRect` uses `RL_CULL_DISTANCE_NEAR/FAR` (0.05/4000), so the editor viewport can stay healthy while Gameplay is clear-only. A Chest-sized frame (~near 1.5, far 6) clips the gameplay camera (~13 units from origin); a Barrel-sized frame (~far 20) can still include the world. Ownership: Preview/thumbnail restore default clip planes after the offscreen pass; `DrawWorld` establishes `RL_CULL_DISTANCE_NEAR/FAR` again before `BeginMode3D`. `GreyboxImmediateStateTest` is the clip-plane regression (default framebuffer).
- Correction 7: removed Correction 5 capture UI/readback/triangle. Added Static Prop Scale gizmo (`EditorTransformMode::Scale`) editing `staticProp.scale` on independent X/Y/Z handles. Primitive Resize is unchanged. No uniform-scale hub, Rotate gizmo, physics, alias, or M50.
- Gizmos: Translate (position); Scale (Static Prop visual scale); Resize (primitive size). Rotation is Inspector-only.
- Delete Asset safety: union of `workingCopy` + `active` + `savedSourceBaseline` before `DeleteStaticModel`.
- Canonical Level 01 unchanged (0 Static Props).
