# Milestone 108 — Humanoid Skeleton Mapping Foundation

## Status
Implementation in progress; awaiting manual acceptance.

## Goal
Introduce a typed, editor-visible humanoid skeleton mapping layer that describes how a character skeleton's actual joints correspond to a bounded canonical humanoid role set, without changing M105/M106 exact skeleton compatibility and without performing animation retargeting.

M108 answers: “Which joints in this character skeleton represent the important humanoid body roles, and is that mapping structurally usable?” It does not transfer animations between different skeletons; that remains M109 scope.

## Authority
- CharacterDefinition remains authored character-data authority.
- Character Database workingCopy/baseline/Save/Reload semantics remain unchanged.
- M103 skeletal loading remains production skeleton authority.
- M105 exact skeleton compatibility remains unchanged and authoritative for reusable animations today.
- M106 validation remains authoritative for current asset suitability/exact compatibility.
- M107 Preview retains current exact-compatible/embedded playback behavior.
- M108 mapping is authored metadata associated with CharacterDefinition and its World Model skeleton.
- Validation reuses production-compatible skeleton data; no second GLB skeleton parser.
- A valid humanoid mapping must NOT make an exact-incompatible animation playable.

## Canonical Humanoid Roles
Introduce a bounded typed role enum:
- Hips
- Spine
- Chest
- Neck
- Head
- LeftShoulder
- LeftUpperArm
- LeftLowerArm
- LeftHand
- RightShoulder
- RightUpperArm
- RightLowerArm
- RightHand
- LeftUpperLeg
- LeftLowerLeg
- LeftFoot
- RightUpperLeg
- RightLowerLeg
- RightFoot

Do not add fingers, face, twist bones, IK targets, weapon sockets, toe detail, or arbitrary semantic tags.

Required roles for a usable mapping:
Hips, Spine, Head, both UpperArm/LowerArm/Hand chains, and both UpperLeg/LowerLeg/Foot chains.

Optional roles:
Chest, Neck, LeftShoulder, RightShoulder.

Optional roles are still validated when assigned. Missing required roles are never synthesized.

## Authored Mapping Data
Add typed humanoid mapping metadata to CharacterDefinition. Each role maps to None or one exact joint name from the current character skeleton.

Persist joint names, not transient indices. Resolve names to indices for inspection/runtime use.

Requirements:
- deterministic persistence;
- no GUID/JSON/Level Format changes;
- typed contract, not an arbitrary public map;
- stale/missing authored joint names remain visible rather than being silently discarded;
- duplicate role fields are rejected;
- malformed persisted data fails safely under existing GameplayDefinition conventions.

Use definitions.gameplay unless repository inspection identifies an already-established stronger authority.

## Mapping Validation
Add typed state:
- No Mapping
- Incomplete
- Invalid
- Usable

Validation details identify specific problems.

At minimum validate:
1. World Model resolves/loads;
2. usable skeleton exists;
3. assigned joint names exist;
4. one joint is not assigned to multiple roles;
5. all required roles exist for Usable;
6. left/right roles are not mapped to the same joint;
7. mapped hierarchy is structurally plausible.

Expected chains:
- Hips -> Spine -> optional Chest -> optional Neck -> Head
- UpperArm -> LowerArm -> Hand per side
- UpperLeg -> LowerLeg -> Foot per side

Optional intermediate roles must not become mandatory hierarchy nodes when absent. Validation must be conservative and deterministic.

## Mapping Suggestions
Provide a bounded deterministic `Suggest Mapping` helper for common humanoid naming conventions (hips/pelvis, spine/chest, neck/head, left/right shoulder/arm/hand/leg/foot).

Suggestions are convenience only:
- never silently Save/promote;
- ambiguous matches remain unresolved;
- operate only on actual loaded joints;
- do not unexpectedly overwrite explicit working-copy assignments;
- no generalized fuzzy/ML mapping system.

## Character Database
Add a compact `Humanoid Skeleton Mapping` section.

For each role:
- show role;
- show current joint or None;
- select from current working-copy World Model skeleton;
- clear assignment;
- preserve/report stale authored names.

Provide bounded `Suggest Mapping` and `Clear Mapping` actions. Make required vs optional roles understandable.

Working-copy edits immediately revalidate without changing active gameplay. Save persists through existing behavior; Reload restores persisted state.

Changing working-copy World Model immediately revalidates existing mapping against the new skeleton. Do not silently clear mappings merely because a temporary model lacks those joints.

## M107 Preview Integration
Keep Character / Animation Preview available while mapping.

A lightweight mapping visualization is optional only if narrowly supported by existing primitives. Bone visualization is not required.

Do not alter animation compatibility behavior:
- exact-compatible reusable animations play;
- None uses embedded clips;
- explicit incompatible reusable animations remain unavailable;
- mapping does not enable incompatible playback.

## Exact Compatibility Hard Boundary
M105 exact compatibility still requires same joint count, same joint name at each index, and same parent index/hierarchy.

A valid humanoid mapping does not make different skeletons M105-compatible. M106 diagnostics remain truthful. M109 will define future retargeting.

## Test Assets
Do not replace the canonical minimal Player asset. Its one-joint skeleton may correctly report No Mapping/Incomplete.

Add only the smallest deterministic repository-owned multi-joint humanoid fixture(s) needed for validation/suggestion tests, consistent with existing generated test-asset practices. No large third-party humanoid character in M108.

## Diagnostics
Development diagnostics should expose mapping state, missing required roles, role->joint name/index, stale joints, duplicates, and structural-chain failures without excessive noise.

## Tests
At minimum cover:
1. deterministic role enumeration;
2. persistence round-trip;
3. None round-trip;
4. duplicate serialized role rejection;
5. stale joint preservation/reporting;
6. no skeleton/static model cannot be Usable;
7. incomplete required roles -> Incomplete;
8. valid required roles -> Usable;
9. optional Chest/Neck/Shoulders may be absent;
10. assigned optional roles validate;
11. duplicate joint assignment -> Invalid;
12. invalid arm/leg/torso chains -> Invalid;
13. left/right same-joint conflict -> Invalid;
14. working-copy World Model change immediately revalidates;
15. working-copy edits do not mutate active registry;
16. Reload restores persisted mapping;
17. Suggest Mapping works deterministically on conventional fixture;
18. ambiguity remains unresolved;
19. suggestions do not unexpectedly overwrite explicit assignments;
20. M105 exact compatibility is unchanged by mapping;
21. M107 does not use mapping to play exact-incompatible animation.

Use real production-compatible skeleton loading where actual GLB/Raylib data matters.

## Manual Acceptance
In Development:
1. Open Character Database and canonical characters/player.
2. Confirm its minimal one-joint skeleton does not falsely report a usable humanoid mapping.
3. Inspect Humanoid Skeleton Mapping and required/optional roles.
4. Use the repository-provided humanoid fixture/CharacterDefinition if supplied.
5. Confirm actual skeleton joints appear in selectors.
6. Run Suggest Mapping; verify suggestions without automatic Save.
7. Confirm a valid fixture reports Usable.
8. Clear one required role -> Incomplete immediately.
9. Restore it.
10. Assign the same joint to two roles -> Invalid.
11. Restore valid mapping.
12. Temporarily change World Model to incompatible/static -> immediate revalidation without destroying authored names.
13. Restore mapped model -> mapping recovers.
14. Save/Reload -> mapping persists.
15. Verify M107 Preview still works.
16. Verify exact-compatible animation behavior is unchanged.
17. Verify mapping does not make an exact-incompatible animation playable.
18. Enter gameplay and verify Player Idle/Move/Jump and movement remain unchanged.

Restore/remove temporary manual-test data according to canonical fixture policy.

## Canonical Data Safety
Do not invent a fake full mapping for canonical characters/player; its skeleton is intentionally minimal.

No incidental semantic/EOL changes to level_01.level or level_02.level. Audit definitions.gameplay carefully and retain only intentional M108 definitions/fixtures.

## Regression Safety
Preserve M101 Character Database authority, M102 stats, M103 skeletal animation/real-Raylib safety, M104 attachments, M105 reusable animations/exact compatibility/fallback, M106 validation, M107 Preview/playback/camera/skinning/resource lifetime, Inventory/Equipment, static rendering, materials, terrain, vegetation, lighting, and shadows.

## Validation
Run relevant C++ tests including GameplayDefinitionTest, CharacterDatabaseEditorTest, CharacterAssetValidatorTest, CharacterPreviewTest, AnimationLibraryTest, SkeletalAnimationTest, PlayerPresentationTest, PlayerAnimationAssetTest, EquipmentAttachmentTest, and new HumanoidSkeletonMapping tests.

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

Perform a real Development startup smoke. Interactive acceptance remains user-owned.

## Documentation
Document the distinction between exact skeleton compatibility, humanoid semantic mapping, and future retargeting. Do not imply M108 implements retargeting.

## Explicitly Out of Scope
No animation retargeting; runtime animation transfer; rest/bind-pose retarget correction; bone-length compensation; translation scaling; root-motion retargeting; IK; twist solving; finger/facial mapping; automatic rigging/skinning; skeleton editing/renaming; animation timeline/keyframes/curves; Animation Graph; blend trees/layers/additive/masks/events; generalized CharacterInstance; placement/spawning; NPC/Enemy runtime; AI/combat/health/damage; FBX/new decoder; generalized asset DB/reimport/watchers; ECS/GUID/JSON/Level Format v2; or M109+.

## Tuning Note
A graphical rotation gizmo for the M107 Character / Animation Preview is intentionally deferred as future tuning. M108 must not absorb it. Existing M107 orbit controls remain sufficient.

## Completion Criteria
Complete only when typed humanoid roles and deterministic role-to-joint persistence exist; Character Database can author/validate mappings from working-copy skeletons; required/optional roles and No Mapping/Incomplete/Invalid/Usable states work; stale/structural errors are visible; conservative suggestions work without silent authoring; working-copy changes revalidate immediately; M105 exact compatibility remains unchanged; M107 does not implicitly retarget; tests/builds/Python/startup/manual acceptance pass; canonical audits are clean; user approves; and Git closure is performed separately.
