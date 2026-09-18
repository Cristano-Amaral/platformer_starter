# Milestone 74 --- Gameplay / Level Polish Pass

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/74-gameplay-level-polish-pass`\
**Cursor:** Grok 4.6 High --- Fast OFF

## Goal

Evaluate and polish the canonical player-facing run
`Main Menu -> level_01 -> level_02 -> Run Complete` using accumulated
systems as a complete game experience. Improve clarity, pacing,
feedback, and coherence using existing systems first. M74 is not a
feature bundle or engine redesign.

## Required gap: Collectible collection SFX

`Collectible` from the Object Palette is distinct from `Item Pickup`.
Add one clear SFX exactly once on genuine successful Collectible
collection. No
proximity/per-frame/reconstruction/restart/respawn/transition/Apply/Reload/Open/Switch/rebuild/blocker
false cues. Preserve Collectible count/progression semantics. Integrate
narrowly with M71--M73 GameplayAudio, using a project-generated staged
WAV and no authored audio field or generic framework.

## Canonical run polish

Review both canonical Levels for spawn/route clarity, progression, Goal
placement, Checkpoint usefulness, Hazard readability, Door/Pressure
Plate relationship, Item Pickup/Collectible discoverability,
confusing/dead space, pacing, and continuity between level_01 and
level_02.

Intentional semantic edits to `level_01.level` and `level_02.level` are
allowed when they improve the run. Use existing supported
objects/properties. Preserve a baseline, explain every intentional
change, avoid whole-file/EOL churn, and keep Level Format v1
parser/editor-writable.

Manual inspection found the new `level_02` Hazard authored at
`7.5 0.7 0` with size `1.2 1 2`, which buried the damage AABB below the
raised floor so a standing player never entered
`FindHazardIndexContaining`. The correction raises only that Hazard
center Y to `1` so the volume sits on the traversable floor. Runtime
Hazard/Health/Death semantics are unchanged.

## Feedback review

Review Collectible, Item Pickup, Checkpoint, Pressure Plate, Door
unlock, Hazard damage, Death/Respawn, Goal, HUD, Pause/Inventory/Main
Menu flow, Footstep/Jump/Landing. Fix only narrow inconsistencies
supported by the existing authorities.

Small tuning of an existing generated WAV is allowed only if clearly
useful; do not redesign audio.

## Diagnostic-only observations

Evaluate, but do not implement in M74, missing UI feedback for Main
Menu/Pause navigation and confirmation, Pause open/close, and Inventory
open/close. Record whether this justifies a focused future UI Audio
milestone.

A hidden Pressure Plate may still emit M73 semantic audio. Do not
reinterpret `visible=false` as `silent` and do not add authored Plate
audio control in M74. Record whether independent authored control is
genuinely useful.

## Preserve

Preserve established Level v1, editor workingCopy/Apply/Save authority,
Inventory, Item Pickup, Collectible, Checkpoint, Plate, Door,
Goal/transition/Run Complete, Main Menu, Pause/input priority, HUD,
Health/Damage/Death/Respawn, M61--M73 feedback/audio, and staged-only
Release behavior unless fixing a demonstrated bug.

## Out of scope

No ECS, generic GameState/SceneManager, generic AudioEngine/event bus,
Trigger/Receiver framework, Settings/config framework, save/progression
framework, quest/tutorial framework, animation system,
material/surface/spatial audio, music/ambient system, UI navigation
audio, input remapping, graphics settings, Undo/Redo, multi-selection,
prefab/GUID migration, Level Format v2, or M75 functionality. Document
demonstrated needs as future candidates instead.

## Collectible audio asset

Prefer `sounds/collectible_collect.wav` unless current repository naming
gives a stronger convention. Extend the existing project-owned
gameplay-SFX generator. Preserve load-once/device lifetime/fixed
volume/staged-only/missing-cue safe no-op/shutdown semantics. Do not
reload on lifecycle operations.

## Validation

Add/extend narrow regressions proving Collectible success emits exactly
once; continued frames/proximity emit none; reconstruction/lifecycle
emit none; Item Pickup collection sound remains distinct; M71--M73 cues
remain intact; Collectible WAV is generated/cooked/staged; missing cue
is safe; Release has no source fallback; audio does not mutate
authored/Dirty; intentional canonical edits parse and remain
editor-writable.

Run directly affected C++ regressions for Level
parsing/writing/canonical scenes, GameFlow, Collectible, Item Pickup,
Checkpoint, Plate, Door, Goal, Health/Death, GameplayAudio,
PhysicsRebuild, AuthoredLifecycle, EditorWorkspace.

Run relevant current Python tests, including:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

and newer gameplay-audio/Collectible regressions discovered in the
repository.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check`. Inspect canonical Level semantic diffs
carefully; do not mechanically restore intentional M74 edits or
normalize EOLs.

## Manual acceptance

Complete a fresh Main Menu -\> level_01 -\> level_02 -\> Run Complete
playthrough. Verify route/pacing/readability, transition/final
completion, intentional authored polish, Collectible SFX exactly once,
Item Pickup distinct SFX, M73 sounds, movement/damage/death audio,
Pause/Inventory/Restart/respawn/Play Again/Main Menu paths, editor save
safety, and Release staged-only run.

Record qualitative observations for M75: UI navigation/confirmation
audio; music/ambience; Settings justification; content length/variety;
editor productivity; new gameplay mechanic need; hidden-Plate
independent audio control.

Automated green is not sufficient.

## Deliverable

Finish with a more coherent canonical run plus an evidence-based
`M75 Candidates` report section. Do not implement those candidates.

## Documentation

Canonical: `docs/milestones/MILESTONE_74.md`. Preserve M72 and M73 as
CLOSED. Keep `docs/MILESTONES.md` compact. After implementation M74 is
**implemented, awaiting manual acceptance**, not CLOSED.

## Report and STOP

Report files changed; initial run observations; every intentional
level_01 and level_02 semantic change/rationale; Collectible
authority/SFX trigger and asset provenance; GameplayAudio
integration/M71--M73 preservation; readability/pacing/presentation
changes; lifecycle safety; authored/Dirty safety; Development/Release;
C++/Python/build results; `git diff --check`; canonical Level diff
summary; confirmation no out-of-scope framework/features; and
`M75 Candidates` with concrete observed bottlenecks.

Then STOP.

Do NOT commit.\
Do NOT push.\
Do NOT merge.\
Do NOT start M75.\
Do NOT mark M74 CLOSED.
