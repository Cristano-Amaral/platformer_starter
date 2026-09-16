## Milestone 05 — Static Platform Collision

### Goal

Make the greybox platforms participate in gameplay collision.

The player must be able to land on top of static platforms while preserving the existing horizontal movement, jump and gravity behavior.

This milestone introduces minimal world collision data and simple top-surface landing against axis-aligned boxes.

Do not integrate Jolt yet.

### Requirements

- Preserve all validated Milestone 04 behavior.
- Keep CMake as the canonical build system.
- Preserve semantic horizontal movement.
- Preserve jump, gravity and grounded state.
- Preserve frame-rate-independent movement.
- Keep player movement constrained to X/Y.
- Keep player Z unchanged.
- Make the existing elevated greybox platforms part of shared world data.
- Renderer and collision logic must use the same platform definitions.
- Allow the player to land on top of static platforms.
- Prevent falling through a platform when descending onto it.
- Allow the player to walk horizontally across the top of a platform.
- When the player walks off a platform, grounded state must become false and gravity must make the player fall.
- The player must still land correctly on the main ground.
- Preserve current camera behavior.
- Preserve resize, X close and ESC close behavior.

### World collision representation

Introduce a minimal project-owned representation for static platform geometry.

A platform may conceptually contain:

    center
    size

using project-owned math types such as `core::Vec3`.

The same data must be used by:

- Renderer for drawing;
- gameplay collision for landing tests.

Do not maintain one set of platform coordinates in Renderer and another independent set in gameplay.

### Collision scope

For Milestone 05, only top-surface landing is required.

Support:

- player falling downward;
- horizontal overlap with a platform;
- crossing the platform top during the current frame;
- snapping the player's feet onto the platform top;
- resetting downward vertical velocity;
- setting grounded state.

Do NOT implement full solid-box collision yet.

### Landing test

Use the player's feet and horizontal footprint.

A platform landing should only occur when:

1. the player is moving downward or stationary vertically;
2. the player's X footprint overlaps the platform X extent;
3. the player's Z footprint overlaps the platform Z extent;
4. the player's feet cross the platform top from above during the frame.

The collision test must distinguish descending onto a platform from being below it.

Do not allow the player to teleport upward onto a platform simply because their current position overlaps it.

### Previous-position requirement

To avoid tunneling through thin platforms, collision resolution should consider the player's previous vertical position or previous foot position.

Conceptually:

    previousBottomY >= platformTopY
    currentBottomY <= platformTopY

combined with horizontal overlap.

The exact implementation may differ if it correctly detects downward crossing.

### Grounded state

The player is grounded only when supported by a valid surface.

Supported surfaces for this milestone:

- main ground;
- static greybox platforms.

When standing on a platform:

- vertical velocity = 0;
- grounded = true.

When walking beyond its horizontal support:

- grounded must become false;
- gravity resumes.

### Collision ownership

Do not embed individual hardcoded platform checks directly in `Player.cpp`.

Prefer a minimal world/collision query such as:

    ResolvePlayerGroundContact(...)

or equivalent.

Player owns movement state.

World/collision code owns knowledge of static collision geometry.

Keep the design small.

### Suggested structure

game/source/
  core/
    Vec3.h

  gameplay/
    Player.h
    Player.cpp
    PlatformerCamera.h

  world/
    GreyboxWorld.h
    GreyboxWorld.cpp
    Collision.h
    Collision.cpp

  render/
    Renderer.h
    Renderer.cpp

The exact organization may vary if a simpler design preserves the same separation.

If the existing `gameplay/Greybox.h` can be cleanly evolved or moved instead of creating more files, prefer the smaller solution.

### Update flow

Conceptually:

1. poll semantic input;
2. obtain delta time;
3. remember previous player position;
4. apply horizontal movement;
5. process jump;
6. apply gravity;
7. integrate vertical motion;
8. resolve ground/platform support against shared world geometry;
9. update grounded state;
10. render.

### Horizontal overlap

Collision should account for the player's actual size.

Do not test only the player's center point.

Conceptually:

    playerMinX = position.x - size.x * 0.5
    playerMaxX = position.x + size.x * 0.5

and equivalent bounds for the platform.

Z overlap should also use player/platform extents even though the player does not currently move on Z.

### Platform behavior

The two existing elevated greybox platforms should become collidable.

The player must be able to:

- jump onto them;
- stand on them;
- move across them;
- jump again while standing on them;
- walk off their edges and fall.

### Camera

Keep the existing camera behavior unchanged.

Do not add:

- smoothing;
- vertical dead zones;
- look-ahead;
- bounds;
- collision;
- shake.

### Architecture constraints

- Gameplay must remain independent from raylib.
- Collision/world code must remain independent from raylib.
- Renderer may convert project-owned world data to raylib types.
- Static platform geometry must not be duplicated across rendering and collision.
- Do not create an ECS.
- Do not create a scene graph.
- Do not create a generic physics engine.
- Do not add Jolt.
- Do not add GLM.
- Prefer explicit, testable code.

### Out of scope

Do NOT implement:

- Jolt
- side collisions against platforms
- ceiling collisions
- pushing against walls
- slopes
- moving platforms
- one-way platforms with drop-through
- ledge grabbing
- wall jumping
- wall sliding
- coyote time
- jump buffering
- variable jump height
- double jump
- dash
- acceleration
- friction
- knockback
- Z movement
- dynamic rigid bodies
- character controller library
- animation
- audio
- ImGui
- camera smoothing
- camera look-ahead
- camera dead zones
- Milestone 06

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. `cmake --build --preset windows-development` succeeds.
3. The application opens normally.
4. Existing horizontal movement still works.
5. Existing jump and gravity still work.
6. The main ground still supports the player.
7. The first elevated greybox platform supports the player.
8. The second elevated greybox platform supports the player.
9. The player can jump onto an elevated platform from below/nearby when physically reachable.
10. The player does not fall through a platform when descending onto its top.
11. The player can stand still on a platform without sinking or jittering visibly.
12. The player can move horizontally while standing on a platform.
13. The player can jump again while standing on a platform.
14. Walking off a platform causes the player to fall.
15. Landing detection uses player/platform extents rather than player center only.
16. Landing detection accounts for downward crossing of the platform top.
17. Platform geometry is shared between rendering and collision rather than duplicated.
18. Gameplay contains no raylib collision calls or raylib types.
19. Collision/world code contains no raylib dependencies.
20. Player Z remains unchanged.
21. No Jolt integration exists.
22. No platform side/ceiling collision is implemented.
23. Resize still works.
24. X and ESC still close normally.
