# Milestone 110 — Character Instance Foundation

## Status
Planned

## Goal
Introduce the first reusable runtime `CharacterInstance` abstraction that binds a `CharacterDefinition` to independent per-instance runtime state and animated presentation, without adding level placement/spawning, NPC behavior, Enemy behavior, AI, combat, health, or persistence.

M110 must prove that multiple runtime character instances can coexist while sharing authored definitions/assets and keeping mutable transform, locomotion, playback, pose, and rendering state independent.

## Architectural Position

M110 builds on M101–M109:

`CharacterDefinition` (authored/shared) → `CharacterInstance` (transient runtime/per-instance) → future M111 placement/spawning and later NPC/Enemy systems.

The existing Player remains gameplay authority. M110 creates reusable character-instance infrastructure; it does not migrate Player input, controller, physics, inventory, equipment, checkpoints, or gameplay state into the generic instance.

## Core Runtime Contract

Add a bounded reusable `CharacterInstance` runtime type containing only genuinely per-instance mutable state. At minimum support:
- session-local unique runtime handle/ID (not serialized, no GUID);
- referenced CharacterDefinition textual identity;
- independent world transform;
- typed locomotion presentation state: Idle / Move / Jump;
- independent animation playback time/state;
- resolved presentation validity/diagnostics;
- current skeletal pose/bone matrices when animated;
- Exact / Retargeted / Unavailable presentation mode as applicable.

Do not duplicate immutable CharacterDefinition data unnecessarily.

## Definition Authority

`CharacterDefinition` remains authored authority for identity, metadata, Character Type, World Model, base stats, animation assignments, and humanoid mapping. Instances resolve definitions through the existing gameplay-definition registry.

Missing/invalid definitions fail safely. Do not create a second character database.

## Runtime Identity

Use the narrowest existing-style session-local handle required to distinguish simultaneous instances. It must be unique during the session, transient, and clearly distinct from authored textual identity. No persistence, GUID, or Level Format changes.

## World Transform

Each instance owns an independent world transform using current renderer conventions. Changing one instance must not affect another instance sharing the same CharacterDefinition/model assets. Generic CharacterInstance does not own physics/controller behavior in M110.

## Locomotion Presentation State

Each instance has Idle / Move / Jump presentation state selecting the existing CharacterDefinition locomotion binding. M110 does not decide gameplay or AI transitions; tests/Development diagnostics may set it explicitly.

## Animation Resolution

Reuse M103–M109 production paths:
1. existing embedded/reusable assignment rules;
2. M105 Exact path when exact-compatible;
3. M109 Retargeted path when exact fails and retarget validation succeeds;
4. otherwise safe Unavailable/rest presentation.

Do not implement a third compatibility system, weaken M105, or duplicate M109 math.

## Per-Instance Independence

This is a hard requirement. Two instances sharing the same CharacterDefinition/animation must support different transforms, locomotion states, playback times, and resulting poses. Restarting/updating one must not mutate the other.

Audit Raylib ownership carefully: immutable/heavy resources may be shared where safe, but mutable model pose/bone state must never leak between instances.

## Resource Ownership

Prefer existing caches/narrow extensions for immutable assets, clips, definitions, materials/textures/shaders. Keep playback clocks, selected state, current pose, bone matrices, and transient presentation state per instance. No generalized asset-manager rewrite. Resource lifetime must be explicit and leak-safe.

## Rendering

Add the narrowest production integration required to render multiple CharacterInstances:
- each at its own world transform;
- material presentation preserved;
- static models safe;
- skinned Exact animation supported;
- skinned Retargeted animation supported;
- each uses its own current bone matrices;
- no pose/transform leakage;
- existing Player rendering preserved;
- preserve shadow behavior where the current character path supports it.

A bounded shared render-helper extraction is allowed if it reduces duplication without changing behavior. No broad renderer rewrite.

## Development Diagnostics / Harness

M111 will own authored placement/spawning. M110 MUST NOT add Character entries to Level Format v1.

Provide a bounded Development-only M110 runtime diagnostics/demo surface proving at least two simultaneous instances with independent transforms and playback/state. Prefer existing tiny repository-owned definitions/assets. Where practical demonstrate Exact and Retargeted instances.

Diagnostics should expose runtime handle, CharacterDefinition identity, validity, locomotion state, Exact/Retargeted/Unavailable, bounded playback info, and transform. Avoid per-frame log spam.

This harness is not a placement/spawning feature and must not persist instances.

## Fixtures

Reuse canonical Player and M108/M109 tiny fixtures where practical. Add new deterministic tiny fixtures only if required. No large third-party character and no canonical Player modification merely to simplify M110.

## Player Boundary

Preserve Player controller/physics, M102 stats, M103 locomotion, M104 equipment, M105 exact animation, and current gameplay behavior. Canonical Player Idle/Move/Jump must remain Exact.

A narrow internal reuse of CharacterInstance presentation code is allowed only if clearly justified and regressions remain green. Generic CharacterInstance must not own Player input, physics, inventory, equipment, checkpoints, respawn, or gameplay state.

## Static / Invalid Safety

Static CharacterDefinition models must not crash; they may render statically with animation unavailable/not applicable. Safely handle missing definition/model, load failure, missing animation/definition/source clip, invalid mapping, and unavailable retargeting. One invalid instance must not prevent valid instances from updating/rendering.

## No Runtime Authoring

CharacterInstance is transient runtime state. Do not add instance records to `definitions.gameplay` or `.level`, Save/Reload UI, transform promotion, placement records, or serialization.

## Automated Tests

Cover at least:
1. valid instance creation;
2. unique session handles;
3. multiple instances sharing one definition;
4. definition data not mutated by instance state;
5. independent transforms;
6. independent locomotion states;
7. independent playback clocks;
8. restarting one does not restart another;
9. Exact path reuse;
10. Retargeted path reuse;
11. Exact preferred when available;
12. independent poses/bone matrices;
13. no pose leakage;
14. static model safety;
15. missing definition/model safety;
16. unavailable animation safety;
17. invalid instance does not break valid instance;
18. simultaneous rendering at distinct transforms;
19. offscreen proof of independent animated state;
20. safe cleanup/create-destroy cycles;
21. canonical Player Exact Idle/Move/Jump regression;
22. Player equipment/stats/controller regressions.

Use real production-compatible Raylib loading/rendering where required.

## Render-Boundary Regression

Add a regression proving two concurrent instances render at distinct world positions; advancing/changing instance A does not alter B; destroying A does not invalidate B; and rendered pixels demonstrate consumption of each instance's transform and pose. It must catch accidental sharing of mutable Raylib model/bone state.

## Manual Acceptance

Development build must provide a bounded M110 diagnostics/demo surface where the user can verify:
1. at least two simultaneous runtime instances;
2. distinct runtime handles and visible definition identities;
3. different transforms and simultaneous rendering;
4. changing/cycling one locomotion state does not change the other;
5. animation phases can differ;
6. Exact/Retargeted labels are truthful;
7. pause/restart/state changes on one do not affect the other if controls exist;
8. removing/recreating one does not disturb the survivor;
9. canonical Player gameplay remains normal and Idle/Move/Jump remain Exact;
10. equipment attachment behavior remains intact;
11. no character placement persists into canonical levels.

## Canonical Data Safety

M110 should normally make no semantic changes to:
- `game/assets/source/levels/level_01.level`
- `game/assets/source/levels/level_02.level`

Audit `game/assets/source/gameplay/definitions.gameplay`; prefer existing definitions and preserve Player plus M108/M109 fixtures. Any new fixture must be narrowly justified. No manual-acceptance residue.

## Regression Safety

Preserve Level Format v1, M101–M109 behavior, Player input/controller/physics, Inventory/Equipment, checkpoints/restart/new-run semantics, static rendering, materials, terrain, vegetation, lighting, and shadows.

## Validation

Run relevant focused C++ tests including GameplayDefinitionTest, CharacterDatabaseEditorTest, CharacterAssetValidatorTest, CharacterPreviewTest, AnimationLibraryTest, SkeletalAnimationTest, PlayerPresentationTest, PlayerAnimationAssetTest, EquipmentAttachmentTest, new CharacterInstance tests, and simultaneous-instance render regression.

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

Perform a real Development executable startup smoke. Interactive acceptance remains user-owned.

## Documentation

Document: CharacterDefinition = authored/shared data; CharacterInstance = transient per-instance runtime state; runtime handle != authored identity; Exact/Retargeted paths are reused; M110 does not introduce placement/spawning or AI.

## Explicitly Out of Scope

No Level Format character placement/spawn records, persistent instances, editor placement tools, prefab/entity framework, NPC/Enemy behavior, AI/perception/navigation, health/damage/combat/death/rewards, dialogue, generic inventory/equipment or physics/controller ownership, ragdoll, IK/procedural animation/root-motion gameplay authority, animation graph/blend trees/layers/events, AI-assisted mapping/motion, runtime APIs, FBX/new decoder, generalized asset DB/reimport/watchers, ECS/GUID/JSON/Level Format v2, M107 graphical gizmo tuning, or M111+ work.

## Completion Criteria

M110 is complete only when reusable transient CharacterInstance infrastructure exists; multiple instances coexist with unique handles and independent transforms/state/playback/poses; Exact reuses M105; Retargeted reuses M109; static/invalid characters are safe; simultaneous production rendering is proven without mutable-state leakage; canonical Player behavior remains unchanged; no placement/spawning is introduced; focused tests/builds/Python/startup smoke pass; manual acceptance passes; canonical audits are clean; the user explicitly approves; and Git closure is performed separately.
