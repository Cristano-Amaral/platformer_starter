## Milestone 57 --- Key Item & Locked Door Interaction

### Status

**Implemented, awaiting manual acceptance.** Do not mark CLOSED. Do not
start Milestone 58.

M56 is CLOSED.

### Branch

`milestone/57-key-locked-door`

### Cursor

**Grok 4.6 High --- Fast OFF**

### Goal

Make M54--M56 Inventory affect gameplay through one concrete
interaction:

**Collect key → approach locked Door → E → consume one key → Door
unlocks → existing Pressure Plate logic can open it.**

This is a concrete Key + Door feature, not a generic
item-use/lock/requirement framework.

### Authored Door data

Extend `DoorSpec` narrowly with `bool requiresKey`, default `false`.
Existing Doors behave exactly as M53. A key-required Door starts each
new applied run locked and requires the concrete M54 item ID `key`.

Do not author arbitrary required item IDs. No key IDs, channels, GUIDs,
ItemDefinition, generic Lock or Requirement component.

### Level Format v1

Keep v1. Extend Door syntax backward-compatibly with an optional
trailing requires-key field, conceptually:
`door <cx> <cy> <cz> <sx> <sy> <sz> <openDistance> [<requiresKey>]`

Old records remain valid and mean false. Writer emits deterministic
canonical form. Strictly validate according to repository conventions.
Round-trip. No v2. Report exact syntax.

### Authored lifecycle/editor

`requiresKey` participates in workingCopy/active/savedSourceBaseline,
equality, Add/Duplicate/Delete, Apply/Revert/Save and Dirty. Add
defaults false; Duplicate preserves it. Inspector adds only
`Requires Key`.

### Runtime lock state

Each applied Door has transient runtime locked/unlocked state. New
run/Apply/reload: non-key Door unlocked; requiresKey Door locked.
Successful unlock consumes exactly one `key` via M54 and unlocks only
that runtime Door. Runtime unlocked state is not serialized and never
changes authored `requiresKey`.

### Door semantics

Preserve M53. Unlocked Door:
`desiredOpen = OR(active linked Pressure Plates)`. Locked Door:
`desiredOpen = false`. A plate can be active while the Door stays
locked. If the player unlocks while a linked plate is already active,
normal M53 opening begins. Unlocking without an active plate does not
open the Door. Preserve +Y motion, openDistance, 2.5 m/s, kinematic
solidity and obstruction-safe closing.

### Door targeting

When not carrying a Dynamic Box, allow targeting a nearby runtime-locked
Door. Reuse M51/M55 facing, deterministic nearest/tie-break and LOS
concepts. Prefer 2.5 m if appropriate. Only authored requiresKey +
currently locked Doors qualify. Report exact rules. No generic
Interactable.

### E arbitration

Reuse the single semantic M51/M55 `grabDropPressed`; no second raw E
read: 1. carrying Dynamic Box → Drop 2. else Dynamic Box target → Grab
3. else Item Pickup target → Collect 4. else locked Door target →
attempt Unlock 5. else nothing

One E performs at most one action. No generic dispatcher.

### Unlock transaction

On valid Door interaction call production
`Inventory::TryRemove("key", 1)`. Only on success mark the runtime Door
unlocked. No key means no mutation. Never unlock first and consume
later.

### Feedback

Targeted locked Door with key: conceptually `E Unlock Door (key)`.
Without key: `Requires key`. Follow current HUD/highlight conventions
narrowly. No notification/dialogue framework.

### M56 integration

M56 remains read-only. Unlock does not require Inventory UI open or
selected `key`; it checks production Inventory. Consumed quantity
automatically appears in M56. While M56 is open, world interaction stays
suppressed. No Use button.

### Lifecycle

Checkpoint respawn preserves Inventory and runtime unlocked Doors. Full
Restart clears Inventory and resets lock state from authored
`requiresKey`; M55 pickups restore normally. Apply/reload resets
Inventory per M54 and rebuilds lock state from active authored data.
PhysicsWorld rebuild alone preserves Inventory and runtime unlocked
state.

### Multiple Doors

Each key Door unlocks independently and each successful unlock consumes
one `key`. `key x2` can unlock two Doors. No key-door pairing.

### Physics

No new Jolt bodies. Existing M53 body budget remains
`fixed(5) + platformCount + dynamicBoxCount + doorCount <= 64`.

### Out of scope

No arbitrary required item IDs; named/colored keys; key-door pairing;
multiple-item requirements; item Use menu/button; consumables;
equipment; hotbar; inventory drop-to-world; lockpicking; door health;
generic Lock/Requirement/Item Action/Interactable/dispatcher/event bus;
Trigger/Receiver abstraction; GUIDs; save game; ECS; prefabs; undo/redo;
Level Format v2; visual cleanup of Content Browser/world models; M58.

### Focused tests

Cover old/new Door syntax and round-trip; invalid lock token; authored
equality/lifecycle/Inspector; non-key M53 behavior; key Door starts
locked; locked Door ignores active plate;
targeting/range/facing/LOS/tie-break; E priority Drop→Grab→Pickup→Door;
no-key failure; key consumption; last-key entry removal; only target
Door unlocks; no authored Dirty mutation; active plate opens immediately
after unlock; unlock without plate stays closed; subsequent plate OR and
obstruction behavior; checkpoint persistence; Restart relock;
Apply/reload reset; PhysicsWorld rebuild preservation; M56 consumed-key
reflection and open-UI suppression; unchanged Jolt budget; M54--M56 and
M51--M53 regressions; canonical cleanup; no generic framework.

### Manual acceptance

With disposable Development fixtures: 1. Add Door with `Requires Key`.
2. Add/link Pressure Plate. 3. Add M55 `key x1` pickup. 4. Apply/run. 5.
Put Dynamic Box on plate before key: plate active, Door remains closed.
6. Approach Door without key: `Requires key`; E does not unlock. 7.
Collect key; M56 shows it. 8. Return while plate active; E unlocks,
consumes key, Door begins normal opening. 9. Remove box: Door closes
with M53 obstruction behavior. 10. Put box back: unlocked Door opens
without another key. 11. Checkpoint after unlock: Door remains unlocked.
12. Full Restart: Door locked again, Inventory empty, key pickup
available again. 13. Verify Dynamic Box and Item Pickup E priority over
Door. 14. Open M56 near Door: no unlock until close + new E. 15.
Optionally test two locked Doors with key x2. 16. Remove all fixtures
before closure.

Canonical closure: Item Pickups 0, Doors 0, Pressure Plates 0, Dynamic
Boxes 0, Static Props 0. Keep legacy removed
`dynamic_box 0 5 0 1 1 1 30` absent.

### Validation

Run relevant C++ tests for M57, M54--M56, M51--M53, Door lifecycle,
input arbitration, PhysicsWorld rebuild, Level Format and canonical
cleanup.

Run: `python tools/test_stage_runtime_assets.py`
`python tools/test_cook_level_v1.py`
`python tools/test_cook_runtime_png.py`
`python tools/test_import_static_glb.py`

Build Debug/Development/Release using current CMake presets. Run
`git diff --check`.

### Completion

Ready when authored `requiresKey` is backward-compatible; key Doors
start locked and ignore plates; E targeting/arbitration works;
successful unlock atomically consumes one production `key`; unlocking
alone does not open Door; checkpoint/rebuild preserve unlock;
Restart/Apply/reload restore authored lock state; M56 reflects
consumption; no generic lock/item-action/interaction framework exists;
tests/builds pass; canonical data is clean.

### STOP

After implementation, validation and report, STOP. No
commit/push/merge/M58/CLOSED. Wait for user manual acceptance and
separate Git closure.
