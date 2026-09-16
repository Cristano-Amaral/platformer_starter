## Milestone 48 — Content Browser / Asset Browser v1

**Status:** Implemented; awaiting manual acceptance.
**Branch:** `milestone/48-content-browser`

### Implemented (this branch)

Development F2 **Content Browser** is a view/controller over M47
`StaticModelCatalog`. It lists registered static GLB assets (filename,
`static_glb`, canonical `models/<file>.glb`), supports case-insensitive
search, Refresh, Import Static GLB (same `LevelEditorRequest::ImportStaticGlb`
authority), and confirmed Delete Asset.

Asset selection lives on `ContentBrowserState.selectedIdentity` and is
not Hierarchy/viewport selection. Import, refresh, search, and delete do
not mutate `workingCopy` / `active` / `savedSourceBaseline` / Modified /
Dirty.

Delete authority is `assets::DeleteStaticModel`. Identity must parse as
`models/<safe.glb>`. Source/cooked/staged paths are joined under explicit
absolute roots. Generated copies are removed first; source last. Missing
cooked/staged counterparts are not errors. Cancellation never calls
delete. No Static Props, placement, thumbnails, or Level Format change.

Focused coverage: `ContentBrowserTest`. M48 is not complete until manual
acceptance.

### Baseline

M00–M47 are CLOSED. `main` is clean and synchronized.

M47 established the official static-GLB import workflow, canonical source models, project-relative model identity, a derived `StaticModelCatalog`, and existing source → cooked → staged authority. Import does not instantiate a level object.

Before implementation, inspect the actual post-M47 repository. Current code, tests, and current documentation are the source of truth. Preserve actual current behavior if names/details differ from this planning document.

### Strategic role

The production workflow is progressing as:

**Import → Visualize/Find → Instantiate → Place → Interact**

M47 implemented **Import**. M48 implements **Visualize/Find and basic asset management**.

M48 must make registered static-model assets visible and manageable inside the Development editor, but must **not instantiate them into the level**. The planning screenshot is only a workflow/productivity reference, not a UI or architecture specification.

### Objective

Create a Development-only **Content Browser / Asset Browser v1** using the M47 catalog as authority, allowing the user to:

1. see available registered static GLB assets inside the editor;
2. identify them by useful human-readable information;
3. select an asset;
4. search/filter the catalog;
5. refresh after import/external changes;
6. invoke the existing M47 import workflow from a natural asset-management surface;
7. remove an asset through an explicit, safe project operation;
8. receive clear success/failure feedback;
9. do all this without modifying level authored state.

### Architectural decisions

#### Catalog remains authority

The Content Browser is a view/controller over the M47 catalog, not a second asset database. No GUIDs and no duplicated browser-owned registry. Small repository-consistent extensions to `StaticModelCatalog` are allowed if needed for refresh/querying, but keep them independent of ImGui.

#### Development-only

The browser belongs to Development editor tooling. Debug/Release must not expose or depend on browser UI. Runtime loading remains unchanged.

#### Static GLB scope remains

Browse only asset types actually supported by M47, initially static GLB/static models. Showing an asset type field is appropriate; broadening import pipelines is not.

#### Asset selection != scene selection

Selecting an asset means selecting a reusable project resource. It must not create a LevelDefinition object, select a Hierarchy object, place anything, mutate `workingCopy`, `active`, `savedSourceBaseline`, or set level Modified/Dirty.

#### No placement

No viewport drag/drop, double-click placement, ghost preview, Static Prop creation, or Add-to-Level action. Those belong to M49/M50.

#### Basic asset deletion belongs in M48

M48 adds an official removal workflow while no Static Prop references exist yet.

A successful Delete Asset operation must:

- target a selected canonical registered asset;
- require explicit confirmation;
- remove the canonical source asset;
- remove only its corresponding cooked/staged generated artifacts when present;
- refresh the catalog/browser;
- report success/failure clearly.

No arbitrary filesystem deletion, no paths outside authorized roots, and no future dependency/reference system.

##### DELETE SOURCE-OF-TRUTH CLARIFICATION

Delete Asset must treat the canonical SOURCE asset as the authored
project authority.

Before deleting anything, validate all source/cooked/staged mappings.

The operation must never reach outside the exact authorized roots for
the selected canonical asset identity.

If deleting generated counterparts fails, do not improvise recovery,
delete neighboring files, or broaden the operation.

The implementation must define and test a deterministic partial-failure
policy based on the actual repository filesystem behavior.

In particular, the final Cursor report must state clearly:

- whether source deletion occurs before or after generated cleanup;
- what happens if cooked deletion fails;
- what happens if staged deletion fails;
- what catalog state is exposed after any partial failure;
- whether the operation is retry-safe.

Do not claim filesystem transactionality if the underlying implementation
does not actually provide it.

#### Generated-artifact cleanup

Inspect actual M47 cooker/stager mapping first. Define one narrow deletion authority that understands current source/cooked/staged mapping. Do not leave stale generated copies that can masquerade as valid runtime content. Missing generated counterparts are acceptable. Document/test partial-failure semantics conservatively.

#### Presentation

At minimum show:

- display name / filename;
- asset type;
- canonical project-relative path.

Do not invent gameplay categories such as Pickup, Grab/Carry, Pressure Plate, etc.

#### Search/filter v1

Provide simple case-insensitive text filtering over useful existing fields, at minimum filename/display name and/or canonical path. No tags, favorites, smart collections, saved searches, or advanced query language.

#### List/grid and thumbnails

Use the smallest productive ImGui presentation consistent with the current editor. A selectable list/table/grid is acceptable.

M48 does **not require rendered thumbnails**. If no safe reusable thumbnail/preview infrastructure exists, use a clean textual/icon-like placeholder presentation. Do not create an offscreen renderer or thumbnail cache merely for M48.

### Editor integration

Add a persistent/toggleable Development editor window named consistently with current UI, preferably **Content Browser** unless existing terminology favors **Asset Browser**.

Integrate with existing menu/window persistence conventions. Provide:

- catalog entries;
- selection;
- search/filter;
- Refresh;
- Import Static GLB access/reuse;
- Delete/Remove selected asset;
- useful status/empty-state messaging.

Reuse M47 import authority and existing Tool Output/status infrastructure where appropriate.

### Import integration

The browser makes importing convenient but M47 remains import authority. After successful import, refresh automatically if practical so the asset becomes visible without restarting Development. Do not auto-place or auto-cook/stage. Existing **Assets > Import Static GLB** must continue working.

### Refresh behavior

Provide explicit Refresh. It must re-discover valid canonical source assets, preserve selection if the same canonical identity still exists when simple/safe, clear selection if it disappeared, and update filtered results deterministically. No filesystem watcher/hot-reload service.

### Delete Asset behavior

Expected flow:

1. select asset;
2. invoke Delete/Remove;
3. show explicit confirmation naming it;
4. validate canonical identity/path;
5. remove only authorized source/cooked/staged files mapped to it;
6. refresh catalog;
7. clear selection if deleted;
8. report result.

Cancellation makes no filesystem changes.

Reject no selection, malformed/noncanonical identity, traversal/out-of-root mapping, unsupported type, arbitrary-file deletion, or any mapping escaping authorized roots.

No recycle bin, undo, trash folder, restore, batch delete, multi-select delete, dependency graph, or force-delete.

### Cook/stage relationship

M48 preserves M47 policy: **Import does not automatically cook/stage.** Existing global Cook Assets / Stage Runtime Assets / Cook & Stage / Cook Stage & Reload remain authoritative.

Per-asset cook buttons or cooked/staged badges are not required. Deletion must safely clean corresponding generated counterparts so removed content is not resurrected later.

### Level/editor authority

Browser operations must not mutate `workingCopy`, `active`, `savedSourceBaseline`, level Modified/Dirty, Apply/Revert, or Save state. Asset selection remains independent from Hierarchy/viewport scene selection. Level Format remains v1.

### Window/layout persistence

Follow existing Development workspace/window persistence. If current editor windows persist open/closed state, Content Browser should participate consistently. Do not redesign workspace architecture.

### Empty state

Zero supported assets is valid. Show useful guidance to import a Static GLB without errors or fake placeholder assets.

### Canonical-data safety

M48 must not semantically modify `game/assets/source/levels/level_01.level`.

Final Level 01 remains:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The legacy `dynamic_box 0 5 0 1 1 1 30` remains absent.

Automated asset-management tests should use temporary directories/fixtures. Do not leave manual-test assets or generated artifacts unintentionally committed.

### Required automated tests

Follow actual post-M47 conventions. At minimum prove:

1. browser/query model exposes valid catalog entries deterministically;
2. filename/type/canonical identity are available;
3. case-insensitive search/filter works;
4. unmatched filter yields a valid empty result;
5. refresh discovers a newly added/imported valid asset;
6. refresh removes an absent asset;
7. selection survives refresh if identity still exists, if implemented;
8. selection clears safely if identity disappears;
9. successful M47 import becomes visible through refresh without restart;
10. browser import dispatch reuses M47 authority;
11. deletion rejects no-selection/invalid identity safely;
12. deletion rejects traversal/out-of-root mapping;
13. deletion cancellation makes no filesystem changes at the testable authority boundary;
14. successful deletion removes canonical source;
15. successful deletion removes only corresponding cooked/staged artifacts when present;
16. missing generated counterparts do not cause unrelated deletion;
17. neighboring assets remain untouched;
18. catalog no longer exposes deleted asset after refresh;
19. browser/import/delete operations do not mutate authored level state;
20. zero-assets state is valid;
21. Debug/Release remain free of browser UI dependencies as appropriate.

Keep filesystem authority tests outside ImGui where practical.

### Required regressions

Run relevant current tests for M47 import/catalog, `EditorToolRunner`, editor workspace/layout persistence, menu behavior if modified, authored lifecycle integration, LevelFile, CanonicalSceneCleanup, CookStageReloadWorkflow, and editor selection/picking/gizmo tests if shared selection code is touched.

Run current relevant Python tests, including equivalents of:

```powershell
python tools/test_import_static_glb.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Add focused deletion/cook-stage cleanup tests if Python/CMake tooling changes.

### Build validation

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

All configurations must succeed. Also run `git diff --check`.

### Required manual acceptance

#### A. Browser visibility
- Launch Development and open Content Browser.
- Confirm registered static models appear.
- Confirm filename/display name, type, and canonical path are understandable.
- Select different assets and confirm stable selection.

#### B. Search/filter
- Search with part of an asset filename/path using different casing.
- Confirm expected match.
- Enter unmatched text and confirm clean empty result.
- Clear search and confirm full catalog returns.

#### C. Import integration
- Invoke Import Static GLB from the browser.
- Import a compatible GLB.
- Confirm normal M47 validation/success feedback.
- Confirm it appears without restarting Development.
- Confirm it is not placed in the level.

#### D. Refresh
- Use Refresh and confirm catalog correctness and sensible deterministic selection behavior.

#### E. Delete cancellation
- Select a disposable asset, invoke Delete, cancel, and confirm source/cooked/staged and catalog entry remain unchanged.

#### F. Delete success
- Cook/stage a disposable asset so generated counterparts exist.
- Delete it through Content Browser and confirm.
- Confirm source and corresponding cooked/staged files are removed.
- Confirm unrelated assets remain and browser entry disappears.

#### G. Level/editor isolation
Throughout browser/import/delete/search/refresh, level must not become Modified/Dirty, Apply/Revert/Save remain unchanged, Hierarchy/viewport scene selection remains coherent, and no level object is created.

#### H. Scope boundary
Confirm no viewport placement, Static Prop, scene drag/drop, double-click instantiate, new rendered-thumbnail pipeline, gameplay classification, Grab/Carry, or Inventory behavior.

#### I. Cleanup
Remove disposable manual-test imports, verify no unintended generated/temp artifacts, canonical Level 01 unchanged, and inspect asset-tree diffs.

### Documentation

Update only affected current docs, likely `README.md`, `AGENTS.md`, `docs/ARCHITECTURE.md`, `docs/MILESTONES.md`, `game/assets/README.md`, and `tools/README.md` if deletion affects tooling.

Document Content Browser v1, catalog authority, asset-vs-scene selection, search/filter, refresh, import integration, deletion semantics, generated-artifact cleanup, and limitations. Do not document M49+ as implemented.

### Explicit non-goals

M48 does not implement:

- Static Props / Generic Scene Objects;
- Level Format asset records;
- viewport placement / drag-drop / double-click instantiate / ghost placement;
- prop Rotate/Scale workflow;
- rendered thumbnail generation unless a trivial reusable current facility already exists with no new architecture;
- model preview viewport / thumbnail cache;
- tags/favorites/recents/virtual folders;
- filesystem folder management;
- asset rename/move;
- overwrite/reimport;
- batch import;
- multi-select/batch delete;
- recycle bin/undo delete;
- dependency graph/reference checking for future Static Props;
- GUIDs/prefabs/ECS/components;
- new asset formats;
- Grab/Carry;
- Pressure Plate;
- Game Feature Configuration;
- Inventory;
- M49 work.

### Completion criteria

M48 is ready for manual acceptance when Development has a usable Content Browser v1 driven by the M47 catalog; filename/type/path are visible; asset selection is independent of scene selection; search/filter and refresh work; M47 import is reused and newly imported assets become visible; Delete Asset is confirmed/path-safe and cleans corresponding source/cooked/staged artifacts without touching unrelated content; level authored state is unchanged; no placement/Static Prop behavior exists; Level Format remains v1; tests/regressions/Python pass; Debug/Development/Release build; `git diff --check` passes; canonical Level 01 is unchanged; test artifacts are cleaned; docs are current; and Cursor supplies the required report.

Manual acceptance and Git closure remain mandatory. M48 is CLOSED only after automated validation + explicit manual approval + Git closure + clean synchronized `main`.

### Cursor final report requirements

Report:

1. root implementation approach;
2. exact files changed;
3. Content Browser window/menu integration;
4. catalog authority and any M47 catalog changes;
5. browser entry metadata;
6. selection model and separation from scene selection;
7. search/filter semantics;
8. refresh semantics;
9. import integration and M47 authority reuse;
10. delete confirmation flow;
11. delete authority/path-safety rules;
12. exact source/cooked/staged mapping and cleanup behavior;
13. partial-delete/failure semantics;
14. authored-level isolation;
15. workspace persistence behavior;
16. tests added/changed;
17. focused results;
18. M47/editor regressions;
19. Python/tooling results;
20. Debug/Development/Release builds;
21. canonical Level 01 verification;
22. asset-tree/generated-artifact verification;
23. `git diff --check`;
24. remaining risks/limitations.

### STOP condition

After implementation, validation, builds, canonical-data verification, documentation, and detailed report:

- Do **NOT** commit.
- Do **NOT** push.
- Do **NOT** merge.
- Do **NOT** start M49.
- Do **NOT** declare M48 CLOSED.
- **STOP.**

M48 then waits for mandatory manual user acceptance and Git closure.
