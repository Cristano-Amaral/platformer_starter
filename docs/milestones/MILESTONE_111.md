# Milestone 111 — Character Placement & Spawning

## Status
Implementation in progress; manual acceptance and completion approval remain user-owned.

## Branch
`milestone/111-character-placement-spawning`

## Goal
Promote the transient runtime `CharacterInstance` foundation from M110 into authored Level Format v1 character placement and deterministic runtime spawning.

M111 establishes: Level Editor authored Character placement → Level Format v1 Character record → Apply/Save/Reload → runtime spawn → M110 CharacterInstance → existing Exact/Retargeted/Static presentation.

The milestone is about **where a character instance comes from and where it exists in a level**. It is not about NPC or Enemy behavior.

## Architectural Boundary
Reuse existing authorities:
- `CharacterDefinition` = authored/shared character data.
- `CharacterInstance` = transient per-instance runtime state from M110.
- Level Character placement = persistent authored data requesting one CharacterInstance at a transform.
- Runtime handle = transient session identity created when the placement is instantiated.

Do not serialize runtime handles, duplicate CharacterDefinition payload into levels, create a second character-definition system, add GUIDs, JSON, ECS, or Level Format v2.

## Level Format v1 Character Placement
Extend Level Format v1 with one bounded repeatable `Character` category/record consistent with current parser/writer conventions.

Persist only the minimum M111 authored data:
- CharacterDefinition textual identity `characters/<name>`;
- world position;
- world rotation using current character/world conventions;
- positive scale only if required by existing Level Format/M110 transform conventions.

Do not persist runtime handle, playback clock, pose, bone matrices, locomotion state, AI, health, inventory/equipment, or physics/controller state. Respect current Level Format v1 limits and validation conventions.

## Authored vs Runtime Identity
A placement references stable CharacterDefinition textual identity. Instantiation creates a new M110 session-local runtime handle. Save/Reload preserves authored placement semantics, not the old handle. Rebuild/reload may produce different handles while preserving the same placement.

## Editor Authoring
Integrate Character placement into the current Level Editor workflow. Support create/add, CharacterDefinition picker, None/Resolved/Missing visibility, preservation of missing textual references, selection, supported transform editing, duplicate, delete, working copy, Apply, Save, Reload, and level switching according to existing editor conventions.

Do not build a new editor framework.

## CharacterDefinition Picker
Reuse the existing Character Database/registry and picker conventions. Show Display Name plus stable identity, allow repair/clear as current working-copy rules permit, preserve Missing references, and never create/edit CharacterDefinitions from placement UI.

## Working Copy / Apply Authority
Preserve existing editor authority: `workingCopy` may temporarily contain incomplete edits; Apply validates/promotes valid authored state to `active`; Save writes authored state according to current conventions. Follow existing rules for newly-created invalid objects. Do not silently default to `characters/player`.

## Runtime Spawning
When active level data is instantiated/rebuilt, each valid Character placement creates exactly one M110 CharacterInstance.

Requirements:
- one valid placement → one instance;
- two placements sharing one definition → two independent instances;
- distinct transient handles;
- authored transform initializes runtime transform;
- M110 handles Exact/Retargeted/Static/Unavailable presentation;
- invalid placement does not block valid placements;
- missing definition fails safely/visibly;
- Apply/rebuild/reload does not accumulate duplicate instances;
- stale instances are destroyed/replaced safely.

Do not create a generalized spawn framework beyond this bounded character path.

## Player Boundary
The gameplay Player remains special and authoritative. A placed `characters/player` must not silently become a second gameplay-controlled Player. Choose the narrowest current-architecture-compatible policy: reserve/reject canonical Player for generic placement, or allow only an explicitly generic visual CharacterInstance with no Player controller ownership. Document and test the choice.

Do not migrate Player spawning/controller ownership in M111.

## Character Type Boundary
Player/Enemy/NPC/Animal CharacterDefinitions may only produce generic CharacterInstances subject to the Player rule. Character Type does not activate behavior. Enemy is not yet Enemy runtime logic; NPC is not yet NPC runtime logic; Animal has no behavior yet.

## Initial Presentation
Spawned generic instances use deterministic initial presentation, normally Idle. Do not serialize locomotion state or add AI transitions.

## Transform / Grounding
Use one clear authored transform meaning consistent with M110/current world-character conventions. No hidden magic Y offsets. Editor preview/ghost and runtime spawn must agree. Changing definition/model must not unpredictably rewrite authored position.

## Editor Preview / Ghost
Resolved placements should show useful World Model feedback at authored transform using existing infrastructure where practical. Static/skinned models must be safe. Missing/unresolved references use established placeholder/missing presentation. Preview creates no gameplay behavior and no persistent runtime identity. Do not duplicate M107 Preview inside the Level Editor.

## Apply / Rebuild Lifecycle
Audit Apply, Reload, level load, restart, New Run, checkpoint-related rebuild, and level transition where applicable. Preserve existing lifecycle authority. Active CharacterInstances must match active authored placements without duplicates/leaks. Do not reset unrelated runtime systems unless existing authority requires it.

## Save / Reload Round Trip
Valid placement must survive Create → assign definition → transform → Apply → Save → Reload with semantic authored data preserved. Runtime handles are excluded. Writer output must remain deterministic/canonical.

## Missing References
A syntactically valid missing `characters/<name>` identity is preserved and shown Missing, never substituted, never treated as resolved/spawnable, and remains repairable. Malformed identities follow existing parser/validation rejection rules.

## Selection / Duplicate / Delete
Character placements participate in existing level-object editing. Duplicate copies authored fields but creates a distinct placement; Apply produces a distinct runtime instance/handle. Delete + Apply removes its runtime instance with no orphan. Preserve generic selection/group behavior without expanding grouping architecture.

## Rendering / Shadows
Spawned instances reuse M110 production rendering: independent transforms/poses, Exact, Retargeted, static, materials, and directional shadows where supported. Do not add a second renderer.

## Development Diagnostics
Provide bounded diagnostics correlating authored placements and runtime instances using existing level-object indexing/identification rather than GUIDs. Show definition identity, resolved/missing state, runtime handle when spawned, transform, and presentation mode. Avoid log spam.

## Automated Tests
Cover at least:
1. valid Character record parse;
2. deterministic writer round-trip;
3. multiple records;
4. malformed identity rejection;
5. valid missing identity preservation;
6. definition resolution;
7. one valid placement spawns exactly one M110 instance;
8. two same-definition placements spawn independent instances;
9. handles distinct and never serialized;
10. authored transform initializes runtime transform;
11. deterministic Idle initial state;
12. Exact spawn path;
13. Retargeted spawn path;
14. static safety;
15. missing reference isolation;
16. rebuild does not duplicate;
17. delete + Apply removes instance;
18. duplicate + Apply creates independent instance;
19. Save/Reload preserves authored identity/transform;
20. handles may change across rebuild without authored semantic change;
21. working-copy edits do not affect active runtime before Apply;
22. Apply promotes valid edits;
23. incomplete/invalid working copy does not corrupt active runtime;
24. preview/ghost transform agrees with runtime spawn;
25. Player gameplay regression;
26. M110 independence regression;
27. M109 retarget regression;
28. canonical levels remain semantically clean unless a narrowly justified fixture is required.

Use production paths, not test-only alternate spawning.

## Production-Boundary Regression
Cross the real boundary: authored Level Character records → parse/load → active level → runtime spawn → M110 CharacterInstances → production renderer.

Prove two distinct authored placements become distinct runtime instances at authored transforms and render simultaneously; exercise real animation (Exact and Retargeted where practical); rebuilding does not accumulate duplicates; deleting one removes only it; rendered pixels prove spawned instances reach the production renderer.

## Manual Acceptance
Development build must allow one-at-a-time verification of:
1. add Character placement;
2. select CharacterDefinition;
3. transform it;
4. Apply → exactly one spawned runtime character at authored transform;
5. duplicate/add same definition → two independent instances;
6. distinct runtime handles;
7. delete one + Apply removes only it;
8. Retargeted fixture spawns/renders via M109/M110;
9. Save → Reload preserves placements/transforms;
10. runtime handles are not persistent authored IDs;
11. Missing definition is visible/safe;
12. restart/rebuild does not accumulate duplicates;
13. Player gameplay remains normal;
14. Player Idle/Move/Jump remain correct;
15. equipment attachment remains correct;
16. shadows/general rendering remain correct.

## Canonical Data Safety
Do not add demo Character placements to canonical levels merely to prove M111. Audit `level_01.level`, `level_02.level`, and `definitions.gameplay`. Prefer automated/in-memory fixtures. If manual acceptance changes a canonical level, restore only intentional semantic test edits without EOL normalization. Preserve the M45 removal of `dynamic_box 0 5 0 1 1 1 30`.

## Regression Safety
Preserve Level Format v1 limits/categories; workingCopy/active/Save authority; selection/groups/material/terrain/lighting; M101–M110 behavior; Player input/controller/physics; Inventory/Equipment; checkpoints/restart/new-run; static rendering; materials; terrain; vegetation; lighting; shadows.

## Validation
Run new M111 tests plus affected Level Format/parser/writer/editor tests, CharacterInstanceTest, GameplayDefinitionTest, CharacterAssetValidatorTest, AnimationLibraryTest, SkeletalAnimationTest, PlayerPresentationTest, PlayerAnimationAssetTest, EquipmentAttachmentTest, and M109/M110 render-boundary regressions.

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

Run any existing Level Format/cook tests required by current repository conventions and a real Development startup smoke. Interactive acceptance remains user-owned.

## Documentation
Document: CharacterDefinition = authored/shared data; Level Character placement = persistent authored request for one occurrence; CharacterInstance = transient runtime realization; runtime handle = session-local and never serialized. M111 adds placement/spawning only, not NPC/Enemy/Animal behavior.

## Explicitly Out of Scope
No NPC/Enemy/Animal behavior, AI, perception, navigation/pathfinding, health/damage/combat/death/rewards, dialogue, interaction behavior, generic character physics/controller, Player spawn migration, generic inventory/equipment for spawned characters, runtime save-game persistence, spawn waves, dynamic gameplay spawn APIs, scripting, prefab/entity framework, ECS, GUID, JSON, Level Format v2, IK, ragdoll, procedural animation, root-motion gameplay authority, animation graph/state machine/blend trees/layers/events, AI-assisted mapping/motion, FBX/new decoder, generalized asset DB/reimport/watchers, M107 preview gizmo tuning, or M112+.

## Completion Criteria
M111 is complete only when Level Format v1 and Level Editor support bounded Character placement; stable CharacterDefinition references and missing references behave correctly; Apply spawns exactly one M110 instance per valid active placement; duplicate/shared-definition placements remain independent; runtime handles are transient/nonserialized; Save/Reload preserves authored semantics; rebuilds do not accumulate duplicates; delete removes stale runtime instances; Exact/Retargeted/Static reuse M110; a production-boundary regression reaches the real renderer; Player remains unchanged; no NPC/Enemy/AI behavior is introduced; tests/builds/Python/startup smoke/manual acceptance pass; canonical audits are clean; user explicitly approves; and Git closure is performed separately.
