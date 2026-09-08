# Assets

- `source/`: authored inputs. Tracked according to licensing/project policy.
- `source/blender/`: editable Blender files (`.blend`). Authoring only; never cooked; never loaded at runtime.
- `cooked/`: generated runtime-ready output from `python tools/cook_assets.py`. Not source of truth. Normally ignored by Git except `.gitkeep`.

The game loads cooked/runtime-staged files only. It never loads test assets from `source/`.

Milestone 15 test texture (standalone **runtime** PNG):
- logical id: `textures/test_checker.png`
- source: `game/assets/source/textures/test_checker.png`
- cooked: `game/assets/cooked/textures/test_checker.png`
- runtime (after CMake staging): `<executable directory>/assets/textures/test_checker.png`
- cooker kind: `runtime_png` (recipe `runtime_png.max512.lanczos.v1`; 16×16 is copied unchanged)
- not every PNG under `source/textures/` is a runtime asset
- cooker test fixture `tools/fixtures/textures/test_large_checker.png` is test-only; not cooked, not staged

Milestone 16 test model:
- logical id: `models/test_static.glb`
- source: `game/assets/source/models/test_static.glb`
- cooked: `game/assets/cooked/models/test_static.glb`
- runtime (after CMake staging): `<executable directory>/assets/models/test_static.glb`
- cooker copies the GLB unchanged
- visual-only technical test asset; model collision is not supported

Milestone 17 authored model:
- authored: `game/assets/source/blender/test_authored.blend`
- runtime source: `game/assets/source/models/test_authored.glb`
- logical id: `models/test_authored.glb`
- cooked: `game/assets/cooked/models/test_authored.glb`
- runtime (after CMake staging): `<executable directory>/assets/models/test_authored.glb`
- cooker copies the GLB unchanged; `.blend` is not a cooker input
- visual-only; model collision is not supported
- see `docs/BLENDER_WORKFLOW.md`

Milestone 18 textured model:
- authored: `game/assets/source/blender/test_textured.blend`
- authoring Base Color: `game/assets/source/textures/test_textured_basecolor.png` (**Blender authoring texture**; not cooked, not staged, not loaded by the game, not a Milestone 19 resize target)
- runtime source: `game/assets/source/models/test_textured.glb` (texture must be embedded in the GLB)
- logical id: `models/test_textured.glb`
- cooked/staged: `<cooked or exe>/assets/models/test_textured.glb` only
- cooker kind: `copy` (opaque GLB; embedded images are not resized in Milestone 19)
- visual-only; model collision is not supported
- see `docs/BLENDER_WORKFLOW.md`

Milestone 47 imported static models:
- destination: `game/assets/source/models/<filename>.glb` (external filename preserved)
- identity: `models/<filename>.glb` (project-relative; never an absolute path)
- catalog: derived from valid `source/models/*.glb`; not a persistent database
- collision: existing destination fails; never overwrite
- cook/stage: extra models are cooked as `copy` and staged from cooked `models/*.glb`
- import does not instantiate a level object

Milestone 48 Content Browser / Delete Asset:
- browser is a view over the M47 catalog; it is not a second registry
- asset selection is not scene/Hierarchy selection and does not mutate Level 01
- Delete Asset is confirmed, identity-based, and limited to authorized source/cooked/staged roots
- successful delete removes canonical source plus matching cooked/staged copies when present
- missing cooked/staged counterparts do not block source deletion
- generated copies are removed first; source last, so a failed generated cleanup is retry-safe
- cancellation makes no filesystem changes
- delete is not Cook/Stage, not a recycle bin, and not a level edit

Milestone 48.1 Content Browser thumbnails:
- derived editor visualization over the same M47 catalog; not a second registry
- default view is Thumbnails; List remains available
- view mode persists as `%LOCALAPPDATA%/Platformer3D/editor_content_browser_view.txt`
- thumbnail cache is `%LOCALAPPDATA%/Platformer3D/thumbnails/` (`{hash}.png` + `{hash}.meta`)
- cache is not source, not cooked, not staged, and not Git-tracked
- invalidation uses schema version 1 plus source last-write time and size
- generation is lazy and synchronous (at most one GLB per editor frame)
- Delete Asset also removes that asset's mapped thumbnail cache entry
- cancellation of Delete does not touch thumbnail cache
