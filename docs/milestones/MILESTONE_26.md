# Milestone 26 — Collectibles + Run Counter

## Status

**Complete (manually approved).** Do not reopen M26. Milestone 27 is the active milestone.

## Goal
Add the first non-lethal collectible gameplay loop to the existing platformer level while preserving all M00–M25 behavior.

The level will contain exactly **3 fixed collectibles**. Collecting one removes it for the current run and increments an Application-owned run counter. Collectibles are restored only by the existing full **Enter restart** after level completion; ordinary Manual/Fall/Hazard respawns do not restore them.

This milestone intentionally remains a small hardcoded gameplay feature. It does **not** introduce score, inventory, save/load, generic item systems, ECS, level serialization, audio, particles, or new assets.

## Player-facing loop

```text
Traverse level
    ↓
Touch collectible
    ↓
Collectible disappears
    ↓
Collected: N / 3
    ↓
Respawn from R / Fall / Hazard
    ↓
Already collected items remain collected
    ↓
Complete level
    ↓
Enter restart
    ↓
Fresh run: 0 / 3 and all collectibles restored
```

## Architectural intent

- `world` owns immutable fixed collectible specifications.
- `Application` owns per-run collected state and detection semantics.
- `Renderer` draws only collectibles that are still available.
- `Player` remains movement-only.
- `PhysicsWorld` remains physics-only.
- Collectibles use project-owned point-vs-AABB gameplay detection, not Jolt sensors.
- Exactly 3 collectibles; use a fixed-size representation such as `std::array` / bitset-like value state.
- No `CollectibleManager`, `ItemManager`, generic trigger abstraction, event bus, or ECS.

## Phase A — Inventory, placement design, and scaffolding

Phase A must inspect the final M25 world before choosing live coordinates.

Deliverables:

1. Confirm final M25 geometry, checkpoints, hazards, goal, moving-platform sweep, slopes, Player dimensions, camera behavior, and update order.
2. Propose a minimal `CollectibleSpec` and exactly 3 compile-time collectible specs.
3. Choose exact positions only after proving they are reachable, readable, and do not overlap respawn/checkpoint/goal/hazard volumes.
4. At least one collectible should reward the right-side/CP1 portion of the route.
5. At least one collectible should reward the moving-platform/central traversal.
6. At least one collectible should reward the left-side/CP2-to-goal portion of the route.
7. Collection must be optional: the goal remains completable even if the player has collected 0/3.
8. Design Application-owned run state, preferably fixed-size booleans/bitset plus a derived or maintained count.
9. Define same-frame semantics for collectible + Fall/Hazard/Manual/checkpoint/goal/Enter.
10. No live collection behavior in Phase A unless tiny scaffolding is required for compilation.

### Phase A design record

Exactly three static hop collectibles (`kCollectibleSize = {1.0, 1.2, 1.0}`). Hover = support top + 1.5 so standing center stays ~0.1 below the AABB.

- Collectible 1: `{5.0, 2.5, 0}` on the right platform (optional vs ground → CP1).
- Collectible 2: `{-4.5, 4.0, 0}` on the left landing (reached via moving platform; hop optional).
- Collectible 3: `{-10.0, 3.75, 0}` on the middle-left step (intended route; hop optional).

`CollectibleRunState.collected` is the source of truth; `CollectedCount` is derived. Proposed Phase B: collect in the no-respawn branch after checkpoint/goal, skip collection when `restartAvailableAtFrameStart && Enter`, then existing RestartRun. Fall/Hazard/Manual still win over collection. One uncollected match per frame.

Scaffolding: `world/CollectibleWorld.h`, `gameplay/CollectibleRunState.h`. Do not mark M26 complete. Do not start Milestone 27.

## Approved semantic direction to validate in Phase A

Collection should be evaluated only on a frame in which no respawn occurred. Therefore:

```text
Fall / Hazard / Manual respawn
    > collectible collection
```

Within a normal non-respawn frame, collectible collection should occur before or alongside checkpoint/goal evaluation with deterministic semantics. Enter restart must clear all collected state and restore all collectibles.

A collectible can be collected at most once per run. Multiple collectibles touched in one frame may each be collected if the geometry genuinely permits it, but the final design should spatially separate them so this is not a normal gameplay case.

## Phase B — Implementation (current)

Live:

- `FindAvailableCollectibleIndexContaining(Player::Position(), collectibleRunState)`;
- collection in the no-respawn branch after checkpoint/goal;
- skip collection when `restartAvailableAtFrameStart && Enter`;
- `RestartRun` clears `collectibleRunState`;
- Renderer gold 0.45 cubes for available items;
- `COLLECTED N / 3` upper-right in all configurations;
- Debug/Development Collectibles metrics.

Phase B implementation is complete. Phase C was manually approved.

## Phase C — Manual validation

Final validation must cover at least:

- Collect each of the 3 individually.
- Counter progresses 0/3 → 1/3 → 2/3 → 3/3.
- Collected item disappears and cannot increment twice.
- R after collection preserves it.
- Fall after collection preserves it.
- Hazard death after collection preserves it.
- CP1/CP2 progression remains correct.
- Goal can complete with fewer than 3 collectibles.
- Goal can complete with 3/3.
- Completion UI and collectible counter coexist correctly.
- Enter after completion restores all 3 and resets counter to 0/3.
- Second run can collect all 3 again.
- Moving platform, cyan dynamic box, slopes, hazards, camera, and Release behavior regressions pass.

## Completion criteria

M26 is complete only when:

- Exactly 3 fixed collectibles exist.
- All 3 are reachable through normal validated movement.
- Collection is non-lethal and one-time per run.
- Counter is correct and Release-visible.
- Ordinary respawns preserve collection progress.
- Enter fresh-run restart resets collection progress and restores visuals.
- Goal completion does not require collectibles.
- No new dependency or asset is introduced.
- No generic item/trigger/level system is introduced.
- Debug, Development, and Release build successfully.
- User manually approves Phase C.

## Explicitly out of scope

- Milestone 27
- score/points
- lives/game over
- health/damage changes
- inventory
- currency/economy
- collectible effects/power-ups
- required collectible gate for goal
- save/load/persistence
- achievements
- audio
- particles
- animation system
- new textures/models
- moving collectibles
- procedural placement
- generic collectible/item manager
- generic trigger framework
- ECS/event bus
- JSON level data
- camera redesign
- Player tuning
- physics redesign
- asset cooker changes
- new dependencies

## Git branch

`milestone/26-collectibles`

## Recommended Cursor model

Grok 4.6 High — Fast OFF for Phase A architecture/placement analysis.
