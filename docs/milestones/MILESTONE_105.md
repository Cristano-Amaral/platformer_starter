# Milestone 105 — Reusable Animation Assets & Animation Library Foundation

## Status

Implemented, awaiting manual acceptance.

## Goal

Promote skeletal animation clips from an implementation detail embedded only inside a Character's World Model into reusable authored animation assets that can be discovered, validated, referenced, and assigned to CharacterDefinition locomotion bindings.

M105 builds on the closed M103 Character Animation Foundation and preserves its runtime authority. It does not introduce a generalized animation graph, retargeting system, or in-editor keyframe authoring.

The intended pipeline becomes:

```text
Animated character model / skeleton
        +
Reusable animation asset
        ↓
CharacterDefinition typed Idle / Move / Jump bindings
        ↓
M103 skeletal animation playback
        ↓
Player presentation
```

An animation may still come from a clip embedded in the Character World Model. M105 adds a bounded reusable-asset path rather than removing the existing embedded-clip path.

## Motivation

M103 proved the skeletal animation runtime using a self-contained `player.glb` with embedded `Idle`, `Move`, and `Jump` clips. That is appropriate for the foundation but does not scale well to a content pipeline where multiple game-ready characters should share a library of compatible humanoid animations.

M105 establishes animation as first-class authored content while keeping creation of meshes, rigs, skin weights, and animation keyframes in external DCC/AI tools.

This milestone is especially intended to support a future workflow where AI-generated game-ready characters conform to a known skeleton contract and reuse project animation assets.

## Required Scope

### 1. Animation Asset Identity

Add a stable textual authoring identity for reusable animation assets.

Use a bounded identity category consistent with the repository's existing textual-identity conventions, for example:

```text
animations/humanoid_idle
animations/humanoid_walk
animations/humanoid_jump
```

The exact persistence representation should follow current repository conventions and remain deterministic.

Do not introduce GUIDs, JSON, or a generic asset database framework.

### 2. Animation Asset Contract

Define the minimum typed metadata needed for a reusable skeletal animation asset.

At minimum it must identify:

- the source animation-bearing asset;
- the source clip name inside that asset;
- compatibility information sufficient for safe assignment/playback under the bounded M105 rules;
- deterministic loop/clamp playback intent if this belongs naturally at the reusable asset level.

Avoid duplicating skeleton/clip data already obtainable from the source asset.

The contract must distinguish at least:

- resolved;
- missing source asset;
- missing source clip;
- incompatible skeleton;
- malformed authoring.

### 3. Source Format

Reuse the existing GLB/glTF animation support from M103 where practical.

A reusable animation source may be a GLB containing skeleton/animation data without requiring it to be the Character's rendered World Model.

Do not build a new FBX importer in M105.

Do not require every animation to be extracted into a custom binary format.

### 4. Skeleton Compatibility

M105 must define and enforce a bounded compatibility contract for applying a reusable animation to a character skeleton.

Prefer exact structural compatibility based on the information already available to the runtime, such as joint names/order/hierarchy and required transforms, rather than implementing retargeting.

Compatible assets must be deterministic and safe.

Incompatible assets must fail diagnostically and must not corrupt pose evaluation.

Retargeting is explicitly out of scope.

### 5. Animation Catalog / Library

Add a bounded catalog/library for reusable animation assets, following existing repository patterns such as the StaticModelCatalog where appropriate without forcing an inappropriate abstraction.

It must support the needs of Development authoring:

- discover authored animation identities;
- resolve their source assets/clips;
- expose compatibility/resolution status;
- refresh deterministically when the existing content workflow requires it.

Do not introduce filesystem watching or a generalized hot-reload framework.

### 6. Content Browser Integration

Expose reusable animation assets in the Development Content Browser in a manner consistent with the current browser architecture.

The user must be able to discover animation assets without treating them as ordinary static models.

A simple animation-specific presentation is sufficient. Do not build animated thumbnail rendering unless it is already trivial with current infrastructure.

Preserve existing model, texture, material, terrain, and other Content Browser behavior.

### 7. CharacterDefinition Typed Bindings

Evolve the M103 typed locomotion bindings so `Idle`, `Move`, and `Jump` can reference reusable animation assets.

Preserve a bounded typed contract.

Do not replace these fields with a generic string map or arbitrary state table.

Existing M103 embedded-clip authoring must either:

- remain backward compatible; or
- have a narrow deterministic migration path if repository inspection proves that a representation change is cleaner.

Do not silently break existing definitions.

### 8. Character Database Editor

Extend the Development Character Database so Idle / Move / Jump bindings can be assigned from the reusable Animation Library.

The editor should make it practical to:

- inspect the selected Character World Model/skeleton;
- select or clear a reusable animation asset for Idle;
- select or clear one for Move;
- select or clear one for Jump;
- distinguish resolved, missing, malformed, and incompatible assignments;
- Save/Reload using the established working-copy/baseline authority.

Where useful, show source clip information and compatibility status.

Do not build a timeline, curve editor, skeleton editor, or state-machine graph.

### 9. Player Runtime Integration

The Player remains the only required runtime consumer.

M103 remains authoritative for:

- Idle / Move / Jump state selection;
- playback time;
- loop/clamp behavior unless deliberately moved into the reusable typed asset contract;
- cross-fade behavior;
- skinning and rendering;
- presentation-only animation authority.

M105 changes where a bound clip can be sourced from, not the gameplay state machine.

A reusable animation assignment must feed the existing M103 playback path rather than creating a second animation runtime.

### 10. Embedded Clip Compatibility

The canonical M103 path must remain safe.

Characters using clips embedded in their World Model must continue to function unless a deliberate canonical migration is performed as part of M105.

If the canonical `characters/player` is migrated to reusable animation identities, the migration must be minimal, deterministic, documented, and covered by tests.

### 11. Canonical Demonstration Assets

Provide the minimum repository-owned animation assets necessary to prove the reusable path.

Prefer deriving/splitting the existing tiny canonical M103 animation data rather than adding large external art assets.

The canonical demonstration must prove that at least one Player locomotion animation can be sourced independently from the Player World Model.

Ideally prove Idle, Move, and Jump through reusable assets if this remains small and deterministic.

Do not add third-party art packs.

### 12. Development Diagnostics

Extend Development diagnostics with bounded information sufficient to inspect the active Player animation source.

At minimum expose:

- current locomotion state;
- bound animation identity or embedded-clip source;
- source asset resolution;
- source clip name;
- compatibility status;
- current clip/playback time;
- blend state where already provided by M103.

### 13. Runtime Asset Staging

Ensure reusable animation source assets and any required authored metadata are correctly staged for Debug/Development/Release according to the existing asset pipeline.

Development source/runtime authority must remain consistent with existing project conventions.

Do not introduce a second unrelated staging pipeline.

## Tests

Add focused automated coverage for the final implementation, including at minimum:

- animation textual identity validation;
- animation metadata parsing/serialization if persisted in gameplay definitions or another authored text file;
- deterministic round trip;
- catalog discovery/resolution;
- missing source asset;
- missing source clip;
- compatible skeleton resolution;
- incompatible skeleton rejection;
- reusable clip sampling through the M103 runtime;
- loop/clamp behavior;
- CharacterDefinition Idle/Move/Jump reusable bindings;
- Character Database Save/Reload;
- safe missing/incompatible binding behavior;
- canonical reusable animation asset loading through the real Raylib/GLB path where applicable;
- M103 embedded-animation regression;
- M103 Player state-selection/cross-fade regression;
- M104 visible-equipment attachment regression while the Player is animated;
- static-model/material/rendering regressions affected by the implementation.

Remember the M103 escaped-test lesson: structural parsing tests alone are insufficient when a real Raylib loader path is involved.

## Manual Acceptance

Manual Development acceptance is mandatory.

The user must be able to verify at minimum:

1. Normal startup.
2. Character Database exposes reusable animation assignments for Player Idle/Move/Jump.
3. Animation assets can be discovered/selected/cleared.
4. Resolved compatible assignments are visibly used by the Player.
5. Idle still plays while grounded/stationary.
6. Move still plays while grounded/moving.
7. Jump still plays while airborne.
8. Existing M103 cross-fade remains stable.
9. Missing animation assignment is safe.
10. Incompatible animation assignment is clearly diagnosed and safe.
11. M104 visible equipment still follows the animated Player.
12. Player movement, jump, gravity, collision, M102 stats, Inventory, and Equipment remain unchanged.
13. Static models, terrain, vegetation, materials, lighting, and shadows show no obvious regression.

## Canonical Data Safety

Before closure:

- `game/assets/source/levels/level_01.level` must have no incidental diff.
- `game/assets/source/levels/level_02.level` must have no incidental diff.
- Any intentional `definitions.gameplay` change must contain only the canonical M105 migration/additions.
- No temporary animation definitions, test characters, or manual acceptance fixtures may remain.
- Do not mechanically normalize line endings.

## Explicitly Out of Scope

M105 does **not** implement:

- animation retargeting;
- humanoid bone remapping;
- automatic adaptation between incompatible rigs;
- in-editor keyframe creation;
- animation timeline;
- curve editor;
- pose editor;
- skeleton/rig authoring;
- skin-weight painting;
- generic animation state-machine editor;
- generic animation graph;
- blend trees;
- animation layers;
- additive animation;
- bone masks;
- animation events/notifies;
- root motion;
- IK;
- procedural animation;
- ragdolls;
- facial animation;
- generalized CharacterInstance;
- Enemy/NPC/Animal runtime or spawning;
- AI;
- combat/damage/death;
- attack gameplay;
- equipment sockets redesign;
- skinned armor/clothing;
- FBX importer;
- generalized asset database rewrite;
- filesystem hot reload/watchers;
- save-game persistence;
- ECS migration;
- GUID migration;
- JSON conversion;
- Level Format v2;
- M106 work.

## Validation

Run repository-required validation and all affected tests.

At minimum:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run relevant C++ tests for:

- GameplayDefinition;
- Character Database;
- skeletal animation;
- Player presentation;
- canonical Player animation asset loading;
- new reusable-animation catalog/asset logic;
- M104 equipment attachment;
- affected model/material/rendering systems.

Run required Python suites:

```text
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Run additional tests required by `AGENTS.md` or `DEVELOPMENT_WORKFLOW.md`.

Before reporting completion:

```text
git branch --show-current
git status
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff -- game/assets/source/gameplay/definitions.gameplay
git diff --stat
```

## Completion Gate

The coding agent must STOP after implementation, validation, audits, and final report.

No commit, push, merge, branch switch, milestone closure, or M106 work is authorized during the implementation run.

The user performs local manual acceptance before Git closure.
