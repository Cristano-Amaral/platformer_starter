## Milestone 56 --- Player Inventory UI v1

### Status

**CLOSED.** Milestone 57 is Key Item & Locked Door Interaction.

Milestone 55 is CLOSED.

### Branch

`milestone/56-player-inventory-ui-v1`

### Recommended Cursor Model

**Grok 4.6 High --- Fast OFF**

------------------------------------------------------------------------

### 1. Goal

Turn the M54 runtime Inventory and M55 world acquisition loop into the
first player-facing inventory experience:

**Collect item → open Inventory → browse owned items → select an item →
inspect its ID and quantity → close Inventory → resume gameplay**

M56 is deliberately a **read-only player inventory UI v1**. It
establishes presentation, input focus, deterministic selection, and
pause/resume behavior without introducing item-use semantics.

------------------------------------------------------------------------

### 2. Preserve Current Architecture

Inspect the repository first. Preserve and reuse the current authorities
for:

-   `gameplay::Inventory` from M54;
-   M55 Item Pickup acquisition;
-   current gameplay input abstraction;
-   existing pause/editor/input-focus rules;
-   HUD rendering;
-   Development/Debug/Release separation;
-   current window/render resolution and scaling conventions;
-   Restart Run / checkpoint / Apply/reload inventory lifecycle;
-   M51 E-key Grab/Carry arbitration;
-   editor ImGui behavior.

Do not replace the M54 Inventory or create a second player inventory
model for UI.

------------------------------------------------------------------------

### 3. Runtime Ownership

The player-facing Inventory UI is transient runtime presentation state
owned at the narrowest appropriate application/gameplay UI authority.

It may track only UI concerns such as:

-   open/closed;
-   selected inventory entry/item ID;
-   navigation repeat/debounce state if required by current input
    architecture.

It must not own item quantities or duplicate Inventory entries.

`gameplay::Inventory` remains the single source of truth for contents.

------------------------------------------------------------------------

### 4. Open / Close Input

Inspect current key bindings and choose one non-conflicting semantic
inventory toggle input.

Preferred default if free:

`Tab`

Requirements:

-   use the existing input abstraction/semantic input flow;
-   do not scatter raw key reads through gameplay code;
-   pressing the inventory toggle opens the inventory during normal
    gameplay;
-   pressing it again closes the inventory;
-   `Esc` closes the inventory if it is open before performing any
    broader escape behavior that would otherwise conflict;
-   report the exact final binding and arbitration.

Do not reuse `E`, which remains M51/M55 world interaction.

------------------------------------------------------------------------

### 5. Gameplay Focus While Open

While the player inventory is open:

-   suppress player movement;
-   suppress jump;
-   suppress Grab/Drop;
-   suppress Item Pickup collection;
-   suppress other normal gameplay actions that could accidentally fire
    through the UI;
-   keep the underlying runtime world state intact.

Prefer pausing gameplay simulation while the inventory is open if this
matches the current pause architecture cleanly.

If a true simulation pause would conflict with existing architecture,
freeze/suppress the minimum relevant gameplay updates consistently and
document the exact behavior.

The inventory must not cause a Dynamic Box to be dropped merely by
opening it.

Opening the inventory while carrying a Dynamic Box preserves the carry
state. Closing resumes it.

Do not invent a generalized game-state framework.

------------------------------------------------------------------------

### 6. Player-Facing UI

Render a clear centered inventory panel suitable for the current game
presentation.

Minimum UI:

-   title: `Inventory`;
-   deterministic list/grid of currently owned entries;
-   each entry displays `itemId`;
-   each entry displays quantity;
-   clear selected-item highlight;
-   empty-state message when Inventory has no entries;
-   small control hint for navigation and close.

The UI must be available in normal gameplay builds, including Release.
This is game UI, not Development tooling.

Do not use the M54 F1 ImGui harness as the player-facing UI.

Prefer the existing game renderer/HUD path rather than introducing ImGui
into Release.

------------------------------------------------------------------------

### 7. Ordering

Use M54 Inventory deterministic enumeration as the presentation source.

Do not independently sort with a different rule unless required by UI
layout.

The visible order should therefore remain deterministic and consistent
with the production Inventory API.

------------------------------------------------------------------------

### 8. Selection Model

When the inventory opens and contains items:

-   establish one deterministic selected entry.

Preferred rule:

-   preserve the previously selected `itemId` if it still exists;
-   otherwise select the first deterministic Inventory entry.

When empty:

-   no selected item.

When inventory contents change while the UI is open or between openings:

-   if selected item still exists, preserve it;
-   if it was removed, choose the nearest valid deterministic fallback,
    preferably the first entry;
-   never hold an invalid index/reference into Inventory storage.

Prefer storing selection by stable `itemId` rather than vector
pointer/reference/index if that better survives M54 entry
insertion/removal.

Do not introduce GUIDs.

------------------------------------------------------------------------

### 9. Navigation

Support keyboard navigation through owned items.

At minimum:

-   previous item;
-   next item.

Preferred bindings if compatible with current input conventions:

-   Up / Left = previous;
-   Down / Right = next.

Wrap-around is acceptable and preferred for v1 if simple and
deterministic.

One discrete press should produce one deterministic selection step. If
current input abstraction supports key-repeat, keep it controlled and
predictable.

No mouse requirement in M56.

No gamepad requirement unless current gameplay input abstraction already
makes it trivial without expanding scope.

Report exact bindings.

------------------------------------------------------------------------

### 10. Selected Item Details

For the selected item, show only information M54 actually owns:

-   `itemId`;
-   quantity.

Do not invent:

-   display name database;
-   description;
-   icon;
-   rarity;
-   weight;
-   value;
-   category;
-   use action;
-   equipment slot.

The UI may format the canonical `itemId` directly for v1.

------------------------------------------------------------------------

### 11. Empty Inventory

If Inventory is empty:

-   opening the UI succeeds;
-   show a clear empty state such as `Inventory is empty`;
-   no selection exists;
-   navigation does nothing safely;
-   closing still works normally.

------------------------------------------------------------------------

### 12. M55 Integration

Items collected through M55 must appear in the player-facing UI
automatically because the UI reads the production M54 Inventory.

No synchronization/copy step.

Example:

-   collect `key x1`;
-   open inventory;
-   see `key` quantity `1`;
-   collect another `key x2`;
-   open inventory;
-   see one `key` entry quantity `3`.

Do not alter M55 pickup semantics.

------------------------------------------------------------------------

### 13. Inventory Mutation While UI Is Open

Normal gameplay acquisition is suppressed while the UI is open, so M55
should not mutate Inventory through world pickup at that time.

However, the UI must remain robust if Inventory contents change through
legitimate production/test/lifecycle code:

-   no dangling references;
-   selection repairs deterministically;
-   empty transition is safe.

Do not add mutation buttons to the player-facing UI.

------------------------------------------------------------------------

### 14. Lifecycle

The UI must reflect M54 lifecycle naturally.

#### Checkpoint respawn

Inventory contents remain preserved. UI selection may remain logically
preserved if the selected item still exists.

#### Full Restart Run

Inventory clears. If UI state survives the transition, selection must
become empty safely. Prefer closing the inventory as part of full run
restart if consistent with current run reset behavior.

#### Apply / successful reload

Inventory resets according to M54. UI must not retain an invalid
selected entry. Prefer closing/resetting UI presentation on committed
run reset.

#### PhysicsWorld rebuild alone

Inventory and valid UI selection remain preserved.

The UI must never become the authority for Inventory lifecycle.

------------------------------------------------------------------------

### 15. M51 Carry Isolation

Opening/closing Inventory must not mutate M51 carry state.

If player is carrying a Dynamic Box:

-   open Inventory;
-   carry state remains;
-   no Drop occurs;
-   gameplay movement/interaction is suppressed while open;
-   close Inventory;
-   carry resumes normally.

No inventory action converts a Dynamic Box into an item.

------------------------------------------------------------------------

### 16. M52 / M53 Isolation

Inventory UI does not control Pressure Plates or Doors.

If gameplay simulation is paused while inventory is open, plate/door
motion may pause consistently with the world. On close, it resumes from
the same runtime state.

Do not add Inventory → Door behavior.

------------------------------------------------------------------------

### 17. M55 Pickup Isolation

Opening Inventory near a targeted Item Pickup must not collect it.

Navigation/close keys must not trigger M55 pickup interaction.

`E` remains suppressed as gameplay interaction while inventory is open.

Closing the inventory does not automatically collect the nearby pickup;
collection requires a subsequent valid gameplay interaction press.

------------------------------------------------------------------------

### 18. M54 Development Harness

Keep the M54 `Inventory (Test)` Development harness unless current
architecture requires a very small adjustment.

It remains a Development test/debug tool.

The new player-facing Inventory UI is separate and reads the same
production Inventory.

Do not remove the harness merely because M56 adds game UI.

Do not create a second Inventory storage.

------------------------------------------------------------------------

### 19. Release Requirements

The player-facing Inventory UI must work in Release.

The Development-only M54 test harness remains absent from Release.

Do not add ImGui as a Release dependency merely for M56.

Preserve existing Debug/Development/Release architecture.

------------------------------------------------------------------------

### 20. Visual Scope

M56 should produce a clean functional v1, but avoid turning into a broad
UI-art milestone.

Allowed:

-   panel/background;
-   text;
-   selected highlight;
-   simple spacing/layout;
-   quantity labels;
-   control hints.

Out of scope:

-   item icons;
-   3D item preview;
-   animated transitions;
-   complex skins/themes;
-   inventory art asset pipeline;
-   drag/drop;
-   slot textures;
-   tooltips;
-   responsive UI framework;
-   localization system.

Visual polish can be addressed in a later dedicated pass, especially
together with the broader Content Browser/world-object visual concerns
already identified by the user.

------------------------------------------------------------------------

### 21. Explicitly Out of Scope

Do not implement:

-   item use;
-   item consume;
-   equip/unequip;
-   weapons;
-   hotbar;
-   quick slots;
-   dropping Inventory items into the world;
-   world spawning from Inventory;
-   key opening Door;
-   context actions;
-   inspect 3D model;
-   item icons;
-   ItemDefinition;
-   ItemCatalog;
-   ItemDatabase;
-   `.item` assets;
-   item metadata;
-   inventory capacity;
-   slot capacity;
-   stack limits beyond M54;
-   weight;
-   sorting modes;
-   filtering/search;
-   mouse drag/drop;
-   controller-navigation framework;
-   save-game persistence;
-   generic menu framework;
-   generalized screen/state stack;
-   generic Interactable;
-   GUIDs;
-   ECS;
-   prefabs;
-   undo/redo;
-   Level Format changes;
-   M57 functionality.

------------------------------------------------------------------------

### 22. Focused Tests

Add focused coverage for actual equivalents of:

1.  UI starts closed;
2.  toggle input opens it;
3.  toggle input closes it;
4.  Esc closes it safely;
5.  empty Inventory opens safely;
6.  empty state has no selection;
7.  non-empty open selects deterministically;
8.  M54 enumeration order is used;
9.  previous navigation;
10. next navigation;
11. wrap behavior if implemented;
12. selection preserved by itemId when still valid;
13. removed selected item repairs selection;
14. cleared Inventory clears selection;
15. no dangling pointer/reference/index behavior;
16. selected item details reflect production quantity;
17. M55 collected item appears without synchronization copy;
18. repeated M55 additions show merged M54 quantity;
19. opening UI suppresses movement;
20. opening UI suppresses jump;
21. opening UI suppresses Grab/Drop;
22. opening UI suppresses pickup collection;
23. opening UI does not drop currently carried box;
24. closing resumes gameplay;
25. closing does not synthesize an E interaction;
26. checkpoint lifecycle preserves Inventory and valid selection;
27. Restart clears Inventory and resets/closes UI safely;
28. Apply/reload resets UI safely with M54 lifecycle;
29. PhysicsWorld rebuild preserves Inventory/UI selection;
30. Pressure Plate/Door behavior remains isolated;
31. M54 Development harness still uses same Inventory;
32. Release includes player Inventory UI;
33. Release excludes Development test harness;
34. no ImGui Release dependency introduced for this UI;
35. M54 Inventory regression tests;
36. M55 Item Pickup regression tests;
37. M51 Grab/Carry regressions;
38. M52 Pressure Plate regressions;
39. M53 Door regressions;
40. M49/M50 regressions;
41. canonical Level 01 unchanged;
42. no Level Format change;
43. no item-use/equip/hotbar/M57 scope.

Do not expose public production APIs solely for tests.

------------------------------------------------------------------------

### 23. Manual Acceptance

Use Development first:

1.  Start with empty Inventory.
2.  Open player Inventory using the new gameplay binding.
3.  Verify clear empty state.
4.  Close it.
5.  Add/collect `key x1` through M55.
6.  Open Inventory and verify `key = 1`.
7.  Collect `coin x5`.
8.  Open Inventory and verify deterministic two-item navigation.
9.  Navigate previous/next and verify selected highlight/details.
10. Collect another `key x2`; verify UI shows one `key = 3`.
11. Open Inventory near a pickup; press navigation keys/E and confirm no
    accidental collection.
12. While carrying a Dynamic Box, open Inventory; confirm box is not
    dropped. Close and resume carry.
13. Confirm player movement/jump/world interaction do not fire while
    Inventory is open.
14. Checkpoint respawn: Inventory contents remain.
15. Full Restart: Inventory clears and UI resets/closes safely.
16. Apply/reload: UI and selection reset consistently with M54.
17. Exercise Pressure Plate/Door before/after opening Inventory and
    confirm no inventory-specific integration.
18. Verify M54 F1 Inventory test harness still works in Development.
19. Run Release and verify the player-facing Inventory UI works there.
20. Verify the Development `Inventory (Test)` harness is absent in
    Release.

No canonical level fixture should be required for M56. If M55 pickups
are temporarily added for testing, remove them before closure.

------------------------------------------------------------------------

### 24. Validation

Run relevant C++ tests for:

-   M56 Inventory UI;
-   input focus/arbitration;
-   M54 Inventory;
-   M55 Item Pickup;
-   M51 Grab/Carry;
-   M52 Pressure Plate;
-   M53 Door;
-   runtime lifecycle;
-   Release separation;
-   canonical cleanup.

Run existing Python regressions:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:

`git diff --check`

------------------------------------------------------------------------

### 25. Canonical Data Safety

M56 requires no Level Format change and no canonical authored data.

`game/assets/source/levels/level_01.level` should remain semantically
unchanged.

Expected closure remains:

-   Item Pickups: 0;
-   Doors: 0;
-   Pressure Plates: 0;
-   Dynamic Boxes: 0;
-   Static Props: 0.

Keep intentionally removed line absent:

`dynamic_box 0 5 0 1 1 1 30`

------------------------------------------------------------------------

### 26. Completion Criteria

M56 is ready for manual acceptance when:

-   player-facing Inventory UI opens/closes during gameplay;
-   empty state works;
-   M54 contents are shown directly;
-   deterministic item selection/navigation works;
-   selected item ID and quantity are visible;
-   M55 pickups appear automatically after collection;
-   gameplay inputs do not leak through while UI is open;
-   carrying a Dynamic Box is preserved across open/close;
-   lifecycle resets cannot leave invalid selection;
-   UI works in Release without the Development harness;
-   M54/M55/M51/M52/M53 behavior remains intact;
-   no item-use/equipment/hotbar/general menu framework is introduced;
-   tests/builds pass;
-   canonical data remains clean.

------------------------------------------------------------------------

### 27. STOP Rule

After implementation, validation and report, Cursor must STOP.

Do not commit, push, merge, start M57, or declare M56 CLOSED.

Closure occurs only after user manual acceptance and the separate Git
closure workflow.
