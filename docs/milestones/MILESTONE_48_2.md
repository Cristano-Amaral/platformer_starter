## Milestone 48.2 — Interactive Static Model Preview

**Status:** CLOSED.
**Branch:** `milestone/48.2-interactive-static-model-preview`
**Cursor model:** Grok 4.6 High — Fast OFF

### Baseline

M00–M48.1 are CLOSED. `main` is clean and synchronized.

M47 established Static GLB import/catalog and canonical project-relative asset identity. M48 established the Development Content Browser, independent asset selection, search, Refresh, Import and Delete. M48.1 added cached static-model thumbnails and persistent Thumbnails/List modes.

Before implementation, inspect the actual post-M48.1 repository. Current code, tests and documentation are authoritative. In particular, inspect the thumbnail implementation before deciding which loading, bounds, camera, render-target and resource-lifetime pieces should be shared with an interactive preview.

### Objective

Add a persistent/toggleable Development **Model Preview** window that renders the real static `.glb` currently selected in the Content Browser at a larger interactive size, without instantiating it into the level.

The Preview must:

- use Content Browser asset selection as its authority;
- render the real model rather than enlarge the cached thumbnail;
- automatically frame the model from its bounds;
- support orbit;
- support zoom/dolly;
- support Frame/Reset View;
- consider pan only if it is simple and consistent with current controls;
- follow Content Browser selection changes;
- handle no selection, Refresh, Import and Delete safely;
- avoid unnecessary model reload/resource recreation every frame;
- remain Development-only;
- preserve M47, M48 and M48.1 behavior.

### Architectural principle: share reusable preview primitives, not thumbnail cache authority

The M48.1 thumbnail is a derived cached representation. The M48.2 Preview must render the actual selected asset through the appropriate rendering path.

Inspect whether M48.1 contains reusable low-level pieces for:

- validated static GLB loading;
- model lifetime;
- model bounds;
- framing math;
- camera construction;
- render targets;
- background/render setup;
- texture/resource ownership.

Refactor narrowly only when it removes real duplication between thumbnail rendering and preview rendering. Do not make the interactive Preview depend on thumbnail PNG/cache validity. Do not turn `StaticModelThumbnailCache` into a generic asset system.

### Model Preview window

Add a Development editor window named consistently with current terminology, preferably **Model Preview**.

It must participate in the current View menu/workspace/layout persistence conventions:

- toggleable like existing editor windows;
- persistent open/closed state/pose according to current workspace behavior;
- Reset Editor Layout restores the project-defined default state consistently.

Choose the default visible/hidden state only after inspecting current workspace conventions; report the choice. Do not redesign the workspace.

### Selection authority

The Preview consumes the selected canonical asset identity from the Content Browser.

It must not create a second selection authority.

When selection changes:

- if the same identity remains selected, reuse current preview resources;
- if a different valid static model is selected, replace preview resources safely;
- if selection becomes empty/invalid/deleted, release or clear model resources and show a useful no-selection state.

Asset selection remains independent of scene/Hierarchy selection.

### Real model rendering

The Preview must load/render the real selected static GLB from the canonical source path or the existing appropriate Development asset path established by M47/M48.1.

Do not display the thumbnail PNG as the model preview.

Reuse M48.1's validated model-loading/rendering path when architecturally appropriate, but separate:

- thumbnail disk cache state;
- interactive preview model/camera state.

The Preview is not runtime level authority and does not change source/cooked/staged authority.

### Bounds and automatic framing

Use actual model bounds. Prefer shared M48.1 framing/bounds helpers if they are cleanly reusable.

Automatic framing must handle:

- centered bounds;
- off-origin bounds;
- non-uniform extents;
- tiny/near-degenerate bounds;
- large bounds;
- finite camera outputs;
- clipping safety.

On first selection/load, frame the complete model with reasonable padding.

`Frame` / `Reset View` must return to a deterministic useful framing based on current model bounds.

Do not modify the asset pivot or source transform.

### Camera interaction

#### Orbit — required

Allow mouse-driven orbit around a sensible target, initially the framed model center unless repository conventions provide a better equivalent.

Orbit must:

- be deterministic and stable;
- avoid pathological pole behavior/gimbal-like flips;
- not affect the main editor viewport camera;
- only capture interaction when appropriate for the Preview region/window.

#### Zoom/dolly — required

Allow zoom/dolly with the current editor-consistent input convention, preferably mouse wheel when the Preview is hovered.

Clamp distance/zoom to safe values derived from model scale/bounds so the camera cannot collapse into invalid state or become unusably distant.

#### Pan — optional

Implement pan only if it is small, natural and consistent with existing editor controls. Its omission is acceptable and must not block M48.2.

Do not add a generalized camera-controller framework solely for this window.

### Frame / Reset View

Provide a clear control, e.g. **Frame** or **Reset View**, that restores:

- target;
- orbit orientation/default orientation;
- useful distance/zoom;
- any optional pan offset;

from current model bounds.

The exact control should fit current editor UI conventions.

### Render target and resizing

Render the model into a preview area appropriate to the current window size using existing raylib/render-target infrastructure where practical.

The Preview should respond sensibly when the window changes size.

Do not recreate the model every time the window size changes.

Recreate/resize only render-target resources that genuinely depend on dimensions, and avoid churn from tiny size oscillations if current ImGui/raylib integration makes that relevant.

Handle zero/minimized/invalid content dimensions safely.

### Lighting and background

Reuse M48.1 rendering conventions where they remain readable at larger preview size, or make the smallest preview-specific improvement justified by actual rendering infrastructure.

Keep lighting/background deterministic and neutral.

Do not build:

- material editor;
- HDRI browser;
- studio-light system;
- shader editor.

### Simple asset information

The Preview may display information already naturally available, such as:

- display name;
- canonical project-relative path;
- asset type;
- model bounds/dimensions.

Do not introduce new authoritative metadata merely to populate the window.

### Resource lifetime

The Preview must not reload the selected GLB every frame.

Maintain explicit state for the currently loaded preview asset/resources.

Expected behavior:

- no selection -> no model resource;
- first valid selection -> load once;
- same selection across frames -> reuse;
- selection changes -> unload old model and load new one;
- selected asset source changes and Refresh makes that relevant -> reload safely;
- selected asset is deleted -> unload/clear safely;
- editor shutdown -> release resources;
- render target resizes independently of model resource lifetime.

Do not leak GPU/model resources.

If loading fails, show a stable fallback/error state and do not retry every frame. A controlled retry on Refresh/source change/reselection is acceptable.

### Refresh

Preserve M48/M48.1 Refresh semantics.

After Refresh:

- if selected identity still exists and source stamp is unchanged, keep/reuse preview resources when safe;
- if the selected source changed, reload it according to the narrow change-detection mechanism already established or naturally available;
- if selection disappears, clear Preview;
- failed load may become retry-eligible according to the chosen policy.

No filesystem watcher.

### Import

Preserve the existing M47/M48 import authority.

After Import:

- catalog/browser behavior remains unchanged;
- if the imported asset becomes selected according to existing M48 behavior, Preview should load it through normal selection flow;
- otherwise Preview remains on current selection;
- no automatic level object;
- no automatic Cook/Stage change;
- no authored-level mutation.

### Delete

Preserve M48 Delete authority and M48.1 thumbnail cleanup.

On Delete cancellation, Preview state must remain unchanged.

On successful deletion of the selected asset:

- release its Preview model/resources;
- clear Preview selection-dependent state naturally when Content Browser selection clears;
- do not touch unrelated Preview/thumbnail/cache resources;
- preserve existing source/cooked/staged deletion semantics.

### Thumbnail interaction

M48.1 thumbnail cache remains independent derived state.

Preview may share low-level rendering/framing helpers with thumbnail generation, but must not require:

- thumbnail PNG existence;
- thumbnail cache metadata;
- thumbnail cache hit;
- thumbnail texture as source content.

Thumbnail failure must not inherently prevent Preview from trying to render the real asset, and Preview failure must not corrupt the thumbnail cache.

### Development / Debug / Release boundaries

#### Development

Owns the Model Preview UI and interactive rendering behavior.

#### Debug

Must configure/build successfully. No Model Preview editor UI is required.

#### Release

Must configure/build successfully. No Model Preview editor UI/resource management is required by runtime.

Keep Development-only dependencies behind current build boundaries. Shared low-level helpers are acceptable only when consistent with existing architecture.

### Authored-level isolation

M48.2 must not modify:

- `LevelDefinition`;
- `workingCopy`;
- `active`;
- `savedSourceBaseline`;
- Modified;
- Dirty;
- Apply/Revert;
- Save;
- Level Format.

Orbit, zoom, Frame, selection changes and Preview rendering are editor visualization state only.

The Preview must not affect the main editor viewport camera or authored transforms.

### Level Format / canonical data

Level Format remains v1.

Do not semantically modify `game/assets/source/levels/level_01.level`.

Final Level 01 remains:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The legacy line `dynamic_box 0 5 0 1 1 1 30` remains absent.

### Required focused tests

Follow actual post-M48.1 repository conventions. Keep rendering-independent state/math testable outside ImGui where practical.

At minimum cover:

1. Content Browser selected asset identity feeds Preview state correctly;
2. no selection produces valid empty Preview state;
3. changing selection requests/replaces the loaded asset exactly when required;
4. same selection does not reload every frame;
5. deleting/clearing selected asset releases/clears Preview state;
6. failed load enters stable fallback and is not retried every frame;
7. controlled retry after Refresh/source change/reselection works according to chosen policy;
8. ordinary bounds produce finite useful framing;
9. off-origin bounds frame around the correct target;
10. non-uniform/tiny/large bounds remain safe;
11. orbit state updates deterministically;
12. orbit clamping/pole handling remains safe;
13. zoom/dolly updates and clamps safely relative to model scale;
14. Frame/Reset restores deterministic default framing;
15. optional pan, if implemented, resets correctly;
16. Preview camera state is independent of main editor viewport camera state;
17. render-target size policy handles valid resize and zero/invalid dimensions safely;
18. model resource lifetime is independent from render-target resize lifetime;
19. Refresh with unchanged source avoids unnecessary model reload;
20. Refresh with changed selected source causes appropriate reload;
21. Import integrates through existing selection/catalog behavior;
22. Delete cancellation leaves Preview intact;
23. successful Delete of selected asset clears/releases Preview;
24. thumbnail cache validity is not required for Preview loading;
25. Preview operations do not mutate authored level state;
26. workspace visibility/layout persistence works according to current conventions;
27. Debug/Release boundaries remain valid.

Do not expose public production APIs solely for tests. Reuse existing test-access patterns if needed.

### Required regressions

Run relevant current M47/M48/M48.1/editor tests, including actual equivalents of:

- `StaticGlbImportTest`;
- `ContentBrowserTest`;
- `ContentBrowserThumbnailTest`;
- `StaticModelCatalog` tests;
- `EditorToolRunnerTest`;
- `EditorWorkspaceTest`;
- editor layout persistence tests;
- `AuthoredLifecycleIntegrationTest`;
- `LevelFileTest`;
- `CanonicalSceneCleanupTest`;
- `CookStageReloadWorkflowTest`;
- editor selection/picking/gizmo/orientation tests if shared input/camera paths are touched.

Run current Python tests:

```powershell
python tools/test_import_static_glb.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

### Build validation

Run:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

All must succeed.

Also run `git diff --check`.

### Manual acceptance

Use several static GLBs with visibly different shapes/scales.

#### A. Window/workspace

- Open/close Model Preview through current View/workspace UI.
- Confirm persistence follows current editor conventions.
- Confirm Reset Editor Layout behaves consistently.

#### B. Selection

- Select asset A in Content Browser; Preview shows A.
- Select B; Preview replaces A with B.
- Clear/remove selection; Preview shows a useful no-selection state.
- Scene/Hierarchy selection remains independent.

#### C. Real model rendering

- Confirm Preview renders the real model at larger size, not an enlarged thumbnail.
- Confirm it can work independently of thumbnail cache state.

#### D. Framing

- Test centered, off-origin, tall/wide and differently scaled models.
- Confirm complete model is visible without obvious clipping.
- Use Frame/Reset and confirm useful deterministic framing returns.

#### E. Orbit

- Orbit around model with intended input.
- Confirm smooth/stable behavior.
- Confirm no pole flip/pathological camera state.
- Confirm main viewport camera does not move.

#### F. Zoom/dolly

- Zoom in/out.
- Confirm useful clamps.
- Confirm Frame/Reset restores useful distance.

#### G. Pan, if implemented

- Confirm controls are natural and isolated to Preview.
- Confirm Reset clears pan offset.

#### H. Resize

- Resize Model Preview window substantially.
- Confirm rendering adapts.
- Confirm model is not reloaded simply because the window resized.
- Confirm minimized/tiny content does not break rendering.

#### I. Refresh/source change

- Refresh unchanged asset and confirm no visible unnecessary reload/churn.
- Safely replace/update a disposable selected source GLB.
- Refresh and confirm Preview updates according to implemented source-change rule.

#### J. Import

- Import a static GLB through existing workflow.
- Select it and confirm Preview loads normally.
- Confirm no level object is created.

#### K. Delete

- Cancel deletion of selected previewed asset: Preview remains.
- Delete for real: Preview clears/releases safely when selection disappears.
- Unrelated assets/thumbnails remain correct.

#### L. Failure

- Exercise a safe preview-load failure fixture/case if practical.
- Confirm stable fallback/error state.
- Confirm no per-frame error spam/reload loop.

#### M. Authored-state isolation

Throughout all Preview operations confirm:

- no Modified/Dirty;
- no Apply/Revert change;
- no Save-state change;
- no LevelDefinition change;
- no scene object creation;
- no authored transform changes.

#### N. Repository safety

Before approval:

- no disposable test assets remain unintentionally;
- no new preview-derived files are staged unless intentionally part of code/docs;
- thumbnail cache remains non-versioned;
- canonical Level 01 remains unchanged;
- `git diff --check` passes.

### Documentation

Update only affected current documentation. Document the architecture actually chosen:

- Model Preview window/workspace behavior;
- selection authority;
- real-model rendering path;
- what M48.1 infrastructure was reused/refactored;
- bounds/framing;
- orbit/zoom controls;
- optional pan if present;
- Frame/Reset;
- render-target resizing;
- resource lifetime/reload policy;
- Refresh/Import/Delete interactions;
- thumbnail-cache independence;
- Development-only boundary;
- limitations.

Do not document M49+ as implemented.

### Explicit non-goals

M48.2 does not implement:

- Static Props;
- Generic Scene Objects;
- viewport placement;
- drag-and-drop to level;
- double-click instantiate;
- ghost placement;
- asset transformation gizmos;
- mesh editing;
- pivot editing;
- material editing;
- texture editing;
- shader editing;
- animation;
- generic Asset Editor;
- generic property system;
- new asset formats;
- GUIDs;
- Grab/Carry;
- Pressure Plate;
- Inventory;
- M49 functionality.

Avoid unrelated refactors.

### Completion criteria

M48.2 is ready for manual acceptance when:

- a persistent/toggleable Development Model Preview window exists;
- it follows Content Browser asset selection;
- it renders the real selected static GLB;
- it does not depend on thumbnail PNG/cache validity;
- automatic bounds-based framing works;
- orbit works;
- zoom/dolly works with safe limits;
- Frame/Reset works;
- pan is either cleanly implemented or intentionally omitted;
- selection changes/clears replace/release resources correctly;
- model resources are not reloaded every frame;
- render-target resize does not reload the model;
- Refresh/Import/Delete behave correctly;
- authored-level state remains isolated;
- no Static Props/placement are introduced;
- focused tests pass;
- M47/M48/M48.1 regressions pass;
- Python tests pass;
- Debug/Development/Release build;
- canonical Level 01 remains unchanged;
- `git diff --check` passes;
- Cursor provides the required detailed report.

M48.2 is CLOSED only after automated validation + explicit manual approval + Git closure + clean synchronized `main`.

### Cursor final report requirements

Report:

1. repository infrastructure discovered before architecture choice;
2. root implementation approach;
3. exact files changed;
4. Model Preview window/workspace integration;
5. selection authority/data flow from Content Browser;
6. actual model loading/rendering path;
7. M48.1 infrastructure reused/refactored and what remained thumbnail-specific;
8. bounds source/computation;
9. automatic framing algorithm;
10. orbit implementation/input/clamps;
11. zoom/dolly implementation/input/clamps;
12. pan decision/implementation;
13. Frame/Reset behavior;
14. lighting/background;
15. render-target sizing/recreation policy;
16. model/resource lifetime and reload policy;
17. source-change detection/retry/failure behavior;
18. no-selection/fallback behavior;
19. Refresh interaction;
20. Import interaction;
21. Delete interaction;
22. thumbnail-cache independence;
23. simple asset information shown;
24. asset-selection vs scene-selection isolation;
25. authored-level isolation;
26. tests added/changed;
27. focused Preview/camera/resource tests;
28. M47/M48/M48.1/editor regressions;
29. Python results;
30. Debug/Development/Release builds;
31. canonical Level 01 verification;
32. asset/cache/repository safety;
33. `git diff --check`;
34. remaining risks/limitations.

### STOP condition

After implementation, tests, regressions, builds, repository/canonical-data verification, documentation and final report:

- Do **NOT** commit.
- Do **NOT** push.
- Do **NOT** merge.
- Do **NOT** start M49.
- Do **NOT** declare M48.2 CLOSED.
- **STOP.**

M48.2 then waits for mandatory manual user acceptance and Git closure.

### Implementation notes (post-inspection)

Reusable from M48.1: `ValidateStaticGlbFile`, raylib `LoadModel` / `GetModelBoundingBox` / `LoadRenderTexture` / `DrawModel`, source stamp (`mtime` + `size`), and bounds/framing math (moved to `editor/StaticModelFraming.h`). Thumbnail PNG/cache/GPU textures remain thumbnail-specific.

Preview: `render::StaticModelPreviewRenderer` keeps one loaded `Model` and a resizeable `RenderTexture`. Selection authority is `ContentBrowserState.selectedIdentity`. Orbit is LMB; zoom is wheel; pan is omitted. Default window visibility is **true** (same as Content Browser / Object Palette). Name/type/identity/bounds are in a collapsed-by-default **Asset Details** `CollapsingHeader` below the interactive preview (Correction 1).
