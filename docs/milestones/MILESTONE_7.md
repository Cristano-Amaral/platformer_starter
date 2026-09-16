## Milestone 07 — Player Movement Feel

### Goal

Improve the responsiveness and feel of the existing custom kinematic player controller.

Add:

- horizontal acceleration;
- horizontal deceleration;
- coyote time;
- jump buffering.

Preserve all validated Milestone 06 collision behavior.

Do not integrate Jolt yet.

### Requirements

- Preserve CMake as the canonical build system.
- Preserve semantic input.
- Preserve solid static AABB collision.
- Preserve side collision.
- Preserve ceiling collision.
- Preserve stable top support.
- Preserve falling from edges.
- Preserve fixed Z.
- Preserve current camera behavior.
- Keep gameplay/world collision independent from raylib.
- Keep player movement frame-rate independent.

### Horizontal movement

Replace instantaneous horizontal velocity changes with a small project-owned horizontal velocity model.

The player should own horizontal velocity.

Conceptually:

    desiredVelocityX = input.moveX * maxMoveSpeed

Move current horizontal velocity toward the desired velocity using acceleration.

When there is no horizontal input, move horizontal velocity toward zero using deceleration.

Suggested starting values:

    maxMoveSpeed = 6.0 units/s
    acceleration = 40.0 units/s²
    deceleration = 50.0 units/s²

These values are starting points only.

Do not add friction simulation or a physics-material system.

### Horizontal collision

Preserve the Milestone 06 side-collision behavior.

If horizontal movement is blocked by a solid surface in the direction of travel:

- resolve player position as before;
- set horizontal velocity to zero in the blocked direction.

Do not allow horizontal velocity to keep accumulating into a wall.

### Air control

Use the same horizontal acceleration model while airborne for this milestone.

Do not add a separate air-control factor yet.

### Coyote time

Allow the player to jump for a short time after walking off a supporting surface.

Suggested starting value:

    coyoteTime = 0.10 seconds

The player should track time since it was last grounded.

A jump is allowed when:

- player is currently grounded; or
- time since leaving ground is within the coyote-time window.

Coyote time must not allow repeated jumps in midair.

Once a jump is consumed, that opportunity is gone until the player is grounded again.

### Jump buffering

If jump is pressed shortly before landing, remember the jump request for a short time.

Suggested starting value:

    jumpBufferTime = 0.10 seconds

When jump is pressed:

- store/reset a jump-buffer timer.

When the player becomes eligible to jump while the buffer is still active:

- consume the buffered jump immediately;
- apply jump velocity;
- clear the buffer.

Do not require the player to press jump on the exact landing frame.

### Jump interaction

A valid jump should:

- set vertical velocity to the existing jump speed;
- clear grounded state;
- consume coyote-time eligibility;
- consume the jump buffer.

Jumping into a ceiling must still stop upward motion as in Milestone 06.

### Timer behavior

All movement timers must use delta time.

Do not use frame counters.

Timers should be owned by Player unless a smaller existing responsibility is clearly more appropriate.

### Input

Keep semantic input.

Do not add raw raylib key checks to Player.

Continue using `jumpPressed` as a press event.

### Architecture constraints

- Do not modify collision geometry to make movement feel correct.
- Do not duplicate world geometry.
- Do not add Jolt.
- Do not create a general movement-state machine.
- Do not create an ECS.
- Do not create a scene graph.
- Do not create animation systems.
- Keep the implementation explicit and small.

### Out of scope

Do NOT implement:

- Jolt
- variable jump height
- jump-cut on button release
- double jump
- wall jump
- wall slide
- dash
- ledge grab
- slope handling
- step climbing
- moving platforms
- Z-axis movement
- character animation
- stamina
- sprint
- crouch
- physics materials
- camera smoothing
- camera look-ahead
- camera dead zones
- audio
- ImGui
- Milestone 08

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. `cmake --build --preset windows-development` succeeds.
3. Application opens normally.
4. Horizontal movement no longer jumps instantly from zero to full speed.
5. Releasing horizontal input decelerates the player smoothly to zero.
6. Maximum horizontal speed remains bounded.
7. Side collision still prevents passing through static boxes.
8. Horizontal velocity is cleared appropriately when blocked by a wall.
9. Existing jump height remains approximately unchanged.
10. Existing gravity behavior remains intact.
11. Existing ceiling collision remains intact.
12. Existing platform landing remains intact.
13. Existing edge falling remains intact.
14. Player can jump shortly after walking off an edge within the coyote-time window.
15. Player cannot use coyote time for repeated midair jumps.
16. Pressing jump shortly before landing triggers a jump on landing while the buffer is active.
17. Expired jump-buffer input does not cause a delayed jump later.
18. Jump buffer is consumed after a successful jump.
19. Coyote opportunity is consumed after a successful jump.
20. All new timers use delta time rather than frame counts.
21. Gameplay remains free of raylib input calls.
22. Collision/world code remains raylib-free.
23. Player Z remains unchanged.
24. No Jolt integration exists.
25. Resize still works.
26. X and ESC still close normally.

### Deferred validation

When the Debug/Development metrics panel is implemented, quantitatively
revalidate Milestone 07 movement feel:

- horizontal acceleration and time to max speed;
- horizontal deceleration and time to stop;
- coyote-time duration and consumption;
- jump-buffer duration and consumption.

Expose the relevant Player state/timers in the debug UI so these behaviors
can be verified numerically rather than only by visual observation.
