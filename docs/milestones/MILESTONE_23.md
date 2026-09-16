# Milestone 23 --- Dynamic Body Interaction Safety

## Goal

Harden the existing Player ↔ dynamic-body interaction so the cyan Jolt
test box cannot leave the CharacterVirtual permanently wedged or unable
to move when the box, player, moving platform, and static world geometry
converge.

This milestone is based on the manually observed issue after M22:

``` text
dynamic cyan box moves/falls to the right
        +
player enters the same constrained area
        +
moving platform / static geometry closes space
        ↓
player can become physically stuck
        ↓
manual R respawn is currently required to recover
```

M23 must diagnose the actual cause in the current Jolt/CharacterVirtual
integration before choosing a fix.

The goal is **interaction safety**, not a generic physics redesign.

------------------------------------------------------------------------

## Why M23 Comes Next

M22 completed the single-level run loop:

``` text
spawn -> traversal -> checkpoint -> goal -> restart -> new run
```

Before expanding the level or introducing multiple checkpoints, the
current physics sandbox should be robust enough that an existing dynamic
object does not create an avoidable gameplay hard-lock.

The later idea of a longer level and multiple checkpoints is
intentionally deferred.

------------------------------------------------------------------------

## Primary Success Condition

After M23:

-   the cyan dynamic box remains a real Jolt dynamic body;
-   the player can still physically interact with it;
-   the box can still fall, move, collide, sleep/wake, and reset on M22
    restart;
-   the moving platform still works;
-   but the player should not become permanently trapped in the observed
    box/platform/static-geometry situation during normal traversal.

A temporary collision block is acceptable.

A state requiring `R` solely because the CharacterVirtual became
physically wedged by the technical dynamic box is not.

------------------------------------------------------------------------

## Phase A Must Diagnose First

Do not assume the fix.

Inspect the current implementation and determine:

1.  how CharacterVirtual contacts dynamic bodies;
2.  whether the CharacterVirtual currently applies impulses/forces to
    dynamic bodies;
3.  whether dynamic bodies can push the CharacterVirtual;
4.  how `UpdateGroundVelocity`, `SetLinearVelocity`,
    `CharacterVirtual::Update`, and `PhysicsSystem::Update` interact;
5.  whether the observed lock is:
    -   expected geometric blocking,
    -   penetration/recovery failure,
    -   stale support/contact state,
    -   insufficient player push strength,
    -   dynamic-body mass/response issue,
    -   moving-platform compression,
    -   or another cause;
6.  which Jolt CharacterVirtual facilities are already available in the
    pinned Jolt version;
7.  whether the current cyan-box mass/body settings contribute to the
    problem.

Do not tune blindly.

------------------------------------------------------------------------

## Preferred Fix Philosophy

Use the smallest solution supported by the actual diagnosis.

Preferred order:

1.  correct misuse/incomplete use of CharacterVirtual dynamic-body
    interaction if present;
2.  add a narrowly scoped CharacterVirtual contact/push policy if Jolt
    expects one;
3.  adjust only the technical cyan-box physical properties if they are
    clearly inappropriate;
4.  add a small anti-wedge safety measure only if the physics
    integration itself cannot reasonably prevent the observed trap.

Do not begin with teleport-based unstuck logic.

Manual respawn remains a gameplay feature, not the primary physics fix.

------------------------------------------------------------------------

## CharacterVirtual / Jolt Boundary

All Jolt-specific implementation remains inside:

``` text
PhysicsWorld.cpp
```

Public project headers must continue to expose only project-owned types.

Do not leak:

-   JPH::BodyID
-   JPH::CharacterVirtual
-   JPH::ContactSettings
-   JPH::Vec3
-   JPH::RVec3
-   JPH::Quat
-   Jolt listener types

If a listener/callback is required, keep it private to the PhysicsWorld
implementation.

------------------------------------------------------------------------

## Player Ownership

Player continues to own:

-   movement intent;
-   acceleration/deceleration;
-   jump policy;
-   gravity policy;
-   coyote time;
-   jump buffer;
-   relative horizontal velocity.

PhysicsWorld continues to own:

-   CharacterVirtual;
-   physical contacts;
-   static/dynamic/kinematic bodies;
-   Jolt-specific collision response;
-   moving-platform physical state.

Do not move gameplay movement policy into Jolt callbacks.

------------------------------------------------------------------------

## Dynamic Box

The cyan box remains a technical dynamic physics body.

Current canonical restart state from M22:

``` text
position         = {0.0, 5.0, 0.0}
size             = {1.0, 1.0, 1.0}
rotation         = identity
linear velocity  = zero
angular velocity = zero
activation       = active
```

M23 may change its physical material/mass/inertia-related setup only if
Phase A demonstrates that the existing setup is part of the problem.

Do not change its visual identity or convert it to static/kinematic
merely to avoid interaction.

------------------------------------------------------------------------

## Moving Platform

Preserve M13:

``` text
start            = {0.0, 1.3, 0.0}
path X           = [-6, +6]
speed            = 2.5
size             = {4.0, 0.4, 3.0}
initial direction= +X
```

Preserve:

-   standing carry;
-   airborne carry;
-   reversal;
-   normal M20 respawn behavior;
-   deterministic M22 restart.

Do not solve the box problem by disabling moving-platform collision.

------------------------------------------------------------------------

## Static World / Slopes

Preserve existing M14 geometry and slope behavior.

Do not move platforms, slopes, checkpoint, goal, or kill plane as the
primary M23 fix.

The observed issue should first be solved at the dynamic-body
interaction layer.

------------------------------------------------------------------------

## No New Gameplay Mechanic

M23 does not add:

-   attack;
-   grab;
-   push button;
-   dash;
-   crouch;
-   wall jump;
-   damage;
-   health;
-   lives;
-   enemy logic.

The player should interact with the box using the existing movement
only.

------------------------------------------------------------------------

## Debug Diagnostics

Phase B may add small Debug/Development-only diagnostics if they
materially help verify the fix.

Useful candidates:

``` text
dynamic contact count
dynamic body contact this frame
support body type
player world velocity
box position / velocity
box active/sleeping
```

Only add values that are actually useful to the diagnosis.

Do not build a generic physics inspector.

Release remains free of Dear ImGui.

------------------------------------------------------------------------

## Optional Debug Visualization

If the diagnosis genuinely benefits from it, a minimal
Debug/Development-only visualization may show:

-   CharacterVirtual bounds;
-   cyan-box bounds;
-   contact normal(s);
-   support/contact point.

This is optional.

Do not create a general collision-debug renderer.

Do not add Release visualization.

------------------------------------------------------------------------

## Anti-Wedge Safety Rule

If, after correct Jolt dynamic-body interaction is implemented, a
residual compression case still exists, Phase A/B may define one
narrowly scoped safety invariant.

Example concept:

``` text
the CharacterVirtual must retain a physically valid escape direction
instead of accumulating unrecoverable penetration/contact state
```

Any safety mechanism must:

-   be deterministic;
-   not teleport during ordinary contacts unless absolutely justified;
-   not trigger during normal wall/platform blocking;
-   not bypass the kill-plane/respawn system;
-   not create upward boosts;
-   not alter jump/coyote/buffer semantics;
-   not become a generic "unstuck system" unless evidence requires it.

------------------------------------------------------------------------

## Update Order

Preserve the proven M22 ordering unless diagnosis shows a concrete
defect:

``` text
Poll
↓
capture restart availability
↓
UpdateMovingPlatform
↓
Player::Update / CharacterVirtual movement
↓
Fall / Manual
↓
checkpoint / goal
↓
Restart if eligible
↓
PhysicsWorld::Update
↓
camera update when allowed
↓
render
```

Do not reorder CharacterVirtual and PhysicsSystem steps speculatively.

If Phase A finds that ordering is the actual cause, report the evidence
and propose the smallest correction before implementing it.

------------------------------------------------------------------------

## M20 / M21 / M22 Preservation

M23 must preserve:

### M20

-   R manual respawn;
-   kill-plane fall;
-   checkpoint;
-   death count;
-   camera snap;
-   moving-platform carry semantics.

### M21

-   one goal;
-   one-way completion within a run;
-   `LEVEL COMPLETE`.

### M22

-   Enter restart only after prior-frame completion;
-   restart clears run state;
-   moving platform reset;
-   cyan-box reset;
-   second completion;
-   `PRESS ENTER TO RESTART`.

The box-interaction fix must not change restart/respawn semantics.

------------------------------------------------------------------------

## Gameplay Constants

Do not change:

``` text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Do not use movement retuning to hide the physics issue.

------------------------------------------------------------------------

## Asset Pipeline

No asset work.

Do not modify:

-   source assets;
-   cooked assets;
-   Blender workflow;
-   Pillow;
-   cooker;
-   runtime PNG recipe;
-   CMake asset staging.

No new dependency.

------------------------------------------------------------------------

## Manual Validation

M23 is complete when the following are manually validated:

  -----------------------------------------------------------------------
  Test                                Expected
  ----------------------------------- -----------------------------------
  Player meets cyan box on open       Physical interaction remains stable
  ground                              

  Player pushes/runs into box         No unrecoverable CharacterVirtual
                                      lock

  Box approaches static geometry      Player can back away when space
                                      permits

  Box + moving platform interaction   No persistent player wedge in
                                      reproduced problem area

  Platform compresses box near player Stable response; no permanent stuck
                                      state

  Repeated attempts                   Issue does not reproduce under the
                                      known scenario

  Jump near box                       No abnormal boost / inherited
                                      velocity

  Stand near/on valid support         Ground classification remains
                                      correct

  R while interacting with box        M20 respawn still clears player
                                      state

  Fall                                M20 death/respawn unchanged

  Complete level                      M21 unchanged

  Enter restart                       M22 resets box/platform/player
                                      correctly

  After restart                       Box begins normal simulation again

  Moving platform                     carry / jump-carry / reversal
                                      unchanged

  Slopes                              30° walkable / 60° steep unchanged

  Release                             physics fix works without debug UI
  -----------------------------------------------------------------------

------------------------------------------------------------------------

## Known Scenario to Reproduce

Before declaring success, deliberately attempt the user-observed
scenario:

1.  allow the cyan box to fall and move toward the right side;
2.  move the Player into the same area;
3.  time the interaction with the moving platform;
4.  create the constrained box/player/platform/static-geometry
    situation;
5.  attempt to move both left and right;
6.  repeat multiple times.

Record whether the result is:

``` text
normal blocking
temporary compression
recoverable contact
permanent wedge
```

The milestone is specifically intended to eliminate the last category.

------------------------------------------------------------------------

## Explicitly Out of Scope

Do not implement:

-   Milestone 24;
-   longer level;
-   larger main platform as level-design expansion;
-   multiple checkpoints;
-   checkpoint arrays/system;
-   multiple levels;
-   enemies;
-   combat;
-   health;
-   lives;
-   collectibles;
-   score;
-   timer;
-   audio;
-   particles;
-   generic unstuck manager;
-   generic physics event bus;
-   generic contact framework;
-   ECS;
-   scene manager;
-   level manager;
-   new assets;
-   new dependencies;
-   cooker changes.

The user's longer-platform / multiple-checkpoint experiment is
deliberately deferred to a later milestone.

------------------------------------------------------------------------

## Completion Criteria

-   [ ] Observed dynamic-box wedge scenario is deliberately
    investigated.
-   [ ] Root cause is documented from the actual implementation.
-   [ ] Fix uses Jolt/CharacterVirtual correctly rather than blind
    tuning.
-   [ ] Cyan box remains dynamic.
-   [ ] Player ↔ dynamic-box interaction remains physical.
-   [ ] Player is not permanently wedged in the reproduced scenario.
-   [ ] No inappropriate teleport-based workaround is introduced.
-   [ ] Player movement constants remain unchanged.
-   [ ] CharacterVirtual remains behind PhysicsWorld.
-   [ ] No Jolt types leak into public headers.
-   [ ] Moving-platform carry remains correct.
-   [ ] Moving-platform reversal remains correct.
-   [ ] M20 respawn remains correct.
-   [ ] M21 completion remains correct.
-   [ ] M22 restart remains correct.
-   [ ] Cyan box still resets on restart.
-   [ ] Slopes remain correct.
-   [ ] Debug diagnostics are Debug/Development-only.
-   [ ] Release contains no Dear ImGui.
-   [ ] Asset pipeline remains unchanged.
-   [ ] No dependency is added.
-   [ ] No generic physics/gameplay framework is introduced.
-   [x] Milestone 24 is not started. (superseded: M24 Phase A is active)

------------------------------------------------------------------------

## Recommended Phases

### Phase A --- Reproduce, Inspect, Diagnose

**Phase A:** Diagnosis complete and approved. CharacterVirtual already pushes dynamic bodies with `mMaxStrength * dt` in `HandleContact` (no listener required for impulses). No inner body and no `SetListener` in Phase A. Cyan box used Jolt default density 1000 kg/m³ (~1000 kg) vs 100 N max strength. Kinematic platform can carry/compress the box after `CharacterVirtual::Update`. Debug/Development metrics expose box velocity, support body kind, dynamic contact, and CharacterVirtual world velocity.

### Phase B --- Implement Narrow Physics Fix

**Phase B:** CharacterVirtual inner body enabled (0.9-scale translated capsule, layer Moving). Cyan box mass overridden to 30 kg via `CalculateInertia`. Character mass 70 / maxStrength 100 unchanged. No listener, no anti-wedge, no update-order change. Approved.

### Phase C --- Regression + Manual Stress Test

**Phase C:** Complete and manually validated. No listener, tuning, or anti-wedge added. Player is a physical barrier, the 30 kg box is pushable, and the previously observed persistent wedge is no longer reproduced. Milestone 24 is now the active milestone.

------------------------------------------------------------------------

## Git Branch

``` powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/23-dynamic-body-safety
git branch --show-current
```

Expected:

``` text
milestone/23-dynamic-body-safety
```

------------------------------------------------------------------------

## Recommended Model

Use **Grok 4.6 High --- Fast OFF** for Phase A.

This milestone is diagnosis-heavy and depends on understanding the
current CharacterVirtual/Jolt contact semantics rather than simply
adding gameplay code.

Keep Fast OFF for Phase B as well if the approved fix touches
CharacterVirtual contact listeners, body impulses, collision response,
or update ordering.

------------------------------------------------------------------------

## Workflow

``` text
main clean
    ↓
define M23
    ↓
update docs/MILESTONES.md
    ↓
create milestone branch
    ↓
Phase A diagnosis
    ↓
review
    ↓
Phase B narrow fix
    ↓
review
    ↓
Phase C stress/regression tests
    ↓
approve
    ↓
commit
    ↓
push branch
    ↓
merge main
    ↓
push main
    ↓
verify clean main
```
