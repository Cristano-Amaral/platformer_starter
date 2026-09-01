# Assets

- `source/`: authored inputs. Tracked according to licensing/project policy.
- `cooked/`: generated runtime-ready output from `python tools/cook_assets.py`. Not source of truth. Normally ignored by Git except `.gitkeep`.

The game loads cooked/runtime-staged files only. It never loads the test texture from `source/`.

Milestone 15 test asset:
- logical id: `textures/test_checker.png`
- source: `game/assets/source/textures/test_checker.png`
- cooked: `game/assets/cooked/textures/test_checker.png`
- runtime (after CMake staging): `<executable directory>/assets/textures/test_checker.png`
