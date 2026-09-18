# Milestone 70 --- Death, Damage Feedback & Respawn

**Status:** Implemented, awaiting manual acceptance.\
**Branch:** `milestone/70-death-damage-feedback-respawn`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Complete the Health loop introduced in M69 with a narrow Release-capable
damage vignette, an Application-owned death phase when Health reaches
zero, and respawn at the existing active Checkpoint or current-Level
player spawn.

M70 must preserve the current run rather than treating death as Restart
Level. It must reuse existing Health, Hazard, Checkpoint, spawn, Game
Flow, simulation-pause, HUD, editor, and staged-runtime authorities.

## Damage vignette

Trigger a brief procedural red damage vignette only when Hazard contact
actually reduces Player Health. Use fixed named constants for
duration/intensity and existing raylib drawing primitives. It must fade
cleanly, refresh deterministically on later successful damage, pause
coherently with gameplay blockers, and never require source assets.

Do not add a generic post-processing, shader, screen-effect,
notification, or overlay framework.

## Health zero and death

M69's `Health == 0` behavior changes in M70. When an allowed damage
event transitions Health from greater than zero to zero:

1.  detect the transition exactly once;
2.  enter a narrow Application-owned death/respawn phase;
3.  block ordinary gameplay;
4.  show brief Release-capable death feedback such as `YOU DIED`;
5.  after a short fixed named delay, respawn;
6.  restore Health to `gameplay::kMaxPlayerHealth`;
7.  continue the same Level and run.

Do not repeatedly enter death while Health remains zero. A narrow
`PlayerDeathState`-style value is acceptable if needed; a generic
GameState/state stack is not.

## Respawn destination

Reuse the existing Checkpoint/player-spawn authority:

-   active Checkpoint in the current Level → respawn there;
-   otherwise → respawn at the current Level's normal player spawn.

Do not add Health snapshots to Checkpoints, checkpoint IDs, authored
checkpoint Health, cross-Level checkpoints, save persistence, or
checkpoint-selection UI.

## Death respawn lifecycle

Death respawn is not Restart Current Level and must not reload the
Level.

After death respawn:

-   Health resets to maximum;
-   `currentRuntimeLevelId` is preserved;
-   run timer is preserved;
-   Inventory is preserved;
-   collected Item Pickups are preserved;
-   unlocked/consumed Door state is preserved;
-   Level Goal/completion state is preserved;
-   M61/M62 already-fired collection feedback is not replayed;
-   Pressure Plate and Dynamic Box runtime state should remain governed
    by their existing runtime/physics authority rather than being
    globally reset.

If the existing player-respawn helper performs broader resets, refactor
only as narrowly as necessary. Do not use Restart Level as a shortcut.

Clear/reset M69 Hazard-contact cadence on death respawn so stale contact
cannot cause catch-up damage or an immediate repeated death. If a
spawn/checkpoint overlaps a Hazard, implement and test a narrow
deterministic safety rule without modifying canonical Level data.

## Blocking and game flow

During death/respawn:

-   block movement, physics/gameplay progression, E interactions,
    Grab/Drop, pickups, Door interaction, Hazard damage, Level Goal
    progression, Inventory and Pause;
-   hide ordinary interaction prompts;
-   prevent input carry-through on the respawn frame.

Death must not compete with Main Menu, destination `LEVEL COMPLETE`, or
`RUN COMPLETE`. Existing higher-level completion authorities remain
authoritative.

A lethal hit can only begin from damage-allowed active Gameplay.

For F2/editor, inspect the existing M68 authority and implement the
narrowest coherent behavior. The death lifecycle must not advance
invisibly, duplicate respawn, or require a state stack.

## Existing fall/manual respawn

Inspect the existing Fall/Manual R respawn contracts. The new mandatory
rule is specifically:

**Health-zero death restores maximum Health and respawns at the active
Checkpoint/current-Level spawn.**

Do not opportunistically redefine fall/manual respawn unless narrow
unification is required for correctness and protected by regressions.

## Existing M69 lifecycle to preserve

-   Play/New Run → maximum Health.
-   Play Again → maximum Health.
-   Main Menu → Play → maximum Health.
-   successful `level_01 -> level_02` transition → preserve Health.
-   failed transition → Health unchanged atomically.
-   Restart Current Level → maximum Health.
-   Apply/Reload and Development Open/Switch → maximum Health for the
    newly established runtime.
-   physics-only rebuild preserving the run → do not independently reset
    Health.

Death must not trigger Level transition or Run Complete.

## HUD and presentation

Preserve the M69 Health HUD authority. During death, death feedback has
visual priority; ordinary prompts must be hidden and objective/Health
presentation must not compete with it. After respawn, Health HUD must
immediately show maximum Health.

The Damage Vignette should be noticeable but not obscure gameplay. No
new damage sound, camera shake, particles, floating damage numbers,
rumble, generic effect framework, or authored presentation data in M70.

## Authored / Dirty safety

Death, vignette, and respawn are runtime-only and must never mutate or
serialize into `LevelDefinition`, editor `workingCopy`,
Hazard/Checkpoint/Goal/Pickup/Door/Plate/Box specs, editor layout, or
Level files. They must never mark Dirty.

M70 requires **no semantic changes** to:

-   `game/assets/source/levels/level_01.level`
-   `game/assets/source/levels/level_02.level`

Do not add temporary canonical fixtures.

## Release / performance

All player-facing behavior must work in Development and Release. Release
must not depend on ImGui, authoring source tree, editor catalogs, or
source fallback. No per-frame filesystem I/O or source scans. Prefer a
cheap procedural vignette and already-loaded runtime data.

## Out of scope

Do not implement Game Over, lives, Continue/retry menu, healing, Health
pickups, regeneration, max-Health upgrades, armor/shields, Damage Types,
resistances, status effects, enemies/combat AI, weapons, authored Hazard
damage, authored Player stats, generic Attribute/Stats/Combat/Damage
framework, generic post-processing/shader/effect framework, generic
event bus, checkpoint persistence, save/load, campaign progress, Level
Select, Settings, generic GameState/state stack, SceneManager, generic
HUD/UI framework, GUIDs, Level Format v2, or M71 functionality.

## Automated validation

Inspect current C++ test targets first and extend the narrowest relevant
tests. Cover at minimum:

-   successful non-lethal damage triggers vignette; blocked/no-op damage
    does not;
-   vignette timing/fade is deterministic and does not catch up
    incorrectly through Pause/Inventory/editor blocking;
-   lethal damage enters death exactly once;
-   Health remains clamped at zero during death;
-   gameplay/interactions/Pause/Inventory are blocked during death;
-   deterministic death delay;
-   active Checkpoint respawn and no-Checkpoint player-spawn fallback;
-   death respawn restores maximum Health;
-   death preserves current Level, timer, Inventory, collected pickups,
    and the defined Door/Plate/Box/run-local state;
-   Hazard contact is safely reset with no stale/catch-up damage or
    repeated death loop;
-   successful Level transition still preserves Health;
-   Restart/Play/New Run/Play Again retain M69 semantics;
-   completion/results/Main Menu receive no new Hazard/death activity;
-   authored/workingCopy/Dirty remain unchanged;
-   Release has no editor/source dependency.

Run relevant current C++ regressions covering PlayerHealth,
GameFlow/Pause, Hazard overlap, Checkpoint/player respawn,
GameplayObjectiveHud/Health HUD, LevelTransition, LevelGoal,
Inventory/InventoryUi, Item Pickup/M61/M62, Door/Pressure Plate, Dynamic
Box/Grab, PhysicsRebuild, authored lifecycle, EditorWorkspace,
LevelFile/canonical safety, plus directly affected targets discovered in
the repository.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Then run `git diff --check` and verify explicitly that canonical
`level_01.level` and `level_02.level` have no unintended semantic
changes.

## Manual acceptance

Manual acceptance must verify:

1.  fresh run starts at maximum Health;
2.  each successful Hazard damage tick gives immediate visible vignette
    feedback;
3.  vignette is noticeable but Gameplay remains readable;
4.  Pause/Resume does not create catch-up/duplicate damage or broken
    vignette timing;
5.  zero Health enters one death phase;
6.  movement/interactions/Pause/Inventory are blocked during death;
7.  without active Checkpoint, death respawns at current-Level spawn
    with maximum Health;
8.  with active Checkpoint, death respawns there with maximum Health;
9.  collected Item/run-local state is preserved according to the
    contract rather than behaving like Restart Level;
10. run timer is not reset by death;
11. no stale Hazard damage/death loop occurs after respawn;
12. `level_01 -> level_02` still preserves nonzero Health;
13. Restart restores maximum Health;
14. Main Menu → Play and Run Complete → Play Again start at maximum
    Health;
15. Pause, Inventory, F2/editor, `LEVEL COMPLETE`, `RUN COMPLETE`, and
    Main Menu remain coherent;
16. essential flow works in Release.

Automated green is not sufficient.

## Documentation

Canonical active document: `docs/milestones/MILESTONE_70.md`.

Preserve M69 as CLOSED and keep `docs/MILESTONES.md` compact. Update
current architecture/README/AGENTS only according to repository
conventions and actual implementation.

After implementation, M70 is **implemented, awaiting manual
acceptance**, not CLOSED.

## Cursor report and STOP

Report files changed; vignette trigger/intensity/duration; death-state
authority/delay; zero-Health handling; respawn destination; Health
restoration; runtime state preserved/reset; Hazard-contact safety after
respawn; Pause/Inventory/F2/completion behavior; HUD/death presentation;
fall/manual respawn behavior and whether it changed; M69 lifecycle
preservation; authored/Dirty safety; Development/Release behavior;
C++/Python/build results; `git diff --check`; canonical Level status;
and explicit confirmation that no generic
combat/damage/post-processing/state/UI framework, Level Format v2, or
M71 functionality was added.

Then STOP.

Do NOT commit.\
Do NOT push.\
Do NOT merge.\
Do NOT start M71.\
Do NOT mark M70 CLOSED.
