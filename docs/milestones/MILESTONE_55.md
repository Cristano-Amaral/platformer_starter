## Milestone 55 --- World Item Pickup & Inventory Integration

### Status

**CLOSED.** Milestone 56 is Player Inventory UI v1.

### Branch

`milestone/55-world-item-pickup`

### Cursor

**Grok 4.6 High --- Fast OFF**

### Goal

Connect the M54 runtime Inventory to authored world objects:

**See item → approach → target → press E → item enters Inventory →
pickup disappears from runtime.**

Add one concrete repeatable authored gameplay object: **Item Pickup**.
This is world acquisition, not the final player inventory screen.

### Preserve

Inspect/reuse Level Format v1; workingCopy/active/savedSourceBaseline;
authored lifecycle; Hierarchy/Inspector; Object Palette
Ground/Platform/Slope placement; M47--M50 Static Model pipeline/catalog;
staged runtime models; M51 semantic E input/targeting; M54 Inventory
API/lifecycle; Restart/checkpoint/Apply/reload; build separation.

### Authored Item Pickup

Smallest data: position, M54 `itemId`, quantity, and optionally an
existing static-model identity for visuals if this can reuse M49
narrowly. Prefer existing model identity/catalog/runtime path; if
optional, provide a primitive fallback. No ItemDefinition, GUID, script,
rarity, weight, value, behavior class, equipment/consumable data.

### Level Format v1

Keep v1. Add repeatable syntax consistent with current parser,
conceptually:
`item_pickup <px> <py> <pz> <quantity> <itemId> [<modelIdentity...>]`
Exact syntax after repository inspection. Validate finite position, M54
itemId, M54 quantity range, and canonical model identity if present.
Round-trip. Runtime collected state is never serialized. Reuse M54
validation authority where practical. No v2.

### Authored lifecycle/editor

Support Add/Duplicate/Delete/Apply/Revert/Save/Modified/Dirty and all
three authored authorities. Duplicate uses deterministic positional
offset and preserves item data. Runtime collection never deletes
authored data.

Hierarchy: `Item Pickups`. Inspector: Position, Item ID, Quantity, plus
model identity only if included. Reuse Translate only; no
Resize/Scale/Rotate unless repository constraints require it. Integrate
Add/Object Palette using existing Ground/Platform/Slope placement. Item
Pickups are not placement surfaces.

### Runtime state

Each applied pickup has transient available/collected state. New run or
Apply/reload: available. Successful collection: add through M54
production Inventory API, mark only that runtime pickup collected, stop
rendering/targeting it. Never mutate authored authorities or Dirty.

### Targeting

When not carrying a Dynamic Box, target nearby available pickups. Reuse
M51 facing/distance/deterministic concepts narrowly; no generic
Interactable. Nearest valid candidate, stable session-index tie break.
Use appropriate existing LOS so Ground/Platforms/Slopes/Doors prevent
through-wall pickup. Static Props block only if current collision/query
authority supports it. Report exact rules.

### E arbitration

Critical: M51 already maps semantic E (`grabDropPressed`). No second raw
keyboard read. Priority: 1. carrying Dynamic Box → E drops; 2. else
valid Dynamic Box grab target → E grabs; 3. else valid Item Pickup
target → E collects; 4. else nothing. One press performs at most one
action. No generic interaction dispatcher.

### Atomic collection

On E: resolve valid pickup; call `Inventory::TryAdd`; only on success
mark collected. Inventory rejection/overflow leaves pickup available. No
partial collection.

### Visuals

Render available pickup using existing staged static model if authored
and valid; otherwise recognizable primitive fallback. Target feedback
consistent with M51 and minimal `E Pick Up` prompt (itemId/quantity may
be shown narrowly). Collected pickup hidden. No icons/final inventory
UI.

### Physics

Prefer no Jolt body. Pickup is visual/query-only, not solid, not Dynamic
Box, not Pressure Plate activator, and consumes no M53 body budget. No
gravity/throw/drop/physics pickup.

### Lifecycle

Checkpoint respawn: collected stays collected; Inventory preserved. Full
Restart Run: M54 Inventory clears; all applied pickups available again.
Apply/successful reload: follow M54 run-reset authority; Inventory
resets and pickups rebuild available. PhysicsWorld rebuild alone:
preserves Inventory and pickup collected state.

### Isolation

M51 Grab/Carry keeps E priority. Pickups cannot be grabbed as boxes.
Pickups do not activate plates. Doors do not read Inventory/pickups. No
key/lock behavior.

### Asset deletion

If pickup supports Static Model identity, extend existing authored
reference protection union (workingCopy + active + savedSourceBaseline)
narrowly so referenced models cannot be deleted. Collected runtime state
does not weaken protection. No generic dependency graph.

### Out of scope

No final Inventory UI/navigation/selection; item
use/equipment/hotbar/drop-to-world; respawn timers; physics/moving
pickups; animation framework; icons;
ItemDefinition/catalog/database/.item; metadata; capacity/slots; keys
opening Doors; consumables/weapons/crafting/quests; generic
Interactable/InteractionComponent/dispatcher; event bus;
Trigger/Receiver; GUIDs/generic persistent IDs; save games; multiplayer;
ECS; prefabs; undo/redo; Level Format v2; M56.

### Focused tests

Cover parse/save/validation/repeatability; authored lifecycle;
Hierarchy/Inspector/Translate/placement; runtime availability;
range/LOS/deterministic targeting; successful Inventory add; collected
hidden/untargetable; failed add preserves pickup; one E one action;
carry→drop priority; box-target→grab priority; pickup collection
fallback; no authored Dirty mutation; checkpoint preservation; Restart
restoration + Inventory clear; Apply/reload reset; physics rebuild
preservation; no plate activation/Jolt body/body-budget change; optional
model asset protection; M54 Inventory and M51/M52/M53/M49/M50
regressions; canonical cleanup; no generic interaction/M56 scope.

### Manual acceptance

In Development with disposable fixtures: 1. Add `key x1` Item Pickup;
verify Hierarchy/Inspector/Translate; Apply. 2. Approach in Gameplay;
see target feedback / E Pick Up. 3. E once: pickup disappears; M54 F1
Inventory shows `key = 1`. 4. Same pickup cannot be collected twice. 5.
Two pickups: only target is collected. 6. Dynamic Box + pickup nearby:
box grab wins E. 7. Carrying box: E drops, does not collect. 8. Pickup
behind solid geometry/closed Door cannot be collected through it. 9.
Checkpoint respawn after collection preserves collected state +
Inventory. 10. Full Restart clears Inventory and restores pickup. 11.
Apply/reload resets consistently with M54. 12. Pickup does not activate
Pressure Plate/control Door. 13. Inventory/pickup runtime actions do not
mark level Dirty. 14. If model-backed, verify asset deletion protection.
15. Remove all fixtures before closure.

Expected canonical: Item Pickups 0, Doors 0, Pressure Plates 0, Dynamic
Boxes 0, Static Props 0; legacy removed dynamic_box line absent.

### Validation

Run relevant C++ tests for pickup, Inventory,
lifecycle/editor/placement/input arbitration/physics rebuild, M51--M54,
M49/M50, asset protection, canonical cleanup. Run:
`python tools/test_stage_runtime_assets.py`
`python tools/test_cook_level_v1.py`
`python tools/test_cook_runtime_png.py` and, if standard,
`python tools/test_import_static_glb.py`.

Build all: `cmake --preset windows-vs2022`
`cmake --build --preset windows-debug`
`cmake --build --preset windows-development`
`cmake --build --preset windows-release`

Run `git diff --check`.

### Canonical safety

No manual M55 fixture remains in canonical Level 01. Keep
`dynamic_box 0 5 0 1 1 1 30` absent. Do not overwrite unrelated semantic
changes.

### Completion

Ready when authored repeatable pickups exist; M54 itemId/quantity used;
deterministic targeting and M51 E priority work; successful collection
atomically enters Inventory and hides runtime pickup; failed add does
not consume it; checkpoint/Restart/rebuild semantics are correct; no
pickup Jolt body; M51--M54 intact; no generic interaction/final
Inventory UI; tests/builds green; canonical clean.

### STOP

After implementation, validation and report, STOP. No
commit/push/merge/M56/CLOSED. Wait for user manual acceptance and
separate Git closure.
