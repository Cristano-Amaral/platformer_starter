## Milestone 50 --- Static Prop Placement Workflow

### Status

Implemented; awaiting mandatory manual acceptance. Not CLOSED. Milestone 49 is CLOSED. Milestone 51 has not started.

This milestone may begin only from a clean and synchronized `main` after
Milestone 49 is fully closed.

### Branch

`milestone/50-static-prop-placement`

### Recommended Cursor Model

**Grok 4.6 High --- Fast OFF**

------------------------------------------------------------------------

### 1. Goal

Milestone 50 turns the Static Model asset workflow completed in M47--M49
into a productive level-building loop.

The editor must allow a user to select an imported static model in the
Content Browser, enter a dedicated Static Prop placement mode, preview
the future instance in the 3D viewport, choose a valid authored
placement surface, and click to create the Static Prop.

The resulting object is the same authored Static Prop introduced in M49.
M50 must not create a second prop representation, placement-only object
type, asset registry, transform system, or lifecycle authority.

The intended production loop is:

**Import → Find → Preview → Place → Transform → Duplicate → Apply → Save
→ Cook/Stage → Run**

M50 focuses on the **Preview → Place** part and integrates it with the
existing M49 Static Prop lifecycle and editor tools.

------------------------------------------------------------------------

### 2. Existing Architecture to Reuse

M50 must inspect the current repository and reuse the actual
implementations and conventions established by previous milestones.

Relevant existing concepts include:

-   `StaticModelCatalog` as the static-model catalog authority.
-   Content Browser asset selection, independent from scene-object
    selection.
-   M48.1 thumbnails and List/Thumbnails view.
-   M48.2 Model Preview.
-   M49 authored Static Props.
-   M49 complete Static Prop authored transform:
    -   position
    -   rotation
    -   scale
-   M49 Static Prop Add/Duplicate/Delete lifecycle.
-   `workingCopy` / `active` / `savedSourceBaseline` authority.
-   Apply / Revert / Save / Modified behavior.
-   Hierarchy and Inspector integration.
-   Translate and Static Prop Scale gizmos.
-   Existing Object Palette / placement-surface concepts from earlier
    milestones where semantically appropriate.
-   Existing editor viewport picking/ray logic where appropriate.
-   Existing Ground / Platform / Slope placement surfaces.
-   M49 clip-plane isolation rules for Model Preview, thumbnails, and
    Gameplay.

Do not build parallel systems where an existing authority is adequate.

------------------------------------------------------------------------

### 3. Scope

#### 3.1 Placement entry point

The Content Browser must provide an explicit way to start placement for
the currently selected valid Static Model.

Use a clear action such as:

`Place Static Prop`

The exact UI placement should follow the current Content Browser
conventions.

The existing M49 direct-add behavior may remain available as a distinct
operation if it currently exists and remains useful.

**Direct Add and Placement are not the same operation.**

Direct Add creates immediately using its established deterministic
behavior.

Placement enters an interactive viewport placement mode and does not
author an object until placement is confirmed.

#### 3.2 Placement mode

Starting placement must create a transient editor placement state
containing only what is needed to place a Static Prop.

At minimum it needs the selected canonical static-model identity.

The placement state is **not** a `LevelDefinition` object.

Entering placement must not mutate:

-   `workingCopy`
-   `active`
-   `savedSourceBaseline`

until the user confirms a placement.

#### 3.3 Real-model placement preview

While Static Prop placement mode is active, render a transient
preview/ghost of the actual selected static model in the editor
viewport.

Do not use the old cyan primitive pending-add proxy as the primary
Static Prop placement preview.

The preview should make it obvious that it is not yet authored. Use the
narrowest visual treatment compatible with the current renderer, such as
tint/transparency/outline or another existing editor convention.

The preview must:

-   use the selected static-model identity;
-   use the same low-level model resource/bounds authority as the
    current Static Prop renderer where appropriate;
-   not reload the GLB every frame;
-   not mutate the shared model resource;
-   not become a scene object until confirmed;
-   not create physics;
-   not appear in Gameplay;
-   not affect Apply/Save/Dirty by merely moving the mouse.

#### 3.4 Placement surfaces

Reuse the existing authored placement-surface concept where appropriate.

For M50, valid placement surfaces are:

-   Ground
-   Platform
-   Slope

Static Props themselves are **not** placement surfaces in M50.

Dynamic Boxes are **not** placement surfaces.

Hazards, Collectibles, Checkpoints and other authored markers are not
placement surfaces.

Do not add arbitrary mesh-surface placement or
Static-Prop-on-Static-Prop placement.

#### 3.5 Surface hit and position

Use the current editor viewport ray/picking infrastructure or the
narrowest reusable part of the existing Object Palette placement
implementation.

The placement preview position must follow the valid surface under the
mouse.

The final authored position must be deterministic and correspond to the
preview shown at confirmation.

Avoid a second independent ray/surface algorithm if the existing
placement path can be safely reused.

#### 3.6 Surface contact / model bounds

Static Props have real model bounds.

The placement workflow should place the model so that its authored
preview rests sensibly on the selected surface rather than blindly
placing the model origin at the surface hit if that would visibly bury
or float normal models.

Use the existing loaded model bounds and authored transform where
practical.

For the initial M50 placement preview:

-   default rotation = the established Static Prop default;
-   default scale = the established Static Prop default;
-   use the model's local bounds to derive the vertical/support offset
    required for surface contact.

Do not modify the GLB or normalize its geometry.

Do not introduce pivots, custom origins, per-asset import settings, or
asset metadata.

If an unusual model has an unusual authored origin, M50 only needs
deterministic bounds-based placement consistent with the current model
representation.

#### 3.7 Confirmation

Primary viewport confirmation (normally left click, following current
editor conventions) creates exactly one authored Static Prop in:

`workingCopy`

using:

-   selected asset identity;
-   preview position;
-   default authored rotation;
-   default authored scale.

After creation:

-   the new Static Prop becomes the scene selection;
-   Hierarchy and Inspector reflect it;
-   the level becomes Modified according to existing authored lifecycle
    semantics;
-   `active` remains unchanged until Apply;
-   Save semantics remain unchanged.

#### 3.8 Repeated placement

A productive repeated-placement loop is in scope.

After confirming one Static Prop, placement mode should remain active
with the same asset selected so the user can place additional instances
without returning to the Content Browser after every click.

This behavior must be explicit and easy to exit.

The exact UI may follow existing Object Palette placement conventions.

Repeated placement must create one authored Static Prop per confirmed
click and must not accumulate duplicate requests from one click.

#### 3.9 Cancel / exit placement

The user must be able to cancel/exit Static Prop placement without
authoring the current preview.

At minimum support the editor's established cancel convention,
preferably `Esc` if that is already used by placement/tools.

Changing to an incompatible editor operation may also cancel placement
if that matches current editor conventions.

Cancel must:

-   remove the transient preview;
-   leave existing authored Static Props untouched;
-   not mutate `active`;
-   not mutate the saved baseline;
-   not mark the level Modified solely because placement was cancelled.

#### 3.10 Content Browser selection changes

Asset selection and scene selection remain separate.

If the user starts placement with asset A and then intentionally selects
asset B in the Content Browser while placement mode remains active,
choose the smallest coherent behavior consistent with the current UI
architecture:

-   either update the pending placement asset to B; or
-   require an explicit new `Place Static Prop` action.

Do not silently mix asset identity and scene selection.

Document and test the chosen behavior.

#### 3.11 Placement and existing transform tools

After a Static Prop is created, the normal M49 authored workflow takes
over.

The user may then use:

-   Inspector Position
-   Inspector Rotation
-   Inspector Scale
-   Translate gizmo
-   Scale gizmo
-   Duplicate
-   Delete
-   Apply
-   Revert
-   Save

M50 does not need to transform the transient preview with the normal
gizmos.

#### 3.12 Hierarchy / Inspector

The transient placement preview must not appear as a normal Hierarchy
object.

Only confirmed Static Props appear in Hierarchy.

Only confirmed Static Props are edited in Inspector as authored scene
instances.

Do not create a fake `workingCopy` entry just to drive the preview.

#### 3.13 Runtime and persistence

M50 does not change the Static Prop runtime representation or Level
Format syntax introduced by M49.

Once confirmed, a placed Static Prop must flow through the existing M49
path:

`workingCopy → Apply → active → Save → cook/stage → staged runtime rendering`

No new runtime placement concept is required.

------------------------------------------------------------------------

### 4. Placement UX Requirements

The workflow should support this sequence without unnecessary mode
switching:

1.  Open Content Browser.
2.  Find/select a static model.
3.  Click `Place Static Prop`.
4.  Move mouse over the editor viewport.
5.  See the real-model placement preview follow valid surfaces.
6.  Click to create.
7.  Move to another valid surface.
8.  Click to create another instance.
9.  Press `Esc` or the established cancel action to leave placement.
10. Select any created prop.
11. Translate/Scale or edit transform in Inspector.
12. Apply.
13. Save.

The user must not need to reopen the import dialog, Model Preview, or
Object Palette between repeated placements.

------------------------------------------------------------------------

### 5. Preview Validity Feedback

The user must be able to distinguish a valid placement from an invalid
one.

When the cursor has no valid Ground/Platform/Slope surface:

-   do not create a Static Prop on click;
-   do not guess a world position;
-   show the preview as invalid or hide it, following the narrowest
    current editor convention.

Do not place at camera target, origin, last valid hit, or another
fallback when the current cursor position is invalid.

------------------------------------------------------------------------

### 6. Resource and Renderer Safety

Placement preview rendering must preserve the renderer-state ownership
rules established by M49.

In particular:

-   no clip-plane leakage;
-   no shader/material state leakage into the editor or Gameplay;
-   no mutation of shared `Model` resources;
-   no per-frame `LoadModel`;
-   no resource double-unload;
-   preview lifetime must be deterministic;
-   switching/cancelling placement must not leave stale draw
    submissions.

If the current Static Model scene/resource store can be narrowly reused
for preview resources, prefer that over a second loader/cache.

Do not create a generalized asset manager.

------------------------------------------------------------------------

### 7. Authored-State Authority

Placement preview is transient.

Before confirmation:

-   `workingCopy` unchanged;
-   `active` unchanged;
-   `savedSourceBaseline` unchanged;
-   Modified/Dirty unchanged.

On confirmation:

-   exactly one Static Prop is added to `workingCopy`;
-   Modified becomes true according to current lifecycle;
-   `active` remains unchanged until Apply.

Apply/Revert/Save semantics remain those of M49.

------------------------------------------------------------------------

### 8. Direct Add Compatibility

M49 introduced direct-add of a selected Content Browser asset.

M50 must not accidentally convert Direct Add into interactive placement.

If Direct Add remains in the UI:

-   it continues to create immediately;
-   Placement is a separate explicit action;
-   both use the same Static Prop authored representation and
    validation;
-   both preserve asset-reference safety.

If repository inspection shows Direct Add has become redundant or
confusing, do not remove it opportunistically. Report the UX concern and
keep scope stable unless removal is necessary for correctness.

------------------------------------------------------------------------

### 9. Delete Asset Reference Safety

M49 prevents deletion of a static-model asset referenced by relevant
authored Static Prop authority.

M50 must preserve this.

A transient placement preview alone is not an authored level reference.

Do not expand M49 into a generalized dependency graph.

If Delete Asset is attempted while the same asset is only in transient
placement mode, use the narrowest safe behavior consistent with current
UI ownership, such as cancelling placement before deletion or rejecting
the action while placement is active.

Document the chosen behavior.

Confirmed Static Props must continue to protect their referenced asset
exactly as in M49.

------------------------------------------------------------------------

### 10. Level Format

No new Level Format version.

No new Static Prop syntax is required.

Placed props serialize through the M49 `static_prop` representation.

Do not introduce placement metadata into the level file.

Do not persist:

-   placement mode;
-   ghost state;
-   last mouse hit;
-   placement surface identity;
-   editor-only preview state.

------------------------------------------------------------------------

### 11. Development / Debug / Release

Interactive authored placement is a Development editor feature.

Debug and Release must remain free of Development editor UI.

Release runtime continues to render applied/staged Static Props through
the existing M49 runtime path.

------------------------------------------------------------------------

### 12. Explicitly Out of Scope

Do **not** implement in M50:

-   Static Prop collision;
-   Static Prop Jolt bodies;
-   physics configuration;
-   gravity;
-   mass/friction/restitution;
-   Grab/Carry;
-   pressure plates;
-   inventory;
-   interaction policies;
-   aliases/authored display names;
-   GUIDs;
-   ECS;
-   prefabs;
-   scene graph/parenting;
-   folders in Hierarchy;
-   undo/redo;
-   generic asset dependency graph;
-   drag-and-drop from Content Browser;
-   drag-and-drop from OS;
-   Static-Prop-on-Static-Prop placement;
-   arbitrary mesh-surface placement;
-   placement on Dynamic Boxes;
-   vertex/face snapping;
-   grid snapping framework;
-   rotation snapping;
-   scale snapping;
-   local/world transform-space framework;
-   Rotate gizmo;
-   Level Format v2;
-   generic placement framework for future gameplay objects.

M50 may reuse existing generic-enough placement helpers, but must not
create speculative abstractions for later milestones.

------------------------------------------------------------------------

### 13. Expected Tests

Cursor must inspect the actual test architecture and add focused
regression coverage at the narrowest practical boundaries.

At minimum cover equivalents of:

1.  placement cannot start without a valid selected static model;
2.  placement can start from a valid Content Browser static model;
3.  entering placement does not mutate `workingCopy`;
4.  entering placement does not mutate `active`;
5.  entering placement does not mutate saved baseline;
6.  moving preview does not mark Modified;
7.  valid Ground hit produces valid preview;
8.  valid Platform hit produces valid preview;
9.  valid Slope hit produces valid preview;
10. Dynamic Box is not a placement surface;
11. Static Prop is not a placement surface;
12. invalid/no hit cannot confirm;
13. confirmation creates exactly one Static Prop in `workingCopy`;
14. created identity equals selected asset identity;
15. created position matches preview result;
16. default rotation is correct;
17. default scale is correct;
18. bounds-based support offset is deterministic;
19. confirmation does not mutate `active` before Apply;
20. Apply promotes the placed prop;
21. Revert behavior remains correct;
22. repeated placement creates one instance per confirmation;
23. repeated instances remain independent;
24. cancel creates no object;
25. cancel clears transient preview;
26. cancel alone does not mark Modified;
27. confirmed prop becomes scene selection;
28. Content Browser asset selection remains independent from scene
    selection;
29. Hierarchy contains confirmed prop but not transient preview;
30. Direct Add behavior remains distinct and functional;
31. M49 Translate gizmo remains functional;
32. M49 Scale gizmo remains functional;
33. primitive Resize remains unchanged;
34. Delete Asset reference protection remains correct;
35. placement preview alone does not become a persistent authored
    dependency;
36. preview resource is not loaded every frame;
37. switching/cancelling does not leave stale preview submissions;
38. Preview/thumbnail/placement rendering does not leak clip planes into
    Gameplay;
39. Save/Level Format round-trip remains green;
40. no M50 placement state is serialized.

Do not expose production APIs solely for tests.

------------------------------------------------------------------------

### 14. Manual Acceptance

Manual acceptance must include at least:

#### A. Enter placement

-   Select Barrel or another valid imported static model in Content
    Browser.
-   Click `Place Static Prop`.
-   Confirm the editor enters placement mode without immediately
    creating a prop.

#### B. Real-model preview

-   Move over Ground.
-   Confirm the actual model preview follows the cursor/surface.
-   Confirm it visually rests on the surface rather than obviously using
    a primitive proxy.

#### C. Platform

-   Move over a Platform.
-   Confirm valid preview and placement.

#### D. Slope

-   Move over a Slope.
-   Confirm valid preview and placement orientation/position remains
    deterministic under M50 semantics.

M50 does not require automatic alignment of model rotation to the slope
normal unless the existing placement convention already does this
narrowly. Do not invent slope-normal rotation if not already supported.

#### E. Invalid target

-   Move over empty space or an invalid object.
-   Confirm click does not create a prop.

#### F. Repeated placement

-   Place several instances of the same asset without returning to
    Content Browser.
-   Confirm exactly one instance per click.

#### G. Cancel

-   Press the established cancel action.
-   Confirm preview disappears.
-   Confirm no extra prop is created.

#### H. Transform

-   Select a placed prop.
-   Use Translate.
-   Use Scale.
-   Edit Rotation in Inspector.
-   Confirm normal M49 transform workflow remains correct.

#### I. Apply / Revert

-   Place a prop.
-   Revert and confirm pending placement disappears according to
    existing authored semantics.
-   Place again.
-   Apply and confirm it becomes active.

#### J. Save / Reload

-   Save a disposable authored test if appropriate.
-   Reload.
-   Confirm the placed Static Prop persists with identity and transform.

#### K. Multiple assets

-   Place at least two different static-model assets.
-   Confirm each instance keeps the correct identity/resource.

#### L. Gameplay

-   Apply placed props.
-   Enter Gameplay.
-   Confirm they render correctly.
-   Confirm no preview/ghost appears in Gameplay.
-   Confirm the M49 Chest/clip-plane regression remains fixed.

#### M. Direct Add

-   If Direct Add remains available, confirm it still behaves as
    immediate add and is distinct from interactive placement.

#### N. Canonical cleanup

-   Remove manual test Static Props from canonical Level 01.
-   Apply and Save.
-   Confirm canonical Level 01 returns to 0 Static Props before closure.

------------------------------------------------------------------------

### 15. Validation

Run the current relevant C++ test suite, including actual equivalents
of:

-   Static Prop lifecycle tests
-   Static Prop render/transform tests
-   Static Prop scene/resource isolation tests
-   Content Browser tests
-   Content Browser thumbnail tests
-   Static Model Preview tests
-   Editor placement tests
-   Editor picking tests
-   Editor gizmo tests
-   Editor quick-toolbar tests
-   authored lifecycle integration tests
-   Level file tests
-   cook/stage/reload workflow tests
-   canonical scene cleanup tests
-   M49 clip-plane regression test

Run:

``` text
python tools/test_import_static_glb.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:

``` text
git diff --check
```

------------------------------------------------------------------------

### 16. Canonical Data Safety

`game/assets/source/levels/level_01.level` must not retain manual M50
placement fixtures at closure.

Expected final canonical state remains:

-   Static Props: 0 unless the user explicitly authorizes production
    authored props later.
-   Legacy `dynamic_box 0 5 0 1 1 1 30` remains absent.

Do not mechanically overwrite unrelated authored semantic changes.

------------------------------------------------------------------------

### 17. Completion Criteria

M50 is ready for user manual acceptance only when:

-   Content Browser can explicitly enter Static Prop placement mode.
-   Real selected model appears as transient viewport preview.
-   Ground / Platform / Slope placement works.
-   Invalid targets cannot create props.
-   Confirmation creates exactly one M49 Static Prop in `workingCopy`.
-   Repeated placement works.
-   Cancel works without authored mutation.
-   Bounds-based surface contact is deterministic.
-   Scene and asset selection remain separate.
-   Apply/Revert/Save remain correct.
-   Direct Add remains distinct if retained.
-   Translate and Scale gizmos remain correct after creation.
-   runtime rendering remains correct.
-   clip-plane regression remains fixed.
-   no physics/collision/interaction features were added.
-   no M51+ functionality was anticipated.
-   all required tests/builds pass.
-   canonical Level 01 is clean.

------------------------------------------------------------------------

### 18. STOP Rule

After implementation and validation, Cursor must:

-   report results;
-   identify any limitation or deviation;
-   STOP.

Cursor must **not**:

-   commit;
-   push;
-   merge;
-   start M51;
-   declare M50 closed.

M50 closes only after user manual acceptance and the separate Git
closure workflow.
