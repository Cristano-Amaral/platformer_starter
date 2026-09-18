# Milestone 72 --- Player Movement Audio Feedback

**Status:** CLOSED\
**Branch:** `milestone/72-player-movement-audio-feedback`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## Goal

Build directly on M71 Gameplay Audio Foundation with Release-capable
player movement SFX: footsteps, jump, and landing. Audio observes
existing movement/physics authority and must not change movement
behavior.

## Required cues

### Footsteps

Play only during active Gameplay while authoritatively grounded and
meaningfully moving horizontally. Use a deterministic cadence,
preferably fixed horizontal distance travelled; if invasive, a fixed
grounded-moving time cadence is acceptable. Never play every frame.
Stop/reset cleanly when stationary, airborne, blocked, dead, teleported,
respawned, transitioned, restarted, or runtime player is replaced. No
catch-up burst after blockers.

### Jump

Play exactly once when the existing controller actually accepts/performs
a jump, not merely on raw jump input. Rejected airborne/blocked input
emits nothing. Do not change jump physics.

### Landing

Play exactly once on a genuine airborne-to-grounded transition after a
meaningful fall. Use the narrowest fixed qualification threshold
available from existing motion authority (airborne duration, downward
velocity, or vertical displacement). Tiny contact jitter, startup/spawn,
teleports, respawns, rebuilds and Level transitions must not emit
landing audio. Do not add fall damage.

## Authority

Extend M71 `platform::GameplayAudio` narrowly with footstep/jump/landing
cues. A small transient `PlayerMovementSfxState`/tracker is acceptable
for cadence, previous grounded state and landing qualification. Do not
create a generic event bus, locomotion state machine, animation-event
system, AudioEngine, ECS audio component or parallel movement authority.

## Assets

Add project-owned/generated simple staged WAV cues following M71
conventions:

-   `sounds/player_footstep.wav`
-   `sounds/player_jump.wav`
-   `sounds/player_land.wav`

Exact names may follow stronger repository convention discovered during
inspection. Prefer the same PCM WAV conventions and generator approach
used by M71. No third-party/copyright-unclear assets and no source
fallback.

## Lifecycle and respawn

Preserve M70/M71 distinctions. Health death keeps Death SFX →
Health-death Respawn SFX. Fall respawn preserves Health and does not
become Health-death respawn.

All teleport/respawn/rebuild/runtime-replacement paths must
reset/re-anchor movement-audio tracking so they never synthesize
landing, jump, or a footstep burst.

Fixed GameplayAudio resources remain application/runtime-owned and must
not reload on Restart, Play, Level transition, death respawn,
Apply/Reload, physics rebuild or editor Open/Switch.

## Blocking

No new movement SFX while blocked by Pause, Inventory, F2/editor, Main
Menu, M70 death phase, destination LEVEL COMPLETE or RUN COMPLETE.
Tracking/cadence must not accumulate hidden progress that catches up on
resume. Existing one-shots need not be forcibly stopped merely because a
blocker opens.

## No gameplay changes

Do not change movement speed, acceleration, friction, jump
impulse/height, gravity, grounded semantics, collision dimensions,
respawn positions, Checkpoints, Hazard/Health, timer, Inventory, Goal,
Door, Plate or Box behavior. Narrow refactoring to expose an
already-authoritative jump/grounded transition is allowed only with
behavior-preserving regression coverage.

## Authored safety

Runtime presentation only. Never mutate `LevelDefinition`,
`workingCopy`, authored specs, editor layout or Level files; never mark
Dirty. No semantic changes to canonical `level_01.level` or
`level_02.level`.

## Release / portability / performance

Development and Release must work from staged assets only. No
ImGui/runtime source-tree dependency, Windows-specific audio API,
per-frame filesystem I/O or scanning. Tracking should be
constant-time/allocation-free per gameplay frame.

## Out of scope

No walk/run/sprint modes, stamina, crouch, double jump,
coyote-time/jump-buffer changes, movement tuning, fall damage,
surface/material detection, per-surface footsteps, randomized footstep
banks, animation sync, spatial audio, music, UI/Door/Plate/Goal/ambient
sounds, Settings, mixer buses, generic AudioEngine/event bus/locomotion
framework, authored audio fields, Level Format v2, or M73 functionality.

## Automated validation

Inspect current tests first. Add narrow coverage for:

1.  grounded meaningful movement emits footsteps at documented cadence;
2.  stationary/airborne player emits no footsteps;
3.  footsteps never emit every frame;
4.  no cadence catch-up after Pause/Inventory/F2/death;
5.  accepted jump emits exactly one jump request;
6.  rejected/airborne/blocked jump emits none;
7.  meaningful airborne-to-grounded transition emits one landing
    request;
8.  ordinary grounded frames/tiny jitter emit none;
9.  startup/spawn emits no landing;
10. Health-death respawn and Fall respawn synthesize no movement cue;
11. Restart/Play/Play Again/transition/Apply/Reload/Open/Switch/rebuild
    reset/re-anchor tracking;
12. MainMenu/LevelComplete/RunComplete emit no movement SFX;
13. all M71 Pickup/Damage/Death/Respawn cues remain unchanged;
14. new WAVs are cooked/staged;
15. missing cue is safe no-op;
16. Release has no source fallback;
17. authored/workingCopy/Dirty state remains unchanged.

Run affected player/controller, GameplayAudio, GameFlow/Pause,
PlayerHealth/Death, LevelTransition, PhysicsRebuild, Inventory, authored
lifecycle, EditorWorkspace and canonical safety tests.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Also run any M71 gameplay-audio generation/staging regression discovered
in the repository.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check` and verify canonical Level files have no
unintended semantic changes.

## Manual acceptance

Verify: stable footsteps while walking; silence while stationary;
exactly one jump cue per accepted jump; no airborne footstep spam;
exactly one landing cue after meaningful landing; no landing jitter
spam; no catch-up after Pause/Inventory/F2; Health-death keeps M71
Death/Respawn cues without accidental landing/footstep; Fall respawn
preserves established Health/audio semantics without synthesized
landing; M71 Pickup/Damage/Death/Respawn remain correct;
transition/Restart remain coherent; essential movement SFX work in
Release.

Automated green is not sufficient.

## Documentation

Canonical active document: `docs/milestones/MILESTONE_72.md`. Preserve
M71 as CLOSED and keep `docs/MILESTONES.md` compact. Update
architecture/README/AGENTS only for actual implementation. After
implementation M72 is **implemented, awaiting manual acceptance**, not
CLOSED.

## Cursor report and STOP

Report files changed; movement-audio tracker authority; exact footstep
cadence rule/constant; exact jump trigger; landing
qualification/threshold; reset/re-anchor lifecycle; new sound
paths/provenance; GameplayAudio integration; Health-death/Fall
interaction; blocker behavior; confirmation movement physics did not
change; missing-asset behavior; cook/stage changes; authored safety;
Development/Release behavior; C++/Python/build results;
`git diff --check`; canonical Level status; and confirmation that no
surface/material system, fall damage, generic AudioEngine/event
bus/locomotion framework, Level Format v2 or M73 functionality was
added.

Then STOP.

Do NOT commit.\
Do NOT push.\
Do NOT merge.\
Do NOT start M73.\
Do NOT mark M72 CLOSED.
