# Blender authored asset workflow

Blender is an **authoring dependency only**.

It is not a CMake/build dependency and not a runtime dependency. After the
exported GLB exists, a machine without Blender can still configure, build,
cook, and run the game.

## Terminology

| Term | Meaning |
|---|---|
| authored source (`.blend`) | Editable Blender file. Never cooked. Never loaded at runtime. |
| runtime source (`.glb`) | Exported glTF 2.0 binary under `game/assets/source/models/`. Input to the cooker. |
| cooked | Cooker output under `game/assets/cooked/`. Generated. |
| runtime staged | Copy next to the executable: `<exe>/assets/...` |

The cooker consumes the exported GLB, never `.blend`.
The game loads only staged cooked files, never `.blend`, never `source/`, never `cooked/` directly.

## Layout

```text
game/assets/source/
├─ blender/
│  ├─ test_authored.blend
│  └─ test_textured.blend
├─ models/
│  ├─ test_static.glb
│  ├─ test_authored.glb
│  └─ test_textured.glb              (texture must be embedded)
└─ textures/
   ├─ test_checker.png
   └─ test_textured_basecolor.png    (authoring input; not cooked)
```

Logical identity (cooker / runtime):

```text
models/test_authored.glb
```

Cooked / staged:

```text
game/assets/cooked/models/test_authored.glb
<executable directory>/assets/models/test_authored.glb
```

`game/assets/source/blender/` is **not** a runtime asset directory.

## Pipeline

```text
game/assets/source/blender/test_authored.blend
        ->
    manual Blender export (File -> Export -> glTF 2.0, GLB)
        ->
game/assets/source/models/test_authored.glb
        ->
    python tools/cook_assets.py
        ->
game/assets/cooked/models/test_authored.glb
        ->
    CMake POST_BUILD staging
        ->
<exe>/assets/models/test_authored.glb
        ->
    Renderer (LoadModel once / DrawModel / UnloadModel once)
```

The cooker does not invoke Blender.
CMake does not invoke Blender.
CMake does not cook assets.

## Recorded Blender export (Milestone 17)

Blender version: **5.2.1** (Khronos glTF Blender I/O v5.2.40 in the GLB asset tag).

| Setting | Value |
|---|---|
| Format | glTF Binary (.glb) |
| Selected Objects | ON |
| +Y Up | ON |
| Apply Modifiers | ON |
| UVs | ON |
| Normals | ON |
| Tangents | OFF |
| External textures | none |
| Animation | none |

## Coordinate conventions

Project (runtime):

```text
X = horizontal
Y = up
Z = depth
```

Blender authoring:

```text
X = horizontal
Y = depth
Z = up
```

The export was performed with **+Y Up** enabled. No manual mesh rotation was
added in the game merely to compensate for Blender axes. Runtime orientation
is validated by rendering the staged GLB through the existing loader.

## Export requirements (technical test asset)

- Format: **GLB** (single self-contained `.glb`)
- glTF 2.0
- No external buffers
- No external textures
- Static mesh only: no animation, armature, bones, or skinning
- Keep the object near the scene origin
- Use a sensible scale
- Simple material is acceptable
- Collision from this mesh is **intentionally unsupported**

## Git

Version authored `.blend` files. Do **not** gitignore `*.blend`.

Blender backup files may be ignored:

```text
*.blend1
*.blend2
```

## Cooker

```powershell
python tools/cook_assets.py
```

Python 3, standard library only. SHA-256 incremental copy. Manifest
`schemaVersion` 1. Known runtime-source assets:

```text
textures/test_checker.png
models/test_static.glb
models/test_authored.glb
```

No `.blend` entries in the manifest.

## Re-export after editing the .blend

1. Open `game/assets/source/blender/test_authored.blend` in Blender.
2. File → Export → glTF 2.0.
3. Keep the settings above (GLB, Selected Objects, +Y Up, no animation, no external textures).
4. Overwrite `game/assets/source/models/test_authored.glb`.
5. From the repository root: `python tools/cook_assets.py`
6. Rebuild so CMake restages `<exe>/assets/models/test_authored.glb`.

## Blender automation

Out of scope for Milestone 17. No `blender --background`, no Blender Python
export, no CMake/cooker Blender invocation.

## Runtime notes (Milestone 17)

The authored model is drawn at approximately `(-2.5, 1.0, 2.5)`. It is visual
only: no GreyboxWorld entry, no Jolt body, no mesh collision.

---

## Milestone 18 — embedded Base Color texture

Milestone 18 proves one Blender-authored GLB whose **Base Color image is
embedded in the GLB**. The game must not load a sidecar PNG for that model.

### File contract

| Role | Path |
|---|---|
| Authored `.blend` | `game/assets/source/blender/test_textured.blend` |
| Authoring Base Color PNG | `game/assets/source/textures/test_textured_basecolor.png` |
| Exported runtime-source GLB | `game/assets/source/models/test_textured.glb` |
| Logical id | `models/test_textured.glb` |
| Cooked | `game/assets/cooked/models/test_textured.glb` |
| Runtime staged | `<exe>/assets/models/test_textured.glb` |

- `.blend` = editable authoring source. Never cooked. Never loaded at runtime.
- `test_textured_basecolor.png` = authored Base Color source used by Blender.
  Version it. It is **not** a cooker input, **not** staged, and **not** loaded
  by the game.
- Exported `test_textured.glb` = runtime-source model. The Base Color image
  must be embedded inside this file (no sidecar PNG, no external URI).
- Cooker consumes **only** the GLB for this model.
- Runtime consumes **only** the staged GLB via
  `RuntimeAssetPath("models/test_textured.glb")` → `LoadModel`.
- No runtime path lookup for `textures/test_textured_basecolor.png`.
- Blender remains authoring-only. CMake and the cooker do not invoke Blender.

The model is drawn at approximately `(4.0, 1.0, 2.5)`. Visual only: no
GreyboxWorld entry, no Jolt body, no mesh collision.

### Established export settings (Blender 5.2.1)

- Format: glTF Binary (.glb)
- Selected Objects: ON
- +Y Up: ON
- Apply Modifiers: ON
- UVs: ON
- Normals: ON
- Tangents: OFF
- Animations: OFF
- Images: **must be included/embedded** (do not export Images = None)

No external sidecar texture is required next to the GLB or the executable.

### Manual re-export

1. Open `game/assets/source/blender/test_textured.blend` in Blender 5.2.1.
2. Confirm the Image Texture node still uses
   `game/assets/source/textures/test_textured_basecolor.png`.
3. Confirm the mesh shows the image in the viewport.
4. **File → Export → glTF 2.0** as GLB to
   `game/assets/source/models/test_textured.glb` (overwrite).
5. Keep the settings above, including embedded images.
6. From the repository root: `python tools/cook_assets.py`
7. Rebuild so CMake restages `<exe>/assets/models/test_textured.glb`.

Do not copy the PNG next to the executable. If the viewport is textured but
the GLB JSON has no `images`/`textures`, the exporter omitted the image —
re-export with Images included.

### Source texture guidance

- Small technical PNG: **128×128** or **256×256**.
- Asymmetric pattern so UV orientation is obvious.
- Do not resize/compress/mipmap in the cooker. No DDS/KTX.

The cooker still does not invoke Blender. CMake still does not invoke Blender.
