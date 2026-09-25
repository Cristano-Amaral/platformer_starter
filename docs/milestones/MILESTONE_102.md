# Milestone 102 — Character Stats & Player Definition Integration

## Status
IMPLEMENTED — AWAITING MANUAL ACCEPTANCE

## Branch
`milestone/102-character-stats-player-integration`

## Goal

Connect the authored Character Definition foundation from Milestones 98/101 to the existing Player runtime and the Inventory/Equipment foundation from Milestone 100.

Milestone 102 introduces a small, typed runtime character-stat layer in which the Player resolves a configured `characters/...` definition, uses its authored base stats as gameplay inputs, and derives effective stats by applying modifiers from currently equipped Item Definitions.

This milestone is deliberately focused on the Player and stat consumption. It does not introduce a general CharacterInstance/spawning framework, AI, combat, animation, equipment sockets, prefabs, ECS, GUIDs, or Level Format v2.

## Architectural Intent

The data flow for the Player becomes:

`CharacterDefinition authored base stats`
→ `Player runtime base stats`
→ `equipped ItemDefinition modifiers`
→ `effective Player stats`
→ `existing Player gameplay/controller parameters`

The authoritative reusable data remains in the shared Gameplay Definition registry:

- Character definitions own authored character metadata and base stats.
- Item definitions own authored item metadata and stat modifiers.
- Inventory owns carried item quantities.
- Equipment owns equipped item identities by typed slot.
- The Player consumes the resolved Character Definition and Equipment state; it does not duplicate authored metadata.

Runtime state remains session/transient unless already covered by existing authored systems.

## Scope

### 1. Player Character Definition Reference

Add one explicit Player character-definition identity using the existing textual identity contract:

`characters/<name>`

The default/canonical Player must resolve `characters/player`.

Use the narrowest integration point consistent with the current repository. Do not add a generalized character placement/spawning format merely to store this reference.

The runtime must distinguish at least:

- resolved Player Character Definition;
- missing reference;
- malformed reference;
- category mismatch.

A bad reference must fail safely and produce useful Development diagnostics rather than crash or silently reinterpret another category.

### 2. Typed Runtime Character Stats

Introduce a small typed runtime representation based exclusively on the existing seven `GameplayStatId` values:

- MaxHealth
- MoveSpeed
- JumpStrength
- GravityScale
- AttackPower
- Defense
- InteractionRange

Do not introduce a generic string/property map, reflection system, JSON schema, or duplicate stat vocabulary.

The runtime must be able to represent:

- authored/base value;
- equipment modifier contribution;
- effective value.

For this milestone:

`effective = base + sum(equipped item additive modifiers)`

Only the existing additive modifier contract is supported.

Do not add multiplicative modifiers, percentages, modifier priorities, stacking rules, buffs/debuffs, timed effects, status effects, or derived-stat formulas.

### 3. Character Definition → Player Runtime

When `characters/player` resolves, the Player must consume the Character Definition's authored base stats.

Integrate only stats that have a meaningful existing Player runtime consumer in the current codebase.

Expected integrations include, where corresponding current runtime behavior exists:

- MoveSpeed → Player movement speed
- JumpStrength → Player jump strength/impulse
- GravityScale → Player gravity behavior

MaxHealth, AttackPower, Defense, and InteractionRange must still participate correctly in the typed base/effective stat calculation and diagnostics, but M102 must not invent health/combat/interaction systems merely to consume them.

Preserve existing Player behavior for any runtime parameter that has no authored stat value or cannot be safely migrated within the milestone's bounded scope. The implementation must define and test the fallback behavior explicitly.

### 4. Equipment → Effective Player Stats

Use the existing M100 Equipment slots:

- Head
- Body
- MainHand
- OffHand
- Accessory

For each occupied slot:

1. resolve the equipped textual item identity through the Gameplay Definition registry;
2. obtain the Item Definition;
3. accumulate its existing additive stat modifiers;
4. calculate effective Player stats.

The calculation must be deterministic and independent of UI ordering.

Missing, malformed, or category-mismatched equipped definitions must fail safely and must not corrupt unrelated stat values.

Inventory contents that are not equipped must not affect effective Player stats.

### 5. Immediate Runtime Recalculation

Effective Player stats must update when Equipment state changes through the existing equip/unequip flow.

No restart or level reload should be required to observe a valid equipment modifier.

Recalculation should also occur at the appropriate existing lifecycle boundaries when runtime inventory/equipment is reset or reconstructed.

Do not introduce a generalized event bus or observer framework solely for this milestone. Prefer a small explicit integration with the existing runtime ownership/lifecycle.

### 6. Development Diagnostics

Extend existing Development diagnostics to make the integration inspectable.

At minimum expose:

- active Player Character Definition identity;
- resolution status;
- base value for each of the seven stats;
- equipment modifier contribution for each stat;
- effective value for each stat.

Diagnostics should make it possible to verify that an equipped item changes an effective stat and that unequipping it restores the base value.

Do not turn this into a new general-purpose gameplay inspector framework.

### 7. Existing Editor/Data Compatibility

Character Database from M101 remains the authoring authority for Character base stats.

Item Database from M99/M100 remains the authoring authority for Item stat modifiers and Equipment Slot.

M102 should consume those definitions rather than duplicate their editing UI.

Only make narrowly necessary editor changes for observing/testing the Player integration. Do not add runtime-character authoring, character placement, animation controls, AI controls, combat controls, or socket authoring.

### 8. Persistence and Session Semantics

Preserve M100 runtime lifecycle semantics:

- Inventory and Equipment remain session-only.
- New Run / Restart / Apply clear Inventory and Equipment as currently defined.
- Checkpoint / physics rebuild / level transition preserve them as currently defined.

Character Definition data remains authored/persistent through `definitions.gameplay`.

Effective Player stats are runtime-derived state and must not be serialized back into Character Definitions or Item Definitions.

## Canonical Data

Preserve the existing canonical `characters/player` definition introduced/extended through M98/M101.

M102 may adjust its authored stat values only when necessary to preserve the current Player feel while moving existing runtime constants under Character Definition authority.

Any such canonical data change must be intentional, minimal, documented, and covered by tests.

Do not add temporary manual-test definitions to canonical gameplay data.

Canonical level files, especially:

- `game/assets/source/levels/level_01.level`
- `game/assets/source/levels/level_02.level`

must not receive incidental or line-ending-only changes.

## Validation / Automated Tests

Add or extend focused automated coverage for at least:

1. resolved `characters/player` base-stat consumption;
2. typed calculation for all seven stats;
3. additive modifiers from one equipped item;
4. additive modifiers from multiple equipped items;
5. unequip restoring the corresponding effective values;
6. inventory-only items not modifying Player stats;
7. missing/malformed/category-mismatched Player definition handling;
8. missing/malformed/category-mismatched equipped Item handling;
9. deterministic recalculation across Equipment slots;
10. movement integration for MoveSpeed;
11. jump integration for JumpStrength;
12. gravity integration for GravityScale, if the current Player runtime has a corresponding consumer;
13. existing Inventory/Equipment behavior regression coverage;
14. existing Item Database and Character Database behavior remaining intact;
15. gameplay definition read/write behavior remaining intact.

Use focused tests rather than introducing a generalized test framework.

Run the repository's relevant C++ tests plus the standard milestone Python validation suite.

Expected build validation:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Expected Python regression validation:

```text
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

## Manual Acceptance

Manual acceptance is mandatory after automated validation.

The Development build must allow verification that:

1. the active Player resolves the canonical `characters/player`;
2. Development diagnostics show base, equipment contribution, and effective values for all seven stats;
3. the Player's normal movement/jump/gravity behavior remains functional using Character Definition-backed values;
4. equipping an Equipment item with a visible MoveSpeed, JumpStrength, or GravityScale additive modifier changes the corresponding effective value and observable Player behavior where applicable;
5. unequipping it restores the base/effective behavior;
6. carrying the same item only in Inventory does not affect Player stats;
7. modifiers on stats not yet consumed by gameplay (for example AttackPower/Defense if combat is still absent) still appear correctly in effective-stat diagnostics without inventing new gameplay systems;
8. existing Inventory and Equipment UI/workflow remains functional;
9. Item Database and Character Database remain functional;
10. Save/Reload of authored definitions still works;
11. no unexpected canonical level changes occur.

Temporary definitions or modifier values created for manual testing must be removed or restored before Git closure.

## Explicitly Out of Scope

Do NOT implement any of the following in M102:

- generalized `CharacterInstance` architecture;
- character placement or spawning system;
- Enemy/NPC/Animal runtime controllers;
- AI or behavior trees;
- combat system;
- damage/health gameplay;
- death/respawn redesign;
- animation system or animation state machine;
- skeletal animation;
- equipment sockets or attachment rendering;
- visible equipped weapons/armor;
- buffs, debuffs, timed effects, status effects;
- multiplicative/percentage modifiers;
- skill trees, levels, XP, attributes, or derived-stat formulas;
- Player prefab/archetype framework;
- generic property bags;
- JSON gameplay-definition format;
- GUID migration;
- ECS migration;
- Level Format v2;
- save-game persistence for Inventory/Equipment/effective stats;
- Milestone 103 functionality.

## Documentation

Update the relevant repository documentation so it accurately describes:

- Character Definition as the authored Player base-stat source;
- Equipment Item modifiers as additive runtime contributions;
- effective-stat formula;
- which stats are currently consumed by Player gameplay;
- which typed stats exist but do not yet have runtime gameplay systems;
- Development diagnostics;
- M102 scope boundaries.

Keep `AGENTS.md`, milestone index/testing expectations, and architecture documentation consistent with repository conventions.

## Safety / Repository Rules

Follow `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md`.

Before implementation, inspect the current repository and treat current code/tests/docs as the source of truth.

Do not normalize line endings opportunistically.

Do not mechanically restore canonical files.

Do not use broad cleanup commands.

Do not commit, push, merge, or begin M103.

At completion, report:

- files changed;
- architecture implemented;
- runtime stat formula;
- Player consumers migrated;
- fallback/error behavior;
- tests added/updated;
- commands run and results;
- canonical-data safety status;
- known limitations explicitly left for future milestones.

Then STOP for review and manual acceptance.
