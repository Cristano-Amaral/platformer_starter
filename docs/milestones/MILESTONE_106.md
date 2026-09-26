# Milestone 106 — Character Asset Validation & Compatibility

## Status
Implemented, awaiting manual acceptance

## Goal

Make character-model suitability and reusable-animation compatibility explicit, inspectable, and testable before gameplay.

Milestone 106 builds directly on the Character Database and skeletal-animation foundations from M101–M105. A user selecting a GLB as a Character World Model should be able to determine whether the asset is usable by the current character pipeline and whether it is exactly compatible with assigned reusable Animation Assets.

This milestone is validation and diagnostics tooling. It does not introduce retargeting or a new character import pipeline.

## Motivation

The current editor can assign any model from the existing static/model catalog as a Character World Model. M103–M105 can animate skinned models and reuse animation clips, but reusable animation compatibility currently requires an exact skeleton match.

A visually valid GLB is not necessarily a valid animated character asset. A model may be static, lack a skin, have no usable skeleton, or have a skeleton incompatible with an assigned reusable animation.

The editor should expose those facts before the user discovers them indirectly in gameplay.

## Authority and Architectural Constraints

- `CharacterDefinition` remains the authored character-data authority.
- `AnimationDefinition` / Animation Library remain the authored reusable-animation authority.
- The existing M103 skeletal loading path remains the production source for skeleton/animation information.
- M105 exact skeleton compatibility remains authoritative:
  - same joint count;
  - same joint name at the same index;
  - same parent index / hierarchy;
  - no remapping.
- Validation must reuse production-compatible inspection/loading logic rather than creating a second independent interpretation of GLB skeletons.
- Editor working-copy/baseline/Save semantics remain unchanged.
- Runtime gameplay must not read Character Database working-copy state.
- Static models remain valid static assets; this milestone must not redefine the general model catalog around character requirements.

## Required Validation Model

Introduce a small typed character-asset validation result suitable for editor presentation and tests.

At minimum it must distinguish:

- source model reference state:
  - None;
  - Resolved;
  - Missing;
  - Malformed / load failure where the existing asset path can distinguish it;
- model character capability:
  - model loaded;
  - skinned versus non-skinned/static;
  - skeleton present;
  - joint count;
  - bounded joint metadata useful for diagnostics;
- reusable-animation compatibility for each authored locomotion binding:
  - None / embedded path;
  - Resolved and Compatible;
  - Missing animation definition;
  - Missing animation source asset;
  - Missing source clip;
  - Malformed/load failure;
  - Skeleton Incompatible.

Use typed enums/structs. Do not use stringly-typed generic status maps.

## Character Database Integration

Extend the existing Character Database rather than creating a separate character editor.

For the selected CharacterDefinition, show a compact Character Asset / Compatibility section that makes the current World Model status understandable.

For a resolved skinned character model, expose at least:

- World Model resolution;
- whether it is skinned;
- skeleton availability;
- joint count;
- a bounded joint-name/hierarchy diagnostic view or equivalent discoverability.

For Idle, Move, and Jump, show compatibility of the currently authored reusable Animation Asset against the selected Character World Model.

The editor must clearly distinguish:

- no reusable asset assigned, where the existing embedded clip path remains applicable;
- a compatible reusable animation;
- an explicitly assigned reusable animation that cannot resolve;
- a resolved reusable animation whose skeleton is incompatible.

Changing the World Model or an Idle/Move/Jump reusable assignment in the Character Database working copy must refresh the validation shown for that working copy without requiring entry into gameplay.

This preview validation must not promote the working copy to active gameplay state and must not change Save/Reload authority.

## Diagnostics

Development diagnostics may expose additional bounded information useful for verifying the production path, but avoid duplicating the Character Database UI unnecessarily.

Existing M105 gameplay diagnostics must continue to report the actual active runtime binding source and compatibility.

## Canonical Validation Assets

Reuse existing repository assets wherever possible.

The canonical M105 pair:

- `models/player.glb`
- `models/humanoid_animations.glb`

must remain the known-compatible character/animation validation case.

Use an existing known static/non-skinned model from the repository for the negative “not an animated character” case if one is stable and appropriate.

Prefer constructing incompatible skeleton fixtures in tests or deriving a tiny bounded test asset only if production-path validation genuinely requires a real GLB.

Do not add large third-party character assets in this milestone.

## Tests

Add focused regression coverage for the validation contract.

At minimum cover:

1. canonical Player World Model resolves as an animated/skinned character asset;
2. its skeleton metadata is reported deterministically;
3. canonical `animations/humanoid_idle`, `animations/humanoid_move`, and `animations/humanoid_jump` report compatible with the canonical Player skeleton;
4. a static/non-skinned model is reported as unsuitable for reusable skeletal animation without crashing;
5. missing World Model is reported safely;
6. missing reusable animation identity is distinguished from skeleton incompatibility;
7. missing animation source asset is safe;
8. missing source clip is safe;
9. exact skeleton mismatch is reported incompatible;
10. changing the Character Database working-copy World Model immediately recomputes editor validation without mutating the active registry;
11. changing/clearing reusable Idle/Move/Jump assignments recomputes editor validation;
12. Save/Reload preserves existing Character Database semantics;
13. production-compatible Raylib loading is exercised for the canonical real GLB path where appropriate, preserving the escaped-crash protection established in M103.

Prefer extending existing Character Database, Animation Library, Player Animation Asset, and skeletal-animation tests when that produces clearer coverage. Add a dedicated validator test if the new typed component warrants it.

## Manual Acceptance

In a Development build:

1. Open Character Database and select `characters/player`.
2. Confirm `models/player.glb` is reported resolved, skinned, and has a valid skeleton.
3. Confirm Idle/Move/Jump reusable assets are reported compatible.
4. Temporarily select a known static/non-character GLB as World Model.
5. Confirm the Character Database reports the model as static/non-skinned or otherwise unsuitable for reusable skeletal animation instead of presenting it as a valid animated character.
6. Confirm Idle/Move/Jump compatibility reflects the changed working-copy model immediately, before Save or entering gameplay.
7. Restore `models/player.glb`.
8. Temporarily clear one reusable locomotion assignment and confirm the UI distinguishes the embedded/no-reusable path from an error.
9. Restore the canonical reusable assignment.
10. Save, Reload, and confirm the canonical values and validation remain correct.
11. Enter gameplay and verify Player Idle/Move/Jump behavior, M104 visible equipment attachment, and M105 diagnostics still behave normally.

Restore all canonical authored data before closure.

## Canonical Data Safety

Before final reporting and Git closure, canonical Player data must remain:

- World Model: `models/player.glb`
- Idle reusable asset: `animations/humanoid_idle`
- Move reusable asset: `animations/humanoid_move`
- Jump reusable asset: `animations/humanoid_jump`

Do not leave manual-validation models, None assignments, malformed identities, test definitions, or temporary stats in `definitions.gameplay`.

Do not make incidental semantic or EOL changes to:

- `game/assets/source/levels/level_01.level`
- `game/assets/source/levels/level_02.level`

## Validation

Run focused affected C++ tests, including at minimum the relevant:

- Character Database editor tests;
- Gameplay Definition tests;
- Animation Library tests;
- skeletal-animation tests;
- Player Presentation tests;
- Player Animation Asset real-Raylib test;
- Equipment Attachment tests;
- new Character Asset validation tests if introduced.

Run:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run the required Python regressions:

```text
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Perform an actual Development executable startup smoke after automated validation.

## Explicitly Out of Scope

Milestone 106 does NOT include:

- skeleton retargeting;
- humanoid bone mapping/remapping;
- Character/Animation 3D preview viewport;
- animation timeline or keyframe editing;
- animation graph/state-machine editor;
- blend trees, layers, additive animation, masks, events;
- root motion;
- IK;
- ragdoll;
- facial animation;
- FBX support or a second model decoder;
- generalized model reimport pipeline;
- filesystem watchers/general hot reload;
- automatic skeleton repair;
- automatic rigging/skinning;
- generalized CharacterInstance;
- Character placement/spawning;
- NPC/Enemy runtime;
- AI/combat/health/damage;
- ECS/GUID/JSON/Level Format v2;
- M107+ features.

## Completion Criteria

Milestone 106 is complete only when:

- character suitability is represented by a typed validation contract;
- Character Database shows useful World Model character/skeleton validation;
- Idle/Move/Jump reusable compatibility is visible against the working-copy World Model;
- exact M105 skeleton compatibility remains authoritative;
- negative/missing/incompatible cases fail safely and are distinguishable;
- working-copy validation does not violate editor/runtime authority;
- canonical real GLB loading remains covered;
- all required builds/tests pass;
- Development startup succeeds;
- manual acceptance passes;
- canonical Player and level data are clean;
- user explicitly approves completion;
- Git closure is performed separately afterward.
