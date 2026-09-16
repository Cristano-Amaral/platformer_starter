## Milestone 06 — Solid Static AABB Collision

### Goal

Extend the current kinematic player collision so static greybox boxes behave as simple solid obstacles.

The player must collide with:

- platform tops;
- platform sides;
- platform undersides / ceilings.

Keep the existing custom kinematic controller.

Do not integrate Jolt yet.

### Requirements

- Preserve all validated Milestone 05 behavior.
- Keep CMake as the canonical build system.
- Preserve semantic input.
- Preserve horizontal movement, jump and gravity.
- Preserve delta-time-based movement.
- Keep player movement constrained to X/Y.
- Keep player Z unchanged.
- Continue using shared project-owned world geometry.
- Treat the main ground and elevated platforms as static axis-aligned collision boxes.
- Preserve top landing behavior.
- Add horizontal side collision.
- Add underside / ceiling collision.
- Prevent the player from passing horizontally through solid platforms.
- Prevent the player from passing upward through the underside of a platform.
- Preserve stable standing on platform tops.
- Preserve walking off edges and falling.
- Preserve jumping from platforms.
- Preserve resize, X close and ESC close.

### Collision model

Use simple axis-aligned bounding boxes (AABB).

Both player and static world boxes must use project-owned geometry/math types.

Do not use raylib collision helpers in gameplay/world collision code.

For this milestone, the player remains an axis-aligned box.

No rotation is required.

### Player bounds

Collision must use the player's full extents.

Conceptually:

    playerMinX = position.x - size.x * 0.5
    playerMaxX = position.x + size.x * 0.5

    playerMinY = position.y - size.y * 0.5
    playerMaxY = position.y + size.y * 0.5

    playerMinZ = position.z - size.z * 0.5
    playerMaxZ = position.z + size.z * 0.5

Equivalent bounds apply to world boxes.

### Axis-separated movement

Prefer resolving movement one axis at a time.

Conceptually:

1. start from previous position;
2. apply horizontal X movement;
3. resolve X collisions;
4. process jump/gravity;
5. apply vertical Y movement;
6. resolve Y collisions;
7. update grounded state;
8. render.

Do not implement a general iterative physics solver.

### Horizontal side collision

When moving along X:

- if the player's box overlaps a static box in Y and Z;
- and the player's X movement crosses into the box;
- stop the player at the contacted side.

Moving right:

    playerRight = boxLeft

Moving left:

    playerLeft = boxRight

The player must not penetrate the box.

Do not change vertical velocity due to side collision.

### Ceiling collision

When moving upward:

- if the player overlaps a static box in X and Z;
- and the player's top crosses the box bottom;
- stop upward movement at the underside;
- place the player immediately below the box;
- set vertical velocity to zero;
- keep grounded = false.

The player must then fall due to gravity on subsequent updates.

### Ground / top collision

Preserve existing landing behavior:

- only resolve top support while descending;
- snap the player's feet to the support surface;
- zero downward vertical velocity;
- set grounded = true.

The player must remain stable while supported.

### Collision ordering

Collision behavior must be deterministic.

For horizontal movement:

- resolve the nearest valid blocking surface in the direction of travel.

For vertical movement:

- when moving upward, resolve the nearest valid ceiling;
- when moving downward, resolve the highest valid supporting surface crossed.

Do not depend on array ordering when multiple boxes could be candidates.

### World geometry

Continue using shared world geometry.

Renderer and collision code must refer to the same static boxes.

Do not duplicate positions or sizes in Renderer.

### Main ground behavior

The finite main ground box should also behave as a solid static box where relevant.

The player may still fall off its left or right edge.

Do not create invisible infinite ground.

### Architecture

Responsibilities should remain approximately:

Application
  ├── Input
  ├── Time
  ├── Player movement
  ├── World collision resolution
  └── Renderer

Player
  └── owns movement state:
      - position
      - vertical velocity
      - grounded

World / Collision
  └── owns static geometry queries and collision resolution

Renderer
  └── reads the same world geometry and player state

Gameplay/world collision must remain independent from raylib.

### Suggested structure

game/source/
  gameplay/
    Player.h
    Player.cpp

  world/
    GreyboxWorld.h
    Collision.h
    Collision.cpp

No new collision framework is required if the existing files can be cleanly extended.

### Out of scope

Do NOT implement:

- Jolt
- slopes
- rotated collision boxes
- moving platforms
- dynamic rigid bodies
- pushing objects
- friction simulation
- acceleration/deceleration
- step climbing
- ledge grabbing
- wall sliding
- wall jumping
- coyote time
- jump buffering
- variable jump height
- double jump
- dash
- Z-axis movement
- capsule collision
- swept general-purpose collision
- arbitrary meshes
- character controller library
- ECS
- scene graph
- animation
- audio
- ImGui
- camera improvements
- Milestone 07

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. `cmake --build --preset windows-development` succeeds.
3. Application opens normally.
4. Existing horizontal movement still works.
5. Existing jump and gravity still work.
6. Existing top-platform landing still works.
7. Player remains stable while standing on a platform.
8. Player can walk off a platform and fall.
9. Moving into the side of a platform stops the player.
10. Player does not pass horizontally through a platform.
11. Side collision does not incorrectly set grounded.
12. Jumping into the underside of a platform stops upward movement.
13. Ceiling collision resets upward vertical velocity.
14. After hitting a ceiling, gravity makes the player fall.
15. Player does not pass upward through a platform.
16. Player can still land on top of that platform later.
17. X collision uses player/world extents rather than center-only tests.
18. Y collision uses player/world extents rather than center-only tests.
19. Z overlap is still considered for collision.
20. Collision chooses the nearest valid blocking surface deterministically.
21. Renderer and collision still share the same world geometry.
22. Gameplay/world collision contains no raylib dependencies.
23. Player Z remains unchanged.
24. No Jolt integration exists.
25. Resize still works.
26. X and ESC still close normally.
