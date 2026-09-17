# Milestone 64 --- Multiple Levels & Level Transition

## Status

**Planned.** M63 is CLOSED. Start only from clean synchronized `main`.

## Canonical Path

`docs/milestones/MILESTONE_64.md`

## Branch

`milestone/64-multiple-levels-transition`

## Cursor Model

**Grok 4.6 High --- Fast OFF**

## Goal

Evolve the M63 authored Level Goal into the narrowest useful multi-level
progression: a destination-bearing goal can safely transition the
running game from one staged/cooked level to another, proving at least
`level_01 -> level_02`.

M64 is not a campaign system.

## Context / Authority

Follow the post-M60 contract. Read this file first, then current
code/tests/architecture/relevant docs. Read older milestones only for
concrete dependencies.

Authority: current code → tests → current architecture/docs → active
milestone → latest relevant current docs → older history.

## Preserve

Preserve Level Format v1; workingCopy → Apply → active; Save authored
state; M63 repeatable goals, player-only overlap, idempotent completion,
editor/gameplay visualization; current Restart/checkpoint semantics; all
authored gameplay categories; Inventory/UI; E priority Drop → Grab →
Collect → Unlock; M58--M62; staged runtime authority;
Development/Release separation.

## Goal Destination

Preferred v1 evolution:

``` text
level_goal <cx> <cy> <cz> <sx> <sy> <sz> [<nextLevelId>]
```

Examples:

``` text
level_goal -21 3.8 0 2 1.6 1.8 level_02
level_goal 10 2 4 2 2 2
```

Existing 7-token M63 goals remain terminal and valid. A
destination-bearing goal completes through the existing M63 authority,
schedules one safe deferred transition, loads the staged destination,
rebuilds it as a fresh runtime, and continues Gameplay there.

If attaching destination to the goal conflicts with established current
architecture, use the narrowest equivalent authored representation and
report why. Do not build a campaign graph.

## Level Identity / Safety

Prefer existing level basename/stem identities such as `level_01`,
`level_02`. Reuse current safe asset/path conventions. Reject absolute
paths, traversal and unsafe IDs. Resolve runtime destinations only
through staged runtime level authority. No source-tree fallback. No
GUIDs.

## Level Format v1

Keep v1. Existing destination-less `level_goal` records remain valid.
Destination records add only the optional identity token. Writer emits
it only when non-empty; round-trip preserves it; malformed/unsafe IDs
reject cleanly; v1 limits remain. Never restore legacy singleton `goal`.

Preferred authored addition is only `std::string nextLevelId` (empty =
terminal). No goal ID, transition type, spawn name, campaign node,
requirements, visual fields or arbitrary path.

## Editor

Expose `Next Level` in Level Goal Inspector. Empty means terminal;
e.g. `level_02` means destination. Reuse a safe existing level
catalog/combo if already available; otherwise a validated text field is
sufficient. Do not build a new catalog framework solely for M64.

Preserve Palette, selection, picking, Translate, Resize,
Duplicate/Delete, Apply, Save/reload and Dirty/equality. Duplicate
copies destination. No new gizmo mode.

## Level 02

Add a minimal intentional source level at
`game/assets/source/levels/level_02.level`, visually distinguishable
from level_01 and valid through the normal cook/stage pipeline. Keep it
small; this is not a level-design milestone. It may be terminal.

Generated/cooked/staged counterparts must come through existing tooling
rather than divergent hand-maintained copies.

## Staging / Cooking

Both levels must follow existing Debug/Development/Release cook/stage
conventions. Transition loads destination from staged runtime assets
only. Missing/malformed destination fails safely. Release includes
level_02. Do not create a generic asset database.

## Safe Deferred Transition

Do not replace levels from an unsafe physics/render/iteration callback.
Use the narrowest safe Application/runtime orchestration point.

Conceptually:

``` text
player overlaps destination goal
→ M63 completion succeeds once
→ capture pending nextLevelId
→ finish safe update step
→ load + validate staged destination
→ replace active level
→ rebuild physics/runtime
→ reset destination-local transient state
→ continue Gameplay
```

No generic SceneManager.

## Completion Feedback

M63 completion remains authoritative. Destination-bearing goals may
transition at the next safe point without Continue. No new
fade/timer/loading-screen framework. A brief Level Complete frame is
acceptable. Terminal goals retain persistent M63 HUD / Enter-to-Restart
behavior.

## Fresh Destination Runtime

Successful transition starts the destination as a fresh level/run. Reset
actual equivalents of player spawn/transform, physics, completion,
checkpoint, pickup collected state, M61/M62 feedback, grab/drop state,
Door/Plate runtime state, run timer and other level-local transient
state found during inspection.

### Inventory

**Preserve Inventory across a successful level-to-level transition.**

This is session-only carry-over, not savegame/campaign persistence.
Consumed items remain consumed. M64 does not redefine existing Restart
Inventory semantics.

## Failure Safety

Missing/invalid/malformed destination must not half-switch runtime.
Validate before destructive replacement. On failure: current
level/physics/Inventory remain coherent; completed state remains; report
narrowly through existing logging/HUD conventions; do not retry every
frame; no source fallback. Empty destination is terminal, not error.

## Editor Authority

Keep Development editor coherent. Do not silently discard unsaved
workingCopy edits during a runtime transition. Respect current
Apply/Dirty authority. Reinitialize editor authored state to destination
only where current architecture considers it safe and without false
Dirty. No multi-document editor.

## Restart / Checkpoint

After transition, Restart restarts the **currently loaded destination
level**, not level_01. Existing Restart Inventory semantics remain.

Checkpoint state is level-local and must not carry from level_01 to
level_02.

The Application must track current runtime level identity narrowly
enough for restart/source-destination reporting without hardcoding
level_01 or introducing campaign ordering.

## Release / Performance

End-to-end staged transition works in Release without ImGui/source
fallback. No per-frame level file I/O except actual transition attempt;
no repeated failed-load loop; safely replace old physics/runtime
resources; no unbounded history/general cache.

## Focused Tests

Cover equivalents of: 1. old M63 terminal goal still parses; 2.
destination goal parse/write/round-trip; 3. unsafe destination rejects;
4. Duplicate/Dirty/Apply/Save/reload preserve destination; 5. level_02
validates/cooks/stages; 6. completion schedules exactly one deferred
transition; 7. successful transition replaces runtime safely; 8. player
starts at destination spawn; 9.
completion/checkpoint/pickup/M61/M62/grab/Door/Plate level-local state
resets; 10. Inventory survives transition and consumed items stay
consumed; 11. Restart after transition restarts destination; 12.
terminal goal retains M63 behavior; 13. missing/malformed destination
fails without partial replacement or retry storm; 14. failed transition
preserves coherent physics/runtime/Inventory; 15. staged-only
authority/no source fallback; 16. Release transition works; 17. existing
gameplay/E arbitration regressions remain green; 18. canonical data
safety remains correct.

Avoid generic SceneManager/campaign APIs solely for tests.

## Manual Acceptance

1.  Confirm minimal level_02 exists and is visually distinguishable.
2.  Author a level_01 goal with `Next Level = level_02`, Apply/Save as
    appropriate, enter Gameplay and reach it.
3.  Confirm completion occurs once and runtime transitions to level_02;
    level_01 geometry/physics is gone; player starts at level_02 spawn.
4.  Collect an item before transition and confirm Inventory quantity
    survives; consumed inventory remains consumed.
5.  Confirm old checkpoint and level-local transient states do not
    carry; completion is false again.
6.  Restart in level_02 and confirm level_02 restarts, not level_01,
    using existing Restart Inventory semantics.
7.  Test an empty-destination goal and confirm M63 terminal behavior.
8.  With disposable data, target a nonexistent level and confirm safe
    completed-state failure without retry storm/source fallback.
9.  Run Release and confirm staged level_01 → level_02 transition.

Remove disposable level_01 transition fixtures before Git closure unless
current repository design explicitly justifies making one canonical. M64
must not silently turn level_01 into a test fixture.

## Validation

Run relevant current C++ tests for LevelFile/LevelGoal/editor lifecycle,
Application/runtime loading, PhysicsWorld, Checkpoint, Item
Pickup/Inventory, Door, Pressure Plate, Dynamic Box, runtime state and
canonical cleanup.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Add/update narrow level cook/stage tests for level_02 and destination
goals.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check`.

## Documentation

Update compact milestone index/status and current
architecture/tooling/Level Format docs only as post-M60 conventions
require. Document optional destination syntax, staged-only authority,
terminal vs destination goals, fresh-level semantics, Inventory
carry-over and Restart-current-level behavior.

## Canonical Data Safety

M63 intentionally removed legacy `goal -21 3.8 0 2 1.6 1.8`; do not
restore it. Keep absent `dynamic_box 0 5 0 1 1 1 30`.

Prefer canonical `level_01.level` fixture-free at closure.
`level_02.level` is an intentional new canonical content asset and must
remain minimal/deterministic/free of manual-test debris.

## Out of Scope

No campaign graph/order framework/save/progress; savegame; Main Menu;
Level Select; Continue/results UI; transition/loading-screen framework;
arbitrary/named spawn routing; goal requirements; generic SceneManager;
generic Objective/Trigger/Receiver/Event/GameState framework; generic
asset database; GUIDs; multi-document editor; Level Format v2; M65.

## Completion Criteria

Ready for manual acceptance when M63 terminal goals remain valid;
destination-bearing goals safely identify staged levels; level_02
exists/cooks/stages; completion transitions safely between levels;
destination begins fresh except Inventory carries; prior
checkpoint/transients do not carry; Restart restarts current
destination; terminal behavior remains; invalid destinations fail
atomically without retry/source fallback; Development/Release pass;
canonical level_01 has no accidental fixture; and no
campaign/SceneManager/generic event framework, v2 or M65 was introduced.

## STOP

After implementation, validation, documentation and report: **STOP**. Do
not commit, push, merge, start M65, or declare M64 CLOSED. Wait for user
manual acceptance and separate Git closure.
