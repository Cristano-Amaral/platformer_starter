# Milestone 19 — Asset Cooker Texture Optimization Foundation

## Status

**Complete.** Standalone runtime PNGs use cooker recipe `runtime_png.max512.lanczos.v1` with cooker-only Pillow 12.3.0. GLBs remain opaque copies. Embedded GLB images and Blender authoring PNGs are not transformed.

## Goal

Extend the existing asset pipeline with the first conservative **texture-processing step** while preserving the architecture proven in Milestones 15–18.

Milestone 19 introduces deterministic image validation and optional size normalization for runtime PNG texture assets handled by the cooker. It does **not** introduce GPU texture compression, platform-specific formats, a general material system, or changes to textures embedded inside GLB files.

The milestone should prove this pipeline:

```text
runtime-source PNG
      ↓
Python cooker
      ↓
validate dimensions / format
      ↓
conservative deterministic resize when required
      ↓
cooked PNG
      ↓
CMake staging
      ↓
runtime
```

The existing Blender-authored textured GLB workflow remains unchanged:

```text
Base Color PNG (authoring only)
      ↓
Blender
      ↓ embedded into GLB
self-contained GLB
      ↓
existing cooker as opaque runtime asset
```

The cooker must **not** extract, resize, recompress, or otherwise modify images embedded inside GLB files in M19.

---

## Why this milestone now

Milestones 15–18 established:

- deterministic SHA-256 asset cooking;
- source → cooked → staged → runtime separation;
- executable-relative runtime paths;
- PNG runtime assets;
- static GLB runtime assets;
- Blender-authored GLB assets;
- embedded Base Color textures inside self-contained GLBs.

The next useful pipeline step is to establish a small, measurable texture policy before introducing more assets.

This also prepares for constrained targets such as Raspberry Pi-class hardware without prematurely implementing DDS, KTX2, Basis Universal, GPU compression, texture streaming, or platform-specific asset variants.

---

# 1. Scope

M19 applies **only to standalone runtime PNG textures that are explicit cooker assets**.

Initially this includes the existing runtime texture:

```text
game/assets/source/textures/test_checker.png
```

The M18 authoring texture:

```text
game/assets/source/textures/test_textured_basecolor.png
```

is **not** a runtime cooker asset and must remain excluded from the manifest/staging/runtime lookup.

Images embedded inside:

```text
models/test_authored.glb
models/test_textured.glb
```

must remain opaque to the cooker.

---

# 2. Texture policy

Introduce an explicit project-owned runtime PNG policy.

For M19:

```text
preferred maximum dimension: 512 px
supported source format: PNG
output format: PNG
aspect ratio: preserved
upscaling: forbidden
```

If both source dimensions are already `<= 512`, preserve the original dimensions.

If either source dimension is `> 512`, resize proportionally so the largest output dimension is exactly `512` pixels.

Examples:

```text
256 × 256   → 256 × 256
512 × 256   → 512 × 256
1024 × 1024 → 512 × 512
1024 × 512  → 512 × 256
400 × 800   → 256 × 512
```

Do not crop.
Do not stretch.
Do not upscale.

---

# 3. Dependency policy

The existing cooker is Python standard-library-only.

Do **not** silently add Pillow, ImageMagick, Blender, or another dependency merely to implement resizing.

Phase A must first determine whether the required PNG processing can be implemented cleanly with the current dependency policy.

If reliable PNG resizing would require a new dependency, stop and report that architectural decision rather than adding it automatically.

The preferred outcome is to keep the cooker dependency-light, but correctness is more important than writing a fragile custom PNG/image implementation.

Do not implement a custom PNG decoder/resampler from scratch merely to preserve the standard-library-only rule.

A dependency change, if ultimately needed, requires explicit approval before implementation.

---

# 4. Asset metadata

Extend the cooker/reporting only as much as needed to make texture processing observable.

For standalone runtime PNG assets, the cooker should be able to report at least:

```text
logical id
source dimensions
cooked dimensions
whether resize was required
source SHA-256 / content identity behavior consistent with existing cooker
```

Do not turn the manifest into a general asset database.

Keep `schemaVersion: 1` if the existing schema can accommodate the required information without ambiguity. If a manifest schema change is genuinely required, explain why before changing it.

Determinism remains mandatory:

- stable ordering;
- no timestamps;
- no absolute paths;
- no machine-specific metadata.

---

# 5. Incremental cooking

Preserve the existing incremental behavior.

The cooker must not rewrite a cooked PNG when:

- source content is unchanged; and
- the relevant texture-processing policy is unchanged.

If processing policy affects cooked output, the cooker must account for that so a future policy change cannot incorrectly reuse stale output.

Do not rely on file modification time as content identity.

The implementation should remain simple and explicit.

---

# 6. Test asset strategy

Do not modify the visual appearance of the existing M15 checker merely to force a resize test.

Use the smallest clean technical validation strategy.

Acceptable approaches include:

- a temporary/generated test fixture used only by cooker tests; or
- a small deterministic source test PNG specifically introduced for M19 if it is clearly justified.

Do not add unnecessary runtime-visible assets solely for automated testing.

The test must prove at least:

1. PNG already within limit remains the same dimensions.
2. PNG above the limit is reduced proportionally.
3. Aspect ratio is preserved.
4. No upscaling occurs.
5. Re-running unchanged input does not rewrite output.
6. Invalid/unsupported input fails clearly rather than producing corrupt output.

---

# 7. Runtime behavior

M19 must not require a new runtime texture API.

The existing M15 checker should continue to load from:

```text
assets/textures/test_checker.png
```

through the established executable-relative runtime path abstraction.

The runtime does not need to know whether the cooker resized a texture.

No gameplay code should know about texture dimensions or cooker policy.

---

# 8. Debug/Development diagnostics

Extend the existing read-only Assets metrics only if useful and cheap.

A suitable M19 diagnostic for the existing standalone checker texture could include:

```text
Standalone Runtime Texture
  loaded: true/false
  id: textures/test_checker.png
  runtime width: ...
  runtime height: ...
  fallback: true/false
```

Do not add editable controls.
Do not add texture reload.
Do not add an asset browser.
Do not expose cooker internals to gameplay.

Release remains free of Dear ImGui.

---

# 9. Documentation

Update the asset documentation to clearly distinguish three categories:

### A. Standalone runtime texture

Example:

```text
textures/test_checker.png
```

Consumed directly by the cooker and staged as a runtime PNG.

### B. Blender authoring texture

Example:

```text
textures/test_textured_basecolor.png
```

Used by Blender and versioned as source, but not directly cooked/staged for runtime.

### C. Texture embedded inside GLB

Example:

```text
models/test_textured.glb
```

The cooker treats the GLB as an opaque runtime asset in M19. Embedded image data is not independently processed.

Document the M19 maximum-dimension policy and the fact that it is a **foundation policy**, not the final multiplatform texture-compression solution.

---

# 10. Preserve existing architecture

Do not change gameplay tuning:

```text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10
jump buffer          = 0.10
```

Preserve:

- CharacterVirtual;
- fixed gameplay Z;
- moving platform and carry behavior;
- 30-degree walkable slope;
- 60-degree steep slope;
- platformer camera;
- cyan dynamic Jolt test box;
- greybox world;
- M15 checker;
- M16 static GLB;
- M17 Blender-authored GLB;
- M18 textured self-contained GLB;
- executable-relative runtime paths;
- current CMake staging architecture.

No unrelated gameplay, physics, camera, rendering, or architecture refactors.

---

# 11. Explicitly out of scope

Do **not** implement in M19:

- Milestone 20;
- DDS;
- KTX/KTX2;
- Basis Universal;
- BCn/ETC/ASTC compression;
- GPU texture compression;
- platform-specific texture variants;
- automatic texture atlases;
- texture arrays;
- texture streaming;
- async loading;
- virtual textures;
- generic texture manager;
- generic material system;
- custom shaders;
- normal-map pipeline;
- metallic/roughness pipeline;
- emissive pipeline;
- occlusion pipeline;
- extraction of textures from GLB;
- modification of embedded GLB images;
- Blender automation;
- Blender CLI integration;
- cooker invoking Blender;
- CMake invoking Blender/cooker;
- hot reload;
- VFS;
- PAK/bundles;
- LOD;
- billboards;
- instancing;
- mesh optimization;
- editor tooling.

---

# 12. Recommended implementation phases

## Phase A — Design and dependency feasibility

Cursor should:

1. Inspect the existing cooker and asset contracts.
2. Define the smallest texture-processing abstraction needed.
3. Determine whether reliable resize support is possible without a new Python dependency.
4. Prepare documentation/tests/contracts without changing runtime behavior unnecessarily.
5. Stop for review if a new image-processing dependency is required.

Phase A must not silently install dependencies.

**Phase A result:** Python stdlib can read PNG IHDR dimensions but cannot decode/resample/encode PNG. raylib's stb is a runtime/build FetchContent dependency, not a cooker API. No suitable resize capability exists in-tree. Do not implement a custom PNG codec. Recommended Phase B dependency (needs explicit approval): pinned Pillow, cooker-only, not a CMake/runtime dependency. `test_checker.png` is 16×16 and cannot prove downscale; Phase B should add a dedicated oversized standalone PNG such as `textures/test_large_checker.png` (1024×512 or 1024×1024), not the M18 authoring PNG. Incremental skip must recompute cook output under the current `runtime_png.max512.v1` recipe so a max-dimension policy change recooks. Keep `schemaVersion` 1; optional `recipe`/dimension fields may be added later without bumping schema. Embedded GLB images stay opaque copies.

## Phase B — Texture cooker implementation

After Phase A approval:

1. Implement validated PNG dimension handling.
2. Implement deterministic proportional downscaling only if the approved dependency strategy supports it.
3. Add focused cooker tests/fixtures.
4. Preserve SHA-256 incremental behavior.
5. Keep GLB processing opaque.

**Phase B result:** Pillow `12.3.0` is a cooker-only pin in `tools/requirements.txt`. Recipe `runtime_png.max512.lanczos.v1` downscales declared runtime PNGs with LANCZOS. Within-limit PNGs stay byte-identical. Test fixture `tools/fixtures/textures/test_large_checker.png` (1024×512) is not a runtime asset. Embedded GLB images remain opaque copies.

## Phase C — Integration and final validation

1. Cook assets twice.
2. Configure/build Debug, Development, Release.
3. Verify staging.
4. Run Development.
5. Verify standalone checker still renders correctly.
6. Verify M18 embedded-texture GLB remains correct.
7. Verify Release from an unrelated CWD.
8. Verify no unintended authoring PNG is staged.
9. Review Debug/Development metrics if added.
10. Perform final Git review before commit.

**Phase C result:** Tooling tests pass. Two cooker runs skip all runtime assets and the manifest. `test_checker.png` remains 16×16 byte-identical. Fixture and M18 Base Color PNG stay out of runtime staging. Debug/Development/Release build. Pillow remains cooker-only.

---

# 13. Acceptance criteria

Milestone 19 is complete when all of the following are true:

- standalone runtime PNG texture policy is documented;
- maximum dimension is explicitly `512 px`;
- aspect ratio is preserved;
- no upscaling occurs;
- PNG source/output policy is explicit;
- processing is deterministic;
- incremental cooking remains correct;
- source-content changes trigger recooking;
- relevant processing-policy changes cannot silently reuse stale cooked output;
- invalid inputs fail clearly;
- cooker tests cover within-limit and over-limit images;
- M15 checker still renders;
- M16/M17 GLBs still render;
- M18 embedded Base Color GLB still renders;
- M18 source Base Color PNG remains absent from runtime staging;
- embedded GLB images remain untouched by the cooker;
- Debug, Development, and Release build successfully;
- Release remains independent of current working directory;
- no gameplay/physics tuning changes;
- no unapproved image-processing dependency was introduced;
- no Milestone 20 work was started.

---

# 14. Final report requirements

At completion, report:

1. implementation summary;
2. files created/modified;
3. final standalone runtime texture policy;
4. dependency decision;
5. PNG validation implementation;
6. resize implementation, if approved/implemented;
7. resizing algorithm/library used;
8. deterministic-output behavior;
9. incremental-cooking behavior;
10. how processing-policy changes invalidate stale output;
11. test assets/fixtures used;
12. within-limit test result;
13. over-limit test result;
14. aspect-ratio test result;
15. no-upscale test result;
16. invalid-input test result;
17. first cooker result;
18. second cooker result;
19. manifest behavior/schema;
20. confirmation authoring Base Color PNG remains outside runtime manifest;
21. confirmation GLB embedded images are untouched;
22. CMake staging result;
23. Debug build result;
24. Development build result;
25. Release build result;
26. runtime validation performed;
27. M15 validation;
28. M16 validation;
29. M17 validation;
30. M18 embedded-texture validation;
31. Release unrelated-CWD validation;
32. Debug Metrics changes, if any;
33. confirmation Release has no ImGui;
34. confirmation gameplay/physics tuning unchanged;
35. confirmation no unrelated dependency was added;
36. confirmation Milestone 20 was not started.

Do not commit, push, or merge until manual validation and approval are complete.
