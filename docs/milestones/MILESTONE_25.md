# Milestone 25 — Static Hazards + Hazard Respawn

## Status

**Complete (manually approved).** Milestone 26 is now the active milestone.

## Branch

`milestone/25-static-hazards`

## Recommended Cursor model

**Grok 4.6 High — Fast OFF**

Use the non-Fast mode for Phase A because this milestone touches the established respawn priority contract, checkpoint progression, CharacterVirtual positioning, world geometry, and Release-visible gameplay markers.

---

## 1. Goal

Add the first explicit non-fall gameplay hazard to the platformer while preserving all M20–M24 respawn/checkpoint/restart semantics.

M25 introduces a **small fixed set of static hazard volumes** placed in the existing M24 greybox route. Touching a hazard causes a respawn at the latest valid checkpoint and increments the death counter exactly like falling below the kill plane.

Conceptual loop:

```text
Traversal
  -> touch static hazard
  -> deathCount +1
  -> respawn at latest checkpoint
  -> continue run
```

This milestone is intentionally **not** a health/combat system and **not** a generic trigger framework.

---

## 2. Why M25

M24 established a longer route with two ordered checkpoints. The next useful gameplay pressure is to make those checkpoints matter for more than falling off the world.

Static hazards exercise the existing systems without introducing enemies, health, combat, animation, audio, or arbitrary level scripting.

M25 should prove that the current run-state architecture can support another respawn cause cleanly.

---

## 3. Scope

M25 adds:

- a project-owned `HazardSpec` or equally small fixed data type;
- a small fixed-size collection of static hazard volumes;
- primitive hazard rendering;
- point/AABB hazard detection using the existing Player visual-center convention unless Phase A proves a better already-established project convention is required;
- a new `RespawnReason::Hazard`;
- hazard death semantics integrated into the existing single-respawn-per-frame contract;
- Debug/Development hazard metrics;
- manual regression validation across M20–M24 behavior.

M25 does **not** add:

- health;
- damage points;
- invulnerability frames;
- knockback;
- enemies;
- combat;
- lives;
- game over;
- moving hazards;
- Jolt sensor bodies;
- a generic trigger/event framework;
- arbitrary runtime hazard registration;
- new assets;
- new dependencies.

---

## 4. Baseline that must remain intact

### Player movement

- max horizontal speed: `6`
- acceleration: `40`
- deceleration: `50`
- jump speed: `8`
- gravity: `20`
- coyote time: `0.10`
- jump buffer: `0.10`

### CharacterVirtual / M23

- Character mass: `70 kg`
- maxStrength: `100 N`
- inner body enabled
- inner body remains private to `PhysicsWorld.cpp`
- cyan box remains Dynamic, `30 kg`, spawn `{0,5,0}`
- Player remains a physical barrier to the cyan box
- Player can push the cyan box
- no persistent wedge regression

### Moving platform / M13

- size `{4,0.4,3}`
- Y `1.3`
- path X `[-6,+6]`
- speed `2.5`
- initial direction `+X`
- carry/reversal semantics unchanged
- ordinary respawn does not reset it
- Enter restart resets it

### Slopes

- max slope `50°`
- 30° slope remains walkable and optional
- 60° slope remains steep and optional

### M24 world

- ground center `{0,-0.25,0}`, size `{56,0.5,8}`
- CP1 platform center `{16.5,0.75,0}`
- CP2 platform center `{-15.5,1.75,0}`
- goal platform center `{-21,2.75,0}`
- 30° slope center `{21.70,1.6732,0}`
- 60° slope center `{25.60,0.966,0}`
- exactly two ordered checkpoints
- CP1 before CP2
- no downgrade after CP2
- goal center `{-21,3.8,0}`
- kill plane `Y=-8`

---

## 5. Hazard representation

Prefer a minimal project-owned world type such as:

```cpp
struct HazardSpec
{
    core::Vec3 center;
    core::Vec3 size;
};
```

Use a fixed-size collection, preferably `std::array<HazardSpec, N>` with a deliberately small `N` selected in Phase A.

The target is **2–3 hazards**, not an arbitrary system.

Do not use:

- `std::vector` for runtime registration;
- heap allocation;
- Jolt IDs as gameplay identity;
- polymorphic hazard classes;
- a `HazardManager`;
- a `TriggerManager`.

---

## 6. Hazard placement design

Phase A must inspect the final M24 route before choosing coordinates.

Hazards should:

- make checkpoints useful;
- be visually readable;
- be avoidable with normal movement;
- not require frame-perfect jumps;
- not overlap checkpoint respawn volumes;
- not overlap the goal trigger;
- not occupy the moving-platform swept volume in a way that creates unavoidable deaths;
- not create a new Player/cyan-box/moving-platform compression trap;
- not make the cyan box a required puzzle object;
- not turn the optional slope tests into mandatory traversal.

Preferred distribution:

- one hazard after CP1 but before CP2;
- one hazard after CP2 or in the final approach to the goal;
- optionally one early hazard only if the route remains readable and fair.

Exact coordinates are recorded in the Phase A design section below. Do not treat this definition's earlier "not approved" note as current; Phase A chose the live volumes in `world/HazardWorld.h`.

---

## 7. Detection

Prefer the same simple project-owned overlap style already used by checkpoint/goal logic.

Phase A must inspect whether the existing code uses Player visual-center point/AABB checks and determine the smallest consistent implementation.

No Jolt sensor bodies.

No collision-response hazard body is required.

The hazard visual may be a primitive while the gameplay volume remains project-owned world data.

---

## 8. RespawnReason

Extend the existing reason enum with:

```text
Hazard
```

Final conceptual reasons:

```text
None
Fall
Manual
Hazard
```

Do not add Restart as a respawn reason unless the existing architecture already deliberately models it that way. M22 restart remains a distinct fresh-run action.

---

## 9. Death-count semantics

Hazard contact is a death event.

Therefore:

```text
Hazard -> deathCount +1
Fall   -> deathCount +1
Manual R -> deathCount unchanged
```

A hazard death must respawn at:

- initial spawn if no checkpoint is active;
- CP1 if CP1 is current;
- CP2 if CP2 is current.

Checkpoint progress is preserved.

Level completion is preserved if a hazard is touched after completion, matching the existing Fall behavior.

---

## 10. Same-frame priority

M25 must preserve the one-reset-per-frame rule.

Phase A must inspect and explicitly propose the final priority among:

- Fall
- Manual R
- Hazard
- checkpoint activation
- goal completion
- Enter restart

The existing established rule already gives Fall priority over Manual and keeps checkpoint/goal evaluation out of respawn frames.

M25 must not guess a hazard priority casually. The preferred policy is that **automatic lethal causes are resolved before Manual R**, but Phase A must verify the current control flow and propose one deterministic order.

Whatever is chosen must guarantee:

- at most one teleport/reset per frame;
- deathCount increments at most once per frame;
- a hazard respawn frame cannot activate a checkpoint;
- a hazard respawn frame cannot newly complete the goal;
- Enter restart still obeys M22's `restartAvailableAtFrameStart` contract.

---

## 11. Hazard contact after completion

If the level is already complete and the Player touches a hazard:

- `deathCount += 1`;
- respawn at latest checkpoint;
- `completed` remains `true`;
- `LEVEL COMPLETE` remains visible;
- `PRESS ENTER TO RESTART` remains visible.

Only Enter performs the fresh-run reset.

---

## 12. Enter restart

M22 restart must reset hazard-related transient state automatically.

Hazards themselves are static world specs, so they do not need physical reset.

After Enter:

- Player returns to initial spawn;
- checkpoint progression clears;
- deathCount returns to `0`;
- last reason returns to `None`;
- completion clears;
- moving platform resets;
- cyan box resets;
- camera snaps;
- hazard world remains in the same canonical positions.

---

## 13. Rendering

Hazards are visible gameplay geometry in all configurations.

Use raylib primitives through the existing Renderer boundary.

Preferred visual language:

- clearly dangerous shape/color;
- simple repeated teeth/spikes/bars or low boxes;
- no textures/models required;
- no animation required.

Do not add new authored assets.

Renderer only consumes hazard specs and draws them. It does not decide whether the Player dies.

---

## 14. Physics boundary

Hazard gameplay meaning belongs outside `PhysicsWorld`.

Do not add Jolt hazard sensors.

If hazard visuals are intentionally non-solid, they do not need Jolt bodies.

If Phase A finds a compelling reason for solid supporting geometry around a hazard, that supporting geometry must remain ordinary canonical static world geometry; the lethal volume still stays project-owned gameplay data.

---

## 15. Debug metrics

Debug/Development should show a small hazard section, for example:

- `Inside hazard: None / 1 / 2 / ...`
- `Hazard contact this frame`
- existing `Last respawn reason` must display `Hazard`

Preserve M23/M24 metrics.

Release remains free of ImGui.

Do not expose Jolt BodyIDs.

---

## 16. Architecture

Preserve:

```text
world/*
    fixed hazard specifications

Application
    hazard detection and run-state meaning

Player
    movement policy

PhysicsWorld
    physical simulation only

Renderer
    hazard visualization only
```

Do not introduce a generic hazard subsystem unless a concrete requirement appears later.

---

## 17. Phase plan

### Phase A — Hazard placement + respawn-priority design

Complete. Specs live in `world/HazardWorld.h`. Coordinates and Fall > Hazard > Manual policy approved.

Inspected live M24 geometry, Player envelope, and Application reset order. Scaffolding is in `world/HazardWorld.h` and `RespawnReason::Hazard`. Detection/rendering/death are **not** wired.

#### Chosen count: exactly 2

Hazard 1 makes CP1 useful (death on the CP1 ↔ center corridor). Hazard 2 makes CP2 useful (missed CP2 → goal jump). A third early hazard would crowd spawn, the moving-platform sweep, and the cyan-box region without adding checkpoint value.

#### Hazard 1 — corridor spikes (index 0)

- center `{11.5, 0.5, 0}`, size `{1.4, 1.0, 2.0}`
- AABB X [10.8, 12.2], Y [0, 1.0], Z [-1, 1]
- On ground between CP1 platform left (14.5) and moving-platform swept max X (8)
- Jump over on the way to CP1 and on the return. Width 1.4 at speed 6 ≈ 0.23 s; airborne center Y ≈ 2.12 vs lethal top 1.0 (margin ≈ 1.1). Rise 1.6 / air 4.8 m.

#### Hazard 2 — goal-gap spikes (index 1)

- center `{-18.5, 0.5, 0}`, size `{1.2, 1.0, 2.0}`
- AABB X [-19.1, -17.9], Y [0, 1.0], Z [-1, 1]
- In the 2.0 m ground gap between CP2 left (−17.5) and goal-platform right (−19.5)
- Successful CP2 → goal jump stays airborne above the bar. Landing in the gap is a Hazard death and returns to CP2.

#### Proposed Phase B priority

```text
Fall > Hazard > Manual R > checkpoint / goal > Enter (M22 restartAvailableAtFrameStart)
```

Fall stays first because that is the live first branch and ground hazards cannot also be below Y = −8. Multiple overlapping hazard volumes still produce one Hazard death. Hazard frames skip checkpoint/goal (already in the no-respawn else). Hazard wins over Enter that frame.

Compile-time invariants: respawn visual AABBs, checkpoint triggers, goal trigger, and moving-platform swept AABB do not overlap either hazard.

Build all configurations and stop for review. Do not mark M25 complete. Do not start Milestone 26.

### Phase B — Implementation

Complete. Live detection, Fall > Hazard > Manual priority, primitive visuals, and Debug/Development hazard metrics.

### Phase C — Manual validation

**Phase C:** Complete and manually approved. Hazard death +1, latest-checkpoint respawn (spawn / CP1 / CP2), and Enter restart deathCount 0 are accepted. Milestone 26 is now the active milestone.

---

## 18. Manual acceptance targets

M25 is complete only after manual validation confirms:

1. Fresh run can avoid hazards using normal movement.
2. Hazard before any checkpoint respawns at initial spawn and adds exactly one death.
3. Hazard after CP1 respawns at CP1 and adds exactly one death.
4. Hazard after CP2 respawns at CP2 and adds exactly one death.
5. Hazard contact does not downgrade checkpoint progress.
6. Hazard respawn does not reset the moving platform.
7. Hazard respawn does not reset the cyan box.
8. Manual R still does not add a death.
9. Fall still adds exactly one death.
10. Hazard after completion preserves completion UI/state.
11. Enter restart clears run state and death count but leaves static hazards in their canonical positions.
12. Second run behaves identically.
13. Cyan box remains pushable/solid without persistent wedge.
14. Moving-platform carry/reversal remains correct.
15. 30°/60° slope behavior remains correct.
16. Camera remains usable across the M24 world.
17. Release contains gameplay hazard visuals but no ImGui.

---

## 19. Explicitly out of scope

Do not implement in M25:

- Milestone 26;
- health points;
- damage amounts;
- invulnerability;
- knockback;
- lives/game over;
- enemies;
- combat;
- projectiles;
- moving hazards;
- animated hazards;
- hazard audio;
- particles;
- collectibles;
- score;
- timer;
- save/load;
- serialization;
- generic trigger system;
- generic hazard manager;
- LevelManager;
- SceneManager;
- ECS/event bus;
- JSON level format;
- new assets;
- new dependencies;
- cooker changes;
- camera redesign;
- Player movement tuning;
- CharacterVirtual tuning.

---

## 20. Completion definition

M25 is complete when:

- a small fixed set of static hazards exists in the M24 route;
- hazards are visible and avoidable;
- touching one produces exactly one hazard death/respawn;
- latest-checkpoint semantics are correct;
- `RespawnReason::Hazard` is visible in Debug/Development;
- same-frame priority is deterministic;
- checkpoint/goal/restart contracts remain intact;
- M23 dynamic-body behavior remains intact;
- all three builds pass;
- Release remains free of ImGui;
- user manually validates the gameplay;
- Git milestone workflow is completed;
- Milestone 26 has not started prematurely.
