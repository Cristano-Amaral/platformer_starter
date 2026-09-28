# Milestone 115 — Death & Defeat Foundation

## Status
**IMPLEMENTATION VALIDATED — manual acceptance and Git closure pending**

## Goal
Build the first bounded runtime death/defeat layer on top of Milestone 114 Runtime Health. Connect health depletion to explicit lifecycle consequences without introducing combat, attacks, hit detection, AI combat, loot, ragdolls, death-animation authoring, generalized gameplay events, or a respawn framework.

The special Player reuses the existing production death/respawn flow. Valid NPC and Enemy instances gain a transient defeated state with deterministic minimal consequences.

## Architectural Context
M114 established reusable Runtime Health (`Maximum`, `Current`, `Depleted`), direct already-resolved Damage/Heal, Player Maximum from effective `MaxHealth`, NPC/Enemy Maximum from Character Definitions, lifecycle resets, and Development diagnostics. It intentionally did not make zero health cause death.

M114 manual acceptance exposed the expected transitional gap: Runtime Health can reach zero without entering Player death, while the legacy Hazard path still kills a normally healthy Player. M115 closes that gap.

## Scope

### 1. Explicit Runtime Life State
Introduce the smallest reusable representation needed to distinguish **Alive** and **Defeated**. It is transient runtime state; do not create a generalized state-machine framework.

Fresh valid runtime characters start Alive. Accepted runtime damage that transitions health from positive to zero transitions Alive → Defeated. The transition is deterministic and idempotent. Healing does not revive in M115.

### 2. Player Depletion → Existing Death Flow
For the special Player, accepted runtime damage that transitions health to zero must request the existing production Player death flow exactly once. Reuse the existing `PlayerDeath` / Application lifecycle; do not create a second death or respawn system. Existing death delay/transition behavior remains authoritative. After normal respawn/restart completion, Player health/life state return to fresh runtime values.

### 3. Preserve Legacy Hazard Death
Existing Hazard death must remain functional. The Player lifecycle must be coherent whether death originates from the legacy Hazard path or M114 Runtime Health depletion. Do not redesign Hazards into generalized damage volumes unless a tiny adapter is required by the current architecture. The M114 zero-health-but-playable transitional state must no longer persist.

### 4. NPC Defeat
For resolved Character Type `NPC`, reaching zero marks only that runtime instance Defeated, leaves health at zero, stops M112 patrol deterministically, and keeps the instance present and rendered. No despawn, death animation, ragdoll, collision redesign, loot, reward, or respawn timer.

### 5. Enemy Defeat
For resolved Character Type `Enemy`, reaching zero marks only that runtime instance Defeated, leaves health at zero, stops M113 patrol deterministically, and keeps the instance present and rendered. No despawn, death animation, ragdoll, collision redesign, loot, reward, or respawn timer. NPC and Enemy remain separately typed even if a small helper is shared.

### 6. Damage / Healing Contract
Preserve M114 validation and clamping. Defeat occurs only on Alive → zero. Damage to an already Defeated instance must not generate another defeat transition.

Healing remains finite/positive/clamped but must not revive. Inspect the current M114 API and choose/document the smallest deterministic behavior for Heal while Defeated: reject/ignore it, or allow numeric Current to change while life state remains Defeated. Prefer preservation of the existing M114 contract over architectural expansion.

### 7. Lifecycle Resets
Player New Run, full Restart, normal existing death→respawn completion, and real `PerformRespawn(Manual)` restore Alive/full health according to current production lifecycle.

NPC/Enemy editor Apply/runtime rebuild starts fresh instances Alive/full. Real `PerformRespawn(Manual)` restores Alive/full, authored origins, authored patrol behavior, stable counts, and no duplicates. Repeated resets remain stable.

### 8. Type Boundaries
M115 behavior applies only to special Player and resolved NPC/Enemy types. Other Character types remain presentation-only unless earlier milestones already govern them. Missing/unresolved definitions must not accidentally gain defeat behavior. Use typed contracts, not identity strings or placement order.

### 9. Development Diagnostics
Extend the existing M114 diagnostic UI. Expose identity/placement identity as currently available, Current/Maximum, Alive/Defeated, Damage, and Heal. No shipping health/death HUD system.

## Runtime Invariants
1. `0 <= CurrentHealth <= MaximumHealth`.
2. Maximum remains finite and positive.
3. Fresh/reset runtime characters start Alive/full.
4. One Alive→zero transition produces at most one defeat/death transition.
5. Defeated NPC/Enemy instances do not patrol.
6. Defeated NPC/Enemy instances remain present/rendered.
7. Healing does not revive.
8. Player depletion uses the existing Player death lifecycle.
9. Manual respawn restores Player/NPC/Enemy without duplication.
10. M114 equipment-driven MaxHealth behavior remains intact.
11. NPC/Enemy instances remain independent.
12. Legacy Hazard death remains functional.

## Persistence
Life/death state is runtime-only. Do not add current health/life state to `.level`, Character Definitions, per-placement overrides, save-game data, or file-format versions. Runtime defeat must not dirty authored content.

## Production-Boundary Regression Requirement
Tests must exercise real production boundaries, not only helpers. At minimum prove:
- Player damage through the real Application/runtime-health path reaches zero and enters existing death exactly once;
- real death/respawn restores Player health/life state;
- legacy Hazard death still works;
- real `PerformRespawn(Manual)` restores Player/NPC/Enemy health/life state;
- M112/M113 patrol reset behavior remains intact;
- no duplicate instances;
- one defeated NPC/Enemy does not affect another.

## Editor Authority
Preserve `workingCopy` → Apply → `active` → Save authority. Runtime health/life state is transient and must not mutate Character Placements or Character Definitions.

## Explicit Non-Goals
No Player/Enemy attacks, weapon execution, hitboxes/hurtboxes, collision combat damage, AttackPower/Defense formulas, critical hits, damage types, aggro/perception, chase/navigation, combat AI, target selection, knockback/stagger, invulnerability frames, death animations/events, ragdolls, corpse physics, despawn, NPC/Enemy respawn timers, resurrection, loot, XP/rewards, generalized event bus, generalized character state-machine framework, save-game persistence, or new level grammar.

## Required Validation
Follow current `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md` as authority.

Standard builds:
```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run relevant C++ tests plus focused coverage for life transition, Player depletion→death, idempotence, death/respawn restoration, Hazard regression, NPC/Enemy defeat, patrol stop, independent instances, manual respawn, repeated reset/no duplication, and M114 equipment MaxHealth regression.

Run repository Python validation suites as applicable:
```text
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

## Canonical Data Safety
M115 should require no semantic canonical level/gameplay-definition changes. Before reporting:
```text
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff -- game/assets/source/gameplay/definitions.gameplay
```
Remove test residue selectively. Do not normalize EOLs because of LF/CRLF warnings. Preserve the historical M45 removal of `dynamic_box 0 5 0 1 1 1 30`.

## Documentation
Update only materially affected architecture/runtime-health/death/NPC/Enemy/milestone documentation and future-agent invariants. Do not document future combat systems as implemented.

## Manual Acceptance — One Test at a Time
1. Player Runtime Health Damage to zero enters existing production death exactly once and Player does not remain playable at zero.
2. Existing death→respawn restores Player Alive/full; movement/jump/animation work.
3. Healthy Player Hazard death still works and restores Alive/full.
4. Patrol-enabled NPC damaged to zero becomes Defeated, stops, remains rendered.
5. Two patrol-enabled Enemies: defeat one; only it stops, other remains Alive/patrolling.
6. NPC + Enemy independence.
7. Real Gameplay manual respawn restores Player/NPC/Enemy Alive/full, authored origins/patrol, stable counts; repeat once.
8. M114 equipment MaxHealth regression using a temporary/known modifier if needed; remove any temporary authored modifier before closure.
9. General regression: Player movement/jump/Idle-Move-Jump, visible equipment, rendering/shadows, Alive patrol, no runtime-authored dirtiness.

## Completion Criteria
Implementation-complete only when the life state is bounded; Player depletion uses existing production death; the M114 zero-health-playable gap is closed; Hazard death remains functional; NPC/Enemy defeat independently stops patrol without despawn; reset boundaries restore state; production regressions exist; automated validation and manual acceptance pass; canonical asset diffs are clean unless justified.

The milestone is not CLOSED until explicit user approval and Git closure.

## Agent Stop Rule
After implementation/validation: summarize implementation, validation, limitations/unexpected repository facts, and relevant Git/diff status; then **STOP**. Do not commit, push, merge, start M116, or close the milestone unless explicitly instructed later.

## Implementation / Automated Validation

RuntimeHealth now latches Alive → Defeated on accepted positive-to-zero damage and rejects healing while Defeated. Reset restores Alive/full. Application direct damage and legacy Hazard damage use one existing PlayerDeath entry path; the original delay, respawn destination, audio and run semantics remain authoritative. NPC and Enemy actors stop patrol and present Idle at their last transform without despawn. F2 Player Damage is disabled because the existing editor freezes death delay and blocks exit during death; use the diagnostic in Gameplay.

CharacterPlacementTest exercises actual Application direct damage, legacy Hazard handling, delay completion, Manual/New Run/Restart health boundaries, equipment maximum synchronization, NPC/Enemy independence and repeated patrol reset; production Renderer checks keep defeated actors visible. PlayerCharacterStatsTest covers rejected defeated healing and Alive reset.

Validated Windows configure and Debug/Development/Release builds, all Development C++ test executables, focused health/character tests in Debug and Release, and all seven workflow Python suites. MSBuild required case-normalized process environment keys (duplicate PATH/Path); EditorToolRunnerTest required an outside-sandbox retry for child-process capture. Both retries passed. Canonical Level 01/02 and gameplay-definition assets remain unchanged. Manual acceptance above remains with the User; no Git closure performed.
