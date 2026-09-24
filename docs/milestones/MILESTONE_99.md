# Milestone 99 --- Item Database & Item Editor

## Status

Implemented, awaiting manual acceptance.

## Branch

`milestone/99-item-database-editor`

## Recommended Agent / Model

Cursor --- Grok 4.6 High --- Fast OFF

## Objective

Build the first concrete authoring system on top of M98: a central Item
Database and Development editor for reusable Item definitions.

M99 extends the M98 Item category into a practical typed ItemDefinition
schema and authoring workflow while preserving stable textual
identities. It must not implement Inventory/Equipment integration, Item
Pickup migration, item-use effects, Characters, or a generalized
gameplay database framework.

## Core Contract

-   Item definitions are reusable gameplay definitions.
-   Authored identity remains the stable key (`items/<name>`).
-   Runtime ordering/indexing is never the authored identity.
-   The Item Database owns Item definition data.
-   Use explicit typed fields/enums, not generic key/value or JSON
    property bags.
-   M98 Character definitions remain valid.

## ItemDefinition

At minimum support: - Identity --- M98 `items/<name>` - Display Name ---
human-readable authored name - Description --- short authored
description - Item Type --- typed enum - Stackable --- bool - Max Stack
--- bounded integer - World Model --- optional static-model identity
using existing catalog conventions - Icon Texture --- optional PNG
identity using existing texture/content conventions - Stat Modifiers ---
zero or more M98 typed `GameplayStatModifier` entries

Initial Item Type vocabulary: - Generic - Consumable - Equipment - Key -
Quest

Item Type is classification only in M99; it must not automatically
create gameplay behavior.

Stack invariants must be deterministic. At minimum `maxStack >= 1`;
non-stackable Items behave as max stack 1. Contradictory persisted data
should preferably be rejected rather than silently rewritten.

Stat modifiers reuse the M98 typed stat vocabulary and additive
semantics. Invalid/non-finite addends are rejected. M99 does not apply
modifiers to the Player.

## Persistence

Evolve the M98 authored gameplay-definition persistence in the smallest
repository-consistent way, preferably
`game/assets/source/gameplay/definitions.gameplay`.

Requirements: - existing valid M98 Character definitions remain
readable; - deterministic bounded read/write; - deterministic writer
output; - clear errors for malformed records and unknown Item Type/stat
tokens; - optional model/icon identities survive even if the asset is
missing; - mandatory round-trip coverage; - no JSON, GUIDs,
reflection/schema framework, or speculative versioning framework.

## Item Database Editor

Add a Development-only central Item Database editor using existing UI
conventions.

It must support: - open Item Database; - list Item definitions; - stable
selection; - search/filter by identity and/or display name; - create
Item; - edit selected Item; - delete Item safely; - Save; - Reload; -
visible dirty state; - clear validation/errors.

The selected Item editor exposes all typed M99 fields.

Creating an Item must result in a valid unique `items/<name>` identity
and must never silently overwrite an existing definition.

Identity rename may be supported only if safe and explicit. If
repository inspection shows no useful safe rename contract yet, identity
may remain immutable after creation; document the choice.

Deletion must guard known M99 authored references. Do not invent
integration with the legacy Inventory or Item Pickup systems solely to
create delete guards.

## Asset Pickers

### World Model

Reuse `StaticModelCatalog` and established model thumbnail/card picker
patterns.

Support: - Assign/Replace/Clear; - Search; - visible cards/thumbnails; -
explicit empty state; - correct popup input capture/pointer lock.

### Icon Texture

Reuse `SourceTextureCatalog` and existing texture/content-browser
conventions.

Support: - Assign/Replace/Clear; - Search; - thumbnail/card selection; -
explicit empty state; - preservation/diagnosis of missing authored
identities.

Do not create parallel asset catalogs or import pipelines.

## Authoring Lifecycle

Use the smallest lifecycle appropriate to a standalone database/editor.

At minimum: - unsaved changes are visible; - Save validates before
writing; - Save is deterministic; - failed Save must not corrupt the
previous valid file; - Reload must not silently discard dirty edits
without warning/confirmation.

Do not force the Level Editor workingCopy/active lifecycle onto this
standalone database unless repository conventions genuinely require it.

## M98 Diagnostics

The Development F1 Gameplay Definitions inspection may be updated for
richer Items but must continue to demonstrate: - valid resolution; -
missing resolution; - category mismatch; - textual identity authority.

## Compatibility

Preserve: - M98 Character definitions; - current Player behavior; -
current M54 Inventory behavior; - current Item Pickup behavior; - Level
Format v1; - Terrain/Vegetation/Ground Cover; - Content Browser and
existing asset pickers.

M99 must not connect the new Item Database to current Inventory or Item
Pickups.

## Validation and Tests

Explicitly cover: 1. Item Type name ↔ enum conversion; 2. ItemDefinition
defaults; 3. stack validation; 4. typed modifier persistence/validation;
5. optional model/icon persistence; 6. missing asset identity
preservation; 7. Item serialization round-trip; 8. M98 Character
compatibility; 9. unique Item creation; 10. duplicate identity
rejection; 11. delete semantics; 12. search/filter behavior; 13. model
picker select/clear; 14. icon picker select/clear; 15. popup opens with
zero results; 16. picker card hit target; 17. pointer/input capture; 18.
dirty/save/reload behavior; 19. invalid data is not persisted; 20. M98
registry/reference regression.

Run relevant existing Content Browser, Static Model, Texture, Inventory,
Item Pickup, Level, and authoring tests for touched code.

## Manual Acceptance

After automated validation, STOP for user acceptance. The report must
give exact concrete steps.

Acceptance should prove at minimum: - Development starts normally; -
Item Database editor opens; - M98 fixture Items are visible; - create a
temporary valid Item; - edit Display Name, Description, Item Type,
Stackable/Max Stack; - Assign/Replace/Clear World Model through existing
model catalog picker; - Assign/Replace/Clear Icon through texture
picker; - add/edit/remove a typed stat modifier; - Save and verify
persistence after Reload/restart; - delete the temporary Item and
Save; - M98 valid/missing/category-mismatch diagnostics still work; -
PLAY/movement/jump remain normal; - Tab/current Inventory remains
unchanged; - existing Item Pickup behavior remains unchanged; - F2
Content Browser remains normal.

Do not require destructive canonical-level edits.

## Explicitly Out of Scope

Do not implement: - new Inventory architecture; - Item Database ↔
current Inventory integration; - Equipment slots/UI or equip/unequip; -
applying Item modifiers to Player; - item use/consumption effects; -
Item Pickup migration; - item drop/spawn gameplay; - loot tables,
crafting, shops/economy; - speculative rarity system; - Character
Database/editor; - Player conversion to CharacterDefinition; -
Enemy/NPC/Animal systems; - animation, sockets, AI; - quests/dialogue; -
vehicles; - ECS migration; - GUIDs; - prefabs; - undo/redo; -
generalized reflection/property systems; - generic JSON/key-value
gameplay data; - generalized asset database.

## Expected Follow-on Direction

M99 should prepare cleanly for: - M100 --- Inventory & Equipment
Foundation - M101 --- Character Database & Character Editor Foundation -
M102 --- Character Stats / Player Definition / Equipment Integration

Future milestone details remain unfrozen until prior milestones close.

## Implementation Expectations

Before editing inspect: - `AGENTS.md` - `DEVELOPMENT_WORKFLOW.md` -
`docs/milestones/MILESTONE_98.md` - M98 gameplay-definition
implementation/tests - current Inventory/Item Pickup only for
compatibility - StaticModelCatalog/model picker patterns -
SourceTextureCatalog/texture picker patterns - Content Browser
card/thumbnail patterns - existing standalone editor save/dirty/reload
patterns

Repository code/docs are authoritative. Reuse existing catalogs and UX
rather than creating parallel systems.

## Builds and Validation

Run:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run applicable C++ tests and:

``` text
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

## Canonical Data Safety

Before completion:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

Do not use `git restore .` or `git clean -fd`. Do not normalize line
endings or mechanically rewrite canonical levels.

## Completion / STOP

Report: - files changed; - ItemDefinition schema/defaults; - Item Type
vocabulary; - persistence grammar; - editor entry point/UX; - asset
picker reuse; - dirty/save/reload lifecycle; - validation/delete
behavior; - tests/builds/Python results; - canonical-level diffs; -
exact manual acceptance steps; - intentionally deferred limitations.

Then STOP. Do not commit, push, merge, close M99, or start M100. Git
closure occurs only after explicit manual approval.
