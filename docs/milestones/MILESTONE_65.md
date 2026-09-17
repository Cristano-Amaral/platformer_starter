# Milestone 65 --- Game Flow & Run Completion

## Status

**Implemented, awaiting manual acceptance.** M64.1 is CLOSED. Do not start Milestone 66.

## Canonical Path

`docs/milestones/MILESTONE_65.md`

## Branch

`milestone/65-game-flow`

## Cursor Model

**Grok 4.6 High --- Fast OFF**

## Goal

Add the first narrow explicit game-flow layer on top of M63/M64/M64.1
without a generic GameState/campaign framework.

Distinguish: - destination Level Goal → existing M64.1 completion hold →
next staged Level; - terminal Level Goal → explicit **RUN COMPLETE**
results flow; - Restart Current Level → existing Restart semantics; -
Play Again / Restart Run → staged `level_01` with a fresh session.

No Main Menu, campaign graph, savegame or generic state machine.

## Context / Authority

Follow the post-M60 contract. Read this file first, then current
code/tests/current docs. Read older milestones only for concrete
dependencies.

Authority: code → tests → current architecture/docs → active milestone →
latest relevant current docs → history.

Inspect current completion, transition, run timer, SessionBestTimeState,
Inventory lifecycle, Restart, renderer HUD and input paths before
coding. If concrete code conflicts with terminology here, use the
narrowest equivalent preserving existing authority and report it.

## Preserve

Preserve Level Format v1, `nextLevelId`, staged-only/atomic M64
transitions, Inventory carry-over between Levels, fresh destination
state, checkpoint locality, Restart-current-level semantics, M64.1 1.75
s destination hold/Enter Continue, Level Browser/New/Open/Dirty
guard/Next Level selector, editor authority, gameplay interaction
priority and Release independence from ImGui/source assets.

## Narrow Game Flow

Do not introduce generic `GameState`, SceneManager, state stack,
transition graph or event framework. Application may own only the
minimum explicit state needed for normal gameplay, existing destination
completion/transition, and terminal Run Complete/results.

## Terminal Goal → Run Complete

A terminal goal (`nextLevelId` empty) ends the current run exactly once
and enters a Release-capable results overlay.

Preferred semantics:

``` text
RUN COMPLETE

TIME  <captured final time>
BEST  <only if existing SessionBestTimeState is legitimately compatible>

ENTER   PLAY AGAIN
R       RESTART LEVEL
```

Exact layout may follow current renderer conventions. If
SessionBestTimeState is not semantically valid without scope expansion,
omit BEST and explain. No persistence/leaderboard/profile work.

## Play Again / Restart Run

Enter from Run Complete starts a fresh run from staged `level_01`.

It must safely load/rebuild `level_01`, start at its spawn, clear
completion/transition/results, Inventory, checkpoint, collected state,
M61/M62 feedback, grab/carry, Door/Pressure Plate runtime and other
discovered run-local transient state, and reset timing according to
existing NewRun semantics.

`level_01` remains the narrow bootstrap initial Level; do not infer
campaign ordering or add a campaign asset.

Prepare/validate before destructive replacement. Failure must remain
coherent and staged-only.

## Restart Current Level

R from Run Complete restarts the current terminal Level using existing
Restart semantics. Do not redefine existing Inventory/timer Restart
lifecycle. Stay on current Level identity, leave Run Complete and return
to Gameplay.

## Destination Goals

Preserve M64.1 exactly: `LEVEL COMPLETE / PRESS ENTER TO CONTINUE`, 1.75
s hold, timeout auto-continue, Enter skip, staged-only safe transition,
Inventory carry-over, fresh destination-local state. No Run Complete
overlay between linked Levels.

## Input Arbitration

Normal Gameplay controls remain unchanged.

Destination completion: Enter = Continue/skip; timeout = automatic
Continue.

Terminal Run Complete: Enter = Play Again; R = Restart Current Level.

No action may double-trigger, leak held input, also invoke legacy
restart, destination transition or gameplay interaction. Use existing
edge semantics. Terminal completion is emitted exactly once.

## Timer / Results

Capture terminal completion time exactly once and freeze displayed final
time. Play Again follows NewRun timing. Restart Current Level follows
existing Restart timing. Ordinary level transition timing remains
unchanged.

Reuse SessionBestTimeState only if already semantically compatible. No
splits, leaderboards or persistence.

## Pause / Editor / Inventory

Run Complete is gameplay flow, not an editor modal. Preserve current
F2/Inventory pause authority. Results must not allow gameplay/Inventory
mutation of the completed run, editor round-trips must not retrigger
completion/actions, and result timing stays frozen. Report exact
behavior.

## Lifecycle

Test/document Initialize/NewRun, Restart Current Level, Play Again,
Apply, Reload, editor Open/Switch, successful/failed destination
transition, terminal completion, checkpoint and physics rebuild. No
stale results state may leak.

## Failure Safety

Preserve M64 atomic principles for Play Again. Missing/malformed staged
`level_01` must not create a half-reset runtime. No source fallback or
retry storm; use narrow existing diagnostics.

## Release

Release must support linked transition, terminal Run Complete, Enter
Play Again from staged level_01, and R Restart Current Level, with no
ImGui/source-tree dependency.

## Performance

No per-frame level file I/O, bounded explicit flow state, no
history/state stack, no generic state/event framework, no unbounded
results history.

## Focused Tests

Cover equivalents of: 1. terminal goal enters Run Complete exactly once;
2. destination goal never enters Run Complete; 3. M64.1 destination
hold/timeout/Enter remains; 4. final time captured/frozen; 5.
terminal-only results HUD; 6. Enter Play Again exactly once; 7. Play
Again loads staged level_01, never source; 8. Play Again resets
Inventory and run-local state; 9. level_01 spawn after Play Again; 10.
atomic missing/malformed initial staged Level failure/no retry; 11. R
restarts current terminal Level; 12. Restart preserves existing Restart
Inventory/timer semantics; 13. R never loads level_01 as Play Again; 14.
Enter/R edge handling prevents double/carry-through; 15. destination
transition still preserves Inventory/fresh destination; 16.
Apply/Reload/editor Open-Switch reconcile results; 17.
checkpoint/physics rebuild do not fabricate results; 18. SessionBestTime
remains compatible if displayed; 19. Release works without ImGui; 20.
existing gameplay/editor/E-arbitration regressions; 21. canonical data
safety.

## Manual Acceptance

Use disposable goals and remove them before closure unless explicitly
canonical.

1.  Test linked level_01 → level_02 and confirm M64.1 hold/timeout/Enter
    behavior.
2.  Use a terminal goal and confirm **RUN COMPLETE** replaces the old
    terminal restart-only flow.
3.  Confirm final time is frozen/readable; BEST only if valid.
4.  Press R: current terminal Level restarts, not level_01; verify
    existing Restart Inventory semantics.
5.  Complete again; press Enter: Play Again returns to staged level_01
    spawn with fresh
    Inventory/checkpoint/pickup/feedback/grab/Door/Plate/completion/timer
    state.
6.  Confirm no double action/input carry-through.
7.  Confirm linked transition still preserves Inventory.
8.  Check F2/Inventory behavior around Run Complete.
9.  Repeat linked transition, Run Complete, R and Enter in Release.
10. Where practical, verify failed initial staged load is atomic/no
    source fallback.

Remove all disposable fixtures before closure.

## Validation

Run relevant C++ tests for goals/transitions/results, Inventory
lifecycle/UI, checkpoint, pickups/M61/M62, grab, Door/Plate, Physics
rebuild, renderer/HUD/input, editor lifecycle and canonical cleanup.

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

Update compact milestone index/status and only current
architecture/game-flow docs required by post-M60 conventions. Document
terminal vs destination completion, Play Again, Restart Current Level,
fresh-run lifecycle, timer/result authority, staged-only initial restart
and Release behavior.

## Canonical Data Safety

Keep legacy `goal -21 3.8 0 2 1.6 1.8` and `dynamic_box 0 5 0 1 1 1 30`
absent. Keep canonical level_01 free of manual acceptance Level Goal
fixtures unless explicitly chosen otherwise. Keep intentional level_02.
Leave no disposable levels/goals. Do not mechanically normalize EOLs.

## Out of Scope

No Main Menu; player-facing Level Select; campaign graph/order asset;
savegame/progress persistence; Continue-from-save; pause/settings menu;
results history; leaderboard; online services; fade/loading screen;
credits; cutscene; named spawn routing; generic SceneManager; generic
GameState/state stack; generic Objective/Trigger/Receiver/Event
framework; GUIDs; Level Format v2; M66.

## Completion Criteria

Ready for manual acceptance when terminal goals produce a clear
Release-capable Run Complete flow, Enter starts a fresh staged level_01
run, R restarts the current terminal Level with existing semantics,
destination goals retain M64.1 behavior and Inventory carry-over,
lifecycle/timer/input/failure semantics are tested, Release works,
validation passes, canonical content is clean, and no Main
Menu/campaign/savegame/generic state framework/v2/M66 was added.

## STOP

After implementation, validation, documentation and report: **STOP**. Do
not commit, push, merge, start M66, or declare M65 CLOSED. Wait for user
manual acceptance and separate Git closure.
