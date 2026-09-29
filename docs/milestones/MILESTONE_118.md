# Milestone 118 --- Attack Foundation

## Status

Implementation validated; manual acceptance and Git closure pending.

Branch: `milestone/118-attack-foundation`.

## Goal

Introduce the smallest reusable runtime **attack action** foundation for
Player, NPC, and Enemy characters, building on the existing Character
Definition, reusable animation, humanoid mapping/retargeting,
CharacterInstance, Health/Damage, Death/Defeat, Damage Feedback, and Hit
Reaction foundations.

This milestone defines **the act of attacking**. It does **not** yet
define spatial hit detection or weapon collision.

## Scope

### 1. Optional Attack animation semantic

Add the smallest optional Attack animation slot/semantic to the existing
character animation definition and editor workflow.

The slot must reuse the existing animation architecture: - embedded
clips where already supported; - reusable animation assets; -
compatibility validation; - humanoid mapping; - retargeting; - Character
Preview where appropriate.

Missing or invalid Attack animation assignments must degrade safely.

### 2. Transient runtime attack state

Add a minimal transient runtime attack state for character instances.

A valid attack request: - starts the configured Attack animation; -
remains active only for the bounded duration required by the resolved
animation/action; - returns to the previous normal runtime behavior when
complete; - must not create a generalized animation state machine,
montage system, action graph, ability system, or combat framework.

### 3. Player attack trigger

Provide the smallest explicit gameplay input needed to request a Player
attack.

The exact key/button should follow current repository conventions and
avoid conflicting with existing controls.

The input only requests the attack action. It must not perform damage
merely because the key was pressed.

### 4. NPC and Enemy runtime support

NPC and Enemy CharacterInstances must be able to execute the same
reusable attack action through a narrow runtime/API entry point suitable
for tests and later gameplay systems.

M118 does **not** add autonomous attack decisions, aggro, chase, target
selection, combat AI, or perception.

While a valid attack action is active: - NPC/Enemy patrol translation
pauses; - normal patrol resumes when the attack completes if the
character is still Alive; - Defeated characters cannot begin attacks.

### 5. Runtime priority and interruption

Preserve the existing lifecycle authority.

Required priorities: - Defeated/death behavior wins over Attack; -
accepted damage may still produce M116 Damage Feedback; - M117 Hit
Reaction interrupts/cancels an active nonlethal Attack when a valid
reaction resolves; - lifecycle reset/respawn clears Attack state; - no
duplicate or parallel attack state survives reset.

Do not introduce stagger, stun, poise, hit-stop, invulnerability, combo
logic, cancel windows, or attack buffering.

### 6. Presentation and diagnostics

Expose only the minimum diagnostics needed to verify: - attack animation
resolution; - Active/Inactive attack state; - remaining attack time if
the current runtime diagnostics use this pattern.

Do not turn Debug UI into a combat-authoring system.

### 7. Editor integration

Character Database must support authoring the optional Attack animation
with the same workingCopy / Apply / Save authority already used by
existing character animation semantics.

Character Preview should be able to preview the Attack assignment using
existing preview mechanisms where practical.

Do not add unrelated editor frameworks.

## Explicitly Out of Scope

M118 must **not** implement: - hitboxes or hurtboxes; - weapon
collision; - overlap/raycast attack detection; - automatic damage
application from attacks; - attack range evaluation; - attack
power/defense formulas; - combo chains; - light/heavy attacks; - charged
attacks; - directional attacks; - aerial attacks; - attack buffering or
cancel windows; - stamina/mana/resources; - cooldown systems; - aggro,
chase, perception, target selection, or combat AI; - knockback; -
stagger/stun/poise; - hit-stop; - invulnerability frames; - camera
shake; - particles; - damage numbers; - combat sound/rumble; - death
animations; - ragdoll/despawn; - loot/XP; - generalized animation state
machine/action graph/montage/ability framework; - save-game work; - M119
features.

## Data and Canonical Asset Safety

No canonical level or gameplay-definition semantic changes are expected
merely to implement M118.

Temporary animation assignments used for manual acceptance should use
Apply without Save whenever possible.

Before closure, explicitly audit: -
`game/assets/source/levels/level_01.level` -
`game/assets/source/levels/level_02.level` -
`game/assets/source/gameplay/definitions.gameplay`

Do not mechanically restore canonical files and do not normalize line
endings.

## Required Automated Coverage

Add focused regression coverage for at least:

1.  valid Player Attack starts and completes;
2.  missing Attack assignment safely falls back;
3.  incompatible Attack assignment safely falls back;
4.  Player attack input does not directly damage another character;
5.  valid NPC Attack pauses actual patrol movement and resumes
    afterward;
6.  valid Enemy Attack pauses actual patrol movement and resumes
    afterward;
7.  Defeated NPC/Enemy cannot start Attack;
8.  nonlethal valid Hit Reaction interrupts/cancels Attack;
9.  lethal damage/death/Defeat wins over Attack;
10. reset/respawn clears Attack and restores existing lifecycle
    behavior;
11. M116 Damage Feedback remains functional;
12. existing animation, placement, patrol, health, death/defeat, and
    hit-reaction regressions remain green.

Where patrol behavior is involved, test the production-facing behavior
(actual position/phase/rendered transform), not only internal flags.

## Validation

Run the repository-prescribed validation, including:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run all relevant C++ tests and the standard Python validation suites
required by the repository documentation.

Also run:

``` text
git diff --check
```

Report any process-only build workarounds separately from source
changes.

## Manual Acceptance

Manual acceptance is mandatory after automated validation.

At minimum verify: - Player with no Attack assignment remains safe; -
Player with a temporary compatible Attack assignment visibly attacks and
returns to normal locomotion; - attack input itself causes no damage; -
NPC and Enemy valid attacks freeze patrol translation and then resume
it; - Hit Reaction interrupts an active attack; - lethal damage/Defeat
takes priority over attack; - real reset/respawn clears transient attack
state; - no residual animation state, red tint, duplicate
CharacterInstance, or patrol regression remains.

Temporary authored test assignments must be restored before Git closure.

## Architectural Constraints

Prefer extension of the current character animation/runtime structures
over new generalized abstractions.

Keep Attack as a narrow, explicit transient character action.

Repository code, tests, current documentation, this milestone, and the
latest checkpoint remain the source-of-truth order.

No opportunistic ECS, GUID, prefab, undo/redo, file-format-version,
ability-system, or next-milestone work.

## Agent Stop Condition

After implementation and validation: - report changed files and
behavior; - report tests/builds run and results; - report canonical-file
audit; - report any manual acceptance setup still required; - STOP.

Do not commit, push, merge, or begin M119.
