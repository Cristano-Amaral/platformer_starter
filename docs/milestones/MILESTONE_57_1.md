## Milestone 57.1 — Specific Door Item Requirement & Pressure Plate Modes

### Status

**Implemented, awaiting manual acceptance**

Milestone 57 is CLOSED. M57.1 may begin only from a clean and synchronized `main`.

### Branch

`milestone/57.1-door-item-pressure-plate-modes`

### Recommended Cursor Model

**Grok 4.6 High — Fast OFF**

### Goal

Refine the M57 Door + Inventory interaction and expand M52/M53 Pressure Plates just enough to support useful gameplay setups:

1. Doors can require a specific Inventory `itemId`.
2. Pressure Plates can be activated by Dynamic Boxes, the Player, or both.
3. Pressure Plates can be hidden in Gameplay while remaining fully editable in the Editor.

This enables `card`, `key`, `red_key`, visible puzzle plates, and invisible player-operated automatic-door trigger volumes without introducing a generic interaction or trigger framework.

### Door item identity

A Door must reference the logical Inventory `itemId`, not a particular Item Pickup instance.

Two different Item Pickups may both grant `card`; either must satisfy a Door requiring `card`.

Do not store pickup indices, pointers, GUIDs, or runtime object references in Door authored data.

### Door authored model

Evolve the narrow M57 `requiresKey` representation into one canonical required item identity, preferably:

`std::string requiredItemId`

Semantics:

- empty = no Inventory requirement;
- non-empty = Door starts the run locked and requires one unit of that item.

Use M54 `gameplay::IsValidItemId` as the single validation authority.

Examples:

- `key`
- `card`
- `red_key`

`redKey` remains invalid because current item IDs are lowercase by contract.

Avoid redundant long-term authored state such as both `requiresKey` and `requiredItemId`, except as a narrow migration detail.

### Door Level Format v1 compatibility

Preserve Level Format v1 and M57 compatibility.

Parser must accept actual equivalents of:

- pre-M57 Door with no lock field = no requirement;
- M57 trailing `0` = no requirement;
- M57 trailing `1` = required item `key`;
- M57.1 trailing valid `itemId` = that specific item.

Preferred canonical writer:

- no requirement: trailing `0`;
- required item: trailing canonical item ID, e.g. `card`, `key`, `red_key`.

Report the exact final syntax and migration behavior.

### Door Inspector

Evolve `Requires Key` into:

- `Requires Item` checkbox;
- `Required Item` selector when enabled.

The selector should list unique deterministic `itemId` values currently authored by Item Pickups in `workingCopy`.

Selecting an option writes the `itemId`, not an Item Pickup index.

If a valid required item no longer has a matching Item Pickup in the level, do not silently erase it. Preserve and clearly display the authored identity.

No ItemCatalog or ItemDefinition system.

### Door authored lifecycle

The required item participates in:

- workingCopy;
- active;
- savedSourceBaseline;
- authored equality;
- Modified/Dirty;
- Add/Duplicate/Delete;
- Apply/Revert/Save.

Add Door defaults to no requirement. Duplicate preserves the item requirement.

Deleting an Item Pickup must not rewrite Door requirements because the reference is logical `itemId`, not pickup identity.

### Door runtime unlock

Preserve M57 runtime semantics, replacing literal `"key"` with the Door's authored required item.

For a valid targeted locked Door:

`Inventory::TryRemove(requiredItemId, 1)`

Only on success mark that runtime Door unlocked.

Wrong/missing item leaves Door and Inventory unchanged.

M56 reflects the consumed quantity automatically because it reads production Inventory.

Preserve M57 lifecycle:

- checkpoint respawn preserves runtime unlocked state;
- Restart restores authored locked state;
- Apply/reload rebuilds lock state;
- PhysicsWorld rebuild alone preserves runtime unlock state.

Unlocking still does not directly open the Door. M53 linked Pressure Plates remain the opening authority.

### E arbitration

Preserve exactly:

1. carrying Dynamic Box -> Drop;
2. else Dynamic Box target -> Grab;
3. else Item Pickup target -> Collect;
4. else locked Door target -> attempt Unlock;
5. else nothing.

One E performs at most one action. No generic dispatcher.

### Pressure Plate authored modes

Extend `PressurePlateSpec` narrowly with:

- `activateByDynamicBox`
- `activateByPlayer`
- `visibleInGameplay`

Defaults preserve existing behavior:

- `activateByDynamicBox = true`
- `activateByPlayer = false`
- `visibleInGameplay = true`

### Pressure Plate Level Format v1

Preserve old syntax and extend it backward-compatibly.

Existing conceptual syntax:

`pressure_plate <cx> <cy> <cz> <sx> <sy> <sz> [<doorIndex>]`

Preferred canonical extended form:

`pressure_plate <cx> <cy> <cz> <sx> <sy> <sz> <doorIndex> <activateByDynamicBox> <activateByPlayer> <visibleInGameplay>`

Compatibility:

- old 7-token record remains valid;
- old 8-token record remains valid;
- omitted new flags use legacy defaults box=true/player=false/visible=true;
- canonical writer emits deterministic values;
- strict `0`/`1` booleans preferred;
- no Level Format v2.

### Pressure Plate Inspector

Add:

- `Activate By Dynamic Box`
- `Activate By Player`
- `Visible In Gameplay`

Preserve Position, Size, linked Door selection, Translate and Resize behavior.

Allow:

- box only;
- player only;
- both;
- neither.

Both disabled is valid and means never active.

### Runtime activation

Use:

`active = boxActive || playerActive`

with:

`boxActive = activateByDynamicBox && any Dynamic Box overlaps plate`

`playerActive = activateByPlayer && Player overlaps plate`

Preserve existing Dynamic Box overlap semantics.

For Player overlap, reuse the actual current CharacterVirtual/player collision or query bounds. Do not approximate from only the rendered visual if an authoritative runtime collision representation exists.

No new Jolt sensor body.

### Player activation behavior

Player-enabled plates activate from physical overlap only:

- no E;
- no facing test;
- no Inventory requirement;
- continuous activation/deactivation;
- OR with Dynamic Box if both modes enabled.

Player overlap must not mutate authored data.

### Invisible Pressure Plate

If `visibleInGameplay == false`:

- suppress its normal gameplay plate rendering;
- activation still runs;
- linked Door behavior still runs;
- Development diagnostics may still count it.

It must remain visible/selectable/editable in Editor authoring mode.

The designer must still be able to select, move, resize, inspect and link it.

Do not make invisible plates disappear from Hierarchy or editor authoring visualization.

### Automatic Door use case

Support:

Pressure Plate:
- linked Door = target Door;
- box activation off;
- player activation on;
- gameplay visibility off.

Door:
- no required item.

Expected:

- player enters invisible plate volume;
- Door opens through existing M53 logic;
- player leaves;
- Door closes with existing obstruction protection.

This remains a Pressure Plate configuration, not a new trigger entity.

### Locked automatic-volume combination

Also support:

- invisible player plate is active;
- linked Door requires `card`;
- Door remains closed while locked;
- player explicitly targets Door and spends `card`;
- because the player plate is already active, Door begins opening after unlock.

Do not auto-consume an item merely because the Player overlaps a plate.

### Physics

No extra Jolt body.

Pressure Plates remain lightweight overlap/query objects.

M53 body accounting stays:

`fixed(5) + platformCount + dynamicBoxCount + doorCount <= 64`

### Explicitly out of scope

Do not implement ItemDefinition, ItemCatalog, item assets, display names, icons, multiple required items, required quantities greater than one, reusable keys, inventory Use, equipment, hotbar, drop-to-world, pickup-instance references, GUIDs, generic Lock/Requirement/Trigger/Interactable systems, dispatcher, event bus, trigger channels, damage/transition/script triggers, visual cleanup of Content Browser/world models, save-game persistence, ECS, prefabs, undo/redo, Level Format v2, or M58 functionality.

### Focused tests

Cover actual equivalents of:

#### Door
- old pre-M57 syntax -> no requirement;
- M57 `0` -> no requirement;
- M57 `1` -> `key`;
- valid `card`;
- valid `red_key`;
- invalid item ID rejected through M54 validation;
- canonical writer/round-trip;
- Add default;
- Duplicate preservation;
- Inspector selection;
- authored equality/lifecycle;
- deleting Item Pickup does not rewrite requirement;
- `card` cannot satisfy `key` Door;
- `key` cannot satisfy `card` Door;
- correct item consumes exactly one;
- final quantity removal;
- M56 reflects consumption;
- M57 E priority unchanged;
- checkpoint/Restart/Apply/reload/PhysicsWorld rebuild semantics preserved.

#### Pressure Plate
- old syntax defaults box=true/player=false/visible=true;
- new flags parse/save;
- invalid flags rejected;
- Add defaults;
- Duplicate preserves flags;
- equality/Dirty/Apply/Revert/Save;
- box-only works and ignores Player;
- player-only works and ignores Dynamic Box;
- both enabled OR correctly;
- both disabled never active;
- final qualifying overlap removal deactivates;
- Player uses runtime collision/query bounds;
- carried box only counts if box mode enabled and actually overlaps;
- invisible plate still activates;
- invisible plate hidden in Gameplay;
- invisible plate remains editor-visible/selectable;
- linked Door responds normally;
- invisible player plate supports automatic Door;
- locked Door still gates active invisible player plate;
- no extra Jolt body/body-budget change.

Also run regressions for M51 through M57, Level Format, editor lifecycle/gizmos/picking, renderer, PhysicsWorld rebuild and canonical cleanup.

Do not expose public production APIs solely for tests.

### Manual acceptance

Use disposable Development fixtures.

#### Specific Door items
1. Add Item Pickups `card`, `key`, and `red_key`.
2. Add three Doors.
3. Configure requirements respectively as `card`, `key`, and `red_key`.
4. Give them linked Pressure Plates or another clear test layout.
5. Confirm wrong items do not unlock each Door.
6. Confirm correct item unlocks only its Door and consumes one.
7. Confirm M56 immediately reflects consumption.

#### Player plate
8. Create a plate with Dynamic Box off, Player on.
9. Link it to an unlocked Door.
10. Walk onto it: Door opens.
11. Walk away: Door closes.
12. Put Dynamic Box on it: box alone does not activate.

#### Box regression
13. Create box=true/player=false plate.
14. Player standing on it does not activate.
15. Dynamic Box does.

#### Both
16. Create box=true/player=true plate.
17. Either source activates.
18. Removing one source while the other remains keeps it active.
19. Removing the last source deactivates.

#### Invisible automatic Door
20. Configure player-only + invisible plate linked to unlocked Door.
21. In Gameplay the plate is not rendered.
22. Entering volume opens Door.
23. Leaving closes Door.
24. Return to Editor and verify the plate remains authorable.

#### Invisible plate + locked Door
25. Link invisible player plate to Door requiring `card`.
26. Enter without card: Door stays closed.
27. Acquire card.
28. While plate is active, explicitly target Door and press E.
29. Card is consumed and Door opens.

#### Lifecycle
30. Checkpoint preserves unlocked Door.
31. Full Restart relocks and resets Inventory/pickups according to existing rules.
32. Apply/reload rebuilds authored settings.
33. PhysicsWorld rebuild alone preserves runtime Door unlock.

Remove all disposable fixtures before closure.

### Validation

Run relevant current C++ tests plus:

`python tools/test_stage_runtime_assets.py`

`python tools/test_cook_level_v1.py`

`python tools/test_cook_runtime_png.py`

`python tools/test_import_static_glb.py`

Build:

`cmake --preset windows-vs2022`

`cmake --build --preset windows-debug`

`cmake --build --preset windows-development`

`cmake --build --preset windows-release`

Run:

`git diff --check`

### Canonical data safety

M57.1 changes Door and Pressure Plate serialization behavior but must leave canonical Level 01 without test fixtures.

Expected closure:

- Item Pickups: 0
- Doors: 0
- Pressure Plates: 0
- Dynamic Boxes: 0
- Static Props: 0

Keep absent:

`dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

### Completion criteria

Ready for manual acceptance when:

- Doors require a specific valid Inventory `itemId`;
- M57 `0/1` Door records remain compatible;
- M57 `1` maps semantically to `key`;
- Door Inspector offers authored Item Pickup item IDs without storing pickup indices;
- only the correct item unlocks;
- one item is consumed atomically;
- Pressure Plates independently support Dynamic Box and Player activation;
- activation sources OR correctly;
- plates can be invisible in Gameplay while remaining editor-authorable;
- invisible player plates can implement automatic Doors;
- locked Doors still gate active plates;
- no extra physics bodies are introduced;
- M51-M57 regressions remain green;
- no generic requirement/trigger/interactable framework is introduced;
- canonical data is clean.

### STOP

After implementation, validation and report, STOP.

Do not commit, push, merge, start M58, or declare M57.1 CLOSED.

Wait for user manual acceptance and the separate Git closure workflow.
