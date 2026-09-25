# Milestone 103 — Character Animation Foundation

## Status

Planned.

This milestone starts only after Milestone 102 is fully closed and `main` is clean and synchronized.

## Goal

Establish the first reusable skeletal character-animation runtime for the engine and connect it to the existing Player without introducing a generalized CharacterInstance system, AI, combat, equipment sockets, or a large animation-graph framework.

The milestone should make the canonical Player capable of loading an authored skinned character model with skeletal animation clips and selecting a small typed set of locomotion animations from existing Player runtime state.

The intended architectural flow is:

```text
CharacterDefinition
    -> optional authored animated character asset / animation bindings
    -> reusable skeletal animation runtime
    -> Player animation state selection
    -> animated skinned rendering
```

Milestone 103 is a foundation milestone. It should create the minimum reusable boundary needed for future Player/Enemy/NPC/Animal animation work while keeping the first runtime consumer limited to the existing Player.

## Repository and Workflow Authority

The coding agent must inspect the current repository before implementation.

Authority order:

1. repository code;
2. tests;
3. current repository documentation;
4. this active milestone;
5. latest project checkpoint;
6. older milestone/chat context.

`AGENTS.md` and `DEVELOPMENT_WORKFLOW.md` remain authoritative for repository workflow and validation.

Do not implement future milestone scope opportunistically.

## Branch

```text
milestone/103-character-animation-foundation
```

Create it from a clean, synchronized `main`:

```text
git switch -c milestone/103-character-animation-foundation
git status
```

## Required Scope

### 1. Reusable skeletal animation runtime

Add the minimum engine/runtime support required to evaluate skeletal animation for a skinned character model.

The implementation should reuse the repository's existing model/glTF infrastructure wherever possible rather than creating a parallel asset pipeline.

At minimum, the runtime must have explicit typed concepts for:

- skeleton / joint hierarchy;
- inverse bind data required by skinning;
- animation clips;
- per-joint animated transforms;
- current animation playback time;
- looping playback;
- evaluated skin matrices used by rendering.

The exact C++ type names and file organization should follow the current repository conventions discovered during implementation.

Do not introduce ECS, GUIDs, reflection/property bags, scripting, or a generalized animation graph.

### 2. glTF / GLB skinned-model support

Extend the existing GLB/model path only as far as required for character skeletal animation.

Support the subset actually required by the project's character assets, including:

- skin/joint hierarchy;
- joint indices and weights;
- inverse bind matrices;
- animation channels targeting joint translation, rotation, and scale as required by the source assets;
- animation clip names;
- interpolation modes required by the selected canonical test/player asset.

Unsupported data must fail clearly and safely rather than silently corrupting animation.

Static-model behavior introduced by earlier milestones must remain intact.

### 3. GPU skinning

Render the animated character through skeletal skinning integrated with the existing renderer/shader architecture.

Prefer GPU skinning with evaluated joint/skin matrices supplied by the CPU animation runtime.

Keep the implementation deliberately bounded:

- no compute-skinning framework;
- no generalized animation GPU subsystem;
- no mesh deformation unrelated to skeletal character animation;
- no editor viewport/render-pipeline redesign.

Existing static models, terrain, vegetation, lighting, shadows, and materials must continue to work.

### 4. CharacterDefinition animation authoring

Extend the existing typed `CharacterDefinition` contract only with the minimum authored data needed to identify the animated character presentation and its locomotion clips.

Use the existing `definitions.gameplay` authority and the existing Character Database editor.

The exact representation should be typed and explicit. A suitable shape is:

- optional animated World Model / character model reference, reusing the existing World Model field if the current architecture supports skinned GLB through the same model identity;
- optional typed locomotion animation bindings:
  - Idle;
  - Move;
  - Jump.

If the source GLB contains clip names, authored bindings should refer to those names rather than duplicating animation data.

Do not add arbitrary string-keyed animation maps or a generic property bag.

Missing optional bindings must be visible and fail safely.

### 5. Character Database editor integration

Extend the existing Development Character Database editor instead of creating a second animation editor.

For the selected Character definition, expose the minimum controls needed to author/inspect the M103 animation data.

The editor should make it possible to:

- select/clear the character World Model using the existing model catalog/picker conventions;
- inspect/select available animation clips from the selected model when practical within the existing architecture;
- assign Idle, Move, and Jump bindings;
- distinguish None, resolved, and missing/invalid references;
- Save and Reload through the existing Character Database workflow.

Preserve all M101 Character Database behavior.

Do not create a timeline editor, keyframe editor, animation importer UI, state-machine editor, blend-tree editor, or animation-graph editor.

### 6. Player animation integration

The existing Player is the only required runtime consumer in M103.

Resolve the canonical Player Character identity:

```text
characters/player
```

Use the resolved CharacterDefinition to obtain the animated model and animation bindings.

Drive a small typed Player animation state from existing Player/controller runtime facts.

Required states:

```text
Idle
Move
Jump
```

Expected semantic selection:

- Idle when grounded and horizontal movement is below the existing meaningful movement threshold;
- Move when grounded and moving;
- Jump while airborne.

Use current Player/controller authority for grounded/movement state. Do not duplicate physics authority inside the animation system.

State changes should switch to the corresponding authored clip immediately and deterministically.

### 7. Bounded transition smoothing

Avoid visibly harsh locomotion changes if a small transition blend can be implemented cleanly inside the foundation.

A short cross-fade between the previous and next locomotion clips is allowed and preferred if it does not require a generalized animation graph.

The transition implementation must remain narrowly scoped to clip-to-clip locomotion playback.

Do not implement:

- blend trees;
- 1D/2D blend spaces;
- layered animation;
- additive animation;
- animation masks;
- transition-condition graphs;
- arbitrary state-machine authoring.

If the repository/asset constraints make cross-fade disproportionately invasive, correct deterministic clip switching is the minimum acceptable foundation. The coding agent must document the decision.

### 8. Playback semantics

Idle and Move should loop.

Jump may loop or clamp according to the actual authored clip and the smallest safe runtime design, but the behavior must be deterministic and documented.

Playback time is runtime/transient state and must never be serialized into `definitions.gameplay`, levels, or layout files.

Animation playback must reset/reinitialize correctly when the Player runtime is rebuilt or the active character/model definition changes.

### 9. Player gameplay remains authoritative

Animation is presentation driven by gameplay state.

M103 must not make animation events authoritative for:

- movement;
- jump impulse;
- gravity;
- collision;
- checkpoint behavior;
- inventory/equipment;
- level transitions.

Do not add root motion in M103.

The existing Player controller and M102 effective-stat integration remain gameplay authority.

### 10. Rendering and shadow compatibility

The animated Player should participate in the existing visual pipeline as far as the current renderer architecture reasonably supports.

The primary requirement is correct animated main-pass rendering.

If the existing shadow pass renders the Player/skinned meshes, ensure skinning is also applied there so the shadow matches the animated pose.

Do not redesign the shadow system.

### 11. Development diagnostics

Add concise Development diagnostics sufficient to inspect the foundation at runtime.

At minimum expose:

- active Player Character identity;
- character/model resolution status;
- skeleton/joint count;
- available/selected animation binding status;
- current Player animation state;
- current clip name;
- playback time;
- transition/blend information if cross-fade is implemented.

Diagnostics must not become a new animation editor.

## Canonical Data

The canonical identity remains:

```text
characters/player
```

M103 may intentionally update `characters/player` in:

```text
game/assets/source/gameplay/definitions.gameplay
```

only when required to point at the selected canonical animated Player model and bind Idle/Move/Jump clips.

Any canonical-data change must be minimal, intentional, reviewed, and covered by tests where appropriate.

Do not leave temporary test characters, temporary item definitions, experimental clip bindings, or manual-acceptance fixtures in canonical data.

Canonical level files must not receive incidental changes.

## Tests

Add focused automated regression coverage appropriate to the architecture implemented.

At minimum cover the separable/testable parts of:

- skeletal hierarchy evaluation;
- clip lookup/binding;
- animation sampling at deterministic times;
- looping/clamping semantics;
- translation/rotation/scale interpolation used by the canonical asset;
- skin-matrix generation;
- Player Idle/Move/Jump state selection;
- state/clip switching;
- missing CharacterDefinition/model/clip behavior;
- preservation of static-model loading/rendering contracts where affected;
- CharacterDefinition serialization/parsing for any new M103 fields;
- Character Database Save/Reload behavior for animation bindings.

Tests must not depend solely on visual/manual validation.

## Manual Acceptance

A Development build must be manually tested before M103 can close.

Manual acceptance should verify at minimum:

1. the canonical Player renders as the intended animated/skinned character;
2. the Player is visibly in Idle while grounded and stationary;
3. moving the Player selects and plays Move;
4. jumping/being airborne selects and plays Jump;
5. returning to the ground returns to Idle or Move according to actual movement;
6. transitions do not corrupt pose or leave the Player stuck in an invalid state;
7. Player movement speed, jump behavior, gravity, collision, and M102 stat behavior remain correct;
8. Inventory/Equipment remains functional;
9. Character Database can display and persist the Player animation bindings;
10. Save/Reload of Character Database preserves the intended canonical bindings;
11. missing/cleared animation bindings fail visibly and safely during a temporary test;
12. static models, terrain, vegetation, lighting, and shadows show no obvious regression;
13. if animated shadows are supported by the affected path, the Player shadow follows the animated pose;
14. all temporary manual-test data is removed before Git closure.

Automated green tests are not sufficient to close the milestone.

## Validation

Run the repository-standard validation appropriate to the final implementation.

At minimum:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run the focused/new C++ test executables introduced or affected by M103.

Also run the repository's standard Python regression suites:

```text
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

If the current repository contains additional animation/model/rendering tests relevant to changed code, run those too.

Before reporting completion:

```text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff -- game/assets/source/gameplay/definitions.gameplay
git diff --stat
```

Any intentional canonical gameplay-definition change must be explicitly explained in the report.

## Explicitly Out of Scope

Do not implement any of the following in M103:

- generalized `CharacterInstance`;
- runtime Enemy/NPC/Animal spawning or controllers;
- AI or behavior trees;
- combat, attacks, damage, health gameplay, death, or respawn redesign;
- equipment sockets or visible equipped-item attachment;
- root motion;
- inverse kinematics;
- procedural animation;
- ragdolls;
- facial animation;
- morph-target animation unless strictly required by an already-selected canonical skeletal asset and separately justified;
- animation events driving gameplay;
- animation notifies;
- authored animation state-machine editor;
- generic animation graph;
- blend trees / blend spaces;
- layered or additive animation;
- animation masks;
- retargeting system;
- runtime animation import/hot-reload framework;
- save-game persistence;
- ECS;
- GUID migration;
- JSON conversion;
- Level Format v2;
- generic property bags;
- M104 work.

## Completion Criteria

Milestone 103 is implementation-complete only when:

- reusable skeletal animation evaluation exists;
- the required GLB skin/animation subset is supported;
- the animated Player is skinned and rendered correctly;
- `characters/player` resolves its intended animated presentation;
- typed Idle/Move/Jump animation bindings exist;
- Player locomotion state deterministically selects those animations;
- gameplay/controller authority remains unchanged;
- Development diagnostics expose the required animation state;
- focused automated regressions pass;
- required builds/regression suites pass;
- canonical-data changes are intentional and clean;
- canonical levels contain no incidental changes;
- manual Development acceptance passes.

After implementation and validation, the coding agent must report what changed, validation results, known limitations, and any intentional canonical-data changes, then STOP.

The coding agent must not commit, push, merge, switch branches, close M103, or start M104.
