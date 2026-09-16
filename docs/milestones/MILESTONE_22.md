# Milestone 22 --- Level Restart + Run-State Reset [COMPLETE]

## Goal

Add the first explicit restart loop after level completion, without
introducing multiple levels or a generic game-state framework.

M20 established failure/recovery:

``` text
fall / manual respawn -> continue current run
```

M21 established success:

``` text
reach goal -> levelCompleted = true
```

M22 adds a deliberate new-run action:

``` text
reach goal
    ↓
LEVEL COMPLETE
    ↓
press semantic Restart
    ↓
reset run-level state
    ↓
return to initial spawn
    ↓
play the same level again
```

The level is not reloaded from disk. Static world geometry and runtime
assets remain loaded.

------------------------------------------------------------------------

## Scope

Implement exactly one restart action for the current single level.

Restart must reset the gameplay state that belongs to the current run:

-   Player/CharacterVirtual position and transient movement state
-   checkpoint activation
-   respawn destination
-   death count
-   last respawn reason
-   level completion state
-   camera target/smoothing state

Restart must also reset the moving platform to its canonical initial
motion state so the new run begins deterministically.

The cyan dynamic Jolt test box should also return to its canonical
initial transform/velocity if the current PhysicsWorld architecture can
do so through a small project-owned reset API.

Do not reload the process, scene, assets, or physics world.

------------------------------------------------------------------------

## Restart Availability

The restart action is only meaningful after:

``` text
levelCompleted == true
```

Before completion, pressing the restart key should do nothing.

This prevents the new action from becoming a second manual-respawn
shortcut.

------------------------------------------------------------------------

## Semantic Input

Add one semantic action:

``` cpp
bool restartPressed;
```

Recommended keyboard binding:

``` text
Enter -> restartPressed
```

The raylib key constant must remain isolated in the platform/input
backend.

Gameplay/Application sees only `restartPressed`.

Existing input remains unchanged:

-   A/D or arrows -\> movement
-   Space/Up -\> jump
-   R -\> manual respawn
-   Enter -\> restart completed run

Do not overload R.

------------------------------------------------------------------------

## Completion Feedback

After completion, retain:

``` text
LEVEL COMPLETE
```

and add a small instruction such as:

``` text
PRESS ENTER TO RESTART
```

Both are gameplay UI and must exist in Release.

They must be rendered through the existing Renderer/backend boundary,
not through Dear ImGui.

Before completion, neither completion message should be visible.

------------------------------------------------------------------------

## Run-State Ownership

Application remains the owner/coordinator of run-level meaning.

A focused helper such as:

``` text
RestartRun()
```

or equivalent is acceptable in `Application`.

Do not create:

-   GameStateManager
-   LevelManager
-   RestartManager
-   generic state machine
-   event bus
-   ECS

M22 is still a single-level game.

------------------------------------------------------------------------

## Canonical Restart State

After restart, the run should match a fresh launch as closely as
practical.

Expected gameplay state:

``` text
Player visual center      = {0.0, 0.8, 0.0}
Player velocities         = zero
Player carry              = zero
Player grounded/support   = refreshed/clean
jump buffer               = clear
coyote state              = clear

checkpointActive          = false
respawnPosition           = initial spawn
deathCount                = 0
lastRespawnReason         = None

levelCompleted            = false

camera                    = snapped to initial spawn

moving platform           = canonical initial position/direction/target
dynamic cyan box          = canonical initial transform/velocity
```

The exact canonical moving-platform state and cyan-box state must be
derived from the current implementation rather than guessed.

------------------------------------------------------------------------

## Restart vs Respawn

Restart and respawn are different operations.

### Respawn (M20)

``` text
R or fall
    -> teleport Player to current respawnPosition
    -> preserve checkpoint activation
    -> preserve levelCompleted
    -> fall may increment deathCount
    -> moving platform continues
```

### Restart (M22)

``` text
Enter after completion
    -> begin a fresh run of the same level
    -> initial spawn
    -> checkpoint cleared
    -> deathCount cleared
    -> completion cleared
    -> moving platform reset
    -> test dynamic body reset
```

Do not implement restart by abusing the existing manual-respawn path
alone.

Reuse low-level reset helpers where appropriate, but keep the gameplay
semantics explicit.

------------------------------------------------------------------------

## Physics Boundary

PhysicsWorld remains responsible only for physical reset operations.

Small project-owned APIs are acceptable, for example:

``` cpp
ResetCharacter(...)
ResetMovingPlatform()
ResetTestDynamicBody()
```

or one narrowly scoped physical-run reset method if inspection shows
that is cleaner.

Public APIs must use project-owned types only.

No Jolt types in project headers.

PhysicsWorld must not know:

-   checkpointActive
-   deathCount
-   levelCompleted
-   restartPressed

Application coordinates those meanings.

------------------------------------------------------------------------

## Moving Platform Reset

M13 currently lets the moving platform continue through normal respawns.

M22 restart is different: a fresh run should reset it.

Inspect its actual canonical initial values and restore:

-   initial position
-   initial target/direction
-   physical kinematic transform
-   any carried-ground velocity state relevant to the CharacterVirtual

The platform must resume normal M13 movement on subsequent frames.

Do not alter its speed/path/tuning.

------------------------------------------------------------------------

## Cyan Dynamic Box Reset

The existing cyan Jolt box is a technical physics object.

For deterministic restart, reset it to the same initial transform used
at application initialization and clear its linear/angular velocity.

If a tiny focused reset API is required, add it behind PhysicsWorld.

Do not expose Jolt body IDs/types publicly.

Do not redesign the test body into a gameplay entity.

------------------------------------------------------------------------

## Update Ordering

Inspect the actual M21 loop first.

Restart should be evaluated after input is available and after the
current frame's Player update/respawn/completion logic has established
the state, but it must not allow a second Player movement after the
restart teleport.

A likely safe relationship is:

``` text
Poll input
↓
UpdateMovingPlatform
↓
Player::Update
↓
Fall / Manual respawn
↓
if no respawn:
    checkpoint activation
    goal completion
↓
if levelCompleted && restartPressed:
    RestartRun
    mark restartThisFrame
↓
PhysicsWorld::Update
↓
camera.Update only if neither respawn nor restart snapped it
↓
render
```

Phase A must inspect whether restart should occur before or after the
normal moving-platform step for the frame and define the exact
invariant.

Critical rules:

-   at most one restart per frame
-   restart only when completed
-   no Player::Update after restart in the same frame
-   no goal re-completion in the restart frame
-   no checkpoint activation caused by restart teleport
-   camera performs a hard snap
-   the next frame resumes normal gameplay

------------------------------------------------------------------------

## Interaction with Same-Frame Inputs

Define deterministic priority.

Recommended:

``` text
Fall
    > Manual Respawn
    > normal checkpoint/goal evaluation
    > Restart if completed
```

Because restart is only allowed after completion, an already-completed
run may receive R and Enter on the same frame.

Phase A must inspect the cleanest policy.

Preferred behavior:

``` text
if a respawn occurred this frame:
    do not restart this frame
```

This preserves the existing M20 respawn semantics and avoids two
teleports/reset operations in one frame.

Restart may occur on a later fresh Enter press.

------------------------------------------------------------------------

## Camera

Restart must hard-snap the M08 camera to the initial spawn.

No interpolation across the map.

Normal dead zone/smoothing resumes on the next frame.

Do not change:

``` text
horizontal dead zone = 1.5
vertical dead zone   = 0.75
sharpness            = 8
```

------------------------------------------------------------------------

## Debug / Development Metrics

Add a small read-only restart/run section or extend the existing
relevant metrics.

Useful values:

``` text
Level completed
Checkpoint active
Death count
Restart available
Restart pressed
```

Avoid duplicating every existing metric.

Dear ImGui remains Debug/Development only.

------------------------------------------------------------------------

## Release Behavior

Release must contain:

-   LEVEL COMPLETE
-   PRESS ENTER TO RESTART
-   restart gameplay

Release must not contain Dear ImGui/debug metrics.

No restart behavior may depend on `PLATFORMER_ENABLE_DEBUG_UI`.

------------------------------------------------------------------------

## Asset Pipeline

No asset changes.

Do not modify:

-   source assets
-   cooked assets
-   cooker recipes
-   Pillow dependency
-   Blender workflow
-   CMake runtime asset staging

M15--M19 behavior must remain unchanged.

------------------------------------------------------------------------

## Existing Systems to Preserve

Do not change gameplay tuning:

``` text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Preserve:

-   M08 camera behavior
-   M11 CharacterVirtual
-   M13 moving-platform behavior during normal gameplay/respawn
-   M14 slopes
-   M20 checkpoint/fall/manual respawn
-   M21 one-way completion within a run
-   M15--M19 asset/runtime behavior

------------------------------------------------------------------------

## Manual Validation

M22 is complete when the following are manually validated:

  -----------------------------------------------------------------------
  Test                                Expected
  ----------------------------------- -----------------------------------
  Fresh run + Enter                   No restart effect

  Complete level                      `LEVEL COMPLETE` + restart
                                      instruction

  Enter after completion              Fresh run at initial spawn

  Restart Player                      Position/velocities/carry clean

  Restart checkpoint                  Inactive, respawn back to initial
                                      spawn

  Restart death count                 `0`, reason `None`

  Restart completion                  `false`, completion UI disappears

  Restart camera                      Immediate snap to initial spawn

  Restart moving platform             Canonical initial motion state

  Restart cyan box                    Canonical initial
                                      transform/velocity

  R before completion                 Existing M20 manual respawn

  R after completion                  Existing M20 respawn; completion
                                      preserved unless Enter restart is
                                      later used

  Fall after completion               Existing M20 fall; completion
                                      preserved

  Complete again after restart        Goal can be completed again in new
                                      run

  Moving platform after restart       Normal carry/reversal works

  Slopes after restart                30°/60° behavior unchanged

  Release                             Completion/restart UI and behavior
                                      work, no ImGui
  -----------------------------------------------------------------------

------------------------------------------------------------------------

## Explicitly Out of Scope

Do not implement:

-   Milestone 23
-   multiple levels
-   next level
-   level selection
-   scene loading
-   scene reload from disk
-   process restart
-   main menu
-   pause menu
-   completion menu
-   savegame
-   persistence
-   score
-   timer
-   leaderboard
-   collectibles
-   enemies
-   health
-   lives
-   audio
-   particles
-   fade transitions
-   animation system
-   generic trigger system
-   generic event bus
-   ECS
-   generic game-state/state-machine framework
-   generic level manager
-   new assets
-   new dependencies
-   asset cooker changes

------------------------------------------------------------------------

## Completion Criteria

-   [ ] Semantic `restartPressed` exists.
-   [ ] Enter/key constant is isolated in Input backend.
-   [ ] Restart is available only after level completion.
-   [ ] Completion UI includes restart instruction.
-   [ ] Restart returns Player to canonical initial spawn.
-   [ ] CharacterVirtual velocity/state is clean.
-   [ ] Player transient movement state is clean.
-   [ ] Checkpoint is cleared.
-   [ ] Respawn destination returns to initial spawn.
-   [ ] Death count returns to zero.
-   [ ] Last respawn reason returns to None.
-   [ ] Level completion returns to false.
-   [ ] Camera snaps to initial spawn.
-   [ ] Moving platform resets deterministically.
-   [ ] Cyan dynamic box resets deterministically.
-   [ ] Normal M20 respawn does not become restart.
-   [ ] R behavior remains unchanged.
-   [ ] Fall behavior remains unchanged.
-   [ ] Goal can be completed again after restart.
-   [ ] No second Player update occurs on restart frame.
-   [ ] No goal/checkpoint activation occurs because of restart
    teleport.
-   [ ] PhysicsWorld remains free of gameplay restart meaning.
-   [ ] No Jolt types leak into public headers.
-   [ ] Release contains restart gameplay/UI.
-   [ ] Release contains no Dear ImGui metrics.
-   [ ] Gameplay constants remain unchanged.
-   [ ] M15--M19 remain unchanged.
-   [ ] No new dependency is added.
-   [ ] No generic game-state/level framework is introduced.
-   [ ] Milestone 23 is not started.

------------------------------------------------------------------------

## Recommended Phases

### Phase A --- Restart Contract and Reset Inventory

Inspect the real M21 code and define:

-   exact input binding/semantic action
-   exact restart priority/order
-   all run-level state that must reset
-   canonical moving-platform initial state
-   canonical cyan-box initial state
-   PhysicsWorld reset APIs
-   Application restart coordinator
-   camera reset
-   completion/restart UI path
-   metrics
-   regression risks

Small foundational APIs may be added, but do not wire the complete
restart loop yet.

**Phase A:** `restartPressed` maps `Enter` in `Input.cpp` only. `PhysicsWorld::ResetMovingPlatform` / `ResetDynamicTestBox` exist. `Application::RestartRun` coordinates the full reset. Approved.

### Phase B --- Restart Gameplay

**Phase B:** After poll, Application captures `restartAvailableAtFrameStart`. After checkpoint/goal, if no respawn and restart was already available and `restartPressed`, it calls `RestartRun()`. Same-frame goal + Enter completes and does not restart. Renderer draws `PRESS ENTER TO RESTART` under `LEVEL COMPLETE` while completed. Approved.

Implemented:

-   semantic restart input
-   completion restart instruction
-   Application restart coordination
-   physical resets
-   run-state resets
-   camera snap
-   metrics
-   Release behavior

### Phase C --- Regression and Manual Validation

**Phase C:** Implementation complete and manually validated. Restart gameplay, UI, and M20/M21 regressions accepted. Do not start Milestone 23 from this section.

Inspection confirmed:

-   restart only after completion already true at frame start
-   same-frame goal + Enter completes and does not restart
-   Fall > Manual > Restart
-   RestartRun canonical Player/M20/M21/platform/box/camera state
-   M20 respawn does not become restart
-   M21 goal geometry unchanged
-   Release contains restart gameplay/UI and omits Dear ImGui
-   cooker/assets/constants unchanged

------------------------------------------------------------------------

## Git Branch

After `main` is clean and the milestone definition is accepted:

``` powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/22-level-restart
git branch --show-current
```

Expected:

``` text
milestone/22-level-restart
```

------------------------------------------------------------------------

## Recommended Model

Use **Grok 4.6 High --- Fast OFF** for Phase A.

Restart crosses Application run-state ownership, CharacterVirtual reset,
moving-platform kinematic state, the dynamic Jolt body, camera snapping,
input priority, and Release UI. The architectural inspection matters
more than raw implementation speed.

If Phase A produces a clean narrow reset contract, Phase B may use Grok
4.6 High Fast.

------------------------------------------------------------------------

## Workflow

``` text
main clean
    ↓
define M22
    ↓
update docs/MILESTONES.md
    ↓
create milestone branch
    ↓
Phase A
    ↓
review
    ↓
Phase B
    ↓
review
    ↓
Phase C / manual tests
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
