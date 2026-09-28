# Milestone 113 --- Enemy Runtime Foundation

## Status

DEFINED --- implementation not started.

## Goal

Build the first bounded Enemy runtime layer on top of M111 Character
Placement & Spawning and M112 NPC Runtime Foundation.

M113 promotes authored Character placements whose resolved
`CharacterDefinition::CharacterType` is `Enemy` into transient Enemy
runtime actors. It establishes Enemy identity, lifecycle, deterministic
non-combat patrol/state behavior, CharacterInstance presentation, and
Development diagnostics without introducing health/damage, attacks,
combat, perception/aggro, navigation/pathfinding, behavior trees, or
generalized AI frameworks.

The purpose is to establish the correct Enemy runtime
authority/lifecycle boundary before later milestones add health, combat,
and AI.

## Architecture

``` text
CharacterDefinition (Character Type = Enemy)
        +
Level Character placement (M111)
        +
bounded patrol settings
        ↓
active level
        ↓
Enemy runtime actor
        ↓
M110 CharacterInstance
        ↓
Idle / Move presentation
        ↓
Exact / Retargeted / Static
```

Authority: - `CharacterDefinition` owns shared authored metadata and
presentation. - Level Character placement owns persistent transform and
bounded patrol configuration. - Enemy runtime state is
transient/session-local. - `CharacterInstance` remains
presentation/animation authority. - Runtime handles/state are never
serialized. - Player remains special and is not migrated. - M112 NPC
runtime remains independent and unchanged.

## Scope

### 1. Enemy Runtime Actor

Add a bounded transient Enemy runtime representation for eligible active
Character placements. Track at least a session-local Enemy handle,
source placement, Character identity, associated CharacterInstance,
authored origin transform, current runtime transform/position, Idle/Move
state, deterministic patrol direction/state, and diagnostics.

### 2. Character-Type Gating

Enemy behavior requires a valid resolved Character identity with
Character Type Enemy. Player, NPC, and Animal must not receive M113
Enemy behavior. NPC placements continue through M112.
`characters/player` remains only a generic visual placement and never
gains Player gameplay ownership.

### 3. Bounded Enemy Patrol

Provide minimal deterministic movement before perception/combat exists.
Reuse M112 patrol semantics/data where practical rather than creating a
parallel locomotion model.

Settings: - Patrol Enabled - Patrol Distance - Patrol Speed

Semantics: - authored position is the patrol origin; - patrol follows
local X projected onto the horizontal plane; - endpoints are origin ±
distance; - deterministic reversal; - disabled = authored origin /
Idle; - enabled movement = Move; - no root motion; - runtime movement
never rewrites authored placement.

### 4. Level Format v1

Prefer no new grammar if the M112 Character patrol extension is
sufficient:

``` text
character <px> <py> <pz> <rx> <ry> <rz> <sx> <sy> <sz> characters/<name> [npc_patrol <0|1> <distance> <speed>]
```

Despite the historical `npc_patrol` token, M113 may reuse this payload
for Enemy placements if that is the smallest backward-compatible
solution. Do not rename/migrate M112 records merely for terminology.

M111/M112 records remain compatible; deterministic round-trip; no
version bump, JSON, GUID, ECS, sidecar behavior format, or runtime
serialization.

### 5. Level Editor

For resolved Enemy definitions expose a compact Enemy Runtime section
with Patrol Enabled, Distance, Speed, and local-horizontal-X
explanation. Preserve M112 NPC Runtime UI for NPCs. Player/Animal must
not imply active NPC/Enemy behavior.

Preserve `workingCopy → Apply → active → Save`. Working-copy changes
must not affect active Enemy runtime before Apply. Save writes authored
state only. Reopening Development/loading the level rebuilds transient
Enemy state.

### 6. Runtime Lifecycle

Integrate at established M111/M112 boundaries: - one eligible Enemy
placement creates exactly one Enemy runtime actor and corresponding
CharacterInstance; - stale state is destroyed before replacement; -
repeated Apply/rebuild does not accumulate; - deleting a placement
removes its actor; - identity/type changes update eligibility at
authoritative promotion; - Gameplay manual respawn (`R` / production
`PerformRespawn(Manual)` path corrected in M112) resets Enemy transient
state to authored origin; - full restart/reload rebuilds from active
authored data; - handles never serialize.

Do not regress the M112 real Gameplay `R` fix.

### 7. Movement and Presentation

Horizontal deterministic patrol only. No navmesh, pathfinding, obstacle
avoidance, gravity, grounding, terrain intelligence, generalized
Character Controller, chase, perception, attacks, or targeting.

CharacterInstance: stationary → Idle; patrolling → Move. Reuse Exact,
Retargeted, Static, Unavailable-safe presentation, materials, skinning,
and directional shadows. No Enemy-specific renderer.

### 8. Orientation

Enemy faces current horizontal travel direction using transient runtime
orientation derived from authored rotation. Reversal changes runtime
facing only. Never rewrite authored rotation. No generalized steering,
turn-rate framework, root motion, IK, or animation graph.

### 9. Development Diagnostics

Add `M113 Enemy Runtime` diagnostics showing Enemy handle, source
placement, identity/type, patrol state, authored origin, runtime
position, direction/endpoints, Idle/Move, CharacterInstance handle, and
presentation status. M112 NPC diagnostics remain functional.

### 10. Production Manual Respawn Boundary

M112 exposed an escaped-test gap where direct rebuild coverage did not
exercise the real Gameplay R path. M113 must use the corrected
production boundary. Regression must prove Manual Respawn resets Enemy
runtime and preserves corrected NPC behavior, without accumulation or
authored mutation.

Do not change moving-platform respawn semantics.

## Automated Tests

At minimum cover: 1. Enemy type creates one Enemy actor. 2.
Player/NPC/Animal do not receive Enemy behavior. 3. NPC continues
through M112. 4. Missing/malformed references are safe. 5. Disabled
patrol = authored origin/Idle. 6. Enabled patrol = deterministic
movement/Move. 7. Deterministic reversal without authored mutation. 8.
Transient facing without authored rotation mutation. 9. Two placements
sharing one Enemy definition are independent. 10. NPC and Enemy coexist
independently. 11. Repeated Apply/rebuild does not accumulate. 12.
Delete removes stale Enemy while survivors remain. 13. Save/load
preserves authored patrol config, not transient state. 14. Production
Manual Respawn resets Enemy to authored origin/initial state without
accumulation. 15. M112 NPC Manual Respawn remains correct. 16.
Exact/Retargeted/Static/Unavailable remain safe. 17. Production
rendering/shadow regression uses CharacterInstance path. 18. Level
Format compatibility remains intact; avoid new grammar if M112 payload
suffices.

Prefer extending M111/M112 production-boundary tests over mock-only
parallel architecture.

## Manual Acceptance

Verify: - Enemy placement exposes Enemy Runtime controls; - enabling
patrol in workingCopy does not affect active runtime before Apply; -
Apply starts Enemy behavior; - Enemy patrol/Move/reversal/facing work; -
disabling + Apply returns origin/Idle; - Save and closing/reopening
Development preserve authored settings; - two Enemy placements sharing a
definition are independent; - NPC and Enemy coexist and remain
type-gated; - Gameplay R resets both NPC and Enemy to authored origins
without duplicates; - Player movement/animation/equipment/shadows remain
unaffected; - no manual-test residue remains in canonical levels.

Explicit user approval is mandatory.

## Canonical/Test Character Data

Prefer existing definitions/fixtures. If no suitable Enemy definition
exists, add the smallest justified canonical/test-purpose Enemy
definition using existing assets. Any permanent definition must be
documented and audited.

## Documentation

Update relevant repository docs: README, ARCHITECTURE,
CHARACTER_PLACEMENT, NPC_RUNTIME where shared behavior needs
clarification, LEVEL_FORMAT_V1 only if needed, MILESTONES, and AGENTS.
Add a focused Enemy runtime document if useful.

## Canonical Data Safety

Do not leave temporary Enemy/NPC placements in `level_01.level` or
`level_02.level`. Do not change `definitions.gameplay` merely for demo
content unless a minimal persistent Enemy fixture is strictly required
and justified.

Preserve historical M45 removal:

``` text
dynamic_box 0 5 0 1 1 1 30
```

Do not opportunistically normalize line endings.

## Deferred Tunings / Future Work

Outside M113: - Character placement Ghost during
Translate/Rotate/Scale. - Restart option in Pause menu and reconsider
direct R shortcut. - NPC/Enemy grounding, gravity, and Character
Controller/physics. - Moving-platform Manual Respawn behavior observed
to preserve transient progress.

## Explicitly Out of Scope

No health, damage, attacks, hit detection, combat, death/rewards, Player
targeting, aggro, perception, chase, navigation/pathfinding, obstacle
avoidance, behavior trees, GOAP, utility AI, planners, generalized AI,
generalized waypoint graphs, dialogue/quests/schedules, Character
Controller/physics, gravity/grounding, root motion, IK, animation graph
editor, Player migration, Animal runtime, generalized spawning
framework, ECS, GUIDs, JSON, Level Format v2, asset database rewrite,
watchers/hot reload, or M114+ functionality.

## Completion Gate

M113 is complete only after: 1. implementation and automated validation
pass; 2. Development startup succeeds; 3. manual acceptance is
explicitly approved by the user; 4. temporary canonical-data residue is
removed; 5. preclosure diff audit passes; 6. one meaningful milestone
commit is created; 7. milestone branch is pushed; 8. normal merge into
`main` is completed; 9. `main` is pushed; 10. `main` is clean and
synchronized with `origin/main`.

The coding agent must STOP after implementation/report and must not
commit, push, merge, or start M114.
