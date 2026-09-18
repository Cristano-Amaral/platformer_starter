# Milestone 69 — Player Health & Damage

## Status

**CLOSED.** Milestone 70 is the active milestone.

## Goal

Introduce a narrow runtime **Player Health** authority and make the existing **Hazard** gameplay objects cause deterministic, controlled damage to the player, with a small Release-capable Health HUD.

M69 establishes health and damage only. **Player death, death presentation, respawn, checkpoint restoration after death, game-over behavior, and related lifecycle belong to M70 and must not be implemented here.**

The implementation must fit the existing Application-owned gameplay lifecycle and must not introduce a generic attributes/stats/combat framework.

## Branch

`milestone/69-player-health-damage`

## Cursor Model

**Grok 4.6 High — Fast OFF**

## Existing Authorities to Preserve

M69 must reuse and preserve the architecture established through M68, including:

- `gameplay::TopLevelFlow` and the existing Main Menu / Gameplay / Run Complete flow;
- M68 Application-owned Pause state and the existing `simulationPaused` authority;
- `currentRuntimeLevelId`;
- existing staged-only runtime level loading and atomic transition behavior;
- Inventory and Item Pickup lifecycle;
- Checkpoint lifecycle already present in the repository;
- Level Goal / destination transition / Run Complete behavior;
- existing authored `Hazard` representation and editor behavior;
- M67 Gameplay Objective HUD and existing HUD layout conventions;
- Development editor / F2 authority;
- authored `workingCopy` / Apply / Save / Dirty semantics.

Repository code and current tests are the source of truth for exact existing Hazard and Checkpoint semantics.

## Player Health Runtime Authority

Add a narrow Application-owned runtime health state, or an equivalently narrow gameplay value type owned by the Application.

The state must represent at minimum:

- current health;
- maximum health.

Use a fixed game-owned maximum health for M69. Do not add authored per-level/player max-health data merely to make the value configurable.

A reasonable canonical presentation is **100 maximum health**, unless repository conventions discovered during implementation strongly justify another fixed value. Tests must use the same single authority rather than duplicating the constant.

Required invariants:

- `maxHealth > 0`;
- `0 <= currentHealth <= maxHealth`;
- damage cannot increase health;
- damage clamps at zero;
- no negative health;
- no healing system is required in M69.

Do not introduce `AttributeSystem`, `StatsComponent`, `DamageSystem`, `DamageType`, armor, resistance, status effects, teams/factions, combat components, or a generic event framework.

## Hazard Damage

Reuse the existing authored Hazard objects. Do not create a second hazard representation.

When the active gameplay player overlaps an active Hazard, the Hazard must cause health damage using a deterministic rule that cannot accidentally drain the entire health pool every render frame.

Prefer a narrow game-owned damage cadence/cooldown or equivalent deterministic contact rule based on the existing gameplay update architecture. The implementation must make sustained overlap understandable and testable.

For M69:

- use a fixed game-owned Hazard damage amount;
- do not add damage amount/cooldown fields to Level Format v1;
- do not require semantic edits to canonical Level files;
- do not create per-Hazard damage types or configurable combat data;
- damage must only occur during active, unpaused Gameplay simulation;
- leaving and re-entering a Hazard must continue to behave deterministically under the chosen rule.

The exact fixed damage amount and cadence should be named constants with focused tests and documented in the implementation report.

## Zero Health Boundary

M69 must allow Health to reach zero, but **must not implement the M70 death/respawn flow**.

At zero health:

- health remains clamped at zero;
- additional Hazard contact cannot underflow it;
- do not automatically reload the level;
- do not teleport/respawn the player;
- do not restore a checkpoint;
- do not show a Death/Game Over screen;
- do not create a new top-level state;
- do not reset Inventory, pickups, doors, boxes, timer, goals, or other run-local state merely because Health reached zero.

Keep existing gameplay behavior otherwise intact unless a minimal safety guard is necessary to prevent repeated damage bookkeeping at zero.

M70 will define the meaning of death and respawn explicitly.

## Health Lifecycle

Health lifecycle must be explicit and covered by tests.

### Fresh Play / New Run / Play Again

Starting a fresh run through the existing M66 authority initializes Health to maximum.

This includes Main Menu → PLAY and Run Complete → PLAY AGAIN.

### Level-to-Level Transition

A successful destination transition (for example `level_01` → `level_02`) **preserves current Health**, just as it is part of the same run.

Do not reset Health merely because `currentRuntimeLevelId` changes.

A failed transition is atomic and must leave current Health unchanged.

### Restart Current Level

Existing Restart semantics start the current level again. M69 should initialize Health to maximum on Restart unless current repository lifecycle semantics provide a stronger established reset authority that requires the same result through another path.

Restart must continue to preserve/reset other systems exactly according to their existing contracts; M69 must not redefine them.

### Checkpoints

M69 does **not** add health snapshots to checkpoints and does not implement health restoration through death/respawn.

Existing checkpoint activation behavior must remain unchanged. M70 will define how Health interacts with respawn/checkpoint restoration.

Do not opportunistically extend checkpoint serialized/runtime data for M69.

### Apply / Reload / Development Open or Switch

When an authored Level is explicitly Apply/Reloaded or Development Open/Switch replaces the active runtime according to existing editor lifecycle authority, initialize Health to maximum for the newly established gameplay runtime unless the current code's authoritative lifecycle requires an equivalent fresh-runtime reset path.

Do not preserve stale damaged Health across an explicit authored runtime replacement.

This reset is runtime-only and must not mutate authored data or Dirty.

### Physics Rebuild

A physics-only rebuild that is explicitly intended to preserve the active gameplay run must not independently reset Health. Follow the existing rebuild preservation contract rather than treating every rebuild as a new run.

## Pause, Inventory, Menus and Completion

Health damage must obey the existing gameplay pause/flow authorities.

While M68 Pause is active:

- no Hazard damage;
- no health cadence/cooldown progression if that progression uses simulation time;
- Health HUD follows the Pause visibility rules below.

While Inventory blocks gameplay:

- no Hazard damage;
- gameplay remains consistent with the existing Inventory pause semantics.

While Main Menu is active:

- no Hazard damage;
- no Health HUD.

During destination `LEVEL COMPLETE` hold:

- do not allow Hazard damage to alter the completed source-level state while transition presentation has authority.

During `RUN COMPLETE`:

- no Hazard damage;
- no Health HUD.

Development F2/editor behavior must remain coherent with existing M68 rules; editor-paused gameplay must not take Hazard damage.

## Health HUD

Add a small Release-capable raylib Health presentation integrated with the existing Gameplay HUD.

A narrow textual presentation is sufficient, for example:

`HEALTH 100 / 100`

Exact punctuation/spacing may follow current renderer conventions.

Requirements:

- derived directly from the runtime Health authority;
- updates immediately after successful damage;
- no authored display text;
- no per-frame file I/O;
- no ImGui dependency in Release;
- positioned so it does not collide with TIME/BEST, M67 LEVEL/OBJECTIVE, COLLECTED, M62 pickup notifications, interaction prompts, completion overlays, Pause, Run Complete, or Main Menu.

Visibility:

- visible during active Gameplay;
- hidden while Inventory is open if existing gameplay HUD convention suppresses comparable HUD there; follow the current HUD authority consistently;
- hidden during M68 Pause so Pause retains presentation priority;
- hidden during destination `LEVEL COMPLETE` hold;
- hidden during `RUN COMPLETE`;
- hidden in Main Menu;
- hidden while F2/editor suppresses gameplay HUD according to existing conventions.

Do not add health bars, animated damage overlays, screen vignette, floating damage numbers, generic widget frameworks, or HUD layout systems in M69.

## Damage Feedback

M69 requires Health value change and HUD feedback only.

A new sound, camera shake, red flash, particles, invulnerability animation, controller rumble, or other damage feedback is optional only if an existing narrow facility makes it trivial and it does not expand scope. Prefer omitting such polish and leaving presentation refinement for a later milestone.

Do not create a generic feedback/effects framework.

## Authored / Dirty Safety

M69 must not require semantic changes to canonical Level data.

Pause/Health/damage runtime state must not mutate or serialize into:

- `LevelDefinition`;
- editor `workingCopy`;
- Hazard authored specs unless an existing field is already authoritative and unchanged;
- Checkpoint specs;
- Level Goal specs;
- Item Pickup / Door / Pressure Plate / Dynamic Box specs;
- editor layout;
- Level files.

Health changes during gameplay must never set editor Dirty.

`level_01.level` and `level_02.level` should have no semantic changes for M69.

## Release / Performance

M69 must work in Development and Release.

Release must not depend on:

- ImGui;
- source-authored assets;
- editor catalogs;
- source fallback.

No per-frame filesystem I/O, source scans, catalog refreshes, heap-heavy generic combat infrastructure, or unnecessary runtime allocations should be introduced.

Use existing loaded Level/Hazard data and existing player collision/overlap authority.

## Out of Scope

Do **not** implement:

- player death flow;
- respawn;
- checkpoint restoration after death;
- death animation;
- death sound/overlay;
- Game Over screen;
- lives;
- healing items;
- health pickups;
- regeneration;
- max-health upgrades;
- armor/shields;
- damage types;
- resistances;
- status effects;
- enemies/combat AI;
- weapons;
- authored Hazard damage values;
- authored player stats;
- generic Attributes/Stats/Combat/Damage framework;
- generic event/message bus;
- Settings/Options;
- save/progress persistence;
- Level Select;
- generic GameState/state stack;
- SceneManager;
- generic HUD/UI framework;
- GUIDs;
- Level Format v2;
- M70 functionality.

## Automated Validation

Inspect current test targets first and extend the narrowest relevant tests.

Add focused M69 regression coverage for at least:

1. Health initializes to maximum on fresh Play/New Run.
2. Hazard overlap causes the fixed damage amount.
3. Sustained overlap follows the deterministic damage cadence and does not damage once per render frame accidentally.
4. Health clamps at zero and never underflows.
5. Zero Health does not trigger death/respawn/reload in M69.
6. Leaving/re-entering Hazard follows the documented cadence rule.
7. Pause prevents Hazard damage and cadence progression according to simulation-time semantics.
8. Inventory/editor simulation blocking prevents Hazard damage.
9. Main Menu / Run Complete / destination completion hold do not apply Hazard damage.
10. Successful level transition preserves Health.
11. Failed transition preserves Health atomically.
12. Restart Current Level restores maximum Health.
13. Play Again restores maximum Health.
14. Main Menu → Play starts maximum Health.
15. Apply/Reload/Development Open/Switch use the documented fresh-runtime Health reset semantics without Dirty mutation.
16. Physics rebuild preservation paths do not accidentally reset Health.
17. Health HUD shows the authoritative values and obeys visibility rules.
18. Health/damage never mutates authored Level data or editor Dirty.

Run the relevant existing C++ regression tests discovered in the repository, especially those covering:

- GameFlow / TopLevelFlow / Pause;
- GameplayObjectiveHud / renderer HUD behavior;
- Hazard/player overlap;
- LevelTransition / LevelGoal;
- Inventory / InventoryUi;
- Checkpoint;
- Item Pickup;
- authored lifecycle / EditorWorkspace;
- PhysicsRebuild;
- canonical scene/data safety.

Run the existing Python validation suite relevant to the current project, including:

```text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Build all supported Windows configurations:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:

```text
git diff --check
```

Verify explicitly that canonical `level_01.level` and `level_02.level` have no unintended semantic changes.

## Manual Acceptance

After Cursor reports green automated validation, manual acceptance must verify at minimum:

1. Main Menu has no Health HUD and no gameplay damage.
2. PLAY starts `level_01` at full Health.
3. Entering an existing Hazard visibly reduces Health by the documented fixed amount/cadence.
4. Remaining in the Hazard does not drain Health every render frame.
5. Leaving/re-entering produces deterministic behavior.
6. Pause while in/near a Hazard freezes Health; Resume continues correctly without catch-up damage.
7. Inventory/editor blocking does not allow hidden Hazard damage.
8. `level_01` → `level_02` preserves the damaged Health value.
9. Restart `level_02` restores full Health.
10. Complete the run: Run Complete hides Health; Play Again starts `level_01` at full Health.
11. Pause → Main Menu hides Health; subsequent PLAY starts full Health.
12. If practical, drive Health to zero and confirm it stays at zero without death/respawn/reload; this intentionally demonstrates the M69/M70 boundary.
13. Development F2 round-trip remains coherent.
14. Release essential flow works without editor/source dependencies.
15. Existing HUD, prompts, M62 notifications, Level Complete, Pause and Run Complete remain readable and authoritative.

## Documentation

Follow the post-M60 milestone documentation convention:

- canonical active document: `docs/milestones/MILESTONE_69.md`;
- keep `docs/MILESTONES.md` compact;
- update architecture/README/AGENTS only where current repository conventions require it;
- preserve M68 as CLOSED;
- after implementation, M69 is **implemented, awaiting manual acceptance**, not CLOSED.

## Cursor Report Requirements

After implementation and validation, Cursor must report:

1. files changed;
2. exact Health authority and constants;
3. exact Hazard damage amount and cadence;
4. zero-health behavior and confirmation that M70 was not implemented;
5. Health lifecycle for Play, Restart, transition, Play Again, Apply/Reload/Open/Switch and rebuild;
6. Pause/Inventory/editor/completion interaction;
7. HUD presentation and visibility;
8. authored/Dirty safety;
9. Development/Release behavior;
10. C++ test results;
11. Python test results;
12. Debug/Development/Release build results;
13. `git diff --check` result;
14. canonical Level-data status;
15. explicit confirmation that no out-of-scope generic framework or M70 functionality was added.

## STOP Gate

After implementation/report:

- **STOP**;
- do not commit;
- do not push;
- do not merge;
- do not start M70;
- do not mark M69 CLOSED.

Wait for manual acceptance and the separate Git closure workflow.
