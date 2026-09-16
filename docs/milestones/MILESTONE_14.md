## Milestone 14 — Slopes and CharacterVirtual Ground Handling

### Goal

Add one static slope to the greybox test environment and validate
CharacterVirtual ground handling on inclined surfaces.

The Player must be able to:

- walk up a valid slope;
- walk down a valid slope;
- remain supported on the slope;
- jump from the slope;
- transition between flat ground and slope;
- distinguish walkable ground from steep ground.

Preserve all validated behavior from Milestones 07–13.

Do not implement stairs, step-up, crouching or new movement abilities.

### Scope

Implement exactly one primary walkable slope.

Optionally add one small steep-slope test surface only if it is required to
validate `OnSteepGround` cleanly.

Do not create a general terrain system.

Do not add arbitrary mesh collision.

Use simple project-owned test geometry.

### Slope geometry

Add a project-owned slope specification.

Conceptually it should contain:

    center / transform
    size
    rotation
    slope angle

Exact representation may differ.

The slope must be generated from project-owned data.

Renderer and PhysicsWorld must derive from the same slope specification.

Do not independently hardcode separate render and collision transforms.

### Walkable slope angle

Use a deliberately walkable test angle below the current CharacterVirtual
maximum slope angle.

Current CharacterVirtual max slope angle is:

    50 degrees

Choose a primary test slope around:

    30 degrees

or another clearly walkable value below the configured limit.

Document the exact final angle.

Do not change the CharacterVirtual max slope angle unless required by a real
bug.

### Optional steep-slope test

If needed for direct validation of `OnSteepGround`, add one small static test
surface with an angle above the CharacterVirtual maximum slope angle.

For example:

    walkable slope: 30 degrees
    max slope:      50 degrees
    steep test:     60 degrees

Only add the steep test if it remains simple and does not obstruct the main
test area.

It must not become a gameplay feature.

### Jolt shape

Represent the slope using a Jolt static collision shape appropriate to the
simple greybox geometry.

Prefer a simple transformed BoxShape or another convex primitive if it
correctly represents the intended ramp.

Do not introduce mesh collision unless necessary.

Do not add a general triangle-mesh level pipeline.

### Static body

The slope is:

    EMotionType::Static

Use the existing non-moving collision layer.

Do not make the slope kinematic or dynamic.

### CharacterVirtual ground state

Use CharacterVirtual ground information as the authority for support.

Preserve the project-owned ground-state translation.

At minimum distinguish:

- OnGround
- OnSteepGround
- NotSupported
- InAir

Do not expose Jolt enums outside PhysicsWorld.cpp.

### Walkable slope behavior

On a valid walkable slope:

- Player should remain supported;
- Player should not bounce or jitter;
- Player should be able to walk up;
- Player should be able to walk down;
- Player should not slowly slide down while standing still unless Jolt's
  intended behavior for the current configuration makes that unavoidable.

Prefer stable platformer behavior.

Do not add custom slope forces unless necessary.

### Steep ground behavior

If a steep test surface is present:

- CharacterVirtual should identify it as steep/non-walkable support;
- Player must not treat it as normal grounded support for jump refresh;
- Player must not gain infinite coyote/jump refresh while contacting it;
- Player may slide/fall according to CharacterVirtual behavior.

Do not create custom climbing logic.

### Horizontal movement

Preserve M07 movement constants:

    max speed = 6.0
    acceleration = 40.0
    deceleration = 50.0

Player movement intent remains semantic X-axis movement.

Do not introduce free Z movement.

CharacterVirtual should resolve movement against the slope surface.

Do not rewrite movement using slope-specific trigonometric movement unless
required.

### Vertical behavior

Preserve:

    jump speed = 8.0
    gameplay gravity = 20.0

Standing on valid walkable slope:

    grounded == true
    gameplay verticalVelocity approximately 0

Do not accumulate negative vertical velocity while stably supported.

Jumping from the slope:

    verticalVelocity becomes positive
    near the configured jump speed

Do not make jump direction slope-normal.

Jump remains world-up.

### Coyote time

Preserve:

    0.10 seconds

Leaving the top or side of a walkable slope should enter the same coyote
logic as leaving flat ground.

Do not refresh coyote from invalid steep support.

### Jump buffer

Preserve:

    0.10 seconds

Buffered jump when landing on a walkable slope must work.

Do not consume buffered jump from a non-walkable steep contact unless that
contact is considered valid support by the project-owned ground rules.

### Moving platform compatibility

Preserve Milestone 13.

The moving platform remains unchanged.

Do not combine moving-platform motion with the slope.

Do not put the slope on the moving platform.

### Ground velocity

Static slope ground velocity must be approximately:

    (0, 0, 0)

Do not regress moving-platform ground velocity behavior.

### Fixed gameplay plane

Player Z remains fixed.

Slope geometry must be positioned so the Player can traverse it while
remaining on the gameplay plane.

Do not introduce depth movement.

### Camera

Preserve Milestone 08 camera behavior.

The camera may naturally follow the Player vertically while ascending and
descending the slope.

Do not retune:

    horizontal dead zone = 1.5
    vertical dead zone = 0.75
    follow sharpness = 8.0

Do not introduce slope-specific camera behavior.

### Renderer

Render the slope using the same project-owned slope specification used by
PhysicsWorld.

Renderer must not include Jolt types.

Use a simple greybox visual.

Do not add materials or textures for the slope.

### Debug metrics

Extend Debug/Development metrics with a small slope/ground diagnostics section.

Display at minimum:

- Player ground support state;
- grounded;
- ground normal X/Y/Z;
- ground velocity X/Y/Z;
- current ground slope angle in degrees if cleanly derivable;
- whether current ground is considered walkable.

Do not expose raw Jolt enums or BodyID.

Keep metrics read-only.

### Ground normal

Expose the CharacterVirtual ground normal through a project-owned Vec3.

Use it only for diagnostics / classification in this milestone.

Do not yet use ground normal to drive animation, camera tilt or movement
effects.

### Walkable classification

Create a project-owned readable classification such as:

    walkable
    steep
    unsupported

Exact naming may differ.

Do not duplicate Jolt's internal enum directly.

The classification should be sufficient for gameplay rules:

- refresh grounded/coyote only on valid walkable support;
- do not treat steep support as normal grounded terrain.

### CharacterVirtual settings

Preserve current CharacterVirtual settings unless a minimal slope-related fix
is required.

Current max slope angle should remain approximately:

    50 degrees

Do not retune:

- mass
- max strength
- padding
- collision tolerance
- penetration recovery

unless required to correct a demonstrated slope bug.

Any changed setting must be documented and justified.

### Static-world integration

Extend the current project-owned world description with slope data.

Existing:

- main ground;
- elevated platforms;
- moving platform;
- dynamic cyan test box

must remain intact.

Do not break the current static box source of truth.

Do not convert all world geometry into a new scene system.

### Out of scope

Do NOT implement:

- stairs
- step-up
- stair stepping
- automatic ledge climbing
- crouching
- capsule resizing
- wall jump
- wall slide
- double jump
- variable jump
- slope-based acceleration
- skiing/sliding mechanics
- slope friction gameplay tuning
- terrain mesh
- arbitrary mesh collision
- heightmaps
- navmesh
- moving slopes
- rotating platforms
- vertical moving platforms
- triggers
- sensors
- collectibles
- checkpoints
- damage
- animation
- audio
- level loading
- asset cooker
- editor
- fixed timestep framework
- render interpolation
- Milestone 15

### Acceptance criteria

1. CMake configure succeeds.
2. Debug build succeeds.
3. Development build succeeds.
4. Release build succeeds.
5. Jolt remains v5.6.0.
6. One primary walkable slope exists.
7. Walkable slope angle is documented.
8. Walkable slope angle remains below CharacterVirtual max slope angle.
9. Slope is a Jolt static body.
10. Slope uses project-owned geometry data.
11. Renderer and PhysicsWorld share the same slope specification.
12. Renderer contains no Jolt types.
13. Player can approach the slope from flat ground.
14. Player can walk up the slope.
15. Player can walk down the slope.
16. Player remains stable while standing on the slope.
17. Player does not visibly jitter on the slope.
18. Player remains grounded on valid walkable slope.
19. Grounded vertical velocity remains approximately 0.
20. Player can jump from the slope.
21. Jump remains world-up.
22. Jump speed remains approximately 8.
23. Gameplay gravity remains 20.
24. Coyote time remains 0.10.
25. Jump buffer remains 0.10.
26. Leaving the slope starts normal falling/coyote behavior.
27. Ground normal is exposed through project-owned state.
28. Ground slope angle is visible in debug metrics if implemented.
29. Walkable/steep classification is visible in debug metrics.
30. Static-slope ground velocity is approximately zero.
31. Player Z remains fixed.
32. Existing static platforms still work.
33. Moving platform still works.
34. Moving-platform ground velocity still works.
35. Dynamic cyan test box still works.
36. Camera behavior remains unchanged.
37. F1/debug metrics still work.
38. Release still has no active debug UI.
39. Resize still works.
40. X and ESC still close normally.
41. No stairs/step-up system was added.
42. No unrelated mechanics were added.
43. Milestone 15 was not started.
