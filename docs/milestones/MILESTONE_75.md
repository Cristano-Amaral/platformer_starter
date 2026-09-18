# Milestone 75 --- UI Audio & Menu Feedback

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/75-ui-audio-menu-feedback`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Add concise semantic audio feedback to the existing Main Menu, Pause
Menu, and Inventory flows identified as a concrete presentation gap
during M74. Preserve all existing navigation, input priority, pause,
gameplay, editor, and authored-data semantics.

M75 is a focused extension of M71--M74 audio. It is not a Settings,
mixer, music, ambience, UI-framework, menu-redesign, gameplay-content,
or editor-productivity milestone.

## Required cues

Add project-generated staged WAV cues for: - UI Navigate: shared by Main
Menu and Pause when selection genuinely changes. - UI Confirm: shared by
Main Menu and Pause when an existing selected action is activated. -
Pause Open: successful Gameplay -\> Pause only. - Pause Close:
successful Pause -\> Gameplay resume only. - Inventory Open: successful
closed -\> open only. - Inventory Close: successful open -\> closed
only.

Suggested paths: `sounds/ui_navigate.wav`, `sounds/ui_confirm.wav`,
`sounds/pause_open.wav`, `sounds/pause_close.wav`,
`sounds/inventory_open.wav`, `sounds/inventory_close.wav`. Follow
stronger existing repository naming if present.

## Semantic-edge rule

Emit from successful existing semantic transitions, not raw key presses.
No false cue when selection/state does not change, an input is
blocked/consumed by a higher-priority state, or reconstruction merely
establishes state.

Preserve existing input priorities, including Inventory Esc, Pause,
destination hold, Run Complete, death blockers, and Development F2
behavior.

## Main Menu

Navigate emits once per genuine selection change. Confirm emits once
when PLAY or QUIT is activated. PLAY must preserve fresh-run/deferred
transition and Enter carry-through protection. QUIT preserves existing
Window close behavior. No mouse support.

## Pause

Navigate emits once per genuine selection change. Confirm emits once for
an activated selected Pause option. Pause Open emits once only when
Gameplay successfully enters Pause. Pause Close emits once only when
Pause successfully resumes Gameplay.

Esc direct resume emits Close only. RESUME activated with Enter emits
**UiConfirm plus PauseClose** exactly once, because both semantic
actions occurred (`PauseMenuInputAction::ActivateResume`). Direct
Pause -\> Main Menu emits Confirm but must not synthesize a false Pause
Close/Resume transition. `EnterMainMenu` reconstruction is silent.

## Inventory

Open/Close emit once on genuine Inventory state transitions only. Do not
alter contents, lifecycle, pause behavior, HUD visibility, or input
priority.

## Architecture

Integrate narrowly with the existing Application-owned
`platform::GameplayAudio` and semantic SFX request architecture from
M71--M74. Prefer semantic cue names `UiNavigate`, `UiConfirm`,
`PauseOpen`, `PauseClose`, `InventoryOpen`, `InventoryClose` unless
repository conventions are stronger.

Preserve audio device ownership, load-once lifetime, fixed current
volume behavior, safe missing-cue no-op, shutdown ordering, portability,
and staged-only Release loading.

Do not create a second audio owner, generic AudioEngine, event bus, UI
event framework, mixer, audio categories, components, or sound-bank
framework.

## Lifecycle/blocker safety

No synthetic UI cues from initialization, New Run, Restart, Play Again
reconstruction, Level transition, Apply/Reload, editor Open/Switch,
Physics rebuild, Checkpoint respawn, Health-death/fall/manual respawn,
F2 entry/exit, destination hold, or Run Complete reconstruction. No
catch-up playback after blockers. Existing one-shots need not be
forcibly stopped when paused.

## Authored-data safety

Do not change Level Format v1, LevelDefinition, workingCopy, specs,
Dirty state, editor layout, `level_01.level`, or `level_02.level`. M74
canonical content is baseline.

## Out of scope

No Settings/menu sliders, master/music/SFX/UI volume, mixer/categories,
mute, music, ambience, spatial/material audio, random banks, mouse menu
support, menu animation/redesign, input remapping,
graphics/accessibility settings, generic UI/GameState/SceneManager
frameworks, Pressure Plate authored audio controls, enemies/new gameplay
mechanics, editor productivity features, Level Format v2, or M76.

## Automated validation

Add/extend narrow regressions proving: 1. Main Menu genuine selection
change -\> one Navigate; no change -\> none. 2. PLAY activation -\> one
Confirm and unchanged fresh-run behavior. 3. Pause genuine selection
change -\> one Navigate. 4. successful Pause entry -\> one Open; blocked
entry -\> none. 5. Pause Esc resume -\> one Close. 6. Pause RESUME
follows documented Confirm/Close behavior exactly once. 7. Pause MAIN
MENU does not synthesize false Close. 8. Inventory successful open/close
-\> exactly one corresponding cue. 9. blocked/consumed Tab/Esc paths
produce no false Inventory cues. 10. F2/lifecycle/reconstruction produce
no phantom UI cues. 11. M71--M74 gameplay cues remain intact/distinct.
12. all new WAVs generate/cook/stage. 13. missing UI cues are safe
no-op. 14. Release has no source fallback. 15. no authored/Dirty
mutation. 16. canonical level semantics remain unchanged.

Prefer narrow helpers only when needed for testability; do not create a
generalized UI-state framework.

## Validation

Run directly affected C++ tests for GameFlow/Main
Menu/Pause/Inventory/Inventory UI/GameplayAudio/Level Transition/Run
Complete/Death-Respawn/AuthoredLifecycle/EditorWorkspace, plus relevant
M71--M74 audio regressions.

Run relevant current Python tests including:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

and newer audio regressions discovered in the repository.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
```

Canonical Level diffs must be semantically empty in M75.

## Manual acceptance

Verify Main Menu Navigate/PLAY confirmation; QUIT confirmation where
practical; Pause Open/Navigate/RESUME/ Esc-close/Main-Menu behavior;
Inventory Open/Close; no misleading blocked/F2 cues; all existing
M71--M74 gameplay sounds; complete Main Menu -\> level_01 -\> level_02
-\> Run Complete flow; and staged-only Release assets.

Automated green is not sufficient.

## Documentation

Canonical active document: `docs/milestones/MILESTONE_75.md`. Preserve
M73 and M74 as CLOSED. Keep `docs/MILESTONES.md` compact. Update
architecture/README/AGENTS only when actual implementation changes
require it. After implementation M75 is **implemented, awaiting manual
acceptance**, not CLOSED.

## Report and STOP

Report files changed; semantic UI edges used; Main Menu behavior; Pause
behavior; Inventory behavior; duplicate suppression/input priority; cue
paths/provenance; GameplayAudio integration and M71--M74 preservation;
lifecycle/F2 safety; authored/Dirty safety; Development/Release;
C++/Python/build results; `git diff --check`; canonical Level diff
confirmation; and confirmation no Settings/mixer/music/ambience/menu
redesign/general framework/M76 functionality was added.

Then STOP.

Do NOT commit.\
Do NOT push.\
Do NOT merge.\
Do NOT start M76.\
Do NOT mark M75 CLOSED.
