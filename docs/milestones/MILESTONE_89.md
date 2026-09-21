# Milestone 89 --- Content Browser Asset Library & Texture Import

## Status

**IMPLEMENTED — awaiting manual acceptance**

Milestone 88 — Terrain Materials Foundation is CLOSED. `main` was clean
and synchronized before M89 started.

## Branch

`milestone/89-content-browser-asset-library`

## Recommended Cursor model

**Grok 4.6 High --- Fast OFF**

## Purpose

Evolve the existing Content Browser into the first unified Development
asset library. Models and standalone Textures become first-class
browsable assets, with explicit Import Texture, type filtering, texture
thumbnails, Favorites, and user-created logical folders/subfolders.

Organization is editor/project metadata only: reorganizing an asset must
not move physical source/cooked files, change runtime/authored asset
identity, rewrite Levels, or break references. This foundation is
intended for future Terrain Material Painting, Vegetation/Foliage,
Characters, NPCs and Enemies, but none of those future systems are
implemented here.

## Existing behavior to preserve

Preserve existing Content Browser model discovery, `StaticModelCatalog`,
GLB import, refresh/delete/search, GLB thumbnail cache, Thumbnails/List
modes and layout persistence. Preserve M84/M88 texture/runtime
conventions, including `textures/<file>.png`,
`runtime_png.max512.lanczos.v1`, `platform::RuntimeAssetPath`,
dependency-driven staging, Development/Release boundaries, Terrain
Materials, Terrain Sculpt and canonical Level safety.

## Content Browser structure

Provide at minimum: - All Assets - Favorites - Models - Textures -
Folders with user-created logical folders/subfolders

`All Assets` shows all supported first-class assets. Models and Textures
are type filters. Favorites is a virtual collection. Logical folders may
contain mixed Models and Textures.

Do not add empty future asset categories before those first-class
concepts exist.

## Asset kinds and identities

Recognize at least Model and Texture through a small typed editor
representation. Reuse existing model identities and M88
`textures/<file>.png` identities. Do not add GUIDs, ECS, a global asset
database, or generalized registry/reflection infrastructure.

## Logical folders

Logical folders are independent of physical paths. Example:
`textures/grass.png` may be organized under `Environment/Flora/Grass`
without changing its identity or moving source/cooked files.

Support Create Folder, Create Subfolder, Rename, safe Delete,
move/reassign an asset to a folder, and move back to root/unfiled.
Validate names deterministically; reject empty names, separators in a
component, sibling collisions and invalid/reserved names. Define case
rules. Prevent cycles.

Deleting a logical folder must never physically delete its assets.
Choose one simple safe deterministic rule after repository inspection
(for example require empty, or reparent contents) and test/document it.

## Favorites

Any supported asset can be favorited/unfavorited. Favorites persist
across editor restart, do not duplicate/move assets, do not alter
identity, do not affect cook/staging, coexist with folders, and never
mark a Level Dirty.

## Organization metadata

Persist folders, asset-to-folder assignments and Favorites separately
from Level data. Use deterministic project-local metadata, no absolute
machine paths, no runtime/Release dependency, safe stale-reference
reconciliation, and no thumbnail data inside it.

Inspect existing editor/project metadata conventions first. Reuse them
if appropriate. If a small new file is needed, keep it narrowly scoped.
Prefer versioned project organization metadata when it represents
intentional shared project structure; machine-local thumbnail caches
remain non-versioned.

Organization metadata is not a runtime asset registry.

## Import UI

Evolve import UI to explicit asset types: `Import -> Import Model...`
and `Import -> Import Texture...`.

Preserve existing GLB import behavior. Do not add future import types.

## Import Texture

Add a real standalone PNG import workflow using the established
platform/file-dialog approach. Validate the selected PNG, derive a valid
destination under the existing `textures/<file>.png` convention,
import/copy it into the established source texture location, reuse the
M88 runtime PNG cook path, refresh the shared texture catalog, and make
it immediately available in both Content Browser and the M88 Terrain
Material selector.

Reuse `runtime_png.max512.lanczos.v1`; do not implement another PNG
cooker. Follow existing model-import transaction/error conventions where
appropriate. Give clear failure feedback and avoid silent
partial/destructive state.

Identity collisions must not silently overwrite an existing texture.

## Unified discovery/catalog

Keep `StaticModelCatalog` rather than replacing it opportunistically.
Add only the smallest analogous/shared Texture catalog needed by Content
Browser, Import Texture refresh, M88 Terrain selection, search/filter
and thumbnails. Avoid divergent independent `source/textures` scans once
the shared catalog exists. Do not build a generalized runtime
AssetManager.

## M88 integration

Replace the M88 Terrain Inspector's independent PNG listing with the
same Development Texture catalog used by Content Browser. Newly imported
textures become selectable without editor restart. Logical
folders/Favorites never alter Terrain's authored `textures/<file>.png`
identity. Existing tiling, workingCopy, Apply, Save, staging and Release
behavior remains unchanged. Do not implement Terrain Material Painting.

## Texture thumbnails

Textures show their actual image as thumbnail in thumbnail mode, with
reasonable aspect preservation. Avoid per-frame decode/upload, leaks,
stale resources and versioned generated thumbnail caches. Refresh after
import/delete/change. Missing/invalid textures get deterministic
placeholder/fallback. Preserve GLB thumbnails and List mode.

## Search/filter interaction

Search combines with current scope. All Assets searches Models+Textures;
Models only Models; Textures only Textures; Favorites only favorites;
logical folder scope searches that folder. Define and document whether
folder scope includes descendants or only direct children.
Type/filter/search/folder navigation never mutates organization or Level
data.

## Asset actions and physical Texture delete

Preserve model actions. For Textures support selection, favorite,
logical move, refresh and explicit physical Delete.

Logical-folder Delete and physical asset Delete are distinct.

Before physical Texture Delete, inspect existing reference handling.
Never silently corrupt the currently loaded Terrain reference. At
minimum refuse deletion when the loaded authored Level/Terrain
references that texture, with actionable feedback. Do not invent a
project-wide dependency graph if none exists; implement the safest
narrow behavior and report limitations. Successful physical deletion
reconciles organization metadata and preview resources.

## Layout persistence

Preserve Thumbnails/List persistence and use established editor-layout
conventions for appropriate Content Browser view state (such as
high-level collection/type filter and current logical folder where
reasonable). Keep project organization metadata conceptually separate
from machine/editor layout state.

## Level authority

Organization actions, favorites, browser navigation, refresh and
importing an unused asset must not modify `workingCopy`, `active`, or
Level Dirty. Assigning a Texture to Terrain remains an M88 authored
operation.

## Runtime/staging boundary

Folders, Favorites, Content Browser state and thumbnail caches are
Development-only. Release must not require them. Staging remains
dependency-driven by authored runtime identities; favoriting or
organizing an asset must never cause it to be staged. Import Texture
must produce assets compatible with the existing M88 runtime path.

## Canonical Level safety

Do not intentionally modify or normalize: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level`

Both diffs must be empty before completion.

## Automated tests

Add focused coverage for: - valid PNG import; - invalid type/identity
rejection; - collision does not silently overwrite; - correct
source/runtime identity and reuse of existing cook path; - catalog
refresh after import/delete; - deterministic Texture catalog
discovery/order; - M88 selector using shared Texture catalog; -
folder/subfolder create, rename, collision rejection, move, root move,
safe delete semantics and persistence; - organization changes do not
change asset identity or Level Dirty; - stale organization metadata
reconciliation; - favorite/unfavorite persistence, identity and no-Dirty
behavior; - All/Models/Textures/Favorites filtering; - search combined
with collection/folder scope; - texture thumbnail lifecycle and existing
GLB thumbnail regressions; - safe physical Texture delete and
metadata/resource cleanup; - editor restart/reload and relevant F2
lifecycle; - existing model import/browser behavior; - M84
texture/material regressions; - M87 Sculpt regressions; - M88 Terrain
Material regressions; - dependency-driven staging and Release
independence from organization metadata.

Do not create a generalized asset-database test framework.

## Manual acceptance

Verify: 1. Existing Models and GLB thumbnails/list mode still work. 2.
Import clearly separates Model and Texture. 3. Import a valid PNG. 4. It
appears immediately in Content Browser with a useful thumbnail. 5. It is
selectable in M88 Terrain Material without editor restart. 6. Assign it
to Terrain and verify tiling/preview/Apply. 7. Create
`Environment/Flora/Grass` and `Environment/Flora/Trees` logical
organization. 8. Organize both a Texture and Model. 9. Confirm their
runtime/authored identities did not change. 10. Rename a folder;
references remain valid. 11. Favorite a Model and Texture; both appear
in Favorites without moving. 12. Unfavorite; only Favorite membership
changes. 13. Test All/Models/Textures/Favorites and search. 14. Test
folder-scoped search. 15. Test Thumbnails/List and persisted browser
state. 16. Delete a logical folder according to defined safe semantics;
physical assets remain. 17. Attempt to delete a Texture referenced by
loaded Terrain; safe refusal/behavior occurs. 18. Delete an unreferenced
disposable Texture and verify refresh/metadata cleanup. 19.
Restart/reopen Development; folders/Favorites persist. 20. Confirm
organization-only actions never Dirty the Level. 21. Confirm Terrain
Sculpt/Material still work. 22. Confirm staged Release has no
organization-metadata dependency.

## Validation

Run:

    cmake --preset windows-vs2022
    cmake --build --preset windows-debug
    cmake --build --preset windows-development
    cmake --build --preset windows-release

Run relevant C++ tests across existing Content Browser/model
import/thumbnails, M84, M87, M88 and new M89 coverage.

Run applicable Python tests including:

    python tools/test_milestone_docs.py
    python tools/test_stage_runtime_assets.py
    python tools/test_cook_level_v1.py
    python tools/test_cook_runtime_png.py
    python tools/test_import_static_glb.py

Add/run focused Texture import/cook tests where appropriate and relevant
shader/staging tests.

Before reporting:

    git branch --show-current
    git status
    git diff --check
    git diff -- game/assets/source/levels/level_01.level
    git diff -- game/assets/source/levels/level_02.level
    git diff --stat

## Out of scope

Terrain Material Painting; multi-layer splat/weight maps;
vegetation/foliage painting; grass/tree scattering; fauna;
Character/NPC/Enemy asset semantics; prefabs; generic material editor;
audio/animation asset browsers; physical relocation of assets as logical
organization; GUID asset database; project-wide dependency graph unless
already available and trivially reusable; asset rename/reference
rewriting; cloud/marketplace; tags beyond Favorites/folders; Undo/Redo;
ECS migration; generalized registry/reflection; Player Traversal Path;
M90+ features.

## Implementation discipline

Inspect repository code/tests/docs before choosing exact UI, catalogs,
file-dialog behavior, metadata format/location/versioning, folder delete
semantics, case rules, descendant search behavior, thumbnail ownership,
Texture delete safety and layout persistence.

Prefer the smallest architecture that evolves the current browser. Do
not replace `StaticModelCatalog` merely for genericity. Do not couple
logical organization to physical paths. Do not start M90 Terrain
Material Painting. If an established convention conflicts with this
conceptual design, preserve correctness and implement the narrowest
coherent alternative, then report it. If a Level Format bump appears
necessary, STOP and report.

## Cursor completion report

Report: 1. files added/changed; 2. exact Content Browser UI; 3.
supported asset kinds; 4. Import Model/Texture workflow; 5. Texture
validation/collision behavior; 6. Texture catalog and M88 selector
reuse; 7. texture thumbnails/resources; 8. logical-folder model and
operations; 9. folder scope/search semantics; 10. Favorites; 11.
organization metadata format/location/versioning; 12. proof organization
does not change asset identity; 13. filters/search; 14. layout
persistence; 15. physical Texture delete/reference safety; 16.
cooker/staging/Release boundaries; 17. Level Dirty/authority behavior;
18. model browser/import/thumbnail regressions; 19. M84/M87/M88
regressions; 20. tests/results; 21. Debug/Development/Release builds;
22. Python results; 23. canonical Level diffs; 24. legitimate Tuning
Backlog candidates; 25. intentionally deferred items.

Then STOP. Do not commit, push, merge, close M89, or begin M90. Manual
acceptance and Git closure remain separate steps.
