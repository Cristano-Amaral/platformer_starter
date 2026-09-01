# Assets

- `source/`: authored inputs. Tracked according to licensing/project policy.
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
