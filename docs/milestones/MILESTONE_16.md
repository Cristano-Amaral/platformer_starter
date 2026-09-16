## Milestone 16 — First Static 3D Model Asset Pipeline

### Objective

Extend the Milestone 15 asset pipeline so it supports the first authored static 3D model asset in GLB format.

The pipeline must become:

text

game/assets/source/models/test_static.glb
                  ↓
         Python asset cooker
                  ↓
game/assets/cooked/models/test_static.glb
                  ↓
          CMake runtime staging
                  ↓
<exe>/assets/models/test_static.glb
                  ↓
              Renderer

The existing Milestone 15 texture pipeline must continue working unchanged.

This milestone is intentionally limited to proving:

text

GLB source
→ cooked
→ staged
→ loaded
→ rendered

The model is visual only and must not participate in collision or physics.

## Scope

Add exactly one authored static model:

    game/assets/source/models/test_static.glb

Logical asset identity:

    models/test_static.glb

Cooked output:

    game/assets/cooked/models/test_static.glb

Runtime staged path:

    <exe>/assets/models/test_static.glb

The existing Milestone 15 texture remains:

    textures/test_checker.png

After this milestone, the authored asset set should be:

    game/assets/source/
    ├─ textures/
    │  └─ test_checker.png
    └─ models/
   └─ test_static.glb


### Static GLB Test Asset

Create one small deterministic GLB/glTF 2.0 test model locally.

The model may be a simple:

- cube;
- pyramid;
- prism;
- or similarly simple low-poly shape.

Requirements:

- valid GLB/glTF 2.0;
- single self-contained `.glb` file;
- no external buffers;
- no external textures;
- no downloaded artwork;
- no copyrighted third-party model;
- small enough to remain a technical test asset;
- clearly visible as a 3D object at runtime.

The user must not need to provide or download any model manually.

A temporary implementation-time helper script may be used to generate the GLB using only the Python standard library.

However, the asset cooker itself must not become a procedural model generator.

The final authored source asset must exist at:

    game/assets/source/models/test_static.glb

### Asset Cooker

Continue using:

    tools/cook_assets.py

Do not create a second cooker.

The cooker must now process both known authored assets:

    textures/test_checker.png
    models/test_static.glb

A small explicit or declarative list of known assets is acceptable.

Do not implement a generic asset database.

Both assets must preserve the Milestone 15 incremental behavior:

- Python standard library only;
- `pathlib`;
- SHA-256 content identity;
- no mtime-based identity;
- deterministic output;
- no absolute paths in the manifest;
- no timestamps in the manifest;
- stable ordering;
- conservative stale-output cleanup;
- clear non-zero failure on missing required source assets.

If an asset has not changed, its cooked output must not be rewritten.

Example unchanged run:

    textures/test_checker.png → unchanged/skipped
    models/test_static.glb    → unchanged/skipped
    manifest.json             → unchanged/skipped

If only the GLB changes:

    textures/test_checker.png → unchanged/skipped
    models/test_static.glb    → cooked
    manifest.json             → updated


### Manifest

Reuse the current manifest structure.

If the Milestone 15 schema naturally supports multiple entries, keep:

    schemaVersion = 1

Do not increment the schema version merely because a second asset exists.

Expected structure:

    ```json
    {
    "assets": [
        {
        "cooked": "models/test_static.glb",
        "id": "models/test_static.glb",
        "source": "models/test_static.glb",
        "sourceSha256": "..."
        },
        {
        "cooked": "textures/test_checker.png",
        "id": "textures/test_checker.png",
        "source": "textures/test_checker.png",
        "sourceSha256": "..."
        }
    ],
    "schemaVersion": 1
    }
    ```

The exact ordering may follow the current deterministic strategy, but it must remain stable.

### CMake Runtime Staging

CMake must not perform asset cooking.

The user must continue running:

    ```powershell
    python tools/cook_assets.py
    ```

CMake may validate that the required cooked assets exist and fail with an actionable message when they do not.

Extend the existing runtime staging so both assets are copied to:

    ```text
    <TARGET_FILE_DIR>/assets/textures/test_checker.png
    <TARGET_FILE_DIR>/assets/models/test_static.glb
    ```

This must work for:

- Debug;
- Development;
- Release.

Expected runtime layout:

    ```text
    bin/Development/
    ├─ Platformer3D.exe
    └─ assets/
    ├─ textures/
    │  └─ test_checker.png
    └─ models/
        └─ test_static.glb
    ```

The same structure must exist for Debug and Release.

Use `copy_if_different` or equivalent behavior consistent with Milestone 15.

A small CMake list/helper is acceptable if it reduces duplication.

Do not introduce asset archives, bundles, installers, or packaging systems.

### Runtime Asset Paths

Reuse the Milestone 15 runtime path abstraction.

The model must be resolved through the executable directory.
Conceptually:

    ```cpp
    platform::RuntimeAssetPath("models/test_static.glb")
    ```

must resolve to:

    ```text
    <actual executable directory>/assets/models/test_static.glb
    ```

Do not:

- use `std::filesystem::current_path()`;
- depend on the current working directory;
- search upward for the repository root;
- load from `game/assets/source`;
- load directly from `game/assets/cooked`;
- introduce development-only path fallbacks.
- Preserve the current platform-specific executable-directory isolation.

### Renderer Ownership

The Renderer/render backend must own the raylib model resource.

Gameplay must not depend on raylib model types such as:

    ```cpp
    Model
    Mesh
    Material
    Texture2D
    ```

The model lifecycle must be:

    ```text
    graphics context initialized
            ↓
    LoadModel once
            ↓
    render each frame
            ↓
    UnloadModel once
            ↓
    graphics context shutdown
    ```

Do not load the model every frame.

Do not unload it after the graphics context has already shut down.

Preserve the existing Milestone 15 texture lifecycle.

### Visual Test

Render the static model in a dedicated visible location.

A position near:

    x = 2.5
    y ≈ 1.0
    z = 2.5

is acceptable, but may be adjusted slightly for visibility.

The model must:

- be clearly visible;
- appear three-dimensional;
- not significantly overlap the Player;
- not hide the checker texture test surface;
- not affect gameplay.

A simple render-owned position/scale/rotation is sufficient.

Do not introduce:

- generic Transform components;
- scene graph;
- ECS;
- entity abstraction.

### No Collision

The GLB is visual only.

Do not:

- add it to `GreyboxWorld`;
- create a Jolt body for it;
- create mesh collision;
- derive collision from the GLB;
- make `CharacterVirtual` interact with it.

Rendering geometry and physics geometry remain intentionally separate.

### Runtime Failure Handling

If the staged runtime model is missing or cannot be loaded:

- emit a clear diagnostic including:
    models/test_static.glb
- do not crash;
- keep the game running;
- do not fall back to source or cooked development paths.

A simple visual fallback cube, wireframe, or marker is acceptable.

The fallback must remain visual only.

### Debug Metrics

In Debug and Development only, extend the existing read-only Assets diagnostics.

Example:

    Assets

    Texture
      loaded: true
      fallback: false

    Static Model
    loaded: true
    fallback: false
    id: models/test_static.glb

Do not:

- expose editable paths;
- add asset browsing;
- add hot reload.
- Release must remain free of Dear ImGui/debug UI.

### Gameplay and Physics Preservation

Do not change gameplay or physics tuning.
Preserve:

    max horizontal speed = 6
    acceleration         = 40
    deceleration         = 50
    jump speed           = 8
    gravity              = 20
    coyote time          = 0.10 s
    jump buffer          = 0.10 s

Also preserve:

- CharacterVirtual behavior;
- fixed gameplay Z behavior;
- moving platform behavior and carry;
- walkable 30° slope;
- steep 60° slope;
- camera behavior;
- cyan dynamic Jolt test box;
- existing greybox world;
- existing checker texture test.

No unrelated gameplay, physics, or camera refactors.

### Documentation

Update relevant documentation minimally.

Document:

- source model path;
- logical model identity;
- cooked model path;
- runtime staged path;
- cooker command;
- that the GLB is currently copied unchanged;
- that model collision is intentionally unsupported;
- that the GLB is a technical pipeline test asset.

Keep the terminology consistent:

    source
    cooked
    runtime staged

### Explicitly Out of Scope

Do not implement:

- Milestone 17;
- Blender automation;
- Blender Python integration;
- `.blend` source pipeline;
- OBJ;
- FBX;
- animation;
- skeletal animation;
- bones;
- skinning;
- morph targets;
- material pipeline;
- advanced PBR pipeline;
- normal maps;
- custom shaders;
- model LOD;
- mesh optimization;
- mesh compression;
- Draco;
- KTX;
- texture compression;
- mipmap generation;
- collision generated from model meshes;
- Jolt mesh collision;
- scene files;
- scene graph;
- ECS;
- generic asset handles;
- shared-ownership asset manager;
- asynchronous asset loading;
- background loading;
- hot reload;
- VFS;
- archives;
- asset packs;
- runtime asset registry;
- level loading.

### Build Validation

Run the cooker:

```powershell
    python tools/cook_assets.py
```

Run it again without changing anything:

```powershell
    python tools/cook_assets.py
```

The second run must report both assets as unchanged/skipped and must not meaningfully rewrite the manifest.

Then run:

```powershell
    cmake --preset windows-vs2022
    cmake --build --preset windows-debug
    cmake --build --preset windows-development
    cmake --build --preset windows-release
```

All configurations must succeed.

### Manual Validation

**Normal Development Test**

Run:

```powershell
    .\build\windows-vs2022\bin\Development\Platformer3D.exe
```

Verify:

- checker texture appears;
- GLB model appears;
- F1 Assets metrics report the model as loaded;
- Player movement works;
- jump works;
- moving platform works;
- slopes behave as before;
- cyan dynamic box behaves as before;
- resize works;
- X and ESC close the game.

**Release Test From an Unrelated Working Directory**

Run:

```powershell
    $exe = (Resolve-Path ".\build\windows-vs2022\bin\Release\Platformer3D.exe").Path
    cd $env:TEMP
    & $exe
```

Verify:

- checker texture loads;
- GLB model loads;
- runtime behavior does not depend on CWD;
- gameplay remains functional;
- no debug UI is present.
- Missing Runtime Model Test

Temporarily remove:

    build/windows-vs2022/bin/Development/assets/models/test_static.glb

Launch the Development executable.

Verify:

- clear missing-model diagnostic;
- no crash;
- game continues;
- fallback visual appears if implemented;
- no source/cooked fallback loading occurs.

Rebuild afterward to restore the staged asset.

### Acceptance Criteria

1. `cmake --preset windows-vs2022` succeeds.
2. Debug builds successfully.
3. Development builds successfully.
4. Release builds successfully.
5. External dependency versions remain unchanged.
6. Milestone 15 PNG pipeline still works.
7. Exactly one authored test model is added.
8. The model source format is GLB.
9. The GLB is created locally.
10. No external model is downloaded.
11. No third-party copyrighted model is introduced.
12. The GLB is valid glTF 2.0.
13. The GLB is self-contained.
14. Source model is under `game/assets/source/models/`.
15. Cooked model is under `game/assets/cooked/models/`.
16. Runtime model is staged under `<exe>/assets/models/`.
17. Runtime never loads the source model.
18. Runtime never loads directly from `game/assets/cooked`.
19. Existing `tools/cook_assets.py` remains the single cooker.
20. Cooker remains Python-standard-library-only.
21. Cooker processes both PNG and GLB.
22. Both assets use SHA-256 content identity.
23. Unchanged PNG is skipped.
24. Unchanged GLB is skipped.
25. Manifest is not rewritten without changes.
26. Changing only GLB does not recook PNG.
27. Manifest remains deterministic.
28. Manifest contains no absolute paths.
29. Manifest contains no timestamps.
30. Manifest asset ordering is stable.
31. Stale cleanup remains conservative.
32. Missing source GLB produces clear non-zero failure.
33. CMake does not run the cooker.
34. CMake only validates/stages cooked assets.
35. Debug receives the staged GLB.
36. Development receives the staged GLB.
37. Release receives the staged GLB.
38. Runtime model paths use the executable directory.
39. Model loading is independent of CWD.
40. Model loads only once.
41. Model is not loaded every frame.
42. Model unloads before graphics shutdown.
43. raylib model resource types do not leak into gameplay.
44. The GLB model is visibly rendered.
45. The GLB has no gameplay collision.
46. The GLB is not added to `GreyboxWorld`.
47. The GLB creates no Jolt body.
48. Missing runtime model does not crash.
49. Debug/Development Assets metrics expose model state.
50. Release contains no Dear ImGui/debug UI.
51. Player behavior remains unchanged.
52. Moving platform behavior remains unchanged.
53. Slope behavior remains unchanged.
54. Cyan dynamic box remains unchanged.
55. Milestone 17 scope is not implemented.

### Definition of Done

Milestone 16 is complete when:

    GLB source
    → cooker
    → deterministic manifest
    → cooked GLB
    → CMake staging
    → executable-relative runtime path
    → one-time Renderer load
    → visible static model
    → safe unload

works in Debug, Development, and Release while preserving all gameplay and physics behavior from the previous milestone.

No Milestone 17 functionality may be included.
