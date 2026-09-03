# Tools

## Python tooling dependency

Cooker-only. Not a CMake or runtime dependency. Not used from C++.

```
python -m pip install -r tools/requirements.txt
```

Pinned in `tools/requirements.txt`:

```
Pillow==12.3.0
```

The cooker does not install packages. If a standalone runtime PNG must be
resized and Pillow is missing, cooking fails with an install reminder.

## Asset cooker
From the repository root:

```
python tools/cook_assets.py
```

Python 3. Paths are resolved from `tools/cook_assets.py`, not the process CWD.
No Blender.

The cooker copies known authored files from `game/assets/source/` to
`game/assets/cooked/` by explicit identity (`textures/test_checker.png`,
`models/test_static.glb`, `models/test_authored.glb`, `models/test_textured.glb`,
`levels/level_01.level`).
It does not glob every PNG under `source/textures/`. Skips a rewrite when the
cooked bytes already match the current cook result, writes
`game/assets/cooked/manifest.json`, and removes only previously manifested
cooked outputs that are no longer in the known asset list.

Asset kinds:

- `copy`: opaque byte copy. Used for GLBs. Embedded images are not inspected
  or resized.
- `runtime_png`: standalone runtime PNG. Recipe `runtime_png.max512.lanczos.v1`
  downscales with Pillow `Image.Resampling.LANCZOS` when either dimension
  exceeds 512 px, preserving aspect ratio and never upscaling. Sources already
  within 512×512 are copied byte-for-byte (no decode/re-encode).
- `level_v1`: UTF-8 level text. Cooker checks the first non-blank line is
  `PLATFORMER_LEVEL 1`, then copies bytes unchanged. Full grammar validation is
  C++ `ParseLevelText` only.

  Milestone 32 Phase B made the Development editor able to save, so the cooker
  now does see machine-generated source. Nothing changes here: the cooker stays
  a header check plus byte copy, and the game never invokes the cooker. After
  the editor writes `game/assets/source/levels/level_01.level`, the developer
  runs `python tools/cook_assets.py` and rebuilds to propagate it to the cooked
  and staged copies, then relaunches to load it. Expect the first editor save to
  canonicalize formatting (`25.60` becomes `25.6`), which changes
  `sourceSha256` and makes the cooker re-copy the file once.

Not cooker inputs:

- `.blend` files
- Blender authoring textures such as `textures/test_textured_basecolor.png`
- cooker test fixtures under `tools/fixtures/` (not staged, not in the game)

`schemaVersion` remains 1. Copy assets keep `id` / `source` / `cooked` /
`sourceSha256`. Runtime PNGs also record `recipe`, `sourceWidth`,
`sourceHeight`, `cookedWidth`, and `cookedHeight`. No timestamps or absolute
paths.

Incremental identity for `runtime_png` is source SHA-256 **plus** recipe. Changing
the max dimension or resample filter must recook even if source bytes are
unchanged.

Cooker tests (stdlib unittest, not pytest):

```
python tools/test_cook_runtime_png.py
python tools/test_cook_level_v1.py
```

The oversized fixture `tools/fixtures/textures/test_large_checker.png` is
1024×512 test input only. It is not a runtime asset.

See `docs/BLENDER_WORKFLOW.md` for authoring vs runtime texture roles.
