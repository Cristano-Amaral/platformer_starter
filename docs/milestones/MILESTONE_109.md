# Milestone 109 — Animation Retargeting Foundation

## Status
Implementation in progress

## Goal
Introduce a bounded humanoid animation-retargeting path that uses the M108 Humanoid Skeleton Mapping to transfer reusable skeletal animation motion from a different humanoid source skeleton onto a mapped target character skeleton, while preserving the existing M105 exact-compatible path unchanged.

M109 must prove one narrow capability:

> A reusable animation whose source skeleton is not exact-compatible with a target character can be previewed on that target when both sides have valid humanoid semantic mappings and the retargeting contract is satisfied.

M109 is a retargeting foundation, not a general animation system rewrite.

## Architectural Position

Existing path:

```text
Reusable Animation
      |
      v
Exact skeleton compatibility (M105)
      |
      +-- compatible --> existing playback
      |
      +-- incompatible --> unavailable
```

M109 adds a second explicit path:

```text
Reusable Animation source skeleton
      |
      +--> exact-compatible? --> existing M105 playback (preferred)
      |
      +--> otherwise:
             source humanoid mapping
                    +
             target humanoid mapping
                    |
                    v
             M109 retarget validation
                    |
                    v
             retargeted target pose
```

Exact compatibility remains the preferred, unchanged path. Retargeting is used only when exact compatibility fails and an explicitly valid retargeting relationship exists.

## Authorities and Boundaries

- M103 remains skeletal animation/playback authority.
- M105 exact compatibility remains unchanged.
- M106 asset validation remains authoritative for model/skeleton suitability and exact compatibility.
- M107 Preview remains the primary editor surface for inspecting CharacterDefinition animation playback.
- M108 Humanoid Skeleton Mapping remains semantic role authority.
- Character Database working-copy semantics remain unchanged.
- Retargeting must not mutate source animation assets, source skeletons, target skeletons, or authored mappings.
- No second GLB/glTF animation/skeleton decoder.
- Reuse production Raylib-loaded skeleton/animation data and existing pose infrastructure.

## Source Humanoid Mapping

M108 maps CharacterDefinition target skeletons. M109 also needs a semantic mapping for the reusable animation's source skeleton.

Add the narrowest typed authored source-skeleton mapping authority consistent with current AnimationDefinition architecture.

Requirements:
- reuse the same `HumanoidJointRole` contract from M108;
- persist source joint names, not transient indices;
- deterministic persistence;
- stale/missing names remain diagnosable;
- no duplicate role fields;
- no GUID/JSON/new asset database;
- no duplicated independent role enum.

If a reusable animation source GLB is also referenced by an existing CharacterDefinition with a usable identical skeleton mapping, implementation may provide a bounded helper to derive/copy suggestions, but do not create hidden cross-definition authority. The animation definition must have a deterministic source mapping available when retargeting is requested.

## Retarget Compatibility

Introduce a typed retarget validation result distinct from exact compatibility.

At minimum distinguish:
- Not Requested / None
- Source Mapping Missing or Incomplete
- Target Mapping Missing or Incomplete
- Invalid Mapping
- Unsupported Skeleton/Animation
- Retargetable

Diagnostics must explain why retargeting is unavailable.

Retargeting must require:
- valid source animation asset/clip;
- usable source skeleton;
- usable target skeleton;
- valid/usable source humanoid mapping;
- valid/usable target humanoid mapping;
- required roles available on both sides;
- structurally valid mapped chains.

Do not call two skeletons exact-compatible merely because they are Retargetable.

## Retargeting Math — Bounded Foundation

Implement a deterministic local-pose retargeting foundation based on mapped humanoid roles and each skeleton's rest/bind pose.

The intended contract is to transfer animation delta relative to the source rest pose into the target's corresponding mapped joint relative to the target rest pose, rather than copying absolute source joint transforms directly.

For mapped rotational motion, use a mathematically sound rest-pose-relative transform so differently oriented source/target bones can receive corresponding motion.

For translation:
- preserve target rest translations by default for non-root mapped joints;
- do not blindly copy source local translations to differently proportioned target limbs;
- bounded root/hips translation handling may be implemented only as needed for the canonical test and must be documented.

Scale:
- do not introduce generalized per-bone scale retargeting;
- preserve target rest scale unless the current production animation contract requires a narrow safe behavior.

The implementation must be deterministic and bounded. Do not add IK or runtime solvers to hide retargeting errors.

## Required Role Application

Retarget only mapped humanoid roles.

Unmapped target joints must remain at their target rest pose unless they are affected through normal hierarchy propagation from a mapped ancestor.

Optional M108 roles (Chest, Neck, Shoulders) may participate when valid on both source and target.

If an optional role exists only on one side, the foundation may omit direct retargeting for that role rather than inventing motion redistribution.

Do not implement twist-bone distribution, finger propagation, or generalized intermediate-bone solving.

## Exact-Compatible Fast Path

This is a hard regression boundary.

If source and target skeletons are exact-compatible under M105:
- continue using the existing exact playback path;
- do not route through retargeting unnecessarily;
- preserve existing visual behavior and tests.

M109 must not change canonical Player exact-compatible Idle/Move/Jump behavior.

## Character / Animation Preview

Extend M107 Preview so the user can inspect retargeted animation on a mapped target character.

When a reusable assignment is exact-compatible:
- status remains the existing exact-compatible behavior.

When exact compatibility fails but M109 retarget validation succeeds:
- Preview may play the retargeted animation;
- status must clearly identify it as Retargeted, not Exact;
- Play/Pause/Restart and Loop/Clamp continue to work;
- offscreen skinned rendering must consume the retargeted target pose;
- no stale geometry/pose when switching characters/models/animation slots.

When neither exact nor retarget path is valid:
- remain Unavailable with useful diagnostics.

## Character Database / Animation Authoring UI

Add only the UI needed to author and inspect source humanoid mapping and retarget status.

Prefer extending the existing animation-related editor/library surface if one exists. Do not create a broad new Animation Editor.

At minimum the user must be able to:
- inspect source skeleton mapping for a reusable AnimationDefinition;
- assign/clear source joints using actual source skeleton joints;
- use the same bounded Suggest Mapping behavior/conventions as M108 where applicable;
- see source mapping validation;
- see target retargetability in Character Preview/Character Database.

Respect existing working-copy/Save/Reload authority for whichever definition editor owns AnimationDefinition data.

If the current repository has no suitable editable AnimationDefinition surface, implement the narrowest Development editor integration required by M109 and document it; do not expand into timeline/keyframe tooling.

## Canonical Retarget Test Assets

Add the smallest deterministic repository-owned source/target fixtures required to prove real retargeting between non-exact skeletons.

Requirements:
- source and target skeletons must intentionally differ in joint names and/or hierarchy/index layout enough to fail M105 exact compatibility;
- both must have valid M108 humanoid mappings;
- source must contain a small real skeletal animation with visible joint motion;
- target must be skinned so the retargeted motion is visible in the actual render path;
- fixtures must be generated/reproducible where practical;
- keep assets tiny;
- no large third-party character;
- no external licensing dependency.

The existing one-joint canonical Player remains unchanged and continues using exact-compatible animations.

The M108 `humanoid_mapping_fixture` may be reused or evolved only if doing so preserves M108 regression value. Do not destroy the fixture's purpose merely to simplify M109.

## Automated Render/Production Boundary Test

M107 taught us that CPU pose progression alone is insufficient.

Add a regression that reaches the production rendering boundary for retargeted animation.

At minimum prove:
- source and target are exact-incompatible;
- source mapping and target mapping are usable;
- retarget validation is Retargetable;
- source animation time advances;
- target mapped joint pose/bone matrices actually change;
- offscreen rendered target output changes for a visibly animated fixture;
- Pause freezes retargeted rendered state;
- Play resumes;
- Restart returns to frame/time zero;
- Loop behavior works;
- Clamp behavior works where applicable.

The test must fail if CPU retarget data changes but the target skinned render does not consume it.

## Diagnostics

Development diagnostics should distinguish:
- Exact-compatible playback;
- Retargeted playback;
- Unavailable.

For retargeting, expose enough detail to diagnose:
- source mapping state;
- target mapping state;
- missing roles;
- invalid/stale joints;
- source/target skeleton identities or bounded summaries;
- selected source clip;
- reason retargeting is unavailable.

Avoid excessive per-frame logging.

## Manual Acceptance

The Development build must allow the user to verify:

1. canonical `characters/player` still previews and plays Idle/Move/Jump through the existing Exact path;
2. the M109 target fixture is exact-incompatible with its reusable source animation;
3. both source and target humanoid mappings are Usable;
4. Preview reports the target animation as Retargeted rather than Exact;
5. visible source motion is transferred to the target;
6. Play/Pause/Restart work;
7. Loop works for a looping fixture;
8. Clamp works for a clamped fixture if supplied;
9. clearing one required target mapping role makes retargeting unavailable immediately;
10. restoring it recovers Retargeted playback;
11. clearing one required source mapping role makes retargeting unavailable;
12. restoring it recovers;
13. assigning an invalid/duplicate mapping does not crash and blocks retargeting;
14. switching back to an exact-compatible target still uses Exact, not Retargeted;
15. Save/Reload preserves authored mappings/assignments;
16. gameplay canonical Player behavior remains unchanged.

If M109 intentionally does not make the retarget fixture a runtime gameplay character, Preview validation is sufficient for the new capability; do not add CharacterInstance/spawning merely to test it.

## Canonical Data Safety

Audit carefully:
- `game/assets/source/gameplay/definitions.gameplay`
- canonical Player definition
- M108 fixture definition
- any M109 animation/character fixtures

Retain only intentional fixture data.

No incidental semantic/EOL changes to:
- `game/assets/source/levels/level_01.level`
- `game/assets/source/levels/level_02.level`

Do not alter canonical Player model/mappings merely to demonstrate retargeting.

## Regression Safety

Preserve:
- M101 Character Database working-copy/baseline/Save/Reload;
- M102 Player stats;
- M103 skeletal animation and Raylib asset safety;
- M104 equipment attachments;
- M105 reusable animation assets, exact compatibility, embedded fallback;
- M106 Character Asset validation;
- M107 Preview rendering/playback/camera/resource lifetime;
- M108 Humanoid Skeleton Mapping, Suggest Mapping, No Mapping/Incomplete/Invalid/Usable;
- Inventory/Equipment;
- static rendering;
- materials;
- terrain;
- vegetation;
- lighting;
- shadows.

## Tests

Add focused coverage for at least:
1. source humanoid mapping persistence;
2. source mapping malformed/duplicate/stale handling;
3. source mapping validation;
4. exact-compatible path remains exact and unchanged;
5. exact-incompatible + missing source mapping is unavailable;
6. exact-incompatible + missing target mapping is unavailable;
7. invalid source/target mapping is unavailable;
8. valid source + target mappings produce Retargetable;
9. retargeting does not change M105 exact compatibility result;
10. rest-pose-relative mapped rotation transfer;
11. target rest translations preserved for non-root mapped joints;
12. unmapped target joints remain at target rest pose except hierarchy effects;
13. optional roles safely omitted;
14. optional roles retarget when valid on both sides;
15. different joint names can retarget through common roles;
16. different joint indices can retarget;
17. structurally different but supported mapped hierarchy can retarget;
18. source animation progression changes target pose;
19. Pause/Play/Restart;
20. Loop/Clamp;
21. actual offscreen skinned rendered output changes;
22. invalidation/recovery when source/target mapping changes;
23. Character Database/Preview working-copy changes do not mutate active state unexpectedly;
24. Save/Reload persistence;
25. canonical Player exact Idle/Move/Jump regression.

Use real Raylib production-compatible loading/rendering where the contract depends on actual GLB skeleton/animation behavior.

## Validation

Run all focused affected C++ tests, including relevant:
- GameplayDefinitionTest
- CharacterDatabaseEditorTest
- CharacterAssetValidatorTest
- CharacterPreviewTest
- AnimationLibraryTest
- SkeletalAnimationTest
- PlayerPresentationTest
- PlayerAnimationAssetTest
- EquipmentAttachmentTest
- M108 humanoid mapping coverage
- new M109 retargeting tests

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

Perform a real Development executable startup smoke.

Interactive visual acceptance remains user-owned.

## Documentation

Document clearly:
- Exact compatibility (M105) remains preferred and unchanged;
- Humanoid Mapping (M108) supplies semantic correspondence;
- Retargeting (M109) transfers bounded rest-pose-relative humanoid motion between mapped non-exact skeletons;
- M109 is not IK, procedural animation, or generalized motion editing.

## Explicitly Out of Scope

M109 does NOT include:
- IK;
- foot planting;
- hand placement;
- ledge grabbing/climbing mechanics;
- procedural animation;
- AI-generated animation;
- AI-assisted skeleton mapping;
- runtime API calls;
- root motion gameplay authority;
- generalized bone-length compensation;
- generalized translation scaling;
- twist-bone solving/distribution;
- finger retargeting;
- facial retargeting;
- animation timeline/keyframes/curves;
- animation graph/state-machine editor;
- blend trees;
- layers/additive/masks/events;
- generalized CharacterInstance;
- character placement/spawning;
- NPC/Enemy runtime;
- AI/combat/health/damage;
- FBX/new decoder;
- generalized asset DB/reimport/watchers;
- ECS/GUID/JSON/Level Format v2;
- M107 graphical preview gizmo tuning;
- M110+ features.

## Completion Criteria

M109 is complete only when:
- source reusable-animation skeletons can have deterministic humanoid semantic mappings;
- exact compatibility remains unchanged and preferred;
- a real non-exact source/target fixture pair validates as Retargetable;
- mapped source animation motion produces a target pose using bounded rest-pose-relative retargeting;
- target non-root rest translations are preserved by default;
- unmapped target joints remain safe;
- M107 Preview visibly renders retargeted skinned motion and labels it Retargeted;
- invalid/missing mappings disable retargeting safely and recover when restored;
- render-boundary regression proves pixels change from retargeted motion;
- canonical Player exact Idle/Move/Jump remains unchanged;
- focused tests pass;
- Debug/Development/Release builds pass;
- Python regressions pass;
- Development startup smoke passes;
- manual acceptance passes;
- canonical data audits are clean;
- user explicitly approves completion;
- Git closure is performed separately afterward.
