# Milestone 117 — Hit Reaction Foundation

## Status
Implementation validated — manual acceptance and Git closure pending.

## Goal
Add the first bounded animation-level hit reaction for supported runtime characters, building directly on Milestones 114–116 without introducing a combat system.

When the special Player or a resolved NPC/Enemy accepts positive runtime damage, the character may play a short hit-reaction animation and then deterministically return to the animation/state that remains valid for its runtime condition.

Milestone 116's red damage flash remains intact and runs alongside this reaction.

## Scope

### 1. Reusable transient hit-reaction state
- Add the smallest transient runtime state necessary to represent an active hit reaction.
- The reaction starts only when positive damage is actually accepted by the production Runtime Health path.
- Healing, rejected/non-positive damage, MaxHealth synchronization, equipment changes, rebuild/reset, and unrelated state changes do not start a reaction.
- Repeated accepted damage while a reaction is active deterministically restarts the reaction rather than stacking reactions.
- Hit-reaction state is runtime-only and is never serialized.

### 2. Animation asset/clip resolution
- Reuse the existing skeletal-animation, reusable-animation, humanoid-mapping, retargeting, CharacterDefinition, and CharacterInstance architecture.
- Inspect the current repository and use the smallest repository-consistent way to resolve an optional hit-reaction animation for a character.
- Prefer existing animation metadata/assignment mechanisms if they already support an appropriate semantic slot.
- If the current authored character-animation contract must be extended, add only one optional hit-reaction semantic/slot and keep backward compatibility for existing definitions.
- Characters without a valid hit-reaction animation must continue to function normally; damage feedback from M116 still occurs and no runtime failure is allowed.
- Do not create a generalized animation state machine or combo/action framework.

### 3. Player behavior
- Accepted Player damage starts the hit reaction when a valid reaction animation is available.
- During the short reaction, existing Player health and M116 red flash behavior remain authoritative.
- After a non-lethal hit reaction finishes, normal locomotion animation selection resumes deterministically.
- Do not redesign Player movement, jumping, physics, camera, equipment, or stat handling.
- A lethal hit must still enter the existing M115 death/respawn flow exactly once. The hit reaction must not delay, replace, duplicate, or compete with that lifecycle.
- If death becomes authoritative before the reaction completes, death/respawn lifecycle wins and transient hit-reaction state is cleared appropriately.

### 4. NPC behavior
- A resolved CharacterType NPC with a valid hit-reaction animation reacts only when that exact instance accepts damage.
- Reaction state is independent per CharacterInstance.
- Existing M112 patrol semantics remain authoritative outside the bounded reaction.
- During a non-lethal reaction, the NPC must not advance patrol movement; after the reaction it resumes its existing authored patrol deterministically.
- A lethal hit preserves M115 behavior: Defeated, patrol stopped, present/rendered. No death animation is introduced.

### 5. Enemy behavior
- A resolved CharacterType Enemy with a valid hit-reaction animation reacts only when that exact instance accepts damage.
- Reaction state is independent per CharacterInstance.
- Existing M113 patrol semantics remain authoritative outside the bounded reaction.
- During a non-lethal reaction, the Enemy must not advance patrol movement; after the reaction it resumes its existing authored patrol deterministically.
- A lethal hit preserves M115 behavior: Defeated, patrol stopped, present/rendered. No death animation is introduced.

### 6. Interaction with M116 Damage Feedback
- Preserve the M116 transient red tint exactly as a separate presentation effect.
- An accepted hit may produce both the red tint and the animation reaction.
- Neither mechanism may become the authority for Health, Alive/Defeated, Player death, patrol state, or respawn.
- Repeated hits refresh/restart each transient effect according to its own bounded semantics without accumulating permanent state.

### 7. Lifecycle/reset behavior
Clear active hit-reaction state at the real production boundaries relevant to the current architecture, including:
- New Run;
- full Restart;
- Player death -> respawn completion;
- real `PerformRespawn(Manual)` / Gameplay `R`;
- Editor Apply/rebuild;
- CharacterInstance recreation.

Resets must preserve authored origins/patrol settings and stable runtime instance counts.

### 8. Development diagnostics
- Extend existing Development diagnostics only as needed to inspect whether a valid hit-reaction animation is resolved and whether reaction state is active.
- Existing Runtime Health Damage controls remain sufficient to trigger manual tests.
- Do not make the Debug UI authoritative for hit reactions.

## Validation and regression requirements

Add focused automated regression coverage appropriate to the repository's current architecture. At minimum cover:
- accepted positive damage starts hit reaction when a valid reaction clip exists;
- reaction completes and normal animation selection resumes;
- repeated accepted damage deterministically restarts the reaction;
- Heal and rejected/non-positive damage do not start reaction;
- missing/invalid optional reaction animation degrades safely without affecting Health, M116 flash, or runtime stability;
- NPC reaction is independent per instance;
- Enemy reaction is independent per instance;
- one character reacting does not alter another character's animation state;
- non-lethal NPC/Enemy reaction temporarily prevents patrol movement and patrol resumes afterward;
- lethal NPC/Enemy damage preserves M115 Defeated behavior and does not resume patrol;
- lethal Player damage preserves the existing death/respawn flow exactly once;
- M116 red damage feedback remains functional;
- real reset/rebuild boundaries clear active reaction state;
- real `PerformRespawn(Manual)` restores stable Player/NPC/Enemy runtime state with no duplicate instances;
- existing Hazard damage/death/respawn remains functional.

Where behavior crosses Application/runtime/animation/rendering boundaries, include production-boundary regression coverage rather than testing only isolated helpers or Debug UI paths.

## Manual acceptance targets
1. Player non-lethal Damage produces M116 red flash plus a visible hit reaction, then returns to normal locomotion.
2. Repeated Player Damage restarts the reaction cleanly without stuck animation.
3. Heal does not trigger hit reaction.
4. One NPC reacts independently; another NPC remains unaffected.
5. NPC pauses patrol during a non-lethal reaction and resumes afterward.
6. One Enemy reacts independently; another Enemy remains unaffected.
7. Enemy pauses patrol during a non-lethal reaction and resumes afterward.
8. Lethal NPC/Enemy damage ends in M115 Defeated behavior, with no patrol resume or stuck reaction.
9. Lethal Player hit still follows the existing death/respawn path and returns Alive/full with normal animation state.
10. Gameplay `R` during an active reaction clears the reaction and restores stable Player/NPC/Enemy state without duplicates.
11. Hazard damage exercises the production hit-reaction path and preserves normal death/respawn behavior.
12. A character with no valid optional hit-reaction animation remains fully functional and still receives M116 damage flash.

## Explicitly out of scope
- attacks or attack input;
- attack animations as gameplay actions;
- hitboxes or hurtboxes;
- weapon collision;
- damage formulas beyond existing resolved damage;
- aggro, chase, targeting, combat AI, navigation, or pathfinding;
- knockback or launch;
- stagger systems, stun meters, poise, guard break, or crowd control;
- hit-stop;
- invulnerability frames or damage cooldown systems;
- camera shake;
- particles, decals, floating damage numbers, sound effects, or controller rumble;
- directional hit reactions or hit-location selection;
- reaction blending/layering system redesign;
- generalized animation state machine/action framework;
- death animations, defeated poses, ragdolls, despawn, respawn timers, or resurrection;
- loot or XP;
- new combat equipment behavior;
- save-game persistence;
- any Milestone 118 feature.

## Canonical authored-data safety
Do not intentionally modify unless strictly required by the frozen M117 definition and explicitly justified by the current repository architecture:
- `game/assets/source/levels/level_01.level`
- `game/assets/source/levels/level_02.level`
- `game/assets/source/gameplay/definitions.gameplay`

Prefer test fixtures or the repository's existing character/animation authoring mechanisms for validation. If a minimal character-definition animation assignment is required for the feature, keep it narrowly scoped, backward-compatible, and report it explicitly. Do not introduce unrelated canonical gameplay changes.

Do not modify files merely because Git reports LF/CRLF warnings.

## Documentation
Update the current milestone/index, architecture, README/agent guidance, and other current repository documentation required by established repository conventions.

## Completion gate
M117 is complete only after:
- required builds and automated tests pass;
- production-boundary regressions pass;
- canonical authored-data audit is clean/justified;
- manual acceptance is completed by the user;
- any corrections are completed on the same branch and revalidated;
- the user explicitly approves completion;
- Git closure is performed and `main` is clean and synchronized.

The implementation agent must STOP after implementation, validation, and report. It must not commit, push, merge, switch branches, close M117, or begin M118.

## Branch
`milestone/117-hit-reaction-foundation`


## Implementation / Automated Validation

The existing CharacterDefinition animation assignment contract now has one optional Hit Reaction role, with embedded clip and reusable asset fields (`animation_hit_reaction` / `animation_hit_reaction_asset`). Character Database assignment, dirty/equality tracking, parse/write, compatibility diagnostics and preview use the existing authoring mechanisms. Old definitions remain valid. Explicit invalid assets do not fall back to embedded clips. Runtime sampling prefers Exact compatibility and reuses humanoid mapping/retargeting when valid.

RuntimeHealth carries a separate bounded reaction countdown configured from the resolved animation duration, capped at 0.5 seconds. Only positive actual non-lethal damage restarts it; lethal damage clears it immediately. M116 red flash remains independent. CharacterInstance temporarily overrides the sampled clip while retaining locomotion selection. The special Player keeps its existing renderer, movement and locomotion/cross-fade paths. NPC/Enemy patrol progress is held for a step beginning in reaction, then resumes without catch-up. No transient reaction state is serialized.

CharacterPlacementTest covers actual Application damage and rendered Player clip selection/restoration, Hazard damage/death completion, independent NPC/Enemy reactions and patrol suspension/resumption, defeat, active-reaction Manual R, Editor Apply, full Restart and real Main Menu Play/staged New Run replacement. The New Run fixture restores/removes only its generated staged copy. CharacterInstanceTest covers production pose changes, retargeting and safe missing/invalid optional slots. PlayerCharacterStatsTest covers bounded duration, restart, healing/invalid damage/maximum synchronization and lethal priority. GameplayDefinitionTest covers optional-slot round-trip and duplicate rejection.

Validation passed:
- `cmake --preset windows-vs2022`;
- `cmake --build --preset windows-debug`, `windows-development`, and `windows-release` (parallel execution; final incremental builds include the last regression changes);
- all 73 Development C++ test executables; EditorToolRunnerTest passed on an outside-sandbox retry;
- focused CharacterPlacementTest, CharacterInstanceTest, PlayerCharacterStatsTest and GameplayDefinitionTest in Debug and Release, plus focused Development animation/library/compatibility/preview/database tests;
- all seven Python suites required by DEVELOPMENT_WORKFLOW.md;
- `git diff --check` and canonical authored-data audit.

Environment workarounds were process-only: normalized Windows environment key casing for duplicate PATH/Path, disabled MSBuild worker reuse, ran parallel builds outside the sandbox after sandbox worker startup failed, and supplied MSVC `/FS` to resolve PDB contention after a build restart. No repository build configuration was changed.

Canonical Level 01, Level 02 and definitions.gameplay remain unchanged. No real hit-reaction asset or canonical assignment was authored: automated fixtures reuse existing Jump/retarget clips in memory. A suitable optional clip assignment is required for visible manual acceptance. No combat, knockback, layered/blended reaction framework, directional reactions, death animation or M118 behavior was added. Manual acceptance and Git closure remain with the User; implementation stopped without commit, push, merge or branch switch.


## Manual-acceptance correction investigation

The user confirmed the tested NPC was `characters/retarget_target` with `animations/humanoid_jump` assigned as Hit Reaction. Production validation rejects that optional assignment: the skeletons are not Exact compatible and Jump has no usable source humanoid mapping for retargeting. CharacterInstance configures a zero reaction duration, so accepted non-lethal Runtime Health damage starts red feedback but no hit reaction. Patrol therefore continues. The existing typed NPC/Enemy guard already holds movement for a step beginning in a valid reaction; no change to that guard or animation resolution was warranted.

The correction exposes the cached reaction resolution reason in Development Runtime Health, independently of the current locomotion diagnostic. This makes invalid optional assignments distinguishable from active reactions without making the UI authoritative. Use a compatible Exact clip or a reusable clip with usable source/target mappings for manual acceptance; `animations/retarget_clamp` is a fixture-compatible retargeted test assignment, not a newly authored reaction asset.

Previous non-lethal movement assertions used a Player-skeleton Exact fixture and called LevelCharacters directly. They did not cover the manually selected incompatible retarget-target/Jump combination or non-lethal Application frame movement, and CharacterPlacementTest did not inherit the game's configuration definitions. The strengthened test now uses those production definitions/UI linkage and checks actual Application patrol phase/position and rendered instance transforms on 60 Hz steps through reaction expiry and deterministic resumption, for both NPC and Enemy with Exact and retargeted reactions. It separately reproduces incompatible Jump safe degradation with continuing patrol, health reduction and red feedback. CharacterInstanceTest checks the unavailable source-mapping diagnostic. Existing independent/repeated/lethal/reset/Player/Hazard coverage is retained. Fixtures explicitly opt into reactions and restore only in-memory definitions after real editor exit catalog reload.

Incoming canonical-data audit found two existing Jump reaction assignments in `definitions.gameplay`, on Player and Retarget Target. This correction leaves those pre-existing changes untouched and reports them; it makes no canonical authored-data changes. Both protected Level files are unchanged. The earlier implementation audit above describes the tree before manual acceptance, not the current incoming assignments.


Correction validation passed: Windows VS2022 configure; complete Debug, Development and Release builds; focused CharacterPlacementTest, CharacterInstanceTest, PlayerCharacterStatsTest and GameplayDefinitionTest in all three configurations; all 73 Development C++ executables; all seven required Python repository suites; final diff/check and canonical audit. Builds/tests used outside-sandbox host access where required, normalized Windows environment key casing, MSBuild `/nodeReuse:false`, and process-only MSVC `/FS /MP8`; repository compiler settings were not changed. The broader production configuration initially exposed fixture assumptions, which were corrected before final passing runs. No commit, push, merge, branch switch, milestone closure or M118 work was performed. Renewed manual acceptance remains pending.
