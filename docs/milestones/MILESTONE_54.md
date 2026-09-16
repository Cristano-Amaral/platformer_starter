## Milestone 54 --- Player Inventory System v1

### Status

**Implemented — awaiting manual acceptance**

Milestone 53 is CLOSED. M54 begins only from clean synchronized `main`.

### Branch

`milestone/54-player-inventory-v1`

### Cursor

**Grok 4.6 High --- Fast OFF**

### Goal

Introduce a small reliable **player runtime inventory** without
connecting it to world pickups yet:

**item identity → quantity → Add / Remove / Query → runtime player
inventory → Development inspection**

Inventory is runtime gameplay state, not authored Level state. M54 does
not implement world pickups, Static Prop interaction, `E` pickup,
polished inventory UI, equipment, item assets/catalogs, save-game
persistence, or generic interaction architecture. Those integrations
remain later scope, especially M55.

### Preserve current architecture

Inspect and reuse current player/session lifecycle, input/HUD/debug
conventions, Level Format v1 authority, editor
`workingCopy`/`active`/`savedSourceBaseline`, Restart Run, checkpoint
respawn, PhysicsWorld rebuild, M51 Grab/Carry, M52 Pressure Plates, M53
Doors, and Development/Debug/Release separation.

Do not put Inventory into LevelDefinition and do not create parallel
runtime authority.

### Inventory ownership

Add one runtime Inventory at the narrowest existing
gameplay/player-session authority. Contents are transient and never
mutate or serialize `workingCopy`, `active`, `savedSourceBaseline`,
Modified/Dirty, or Level Format.

### Item identity

Use a bounded canonical string/token `itemId`. Follow existing
repository token-validation conventions where possible. Make case policy
explicit and deterministic. Tests may use IDs such as `key`, `coin`,
`battery`.

Do not add GUIDs, ItemDefinition assets/catalog/database, icons, meshes,
descriptions, rarity, value, weight, or classes.

### Data model/API

Smallest model: unique canonical `itemId` + positive integral quantity.
No duplicate logical IDs.

Production operations, using repository naming: - query quantity; -
`Has(itemId, quantity)`; - `TryAdd(itemId, quantity)`; -
`TryRemove(itemId, quantity)`; - `Clear()`; - read-only deterministic
enumeration.

Missing item returns zero. Add positive N increases quantity. Remove
succeeds only when enough exists; exact removal removes the entry.
Invalid IDs/quantities, over-removal and overflow fail without mutation.
Use bounded safe arithmetic.

No slots, capacity, weight, per-stack limits, grid, hotbar or equipment.

### Runtime lifecycle

-   Initial/new gameplay session: empty inventory.
-   **Checkpoint respawn preserves Inventory.**
-   **Full Restart Run clears Inventory to empty.**
-   PhysicsWorld rebuild alone must not clear Inventory.
-   Apply/reload must not accidentally make Inventory physics-owned.
    Inspect actual lifecycle and document exact boundary; only an
    operation that semantically starts/restarts the full run may clear
    it.

### M51/M52/M53 isolation

Dynamic Boxes are not inventory items. Grab/Carry does not alter
inventory. Pressure Plates and Doors do not read inventory. Do not make
keys open Doors. No item-trigger integration.

### Development inspection

Add a narrow **Development-only** inspection/test harness, preferably in
an existing gameplay/debug/tool window rather than a large new UI.

It must show current item IDs/quantities and allow manual: - Add by
itemId + quantity; - Remove by itemId + quantity; - Clear.

Use the production mutation API. This is test tooling, not final
inventory UI. No icons, grid, drag/drop, inventory screen or polished
UX. It must not appear in Release; preserve current Debug/ImGui
separation.

### Safety/determinism

Canonical validation, no duplicate logical IDs, overflow-safe
operations, non-mutating failures, deterministic enumeration, no
dangling references, no physics BodyID or authored-index dependency.

### Explicitly out of scope

No world pickups; Collectible→inventory conversion; Static Prop pickup;
`E` collect interaction; generic Interactable/Interaction component;
item definition catalog/database or `.item` format;
icons/thumbnails/models/descriptions/rarity/value/weight;
capacity/slots/per-stack
max/grid/drag-drop/sorting/filtering/hotbar/equipment/weapons/consumable
use; dropping items into world; crafting; quests; keys/locks; save-game
or cross-launch persistence; checkpoint inventory snapshots;
multiplayer/replication; inventory events/event bus; trigger/receiver
integration; GUIDs; ECS; prefabs; undo/redo; Level Format v2;
generalized Game Feature Configuration; M55 functionality.

### Focused tests

Cover actual equivalents of: 1. new Inventory empty; 2. valid canonical
ID accepted; 3. empty/invalid ID rejected; 4. case policy enforced; 5.
Add one; 6. Add quantity; 7. repeated Add merges logical entry; 8.
missing query returns zero; 9. Has exact; 10. Has over-owned false; 11.
partial Remove; 12. exact Remove removes entry; 13. over-remove fails
unchanged; 14. zero/invalid quantity rejected; 15. overflow fails
unchanged; 16. invalid Add/Remove non-mutating; 17. Clear; 18.
deterministic unique enumeration; 19. Restart Run clears; 20. checkpoint
respawn preserves; 21. PhysicsWorld rebuild preserves; 22. M51/M52/M53
updates do not mutate inventory; 23. inventory mutation does not mark
editor Modified/Dirty; 24. inventory absent from Level serialization;
25. Apply does not make inventory authored; 26. Development
Add/Remove/Clear use production API; 27. Release has no ImGui inventory
tool; 28. M51 regressions; 29. M52 regressions; 30. M53 regressions; 31.
M49/M50 regressions; 32. canonical Level 01 unchanged; 33. no M55
pickup/interaction scope.

Do not expose public production APIs solely for tests; reuse established
test-access conventions.

### Manual acceptance

Development: 1. Start Gameplay; inventory empty. 2. Add `key` x1; then
x2; confirm one `key = 3`. 3. Add `coin` x5; identities independent. 4.
Remove `key` x1; confirm 2. Attempt over-remove; unchanged. Remove final
2; entry disappears. 5. Add items and Clear; inventory empty. 6. Add
item, checkpoint respawn; inventory preserved. 7. Add item, full Restart
Run; inventory cleared. 8. Exercise Grab/Carry, Pressure Plate and Door;
none mutate inventory. 9. Inventory operations do not mark level
Modified/Dirty and Save does not serialize contents. 10. Release has no
Development inventory tool.

No canonical Level 01 fixture should be needed.

### Validation

Run relevant C++ tests: Inventory, player/session lifecycle, Restart,
checkpoint, Physics rebuild, editor isolation, M51, M52, M53, M49/M50,
LevelFile/canonical cleanup.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

If still standard: `python tools/test_import_static_glb.py`.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check`.

### Canonical data safety

M54 should require no canonical level edits. Verify
`game/assets/source/levels/level_01.level` remains semantically
unchanged: 0 Doors, 0 Pressure Plates, 0 Dynamic Boxes, 0 Static Props.
Keep legacy `dynamic_box 0 5 0 1 1 1 30` absent.

### Completion

Ready for manual acceptance when runtime Player Inventory exists; item
IDs/quantities are safe; Add/Remove/Has/Query/Clear work; checkpoint
preserves; Restart clears; physics rebuild preserves; inventory is not
Level-authored state; Development has narrow inspection tooling; Release
has no Development inventory UI; M51/M52/M53 remain unaffected; no world
pickup/generic interaction system exists; tests/builds pass; canonical
data is unchanged.

### STOP

After implementation, validation and report, STOP. Do not commit, push,
merge, start M55, or declare M54 CLOSED. Closure occurs only after user
manual acceptance and separate Git closure.
