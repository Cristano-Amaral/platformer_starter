# Assets

- `source/`: authored inputs. Tracked according to licensing/project policy.
- `source/blender/`: editable Blender files (`.blend`). Authoring only; never cooked; never loaded at runtime.
- `cooked/`: generated runtime-ready output from `python tools/cook_assets.py`. Not source of truth. Normally ignored by Git except `.gitkeep`.

The game loads cooked/runtime-staged files only. It never loads test assets from `source/`.

Milestone 15 test texture:
- logical id: `textures/test_checker.png`
- source: `game/assets/source/textures/test_checker.png`
- cooked: `game/assets/cooked/textures/test_checker.png`
- runtime (after CMake staging): `<executable directory>/assets/textures/test_checker.png`

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
- authoring Base Color: `game/assets/source/textures/test_textured_basecolor.png` (not cooked, not staged, not loaded by the game)
- runtime source: `game/assets/source/models/test_textured.glb` (texture must be embedded in the GLB)
- logical id: `models/test_textured.glb`
- cooked/staged: `<cooked or exe>/assets/models/test_textured.glb` only
- visual-only; model collision is not supported
- see `docs/BLENDER_WORKFLOW.md`
