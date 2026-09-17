# Milestone 68 — Pause Menu & Runtime Navigation

## Status

**Implemented, awaiting manual acceptance.** Milestone 67 is CLOSED. Do not
start Milestone 69. Do not mark M68 CLOSED.

## Goal

Add a narrow, Release-capable Pause Menu to the existing gameplay flow so that `Esc` during active Gameplay pauses the current run instead of closing the application. The paused player can either resume the exact current gameplay state or abandon the run and return to the existing Main Menu.

This milestone must extend the existing Application-owned flow from M65/M66 rather than introducing a generic game-state, scene, menu, or UI framework.

## Branch

`milestone/68-pause-menu-runtime-navigation`

## Cursor Model

**Grok 4.6 High — Fast OFF**

## Required Behavior

### Entering Pause

During active Gameplay, pressing `Esc` enters Pause.

Entering Pause must:

- preserve the current runtime level and all current run-local gameplay state;
- stop gameplay simulation while paused;
- freeze the run timer;
- prevent player movement and gameplay interactions;
- prevent Grab/Drop, Item Pickup, Door, Pressure Plate, Level Goal, Hazard and other gameplay progression while paused;
- prevent destination-transition hold time from advancing if Pause can be entered during that phase; however, completion overlays retain their existing input authority and Pause must not steal `Esc` from an existing completion/result behavior;
- not rebuild physics, reload a Level, mutate authored data, or reset the run.

The implementation must respect the existing `simulationPaused`/flow authorities where applicable rather than creating parallel pause semantics.

### Pause Menu

Render a simple raylib-based Pause Menu that is available in Release builds and does not depend on ImGui.

Required presentation:

```text
PAUSED

RESUME
MAIN MENU
```

Use the existing menu/input conventions established by M66 where practical. Keyboard navigation should remain coherent with the Main Menu. `Enter` activates the selected option.

`Esc` while paused should act as **Resume**, providing the expected toggle behavior:

```text
Gameplay --Esc--> Pause --Esc--> Gameplay
```

Input handling must be edge-based and must not allow the same `Esc` press that enters Pause to immediately resume it.

### Resume

`RESUME` returns to the exact gameplay state that existed before Pause.

It must not:

- reload/rebuild the Level;
- reset player/physics state;
- clear Inventory;
- reset pickups, doors, pressure plates, dynamic boxes, checkpoint, timer, completion state, feedback, or HUD state;
- alter `currentRuntimeLevelId`;
- mutate authored/working-copy state or Dirty.

Gameplay simulation and timer continue normally on the first valid resumed frame without input carry-through causing an unintended gameplay action.

### Main Menu from Pause

`MAIN MENU` abandons the current run and returns to the existing M66 Main Menu.

Once Main Menu is active:

- gameplay simulation remains inactive;
- gameplay HUD/objective presentation remains hidden;
- gameplay interactions remain inactive;
- the next `PLAY` must continue to use the existing M66 fresh-play path, starting staged `level_01` and resetting run-local state according to the already established Play Again/New Run semantics.

Do not invent a separate reset path for Pause → Main Menu → Play if the existing M65/M66 path can remain authoritative.

Returning to Main Menu must not close the application.

### Existing Flow Authority

Preserve the semantics established by M64.1, M65, M66 and M67.

In particular:

- Main Menu remains the top-level startup/player-facing menu.
- `PLAY` remains staged-only and atomic on failure.
- destination Level Complete hold remains authoritative for destination transitions.
- Run Complete retains its existing `Enter > Esc > R` input priority and `Esc MAIN MENU` behavior.
- Inventory retains its existing `Esc` behavior: if Inventory is open, `Esc` closes Inventory first rather than entering Pause.
- M67 Gameplay Objective HUD is hidden while paused.
- M61/M62 transient feedback must not incorrectly advance/replay/reset solely because Pause was entered or resumed; follow existing simulation/UI timing authority.

Do not reinterpret M66 Main Menu or M65 Run Complete as generic states solely to implement Pause.

## F2 / Development Editor Interaction

Development editor behavior must remain coherent with the existing M66/M67 contract.

Do not allow F2/editor overlay handling to accidentally advance paused gameplay.

If F2 is supported while Pause is active under the current architecture, entering/exiting the editor must preserve the fact that gameplay was paused and must not convert Pause into active Gameplay. If the existing input authority makes F2 intentionally unavailable from Pause, preserve that narrow behavior and cover it with tests/documentation rather than creating a larger state-stack system.

No source-authored Open/Switch/Apply/Reload operation should implicitly be introduced as part of Pause behavior.

## HUD / Presentation Priority

While Pause Menu is visible:

- hide the normal Gameplay Objective HUD from M67;
- hide or suppress ordinary gameplay interaction prompts that could imply gameplay is active;
- Pause Menu must have clear visual/input priority over gameplay HUD;
- do not display Pause over Main Menu or Run Complete;
- do not create a generic overlay manager.

Existing completion/result overlays retain their established authority.

## Lifecycle and State Ownership

Use the narrowest Application-owned representation consistent with the existing `gameplay::TopLevelFlow` and completion/transition authorities.

Pause is transient runtime state only.

Required lifecycle cases include:

- Main Menu → Play → Gameplay → Pause → Resume;
- Gameplay → Pause → Main Menu → Play;
- level transition → destination Gameplay → Pause/Resume;
- Restart behavior after a resumed pause;
- Inventory open → `Esc` closes Inventory, subsequent `Esc` may Pause;
- Run Complete `Esc` still goes directly to Main Menu and does not enter Pause;
- failed Play/transition behavior remains atomic and does not expose an invalid Pause state;
- Development F2 round-trip remains coherent.

Do not serialize Pause into Level files or editor layout.

## Authored Data / Dirty Safety

M68 must not semantically change canonical Level data.

Pause/Resume/Main Menu navigation must not mutate:

- `LevelDefinition`;
- editor `workingCopy`;
- authored source files;
- Dirty state;
- Level Goal destinations;
- Item Pickup, Door, Pressure Plate, Dynamic Box, Checkpoint or other authored specs.

`level_01.level` and `level_02.level` should require **no semantic changes** for this milestone.

## Release / Performance

- Pause Menu must work in Development and Release.
- Release must not depend on ImGui, authoring source tree, editor catalogs, or source fallback.
- No per-frame filesystem I/O, source scan, catalog refresh, or allocation-heavy framework is justified for Pause.
- Continue to use staged runtime authority where Level loading is involved through existing flows.

## Explicitly Out of Scope

Do **not** add any of the following in M68:

- Settings/Options menu;
- audio-volume controls;
- graphics settings;
- key rebinding;
- save/load game;
- campaign/progress persistence;
- Level Select;
- confirmation dialog for abandoning a run;
- Pause background screenshot/blur system;
- mouse-driven menu framework;
- generic `GameState`, state stack, SceneManager or Scene system;
- generic menu/navigation framework;
- generic HUD/overlay/UI manager;
- Objective/Quest system;
- localization framework;
- Level Format v2;
- GUIDs;
- M69 functionality.

## Automated Validation

Inspect the repository first and use current tests/targets as the source of truth. Add focused regression coverage for Pause behavior and extend existing Game Flow/input tests where appropriate.

At minimum validate:

- Gameplay `Esc` enters Pause instead of closing the application;
- Pause freezes simulation/timer/gameplay interactions;
- `Esc` from Pause resumes without same-edge carry-through;
- `RESUME` preserves runtime/run-local state;
- `MAIN MENU` returns to M66 Main Menu without quitting;
- subsequent `PLAY` starts a fresh staged `level_01` using existing M66 authority;
- Inventory `Esc` closes Inventory before Pause;
- destination completion hold and Run Complete retain their established authority;
- M67 objective HUD is suppressed while paused and restored after Resume;
- Pause does not mutate authored/Dirty state;
- F2/editor behavior is coherent in Development;
- Release has no editor/source dependency.

Run the relevant existing C++ tests covering GameFlow, LevelTransition, LevelGoal, Inventory/InventoryUi, gameplay HUD/objective, Item Pickup, Door/Pressure Plate, Dynamic Box Grab, authored lifecycle/editor workspace and physics rebuild as applicable.

Run the established Python regression suite relevant to the current project, including:

```text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Build all three configurations:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Also run:

```text
git diff --check
```

Verify canonical Level data has no unintended semantic changes.

## Manual Acceptance

After automated validation, STOP and wait for user acceptance. Manual acceptance should cover at least:

1. Start at Main Menu; Pause UI is not visible.
2. Play `level_01`; press `Esc`; Pause appears and the application does not close.
3. Leave the game paused briefly; player/physics/gameplay progression and run timer remain frozen.
4. Press `Esc`; exact gameplay state resumes.
5. Pause again, select `RESUME` with menu controls; exact gameplay state resumes.
6. Open Inventory; press `Esc`; Inventory closes without opening Pause. Press `Esc` again; Pause opens.
7. Pause and choose `MAIN MENU`; Main Menu appears, gameplay/objective HUD is hidden, application remains open.
8. Choose `PLAY`; a fresh `level_01` run starts according to M66 semantics.
9. Progress to `level_02`; Pause/Resume works there and preserves the destination-level state.
10. During destination `LEVEL COMPLETE`, existing completion behavior retains priority and Pause does not corrupt/steal the flow.
11. At `RUN COMPLETE`, existing Enter/Esc/R behavior remains unchanged; `Esc` returns to Main Menu rather than Pause.
12. In Development, exercise F2/editor round-trip around normal/paused gameplay according to the implemented narrow contract and verify no gameplay advancement/state corruption.
13. In Release, verify the essential Main Menu → Play → Pause → Resume → Pause → Main Menu flow.
14. Confirm Pause presentation does not collide confusingly with existing HUD/prompts and that gameplay prompts/objective HUD do not imply active play while paused.

## Documentation

Update the milestone index/current architecture documentation according to the repository's post-M60 milestone-documentation conventions. Add this file canonically as:

`docs/milestones/MILESTONE_68.md`

Do not mark M68 CLOSED before explicit manual acceptance and Git closure.

## STOP Condition

After implementation, tests, builds and validation, report:

- files changed;
- architecture/state ownership used;
- exact Pause/Menu behavior;
- lifecycle behavior;
- test/build results;
- canonical Level-data status;
- explicit confirmation of excluded future scope.

Then **STOP**.

Do not commit, push, merge, start M69, or mark M68 CLOSED.
