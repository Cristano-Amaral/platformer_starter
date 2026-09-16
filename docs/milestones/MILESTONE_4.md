## Milestone 04 — Vertical Motion Foundation

### Goal

Introduce basic vertical player motion for the platformer.

The player must be able to jump, fall under gravity and land on a simple ground plane.

This milestone establishes the minimum vertical gameplay state required for future platform collision work.

Do not integrate Jolt yet.

### Requirements

- Preserve all validated Milestone 03 behavior.
- Keep CMake as the canonical build system.
- Preserve semantic horizontal input.
- Add a semantic Jump action.
- Default keyboard bindings:
  - Space = Jump
  - optionally Up Arrow = Jump
- Gameplay must not query raw keyboard keys directly.
- Add vertical velocity to the Player.
- Apply gravity every update.
- Allow jumping only while grounded.
- Resolve simple ground contact.
- Prevent the player from falling through the main ground.
- Reset vertical velocity appropriately when landing.
- Keep horizontal movement working while airborne.
- Keep movement frame-rate independent using delta time.
- Preserve the current camera behavior.
- Preserve resize, X close and ESC close behavior.

### World convention

Continue using:

- X = horizontal platformer movement;
- Y = vertical movement;
- Z = world depth.

Gravity acts toward negative Y.

The player remains fixed on Z.

### Semantic input

Extend the existing project-owned input state.

Conceptually:

    struct InputState
    {
        float moveX = 0.0f;
        bool jumpPressed = false;
    };

`jumpPressed` must represent a press event, not "key currently held".

Holding Space must not continuously restart the jump every frame.

Raw raylib input remains isolated in the input/backend layer.

### Player vertical state

Player should own the vertical gameplay state required for jumping and falling.

At minimum:

- position;
- vertical velocity;
- grounded state;
- tunable movement values.

Conceptually:

    float verticalVelocity = 0.0f;
    bool grounded = true;

Exact representation may differ if it remains simple.

### Tunable gameplay values

Keep important movement parameters clearly defined and easy to change.

At minimum:

- horizontal move speed;
- jump speed / jump impulse;
- gravity.

Example conceptual values:

    moveSpeed = 6.0 units/second
    jumpSpeed = 8.0 units/second
    gravity = 20.0 units/second²

These are starting values only and may be adjusted during implementation or manual testing.

Do not build a configuration system yet.

### Update order

The gameplay update should remain simple and deterministic.

Conceptually:

1. Apply semantic horizontal input.
2. If jump was pressed and player is grounded:
   - set positive vertical velocity;
   - mark player airborne.
3. Apply gravity to vertical velocity.
4. Integrate X and Y position using delta time.
5. Resolve simple ground contact.
6. Update grounded state.

Avoid unnecessary physics abstractions.

### Simple ground contact

For this milestone, collision only needs to support the main ground surface.

The main greybox ground currently has a known fixed height.

Use a simple ground plane / ground height test.

Account for the player's dimensions so the player's feet rest on top of the ground rather than its center being placed on the surface.

Conceptually:

    playerBottom = position.y - size.y * 0.5

If the player falls below the ground surface:

- place the player back on the ground;
- set vertical velocity to zero;
- set grounded = true.

Do not implement general box collision yet.

### Ground definition

Avoid silently duplicating unrelated level constants across gameplay and rendering.

Introduce the minimum shared world/greybox data needed so gameplay and rendering agree on the main ground height.

Do not create a full Level system, Scene system or ECS.

A small project-owned constant or minimal greybox/world definition is sufficient.

### Horizontal movement while airborne

The player must retain horizontal control while jumping.

For Milestone 04, use the same horizontal movement speed in the air as on the ground.

Do not implement:

- air acceleration;
- reduced air control;
- momentum;
- friction.

These may be tuned later.

### Camera

Keep the current camera behavior unchanged.

The camera may continue targeting the player's full current position, including Y.

Observe its behavior during jumps, but do not add:

- smoothing;
- vertical dead zone;
- look-ahead;
- camera limits;
- camera collision.

Camera improvements will be handled separately after vertical gameplay is validated.

### Architecture

Responsibilities should remain approximately:

Application
  ├── obtains delta time
  ├── polls semantic input
  ├── updates Player
  └── asks Renderer to draw

Input
  └── converts raw backend input into:
      - moveX
      - jumpPressed

Player
  ├── horizontal movement
  ├── jump state
  ├── gravity
  ├── vertical velocity
  └── simple ground resolution

Renderer
  └── reads state and renders it

Gameplay must remain independent from raylib APIs.

### Out of scope

Do NOT implement:

- Jolt
- general collision system
- collision with elevated platforms
- collision with platform sides
- collision with ceilings
- slopes
- moving platforms
- one-way platforms
- coyote time
- jump buffering
- variable jump height
- double jump
- wall jump
- dash
- acceleration
- deceleration
- friction
- knockback
- free movement on Z
- animation
- gamepad support
- sound effects
- ImGui
- ECS
- scene graph
- camera smoothing
- camera look-ahead
- camera dead zones
- Milestone 05

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. `cmake --build --preset windows-development` succeeds.
3. The application opens normally.
4. A / Left Arrow still move the player toward -X.
5. D / Right Arrow still move the player toward +X.
6. Space triggers a jump while grounded.
7. Holding Space does not repeatedly restart the jump every frame.
8. The player gains positive Y motion when jumping.
9. Gravity causes the player to fall back toward the ground.
10. The player lands on the main ground and does not fall through it.
11. Landing resets vertical velocity appropriately.
12. The player cannot jump again while airborne.
13. The player can jump again after landing.
14. Horizontal movement continues to work while airborne.
15. Player Z remains unchanged.
16. Movement and gravity use delta time.
17. Gameplay contains no direct raylib input calls.
18. Gameplay contains no direct raylib timing calls.
19. No Jolt integration exists.
20. No collision with elevated platforms has been implemented.
21. Resize still works.
22. X and ESC still close the application normally.
