# Milestone 98 --- Gameplay Definition Foundation

## Status

Implemented, awaiting manual acceptance.

## Branch

`milestone/98-gameplay-definition-foundation`

## Recommended Agent / Model

Cursor --- Grok 4.6 High --- Fast OFF

## Objective

Establish the small, typed, reusable gameplay-definition foundation that
later Item and Character systems can share, without implementing the
Item Database, Character Database, inventory, equipment, animation, AI,
vehicles, or a generic scripting/data framework in this milestone.

The milestone should introduce stable authored gameplay identities and a
focused typed stats/property vocabulary, together with
parsing/validation/runtime lookup infrastructure and tests. It must
remain deliberately narrow so M99+ can build on a clean contract rather
than duplicating identity and stat semantics.

## Architectural Direction

### 1. Stable authored definition identity

Gameplay definitions use stable textual identities rather than authored
numeric indices.

Examples:

``` text
items/master_key
items/health_potion
characters/player
characters/guard
```

Requirements: - identity is the durable authored/reference key; -
runtime code may resolve identities to compact indices/handles
internally; - serialized level/content references must not depend on
unstable vector positions or raw runtime indices; - identity comparison
and validation rules must be deterministic; - duplicate identities are
invalid; - malformed identities fail clearly rather than being silently
rewritten.

The implementation should choose and document a minimal identity grammar
consistent with repository conventions. Do not introduce GUIDs.

### 2. Definition categories

Introduce the minimum common category vocabulary required for future
reusable gameplay definitions.

At minimum the foundation must distinguish: - Item - Character

This milestone does **not** implement the complete ItemDefinition or
CharacterDefinition schemas. It only establishes the shared
identity/category contract and registry/lookup behavior needed by later
milestones.

The design must allow later Character definitions to distinguish Player
/ Enemy / NPC / Animal without implementing those authoring systems now.

### 3. Typed gameplay stats/properties

Introduce a focused typed stat/property vocabulary suitable for future
Character and equipment integration.

Initial stat identifiers should cover at least: - MaxHealth -
MoveSpeed - JumpStrength - GravityScale - AttackPower - Defense -
InteractionRange

Requirements: - use typed identifiers/enums rather than arbitrary
gameplay strings; - values use an explicit numeric representation
appropriate to the existing engine; - validation must reject
invalid/non-finite values where applicable; - provide deterministic name
↔ typed-identifier conversion for serialization/editor display; - do not
add a generic JSON/key-value property bag.

This milestone defines the vocabulary and reusable data structures only.
It does not yet replace existing player tuning with these stats unless a
very small integration seam is necessary and explicitly justified.

### 4. Stat modifiers

Provide a minimal typed modifier representation for future
equipment/buffs.

The intended future model is:

`effective stat = base stat + equipment modifiers + temporary/effect modifiers`

M98 only needs the reusable representation and deterministic evaluation
helper(s) necessary to prove that typed modifiers compose correctly.

Keep the modifier model intentionally small. Prefer additive modifiers
for the initial contract unless repository inspection proves that
another minimal operation is already required. Do not build a
generalized effects system.

### 5. Gameplay definition registry/catalog foundation

Provide a deterministic in-memory registry/catalog abstraction that
can: - register/load definition metadata; - reject duplicate
identities; - resolve an identity to the corresponding definition
metadata/runtime handle; - preserve category/type information; - report
missing references safely; - expose stable behavior suitable for later
editor/database UI.

Do not build the M99 Item Database UI or M101 Character Editor UI.

### 6. Authored references

Introduce a small reusable authored reference representation based on
textual identity.

Requirements: - empty/None state must be explicit; - missing identities
must remain diagnosable rather than silently resolving to another
object; - runtime resolution must not mutate authored identity; -
references must remain stable if registry ordering changes.

No level object needs to adopt Item/Character references in M98 unless a
narrowly scoped test fixture is required. Do not add Item Pickup
behavior, inventory, equipment, NPCs, enemies, or player-definition
migration.

## Persistence / Source Format

Repository inspection is authoritative before choosing exact files and
syntax.

M98 should establish the serialization/parsing contract needed for
gameplay definitions in the smallest repository-consistent form. Prefer
a simple textual authored format consistent with the engine's existing
content philosophy.

Requirements: - deterministic read/write; - bounded parsing and
validation; - clear errors for malformed records; - duplicate identity
rejection; - unknown typed stat identifiers rejected; - round-trip
tests; - no JSON dependency solely for this feature; - no GUID layer; -
no file-format/version framework beyond what is actually needed.

If the cleanest architecture is to introduce definition files under an
appropriate source-content directory, do so narrowly and document the
chosen layout. Do not create a full asset database or generalized
resource system.

## Runtime / Editor Scope

M98 is primarily foundation work.

A minimal debug/editor inspection surface is allowed only if it
materially proves that definitions were discovered/resolved correctly. A
polished database editor belongs to later milestones.

Do not implement: - Item Database editor; - Character Database editor; -
Inventory UI; - Equipment UI; - character tabs; - animation editor; - AI
editor; - drag/drop authoring workflow.

## Validation and Failure Behavior

The implementation must explicitly test and handle: - valid
identities; - invalid/malformed identities; - duplicate identities; -
missing references; - category mismatch; - known/unknown typed stats; -
non-finite or otherwise invalid numeric stat values; - deterministic
stat modifier evaluation; - registry ordering changes not changing
authored identity semantics; - serialization round-trip.

Errors should be visible through the repository's existing diagnostics
conventions. Avoid silent fallback that could bind a reference to the
wrong definition.

## Compatibility

M98 must preserve all existing behavior from M97 and earlier: - existing
Level Format v1 content remains valid; - Terrain Albedo/Normal/Roughness
remains unchanged; - Terrain Paint, Vegetation, and Ground Cover remain
unchanged; - current Player behavior remains unchanged unless an
explicitly minimal non-behavioral seam is required; - no existing
authored level is mechanically rewritten; - canonical `level_01.level`
and `level_02.level` must remain semantically untouched.

## Explicitly Out of Scope

Do not implement any of the following in M98: - full ItemDefinition
schema; - Item Database / Item Editor; - inventory; - equipment slots or
equipment UI; - pickups or item-use behavior; - full CharacterDefinition
schema/editor; - Player conversion to CharacterDefinition; -
Enemy/NPC/Animal behavior; - animation state machines; - equipment
sockets; - buffs/debuffs/effects framework; - abilities/skills; -
quests; - dialogue; - AI behavior trees or scripting; - vehicles; - ECS
migration; - GUIDs; - prefab system; - undo/redo; - generalized
reflection/property framework; - generic JSON/key-value gameplay data; -
generalized asset database; - networking/save-game architecture.

These belong to later milestones and must not be pulled forward
opportunistically.

## Expected Follow-on Direction

M98 should make the following later milestones straightforward without
implementing them now:

-   M99 --- Item Database & Item Editor
-   M100 --- Inventory & Equipment Foundation
-   M101 --- Character Database & Character Editor Foundation
-   M102 --- Character Stats / Player Definition / Equipment Integration
-   later Character Animation, sockets/archetypes, and AI/controller
    milestones

Exact future milestone definitions remain unfrozen until each prior
milestone is closed.

## Implementation Expectations

Before editing, inspect: - `AGENTS.md` - `DEVELOPMENT_WORKFLOW.md` -
current gameplay/content parsing and catalog patterns; - existing typed
enum/string conversion patterns; - existing validation/error-reporting
conventions; - current tests and milestone documentation.

Repository code and current documentation are the source of truth. Adapt
names/files to existing conventions rather than imposing a parallel
architecture.

Prefer small, explicit C++ types and functions over generalized
frameworks.

## Automated Validation

Add focused C++ tests for the new foundation and update existing tests
only where required.

At minimum cover: 1. valid identity parsing/validation; 2. malformed
identity rejection; 3. duplicate identity rejection; 4. registry lookup
by identity; 5. missing-reference behavior; 6. category
preservation/mismatch handling; 7. typed stat name ↔ identifier
conversion; 8. unknown stat rejection; 9. stat value validation; 10.
deterministic modifier evaluation; 11. authored reference stability
across registry reorder; 12. definition serialization round-trip; 13.
existing gameplay/content regression tests relevant to touched code.

Run the repository's applicable C++ suite and the standard Python
validation suite:

``` text
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

If repository inspection shows that a listed test is unrelated but still
part of the established validation suite, run it anyway unless there is
a concrete environmental blocker and report that blocker.

## Canonical Data Safety

Before completion:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

Do not use broad cleanup/restoration commands such as: -
`git restore .` - `git clean -fd`

Do not normalize line endings or mechanically rewrite canonical levels.

## Manual Acceptance

After automated validation, stop for user acceptance.

Manual acceptance should prove at minimum: - the engine/editor still
starts normally; - an M98 definition fixture/catalog can be loaded or
inspected through the implemented minimal seam; - a valid identity
resolves deterministically; - a deliberately missing identity is
reported safely; - existing gameplay and editor behavior show no obvious
regression.

The exact manual steps should be reported by the implementation agent
based on the concrete implementation.

## Completion / STOP Rule

After implementation and validation, report: - files changed; -
architecture chosen; - exact authored identity grammar; - exact
definition persistence format/location; - typed stat vocabulary; -
modifier semantics; - registry/reference behavior; - tests
added/updated; - build/test results; - canonical-level diff results; -
remaining manual acceptance steps; - any limitations intentionally
deferred.

Then **STOP**.

Do not: - commit; - push; - merge; - close M98; - start M99.

Git closure happens only after explicit user manual approval.
