# Milestone 64.1 --- Editor Level Browser & Transition UX

## Status

**Status:** Implemented, awaiting manual acceptance. M64 is CLOSED. Start only from clean synchronized `main`.

## Canonical Path

`docs/milestones/MILESTONE_64_1.md`

## Branch

`milestone/64-1-editor-level-browser-transition-ux`

## Cursor Model

**Grok 4.6 High --- Fast OFF**

## Goal

Complete M64's first usable multi-level authoring workflow with two
narrow additions:

1.  **Editor Level Browser & Creation** --- explicitly discover, create
    and open authored levels, and reuse discovery for Level Goal
    `Next Level`.
2.  **Transition UX** --- destination goals show readable completion
    feedback for a short runtime-only hold; Enter skips the remaining
    wait, otherwise transition is automatic.

No campaign, SceneManager or generic game-state framework.

## Context / Authority

Follow the post-M60 contract. Read this file first, then current
code/tests/architecture/docs. Read M64/older files only for concrete
dependencies.

Authority: code → tests → current docs → active milestone → latest
relevant current docs → history.

## Preserve

Preserve M64 optional `nextLevelId`, v1 syntax, safe identities,
staged-only Gameplay/Release loading, `currentRuntimeLevelId`, atomic
failure, Inventory carry-over, fresh destination state,
Restart-current-level, checkpoint reset, terminal M63 goals, M63
visualization, workingCopy/Apply/Save authority and existing gameplay/E
arbitration.

## A --- Editor Level Browser

Add a narrow Development-only **Levels** UI using current ImGui/editor
conventions.

It must discover valid `.level` files from the configured authored
source-level root, validate/parse them, sort deterministically, avoid
duplicates, show logical IDs and clearly mark the current level. It must
allow Open/Switch, New Level and refresh after creation.

Do not scan every frame; refresh at narrow editor lifecycle points.
Gameplay/Release remain staged-only and never use source discovery.

### New Level

`New Level` asks for a safe logical ID using M64 identity rules. Reject
duplicates, traversal, paths, extensions and unsafe names. Never
overwrite. Create a minimal valid/playable Level Format v1 file using
current required singleton/default conventions, refresh discovery, and
preferably open it when Dirty rules permit.

No templates/wizard/procedural generation.

### Cook/Stage

A newly created valid level must enter the normal cook/stage/build flow
without requiring manual per-level CMake/source enumeration edits.
Narrowly make tooling enumeration discovery-driven where appropriate.
Release remains deterministic and staged-only. No generic asset
database.

### Open / Switch

Open validates authored source data, updates current identity
coherently, initializes active/workingCopy according to current
authority, rebuilds Development runtime/physics, clears level-local
transient state, reconciles selection and produces no false Dirty.

### Dirty Guard

Never silently discard pending workingCopy edits when Open/New is
requested.

Minimum: - **Cancel** --- remain unchanged. - **Discard and
Open/Create** --- explicit discard then continue.

A safe existing Save-and-continue option may be reused if already
reliable; do not invent autosave.

Cancel must preserve current level and pending edits.

## Level Goal Next Level Selector

Use the same discovery result in Level Goal Inspector.

-   `None`/empty = terminal;
-   discovered levels selectable;
-   selection writes logical ID to `nextLevelId`;
-   existing valid values round-trip;
-   a currently missing authored destination must be preserved and
    visibly flagged, never silently erased.

Prefer combo/dropdown instead of blind text. Level Format does not
change.

## Not in Scope for Level Management

No Rename/Delete Level, reference rewriting,
folders/tags/favorites/thumbnails/recent-files or multi-document editor.

## B --- Transition UX

Destination-bearing goals currently transition too quickly to read
completion. Add one runtime-only global hold constant.

Preferred duration: **1.75 seconds** (acceptable 1.5--2.0 if current
conventions justify it). Not authored and not serialized.

Destination flow:

``` text
goal completed
→ LEVEL COMPLETE
  PRESS ENTER TO CONTINUE
→ hold
   ├─ timeout → M64 safe deferred transition
   └─ Enter   → skip remaining hold → M64 safe deferred transition
```

Terminal goals remain:

``` text
LEVEL COMPLETE
PRESS ENTER TO RESTART
```

and Enter restarts current level. No automatic terminal transition.

### Enter Safety

Destination Enter means Continue, never Restart. No double scheduling,
held-key retrigger or input carry-through into the destination. Use
existing edge semantics.

### Timing

Use a runtime timing authority that continues after M63 freezes the run
timer. The hold is game-flow/UI timing, not score time. Keep behavior
deterministic and follow current Development pause conventions without
creating a generic timer framework. Report F2/Inventory behavior.

### Failure / Lifecycle

M64 atomic failure remains identical for timeout and Enter-skip. No
retry storm/source fallback.

Hold state is runtime-only and clears on fresh run, Restart, Apply,
Reload, successful editor Open/Switch, successful transition and
appropriate failure lifecycle. Checkpoint/PhysicsWorld rebuild never
fabricate it. Terminal completion never creates it.

## Release

Level Browser/New/Open are Development-only. Transition
hold/HUD/timeout/Enter skip work in Release without ImGui and use staged
assets only.

## Performance / Safety

No directory scan every frame, no file overwrite, atomic safe Open,
bounded deterministic list, no per-frame transition file I/O, no runtime
source fallback, no generic catalog framework.

## Focused Tests

Cover equivalents of: 1. discovery finds level_01/level_02, sorted/no
duplicates; 2. malformed/unsafe entries not offered; 3. safe New Level
succeeds; duplicate/unsafe rejects; never overwrites; 4. new valid level
enters normal cook/stage without per-level hardcoding; 5. Open/Switch
rebuilds coherent Development runtime with no false Dirty; 6. Dirty
switch cannot silently discard; Cancel preserves edits; explicit discard
works; 7. Next Level selector uses same discovery; None terminal;
selection writes ID; missing value preserved/flagged; 8. M64 Level
Format remains unchanged; 9. destination completion starts hold exactly
once and does not immediately transition; 10. timeout transitions
exactly once; 11. Enter skips hold and transitions exactly once, never
Restart; 12. terminal goal retains Enter-to-Restart and no countdown;
13. held/repeated Enter cannot double-trigger/carry through; 14.
countdown uses chosen post-completion timing authority; 15. lifecycle
clears hold; 16. failed transition remains atomic/no retry for timeout
and Enter; 17. Inventory carry-over, fresh destination, checkpoint reset
and Restart-current-level remain; 18. Release transition UX works
without ImGui; 19. existing editor/gameplay regressions remain green.

Avoid broad generic APIs solely for tests.

## Manual Acceptance

1.  Open Development Editor; confirm explicit Levels UI lists
    level_01/level_02 and marks current.
2.  Open level_02 from UI, then return to level_01; verify coherent
    geometry/runtime/editor state.
3.  Make a pending edit and request another level; verify no silent
    discard. Cancel preserves it; explicit discard allows switch.
4.  Create `level_03`; reject duplicate/unsafe IDs; confirm it appears
    and opens.
5.  Run normal cook/stage/build and confirm level_03 becomes staged
    without manual CMake enumeration edits.
6.  In Level Goal Inspector confirm discovery-backed Next Level
    selector; select level_02/03, Apply/Save/reload and verify
    persistence; None remains terminal.
7.  Complete destination goal: verify readable `LEVEL COMPLETE` +
    `PRESS ENTER TO CONTINUE`; no input transitions automatically after
    hold.
8.  Repeat and press Enter promptly; transition happens sooner, once,
    with no Restart/carry-through.
9.  Terminal goal still shows `PRESS ENTER TO RESTART`, never
    auto-transitions.
10. Confirm Inventory carry-over, fresh destination/checkpoint
    semantics, Restart-current-level and missing-destination atomic
    failure.
11. Run Release and verify timeout + Enter-skip behavior.

Remove disposable level_01 goals and test-only level_03 before closure
unless intentionally adopted as canonical content.

## Validation

Run relevant C++ tests for editor Dirty/lifecycle, level
identity/discovery, LevelFile/LevelGoal, transition, runtime loading,
Inventory, Checkpoint, PhysicsWorld and canonical cleanup.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Add/update narrow tooling tests proving discovery-created levels enter
cook/stage without hardcoded enumeration.

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
architecture/tooling/editor docs only where post-M60 conventions
require. Document source discovery/New/Open/Dirty guard,
source-vs-staged authority, Next Level selector, transition hold/Enter
skip, terminal HUD semantics and discovery-driven cook/stage.

## Canonical Data Safety

Keep legacy `goal -21 3.8 0 2 1.6 1.8` and `dynamic_box 0 5 0 1 1 1 30`
absent. Keep canonical level_01 free of manual Level Goal fixtures. Keep
intentional minimal level_02. Do not leave disposable level_03 unless
explicitly chosen as canonical.

## Out of Scope

No Rename/Delete Level/reference rewriting; campaign
graph/order/save/progress; savegame; player-facing Level Select; Main
Menu; results screen; transition fade/loading-screen framework; authored
delay; named spawn routing; multi-document editor; generic Content
Browser/catalog/database; generic SceneManager; generic
Objective/Trigger/Receiver/Event/GameState framework; GUIDs; Level
Format v2; M65.

## Completion Criteria

Ready for manual acceptance when Development explicitly discovers/lists
authored levels, safely creates/opens them with Dirty protection, new
levels enter normal cook/stage without per-level hardcoding, Level Goal
destination is discovery-backed, destination completion has readable
automatic hold plus Enter-to-continue skip, terminal goals retain
Enter-to-Restart, M64 staged-only/atomic/Inventory/fresh-level semantics
remain, Release works, validations pass, canonical content is clean, and
no Rename/Delete/campaign/SceneManager/generic framework/v2/M65 was
added.

## STOP

After implementation, validation, documentation and report: **STOP**. Do
not commit, push, merge, start M65, or declare M64.1 CLOSED. Wait for
user manual acceptance and separate Git closure.
