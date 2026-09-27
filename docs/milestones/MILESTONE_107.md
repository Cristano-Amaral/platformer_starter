# Milestone 107 — Character & Animation Preview

## Status
Planned

## Goal
Provide an editor-only 3D preview of the Character Database working copy, using the production skeletal-animation pipeline to inspect the selected World Model and authored Idle/Move/Jump animations without entering gameplay.

M107 builds on M103–M106. M106 answers whether character assets are technically usable and compatible; M107 adds visual inspection while preserving authored-data, validation, and runtime authority.

## Authority and constraints
- CharacterDefinition remains authored authority.
- Character Database workingCopy remains pending editor state; Save/Reload/baseline semantics are unchanged.
- Preview reads workingCopy for editor visualization only and never promotes it to active gameplay.
- Preview state/camera/playback selection are transient and never serialized.
- M106 CharacterAssetValidator remains editor-facing compatibility authority.
- M103/M105 skeletal loading, sampling, playback, skeleton interpretation, and exact compatibility remain production authority.
- Reuse or narrowly extract production animation functionality; do not create a second skeletal-animation interpretation.
- Do not introduce generalized CharacterInstance or generalized editor viewport frameworks.

## Character Database 3D Preview
Extend the existing Character Database with an embedded 3D preview area. Follow current ImGui layout conventions; do not redesign the whole editor.

Display the current working-copy World Model when loadable. A resolved static/non-skinned model may still be displayed as geometry, but skeletal animation must be clearly unavailable. Missing/load-failed models must clear stale geometry and fail safely.

Changing selected character or working-copy World Model must refresh the preview immediately without Save, Reload, gameplay entry, or restart.

## Animation Preview
Provide a bounded selector for the existing locomotion bindings:
- Idle
- Move
- Jump

For an explicitly assigned reusable Animation Asset, preview it only when M106 reports it resolved and exactly compatible. Respect authored playback intent: Loop loops; Clamp clamps at the end.

When no reusable asset is assigned, use the existing authored embedded clip binding consistently with M103/M105 fallback semantics.

An explicitly authored but invalid reusable assignment (missing definition/source/clip, load failure, incompatible skeleton) must NOT silently preview embedded fallback as if valid. Show the validation/status reason and remain safe.

## Playback Controls
Provide only inspection controls:
- Idle/Move/Jump selection;
- Play/Pause (or equivalent);
- Restart/replay;
- visible status sufficient to understand Loop, Clamp, unavailable, or incompatible state.

No timeline, scrubber, keyframe/curve editor, playback authoring, or animation-event editor.

## Preview Camera
Provide character-inspection controls:
- orbit;
- zoom/dolly;
- reset/reframe.

Initial framing and reset/reframe must derive from model bounds, not player.glb-specific magic offsets. Camera state is transient.

## Rendering
Use an editor-owned offscreen render target or the smallest repository-consistent mechanism to display 3D content inside ImGui.

Do not disturb the main world camera/framebuffer/render state. Preserve required renderer state. Reuse existing model/material presentation where practical; do not create a second material system or a full preview studio.

## Production-path reuse
Share or narrowly reuse existing production behavior for:
- skeletal model interpretation;
- skeleton hierarchy;
- clip loading;
- Animation Asset resolution;
- exact skeleton compatibility;
- clip sampling;
- Loop/Clamp;
- animated pose/skinning.

If functionality is trapped in Player/Renderer code, extract only the smallest reusable component required. Preview must not depend on Player physics, movement, M102 stats, Inventory, or Equipment.

## M106 integration
M106 validation directly informs preview:
- resolved/skinned/compatible -> animated preview;
- static/non-skinned -> static geometry may render, skeletal animation unavailable;
- missing World Model -> no stale model;
- explicit missing/invalid reusable asset -> no silent embedded fallback;
- incompatible skeleton -> model inspectable, incompatible animation not sampled;
- None reusable assignment -> existing embedded clip path when available.

Do not implement an independent UI compatibility algorithm.

## Resource lifetime
Preview resources require explicit bounded ownership. Character/model/binding changes invalidate only necessary preview resources. Repeated changes must not leak models, animations, textures, render targets, or GPU resources. Destruction/closure releases preview-owned resources safely. No generalized hot reload.

## Tests
Add focused non-pixel-perfect regression coverage. At minimum cover:
1. canonical player working-copy model is previewable;
2. canonical Idle/Move/Jump resolve compatible reusable assets;
3. Idle/Move preserve Loop and Jump preserves Clamp;
4. None reusable assignment selects embedded path;
5. explicit invalid reusable assignment does not silently use embedded fallback;
6. missing definition/source/clip are safe;
7. incompatible skeleton is safe and not sampled;
8. static/non-skinned model remains safely inspectable where supported but cannot skeletal-animate;
9. missing World Model clears preview state;
10. working-copy World Model and locomotion changes refresh preview without mutating active registry;
11. switching characters clears stale preview state;
12. playback Loop/Clamp state is deterministic;
13. resource replacement/release is covered where practical;
14. existing real-Raylib canonical Player asset protection remains passing.

Prefer testing extracted preview/controller/resolution logic independently from ImGui. Add a focused CharacterPreview test if warranted.

## Manual acceptance
In Development:
1. Open Character Database and select characters/player.
2. Confirm models/player.glb appears in 3D preview.
3. Confirm automatic framing.
4. Orbit, zoom, and reset/reframe.
5. Preview Idle; confirm looping.
6. Pause/restart.
7. Preview Move; confirm looping.
8. Preview Jump; confirm Clamp behavior.
9. Without Save, select a known static/non-character GLB; confirm immediate viewport change, M106 static/non-skinned status, and unavailable skeletal animation.
10. Without Save, restore models/player.glb; confirm immediate recovery.
11. Temporarily clear reusable Idle; confirm embedded Idle path works rather than reporting an error.
12. Restore animations/humanoid_idle.
13. Save/Reload and confirm canonical values.
14. Enter gameplay and verify Player Idle/Move/Jump remain normal.
15. Verify M104 visible equipment remains normal if available.
16. Return to Character Database and confirm preview initializes normally.

Restore canonical authored data before closure.

## Canonical data safety
Player must finish with:
- World Model: models/player.glb
- embedded Idle: Idle
- embedded Move: Move
- embedded Jump: Jump
- reusable Idle: animations/humanoid_idle
- reusable Move: animations/humanoid_move
- reusable Jump: animations/humanoid_jump

No manual-test residue in definitions.gameplay. No incidental semantic/EOL changes to level_01.level or level_02.level.

## Regression safety
Preserve M101 Character Database authority, M102 stats, M103 skeletal animation/real-Raylib safety, M104 attachments, M105 reusable animations/exact compatibility/fallback/runtime refresh, M106 validation, Inventory/Equipment, static rendering, materials, terrain, vegetation, lighting, and shadows.

## Validation
Run relevant C++ tests including Character Database, CharacterAssetValidator, GameplayDefinition, AnimationLibrary, SkeletalAnimation, PlayerPresentation, PlayerAnimationAsset, EquipmentAttachment, and new preview tests.

Run:
```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:
```text
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Perform a real Development executable startup smoke. Interactive preview acceptance remains the user's responsibility.

## Documentation
Update only repository documentation required by M107 and existing conventions.

## Explicitly out of scope
No retargeting; humanoid mapping/remapping; skeleton repair; rigging/skinning; timeline/scrubber/keyframes/curves; bone manipulation; animation graph/state machine; blend trees/layers/additive/masks/events; root motion authoring; IK; ragdoll; facial animation; preview equipment/loadout authoring; generalized CharacterInstance; placement/spawning; NPC/Enemy runtime; AI/combat/health/damage; FBX/new decoder; generalized reimport/watchers/hot reload; generalized viewport framework; ECS/GUID/JSON/Level Format v2; or M108+.

## Completion criteria
M107 is complete only when the working-copy World Model has a safe embedded 3D preview; unsaved model/binding changes update immediately; Idle/Move/Jump can be visually previewed through production-compatible animation logic; None vs explicit-invalid fallback semantics are preserved; Loop/Clamp works; bounds-derived orbit/zoom/reframe works; invalid/static/missing cases avoid stale state/crashes; M106 remains compatibility authority; preview does not mutate active/persisted state; resources are bounded; builds/tests/startup pass; manual acceptance passes; canonical data is clean; the user approves; and Git closure is performed separately.
