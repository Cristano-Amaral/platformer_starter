## Milestone 18 — Material + Embedded Texture Asset Workflow

## Objective

Extend the proven Blender-authored GLB workflow from Milestone 17 so one authored static model uses a simple textured material whose image data is embedded inside the exported GLB.

The goal is to prove this content path:

```text
Blender authored model + UVs + image texture
        ↓
.blend authored source
        ↓
manual GLB export
        ↓
self-contained textured GLB
        ↓
existing SHA-256 cooker
        ↓
deterministic cooked GLB
        ↓
CMake runtime staging
        ↓
executable-relative runtime loading
        ↓
raylib Model + embedded material/texture
```

Milestone 18 is deliberately **not** a general material system or texture dependency pipeline. It proves exactly one simple textured Blender-authored GLB while preserving the architecture and behavior established through Milestone 17.

---

## Why This Milestone

Milestones 15–17 have already proven:

- one standalone PNG runtime asset;
- one technical static GLB;
- one real Blender-authored static GLB;
- deterministic SHA-256 cooking;
- CMake staging;
- executable-relative runtime lookup;
- safe renderer ownership/lifetime;
- Blender as an authoring-only dependency.

The next useful content-pipeline step is to prove that a Blender-authored model can carry its own simple material and texture through the same GLB path without introducing runtime sidecar dependencies.

This keeps the asset contract friendly to future packaging and constrained targets:

```text
one runtime model asset
→ one staged GLB
→ no runtime search for authoring textures
```

---

## Scope

Create exactly one new Blender-authored textured test asset.

Authored Blender source:

```text
game/assets/source/blender/test_textured.blend
```

Exported runtime source:

```text
game/assets/source/models/test_textured.glb
```

Logical runtime identity:

```text
models/test_textured.glb
```

Cooked output:

```text
game/assets/cooked/models/test_textured.glb
```

Runtime staged path:

```text
<exe>/assets/models/test_textured.glb
```

Preserve all existing assets:

```text
textures/test_checker.png
models/test_static.glb
models/test_authored.glb
```

The M18 model is an additional visual test asset. Do not replace the M15, M16, or M17 assets.

---

## Authored Texture Source

Use exactly one small technical image texture for the M18 Blender asset.

Preferred authored texture source path:

```text
game/assets/source/textures/test_textured_basecolor.png
```

This PNG is an **authoring input** for Blender and must be versioned with the `.blend` source.

It is not a standalone runtime asset in this milestone.

It must **not** be added to the runtime cooker known-assets list and must **not** be staged separately next to the executable.

The exported GLB must contain the image data internally.

Use a small technical texture, preferably:

```text
128x128 or 256x256
```

Do not use an external downloaded/copyrighted texture. Create a simple local test pattern such as stripes, quadrants, arrows, or a small color grid so orientation and UV mapping are visually obvious.

The existing M15 `test_checker.png` remains a separate runtime texture test and should not be repurposed as the M18 authoring texture unless there is a strong reason.

---

## Expected Source Layout

Conceptually:

```text
game/assets/source/
├─ blender/
│  ├─ test_authored.blend
│  └─ test_textured.blend
├─ models/
│  ├─ test_static.glb
│  ├─ test_authored.glb
│  └─ test_textured.glb
└─ textures/
   ├─ test_checker.png
   └─ test_textured_basecolor.png
```

Terminology:

```text
.blend under source/blender          = editable authoring source
PNG under source/textures            = authoring texture source (not a runtime asset)
GLB under source/models              = exported runtime source
asset under assets/cooked            = cooked runtime asset
asset under <exe>/assets              = runtime-staged asset
```

The runtime must never load from `source/blender`.

---

## Blender Model

Create one simple static low-poly object that makes UV orientation easy to inspect.

Good choices:

- beveled crate;
- short pillar;
- rectangular sign/block;
- simple low-poly prop.

A beveled crate or rectangular block is preferred because the texture mapping is easy to inspect visually.

The model must:

- be static;
- remain low-poly;
- use a sensible origin/pivot;
- use a sensible scale;
- have applied scale before final export;
- have no animation;
- have no armature;
- have no bones;
- have no skinning;
- have no collision authority.

---

## UV Requirement

The M18 model must have an explicit UV map.

The purpose is to prove that authored UV data survives:

```text
Blender
→ GLB
→ cooker
→ runtime
```

Use a simple UV unwrap suitable for the chosen test mesh.

Do not introduce automatic UV tooling or a project-wide UV policy.

The technical texture should make obvious if the UV orientation is mirrored, rotated, stretched, or otherwise incorrect.

---

## Material Requirement

Use exactly one simple Blender material for the M18 model.

The intended material graph is minimal:

```text
Image Texture
    ↓
Principled BSDF Base Color
    ↓
Material Output
```

No advanced material authoring is required.

Do not add project-level abstractions for:

- material instances;
- material definitions;
- shaders;
- PBR presets;
- texture slots;
- material databases.

This milestone relies on the material data exported in the GLB and loaded by the existing raylib model loader.

---

## Texture Requirements

The M18 technical texture must:

- be locally created;
- be small;
- use a common PNG format;
- contain no copyrighted third-party artwork;
- be visually distinctive;
- make UV orientation easy to inspect;
- be used as the Base Color texture;
- require no external runtime file after GLB export.

Do not add:

- normal map;
- metallic map;
- roughness map;
- AO map;
- emissive map;
- height map;
- multiple texture sets.

Exactly one authored Base Color image texture is enough.

---

## Embedded GLB Texture Contract

The exported:

```text
game/assets/source/models/test_textured.glb
```

must be self-contained.

The image texture must be embedded in the GLB binary payload rather than referenced by an external URI/file.

The final runtime staging must therefore require only:

```text
assets/models/test_textured.glb
```

for the M18 asset.

Do not stage:

```text
assets/textures/test_textured_basecolor.png
```

The authoring PNG remains under `source/textures/` only. It is not a runtime asset.

The final validation/report must inspect the GLB using existing/local tooling or a small standard-library validation script and confirm that the GLB has no external URI dependency.

Do not add a permanent general-purpose glTF parser to the project merely for this check.

---

## Coordinate Convention

Preserve the M17 Blender export contract.

Blender authoring:

```text
X = horizontal
Y = depth
Z = up
```

Project/runtime:

```text
X = horizontal
Y = up
Z = depth
```

Use the same confirmed Blender export behavior from M17:

```text
Format: glTF Binary (.glb)
Selected Objects: ON
+Y Up: ON
Apply Modifiers: ON
UVs: ON
Normals: ON
Tangents: OFF unless demonstrated necessary
Animation: none
```

Do not manually rotate the model merely to compensate for axes.

---

## Manual Blender Workflow

Milestone 18 keeps Blender export manual.

Do not automate Blender.

Suggested workflow:

```text
create technical PNG
↓
open Blender
↓
create simple mesh
↓
UV unwrap
↓
create one material
↓
connect PNG to Base Color
↓
save .blend
↓
export GLB manually
↓
verify GLB is self-contained
↓
run existing cooker
```

Save authored Blender source as:

```text
game/assets/source/blender/test_textured.blend
```

Export to:

```text
game/assets/source/models/test_textured.glb
```

---

## Asset Cooker

Continue using:

```text
tools/cook_assets.py
```

Final runtime known-assets list should contain exactly the existing runtime assets plus the new M18 GLB:

```text
textures/test_checker.png
models/test_static.glb
models/test_authored.glb
models/test_textured.glb
```

Do not add:

```text
blender/test_textured.blend
textures/test_textured_basecolor.png
```

to the runtime manifest.

Preserve:

- Python standard library only;
- `pathlib`;
- SHA-256 content identity;
- deterministic manifest;
- stable sorting;
- no timestamps;
- no absolute paths;
- conservative stale cleanup;
- required-source failure behavior.

No `pendingAuthored` bridge should be needed in the completed milestone.

---

## Manifest

Keep:

```text
schemaVersion: 1
```

unless the existing schema genuinely cannot represent the new GLB. No schema change is expected.

The manifest should contain four runtime assets and no Blender authoring files.

Conceptually:

```text
models/test_authored.glb
models/test_static.glb
models/test_textured.glb
textures/test_checker.png
```

in deterministic ID ordering.

The authored texture PNG under `source/textures/` must not appear in the runtime manifest.

---

## CMake Runtime Staging

Extend the existing runtime asset list with:

```text
models/test_textured.glb
```

Stage to:

```text
<TARGET_FILE_DIR>/assets/models/test_textured.glb
```

Preserve staging for all previous runtime assets.

CMake must not:

- invoke Blender;
- invoke the cooker;
- stage the M18 authoring PNG separately;
- search `source/blender` at runtime.

The M18 runtime asset is one GLB.

---

## Renderer

Load the new model through the same narrow renderer-side pattern used by M16/M17:

```text
RuntimeAssetPath("models/test_textured.glb")
→ LoadModel once
→ DrawModel each frame
→ UnloadModel once
```

Do not create a generic asset manager or material system.

The model's embedded texture/material should be handled by the existing raylib GLB loading path.

Do not manually load the authored PNG at runtime.

Do not replace the GLB material with a runtime-created texture/material merely to make the visual test pass.

The milestone specifically needs to prove the authored GLB material/texture path.

---

## Visual Placement

Place the M18 textured model at a dedicated visible location that does not overlap the M15/M16/M17 visual tests.

Choose a simple constant position based on the existing scene layout.

The exact position may be adjusted during implementation for visibility, but document the final value.

The model must be visually distinguishable from:

- M15 checker quad;
- M16 technical model;
- M17 Blender-authored untextured/simple-material model;
- Player;
- greybox world;
- slopes;
- moving platform;
- cyan Jolt test body.

---

## Renderer Resource Lifetime

The M18 model must:

- load once after the graphics context is valid;
- remain owned by Renderer/render-facing code;
- draw while loaded;
- unload exactly once before window/graphics shutdown.

Do not expose raylib `Model`, `Material`, `Texture2D`, or other raylib resource types to gameplay.

---

## Failure Handling

If:

```text
assets/models/test_textured.glb
```

is missing or fails to load:

- log a clear diagnostic containing `models/test_textured.glb`;
- keep the game running;
- use a simple visual fallback consistent with M16/M17;
- do not attempt to load the authored PNG;
- do not attempt to load the `.blend`;
- do not load directly from source;
- do not load directly from cooked;
- do not search the repository;
- do not depend on CWD.

The fallback is visual-only.

---

## Debug Metrics

In Debug/Development, extend the existing read-only Assets section with the M18 asset.

Conceptually:

```text
Textured Blender Model
  loaded: true/false
  fallback: true/false
  id: models/test_textured.glb
```

If it fits naturally without introducing new renderer abstractions, also expose a simple read-only diagnostic confirming the loaded model has material/texture data, for example:

```text
material count
```

or another already-available safe diagnostic.

Do not add complex material introspection solely for the metrics panel.

Do not add editable asset paths, asset browser UI, reload buttons, or material controls.

Release remains free of Dear ImGui.

---

## Documentation

Update:

```text
docs/BLENDER_WORKFLOW.md
```

with a concise section covering textured static assets.

Document:

- authored texture location;
- `.blend` location;
- exported GLB location;
- one-material/one-Base-Color-texture scope;
- UV requirement;
- manual texture assignment;
- GLB export settings;
- embedded/self-contained texture requirement;
- authoring PNG is versioned but not a runtime asset;
- runtime stages only the GLB for this model;
- how to re-export after changing the texture/model;
- Blender remains authoring-only;
- collision remains separate.

Update other project docs only where needed to keep terminology accurate. Avoid documentation churn unrelated to M18.

---

## Git Policy

Version authored sources:

```text
game/assets/source/blender/test_textured.blend
game/assets/source/textures/test_textured_basecolor.png
game/assets/source/models/test_textured.glb
```

Continue ignoring Blender backups:

```text
*.blend1
*.blend2
```

Do not ignore:

```text
*.blend
```

Cooked outputs remain governed by the existing cooked-asset policy.

---

## Gameplay and Physics Preservation

Do not change gameplay tuning:

```text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Preserve:

- CharacterVirtual behavior;
- fixed gameplay Z;
- moving-platform carry;
- 30° walkable slope;
- 60° steep slope;
- platformer camera;
- cyan dynamic Jolt box;
- greybox world;
- M15 checker;
- M16 static GLB;
- M17 Blender-authored GLB.

The M18 model is visual only.

Do not create a Jolt body or add it to `GreyboxWorld`.

---

## Implementation Phases

Use the same controlled workflow that worked for M17.

### Phase A — Infrastructure Preparation

Cursor should:

- inspect the completed M17 pipeline;
- prepare the new M18 logical asset identity;
- prepare cooker/CMake/Renderer/metrics integration without breaking the current build before the authored files exist;
- update Blender workflow documentation;
- provide the exact manual Phase B instructions;
- stop before fabricating `.blend` or final GLB content.

Avoid introducing a permanent generic optional/pending asset system merely for the Phase A bridge. If activation must wait until Phase C, prefer small clearly marked Phase C activation points.

**Phase C (current):** `models/test_textured.glb` is a required cooker/CMake/Renderer asset. `pendingAuthored` is removed. The Base Color PNG is not a cooker/runtime asset. See `docs/BLENDER_WORKFLOW.md`.

### Phase B — Manual Blender Authoring

The user should follow the exact 14-step checklist in `docs/BLENDER_WORKFLOW.md` (Blender 5.2.1, UV, 128×128 or 256×256 asymmetric Base Color PNG, one Principled BSDF material, embed on GLB export, M17 coordinate/export convention). Summary:

1. create the technical PNG as `game/assets/source/textures/test_textured_basecolor.png`;
2. create a simple Blender model;
3. UV unwrap it;
4. create one material;
5. connect the PNG to Base Color;
6. save `test_textured.blend`;
7. export `test_textured.glb` manually;
8. confirm the GLB is the only runtime-source file required by this M18 model;
9. report the Blender/export settings used;
10. stop before Phase C.

### Phase C — Integration and Validation

Cursor should then:

- activate the M18 GLB as a required cooker asset;
- cook it;
- verify deterministic incremental behavior;
- verify self-contained GLB/no external URI dependency;
- activate CMake staging;
- activate Renderer loading/drawing/unloading;
- activate Debug Metrics;
- build all configurations;
- prepare/perform runtime tests;
- remove temporary Phase A scaffolding;
- stop before commit so final manual approval can happen.

---

## Build Validation

After Phase C integration, run:

```powershell
python tools/cook_assets.py
python tools/cook_assets.py
```

On the second unchanged run, all four runtime assets and the manifest should be unchanged/skipped.

Then:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

All configurations must succeed.

Verify the M18 runtime stage contains:

```text
assets/models/test_textured.glb
```

and does **not** require:

```text
assets/textures/test_textured_basecolor.png
```

---

## Development Manual Validation

Run:

```powershell
.\build\windows-vs2022\bin\Development\Platformer3D.exe
```

Verify:

- M15 checker still renders;
- M16 static GLB still renders;
- M17 Blender-authored GLB still renders;
- M18 textured GLB renders;
- the M18 Base Color texture is visible on the model;
- UV orientation looks sensible;
- the texture is not unexpectedly mirrored/stretched;
- model orientation is sensible;
- model scale is sensible;
- Debug Assets metrics show the M18 model loaded;
- fallback is false;
- Player movement/jump remain correct;
- moving platform remains correct;
- slopes remain correct;
- cyan dynamic box remains correct.

---

## Runtime Sidecar Validation

Inspect the executable's staged M18 files.

For this asset, the runtime should need:

```text
assets/models/test_textured.glb
```

There must be no requirement for a separately staged:

```text
assets/textures/test_textured_basecolor.png
```

This is a key acceptance condition.

---

## Release CWD Independence

From the repository root:

```powershell
$exe = (Resolve-Path ".\build\windows-vs2022\bin\Release\Platformer3D.exe").Path
cd $env:TEMP
& $exe
```

Verify:

- M15 asset loads;
- M16 model loads;
- M17 model loads;
- M18 textured model loads with its texture;
- runtime remains independent of CWD;
- no authoring PNG lookup occurs;
- no Debug/ImGui UI appears.

---

## Missing Runtime Model Validation

Temporarily remove:

```text
build/windows-vs2022/bin/Development/assets/models/test_textured.glb
```

Run Development.

Verify:

- diagnostic names `models/test_textured.glb`;
- game continues;
- fallback visual is used;
- runtime does not attempt to load `test_textured_basecolor.png`;
- runtime does not search `.blend`, source, or cooked directories.

Rebuild afterward to restore the staged GLB.

---

## Explicitly Out of Scope

Do not implement:

- Milestone 19;
- Blender CLI/background automation;
- Blender Python export automation;
- CMake invoking Blender;
- cooker invoking Blender;
- generic material system;
- generic texture dependency graph;
- external runtime texture dependency for the M18 model;
- multiple materials on the M18 model;
- multiple image textures on the M18 model;
- normal maps;
- metallic maps;
- roughness maps;
- AO maps;
- emissive maps;
- height maps;
- PBR tuning framework;
- custom shaders;
- shader hot reload;
- material editor;
- texture editor;
- runtime material swapping;
- animation;
- armatures;
- bones;
- skinning;
- rigging;
- skeletal animation;
- morph targets;
- collision meshes;
- Jolt mesh collision;
- navmesh;
- LOD;
- mesh optimization/compression;
- texture compression;
- mipmap pipeline changes;
- VFS;
- asset bundles;
- async asset loading;
- ECS;
- scene graph;
- editor tooling.

---

## Acceptance Criteria

- [ ] 1. One new M18 `.blend` authored source exists.
- [ ] 2. One small locally created M18 authoring PNG exists.
- [ ] 3. One exported `test_textured.glb` exists.
- [ ] 4. The M18 `.blend` is versioned.
- [ ] 5. The M18 authoring PNG is versioned.
- [ ] 6. The exported M18 GLB is versioned.
- [ ] 7. The M18 model is static.
- [ ] 8. The M18 model is low-poly/simple.
- [ ] 9. The M18 model has an explicit UV map.
- [ ] 10. The M18 model uses exactly one simple material.
- [ ] 11. The material uses exactly one Base Color image texture for the milestone test.
- [ ] 12. No normal/metallic/roughness/AO/emissive/height texture pipeline is added.
- [ ] 13. The GLB is self-contained.
- [ ] 14. The GLB has no external runtime URI dependency.
- [ ] 15. The authoring PNG is not a standalone runtime asset.
- [ ] 16. The authoring PNG is absent from the runtime manifest.
- [ ] 17. The `.blend` is absent from the runtime manifest.
- [ ] 18. Cooker processes `models/test_textured.glb`.
- [ ] 19. M18 GLB uses SHA-256 content identity.
- [ ] 20. M15 PNG remains incremental.
- [ ] 21. M16 GLB remains incremental.
- [ ] 22. M17 GLB remains incremental.
- [ ] 23. M18 GLB is incremental.
- [ ] 24. Second unchanged cook does not rewrite outputs meaningfully.
- [ ] 25. Manifest remains deterministic.
- [ ] 26. Manifest remains schemaVersion 1.
- [ ] 27. CMake stages `models/test_textured.glb`.
- [ ] 28. CMake does not stage the M18 authoring PNG separately.
- [ ] 29. Debug receives the M18 GLB.
- [ ] 30. Development receives the M18 GLB.
- [ ] 31. Release receives the M18 GLB.
- [ ] 32. Runtime lookup remains executable-relative.
- [ ] 33. Runtime remains independent of CWD.
- [ ] 34. Runtime never loads the M18 `.blend`.
- [ ] 35. Runtime never loads the M18 authoring PNG directly.
- [ ] 36. M18 model loads once.
- [ ] 37. M18 model unloads correctly before graphics shutdown.
- [ ] 38. M18 model renders visibly.
- [ ] 39. Embedded Base Color texture renders visibly.
- [ ] 40. UV orientation is visually sensible.
- [ ] 41. Model orientation is visually sensible.
- [ ] 42. Model scale is visually sensible.
- [ ] 43. M18 model is visually distinguishable from M15/M16/M17 tests.
- [ ] 44. Missing staged M18 GLB does not crash.
- [ ] 45. Missing staged M18 GLB uses safe visual fallback.
- [ ] 46. Debug/Development metrics expose M18 model status.
- [ ] 47. Release remains free of Dear ImGui/debug UI.
- [ ] 48. No Jolt body or gameplay collision is added for M18.
- [ ] 49. Existing gameplay/physics/camera behavior remains unchanged.
- [ ] 50. Blender remains authoring-only.
- [ ] 51. CMake does not invoke Blender.
- [ ] 52. Cooker does not invoke Blender.
- [ ] 53. No generic material system is introduced.
- [ ] 54. No generic texture dependency system is introduced.
- [ ] 55. No Milestone 19 scope is implemented.

---

## Definition of Done

Milestone 18 is complete when this workflow is proven end-to-end:

```text
technical authored PNG
        +
Blender-authored UV mesh/material
        ↓
versioned .blend authoring source
        ↓
manual self-contained GLB export
        ↓
versioned runtime-source GLB
        ↓
existing deterministic SHA-256 cooker
        ↓
cooked GLB
        ↓
CMake stages one M18 GLB
        ↓
executable-relative runtime lookup
        ↓
Renderer loads GLB once
        ↓
embedded Base Color texture renders correctly
        ↓
Renderer unloads model safely
```

The game must not require Blender or the authored texture PNG at build/runtime once the exported GLB exists.

All M15–M17 asset tests and all existing gameplay, physics, camera, and debug behavior must remain intact.

No Milestone 19 functionality may be included.
