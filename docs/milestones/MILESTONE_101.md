# Milestone 101 --- Character Database & Character Editor Foundation

**Status:** Planned\
**Branch:** `milestone/101-character-database-editor`

## Goal

Establish the first authored Character Database and Character Editor on
top of the shared Gameplay Definition foundation introduced in M98,
following the same narrow data-authority and editor workflow principles
used by the Item Database in M99.

M101 makes reusable character definitions authorable and persistent. It
does **not** yet turn the Player, enemies, NPCs, or animals into runtime
Character instances, and it does not add animation, AI, combat,
equipment sockets, or character spawning.

## Context

M98 established the shared `gameplay::` definition contract with stable
textual identities:

-   `items/<name>`
-   `characters/<name>`

It also established typed gameplay stats:

-   MaxHealth
-   MoveSpeed
-   JumpStrength
-   GravityScale
-   AttackPower
-   Defense
-   InteractionRange

M99 added typed ItemDefinition data and the Item Database / Item Editor
while continuing to use the shared `definitions.gameplay` source file.

M100 added Inventory and Equipment foundations using stable textual Item
identities and ItemDefinition metadata.

M101 extends the same architecture to the Character category rather than
introducing a separate generic database, GUID system, JSON property bag,
or runtime character framework.

## Scope

### 1. Typed CharacterDefinition payload

Add a typed CharacterDefinition payload to Gameplay Definitions whose
category is Character.

A CharacterDefinition contains, at minimum:

-   stable textual identity: `characters/<name>`
-   Display Name
-   Description
-   Character Type
-   optional World Model
-   typed base stats using the existing gameplay stat vocabulary

Character Type is a closed typed enum:

-   Player
-   Enemy
-   NPC
-   Animal

Do not use free-form strings for Character Type.

### 2. Character base stats

CharacterDefinition owns authored **base** stat values.

Use the stat types already established by the shared gameplay-definition
contract. Do not introduce a generic string-to-value property system.

The editor must expose only the supported typed stats and persist them
deterministically.

M101 does not calculate equipment-modified/effective character stats.
Integration between CharacterDefinition stats, Player runtime state, and
Equipment belongs to a later milestone.

### 3. Character World Model

CharacterDefinition may reference an optional static World Model using
the repository's existing model identity/catalog conventions.

Reuse the existing model-selection/picker infrastructure where
practical.

The model reference is definition metadata only in M101. Do not replace
the current Player model or instantiate runtime characters from
CharacterDefinition yet.

Missing model references must remain diagnosable and must not corrupt or
silently erase the authored textual reference.

### 4. Persistence

Continue using:

`game/assets/source/gameplay/definitions.gameplay`

Extend the existing deterministic parser/writer with Character-only
fields required by this milestone.

Requirements:

-   preserve existing Item definitions;
-   preserve existing Item-only fields;
-   round-trip Character definitions deterministically;
-   reject malformed Character payloads through the existing
    validation/error-reporting style;
-   preserve stable textual identities;
-   do not introduce a new file-format version merely for M101;
-   do not migrate to JSON;
-   do not introduce GUIDs.

Debug/Release runtime asset staging must continue to follow the existing
staged-asset authority.

### 5. Character Database editor

Add a Development-only Character Database / Character Editor consistent
with the Item Database UX and repository conventions.

The editor must support:

-   list of Character definitions;
-   search/filter;
-   create;
-   rename;
-   delete;
-   selection;
-   editing Display Name;
-   editing Description;
-   editing Character Type;
-   optional World Model selection;
-   editing supported typed base stats;
-   Save;
-   Reload;
-   dirty/working-copy behavior consistent with the Item Database.

Prefer reusing existing Gameplay Definition registry/editor
infrastructure rather than creating a second unrelated database
architecture.

### 6. Identity and validation

Character identities use:

`characters/[a-z][a-z0-9_]{0,31}`

Creation and rename must enforce the existing shared identity rules.

Duplicate identities are rejected.

References and validation continue to use the shared M98 semantics where
applicable:

-   None
-   Resolved
-   Missing
-   Malformed
-   CategoryMismatch

Do not weaken Item validation while adding Character support.

### 7. Development diagnostics

Extend existing Development diagnostics only as necessary so Character
definitions and validation failures can be inspected.

Do not create a new generalized diagnostics framework.

## Editor authority

Follow the established editor authority model:

-   working copy contains pending authored edits;
-   Save persists valid authored definition state;
-   Reload restores persisted state;
-   invalid edits must not silently corrupt persisted definitions.

Match the existing Item Database behavior where it is already
authoritative and appropriate.

## Runtime boundary

M101 is an authoring/data milestone.

It must **not**:

-   replace the current Player runtime representation with
    CharacterDefinition;
-   apply CharacterDefinition stats to the Player;
-   create CharacterInstance;
-   add character placement to Level Format v1;
-   spawn enemies, NPCs, or animals;
-   add AI controllers or behaviors;
-   add combat;
-   add health/damage runtime systems;
-   add animation state machines;
-   add skeletal animation;
-   add equipment sockets;
-   attach equipped models to characters;
-   add prefab/entity frameworks.

Those are future milestones.

## Required regression coverage

Add focused automated coverage for at least:

1.  valid CharacterDefinition construction/validation;
2.  all Character Type enum values;
3.  valid and invalid `characters/<name>` identities;
4.  duplicate Character identity rejection;
5.  typed base-stat persistence and round-trip;
6.  optional World Model persistence and round-trip;
7.  quoted/path-safe World Model serialization if the current grammar
    requires it;
8.  missing World Model reference remains diagnosable without data loss;
9.  Item definitions continue to parse/write unchanged;
10. Item-only fields remain isolated from Character definitions;
11. Character-only fields are rejected or diagnosed on incompatible
    categories according to the existing parser contract;
12. Character Database create/rename/delete/search/Save/Reload behavior
    through existing testable seams;
13. staged gameplay-definition asset behavior remains valid.

Prefer existing production seams. Do not create a parallel test-only
implementation.

## Manual acceptance

After automated validation, manually verify in Development:

1.  Open the Character Database / Character Editor.
2.  Create `characters/player`.
3.  Set a Display Name and Description.
4.  Set Character Type = Player.
5.  Assign a valid World Model.
6.  Set several typed base stats, including MaxHealth, MoveSpeed, and
    JumpStrength.
7.  Save.
8.  Restart/reload the Development application and confirm the
    definition persists exactly.
9.  Create at least one second character with another Character Type.
10. Verify search/filter and selection.
11. Rename a Character definition and verify the identity remains valid
    and persisted.
12. Verify an invalid/duplicate identity is rejected without corrupting
    existing definitions.
13. Verify existing Item Database entries still load and remain
    editable/persistent.

Manual acceptance is mandatory before Git closure.

## Validation

Run the repository-required validation appropriate to the final M101
change set, including the standard builds and relevant Python
validation:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release

python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Run the directly affected C++ test executables/targets according to the
repository's current conventions.

Before completion, verify:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

Canonical level files must not be modified by M101.

## Documentation

Update only the current documentation required by repository
conventions, including the milestone index/current architecture
documentation where appropriate.

Do not restore or rewrite historical milestone documentation
unnecessarily.

## Explicitly out of scope

M101 does not include:

-   CharacterInstance/runtime character database consumption
-   Player migration to CharacterDefinition
-   effective stats or Equipment-to-Character stat application
-   enemy/NPC/animal spawning
-   Level Format character placement
-   character animation
-   animation state machines
-   skeletal meshes/rigging
-   AI
-   combat
-   health/damage gameplay
-   equipment sockets
-   character archetype/prefab framework
-   inventory redesign
-   Item Database redesign
-   savegame/profile persistence
-   GUIDs
-   ECS migration
-   Level Format v2
-   generic reflection/property systems
-   generic asset database replacement

## Completion criteria

M101 is complete only when:

-   typed CharacterDefinition data exists and validates;
-   Character definitions persist in `definitions.gameplay`;
-   Character Database / Character Editor supports the required
    authoring operations;
-   typed base stats and optional World Model round-trip correctly;
-   existing Item definitions remain compatible;
-   automated validation passes;
-   manual acceptance passes;
-   canonical levels are unchanged;
-   the user explicitly approves completion;
-   Git closure is performed separately according to
    `DEVELOPMENT_WORKFLOW.md`.

The implementation agent must STOP after
implementation/validation/reporting and must not commit, push, merge,
close M101, or start M102.
