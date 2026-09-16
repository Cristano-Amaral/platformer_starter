## Milestone 10 — Experimental Jolt Physics Integration

### Goal

Integrate Jolt Physics into the project behind the existing project-owned
physics layer without replacing the validated custom Player controller.

Create a minimal experimental Jolt physics world and prove that:

- Jolt initializes correctly;
- the physics world steps every frame;
- static and dynamic rigid bodies can exist;
- a simple dynamic test body falls under gravity and collides with a static
  floor;
- the experiment can be observed without changing Player gameplay behavior;
- Jolt shuts down cleanly.

The existing Player movement and custom static AABB collision remain the
authoritative gameplay implementation in this milestone.

### Dependency

Integrate Jolt Physics using CMake.

Pin the dependency explicitly to:

    v5.6.0

Do not track:

    master
    main
    latest

Use Jolt's supported CMake integration.

Keep CMake as the canonical project build system.

### Architecture

Jolt must remain behind the project-owned physics layer.

Do not include Jolt headers in:

- Player
- PlatformerCamera
- Renderer
- Input
- world geometry headers
- other gameplay-facing public headers

Create a small physics abstraction/implementation boundary.

Suggested structure:

    physics/
        PhysicsWorld.h
        PhysicsWorld.cpp

and, if useful:

    physics/jolt/
        JoltPhysicsWorld.cpp
        JoltPhysicsWorld.h

Exact names may differ if the current architecture suggests something
simpler.

Project gameplay code should not manipulate JPH::BodyID, JPH::Vec3,
JPH::PhysicsSystem or other Jolt types directly.

### Scope of the experimental world

Create one independent Jolt test scene containing:

1. one static floor body;
2. one dynamic box body above it.

Suggested test geometry:

Static floor:
- center approximately {0, -0.25, 0}
- size approximately {24, 0.5, 8}

Dynamic box:
- center approximately {0, 5, 0}
- size approximately {1, 1, 1}

The exact dynamic start height may change slightly if required.

The dynamic box should:

- begin above the floor;
- fall under Jolt gravity;
- collide with the Jolt static floor;
- settle normally.

This body is an experimental physics object.

It is NOT the Player.

### Player behavior

Do not migrate Player movement to Jolt.

Preserve the existing custom controller exactly unless a build/integration
fix absolutely requires otherwise.

Preserve:

- acceleration/deceleration;
- jump/gravity;
- coyote time;
- jump buffering;
- custom solid AABB collision;
- platform support;
- ceiling collision;
- side collision;
- fixed player Z.

Player remains controlled by the existing project-owned movement code.

### Existing world collision

Do not remove or replace the current custom Greybox collision system.

The current static geometry continues to drive Player collision.

For this milestone, it is acceptable for the Jolt static floor to duplicate
the existing ground geometry because it belongs to an isolated experimental
physics scene.

Do not attempt to unify all world geometry with Jolt yet.

### Jolt initialization

Initialize the minimum required Jolt runtime infrastructure correctly.

This includes the required Jolt initialization steps for the pinned version.

Keep all Jolt-specific initialization inside the physics implementation
layer.

Application should interact with project-owned PhysicsWorld methods such as:

    Initialize
    Update
    Shutdown

rather than directly calling Jolt APIs.

### Physics update

Step the physics world from the Application update loop using deltaSeconds.

Do not tie physics stepping to rendering.

Use an explicit physics update method.

The first implementation may use the frame delta directly if it remains
stable for this isolated experiment.

Do not build a general fixed-timestep framework in this milestone unless
Jolt integration strictly requires it.

If delta time is clamped for safety, keep the rule simple and documented.

### Body state exposure

Expose only the minimum project-owned information required to render or
inspect the experimental dynamic box.

For example:

    core::Vec3 DynamicTestBoxPosition() const;

Do not expose Jolt body handles outside the physics layer.

Renderer must not query Jolt directly.

### Rendering

Render the experimental dynamic box using the existing Renderer path.

Renderer receives project-owned transform data.

Renderer must not include Jolt headers.

Visually distinguish the experimental dynamic box from the Player and
greybox environment using geometry, size or another simple visual choice.

Do not introduce a new rendering system.

### Debug metrics

Extend the existing Debug/Development metrics panel with a small:

    Physics

section.

Display at minimum:

- Jolt initialized: true/false
- dynamic test box position X/Y/Z

If inexpensive and clean to expose, also show:

- active/sleeping state

Do not add a physics profiler or detailed Jolt statistics.

The metrics remain read-only.

Release builds must continue to contain no debug UI.

### Jolt configuration

Prefer conservative CPU settings compatible with future portability.

Do not enable optional CPU instruction sets solely for desktop performance.

Do not enable experimental GPU compute functionality.

Do not enable unnecessary Jolt samples, tests or tooling.

Build only what the game requires.

The integration should remain compatible in principle with future ARM
targets.

### Ownership and lifetime

PhysicsWorld owns all Jolt runtime state created for this experiment.

Initialization and shutdown must be deterministic.

Do not use globals for the project-owned physics world.

Do not leak Jolt objects or allocators.

Use project conventions for ownership and RAII where appropriate.

### Error handling

If Jolt initialization fails:

- fail initialization cleanly;
- report a useful diagnostic;
- do not continue with a partially initialized physics system.

Do not silently ignore creation failures.

### Update order

The conceptual frame order becomes:

1. poll input;
2. update Player using the existing custom controller;
3. update experimental PhysicsWorld;
4. update PlatformerCamera;
5. render world, Player and Jolt test body;
6. render Debug/Development UI if enabled.

The experimental PhysicsWorld must not modify Player state.

### Out of scope

Do NOT implement:

- Player migration to Jolt
- Jolt Character / CharacterVirtual
- Jolt-controlled Player
- replacement of custom AABB collision
- shared world collision generation
- moving platforms
- rigid-body gameplay
- physics-driven camera
- physics materials tuning
- friction gameplay tuning
- restitution gameplay tuning
- constraints
- joints
- triggers
- sensors
- raycasts
- shape casts
- continuous collision detection tuning
- fixed timestep framework
- physics interpolation
- rollback
- networking
- ragdolls
- vehicles
- destructibles
- physics debug renderer
- collision visualization
- editable debug physics values
- Jolt profiling UI
- level editor
- animation
- audio
- Milestone 11

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. Debug build succeeds.
3. Development build succeeds.
4. Release build succeeds.
5. Jolt dependency is pinned to an explicit version/tag.
6. Jolt initializes successfully.
7. Jolt shuts down cleanly.
8. PhysicsWorld owns Jolt-specific runtime state.
9. Application contains no direct Jolt API calls.
10. Player contains no Jolt dependency.
11. PlatformerCamera contains no Jolt dependency.
12. Renderer contains no Jolt dependency.
13. Gameplay-facing headers contain no Jolt types.
14. Existing custom Player movement remains intact.
15. Existing custom Player collision remains intact.
16. Existing coyote time remains intact.
17. Existing jump buffering remains intact.
18. Existing camera behavior remains intact.
19. Experimental Jolt static floor exists.
20. Experimental Jolt dynamic box exists.
21. Dynamic box falls under Jolt gravity.
22. Dynamic box collides with the Jolt floor.
23. Dynamic box settles without obvious instability.
24. Player is not controlled or affected by Jolt.
25. Renderer obtains test-body position through project-owned data.
26. Renderer does not query Jolt directly.
27. Debug/Development metrics show Jolt initialization state.
28. Debug/Development metrics show test-box position.
29. Release still has no debug metrics UI.
30. Resize continues to work.
31. X and ESC continue to close normally.
32. No Player migration to Jolt has started.
33. Milestone 11 has not been started.
