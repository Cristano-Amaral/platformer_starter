# Milestone 119 --- Melee Hit Detection Foundation

## Status

Implementation validated; manual acceptance and Git closure pending.

Branch: `milestone/119-melee-hit-detection-foundation`.

## Goal

Add the first reusable melee hit-detection layer on top of M118 Attack
Foundation.

M119 answers only: **while a valid melee Attack is active, which
eligible runtime character instances were contacted by that attack?**

It establishes attack windows, a spatial melee volume, deterministic
contact detection, and per-Attack multi-hit prevention. It does **not**
apply Damage. Damage integration remains M120.

The implementation must serve the current side-scroller without
unnecessarily encoding side-scroller-only assumptions. Detection should
use the engine's 3D runtime space so a future Free3D mode is not
structurally blocked.

## Foundations to reuse

Reuse the repository after M118, especially Character
Database/Definition authority, Character Placement, `CharacterInstance`,
NPC/Enemy runtime, Health/Damage/Defeat, M116 Damage Feedback, M117 Hit
Reaction, M118 Attack state/playback/priorities/reset behavior, runtime
transforms/facing, Character Database `workingCopy -> Apply -> active`,
diagnostics, and existing test patterns.

Repository code, tests, current docs, this milestone, and the latest
checkpoint remain authoritative according to `DEVELOPMENT_WORKFLOW.md`.

## Functional scope

### 1. Melee hit-detection definition

Add the smallest explicit data required to describe one melee contact
region for the current Attack action. The exact representation must
follow repository conventions after inspection, with semantics
equivalent to: - enabled/disabled melee detection; - a bounded active
window within the resolved Attack; - a local-space 3D volume relative to
the attacker; - finite validated dimensions/extents; - deterministic use
of attacker transform/orientation.

Prefer one simple primitive, such as an oriented box, if it fits
existing infrastructure. Do not create a generalized collision-shape
framework.

Older definitions without melee data remain valid and behave as having
no melee contact region.

### 2. Attack window

The melee volume is active only during an explicit portion of an active
M118 Attack. It must be derived deterministically from Attack
elapsed/duration state, bounded by the Attack, end immediately when
Attack is cancelled/cleared, and not depend on render frame rate for
semantic correctness.

Normalized Attack progress or an equivalent stable representation is
appropriate if consistent with current architecture.

Do not add animation notifies, montages, event tracks, combo windows, or
generalized action timelines.

### 3. Spatial melee volume

During the active window, compute the volume from the attacker's current
runtime world transform: - follows the runtime attacker; - respects
actual transform/facing authority; - operates in 3D world space, not a
hard-coded left/right lane; - shares core semantics across
Player/NPC/Enemy where applicable; - spawns no extra visible gameplay
character.

Prefer a gameplay query region rather than a new physical rigid body
unless existing architecture clearly makes that the narrowest correct
implementation.

### 4. Eligible targets

Detect contacts only against appropriate runtime character instances: -
attacker never contacts itself; - placed instances remain independently
identifiable; - multiple overlapping eligible instances produce
independent contacts; - missing/unresolved instances fail safely; -
Player and placed NPC/Enemy paths are supported according to M118
runtime capabilities.

Do not add teams, factions, hostility, friendly fire, aggro, perception,
or combat AI.

### 5. One contact per target per Attack

Maintain transient per-Attack contact memory: - first valid overlap
produces one contact; - continued overlap does not repeat; -
exit/re-entry during the same Attack does not repeat; - a later Attack
can contact that target again; - different targets can each be contacted
once; - completion/cancellation/reset clears transient contact state
correctly.

Do not introduce a generalized combat-event bus or persistent
relationship system.

### 6. Contact result --- no Damage

Expose a narrow runtime contact result/API suitable for diagnostics,
tests, and M120 consumption. Identify at minimum attacker and target
using existing runtime identity/ownership mechanisms.

A contact must **not** modify Health, call Damage, trigger M116
feedback, trigger M117 Hit Reaction, cause Defeat/death, apply
AttackPower/Defense, or cause knockback/combat effects.

M120 --- Combat Damage Integration owns the connection from contact to
Damage and combat formulas.

### 7. Character Database authoring

If melee fields belong in Character Definition, expose them through the
existing Character Database workflow: - edit `workingCopy`; - Apply
validates/promotes to active runtime without persistence; - Save
persists authored state; - Reload restores persisted state; - invalid
Apply preserves the last valid active state.

Do not redesign Item Database or add Apply to unrelated editors in M119.

### 8. Diagnostics / debug visualization

Expose enough diagnostics to manually verify resolution, active-window
state, contact count/last target, and invalid/unavailable reasons.

If narrow within existing renderer/debug infrastructure, add debug
visualization of the active melee volume. It must be debug/editor-only.
Do not build the broader combat-debugging suite reserved for M126.

## Runtime priorities and lifecycle

1.  No valid active Attack -\> no detection.
2.  Attack outside hit window -\> no contact.
3.  Attack inside hit window -\> query volume.
4.  First overlap with eligible target -\> one contact.
5.  Same target again in same Attack -\> ignored.
6.  Valid M117 Hit Reaction interrupt -\> hit processing ends
    immediately.
7.  Defeat/Player death -\> existing authority ends Attack/hit
    processing.
8.  Attack completion -\> processing ends; per-Attack contact memory
    resets for the next Attack.
9.  Manual respawn, fall respawn, New Run, Restart, runtime reset, and
    placed-instance rebuild cannot preserve stale hit state.

Preserve M118 semantics that repeated Attack requests while active are
ignored.

## Validation requirements

Add focused regressions and production-boundary coverage for: - backward
compatibility without melee data; - deterministic
parse/serialize/round-trip for new grammar; -
invalid/non-finite/out-of-range data; - Character Database Apply without
Save; - invalid Apply preserving active state; - active-window
boundaries; - no detection before/after window; - correct 3D
transform/orientation; - self-exclusion; - one contact per target per
Attack across multi-frame overlap; - exit/re-entry still only once; -
multiple targets each once; - later Attack can contact same target
again; - Hit Reaction interruption; - Defeat/death priority; -
reset/respawn/rebuild cleanup; - Player and placed CharacterInstance
production paths; - contact does not change Health or trigger Damage
Feedback/Hit Reaction; - stable runtime instance counts/no duplicate
spawning.

Production behavior regressions must exercise the real production
boundary rather than only helpers that could diverge from `Application`.

## Manual acceptance

Manual acceptance is mandatory. The implementation report must provide
setup to verify: 1. Attack without configured melee detection remains
safe. 2. Configured Player Attack exposes its hit window only at the
intended portion. 3. Target outside volume produces no contact. 4.
Target inside active volume produces one contact. 5. Continuous overlap
does not repeat during the same Attack. 6. A new Attack can contact the
same target again. 7. Multiple targets can each contact once. 8. Contact
alone leaves Health unchanged and produces no M116/M117 consequences. 9.
Hit Reaction/lethal interruption stops the window immediately. 10.
NPC/Enemy Attack uses the same spatial/contact foundation where M118
permits. 11. Real reset/respawn clears transient state without duplicate
instances. 12. Apply-only test authoring can be discarded without
permanent canonical changes.

The user performs manual acceptance and explicitly approves completion
before Git closure.

## Canonical data safety

Do not make semantic changes to canonical content merely to demonstrate
M119. In particular, permanent changes should not be required in: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level` -
`game/assets/source/gameplay/definitions.gameplay`

Use Apply without Save for temporary manual-test authoring where
practical. Audit canonical diffs before completion. Do not normalize
line endings incidentally.

## Explicitly out of scope

M119 does not include automatic Damage; AttackPower/Defense formulas;
weapon damage; contact-driven reactions; projectiles/ranged combat;
weapon collision simulation; generalized physics redesign;
teams/factions/hostility/friendly fire; combat
AI/aggro/chase/perception; navigation/pathfinding; target lock-on;
combos; light/heavy/charged/directional/aerial attacks; buffering; new
cancel systems; stamina/mana; cooldowns; knockback; stun/poise; hitstop;
iframes; camera shake; particles; damage numbers; combat audio/rumble;
death animation/ragdoll/despawn; rewards/drops/XP; save-game work;
Free3D player/camera; Open World systems; generalized
state-machine/montage/event-track frameworks; Item Database Apply
redesign; or M120+ work.

## Future-facing constraint, not future scope

The current game is a side-scrolling platformer, but M119 should avoid
encoding melee contact as a purely 1D left/right test when existing 3D
runtime transforms support a clean spatial query.

This does **not** request Free3D or Open World functionality. It only
avoids unnecessary side-scroller coupling in a generic engine
capability.

## Documentation

Update current documentation for the implemented M119 contract and
create:

`docs/M119_IMPLEMENTATION_REPORT.md`

The report must summarize architecture/hit-volume representation,
authored grammar/data, attack-window semantics, target identity and
de-duplication, diagnostics/debug visualization, tests and exact
validation results, canonical-data audit, manual acceptance setup,
changed files, and explicit confirmation that contact does not apply
Damage.

## Expected validation

Follow `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md`, including applicable
repository tests and:

``` powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release

python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Before reporting completion:

``` powershell
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff -- game/assets/source/gameplay/definitions.gameplay
git diff --stat
```

## Completion criteria

M119 is implementation-complete only when melee detection exists only
during a valid M118 Attack window; the region is spatially correct in
3D; eligible targets are independently identified; self-contact is
excluded; each target reports at most once per Attack; later Attacks can
contact again; interruption/reset/death lifecycle is correct; contacts
are observable; contacts do not apply Damage; Character Database
authoring preserves Apply/Save authority if used;
regressions/builds/tests pass; canonical data is clean; implementation
report is complete; manual acceptance passes; and the user explicitly
approves Git closure.

## Agent stop condition

After implementation and validation, the coding agent must write/update
`docs/M119_IMPLEMENTATION_REPORT.md`, report results/limitations/Git
summary, and **STOP**.

Do not commit, push, merge, start M120, or perform Git closure unless
explicitly instructed later by the user.
