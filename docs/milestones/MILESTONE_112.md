# Milestone 112 --- NPC Runtime Foundation

## Status

DEFINED --- implementation not started.

## Goal

Build the first bounded NPC runtime layer on top of the Character
Placement & Spawning foundation from M111.

M112 promotes authored Character placements whose resolved
`CharacterDefinition::CharacterType` is `NPC` into NPC-controlled
runtime actors while preserving the existing M110/M111
`CharacterInstance` presentation path.

The milestone establishes NPC identity, lifecycle, locomotion state,
simple authored patrol behavior, and Development diagnostics without
introducing enemy AI, perception, combat, navigation/pathfinding,
dialogue, quests, or a generalized entity/component framework.

## Architectural Direction

``` text
CharacterDefinition (Character Type = NPC)
        +
Level Character placement (M111)
        +
optional bounded NPC placement settings
        ↓
active level
        ↓
NPC runtime actor
        ↓
M110 CharacterInstance
        ↓
Idle / Move presentation
        ↓
Exact / Retargeted / Static
```

Authority remains separated:

-   `CharacterDefinition` owns shared authored character metadata and
    presentation references.
-   Level Character placement owns persistent world transform and
    bounded NPC placement settings.
-   NPC runtime state is transient and session-local.
-   `CharacterInstance` remains the reusable visual/animation
    realization.
-   Runtime NPC handles/state are never serialized.
-   The gameplay Player remains special and is not migrated to this
    system.

## Scope

### 1. NPC Runtime Actor

Add a bounded runtime NPC representation for active Level Character
placements whose resolved Character Definition has
`Character Type = NPC`.

Each NPC runtime actor must have at least:

-   session-local runtime handle;
-   source Character placement index/reference;
-   Character Definition textual identity;
-   associated M110 `CharacterInstance`;
-   spawn/origin transform;
-   current runtime position;
-   locomotion state (`Idle` or `Move`);
-   patrol direction/state when patrol is enabled;
-   concise diagnostic state.

NPC runtime state is transient and must not be written into Level Format
v1.

### 2. Character-Type Gating

Runtime NPC behavior is enabled only when:

1.  the Level Character placement has a syntactically valid Character
    identity;
2.  the identity resolves to a Character Definition;
3.  the resolved Character Definition has `Character Type = NPC`;
4.  the M111 placement is otherwise eligible for runtime realization.

`Player`, `Enemy`, and `Animal` Character Types must not receive NPC
behavior in M112.

They may continue to exist as generic M111 Character placements
according to existing behavior.

A placement using `characters/player` must remain only a generic visual
Character placement and must never gain Player gameplay ownership.

### 3. Bounded NPC Patrol Authoring

Extend the existing M111 Character placement with optional NPC patrol
settings rather than creating a generalized behavior graph.

Required authored settings:

-   Patrol Enabled: boolean;
-   Patrol Distance: positive bounded scalar;
-   Patrol Speed: positive bounded scalar.

Patrol semantics:

-   the authored Character position is the patrol origin;
-   patrol is along the placement's local X axis projected into the
    horizontal world plane;
-   the NPC travels between `origin - distance` and `origin + distance`;
-   it reverses deterministically at the endpoints;
-   when patrol is disabled, the NPC remains at its authored position in
    `Idle`;
-   patrol movement selects `Move`; stationary behavior selects `Idle`;
-   no root motion;
-   movement is deterministic from runtime simulation state and does not
    rewrite authored placement data.

Use conservative finite bounds consistent with existing Level Format v1
validation conventions. Reject malformed/non-finite/non-positive patrol
values where applicable.

### 4. Level Format v1 Persistence

Persist the bounded NPC patrol settings as part of the existing
Character placement contract while respecting all existing Level Format
v1 limits.

Requirements:

-   deterministic parser/writer round-trip;
-   existing M111 Character records remain backward compatible and
    default to patrol disabled;
-   no file-format version bump;
-   no JSON, GUIDs, ECS, sidecar behavior files, or generalized
    component serialization;
-   runtime handles, current patrol position/direction, animation
    clocks, and transient NPC state must never serialize.

The exact grammar may be chosen after repository inspection, but it must
remain compact, deterministic, documented, and compatible with the
existing Character record.

### 5. Level Editor Authoring

Extend the existing Character Inspector with a compact NPC Runtime
section.

For a resolved `Character Type = NPC`, expose:

-   Patrol Enabled;
-   Patrol Distance;
-   Patrol Speed;
-   clear explanation that patrol uses the authored transform's local
    horizontal X axis.

For non-NPC Character Types:

-   do not imply that NPC behavior is active;
-   existing placement editing remains unchanged;
-   stored values, if any can exist through malformed/legacy input, must
    be handled deterministically and safely.

The controls must follow the existing
`workingCopy → Apply → active → Save` authority.

Changing NPC patrol settings in the working copy must not mutate the
active runtime until Apply.

Save persists authored state only.

Reload restores authored state and rebuilds runtime state
deterministically.

### 6. Runtime Lifecycle

Integrate NPC actors at the same active-level promotion/rebuild
boundaries established by M111.

Required behavior:

-   one eligible active NPC placement creates exactly one NPC runtime
    actor;
-   its actor owns/references exactly one corresponding
    `CharacterInstance` through the established safe M110/M111 lifetime
    rules;
-   Apply/rebuild destroys stale NPC runtime state before creating
    replacement state;
-   repeated Apply/Restart/rebuild must not accumulate NPC actors or
    CharacterInstances;
-   deleting a placement removes its NPC actor;
-   changing the referenced Character Definition or Character Type
    updates eligibility on the next authoritative promotion;
-   Restart resets transient NPC state to the authored spawn/origin
    state;
-   level transitions/reloads rebuild from authored active data;
-   session-local handles are never serialized.

### 7. Movement and Presentation

NPC movement must be intentionally simple and deterministic.

Requirements:

-   horizontal patrol only;
-   no navigation mesh;
-   no obstacle avoidance;
-   no pathfinding;
-   no physics-driven character controller requirement;
-   no gravity/falling simulation added by M112;
-   no terrain/ledge intelligence;
-   no interaction with Player collision/gameplay ownership unless
    already naturally provided by existing generic rendering/picking
    systems.

The associated CharacterInstance must receive the runtime transform and
locomotion state so existing M103/M105/M109/M110 animation behavior
remains authoritative:

-   stationary → Idle;
-   patrolling → Move;
-   Exact compatibility remains preferred;
-   Retargeted presentation remains supported;
-   Static presentation remains safe;
-   unavailable animation/model cases fail safely without breaking NPC
    runtime lifecycle.

### 8. Orientation

When patrolling, the NPC should visually face its current horizontal
travel direction using a bounded presentation/runtime rotation derived
from the authored orientation.

Do not overwrite the authored rotation.

Endpoint reversal changes only transient runtime facing/direction.

Avoid introducing generalized steering, turn-rate systems, animation
graphs, IK, or root motion.

### 9. Development Diagnostics

Extend Development diagnostics with an `M112 NPC Runtime` section.

For each runtime NPC, expose enough information to correlate:

-   NPC runtime handle;
-   source placement index;
-   Character identity;
-   Character Type;
-   patrol enabled/disabled;
-   authored origin;
-   current runtime position;
-   current direction/endpoints;
-   Idle/Move state;
-   associated CharacterInstance handle;
-   Exact / Retargeted / Static / Unavailable presentation status.

Diagnostics are Development-only and non-persistent.

### 10. Rendering and Shadows

Reuse the established CharacterInstance rendering path.

NPCs must remain consistent with existing:

-   production character materials;
-   skeletal skinning;
-   exact-compatible animation;
-   M109 retargeted animation;
-   static Character presentation;
-   directional shadow rendering.

Do not create a separate NPC rendering implementation.

## Tests

Add focused automated coverage using repository conventions.

At minimum cover:

1.  Level Format v1 backward compatibility for an M111 Character record
    with no NPC patrol fields/settings.
2.  deterministic round-trip of NPC patrol settings.
3.  malformed, non-finite, zero/negative, and out-of-range patrol values
    are rejected safely.
4.  NPC Character Type creates one NPC runtime actor from one eligible
    active placement.
5.  Player, Enemy, and Animal types do not receive M112 NPC behavior.
6.  missing/malformed Character references remain safe and do not create
    NPC actors.
7.  patrol-disabled NPC remains at authored origin and selects Idle.
8.  patrol-enabled NPC moves deterministically between authored
    endpoints and selects Move.
9.  endpoint reversal is deterministic and does not mutate authored
    placement data.
10. runtime facing follows travel direction without rewriting authored
    rotation.
11. two placements using the same NPC Character Definition have
    independent runtime state.
12. repeated Apply/rebuild/Restart does not accumulate NPC actors or
    CharacterInstances.
13. delete/rebuild removes stale runtime NPC state while preserving
    unrelated survivors.
14. Save/Reload preserves authored patrol configuration but not
    transient runtime state.
15. CharacterInstance presentation remains Exact/Retargeted/Static-safe
    as appropriate.
16. production rendering/shadow regression proves the NPC still uses the
    established CharacterInstance path.

Prefer extending the real production-boundary regression introduced for
M111 rather than creating a parallel mock-only architecture.

## Manual Acceptance

Manual acceptance must verify at least:

-   an authored NPC placement can enable patrol in the Level Editor;
-   Apply starts runtime patrol without rewriting the authored origin;
-   the NPC visibly alternates Move direction between deterministic
    endpoints;
-   disabling patrol and Apply returns the NPC to authored origin/Idle
    behavior;
-   Save/Reload preserves patrol settings;
-   Restart resets transient patrol state;
-   two NPC placements using the same definition move independently;
-   generic non-NPC Character placements remain non-behavioral;
-   Player gameplay, movement, animation, equipment, and shadows remain
    unaffected;
-   no duplicate NPCs appear after repeated Apply/Restart;
-   canonical Level test residue is removed before Git closure.

Automated green is not sufficient; user approval is required before Git
closure.

## Documentation

Update the relevant repository documentation, including as appropriate:

-   `README.md`
-   `docs/ARCHITECTURE.md`
-   `docs/LEVEL_FORMAT_V1.md`
-   `docs/MILESTONES.md`
-   `AGENTS.md`

Add a focused NPC runtime document if it improves architectural clarity.

## Canonical Data Safety

Do not leave temporary/manual-test NPC placements or patrol edits in:

-   `game/assets/source/levels/level_01.level`
-   `game/assets/source/levels/level_02.level`

Do not modify `game/assets/source/gameplay/definitions.gameplay` merely
to create demo content unless a minimal canonical NPC definition is
strictly required by the implementation and explicitly justified.

Preserve the historical M45 removal of:

``` text
dynamic_box 0 5 0 1 1 1 30
```

Do not normalize line endings opportunistically.

## Explicitly Out of Scope

M112 must NOT add:

-   Enemy runtime behavior;
-   Animal runtime behavior;
-   Player migration to Character spawning;
-   combat, attacks, damage, health, death, rewards;
-   AI perception, vision, hearing, aggro;
-   navigation meshes, pathfinding, obstacle avoidance;
-   generalized waypoint/path graph authoring;
-   dialogue, quests, schedules, shops, interaction systems;
-   behavior trees, GOAP, utility AI, planners;
-   generalized Character controller/physics framework;
-   root motion;
-   IK or procedural locomotion;
-   animation graph/state-machine editor;
-   generalized spawning framework;
-   ECS;
-   GUIDs;
-   JSON;
-   Level Format v2;
-   asset database rewrite;
-   file watchers/hot reload;
-   M113+ functionality.

## Deferred Tuning --- Character Placement Ghost

During M111 manual acceptance, Character placement worked correctly but
the existing transform workflow did not provide a sufficiently
perceptible Ghost/preview effect while translating, rotating, or scaling
a Character.

This is recorded as **TUNING**, not an M111 defect.

Do not opportunistically implement it in M112 unless it is strictly
required to fix a regression introduced by M112. Preserve it for a
focused editor UX/tuning pass so NPC runtime scope remains bounded.

Desired future tuning:

-   clearly visible Character Ghost during Translate / Rotate / Scale;
-   Ghost uses the same working-copy Character model/transform
    semantics;
-   active/runtime Character remains visually distinguishable until
    Apply;
-   no change to `workingCopy → Apply → active` authority.

## Completion Gate

M112 is complete only after:

1.  implementation and automated validation pass;
2.  Development startup succeeds;
3.  manual acceptance is explicitly approved by the user;
4.  temporary canonical-data residue is removed;
5.  preclosure diff audit passes;
6.  one meaningful milestone commit is created;
7.  milestone branch is pushed;
8.  normal merge into `main` is completed;
9.  `main` is pushed;
10. `main` is clean and synchronized with `origin/main`.

The coding agent must STOP after implementation/report and must not
commit, push, merge, or start M113.
