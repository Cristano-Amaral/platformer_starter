# Milestone 67 --- Gameplay HUD & Objective Presentation

**Status:** CLOSED. Milestone 68 is the active milestone.

**Canonical path:** `docs/milestones/MILESTONE_67.md`

**Branch:** `milestone/67-gameplay-hud-objective`

**Cursor:** Grok 4.6 High --- Fast OFF

## Goal

Add a narrow Release-capable Gameplay HUD that communicates the current
Level and immediate objective during the canonical
`Main Menu → level_01 → level_02 → Run Complete` flow. Derive
presentation from authorities that already exist; do not create an
Objective/Quest system.

## Presentation

During active Gameplay, show a compact objective area, preferably
top-left unless current HUD layout requires safer placement. Minimum:
player-facing current Level label derived from `currentRuntimeLevelId`,
plus immediate goal wording derived from active `LevelGoalSpec` data.

Canonical semantics should be approximately:

``` text
LEVEL 01
OBJECTIVE: REACH THE LEVEL GOAL
```

and:

``` text
LEVEL 02
OBJECTIVE: REACH THE FINAL LEVEL GOAL
```

Exact casing/layout may follow renderer conventions. Logical IDs may be
narrowly formatted for display without new authored metadata.

A small `NEXT: LEVEL 02` hint for destination goals is optional only if
naturally player-facing.

## Authority

Use `currentRuntimeLevelId`, active `LevelDefinition`,
`LevelGoalSpec::nextLevelId`, and existing
completion/transition/RunComplete state.

No authored objective text, objective IDs, quest steps, localization
keys, GUIDs, campaign nodes, persistence or generic objective manager.

No goals: omit objective or use a neutral narrow fallback.
Multiple/mixed goals: do not invent ordering; use truthful generic
wording.

## Optional Level-Start Banner

A brief presentation-only `LEVEL 01` banner may be added only if narrow
using existing timing/rendering. Bounded, reset on actual Level entry,
never serialized. Do not build a notification framework for it.

## Visibility

Hide/suppress objective HUD during Main Menu, Inventory, Run Complete,
and destination completion hold when completion UI has priority. Follow
current F2/editor HUD suppression conventions.

Do not collide with timer, E/Grab/Door/Pickup prompts, M62
notifications, Level Complete, Run Complete or Main Menu.

## Lifecycle

Refresh correctly on Play, successful transition, Restart Level, Play
Again, RunComplete→Menu→Play, Apply/Reload, Development Open/Switch, and
failed transition/load. Never show the destination as active before
successful replacement. No Dirty/authored mutation.

## Preserve

Preserve M64.1 hold/Continue, M65 RunComplete/PlayAgain/Restart, M66
MainMenu/Play/Quit/Esc-to-menu, staged-only runtime, Inventory
carry-over between linked Levels, fresh-run semantics, editor authority,
E arbitration and Level Format v1.

## Release / Performance

Release-capable, no ImGui/source dependency. No per-frame file
I/O/source scan/catalog work. Derive from already-loaded runtime state.
No generic HUD framework.

## Focused Tests

Cover canonical level_01 and level_02 presentation; destination vs
terminal wording; no-goal and mixed/multiple-goal behavior; visibility
in Main Menu/Inventory/RunComplete/destination completion; transition
refresh; Restart; PlayAgain; Menu→Play; F2/Open-Switch; failed
transition; no authored/Dirty mutation; layout coexistence with
timer/prompts/M62; Release; existing M64.1/M65/M66 regressions. If a
start banner is added, test its bounded timing/lifecycle narrowly.

## Manual Acceptance

1.  Main Menu: no objective HUD.
2.  Play level_01: Level 01/current objective visible.
3.  Inventory: objective suppressed.
4.  level_01 completion: completion UI has priority.
5.  level_02: HUD updates to Level 02/final-goal wording.
6.  terminal completion: RunComplete hides objective HUD.
7.  R Restart: level_02 objective returns.
8.  Enter PlayAgain: level_01 objective returns.
9.  Esc to Menu: no objective; Play restores level_01.
10. F2 round-trip coherent.
11. Essential flow in Release.
12. Timer, interaction prompts and M62 remain readable.

## Validation

Run relevant C++ HUD/renderer, GameFlow, LevelTransition, LevelGoal,
Inventory UI, M62, editor/input lifecycle and canonical tests.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Build Debug/Development/Release with normal presets and run
`git diff --check`.

## Documentation

Update compact milestone index/status and only current
architecture/HUD/game-flow docs required by post-M60. Document
derivation, visibility, lifecycle, Release behavior and absence of
Objective/Quest framework.

## Canonical Safety

Preserve canonical `level_01 → level_02 → Run Complete`; no level_3;
keep legacy `goal -21 3.8 0 2 1.6 1.8` and `dynamic_box 0 5 0 1 1 1 30`
absent. M67 should require no canonical Level changes. Do not normalize
EOLs.

## Out of Scope

No authored objective text; quest/objective system; localization;
minimap; compass; waypoint/beacon; campaign graph; Level Select;
save/progress; Pause/Settings; generic HUD/notification/UI framework;
SceneManager; generic GameState; GUIDs; Level Format v2; M68.

## STOP

After implementation, validation, documentation and report: STOP. Do not
commit, push, merge, start M68, or declare M67 CLOSED. Wait for manual
acceptance and separate Git closure.
