# Milestone 114 --- Health & Damage Foundation

## Status

**Implementation validated — manual acceptance and Git closure pending.**

## Goal

Introduce the first bounded runtime health and direct-damage layer for
the existing Player, NPC, and Enemy runtime actors, without introducing
combat, attacks, hit detection, death/despawn, perception, chase,
navigation, or broader AI.

M114 builds on:

-   M102 Character Stats & Player Definition Integration
-   M110 Character Instance Foundation
-   M111 Character Placement & Spawning
-   M112 NPC Runtime Foundation
-   M113 Enemy Runtime Foundation

The milestone establishes a small reusable health contract so later
combat milestones can consume one authoritative runtime representation
instead of inventing health independently.

## Core Architecture

``` text
CharacterDefinition / Player effective stats
        ↓
typed MaxHealth
        ↓
Runtime Character Health
        ↓
current / maximum health
        +
direct typed damage / healing operations
        ↓
Player / NPC / Enemy runtime diagnostics
```

Health is **transient runtime state**. It is not authored into Level
Format v1 and is not serialized by Save.

## Runtime Health Contract

Add a small reusable runtime health abstraction appropriate to the
repository's current architecture.

It must represent at minimum:

-   maximum health
-   current health
-   whether health is depleted (`current <= 0`)

Required invariants:

-   maximum health is finite and strictly positive at runtime
-   current health is finite
-   current health is clamped to `[0, maximum]`
-   newly created/reset health starts at maximum
-   damage cannot reduce current health below zero
-   healing cannot increase current health above maximum
-   invalid/non-finite/non-positive damage or healing inputs must be
    rejected or ignored safely and deterministically
-   zero health is a state only; M114 must not implement
    death/despawn/rewards/respawn-on-death

Use bounded numeric behavior consistent with existing gameplay-stat
conventions. Do not introduce a generalized attribute framework.

## MaxHealth Authority

### Player

The Player's maximum health must come from the existing M102 effective
`GameplayStatId::MaxHealth` value.

This preserves the existing stat pipeline:

``` text
CharacterDefinition base MaxHealth
        +
equipped item additive MaxHealth modifiers
        ↓
Player effective MaxHealth
        ↓
Player runtime health maximum
```

When the Player's effective MaxHealth changes at runtime because
equipment changes:

-   update the health maximum immediately
-   preserve current health when possible
-   clamp current health downward if the new maximum becomes lower
-   do not automatically refill health merely because maximum health
    increased

Use the existing M102 fallback/stat behavior as the authority rather
than creating a second Player-stat path.

### NPC and Enemy

For valid resolved NPC and Enemy placements, maximum health comes from
their resolved `CharacterDefinition` base `MaxHealth`.

M114 must not apply equipment modifiers to NPCs or Enemies.

If a definition does not provide a usable positive MaxHealth, use one
explicit bounded runtime fallback consistent with the repository's
current conventions and document it. Do not silently create multiple
category-specific fallback rules.

### Other Character placements

Generic M111 Character placements whose Character Type is not NPC or
Enemy remain presentation-only and do not gain a new gameplay-health
actor in M114.

The special Player remains on its existing Player runtime path.

## Damage Contract

Introduce a direct, typed runtime damage operation usable by later
systems.

M114 damage is an already-resolved health reduction amount. It does
**not** calculate attack damage.

Therefore M114 must **not** use:

-   AttackPower
-   Defense
-   equipment weapon properties
-   collision/hitboxes
-   attack ownership
-   teams/factions
-   invulnerability frames
-   critical hits
-   resistances
-   damage types
-   knockback

Those belong to later combat milestones.

A valid direct damage operation subtracts the requested amount from
current health and clamps at zero.

The implementation should expose enough result information for
callers/tests/diagnostics to know the before/after/applied result
without creating an event bus or generalized combat framework.

## Healing Contract

Provide the symmetric bounded direct-healing operation needed to
exercise and maintain the health abstraction.

Healing:

-   accepts an already-resolved positive amount
-   increases current health
-   clamps at maximum health
-   does not revive, respawn, or trigger any special state transition
-   does not introduce consumable-item use or inventory integration

## Runtime Ownership and Lifecycle

Health must follow the existing runtime ownership boundaries.

### Player

Player health is session-local runtime state.

At minimum:

-   New Run initializes health to current effective maximum
-   full Restart Run initializes health to current effective maximum
-   the existing real Gameplay manual respawn path (`R` /
    `PerformRespawn(Manual)`) resets Player health to maximum
-   ordinary rendering/physics rebuilds must not accidentally duplicate
    or corrupt health state

Preserve the existing M100/M102 inventory/equipment lifecycle rules
unless the current repository explicitly establishes a different
authoritative reset boundary.

### NPC and Enemy

Each valid NPC and Enemy runtime actor owns independent transient health
state.

-   two placements using the same CharacterDefinition must not share
    current health
-   Apply/rebuild from active authored level creates fresh health at
    maximum
-   real Gameplay manual respawn must restore NPC and Enemy health to
    maximum while preserving the M112/M113 patrol reset behavior
-   repeated respawn must not duplicate actors or health state
-   authored CharacterDefinition data must never be mutated by runtime
    damage

## Editor / Authoring

M114 does not add health fields to Level Format v1.

MaxHealth remains authored through the existing Character Definition /
Character Database stat system.

Do not add per-placement health overrides.

The Character Database may continue to expose MaxHealth through the
existing typed base-stat UI; only make narrowly necessary changes if the
current UI needs clarification for M114 behavior.

## Development Diagnostics / Manual Exercise Surface

Provide a bounded Development-only way to inspect and directly exercise
health without implementing combat.

Diagnostics must make it possible to identify at least:

-   Player current / maximum health
-   each NPC runtime actor current / maximum health
-   each Enemy runtime actor current / maximum health
-   depleted state
-   stable/session runtime identity sufficient to distinguish multiple
    actors

Provide a minimal Development-only direct test action for applying
damage and healing to selected/identified runtime actors if needed for
manual acceptance.

This surface is diagnostic tooling only. It must not become a combat UI,
gameplay HUD, console framework, or generalized debug-command system.

## Persistence

No Level Format v1 grammar change is required for runtime health.

Do not serialize:

-   current health
-   depleted state
-   runtime damage history

CharacterDefinition MaxHealth continues using the existing
gameplay-definition persistence established by M98/M101/M102.

No file-format version bump, GUID system, JSON migration, or new save
format.

## Rendering and Animation

M114 must not create a health-specific renderer.

Player, NPC, and Enemy presentation continues through the existing
rendering and CharacterInstance paths.

Reaching zero health must **not** automatically:

-   despawn
-   hide the character
-   stop patrol
-   play a death animation
-   ragdoll
-   disable collision
-   award rewards
-   restart the level

Those behaviors are explicitly deferred.

## Required Automated Coverage

Add focused regression coverage appropriate to the current repository.
At minimum cover:

1.  runtime health initialization at maximum
2.  direct damage and zero clamp
3.  direct healing and maximum clamp
4.  invalid/non-finite/non-positive operation handling
5.  depleted-state transition at zero
6.  independent health for two actors sharing one CharacterDefinition
7.  NPC and Enemy MaxHealth resolution from CharacterDefinition
8.  Player maximum health from the existing M102 effective-stat path
9.  Player equipment MaxHealth modifier changes update maximum
    immediately while preserving/clamping current health correctly
10. Apply/rebuild resets NPC/Enemy health from authored definitions
    without mutating authored data
11. real production Gameplay manual respawn (`PerformRespawn(Manual)`)
    restores Player, NPC, and Enemy health and preserves the M112/M113
    patrol-reset behavior
12. repeated manual respawn does not accumulate runtime actors or health
    state
13. canonical level/gameplay-definition safety

The manual-respawn regression must exercise the real Application
lifecycle boundary, not only call a lower-level health or character
rebuild helper. This requirement preserves the lesson from the M112
escaped regression.

## Manual Acceptance

Manual acceptance must verify, one test at a time:

-   Player health initializes correctly and can be damaged/healed
    through the Development diagnostic surface
-   damage clamps at zero and zero does not yet cause death/despawn
-   healing clamps at maximum
-   two Enemies using the same definition maintain independent current
    health
-   an NPC and Enemy maintain independent health while their M112/M113
    patrol behavior continues normally
-   real Gameplay `R` restores Player/NPC/Enemy health to maximum and
    still resets NPC/Enemy patrol state correctly, without duplication
-   Player MaxHealth equipment modifier behavior is immediate and does
    not incorrectly refill current health
-   normal Player movement, Idle/Move/Jump animation, equipment
    attachment, shadows, NPC patrol, and Enemy patrol remain functional

Temporary authored test data must be removed before Git closure.

## Canonical Data Safety

Before closure, audit:

``` text
game/assets/source/levels/level_01.level
game/assets/source/levels/level_02.level
game/assets/source/gameplay/definitions.gameplay
```

Do not retain temporary test placements or temporary gameplay
definitions.

If Editor Save materializes unrelated canonical `environment` /
`directional_light` lines in `level_01.level`, inspect and remove only
the known unintended lines when appropriate. Do not blindly restore the
whole level and do not normalize line endings.

The historical M45 removal of:

``` text
dynamic_box 0 5 0 1 1 1 30
```

must remain preserved.

## Documentation

Update the repository documentation required by the established
milestone workflow, including the milestone index/architecture/runtime
documentation where appropriate.

Document clearly:

-   health ownership
-   MaxHealth authority
-   direct damage/healing semantics
-   reset lifecycle
-   zero-health behavior
-   explicit boundary between M114 health/damage and later combat/death
    milestones

## Explicitly Out of Scope

M114 must not implement:

-   attacks or attack commands
-   melee/ranged combat
-   hitboxes/hurtboxes
-   collision-based damage
-   weapon damage
-   AttackPower/Defense combat formula
-   damage types/resistances
-   critical hits
-   knockback/stagger
-   invulnerability frames
-   Player combat input
-   Enemy attacks
-   NPC combat
-   target selection
-   perception
-   aggro
-   chase
-   AI state machines/behavior trees
-   navigation/pathfinding
-   death animation
-   character death/despawn
-   corpse/ragdoll
-   loot/rewards
-   checkpoint-on-death behavior
-   gameplay HUD/health bar
-   save-game persistence of current health
-   generalized ECS/component migration
-   generalized event bus
-   GUIDs
-   JSON migration
-   Level Format v2
-   M115+ functionality

## Deferred Existing Tunings

Do not absorb these into M114:

-   clearer Character workingCopy Ghost during Translate/Rotate/Scale
-   NPC/Enemy grounding/physics
-   Pause-menu Restart option / possible removal of direct Gameplay `R`
-   moving-platform transient reset behavior on `R`
-   graphical Gizmo in Character/Animation Preview
-   AI-assisted skeleton mapping
-   AI motion generation

## Validation

Follow `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md` as repository
authority.

Expected baseline validation includes the repository's normal Windows
Debug, Development, and Release builds plus the relevant C++ and Python
regression suites.

At minimum preserve the established Python checks:

``` powershell
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Run focused health/runtime tests and the production-boundary
Character/Application lifecycle regression introduced or extended by
M114.

Development startup must remain healthy.

## Agent Stop Rule

The implementation agent must stop after implementation, validation, and
report.

It must **not**:

-   commit
-   push
-   merge
-   start M115

Manual acceptance and Git closure remain user-controlled.

## Branch

``` text
milestone/114-health-damage-foundation
```

Creation command:

``` powershell
git switch -c milestone/114-health-damage-foundation
```


## Implementation Record

- `RuntimeHealth` owns private finite float current/maximum values. Typed `DirectDamage` / `DirectHealing` reject non-finite/non-positive amounts and report accepted, before, after and actual applied change. Damage clamps to zero; healing uses a widened intermediate and clamps to maximum.
- Application's existing Player health now uses this contract. Maximum comes from the existing M102 effective MaxHealth calculation, including equipment addends. M102's missing-stat zero base remains unchanged; only an unusable final effective maximum resolves to the shared runtime fallback. Maximum changes preserve current health, clamping downward without free refill.
- NPC and Enemy actors each own independent health initialized from resolved CharacterDefinition base MaxHealth. The one shared fallback is **100**, matching the existing legacy Player default. Equipment never contributes to placed actors. Other generic Character Types remain presentation-only.
- New Run/full Restart synchronize Player stats after existing equipment lifecycle rules and fill health. Real `PerformRespawn(Manual)` restores Player health and rebuilds fresh NPC/Enemy health while preserving the M112/M113 patrol reset-frame boundary. Actor counts remain stable over repeated respawn. Fall retains its existing narrower authority.
- Direct zero health is Depleted only, without movement/patrol/rendering/death consequences. Existing M69/M70 Hazard/HUD/vignette/hazard-triggered death behavior remains a legacy consumer, with float-compatible health adapters. No new combat, death, renderer, persistence, authoring fields, or future milestone work was added.
- Development F1 metrics hosts **M114 Runtime Health**, with current/maximum/depleted state, category/session handle/placement/definition identity, one amount field, and per-actor Damage/Heal buttons. It operates on active runtime actors.

Validation performed on Windows:

| Check | Result |
| --- | --- |
| `cmake --preset windows-vs2022` | Passed |
| `windows-debug`, `windows-development`, `windows-release` builds | Passed |
| All 73 Development C++ test executables | Passed; EditorToolRunnerTest required an outside-sandbox retry for child-process capture |
| PlayerCharacterStatsTest, PlayerHealthTest, CharacterPlacementTest in Debug and Release | All passed |
| Seven required Python suites | All passed: 98 tests total |
| Final Development startup smoke | Alive after 10 seconds; graceful shutdown exit 0; no error/warning diagnostics |
| `git diff --check` | Clean |
| Canonical Level 01/02 and gameplay definitions | No diffs; M45 dynamic-box removal preserved |

MSBuild validation used normalized environment-key casing to avoid duplicate PATH/Path entries, and parallel workers outside sandbox process restrictions. No repository workaround was added.

Manual acceptance remains pending using the checklist above, including visible zero-health movement/animation/patrol, diagnostics, repeated Gameplay R, equipment synchronization, attachments and shadows. No commit, push, merge, milestone closure or M115 work was performed.
