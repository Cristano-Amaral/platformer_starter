# Milestone 66 --- Main Menu & Play Flow

## Status

**Implemented, awaiting manual acceptance.** M65 is CLOSED. Do not start Milestone 67.

## Canonical Path

`docs/milestones/MILESTONE_66.md`

## Branch

`milestone/66-main-menu-play-flow`

## Cursor Model

**Grok 4.6 High --- Fast OFF**

## Goal

Add the first Release-capable player-facing **Main Menu** on top of M65.

Canonical flow:

``` text
APPLICATION START
→ MAIN MENU
   PLAY → fresh staged level_01 run
   QUIT → normal application close

level_01 → level_02 → RUN COMPLETE
                         ENTER → PLAY AGAIN → level_01
                         R     → RESTART LEVEL
                         ESC   → MAIN MENU
```

No Main Menu framework, campaign, Level Select, Continue, Settings,
savegame or generic GameState/SceneManager.

## Context / Authority

Follow post-M60. Read this file first, then current code/tests/docs.
Read M65/M64.1/older milestones only for concrete dependencies.

Authority: code → tests → current architecture/docs → active milestone →
current relevant docs → history.

Inspect Application initialization, M65 RunCompleteState, NewRun/Play
Again, renderer/input, window-close path, Inventory/F2 and Release loop
before coding.

## Preserve

Preserve canonical `level_01 → level_02 → RUN COMPLETE`, Level Format
v1/authored Goals, M65 Enter Play Again/R Restart, M64.1 1.75 s
destination hold, staged-only atomic loading, Inventory carry-over
between linked Levels, fresh-run resets, editor Level
Browser/New/Open/Dirty authority, E arbitration and Release independence
from ImGui/source assets.

## Narrow Flow State

Application may own only the minimum explicit top-level distinction for
Main Menu vs Gameplay, integrated with existing completion/transition
state. A small explicit enum/flag is acceptable.

No generic GameState, state stack, SceneManager, transition graph,
screen registry or event bus.

## Startup

After platform/window/renderer initialization, player-facing flow starts
at Main Menu. Gameplay simulation and run timer must not advance merely
because the executable launched. Gameplay input is inactive until Play.

If existing architecture needs staged bootstrap preparation during
initialization, it may remain, but Main Menu stays authoritative. Report
final startup authority.

## Main Menu UI

Use existing Release-capable renderer/raylib style, no ImGui dependency.

Minimum: - canonical project/game title if one exists; - **PLAY**; -
**QUIT**.

Keyboard-first is sufficient. Prefer current conventions; do not build
generic menu widgets. Up/Down + Enter is acceptable. Mouse is optional
only if trivial.

No Settings, Continue, Level Select, Credits or profiles.

## Play

PLAY starts a fresh run from staged `levels/level_01.level`.

Reuse M65 Play Again/NewRun reset authority. It must safely start at
level_01 spawn and reset Inventory, checkpoint, collectibles/pickups,
M61/M62, grab/carry, Door/Pressure Plate,
completion/transition/RunComplete and timer.

No source/cooked fallback.

Missing/malformed staged level_01 or rebuild failure must remain
atomically in Main Menu with coherent runtime, narrow diagnostics and no
retry storm.

## Main Menu Authority

While Main Menu is active: - gameplay simulation and timer do not
advance; - E/Grab/Pickup/Door/Goal do not run; - Tab Inventory does not
open; - gameplay HUD/prompts/M61/M62 are not active; -
transition/RunComplete cannot be fabricated; - menu input cannot leak
into first gameplay frame.

## Run Complete → Main Menu

Extend M65 results with:

``` text
RUN COMPLETE
TIME <captured>

ENTER   PLAY AGAIN
R       RESTART LEVEL
ESC     MAIN MENU
```

Esc returns to Main Menu exactly once without starting a run. Results
are no longer authoritative/visible there. Next Play performs normal
fresh staged level_01 reset.

Do not reinterpret Esc during destination hold.

## Gameplay Esc / Pause

A new general Pause Menu is **out of scope**. If an already-existing
narrow pause overlay can be reused without new state/menu architecture,
Cursor may report and reuse it; otherwise ordinary Gameplay Esc keeps
current behavior.

Do not create Pause/Settings in M66.

## M65 Preservation

Destination Goal remains `LEVEL COMPLETE / PRESS ENTER TO CONTINUE`,
1.75 s hold, timeout/Enter Continue, staged-only transition and
Inventory carry-over.

Terminal Goal remains Run Complete. Enter = Play Again, R = Restart
Current Level, final time frozen.

## Input Arbitration

Use edge semantics. Main Menu actions cannot also trigger gameplay. Run
Complete Enter/R/Esc are mutually exclusive and deterministic. Preserve
existing Enter priority over R unless current code provides a concrete
reason otherwise. Prevent held-key/carry-through.

## Development Editor / F2

Development also exposes Main Menu. Preserve safe F2 editor access
according to current architecture: - editor must not accidentally start
a run; - returning restores prior top-level flow coherently; - authored
Open/Switch remains Development-only and must not silently turn Main
Menu into Gameplay; - no source authority leaks into Release.

Report exact behavior.

## Lifecycle

Test/document Initialize→Menu, Menu→Play, Play failure, linked
transition, Run Complete, Play Again, Restart Level, Run Complete→Menu,
Menu→Play after completion, F2 round-trip, relevant
Apply/Reload/Open-Switch and Quit. No stale flow/menu/completion state.

## Quit

QUIT requests normal application/window close through current
platform/raylib authority. No forceful process termination, save prompt
or shutdown framework.

## Release

Release supports:
`Main Menu → Play → level_01 → level_02 → Run Complete → Play Again / Restart Level / Main Menu → Quit`.
All runtime Level loads staged-only.

## Performance

No per-frame level file I/O/source scans, no unnecessary reload every
Menu frame, bounded explicit state, no screen history/state stack or
generic UI framework.

## Focused Tests

Cover equivalents of: 1. initialization enters Main Menu; 2. menu does
not advance simulation/timer; 3. gameplay interactions/Inventory
inactive in menu; 4. Play fresh-run exactly once; 5. Play staged
level_01 only and resets all NewRun state; 6. level_01 spawn; 7. Play
failure stays atomically in menu/no retry/source fallback; 8. menu input
does not carry into gameplay; 9. Quit requests normal close; 10. linked
destination behavior unchanged; 11. terminal Run Complete unchanged; 12.
Enter Play Again unchanged; 13. R Restart unchanged; 14. Esc Run
Complete enters Main Menu exactly once; 15. results HUD advertises Main
Menu; 16. menu hides results/gameplay HUD; 17. Play after returning to
menu is fresh; 18. deterministic result input arbitration; 19.
Development F2 round-trip preserves flow; 20. editor Open/Switch does
not fabricate flow; 21. Release without ImGui/source assets; 22.
canonical progression remains; 23. existing editor/gameplay/E
regressions.

## Manual Acceptance

1.  Launch Development: Main Menu, not active Gameplay.
2.  Idle: gameplay/timer remain stopped; Tab/E inactive.
3.  PLAY: fresh level_01 spawn, empty Inventory, fresh timer.
4.  Follow canonical level_01→level_02 and confirm M64.1 hold.
5.  Terminal goal: M65 Run Complete.
6.  R: Restart level_02.
7.  Complete again; Enter: Play Again to fresh level_01.
8.  Reach Run Complete; Esc: Main Menu.
9.  Play again: another fresh level_01.
10. Confirm no input carry-through.
11. F2 editor round-trip does not accidentally start a run.
12. QUIT closes normally.
13. Repeat essential flow in Release.

No disposable Goals are needed because M65 intentionally made canonical
progression permanent.

## Validation

Run relevant C++
GameFlow/MainMenu/LevelTransition/Inventory/checkpoint/pickup/M61/M62/grab/Door/Plate/Physics/renderer/input/editor/canonical
tests.

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

Run `git diff --check`.

## Documentation

Update compact milestone index/status and current architecture/game-flow
docs required by post-M60. Document startup Menu, Play fresh-run
authority, Quit, RunComplete→Menu, editor behavior, staged-only Release
flow and Pause exclusion.

## Canonical Data Safety

Preserve intentional M65 progression: level_01 destination Goal→level_02
and level_02 terminal Goal. No level_3. Keep legacy
`goal -21 3.8 0 2 1.6 1.8` and `dynamic_box 0 5 0 1 1 1 30` absent. Do
not mechanically normalize EOLs.

## Out of Scope

No new general Pause Menu; Settings; Continue; Level Select; campaign
graph/order; savegame/progress; profiles; Credits; results history;
leaderboard; loading/fade framework; generic menu/widget framework;
generic SceneManager/GameState/state stack/screen registry; generic
Objective/Trigger/Receiver/Event framework; GUIDs; Level Format v2; M67.

## Completion Criteria

Ready for manual acceptance when app starts in a Release-capable Main
Menu, Play starts a fresh staged level_01 run using M65 authority, Quit
closes normally, gameplay is inactive in Menu, Run Complete gains
Esc→Menu while Enter/R remain intact, canonical progression remains,
Development editor access is coherent, Release works, validations pass,
and no Pause/Settings/LevelSelect/campaign/savegame/generic
framework/v2/M67 was added.

## STOP

After implementation, validation, documentation and report: **STOP**. Do
not commit, push, merge, start M67, or declare M66 CLOSED. Wait for user
manual acceptance and separate Git closure.
