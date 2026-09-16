## Milestone 13 — Jolt Moving Platform

### Goal

Add the first moving platform to the platformer using a Jolt kinematic body.

The Player must be able to:

- land on the moving platform;
- remain supported while the platform moves;
- be carried by the platform;
- walk relative to the platform while standing on it;
- jump normally from the moving platform;
- leave the platform normally.

Use the existing Jolt CharacterVirtual Player controller.

Do not implement a custom moving-platform attachment system.

Do not add new movement abilities.

### Scope

Implement exactly one test moving platform.

The platform:

- is a Jolt kinematic body;
- moves horizontally along the X axis;
- follows a deterministic back-and-forth path;
- does not rotate;
- does not move vertically;
- remains on the fixed gameplay Z plane;
- is rendered using project-owned state.

This milestone is specifically about validating:

    CharacterVirtual <-> kinematic moving ground

Do not build a generic platform system or level scripting system yet.

### Moving platform specification

Introduce a small project-owned moving-platform specification/state.

Conceptually it may contain:

    center
    size
    pathStartX
    pathEndX
    speed

and runtime state such as:

    currentPosition
    velocity
    direction

Exact naming may differ.

Do not expose Jolt types.

Use project-owned core/world types.

### Initial test platform

Add one moving platform to the greybox test environment.

Choose a location that is reachable using the existing Player jump:

    jump speed = 8.0
    gravity = 20.0

The platform must be easy to reach manually from an existing static platform.

The moving platform should have enough width that Player riding behavior can
be clearly observed.

Use a horizontal travel distance large enough to visibly verify platform
motion, but keep it inside the existing camera/test environment.

Document the exact:

- platform center;
- platform size;
- minimum X;
- maximum X;
- movement speed.

Do not add additional moving platforms.

### World data ownership

Static world geometry remains in GreyboxWorld.

Add project-owned moving-platform configuration/state without treating it as
static Greybox collision.

Do not duplicate moving-platform dimensions independently in Renderer and
PhysicsWorld.

There must be one project-owned source of truth for its dimensions/path.

PhysicsWorld converts that information into the Jolt kinematic body.

Renderer receives project-owned moving-platform state.

### Jolt body

Represent the moving platform as:

    EMotionType::Kinematic

Do not use:

    Static
    Dynamic

for the moving platform.

Use the existing Moving object layer if appropriate.

Do not create a special physics engine abstraction solely for this feature.

### Platform movement

Move the kinematic platform using the Jolt v5.6.0 API intended for kinematic
motion.

Prefer the appropriate BodyInterface kinematic movement API rather than
teleporting the body every frame.

The movement must have a physically meaningful velocity so CharacterVirtual
can obtain ground velocity from the supporting body.

The platform travels:

    start -> end -> start -> ...

at constant speed.

At endpoints reverse direction deterministically.

Do not use easing.

Do not pause at endpoints.

Do not use animation curves.

### CharacterVirtual moving-ground behavior

When CharacterVirtual is standing on the moving platform, correctly account
for the velocity of the supporting body.

Inspect the Jolt v5.6.0 CharacterVirtual implementation, documentation,
samples and tests before implementing this.

Use the pinned API version.

Pay particular attention to:

    CharacterVirtual::GetGroundVelocity()
    CharacterVirtual::UpdateGroundVelocity()

and the Jolt recommendation that when the character is OnGround and not
moving away from the surface, the intended velocity should account for the
ground velocity.

Do not manually attach the Player position to the platform.

Do not store a "parent platform" transform and manually add platform delta to
Player position unless Jolt's CharacterVirtual API demonstrably requires a
minimal equivalent mechanism.

Prefer native CharacterVirtual moving-ground behavior.

### Relative Player movement

Player semantic movement remains:

    moveX

Player gameplay movement speed remains relative to the supporting platform.

Example:

If the platform moves right at:

    +2 units/s

and Player has no movement input:

the Player should travel with the platform.

If Player runs right at its normal:

    +6 units/s

its intended relative gameplay speed remains +6 while standing on the
platform.

The final world-space character velocity may therefore include platform
ground velocity.

Do not change:

    max horizontal speed = 6.0
    acceleration = 40.0
    deceleration = 50.0

These remain Player-relative gameplay constants.

### Fixed gameplay plane

Player Z remains fixed.

Moving platform Z remains fixed.

Interaction with the moving platform must not introduce Z drift.

### Vertical movement

Preserve the corrected Milestone 11/12 vertical behavior.

Standing on the moving platform:

    grounded == true
    gameplay vertical velocity approximately 0

Do not accumulate downward gameplay velocity while supported.

Jumping from the moving platform:

    jump velocity approximately +8

A residual moving-platform ground contact must not cancel the jump.

Preserve:

    jump speed = 8.0
    gravity = 20.0

### Jump from moving platform

Jumping from the platform must work normally.

The Player should preserve the appropriate horizontal world motion at
takeoff.

Do not introduce unrealistic abrupt cancellation of the supporting
platform's motion simply because ground support was lost.

However, do not redesign the entire Player momentum model.

Use the simplest behavior consistent with CharacterVirtual and the existing
M07 movement model.

Document the final behavior.

### Coyote time

Preserve:

    coyote duration = 0.10 seconds

Walking or being carried off the moving platform must enter the same coyote
logic as leaving a static platform.

Do not create moving-platform-specific coyote logic unless necessary.

### Jump buffer

Preserve:

    jump buffer duration = 0.10 seconds

Buffered jumping immediately before landing on the moving platform must work
the same way as landing on static ground.

### Collision behavior

The moving platform is fully solid.

Player must:

- land on its top;
- collide with its sides;
- collide with its underside;
- remain supported on top.

The platform must not pass through the Player without CharacterVirtual
collision response.

Do not add crushing/damage behavior.

### Platform versus static world

Choose the test path so the moving platform does not intersect existing
static geometry during normal motion.

Do not implement moving-platform/static-platform collision gameplay.

The kinematic platform path should be authored to remain valid.

### Platform versus dynamic test box

The existing cyan dynamic test box remains.

If the moving platform naturally contacts the dynamic box, normal Jolt
kinematic-body interaction may occur.

However, do not intentionally place the test box in the platform path.

Do not tune this interaction.

The test box is not the focus of Milestone 13.

### Physics update ordering

Review the existing Milestone 12 update order before modifying it.

Moving-platform velocity must be available to CharacterVirtual at the correct
point in the frame.

Inspect the pinned Jolt v5.6.0 implementation/tests to determine the correct
ordering between:

- calculating desired moving-platform transform;
- moving the kinematic body;
- updating/refeshing CharacterVirtual ground velocity if required;
- setting CharacterVirtual desired velocity;
- CharacterVirtual::Update;
- PhysicsSystem::Update;
- synchronizing project-owned state.

Do not guess based on unrelated engine behavior.

Document the resulting update order.

Do not introduce a full fixed-timestep framework in this milestone.

### Rendering

Render the moving platform as another greybox-style platform.

Renderer must consume project-owned platform position and size.

Renderer must not:

- include Jolt headers;
- query BodyID;
- query PhysicsWorld using Jolt types.

The rendered platform must stay visually synchronized with its Jolt body.

### Camera

Preserve Milestone 08 camera behavior.

Camera follows the Player, not the platform.

Do not make the camera platform-relative.

Do not change:

    horizontal dead zone = 1.5
    vertical dead zone = 0.75
    follow sharpness = 8.0

Do not change offset or FOV.

### Debug metrics

Extend the Debug/Development metrics panel.

Add a read-only section:

    Moving Platform

Display at minimum:

- initialized/valid;
- position X/Y/Z;
- velocity X/Y/Z;
- movement direction;
- path minimum X;
- path maximum X;
- movement speed.

Extend Player physics diagnostics where useful with:

- ground velocity X/Y/Z;
- whether current valid ground is moving.

Do not expose raw BodyID values.

Do not expose Jolt types.

Do not add editable tuning controls.

### Ground velocity validation

The metrics panel must make this behavior observable.

When Player stands still on static ground:

    ground velocity approximately 0

When Player stands still on the moving platform:

    ground velocity X approximately equals platform velocity X

When Player jumps:

    moving-ground support is lost normally.

### Dependency versions

Do not change:

- raylib 6.0;
- Dear ImGui 1.92.7;
- rlImGui Raylib_6_0;
- Jolt Physics v5.6.0.

### Documentation

Update:

- AGENTS.md if required;
- docs/ARCHITECTURE.md;
- docs/MILESTONES.md;
- README.md.

Document that the physics test world now contains one Jolt kinematic moving
platform.

Do not document future moving-platform systems that have not been built.

### Out of scope

Do NOT implement:

- multiple moving platforms;
- vertical moving platforms;
- diagonal moving platforms;
- rotating platforms;
- elevators;
- platform waypoints;
- arbitrary paths;
- splines;
- easing;
- platform pause timers;
- player/platform parenting;
- generic transform hierarchy;
- crushing;
- damage;
- slopes;
- stairs;
- crouching;
- wall jump;
- wall slide;
- double jump;
- variable jump;
- triggers;
- sensors;
- collectibles;
- checkpoints;
- raycasts;
- level loading;
- asset cooker;
- editor;
- physics debug rendering;
- collision visualization;
- fixed timestep framework;
- render interpolation;
- networking;
- animation;
- audio;
- Milestone 14.

### Acceptance criteria

1. CMake configure succeeds.
2. Debug build succeeds.
3. Development build succeeds.
4. Release build succeeds.
5. Jolt remains v5.6.0.
6. Exactly one moving platform is added.
7. Moving platform uses a Jolt kinematic body.
8. Moving platform does not use a Dynamic body.
9. Moving platform moves only on X.
10. Moving platform Z remains fixed.
11. Platform moves deterministically between two X endpoints.
12. Platform reverses correctly at each endpoint.
13. Platform moves at constant configured speed.
14. Project-owned data defines its dimensions/path.
15. PhysicsWorld contains no separately duplicated geometry/path values.
16. Renderer uses project-owned platform state.
17. Renderer contains no Jolt types.
18. Player can land on the moving platform.
19. Player remains grounded while riding it.
20. Player is carried horizontally when giving no movement input.
21. Player does not visibly slide off a constantly moving platform while
    standing still.
22. Player can walk left/right relative to the platform.
23. Player can jump from the moving platform.
24. Jump velocity remains approximately 8.
25. Gameplay gravity remains 20.
26. Grounded vertical velocity remains approximately 0.
27. Player can walk/be carried off the edge and fall normally.
28. Coyote time remains 0.10.
29. Jump buffer remains 0.10.
30. Player collides with moving-platform sides.
31. Player collides with moving-platform underside.
32. Player Z remains fixed.
33. Static-platform collision still works.
34. Dynamic cyan test-box interaction still works.
35. Camera behavior remains unchanged.
36. Moving-platform debug metrics are visible in Debug/Development.
37. Ground velocity is observable in debug metrics.
38. Static-ground ground velocity is approximately zero.
39. Moving-platform ground velocity reflects platform motion.
40. Release contains no active debug UI.
41. Resize still works.
42. X and ESC still close normally.
43. No unrelated gameplay mechanics were added.
44. Milestone 14 was not started.
