# Milestone 100 --- Inventory & Equipment Foundation

## Status

IMPLEMENTED --- awaiting manual acceptance.

## Branch

`milestone/100-inventory-equipment-foundation`

## Recommended Agent / Model

Cursor --- Grok 4.6 High --- Fast OFF

## Objective

Replace the legacy numeric-item inventory authority with an
ItemDefinition-backed Inventory and establish a typed Equipment
foundation.

M100 makes M99 `items/<name>` identities authoritative for Inventory and
Item Pickup gameplay. It adds basic equip/unequip and typed equipment
slots, but does not yet apply equipment stat modifiers to
Player/Character stats.

## Core Contract

After M100: - Item Pickup -\> `items/<identity>` -\> ItemDefinition -
Inventory -\> stacks of ItemDefinition identities - Equipment -\> slots
containing ItemDefinition identities - ItemDefinition remains owner of
Display Name, Icon, World Model, Item Type, stack policy, equipment
metadata, and stat modifiers. - Runtime indices may be derived
internally but are never authored/serialized item identity. - Content
Browser assets are resources, not a second Item authority.

Do not retain legacy numeric `itemId` as a competing semantic authority.

## Inventory

Inventory stacks contain only stable Item identity plus quantity.
Resolve ItemDefinition for authored metadata.

Use M99 stack policy: - non-stackable: maximum one per stack; -
stackable: ItemDefinition Max Stack; - additions split into stacks as
needed; - removal is deterministic; - zero-quantity stacks do not
remain; - missing/wrong-category definitions never silently resolve to
another Item.

Preserve existing capacity semantics. Do not add
weight/volume/encumbrance speculatively.

## Equipment

Add typed slots: - Head - Body - MainHand - OffHand - Accessory

Extend ItemDefinition only as required with typed Equipment Slot
metadata. Equipment Items must have valid slot semantics; contradictory
persisted data must fail validation.

Equipment state contains at most one Item identity per slot.

Required operations: - CanEquip - Equip - Unequip - query equipped Item
by slot

Preferred contract: - Equip consumes one matching Inventory Item and
places it in its compatible slot. - Unequip returns one Item to
Inventory. - Replacing an occupied slot is atomic: never lose/duplicate
either Item if the transaction cannot complete.

M100 does not apply Item stat modifiers to Player stats.

## Item Pickup Migration

Migrate Item Pickup from legacy numeric `itemId` authority to textual
ItemDefinition identity (`items/<name>`).

Evolve Level Format v1 narrowly and backward-compatibly: - canonical
existing levels remain readable; - legacy numeric pickup records get an
explicit deterministic compatibility/migration path; - newly saved
authored pickups use ItemDefinition identity; - no heuristic matching; -
no silent fallback; - malformed/missing/wrong-category references are
diagnosed.

Document the exact legacy -\> ItemDefinition compatibility rule.

Where appropriate, pickup presentation should derive World Model from
ItemDefinition rather than duplicate a second asset authority. Do not
add a raw Content Browser override mode.

## Inventory UI

Upgrade the current player Inventory UI to display ItemDefinition-backed
data: - Display Name - quantity - Icon when available - explicit
missing-definition state

Provide practical selection suitable for equipment actions. Do not build
a final production inventory UX.

## Equipment UI

Add a simple Equipment section: - five typed slots; - equipped Display
Name/Icon; - equip eligible selected Inventory Item; - unequip equipped
Item; - clearly reject/disable incompatible Items.

Buttons/context actions are sufficient; no drag/drop framework is
required.

## M99 Item Database Editor

Extend only as needed to author Equipment Slot metadata: - typed slot
control for Equipment Items; - deterministic persistence; - validation.

Do not redesign M99.

## Runtime / Save Persistence Boundary

Inspect current runtime/save facilities first.

Persist Item identity wherever Inventory state is already expected to
persist. Do not invent a generalized save-game framework.

If current Inventory is session-only, Equipment may remain session-only,
but all architecture must use stable textual identities suitable for
later persistence. Document the actual boundary.

## Development Diagnostics

Expose enough Development diagnostics to verify: - Inventory
identity/quantity; - ItemDefinition resolution; - Equipment slot
contents; - missing-definition behavior; - legacy Item Pickup
compatibility where applicable.

Preserve M98/M99 diagnostics.

## Failure Semantics

Explicitly handle: - malformed/missing/wrong-category Item identity; -
stack overflow; - non-stackable overflow; - equipping non-Equipment
Item; - slot mismatch; - occupied-slot replacement; - failed unequip if
Inventory cannot accept return; - missing Item Pickup definition; -
unmappable legacy numeric pickup.

Failures preserve state: no loss, duplication, partial swap, or silent
fallback.

## Compatibility

Preserve unrelated systems: Player movement/jump, M98/M99
definitions/editor, Characters, Terrain/Paint, Vegetation, Ground Cover,
Content Browser, asset catalogs, and Level Format v1 limits.

M100 intentionally changes Inventory/Item Pickup item-reference
authority; that migration must be explicit and tested.

## Automated Tests

Cover at minimum: 1. identity-backed inventory add/remove; 2.
non-stackable behavior; 3. max-stack behavior and splitting; 4.
deterministic removal/order; 5. missing/wrong-category definition
handling; 6. Equipment Slot enum serialization; 7. ItemDefinition
Equipment Slot validation; 8. equip eligible Item; 9. reject
non-Equipment and slot mismatch; 10. unequip return; 11. occupied-slot
atomic replacement; 12. failed transaction leaves state unchanged; 13.
Inventory UI ItemDefinition resolution; 14. Equipment UI resolution; 15.
Item Pickup textual identity resolution; 16. legacy Item Pickup
compatibility; 17. missing pickup definition; 18. pickup -\> Inventory
behavior; 19. Item Database Equipment Slot persistence round-trip; 20.
M98/M99 regressions; 21. Level parser/writer round-trip for migrated
pickups; 22. canonical-level compatibility.

Use production paths for persistence/reference migration where
practical.

## Manual Acceptance

After validation, STOP for user acceptance. Intended acceptance: 1.
Development starts normally. 2. M99 Item Database remains valid. 3.
Create/edit a temporary Equipment Item and choose its Equipment Slot. 4.
Save/restart and verify metadata persistence. 5. Use an Item Pickup
referencing an ItemDefinition through the supported editor workflow. 6.
Pickup adds the correct definition-backed Item to Inventory. 7.
Inventory shows Display Name/Icon/quantity from ItemDefinition. 8.
Stackable Item respects Max Stack. 9. Non-stackable Item does not merge
above one. 10. Equip an eligible Item. 11. Inventory quantity and
Equipment slot update correctly. 12. Unequip returns it. 13.
Incompatible Item is rejected without corruption. 14. Existing canonical
pickup gameplay works through compatibility/migration. 15. Diagnostics
show identity-backed Inventory/Equipment. 16. Player movement/jump
remain normal. 17. Content Browser and Item Database remain normal. 18.
Restart behavior matches documented persistence boundary.

## Out of Scope

Do not implement: - equipment stat modifiers applied to
Player/Character; - Character Database/editor; - Player -\>
CharacterDefinition conversion; - consumption effects; -
weapons/combat; - armor rendering; - skeletal sockets/held-item
attachment; - animation integration; - Enemy/NPC/Animal inventory; -
loot/crafting/economy; - weight/encumbrance; -
rarity/durability/randomized instances; - generalized save-game
framework; - hotbar/action bar; - GUIDs, ECS migration, prefabs,
undo/redo; - generalized property/reflection systems; - Content Browser
as a second Item authority.

## Expected Follow-on

M100 prepares for: - M101 --- Character Database & Character Editor
Foundation - M102 --- Character Stats / Player Definition / Equipment
Integration

Future scopes remain unfrozen until prior milestones close.

## Implementation Expectations

Before editing inspect: - `AGENTS.md` - `DEVELOPMENT_WORKFLOW.md` -
M98/M99/M100 milestone docs - GameplayDefinition/ItemDefinition code and
tests - current Inventory implementation/tests/UI - current Item Pickup
grammar/runtime/editor/tests - Level Format v1 parser/writer limits -
M99 Item Database editor - current runtime/save persistence facilities

Repository code/tests/docs are authoritative. Prefer one explicit
migration over parallel old/new item systems.

## Builds

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

Do not use `git restore .` or `git clean -fd`. Do not normalize
canonical level EOLs. If pickup migration genuinely requires a semantic
canonical-level change, report the exact diff/reason for user review
instead of hiding/restoring it.

## Completion / STOP

Report: 1. files changed; 2. Inventory representation/stack semantics;
3. Equipment Slot enum and ItemDefinition extension; 4. equip/unequip
transaction semantics; 5. Item Pickup old/new grammar and compatibility;
6. pickup visual authority; 7. Inventory/Equipment UI; 8. M99 editor
changes; 9. persistence boundary; 10. missing/failure behavior; 11.
diagnostics; 12. tests/build/Python results; 13. `git diff --check`; 14.
canonical-level diffs; 15. exact manual acceptance; 16. deferred
limitations.

Then STOP. Do not commit, push, merge, close M100, or start M101. Git
closure occurs only after explicit manual approval.
