# Milestone 116 — Damage Feedback Foundation

## Status
Implementation validated — manual acceptance and Git closure pending.

## Goal
Add the first bounded, presentation-only damage feedback layer for runtime characters, building directly on the Health/Damage and Death/Defeat foundations from Milestones 114–115.

The milestone makes accepted damage visually readable without introducing combat, attacks, hitboxes, death presentation, or new authored gameplay data.

## Scope

### 1. Damage feedback event/state
- Add the smallest transient runtime mechanism needed to know that a supported character has just accepted positive damage.
- Feedback is triggered only when damage is actually accepted by Runtime Health.
- Healing, rejected damage, zero/invalid damage, equipment MaxHealth changes, reset/rebuild, and unrelated state changes must not trigger hit feedback.
- Repeated accepted damage restarts or refreshes the feedback deterministically; it must not create accumulating permanent state.

### 2. Player hit flash
- When the special Player accepts damage, briefly tint/flash the rendered character red and then return automatically to its normal rendering.
- The flash is presentation-only and must not alter Health, movement, animation selection, death timing, equipment, stats, or authored data.
- Damage that depletes the Player may begin the flash, but the existing M115 death/respawn lifecycle remains authoritative.

### 3. NPC hit flash
- Resolved CharacterType NPC instances briefly flash red when they accept damage.
- Feedback is per CharacterInstance: damaging one NPC must not affect another character.
- An NPC that reaches zero still follows M115 behavior: Defeated, patrol stopped, remains present/rendered. This milestone does not add a defeated pose or death animation.

### 4. Enemy hit flash
- Resolved CharacterType Enemy instances briefly flash red when they accept damage.
- Feedback is independent per instance.
- An Enemy that reaches zero still follows M115 behavior: Defeated, patrol stopped, remains present/rendered.

### 5. Rendering integration
- Reuse the existing character rendering/material path and introduce only the minimum runtime override/tint mechanism necessary for the temporary red flash.
- Preserve the character/model's normal appearance outside the feedback window.
- The implementation must work with the existing Player, CharacterInstance, skeletal animation, retargeting, and visible equipment paths without redesigning them.
- Avoid a generalized effects, material-instance, event-bus, combat-feedback, or animation-state framework unless the current repository already has an appropriate minimal reusable primitive.

### 6. Lifecycle and reset behavior
- Feedback state is transient and never serialized.
- New Run, full Restart, existing Player death/respawn completion, real `PerformRespawn(Manual)` / Gameplay `R`, Editor Apply/rebuild, and character instance recreation must clear any active damage flash.
- These boundaries must not duplicate characters or alter authored patrol/origin behavior.

### 7. Development diagnostics
- Extend the existing Runtime Health Development diagnostics only as needed to exercise and inspect the feature.
- Existing Damage controls remain the primary manual trigger.
- Do not introduce authored damage-feedback settings in this milestone.

## Behavioral Contract
- Accepted positive damage -> Health changes according to M114 and a short red visual flash is triggered for that exact runtime character.
- Flash expires automatically -> original visual appearance is restored.
- A second accepted hit during an active flash deterministically refreshes/restarts the feedback window.
- Damage to character A never flashes character B.
- Rejected/non-damage operations never flash a character.
- Reaching zero continues to use M115 life-state semantics; this milestone adds no new death semantics.

## Required Automated Coverage
Add focused regression coverage appropriate to the repository architecture, including production-boundary coverage where rendering/runtime integration requires it:

1. Accepted damage starts transient feedback for Player.
2. Feedback expires and restores the normal visual state.
3. Repeated accepted damage refreshes/restarts feedback deterministically.
4. Heal and rejected/non-positive damage do not trigger feedback.
5. NPC and Enemy feedback is independent per CharacterInstance.
6. Damage/defeat behavior from M114–M115 remains unchanged.
7. Real reset/rebuild boundaries clear active feedback.
8. Real `PerformRespawn(Manual)` restores stable Player/NPC/Enemy runtime state with no duplicate instances.
9. Existing Hazard death/respawn remains functional.

Tests should validate the underlying production state/path rather than depending only on Debug UI helpers.

## Manual Acceptance
Perform one test at a time after automated validation:

1. Damage a healthy Player once: Health decreases and the Player briefly flashes red, then returns to normal.
2. Damage the Player repeatedly: each accepted hit produces/refreshed feedback without leaving a permanent tint.
3. Damage an NPC: only that NPC flashes; patrol/life behavior remains otherwise unchanged.
4. Damage one of two Enemies: only the damaged Enemy flashes; the other remains visually unchanged.
5. Defeat an NPC and an Enemy: the final damaging hit may flash, then M115 Defeated behavior remains authoritative (0 Health, stopped patrol, still rendered).
6. Verify healthy Player Hazard death/respawn regression.
7. Trigger active feedback and use real Gameplay `R`: Player/NPC/Enemy return with normal appearance, Alive/full Health, authored origin/patrol, and stable instance counts.
8. General gameplay regression: movement, jump, animation, checkpoint, equipment rendering, and character rendering remain normal.

## Explicitly Out of Scope
- Player/NPC/Enemy attacks.
- Combat input or attack commands.
- Hitboxes, hurtboxes, collision-based combat, or weapon collision.
- Damage formulas or attack-vs-defense resolution.
- Aggro, chase, combat AI, targeting, navigation, or pathfinding.
- Knockback, stagger, stun, hit-stop, camera shake, screen shake, particles, decals, floating damage numbers, sound effects, or controller rumble.
- Hit-reaction animations or animation-state redesign.
- Death animations, defeated poses, ragdolls, despawn, respawn timers, resurrection, loot, or XP.
- Invulnerability frames or damage cooldowns.
- Authored flash color/duration or new level/gameplay grammar.
- Generalized effects/material-instance/event-bus frameworks.
- Save-game persistence.

## Canonical Data Safety
The milestone must not intentionally modify canonical authored content:
- `game/assets/source/levels/level_01.level`
- `game/assets/source/levels/level_02.level`
- `game/assets/source/gameplay/definitions.gameplay`

Any temporary manual-test residue must be audited and selectively removed before closure. Do not normalize line endings merely because Git reports LF/CRLF warnings.

## Documentation
Update current repository documentation required by the existing milestone/documentation conventions, including milestone indexes/status and architecture descriptions where the new transient damage-feedback responsibility belongs.

## Validation
At minimum, follow the repository's current authoritative workflow and run the relevant configured Windows builds, focused C++ tests, Development test suite, required Python repository suites, and `git diff --check`.

The implementation agent must inspect `AGENTS.md`, `DEVELOPMENT_WORKFLOW.md`, current code/tests/docs, and Milestones 114–115 before implementation; repository state is authoritative if this definition conflicts with stale historical documentation.

## Completion Gate
Implementation/report -> critical review -> manual acceptance -> canonical-data audit -> explicit user approval -> Git closure.

The implementation agent must STOP after implementation, validation, and report. No commit, push, merge, branch switch, or Milestone 117 work without explicit instruction.

## Branch
`milestone/116-damage-feedback-foundation`

## Implementation Record

RuntimeHealth carries a bounded 0.25-second countdown restarted by positive actual damage reduction only. Runtime character draws read that instance's health and apply a temporary red material tint through the existing material-preserving path. Fallback primitives also tint; equipment attachments retain normal presentation. Timers follow simulation pause; Player final-hit feedback expires during the existing death delay. Reset and instance recreation clear transient feedback without serialization or new death semantics.

Production regressions cover Player damage/presentation, independent NPC/Enemy bindings, exact rendered appearance restoration, active-feedback Manual respawn, Editor Apply, New Run/Restart, M114 equipment maximum behavior, M115 defeat/stopped patrol and Hazard death completion. Existing Damage buttons remain authoritative consumers of runtime damage; diagnostics also show remaining feedback time.

Validation: Windows configure and Debug/Development/Release builds; all 73 Development C++ executables (EditorToolRunnerTest passed on an outside-sandbox retry); focused health and character tests; seven workflow Python suites (98 tests); canonical-data audit and diff whitespace check. MSBuild required process environment key normalization for duplicate PATH/Path. No repository build workaround or canonical authored-data change was introduced. Manual acceptance remains pending; no Git closure performed.
