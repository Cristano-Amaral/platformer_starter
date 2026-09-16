## Milestone 08 — Platformer Camera Follow

### Goal

Improve the existing platformer camera so it follows the player smoothly
without being rigidly attached to every small player movement.

Add:

- persistent camera target state;
- horizontal follow dead zone;
- smooth camera target movement;
- more stable vertical follow behavior.

Preserve all validated Milestone 07 gameplay and collision behavior.

Do not integrate Jolt or ImGui yet.

### Requirements

- Preserve CMake as the canonical build system.
- Preserve all player movement behavior.
- Preserve acceleration/deceleration.
- Preserve coyote time.
- Preserve jump buffering.
- Preserve all static AABB collision behavior.
- Preserve fixed player Z.
- Preserve the existing perspective camera orientation and general framing.
- Keep camera gameplay logic independent from raylib.
- Keep camera behavior frame-rate independent.

### Camera ownership

`PlatformerCamera` should own persistent camera-follow state.

Do not derive the entire camera target directly from the player's current
position every render frame.

The camera should conceptually maintain:

    targetPosition
    offset
    fieldOfView

The exact representation may differ if it remains small and explicit.

Renderer should consume camera state rather than implement gameplay follow
rules.

### Horizontal dead zone

Add a horizontal dead zone around the camera target.

Small player movement inside this region must not move the camera target.

When the player moves beyond the dead-zone boundary:

- move the desired camera target only enough to keep the player at the
  boundary;
- do not immediately snap the camera target directly to the player.

Suggested starting value:

    horizontalDeadZone = 1.5 world units

This is a starting tuning value.

### Horizontal smoothing

Smooth the actual camera target toward the desired target.

The smoothing must:

- use delta time;
- behave consistently across different frame rates;
- avoid obvious snapping;
- remain responsive enough for platforming.

Prefer a frame-rate-independent exponential follow or another explicit
delta-time-based approach.

Do not use a fixed per-frame interpolation factor.

### Vertical behavior

The camera should no longer follow every vertical player movement exactly.

Add a small vertical dead zone around the camera target.

Suggested starting value:

    verticalDeadZone = 0.75 world units

While the player remains inside the vertical dead zone:

- keep the desired camera target Y unchanged.

When the player moves outside it:

- move the desired target only enough to keep the player at the boundary;
- smoothly follow that desired Y.

This should reduce unnecessary camera movement during small vertical changes
while still allowing the camera to follow meaningful height changes.

### Camera initialization

At application start:

- initialize the camera target from the player's initial position;
- avoid a visible interpolation from an unrelated default position.

No startup camera sweep should occur.

### Camera update

Camera follow state must update during the game update, not be hidden inside
rendering.

Conceptually:

1. update player;
2. resolve player collision;
3. update camera from final player position and delta time;
4. render using the resulting camera state.

### Renderer responsibility

Renderer may:

- convert project-owned vectors to raylib vectors;
- construct the backend camera representation;
- begin/end 3D rendering.

Renderer must not decide:

- dead-zone behavior;
- smoothing behavior;
- camera follow timing;
- player tracking rules.

Those belong to `PlatformerCamera`.

### Camera offset

Preserve approximately the existing camera framing:

    offset ≈ {2.0, 3.5, 12.0}

Do not redesign the perspective or viewing direction in this milestone.

Small changes are allowed only if required by the new follow behavior.

### Frame-rate independence

Camera smoothing must explicitly use `deltaSeconds`.

Avoid:

    position += (target - position) * 0.1

when `0.1` is simply a per-frame constant.

Use a delta-time-aware formulation.

### Architecture constraints

- `PlatformerCamera` remains project-owned and raylib-free.
- Renderer remains the raylib-facing camera adapter.
- Player must not know about the camera.
- Collision must not know about the camera.
- Do not move camera logic into Player.
- Do not introduce a generic camera framework.
- Do not create ECS or scene graph.
- Keep implementation small and explicit.

### Out of scope

Do NOT implement:

- Jolt
- ImGui
- debug metrics panel
- camera collision
- camera occlusion handling
- camera shake
- camera zoom
- dynamic FOV
- manual camera control
- mouse camera control
- camera rotation
- look-ahead based on player velocity
- direction-dependent framing
- camera bounds
- level-specific camera zones
- cinematic cameras
- split screen
- animation
- audio
- Milestone 09

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. `cmake --build --preset windows-development` succeeds.
3. Application opens normally.
4. Existing player movement remains intact.
5. Existing jump/gravity behavior remains intact.
6. Existing AABB collision remains intact.
7. Existing coyote time and jump buffering remain intact.
8. Small horizontal player movements inside the dead zone do not move the
   camera target.
9. Moving beyond the horizontal dead zone causes the camera to follow.
10. Horizontal camera movement is visibly smooth rather than snapping.
11. Small vertical player movements inside the vertical dead zone do not move
    the camera target.
12. Larger vertical movement causes the camera to follow.
13. Jumping no longer causes the camera to rigidly mirror every player Y
    movement.
14. Camera follow uses delta time.
15. Camera behavior remains stable at normal frame rates.
16. Camera initializes at the player's starting location without a startup
    sweep.
17. PlatformerCamera contains no raylib dependency.
18. Renderer does not own dead-zone or follow rules.
19. Player does not depend on the camera.
20. Player Z remains unchanged.
21. No Jolt integration exists.
22. No ImGui integration exists.
23. Resize still works.
24. X and ESC still close normally.
