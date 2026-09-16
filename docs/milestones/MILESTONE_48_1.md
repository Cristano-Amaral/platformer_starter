## Milestone 48.1 — Content Browser Thumbnails

**Status:** Implemented; awaiting mandatory manual acceptance. Not CLOSED.
**Branch:** `milestone/48.1-content-browser-thumbnails`
**Cursor model:** Grok 4.6 High — Fast OFF

### Baseline

M00–M48 are CLOSED. `main` is clean and synchronized.

M47 established the static GLB import/catalog workflow.

M48 established the Development Content Browser with:

- `StaticModelCatalog` as asset authority;
- independent asset selection;
- search/filter;
- explicit Refresh;
- reuse of `Import Static GLB`;
- safe Delete Asset;
- source/cooked/staged authority;
- no Static Props or viewport placement;
- no authored-level state mutation.

Before implementation, inspect the actual post-M48 repository. Current code, tests, and current documentation are the source of truth. Do not assume class names, renderer architecture, cache facilities, GLB loading paths, texture APIs, or editor persistence details before inspection.

### Strategic role

The production workflow remains:

**Import → Visualize/Find → Instantiate → Place → Interact**

M48.1 deepens only the **Visualize/Find** stage.

It upgrades the Content Browser from a primarily textual navigator into a visual asset navigator by adding thumbnails for the static `.glb` assets already represented by `StaticModelCatalog`.

This milestone does not create authored scene instances.

### Objective

Evolve the M48 Content Browser so that:

1. static GLB assets can be browsed visually through generated thumbnails;
2. **Thumbnails** is the default view mode;
3. the user can switch between **Thumbnails** and **List**;
4. the view-mode preference persists consistently with the current editor workspace/layout system;
5. existing M48 search, selection, Refresh, Import, Delete, empty state, and status feedback continue to work;
6. thumbnails are generated/cached efficiently;
7. thumbnail failure produces a stable placeholder/fallback;
8. thumbnail work does not continuously reload/render every model every frame;
9. M47/M48 source/cooked/staged authority and authored-level isolation remain unchanged.

### Architectural constraint: inspect before choosing the thumbnail path

Before settling on the implementation, inspect the actual post-M48 repository and report what reusable infrastructure exists for:

- loading static GLB mesh data;
- obtaining model geometry/bounds;
- creating GPU textures;
- rendering meshes;
- render targets/framebuffers/offscreen passes;
- camera/view/projection helpers;
- lighting/shading;
- image encoding/decoding;
- Development-only resources;
- cache/directory helpers;
- file timestamp or content-change detection;
- editor texture ownership/lifetime.

Do not create a new renderer or generalized asset-preview framework unless inspection proves no smaller reuse path exists.

Prefer the smallest architecture consistent with the current engine.

### Thumbnail generation

A thumbnail must visually represent the corresponding static GLB.

The implementation must define, based on actual repository capabilities:

- how the GLB is loaded for thumbnail generation;
- how model bounds are computed or reused;
- how the camera is positioned/framed automatically;
- camera projection and orientation;
- lighting setup;
- background;
- output resolution;
- output image format, if persisted as an image;
- GPU/CPU resource ownership;
- failure behavior.

#### Bounds and framing

Use current mesh/model data when possible.

The thumbnail must frame the model based on its actual bounds rather than relying on arbitrary asset-specific constants.

The framing should:

- center the model visually;
- include the entire model with reasonable padding;
- behave safely for very small/large dimensions;
- handle degenerate/near-zero extents gracefully;
- avoid clipping through the model.

Do not add authored pivot editing or model-normalization tools.

#### Camera

Use one consistent thumbnail camera setup.

A three-quarter perspective view is preferred if it fits current renderer conventions, but exact angles must follow the minimal implementation after repository inspection.

The camera must be deterministic for a given asset/configuration.

Do not add interactive preview controls.

#### Lighting and background

Use a stable, readable setup with a neutral background and sufficient lighting to understand the shape.

Prefer reuse of current renderer/material/light conventions.

Do not build a full studio-lighting subsystem.

The objective is asset recognition, not photorealistic presentation.

### Thumbnail cache

Prefer a local generated cache that is **not versioned in Git**.

The exact location must be chosen only after inspecting current repository conventions.

Desired properties:

- local to the project/editor environment;
- clearly generated/derived;
- not part of source authority;
- safe to delete/rebuild;
- ignored by Git;
- does not pollute canonical source/cooked/staged asset roots;
- does not become runtime content authority.

#### Cache identity

Thumbnail identity must derive from the canonical project-relative asset identity used by M47/M48.

Do not introduce GUIDs.

The mapping from asset identity to cache entry must:

- be deterministic;
- avoid path traversal;
- avoid collisions;
- be safe across supported filesystem semantics;
- remain separate from user-facing display name.

If hashing is used for filenames, the canonical asset identity remains the authority; the hash is only a cache key.

#### Cache invalidation

Avoid regenerating thumbnails unnecessarily.

A cached thumbnail must be regenerated when the underlying source asset has changed enough that the cached image may be stale.

Choose the narrowest reliable invalidation strategy supported by current repository infrastructure.

Possible inputs may include source modification timestamp, source size, source content hash, thumbnail schema/version, or renderer/config version.

Do not add expensive hashing if current infrastructure already provides a reliable cheaper mechanism.

If a small cache metadata record is needed, it is derived cache data, not a new asset registry.

The final implementation must document the invalidation rule precisely.

#### CACHE VALIDATION SAFETY CLARIFICATION

The chosen invalidation mechanism must not rely on file modification
timestamp alone if the actual filesystem/tooling behavior can reasonably
preserve or reproduce the same timestamp after source replacement.

Prefer the smallest reliable fingerprint available in the current
repository.

The implementation does not need cryptographic content hashing unless
repository inspection shows it is necessary.

Whatever strategy is chosen must correctly distinguish, within the
supported project workflow:

- unchanged source -> cache remains valid;
- changed/replaced source -> cache becomes stale.

Document the known limitations of the chosen fingerprint in the final
Cursor report.

#### Cache cleanup

Do not build an aggressive cache-pruning subsystem unless needed.

If obsolete cache cleanup is simple, safe, and naturally fits Refresh/Delete, it may be implemented narrowly.

At minimum, deleting an asset should ensure its in-memory thumbnail state does not remain active.

If cache-file deletion for the removed asset is safe and straightforward, prefer removing it.

Do not scan/delete arbitrary unrelated files.

### When thumbnails are created

Do not eagerly regenerate every asset every frame.

The implementation should choose the smallest productive policy after inspection, such as lazy/on-demand generation.

Preferred behavior:

- visible/needed thumbnail missing or stale → schedule/generate it;
- valid cached thumbnail → reuse it;
- offscreen/unneeded assets should not cause continuous work;
- repeated frames should reuse loaded thumbnail texture/resources.

If the engine already has an appropriate lightweight queue or job system, reuse it. Do not introduce a generalized asynchronous task framework solely for M48.1.

Synchronous generation is acceptable if bounded and responsive enough for the expected Content Browser scale; otherwise use the smallest existing asynchronous mechanism.

The Cursor report must state whether generation is synchronous or asynchronous and why.

### Import behavior

Preserve M48 import authority.

After successful `Import Static GLB`:

- refresh the catalog as M48 already does;
- the imported asset should appear in the browser;
- thumbnail generation should follow the normal missing-cache policy;
- no automatic scene instantiation;
- no authored-level state mutation;
- no change to M47 cook/stage policy.

Do not duplicate import logic.

### Delete behavior

Preserve M48 Delete Asset authority and path-safety rules.

After successful deletion:

- remove the browser entry after normal refresh;
- release any in-memory thumbnail resource for that asset;
- remove its local derived thumbnail cache entry if safely mapped;
- do not touch unrelated thumbnail cache entries;
- preserve existing source/cooked/staged deletion semantics.

Delete cancellation must not modify cache or filesystem state.

### Refresh behavior

Refresh must preserve M48 catalog behavior.

In addition:

- valid cache entries remain reusable;
- stale cache entries are recognized according to the chosen invalidation rule;
- missing entries can regenerate when needed;
- selection behavior remains unchanged;
- filter behavior remains unchanged;
- Refresh must not synchronously rebuild the entire thumbnail cache unless repository scale/architecture makes that clearly the minimal safe design.

No filesystem watcher.

### View modes

Add two Content Browser view modes:

- **Thumbnails**
- **List**

#### Default

**Thumbnails is the default mode.**

#### Thumbnails mode

Display a simple productive grid of asset cards/items.

Each item should expose at least:

- thumbnail or fallback placeholder;
- display name / filename.

Asset type/path may appear in a tooltip, secondary label, or appropriate compact detail if useful.

Selection must be obvious.

The grid should adapt reasonably to available Content Browser width.

Do not add zoom sliders, masonry layout, folders, multi-select, drag/drop, or model preview.

#### List mode

Preserve the useful M48 textual/table view behavior.

At minimum retain:

- filename/display name;
- asset type;
- canonical project-relative identity/path;
- selection;
- search/filter;
- import;
- refresh;
- delete.

Do not regress the M48 list view.

#### Switching modes

Provide a simple toolbar/control to switch between Thumbnails and List.

Switching modes must preserve selection and current search query where possible, avoid unnecessary reload/regeneration, and never mutate authored-level state.

### View-mode persistence

Persist the Thumbnails/List preference using the same repository-consistent workspace/layout persistence approach used by M48.

Requirements:

- fresh/default editor state opens in Thumbnails mode;
- switching to List persists across the same kind of editor restart/layout persistence currently supported;
- Reset Editor Layout restores the project-defined default behavior consistently;
- persistence remains Development tooling state, not LevelDefinition data;
- no Level Format changes.

### Placeholder/fallback

If thumbnail generation, cache loading, texture upload, or asset preview loading fails:

- Content Browser remains usable;
- show a stable visual placeholder/fallback;
- asset remains selectable/searchable/deletable;
- failure is reported in Tool Output/logging as appropriate without spamming every frame;
- do not repeatedly retry every frame.

Define a reasonable retry policy, such as retry only after Refresh/source change/editor session, depending on the minimal architecture chosen.

### Zero-assets behavior

Zero assets remains a valid state.

In Thumbnails mode, show the same kind of useful empty-state guidance introduced in M48.

Do not generate placeholder sample assets.

### Performance expectations

The Content Browser must not:

- load every GLB every frame;
- render every GLB every frame;
- upload the same thumbnail texture every frame;
- regenerate valid cached thumbnails continuously;
- perform unbounded repeated failure work every frame.

Use cache/in-memory state to make steady-state browsing cheap.

Do not prematurely create a global asset streaming system, generalized render job scheduler, generalized content-addressable cache, or generalized thumbnail service for future asset types.

### Development / Debug / Release boundaries

#### Development

Owns Content Browser UI, thumbnail grid, thumbnail generation/cache consumption, and view-mode switching/persistence.

#### Debug

Must still configure/build successfully. Do not introduce Development ImGui thumbnail UI into Debug.

#### Release

Must still configure/build successfully. No Content Browser UI or Development thumbnail management should be required by Release runtime.

### Authored-level isolation

M48.1 must not mutate:

- `workingCopy`;
- `active`;
- `savedSourceBaseline`;
- Modified;
- Dirty;
- Apply/Revert state;
- Save state.

Thumbnail generation/cache operations are editor-derived asset visualization operations.

Asset selection remains independent from scene/Hierarchy selection.

### Level Format and canonical data

Level Format remains v1.

No new records.

Canonical `game/assets/source/levels/level_01.level` must remain semantically unchanged:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The legacy line `dynamic_box 0 5 0 1 1 1 30` remains absent.

### Required focused automated tests

Follow actual post-M48 test conventions. At minimum test the relevant non-ImGui/model boundaries for:

1. deterministic cache identity from canonical asset identity;
2. cache path safety;
3. valid cache hit avoids regeneration;
4. missing cache causes generation request/work;
5. changed source invalidates cache according to chosen rule;
6. unchanged source does not invalidate;
7. thumbnail schema/config version invalidates if such a version is used;
8. failed generation enters stable fallback/failure state;
9. failed item is not retried every frame;
10. Refresh can permit a sensible retry/invalidation;
11. Import makes a new asset eligible for thumbnail generation;
12. Delete releases/removes mapped thumbnail state/cache safely;
13. unrelated cache entries remain untouched;
14. zero-assets state works;
15. Thumbnails is default view mode;
16. switching to List works;
17. switching back to Thumbnails works;
18. view-mode persistence round-trip works;
19. reset/default layout returns to Thumbnails;
20. selection persists across view-mode changes;
21. search/filter behaves identically in both modes;
22. browser operations remain independent from scene selection;
23. authored-level state remains unchanged;
24. Debug/Release build boundaries remain valid.

If visual raster correctness cannot reasonably be asserted in existing unit-test infrastructure, test deterministic generation inputs, bounds/framing math, cache state, and failure behavior outside ImGui.

### Bounds/camera focused tests

If new bounds/framing helpers are introduced, add focused tests for:

- ordinary centered model bounds;
- off-origin model bounds;
- non-uniform extents;
- tiny/near-zero extents;
- large extents;
- deterministic camera target/distance;
- near/far plane safety if computed;
- finite/non-NaN outputs.

Do not expose production API solely for tests. Use existing test-access patterns if needed.

### Required regressions

Run relevant current M47/M48 and editor tests, including actual equivalents of:

- `StaticGlbImportTest`;
- `ContentBrowserTest`;
- `StaticModelCatalog` tests;
- `EditorToolRunnerTest`;
- `EditorWorkspaceTest`;
- layout persistence tests;
- `AuthoredLifecycleIntegrationTest`;
- `LevelFileTest`;
- `CanonicalSceneCleanupTest`;
- `CookStageReloadWorkflowTest`;
- editor selection/picking/gizmo tests if shared UI/selection paths are touched.

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

Also run:

```powershell
git diff --check
```

### Required manual acceptance

Use a small set of compatible static GLBs with visibly different silhouettes.

#### A. Default thumbnails mode

1. Launch Development with default/reset layout state.
2. Open/observe Content Browser.
3. Confirm **Thumbnails** is the default mode.
4. Confirm assets appear as thumbnail cards.
5. Confirm each visible asset has a readable filename.
6. Confirm selection is visually clear.

#### B. Thumbnail quality

For multiple models, confirm full framing, no obvious clipping, consistent orientation/view, readable lighting/background, and placeholder behavior on failure.

#### C. View switching

Switch to List and back. Confirm M48 textual data remains correct, search/selection are preserved, and returning to Thumbnails does not trigger unnecessary full regeneration.

#### D. Persistence

Switch to List, restart according to current persistence behavior, confirm List persists, then Reset Editor Layout and confirm the default returns to Thumbnails.

#### E. Search/filter

Test the same case-insensitive searches in both modes. Results must match M48 behavior.

#### F. Cache reuse

Generate thumbnails, restart/reopen editor, and confirm valid thumbnails are reused rather than regenerated unnecessarily.

#### G. Cache invalidation

Using a disposable asset, generate its thumbnail, safely replace/update the source GLB, Refresh/reopen as required, and confirm the stale thumbnail is regenerated.

#### H. Import integration

Import Static GLB and confirm the asset appears, its thumbnail is produced by the normal policy, no level object is created, and the level remains unmodified.

#### I. Delete integration

Cancel Delete once and confirm nothing changes. Then delete the asset and confirm the browser entry and corresponding thumbnail state/cache disappear while unrelated thumbnails remain.

#### J. Performance sanity

With several assets visible, idle browsing must not continuously regenerate thumbnails; switching views must not reload/render every asset each frame; scrolling should remain responsive at current project scale.

#### K. Authored-state isolation

Throughout all tests confirm no Modified/Dirty, Apply/Revert, Hierarchy/viewport scene-selection, or Static Prop regressions.

#### L. Repository safety

Before approval:

- no generated thumbnail cache is staged for Git;
- local cache location is ignored appropriately;
- no disposable manual test assets remain unless intentionally kept;
- canonical Level 01 remains unchanged;
- `git diff --check` passes.

### Documentation

Update only affected current docs. Document:

- Thumbnails/List modes;
- Thumbnails default;
- thumbnail generation architecture actually chosen;
- bounds/camera/lighting/background behavior;
- cache location and non-authoritative nature;
- cache identity/invalidation policy;
- Import/Delete/Refresh interactions;
- fallback behavior;
- Development-only boundary;
- current limitations.

Do not document M49+ as implemented.

### Explicit non-goals

M48.1 does not implement:

- Static Props;
- Generic Scene Objects;
- viewport placement;
- drag-and-drop to viewport;
- double-click instantiate;
- ghost placement;
- new Level Format records;
- GUIDs;
- asset tags;
- favorites;
- recents;
- generalized asset metadata;
- rename/move;
- new import formats;
- interactive 3D Model Preview Editor;
- generic thumbnail service for future asset types;
- Grab/Carry;
- Pressure Plate;
- Inventory;
- M49 functionality.

Avoid unrelated refactors.

### Completion criteria

M48.1 is ready for manual acceptance when:

- Content Browser defaults to Thumbnails;
- List mode remains available and functional;
- view-mode switching is persistent;
- thumbnails are visually useful and automatically framed;
- cache is local/derived/non-versioned;
- cache identity maps deterministically from canonical asset identity;
- valid cache entries are reused;
- stale entries regenerate according to a documented rule;
- failures produce a stable fallback;
- browser steady state avoids continuous model loading/rendering;
- Import/Delete/Refresh/search/selection from M48 remain correct;
- authored-level state remains isolated;
- no Static Props/placement are introduced;
- focused tests pass;
- M47/M48 regressions pass;
- Python tests pass;
- Debug/Development/Release build;
- canonical Level 01 remains unchanged;
- thumbnail cache is not accidentally staged;
- `git diff --check` passes;
- Cursor provides the required detailed report.

M48.1 is CLOSED only after automated validation + explicit manual approval + Git closure + clean synchronized `main`.

### Cursor final report requirements

Report:

1. repository infrastructure discovered before architecture choice;
2. root implementation approach;
3. exact files changed;
4. thumbnail generation path;
5. model loading path reused;
6. bounds source/computation;
7. camera framing algorithm;
8. lighting/background;
9. thumbnail resolution/format;
10. cache location;
11. cache key/identity mapping;
12. cache metadata if any;
13. invalidation rule;
14. creation policy: lazy/eager/synchronous/asynchronous and why;
15. in-memory texture/resource reuse;
16. failure/fallback/retry behavior;
17. Import interaction;
18. Delete interaction and cache cleanup;
19. Refresh interaction;
20. Thumbnails/List UI behavior;
21. default mode;
22. persistence/reset behavior;
23. asset-selection vs scene-selection isolation;
24. authored-level isolation;
25. tests added/changed;
26. focused thumbnail/cache tests;
27. M47/M48/editor regressions;
28. Python results;
29. Debug/Development/Release builds;
30. canonical Level 01 verification;
31. cache/Git-ignore verification;
32. asset-tree safety;
33. `git diff --check`;
34. remaining risks/limitations.

### STOP condition

After implementation, tests, regressions, builds, cache/Git safety checks, canonical-data verification, documentation, and final report:

- Do **NOT** commit.
- Do **NOT** push.
- Do **NOT** merge.
- Do **NOT** start M49.
- Do **NOT** declare M48.1 CLOSED.
- **STOP.**

M48.1 then waits for mandatory manual user acceptance and Git closure.

### Implementation notes (post-inspection)

Inspection found no existing offscreen pass, thumbnail cache, GLB mesh loader in project code, or content-addressable asset store. `StaticGlb` validates containers only. The runtime renderer does not load catalog GLBs. Editor persistence already uses `%LOCALAPPDATA%\Platformer3D\` (`editor_layout.ini`, `editor_build_selection.txt`).

Chosen smallest path:

- Reuse raylib `LoadModel` / `GetModelBoundingBox` / `LoadRenderTexture` / `DrawModel` / `ExportImage` in `render::StaticModelThumbnailStore` (Development authoring only at the call site).
- 128×128 PNG cache under `%LOCALAPPDATA%\Platformer3D\thumbnails\`, keyed by FNV-1a 64 of the canonical identity `models/<file>.glb`.
- Meta: `schema=1`, identity, source mtime ticks, size. No content hash.
- Camera: three-quarter `normalize(1, 0.85, 1)`, perspective FOV 40°, padding 1.2 around bounds radius.
- Background `{56,60,72}`; lighting is `DrawModel(..., WHITE)`.
- Lazy synchronous generation, at most one GLB per frame; in-memory GPU texture reused while stamp matches.
- Failure placeholder; retry on Refresh (`AllowRetryAll`) or source stamp change.
- View modes Thumbnails (default) and List; persist `editor_content_browser_view.txt`; Reset Editor Layout restores Thumbnails.
