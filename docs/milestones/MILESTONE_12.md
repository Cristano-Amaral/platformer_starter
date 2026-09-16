## Milestone 12 — Physics Cleanup and Consolidation

### Goal

Consolidate the post-Milestone-11 physics architecture.

Remove obsolete Player collision code that is no longer authoritative,
eliminate dead/duplicate physics paths, and make Jolt the single clear
runtime physics backend for Player/world collision.

Preserve all validated Milestone 11 gameplay behavior.

Do not add new gameplay mechanics.

### Current state

After Milestone 11:

- Player physical position and collision are authoritative from
  Jolt CharacterVirtual;
- static greybox collision exists in Jolt;
- the old custom AABB collision implementation remains in the repository but
  is no longer authoritative for Player movement.

Milestone 12 should remove or retire obsolete runtime code so the project no
longer carries two competing Player collision implementations.

### Legacy collision cleanup

Inspect the legacy custom collision system and all call sites.

If the legacy Collision implementation is no longer used at runtime:

- remove it from the build;
- remove unused source/header files if safe;
- remove obsolete includes;
- remove obsolete comments that refer to it as a possible active backend;
- remove dead helper functions/constants that only existed for the custom
  AABB controller.

Do not delete shared world geometry that is still used by rendering or Jolt.

Do not remove useful project-owned geometry structures simply because the
old collision code used them.

If any legacy collision code still has a legitimate non-Player use, keep only
the minimal portion that remains necessary and document why.

### Single source of truth for static geometry

`GreyboxWorld` remains the project-owned source of truth for the current test
environment.

Renderer and Jolt static collision must continue deriving from the same
project-owned box definitions.

Do not duplicate:

- ground dimensions;
- platform positions;
- platform sizes

inside PhysicsWorld or Renderer.

If current code requires unnecessary conversion duplication, consolidate it
behind small project-owned helper functions.

Do not introduce a generic scene graph or entity system.

### Physics ownership

Keep the existing ownership model:

    Application
        owns PhysicsWorld

    PhysicsWorld
        owns Jolt runtime
        owns CharacterVirtual
        owns static Jolt bodies
        owns dynamic test box

Player owns gameplay movement policy.

PhysicsWorld owns physical collision/integration.

Renderer consumes project-owned transforms only.

PlatformerCamera consumes project-owned Player position only.

### PhysicsWorld public API review

Review the current PhysicsWorld public API.

Remove methods that exist only for the retired custom collision path.

Keep the public surface small and project-owned.

Prefer APIs such as conceptually:

    Initialize()
    Update(...)
    Shutdown()

    InitializePlayer(...)
    UpdatePlayer(...)
    GetPlayerPhysicsState()

    GetDynamicTestBox()

Exact names may differ.

Do not expose Jolt types.

Do not introduce a generic physics-engine abstraction framework.

### Player cleanup

Review Player after the CharacterVirtual migration.

Remove obsolete fields/functions that only supported manual collision
integration.

Examples may include old:

- previous-position support checks;
- manual AABB resolution helpers;
- legacy support snapping state;
- manual collision geometry references.

Only remove code that is demonstrably unused after Milestone 11.

Preserve gameplay state used for:

- horizontal acceleration/deceleration;
- vertical velocity;
- grounded state;
- coyote time;
- jump buffer;
- semantic input.

### CharacterVirtual invariants

Preserve the corrected vertical-velocity behavior from Milestone 11.

Required invariants:

Standing on valid support:

    grounded == true
    verticalVelocity approximately 0

Jump takeoff:

    verticalVelocity positive
    approximately jump speed minus current-frame gravity integration

Jump apex:

    verticalVelocity crosses approximately 0

Falling:

    verticalVelocity becomes progressively negative

Landing:

    verticalVelocity returns to approximately 0

Walking off a ledge:

    verticalVelocity begins near 0 and accelerates downward normally

Do not regress these behaviors while removing legacy code.

### Dynamic test box

Keep the Jolt dynamic test box for now as a physics integration test.

The Player must still:

- collide with the box;
- not pass through it;
- physically influence it through normal CharacterVirtual/Jolt interaction.

Do not remove the test box in this milestone.

Do not turn it into gameplay.

### Debug metrics cleanup

Keep the existing Debug/Development metrics panel.

Retain:

#### Frame
- FPS
- delta time

#### Player
- position
- horizontal velocity
- vertical velocity
- grounded
- movement constants
- coyote state
- jump buffer state

#### Character physics
- CharacterVirtual initialized
- physical Player position
- ground support state

#### Camera
- desired target
- smoothed target
- dead zones
- follow sharpness

#### Physics
- Jolt initialized
- dynamic test box position
- dynamic test box active/sleeping state

Remove metrics that only existed to compare against the retired custom AABB
backend, if any.

Do not add editable controls.

### Physics diagnostics

Add a small number of read-only sanity diagnostics if they can be exposed
without leaking Jolt types.

Useful examples:

- static body count created for GreyboxWorld;
- CharacterVirtual valid/initialized;
- dynamic test body valid;
- Player physical position finite;
- Player velocity finite.

Do not build a profiler or generic diagnostics framework.

### Static body creation

Review how Jolt static bodies are created from GreyboxWorld.

Use a small reusable conversion path for project-owned boxes.

For each static greybox:

- convert project center/size to Jolt shape;
- create static body;
- place it in the correct non-moving layer.

Avoid repeated hand-written body creation code for each platform.

Do not introduce asset-driven level loading yet.

### Error handling

If static body creation fails:

- initialization must fail clearly;
- report which project-owned box failed where practical;
- avoid partially initialized runtime state.

Preserve clean shutdown behavior.

### Build cleanup

Review CMake after removing legacy collision files.

Remove source files from the target if they are no longer needed.

Do not leave dead sources in the build.

Do not change Jolt version.

Do not change raylib, Dear ImGui or rlImGui versions.

### Documentation

Update:

- AGENTS.md
- docs/ARCHITECTURE.md
- docs/MILESTONES.md
- README.md

to reflect that:

- Jolt CharacterVirtual is the authoritative Player collision system;
- legacy custom Player AABB collision has been removed or retired;
- GreyboxWorld is shared by rendering and Jolt static collision;
- Player gameplay policy remains separate from physics backend code.

Avoid documenting future systems that are not implemented.

### Out of scope

Do NOT implement:

- moving platforms
- elevators
- slopes
- stair stepping
- wall jump
- wall slide
- double jump
- variable jump
- crouching
- triggers
- sensors
- collectibles
- raycasts
- shape casts
- physics materials tuning
- friction tuning
- restitution tuning
- CCD tuning
- fixed timestep framework
- render interpolation
- physics debug renderer
- collision visualization
- level loading
- asset cooker
- editor
- animation
- audio
- Milestone 13

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. Debug build succeeds.
3. Development build succeeds.
4. Release build succeeds.
5. Jolt remains pinned to v5.6.0.
6. CharacterVirtual remains the authoritative Player collision system.
7. Player no longer calls the old custom AABB collision system.
8. Obsolete custom Player collision code is removed from the build.
9. Obsolete custom Player collision files are removed if fully unused.
10. No unused collision includes remain in Player.
11. GreyboxWorld remains the shared geometry source for Renderer and Jolt.
12. Ground geometry is not duplicated in PhysicsWorld.
13. Elevated-platform geometry is not duplicated in PhysicsWorld.
14. PhysicsWorld public API remains project-owned and free of Jolt types.
15. Player remains free of Jolt headers.
16. Renderer remains free of Jolt headers.
17. PlatformerCamera remains free of Jolt headers.
18. Player horizontal movement remains functional.
19. Player jump remains functional.
20. Player grounded state remains stable.
21. Grounded vertical velocity remains approximately 0.
22. Falling from ledges begins near 0 vertical velocity.
23. Jump arc remains equivalent to Milestone 11.
24. Coyote time remains functional.
25. Jump buffering remains functional.
26. Player still collides with static platforms.
27. Player still collides with platform sides.
28. Player still collides with platform undersides.
29. Player still collides with the dynamic Jolt box.
30. Dynamic test box still reacts physically.
31. Camera behavior remains unchanged.
32. Debug metrics remain functional.
33. Release still has no debug UI.
34. Resize still works.
35. X and ESC still close normally.
36. No new gameplay mechanics were added.
37. Milestone 13 has not been started.
