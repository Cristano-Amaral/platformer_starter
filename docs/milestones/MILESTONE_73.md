# Milestone 73 --- World Interaction Audio Feedback

**Status:** Implemented, awaiting manual acceptance.\
**Branch:** `milestone/73-world-interaction-audio-feedback`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Extend the M71/M72 gameplay-audio foundation with semantic audio for
existing world interactions: Checkpoint activation, Pressure Plate
activate/deactivate, successful item-gated Door unlock, and Level Goal
completion. Audio observes existing authorities and must not change
gameplay, physics, progression, Level Format v1, authored data, or
lifecycle semantics.

## Required cues

-   **Checkpoint Activated:** exactly once on a genuine successful
    active-checkpoint change; no repeat while overlapping or from
    restoration/rebuild/respawn.
-   **Pressure Plate Activate:** exactly once on inactive -\> active.
-   **Pressure Plate Deactivate:** exactly once on active -\> inactive.
-   **Door Unlock:** exactly once when the existing successful
    item-gated unlock consumes the required item and changes the Door to
    unlocked. No cue for failed/missing-item/already-unlocked
    interaction or Plate-driven Door motion.
-   **Level Goal Complete:** exactly once when existing goal overlap
    first completes. Applies to destination goals and terminal goals; no
    replay during LEVEL COMPLETE hold, deferred transition, or RUN
    COMPLETE.

Emit from real semantic transitions/results, not renderer state,
proximity, raw input, or duplicated overlap logic. Narrow result
booleans/structs are acceptable if they expose existing authority
without changing behavior. No generic event bus, Trigger/Receiver
framework, interaction framework, ECS audio system, or generic
AudioEngine.

## GameplayAudio and assets

Extend existing `platform::GameplayAudio` narrowly and preserve Pickup,
Damage, Death, Health-death Respawn, Footstep, Jump, and Landing.

Add project-owned/generated staged PCM WAV effects following current
generator conventions: - `sounds/checkpoint_activate.wav` -
`sounds/pressure_plate_activate.wav` -
`sounds/pressure_plate_deactivate.wav` - `sounds/door_unlock.wav` -
`sounds/level_goal_complete.wav`

Exact names may follow a stronger repository convention. Extend the
current gameplay-SFX generator where practical. No
third-party/copyright-unclear assets. Preserve device lifetime,
load-once behavior, staged-only paths, safe missing-cue no-op, fixed
volume policy, portability and shutdown order.

## Reconstruction and lifecycle safety

Initialization, Play initialization, Play Again initialization, Restart
reconstruction, Level transition load, failed transition, Apply/Reload,
editor Open/Switch, physics rebuild, Checkpoint respawn, Health-death
respawn, Fall/Manual respawn, and Pause/Inventory/F2 entry/exit must not
synthesize or replay world-interaction SFX. Synchronize reconstructed
state silently where needed. No catch-up after blockers.

For destination goals, request completion audio when completion is first
accepted, before/alongside the existing LEVEL COMPLETE hold, without
changing hold duration or Enter behavior. For terminal goals, request it
when terminal completion is first accepted without changing final-time
capture or Run Complete.

Keep distinctions explicit: Plate edge sounds belong to Plate state;
Door Unlock belongs only to successful item-gated unlock; Plate-driven
Door kinematic motion gets no new Door movement sound in M73.

## No gameplay/authored changes

Do not change Checkpoint rules, Plate overlap flags, Door
required-item/inventory semantics, Door movement, Goal
overlap/completion, transition hold, Run Complete, timer, player
movement, Health/death/respawn, pickups, hazards, boxes, or interaction
priority. No Level Format changes or authored audio fields. Never mutate
`LevelDefinition`, `workingCopy`, specs, editor layout or Dirty state.
No semantic changes to canonical `level_01.level` or `level_02.level`.

## Release / portability

Development and Release use staged assets only. No source fallback,
Win32 audio API, ImGui runtime authority, per-frame filesystem I/O, or
source scanning.

## Out of scope

No Door open/close/motor loops, Hazard/collectible ambience,
surface/material audio, spatial audio, attenuation/occlusion/reverb,
music, UI/menu navigation sounds, ambient emitters, authored sound
selection, Settings/volume/mute, mixer buses, randomized banks, generic
AudioEngine/event bus, Trigger/Receiver/Event/Requirement frameworks,
ECS, Level Format v2, or M74 functionality.

## Automated validation

Inspect current tests and add narrow coverage for: genuine Checkpoint
activation exactly once; no continued-overlap repeat; Plate
activate/deactivate edges exactly once and no held-state spam;
successful Door unlock exactly once and no
failed/already-unlocked/Plate-motion false cue; destination and terminal
Goal completion exactly once with no hold/results/transition replay; no
reconstruction/respawn/blocker spurious cues; M71/M72 cues unchanged;
new WAVs cooked/staged; missing cue safe no-op; Release no source
fallback; authored/Dirty unchanged; canonical Levels unchanged.

Run affected Checkpoint, Pressure Plate, Door/Locked Door, Inventory,
Level Goal/Transition, GameFlow, GameplayAudio, PhysicsRebuild, authored
lifecycle, EditorWorkspace and canonical safety C++ tests.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Also run current gameplay-audio generation/staging regressions
discovered in the repository.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check` and verify both canonical Level files have no
unintended semantic changes.

## Manual acceptance

Verify: one Checkpoint cue with no overlap spam; distinct Plate
activate/deactivate cues with no held spam; one successful Door Unlock
cue and no replay/false cue from Plate movement; one destination Goal
completion cue with no hold/transition replay; one terminal Goal cue
with no Run Complete replay; no spurious sounds from
Restart/respawn/transition/Apply/Reload/F2; all M71/M72 cues remain
functional; essential M73 cues work in Release.

Automated green is not sufficient.

## Documentation

Canonical active document: `docs/milestones/MILESTONE_73.md`. Preserve
M72 as CLOSED and keep `docs/MILESTONES.md` compact. After
implementation M73 is **implemented, awaiting manual acceptance**, not
CLOSED.

## Cursor report and STOP

Report: files changed; semantic audio authority; Checkpoint trigger;
Plate edge triggers; Door Unlock trigger; destination/terminal Goal
trigger and ordering; reconstruction/lifecycle suppression; sound
paths/provenance; GameplayAudio integration/M71-M72 preservation;
blockers; confirmation gameplay semantics unchanged; missing assets;
cook/stage; authored safety; Development/Release; C++/Python/build
results; `git diff --check`; canonical Level status; and confirmation no
Door motor audio, spatial audio, music, generic AudioEngine/event
bus/Trigger-Receiver framework, Level Format v2, or M74 functionality
was added.

Then STOP.

Do NOT commit.\
Do NOT push.\
Do NOT merge.\
Do NOT start M74.\
Do NOT mark M73 CLOSED.
