# Milestone 71 — Gameplay Audio Foundation

**Status:** Implemented, awaiting manual acceptance.
**Branch:** `milestone/71-gameplay-audio-foundation`  
**Recommended Cursor model:** Grok 4.6 High — Fast OFF

## Goal

Establish a narrow reusable gameplay-SFX authority and use it for the survival loop completed by M69–M70. Add Release-capable cues for successful Hazard damage, player death, and Health-death respawn, while preserving/consolidating the existing M61 Item Pickup collection sound.

Reuse existing Health, Hazard, Death, Checkpoint/spawn, Game Flow, staged-runtime asset, raylib, and application-lifetime authorities. Do not create a generic AudioEngine, ResourceManager, event bus, mixer, Settings system, or music system.

## Required cues

- **Item Pickup:** preserve the M61 cue exactly once after successful collection; route ownership/playback through the M71 authority where practical without changing collection semantics.
- **Damage:** play exactly once when M69 actually applies positive Health damage, aligned with the M70 Damage Vignette. Never play for overlap-only/cooldown/blocked/no-op damage or Health already at zero.
- **Death:** play exactly once when M70 successfully enters the death phase; never replay per frame during `YOU DIED`.
- **Respawn:** play exactly once when the M70 Health-zero death respawn completes. Do not use it for Level load, Restart, Play/New Run, transitions, Apply/Reload, Open/Switch, or ordinary Fall/Manual R unless repository inspection establishes a compelling existing semantic requirement.

For a lethal hit, choose one deterministic rule: damage+death cues may both play if readable, or death may take precedence. Document and test the rule.

## Narrow audio authority

Introduce the smallest repository-consistent runtime owner/helper. It may be Application-owned `GameplayAudio`/`GameplayAudioState` or a narrow extension of an existing subsystem.

Responsibilities: own/load the small fixed gameplay sound set; expose semantic pickup/damage/death/respawn playback operations; safely unload owned resources; initialize/close the audio device only if not already owned elsewhere; tolerate individual missing/invalid optional cues as safe no-ops with one useful diagnostic.

Fixed gameplay SFX are application/runtime resources, not per-Level authored state. Fresh Play, Restart, Play Again, Level transition, death respawn, Apply/Reload, Physics rebuild, and editor Open/Switch must not unnecessarily reload them.

## Assets and staging

Use project-owned sounds following the existing M61/raylib audio format and asset conventions (prefer the established WAV path if applicable). Required semantics: existing pickup cue plus new damage, death, and respawn cues.

Do not download copyright-unclear third-party assets. If simple generated/project-owned effects are needed, keep them as ordinary fixed assets and document provenance; do not add an audio-generation framework.

Extend the existing narrow cook/stage audio mechanism. Runtime gameplay resolves staged assets only, with no `game/assets/source` fallback in Development or Release. No per-frame scans/I/O. Preserve unrelated staged assets and existing category cleanup rules.

## Missing/invalid audio

Audio is presentation, never gameplay authority. Failure to load one cue must log once/use existing diagnostic conventions, leave gameplay fully functional, and make that cue's playback a safe no-op. Do not turn missing SFX into Level-load failure or spam diagnostics every frame.

## Playback, pause and flow

Semantic events remain authoritative. Repeated Hazard damage may replay once per actual M69 damage tick, never per render frame.

Pause/Inventory/F2/MainMenu/LevelComplete/RunComplete already block relevant damage/events; do not invent a second audio pause/game-flow system. Do not snapshot audio into Pause or replay one-shots after Resume. Preserve M70 death/respawn exactly-once semantics.

Use one fixed centralized gameplay/SFX volume policy consistent with M61. Do not add user-facing volume/mute/settings, mixer buses, or persistence; those belong to M72.

## Music boundary

M71 is SFX foundation only. No background/menu/Level/transition music, playlists, crossfades, or dynamic music.

## Authored/Dirty and canonical safety

Audio must not mutate/serialize `LevelDefinition`, `workingCopy`, gameplay specs, editor layout, or Level files and must never mark Dirty. No semantic changes to canonical `level_01.level` or `level_02.level`.

## Portability

Development and Release must work from staged assets. Use raylib/current portable abstractions; no Windows-specific audio APIs. Preserve C++20/CMake and portability direction.

## Out of scope

No music, Settings UI, volume sliders, persisted mute/volume, mixer buses, spatial/3D audio, occlusion/reverb, streaming architecture, generic AudioEngine/ResourceManager/asset registry/event bus, authored per-object sound fields, footsteps, jump/land, Door/Plate/Goal/UI/Menu/completion sounds, enemies/combat, save/load, Level Format v2, or M72 functionality.

## Automated validation

Inspect existing tests first and cover at minimum: pickup exactly-once audio; no pickup cue on failed collection; damage exactly once per successful damage; no cue on cooldown/blocked/no-op; deterministic lethal-hit cue rule; death exactly once; no death replay while active; Health-death respawn exactly once; Fall/Manual R does not incorrectly use death-respawn cue; no spurious cues through Pause/Inventory/F2/flow; no unnecessary SFX reload across lifecycle operations; missing cue safe no-op; safe resource load/unload; staged required assets; no source fallback; no authored/Dirty mutation.

Run directly affected C++ regressions plus:

```text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Extend the existing pickup-sound staging test or add the narrowest gameplay-audio staging regression if current repository structure requires it.

Build:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check` and verify canonical Level files have no unintended semantic changes.

## Manual acceptance

Verify: pickup cue still exactly once; each real non-lethal damage tick has clear audio aligned with vignette; no per-frame spam; lethal-hit rule is coherent plus one death cue; `YOU DIED` does not replay; Health-death respawn has exactly one cue; normal spawn/Checkpoint/Health restoration remains correct; Fall preserves Health and does not incorrectly use death-respawn audio; Pause/Inventory/F2 cause no duplicates; transition/Restart/MainMenu→Play/PlayAgain remain coherent; missing-sound degradation where practical; essential pickup/damage/death/respawn audio works in Release.

Automated green is not sufficient.

## Documentation and STOP

Canonical active document: `docs/milestones/MILESTONE_71.md`. Preserve M70 as CLOSED, keep `docs/MILESTONES.md` compact, and update architecture/README/AGENTS only according to current conventions and actual implementation. After implementation M71 is **implemented, awaiting manual acceptance**, not CLOSED.

Cursor report must include files changed; exact audio authority/device lifetime; staged paths/provenance; pickup migration; damage/lethal/death/respawn trigger rules; volume policy; Pause/flow behavior; missing-asset behavior; cook/stage changes; authored safety; Development/Release; C++/Python/build results; `git diff --check`; canonical Level status; and confirmation that no generic audio/resource/event/mixer/settings/music system, Level Format v2, or M72 functionality was added.

Then STOP. Do not commit, push, merge, start M72, or mark M71 CLOSED.
