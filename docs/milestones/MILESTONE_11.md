## Milestone 11 — Player CharacterVirtual

### Goal

Migrate the Player's physical movement and world collision from the custom
kinematic AABB controller to Jolt CharacterVirtual.

Preserve the validated gameplay feel introduced in Milestone 07:

- horizontal acceleration;
- horizontal deceleration;
- jump speed;
- coyote time;
- jump buffering.

Preserve Milestone 08 camera behavior and Milestone 09 debug metrics.

The Player must now physically interact with the Jolt world.

The experimental dynamic test box from Milestone 10 should become a useful
interaction test: the Player must no longer pass through it.

### Jolt version

Continue using the already pinned:

    Jolt Physics v5.6.0

Do not change Jolt version in this milestone.

Use APIs compatible with the pinned version.

### Character controller

Use:

    JPH::CharacterVirtual

Do not use:

    JPH::Character

Do not implement a custom replacement for CharacterVirtual.

Keep all CharacterVirtual-specific Jolt types inside the physics layer.

### Architecture

PhysicsWorld remains the owner of the Jolt physics system.

Introduce a project-owned Player physics interface/state between gameplay and
Jolt.

Player must not directly include or manipulate:

- JPH::CharacterVirtual
- JPH::CharacterVirtualSettings
- JPH::BodyID
- JPH::Vec3
- JPH::RVec3
- JPH::PhysicsSystem
- Jolt shapes or filters

Possible project-owned APIs include conceptually:

    InitializePlayer(...)
    UpdatePlayer(...)
    GetPlayerState()

Exact naming may differ.

The public boundary must use only project-owned types.

### Character shape

Represent the Player using a capsule-shaped CharacterVirtual.

Match the existing Player dimensions approximately.

Current visual Player size is approximately:

    width  = 0.8
    height = 1.6
    depth  = 0.8

Choose a capsule radius and cylinder height that approximately preserve this
overall physical size.

Document the exact chosen values.

The character must remain upright.

Do not implement crouching or shape switching.

### Player position ownership

After migration, Jolt CharacterVirtual becomes authoritative for the Player's
physical position.

Do not maintain two independently simulated Player positions.

Gameplay Player state may cache/expose the project-owned position, but it must
be synchronized from the CharacterVirtual result.

Renderer and camera must consume the resulting project-owned Player position.

### Horizontal movement

Preserve the current Milestone 07 movement constants:

    max horizontal speed = 6.0 units/s
    acceleration         = 40.0 units/s²
    deceleration         = 50.0 units/s²

Continue using semantic:

    moveX

Movement remains constrained to the X axis for gameplay.

Player Z must remain fixed at the intended gameplay plane.

Do not add free Z movement.

### Vertical movement

Jolt CharacterVirtual becomes responsible for physical collision response,
but gameplay continues to own jump intent and movement feel.

Preserve:

    jump speed = 8.0 units/s

Use gravity compatible with the current gameplay feel.

Current custom gameplay gravity is:

    20.0 units/s² downward

Do not silently switch the Player to Jolt's default 9.81 gravity if that would
change the validated jump behavior.

Apply vertical velocity through CharacterVirtual in a way that preserves the
existing jump arc as closely as possible.

### Ground state

Replace the custom Player grounded/support detection with CharacterVirtual
ground state.

Translate Jolt ground information into project-owned gameplay state.

At minimum distinguish whether the Player is considered supported enough to:

- stand;
- reset vertical downward velocity as appropriate;
- refresh coyote time;
- jump.

Do not expose Jolt ground-state enums to gameplay-facing headers.

### Coyote time

Preserve:

    coyote duration = 0.10 seconds

Coyote time should now begin when CharacterVirtual transitions from valid
ground support to unsupported/in-air state.

Successful coyote jump must still consume coyote availability.

Do not tune the duration during this milestone.

### Jump buffering

Preserve:

    jump buffer duration = 0.10 seconds

A jump pressed shortly before landing must still trigger when CharacterVirtual
reports valid ground support inside the buffer window.

Do not change the timing value.

### Static world collision

Move the existing greybox world into Jolt static collision.

The Player must collide with:

- main ground;
- right elevated platform;
- left elevated platform.

Do not keep the custom AABB collision authoritative for Player after the
migration.

Use the existing shared greybox geometry as the source of truth where
practical.

Avoid duplicating static platform dimensions independently in several places.

Renderer and Jolt collision should derive from the same project-owned world
geometry.

### Solid collision behavior

Unlike the earlier one-way top-only milestone, Jolt collision is fully solid.

The Player must:

- stand on platforms;
- collide with platform sides;
- collide with platform undersides/ceilings;
- not pass through static geometry.

Preserve the expected M06 solid-collision behavior.

### Dynamic rigid-body interaction

The existing Jolt dynamic test box must now physically interact with the
Player.

Expected behavior:

- Player cannot walk through the box;
- Player contacts the box;
- Player may push the box if CharacterVirtual/Jolt interaction naturally
  permits it;
- the box may respond physically to Player contact.

Do not add custom pushing forces unless required by CharacterVirtual's normal
interaction model.

Do not tune this interaction for final gameplay feel yet.

### CharacterVirtual configuration

Configure CharacterVirtual conservatively.

Document at minimum:

- shape dimensions;
- max slope angle;
- character padding;
- collision tolerance;
- predictive contact distance if changed from default;
- max strength if changed;
- mass if relevant;
- supporting volume;
- penetration recovery behavior if changed.

Prefer defaults unless a project requirement justifies changing them.

Do not blindly copy every setting from the Jolt sample.

### Slope behavior

Slope support should work correctly at the CharacterVirtual level.

However, the current test environment contains flat boxes only.

Do not add slope geometry solely for this milestone.

Do not add slope-specific gameplay mechanics.

### Physics update order

Integrate CharacterVirtual using the correct Jolt update sequence for v5.6.0.

Conceptually:

1. poll semantic input;
2. update gameplay movement intent and timers;
3. apply desired Player velocity to CharacterVirtual;
4. update CharacterVirtual collision/movement;
5. step the Jolt physics world as required;
6. synchronize project-owned Player position/state;
7. update camera;
8. render;
9. render debug UI.

The exact CharacterVirtual/world-step ordering should follow the Jolt API's
requirements for the pinned version.

Document the final order.

### Fixed gameplay plane

Preserve:

    Player Z = fixed gameplay plane

CharacterVirtual must not drift along Z due to contacts or dynamic body
interaction.

Use the minimum explicit constraint/correction needed to preserve the game's
2.5D platformer plane.

Do not add general 3D player locomotion.

### Legacy custom collision

After CharacterVirtual successfully owns Player collision:

- Player must no longer call the custom AABB collision system for movement;
- custom Collision code may remain temporarily in the repository if still
  useful for comparison or later cleanup;
- clearly mark it as no longer authoritative for Player.

Do not delete large amounts of legacy code unless removal is clearly safe and
within this milestone.

A later cleanup milestone may remove unused custom collision code.

### Rendering

Keep the existing visual Player cube.

Do not render the physics capsule as the final Player visual.

Renderer must continue consuming project-owned Player transform data and must
not include Jolt headers.

### Camera

Preserve Milestone 08 exactly:

- horizontal dead zone;
- vertical dead zone;
- smoothing;
- offset;
- FOV.

Camera should follow the new CharacterVirtual-driven Player position.

Do not retune camera constants unless required to fix a regression.

### Debug metrics

Extend the existing metrics panel to make the migration observable.

Display at minimum:

#### Player physics

- CharacterVirtual initialized
- physical Player position X/Y/Z
- horizontal velocity
- vertical velocity
- grounded/supported
- Jolt-derived ground state in project-owned readable form

#### Existing movement state

Continue displaying:

- moveX
- jumpPressed
- coyote timer
- coyote availability
- jump buffer remaining
- existing movement constants

#### Physics interaction

If cleanly available:

- number of active CharacterVirtual contacts

Do not expose raw BodyID values.

Metrics remain read-only.

### Milestone 07 revalidation

Re-run the movement-feel validation after migration.

Verify approximately:

- max horizontal speed remains 6.0;
- acceleration remains approximately 0.15 s from rest to max;
- deceleration remains approximately 0.12 s from max to stop;
- coyote window remains 0.10 s;
- jump buffer remains 0.10 s;
- jump height/arc remains visually close to the previous behavior.

Do not change tuning values unless an actual migration bug makes the old
values unusable.

### Out of scope

Do NOT implement:

- Character class
- crouching
- shape switching
- stair/step-up tuning
- moving platforms
- elevators
- ladders
- wall jump
- wall slide
- double jump
- variable jump height
- slopes added to the test level
- slope gameplay tuning
- custom push-force tuning
- knockback
- damage
- triggers
- sensors
- collectibles
- raycasts
- shape casts outside CharacterVirtual internals
- physics materials tuning
- friction gameplay tuning
- restitution tuning
- CCD tuning
- fixed-timestep framework
- render interpolation
- networking
- rollback
- animation
- audio
- Milestone 12

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. Debug build succeeds.
3. Development build succeeds.
4. Release build succeeds.
5. Jolt remains pinned to v5.6.0.
6. Player uses Jolt CharacterVirtual.
7. Player does not use JPH::Character.
8. CharacterVirtual types remain isolated inside the physics layer.
9. Player headers contain no Jolt headers.
10. Renderer contains no Jolt headers.
11. PlatformerCamera contains no Jolt headers.
12. Player physical position is authoritative from CharacterVirtual.
13. Player X movement remains functional.
14. Player Z remains fixed.
15. Max horizontal speed remains 6.0.
16. Acceleration remains 40.0.
17. Deceleration remains 50.0.
18. Jump speed remains 8.0.
19. Gameplay gravity remains equivalent to 20.0 downward.
20. Coyote duration remains 0.10.
21. Jump-buffer duration remains 0.10.
22. Player can stand on the main ground.
23. Player can land on elevated platforms.
24. Player collides with platform sides.
25. Player collides with platform undersides.
26. Player does not pass through static geometry.
27. Player can walk off platform edges and fall.
28. Player can jump normally.
29. Coyote jump works.
30. Jump buffering works.
31. Existing custom AABB collision is no longer authoritative for Player.
32. Greybox render geometry and Jolt static collision derive from shared
    project-owned world geometry where practical.
33. Player cannot pass through the Jolt dynamic test box.
34. Dynamic box responds to Jolt interaction with the Player.
35. Camera follows the migrated Player normally.
36. Camera dead zones remain functional.
37. Camera smoothing remains functional.
38. Debug metrics expose CharacterVirtual Player state.
39. Existing M07 metrics remain functional.
40. F1 still toggles debug metrics.
41. Release still has no debug UI.
42. Resize still works.
43. X and ESC still close normally.
44. No new gameplay mechanics were added.
45. Milestone 12 has not been started.
