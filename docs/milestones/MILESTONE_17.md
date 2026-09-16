## Milestone 17 — Blender Authored Asset Workflow

### Objective

Introduce Blender as the official 3D model authoring tool without changing the M15/M16 runtime architecture.

```text
Blender
  ↓
.blend authored source
  ↓
manual GLB export
  ↓
existing Python cooker
  ↓
cooked GLB
  ↓
CMake runtime staging
  ↓
Renderer
```

### Scope

Add exactly one Blender source and its exported GLB:

```text
game/assets/source/blender/test_authored.blend
game/assets/source/models/test_authored.glb
```

Logical identity and outputs:

```text
models/test_authored.glb
game/assets/cooked/models/test_authored.glb
<exe>/assets/models/test_authored.glb
```

Preserve `textures/test_checker.png` and `models/test_static.glb`.

### Source Policy

`.blend` is editable authored source. `.glb` is exported runtime source. The cooker consumes the GLB, never the `.blend`. The runtime never loads `.blend`, source assets, or cooked assets directly.

Expected layout:

```text
game/assets/source/
├─ blender/
│  └─ test_authored.blend
├─ models/
│  ├─ test_static.glb
│  └─ test_authored.glb
└─ textures/
   └─ test_checker.png
```

### Blender Role

Blender is an authoring dependency only, not a build or runtime dependency. A machine without Blender must still be able to configure, build, cook existing exported assets, and run the game.

### Blender Test Model

Create one simple static low-poly model, such as a column, pedestal, simple arch, beveled crate, or small decorative structure.

Requirements:

- static and low-poly;
- sensible scale and pivot;
- authored near the Blender origin;
- no animation, armature, bones, or skinning;
- no external texture dependency;
- simple material is acceptable;
- no final-art requirement.

### Coordinate Convention

Document:

```text
Blender: X = horizontal, Y = depth, Z = up
Project: X = horizontal, Y = up, Z = depth
```

Do not create a generic coordinate conversion framework. Document actual glTF exporter axis/scale behavior used.

### Manual Blender Export

Use a manual workflow:

```text
open game/assets/source/blender/test_authored.blend
→ File → Export → glTF 2.0
→ Format: GLB
→ game/assets/source/models/test_authored.glb
```

Do not invoke Blender from CMake or the cooker. No Blender CLI/background/Python export automation in M17.

Document at least:

```text
Format: GLB
Animations: disabled/not exported
External textures: none
```

Use Selected Objects if practical. Exported GLB must be self-contained.

### Asset Cooker

Continue using `tools/cook_assets.py`. Known runtime assets become:

```text
textures/test_checker.png
models/test_static.glb
models/test_authored.glb
```

Preserve Python stdlib only, pathlib, SHA-256 identity, deterministic manifest, stable ordering, no timestamps/absolute paths, conservative stale cleanup, and clear non-zero errors.

The `.blend` must not appear in the runtime manifest.

### Manifest

Keep `schemaVersion: 1` if no schema change is required. It should contain the three runtime-source assets only, including `models/test_authored.glb`, and never `blender/test_authored.blend`.

### CMake Runtime Staging

CMake remains independent of Blender and does not cook assets. Stage:

```text
<TARGET_FILE_DIR>/assets/models/test_authored.glb
<TARGET_FILE_DIR>/assets/models/test_static.glb
<TARGET_FILE_DIR>/assets/textures/test_checker.png
```

for Debug, Development, and Release.

### Runtime Paths

Reuse executable-relative lookup:

```cpp
platform::RuntimeAssetPath("models/test_authored.glb")
```

No CWD dependency, repository-root search, source/cooked fallback, or `.blend` loading.

### Renderer

Follow M16:

```text
RuntimeAssetPath
→ LoadModel once
→ DrawModel each frame
→ UnloadModel once before graphics shutdown
```

No generic asset manager, scene graph, ECS, or generic handles.

### Visual Validation

Render the authored model in a dedicated visible location, approximately:

```text
x = -2.5
y ≈ 1.0
z = 2.5
```

It must be distinguishable from the checker, M16 model, Player, and greybox.

### No Collision

The model is visual only. No GreyboxWorld entry, Jolt body, mesh collision, collision derivation, or CharacterVirtual interaction.

### Failure Handling

If `models/test_authored.glb` is missing/invalid, log clearly, keep running, never try `.blend`/source/cooked fallbacks, and use a visual-only fallback if consistent with M16.

### Debug Metrics

Debug/Development only:

```text
Blender Authored Model
  loaded
  fallback
  id: models/test_authored.glb
```

No editable paths, browser, hot reload, or Blender controls. Release has no debug UI.

### Git Policy

Version:

```text
game/assets/source/blender/test_authored.blend
game/assets/source/models/test_authored.glb
```

Blender backups may be ignored:

```gitignore
*.blend1
*.blend2
```

Do not ignore `*.blend`.

### Documentation

Add `docs/BLENDER_WORKFLOW.md` documenting Blender version, source/export locations, naming and coordinate conventions, export settings, manual export steps, cooker command, source/runtime-source/cooked/runtime-staged terminology, authoring-only Blender role, Git policy, and no model collision.

### Gameplay and Physics Preservation

Do not change:

```text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Preserve CharacterVirtual, fixed gameplay Z, moving platform/carry, slopes, camera, cyan Jolt box, greybox, checker texture, and M16 static model.

### Implementation Phases

#### Phase A — Project Infrastructure

Cursor prepares `source/blender/`, cooker declaration, CMake staging, Renderer support, Debug Metrics, and docs. Cursor must not fabricate a `.blend` binary.

**Phase A:** `game/assets/source/blender/` exists. Cooker/CMake/Renderer did not yet require the authored GLB.

**Phase B:** Manual Blender 5.2.1 export of `test_authored.blend` → `test_authored.glb` (GLB, Selected Objects, +Y Up). See `docs/BLENDER_WORKFLOW.md`.

**Phase C:** Authored GLB is a required cooker/CMake/Renderer asset.

#### Phase B — Manual Blender Authoring

Using installed Blender:

1. Create the simple static model.
2. Save `game/assets/source/blender/test_authored.blend`.
3. Export `game/assets/source/models/test_authored.glb` manually.
4. Record Blender version and actual export settings.

#### Phase C — Integration and Validation

Run cooker twice, validate incremental behavior, build all configs, verify staging/rendering/CWD independence/fallback, and finalize docs.

### Explicitly Out of Scope

No M18, Blender automation/CLI/Python export, CMake/cooker invoking Blender, automatic export detection, dependency graph, material pipeline, texture extraction, PBR/normal maps, animation/rigging/skinning, scene export, prefabs, collision meshes/Jolt mesh collision, navmesh, LOD, mesh optimization/compression, hot reload, VFS, bundles, async loading, ECS, scene graph, or editor tooling.

### Build Validation

After manual export:

```powershell
python tools/cook_assets.py
python tools/cook_assets.py
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Second cook should report all three runtime assets and manifest unchanged/skipped.

### Manual Validation

Development:

```powershell
.\build\windows-vs2022\bin\Development\Platformer3D.exe
```

Verify checker, M16 model, M17 authored model, F1 asset metrics, Player/jump, moving platform, slopes, cyan box, resize, X/ESC.

Release from unrelated CWD:

```powershell
$exe = (Resolve-Path ".\build\windows-vs2022\bin\Release\Platformer3D.exe").Path
cd $env:TEMP
& $exe
```

Verify all assets load and Release has no debug UI.

Missing model test: temporarily remove `build/windows-vs2022/bin/Development/assets/models/test_authored.glb`, launch Development, verify clear diagnostic/no crash/no source fallback, then rebuild.

### Acceptance Criteria

- [ ] Exactly one authored `.blend` test asset exists and is versioned.
- [ ] Blender backup files may be ignored without ignoring `.blend`.
- [ ] Exactly one corresponding Blender-authored GLB exists under `source/models`.
- [ ] GLB is self-contained, static, and requires no external textures.
- [ ] Blender is neither build nor runtime dependency.
- [ ] CMake and cooker do not invoke Blender.
- [ ] GLB export is manual and settings are documented.
- [ ] Coordinate conventions/export behavior are documented.
- [ ] Cooker ignores `.blend` and processes `models/test_authored.glb` incrementally with SHA-256.
- [ ] M15 PNG and M16 GLB remain incremental.
- [ ] Second unchanged cook does not meaningfully rewrite outputs.
- [ ] Manifest remains deterministic, excludes `.blend`, and stays schemaVersion 1 if appropriate.
- [ ] CMake stages M17 GLB in Debug, Development, and Release without cooking.
- [ ] Runtime paths remain executable-relative and CWD-independent.
- [ ] Runtime never loads `.blend` or source assets directly.
- [ ] Authored model loads once and unloads before graphics shutdown.
- [ ] Authored model is visibly distinguishable from M16 model.
- [ ] Authored model has no collision, Jolt body, or GreyboxWorld entry.
- [ ] Missing staged M17 GLB does not crash.
- [ ] Debug/Development metrics expose M17 model status; Release has no debug UI.
- [ ] Player, moving platform, slopes, camera, and cyan box remain unchanged.
- [ ] Milestone 18 scope is not implemented.

### Definition of Done

```text
Blender-authored .blend
→ manual GLB export
→ versioned runtime-source GLB
→ existing SHA-256 cooker
→ deterministic cooked output
→ CMake staging
→ executable-relative runtime lookup
→ one-time Renderer load
→ visible Blender-authored model
→ safe unload
```

The project must build and run without Blender installed once exported assets exist. No Milestone 18 functionality may be included.
