# Milestones

## Milestone 00 — Repository/Foundation Setup
Goal: create a clean Cursor/CMake repository before gameplay implementation.

Acceptance criteria:
- Repository structure exists.
- `AGENTS.md`, Cursor Rules, and project Skills exist and are discoverable.
- CMake config has Debug, Development, and Release conventions.
- A minimal executable target exists.
- Windows Development build can be configured with Visual Studio 2022.
- No gameplay feature is implemented yet.

## Milestone 01 — Window and Game Loop

### Goal

Integrate raylib and establish the minimum runtime foundation of the game.

The application must open a graphical window, run a game loop and shut down cleanly.

No gameplay must be implemented in this milestone.

### Requirements

- Integrate raylib through CMake.
- Use a pinned raylib version. Never depend on a floating `master` branch.
- Keep CMake as the canonical build system.
- Windows remains the only active target for now.
- Create a minimal application lifecycle:
  - Initialize
  - Run
  - Shutdown
- Open a resizable 1280x720 window.
- Window title: `Platformer3D`.
- Render a solid background every frame.
- Close cleanly using the window close button.
- ESC may also close the application.
- Keep raylib-specific code behind the platform/render boundary.
- `Main.cpp` must not contain the game loop implementation.
- Gameplay code must not include raylib headers.

### Suggested structure

game/source/
  Main.cpp
  core/
    Application.h
    Application.cpp
  platform/
    Window.h
    Window.cpp

The exact implementation may vary if a simpler architecture satisfies the same constraints.

### Out of scope

Do NOT implement:

- player
- input abstraction beyond what is required to close the application
- physics
- Jolt
- GLM integration
- Dear ImGui
- camera
- level
- assets
- Asset Cooker
- menus
- audio
- shadows
- post-processing

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. `cmake --build --preset windows-development` succeeds.
3. Running the Development executable opens a 1280x720 graphical window.
4. The window remains open while the game loop runs.
5. The window can be resized.
6. Closing the window exits without errors.
7. No gameplay exists yet.
8. raylib-specific calls are isolated from future gameplay code.
9. CMake continues to be the source of truth for the build.

## Milestone 02 — 3D Greybox and Platformer Camera

### Goal

Establish the first 3D game scene and validate the visual foundation of the platformer.

The application must render a simple greybox environment, a placeholder player and a platformer-style camera.

No player movement or physics must be implemented yet.

### Requirements

- Keep CMake as the canonical build system.
- Continue using raylib through the existing platform/render boundary.
- Render a basic 3D scene.
- Add a simple ground/platform greybox.
- Add a visible placeholder player using primitive geometry.
- Introduce a minimal rendering responsibility separate from the Window class.
- Window remains responsible for window lifecycle.
- Rendering code is responsible for frame rendering and 3D drawing.
- Add a platformer-style camera.
- Camera parameters must be easy to tune in code.
- The camera must look at the player placeholder.
- The player should be shown from a mostly side-facing perspective while preserving visible 3D depth.
- Use perspective projection initially.
- Keep the player stationary for this milestone.
- Preserve the existing clean application lifecycle.

### Suggested structure

game/source/
  Main.cpp
  core/
    Application.h
    Application.cpp
  platform/
    Window.h
    Window.cpp
  render/
    Renderer.h
    Renderer.cpp
  gameplay/
    Player.h
    Player.cpp

The exact implementation may vary if a simpler architecture satisfies the same constraints.

### Player placeholder

The player placeholder may be represented by:

- a cube;
- a capsule-like primitive;
- or another simple raylib primitive.

No animation or model loading is required.

The placeholder must have a world position that can later be controlled by gameplay code.

### Greybox level

Create a minimal scene containing:

- one main ground/platform;
- optionally one or two additional elevated platforms;
- a visible world grid or equivalent visual reference if useful.

Do not build a complete level.

### Camera

Use a simple platformer-style 3D camera.

Initial recommendation:

- perspective projection;
- camera positioned behind and above the player;
- camera looking toward the player;
- lateral composition suitable for a platformer;
- enough depth angle to clearly show that the world is 3D.

Camera parameters such as:

- offset;
- target;
- field of view;

should be defined clearly and remain easy to tune.

Do not implement camera smoothing, shake, collision or dynamic behavior yet.

### Architecture

Responsibilities should remain approximately:

Application
  ├── Window
  └── Renderer
        └── 3D scene

Gameplay owns world state such as the player's position.

Renderer reads that state and draws it.

Do not let gameplay code make direct raylib rendering calls.

Do not introduce unnecessary renderer interfaces, factories or multiple backends yet.

### Out of scope

Do NOT implement:

- player input
- player movement
- jumping
- gravity
- collision response
- Jolt
- gameplay physics
- enemies
- collectibles
- animation
- model loading
- textures
- asset cooker
- ImGui
- audio
- menus
- camera smoothing
- camera collision
- camera shake
- shadows
- post-processing

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. `cmake --build --preset windows-development` succeeds.
3. The Development executable opens normally.
4. A 3D greybox scene is visible.
5. A placeholder player is clearly visible.
6. The player remains stationary.
7. A platformer-style perspective camera is active.
8. The camera presents the scene from a mostly side-facing view with visible 3D depth.
9. Window lifecycle remains isolated from rendering responsibility.
10. Gameplay code contains no direct raylib rendering calls.
11. No movement or physics has been implemented.
12. Closing with X and ESC still works.

## Milestone 03 — Semantic Input and Player Movement

### Goal

Introduce the first interactive gameplay behavior.

The player must move horizontally along the platformer's primary gameplay axis using semantic input and frame-rate-independent movement.

Jumping, gravity and collision are intentionally deferred to a later milestone.

### Gameplay axis

The platformer's primary movement axis is X.

For this milestone:

- X = horizontal platformer movement;
- Y = vertical axis;
- Z = visual/world depth.

The player must remain fixed on Y and Z while moving.

Do not add free 3D movement.

### Requirements

- Preserve all validated Milestone 02 behavior.
- Keep CMake as the canonical build system.
- Introduce a minimal semantic input layer.
- Gameplay must not query raw keyboard keys directly.
- Support:
  - Move Left
  - Move Right
- Default keyboard bindings:
  - A or Left Arrow = Move Left
  - D or Right Arrow = Move Right
- Convert raw input into project-owned semantic input state.
- Update the player through gameplay code.
- Move the player only along the X axis.
- Movement must use delta time and be frame-rate independent.
- Player movement speed must be an easy-to-tune gameplay value.
- Keep Y unchanged.
- Keep Z unchanged.
- Existing camera behavior should naturally follow the moving player.
- X and ESC window behavior must remain unchanged.

### Suggested structure

game/source/
  core/
    Application.h
    Application.cpp
    Vec3.h

  gameplay/
    Player.h
    Player.cpp
    PlatformerCamera.h

  input/
    Input.h
    Input.cpp
    InputState.h

  platform/
    Window.h
    Window.cpp
    Time.h
    Time.cpp

  render/
    Renderer.h
    Renderer.cpp

The exact file organization may vary if a simpler solution preserves the same responsibilities.

### Semantic input

Gameplay must work from semantic state rather than raylib key constants.

A minimal representation may look conceptually like:

    struct InputState
    {
        float moveX = 0.0f;
    };

Expected values:

- -1.0 = move left;
-  0.0 = no horizontal movement;
- +1.0 = move right.

A and Left Arrow may contribute to the same semantic action.

D and Right Arrow may contribute to the same semantic action.

Do not expose raylib KeyboardKey values to gameplay.

### Player update

Player owns its gameplay movement behavior.

Conceptually:

    Player::Update(inputState, deltaSeconds);

Horizontal movement should be equivalent to:

    position.x += movementInput * moveSpeed * deltaSeconds;

The actual implementation may differ if it remains equally simple and clear.

### Timing

Gameplay movement must use elapsed frame time in seconds.

Do not use a fixed amount of movement per rendered frame.

Keep raylib-specific timing calls outside gameplay code.

Do not implement a fixed timestep or physics accumulator yet.

### Camera behavior

The existing platformer camera should continue targeting the player's current position.

As the player moves horizontally, the camera may therefore follow immediately.

Do not add:

- smoothing;
- look-ahead;
- dead zones;
- camera shake;
- camera collision.

Those behaviors will be evaluated later after basic movement exists.

### Architecture

Responsibilities should remain approximately:

Application
  ├── polls semantic input
  ├── obtains delta time
  ├── updates Player
  └── asks Renderer to draw

Input
  └── translates raw platform/backend input
      into project-owned semantic input

Player
  └── owns gameplay movement state and logic

Renderer
  └── reads world state and renders it

Gameplay must not depend on raylib.

Do not create a general input binding system, command system, event bus or action remapping UI yet.

### Out of scope

Do NOT implement:

- jumping
- gravity
- falling
- ground detection
- collision detection
- collision response
- Jolt
- slopes
- acceleration
- deceleration
- movement smoothing
- running
- crouching
- free movement on Z
- gamepad support
- configurable key bindings
- input rebinding UI
- fixed timestep physics
- animation
- audio
- ImGui
- level system
- scene graph
- ECS
- camera smoothing
- camera look-ahead
- camera dead zones
- Milestone 04

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. `cmake --build --preset windows-development` succeeds.
3. The application opens normally.
4. A or Left Arrow moves the player toward -X.
5. D or Right Arrow moves the player toward +X.
6. Releasing movement keys stops horizontal movement.
7. The player does not move on Y.
8. The player does not move on Z.
9. Movement uses delta time rather than a fixed per-frame displacement.
10. Gameplay code contains no raylib key constants or direct raylib input calls.
11. Gameplay code contains no direct raylib timing calls.
12. Player movement speed is clearly tunable.
13. The camera continues following the player's position.
14. No jumping, gravity or collision has been implemented.
15. Resize still works.
16. X and ESC still close the application normally.

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

## Milestone 09 — Debug/Development Metrics

### Goal

Introduce a Debug/Development-only Dear ImGui metrics panel for observing
runtime gameplay and camera state.

Use the panel to quantitatively revalidate the movement behavior introduced
in Milestone 07.

The panel is read-only in this milestone.

Release builds must not include or initialize the debug UI.

### Dependencies

Use:

- Dear ImGui 1.92.7
- rlImGui Raylib_6_0 integration
- existing raylib 6.0 dependency

Pin dependency versions/tags explicitly.

Do not track floating branches such as `main` or `master`.

### Build configurations

Debug UI must be enabled only for:

- Debug
- Development

Debug UI must be disabled for:

- Release

Introduce a project-owned compile definition such as:

    PLATFORMER_ENABLE_DEBUG_UI

Only Debug and Development builds should define it.

Release must compile successfully without executing or referencing debug UI
runtime code.

Avoid scattering configuration preprocessor checks throughout gameplay code.

### Architecture

Introduce a small project-owned debug UI layer.

Suggested responsibility split:

    ui/debug/DebugMetrics
        decides which runtime information is displayed

    ui/debug/DebugUiBackend
        owns Dear ImGui / rlImGui initialization, frame begin/end and shutdown

Exact names may differ if the structure remains small and clear.

Dear ImGui and rlImGui includes must remain inside the UI/debug implementation
layer.

Do not include Dear ImGui, rlImGui or raylib headers in Player,
PlatformerCamera, Collision or other gameplay headers.

Gameplay systems must not know that ImGui exists.

### Debug UI lifecycle

The debug UI backend must have an explicit lifecycle:

    Initialize
    BeginFrame
    Draw / submit widgets
    EndFrame
    Shutdown

Integrate this cleanly with the existing Application and Renderer lifecycle.

Initialize only after the raylib window/graphics context exists.

Shutdown before the raylib window/graphics context is destroyed.

### Panel toggle

Use F1 to show/hide the metrics panel.

F1 handling belongs to the debug/UI or platform-facing layer.

Do not add F1 to gameplay InputState.

The panel should default to visible in Debug/Development builds.

### Metrics panel

Create one window titled approximately:

    Platformer3D Metrics

Display at minimum:

#### Frame

- FPS
- delta time in milliseconds
- delta time in seconds

#### Player transform

- position X
- position Y
- position Z

#### Player movement

- horizontal velocity
- vertical velocity
- grounded state

#### Input

- semantic moveX
- jumpPressed

#### Milestone 07 timers

- coyote time elapsed or remaining
- coyote availability
- jump buffer remaining

#### Movement constants

Display the current values of:

- maximum horizontal speed
- acceleration
- deceleration
- jump speed
- gravity
- coyote duration
- jump-buffer duration

These values are read-only.

Do not duplicate them as independent debug constants.

The debug panel must display the values used by gameplay.

#### Camera

Display:

- actual/smoothed camera target X/Y/Z
- desired camera target X/Y/Z
- horizontal dead-zone size
- vertical dead-zone size
- follow sharpness

These values are read-only.

### Gameplay debug access

Expose only the minimum read-only state necessary for the metrics panel.

Prefer small const getters or a lightweight read-only metrics/snapshot
structure.

Do not:

- make gameplay members public;
- give DebugMetrics friendship solely to access internals;
- duplicate gameplay state inside DebugMetrics;
- let ImGui directly modify gameplay state.

Avoid creating a large generic reflection or telemetry framework.

### Milestone 07 quantitative validation

The metrics panel must make the deferred Milestone 07 validation possible.

The following should be measurable manually:

#### Acceleration

From rest with full horizontal input:

    0 -> approximately 6 world units/second

with expected nominal time:

    approximately 0.15 seconds

based on the current acceleration constant.

#### Deceleration

From approximately maximum speed with input released:

    approximately 6 -> 0 world units/second

with expected nominal time:

    approximately 0.12 seconds

based on the current deceleration constant.

#### Coyote time

After leaving valid ground support:

- coyote timer should advance or remaining time should decrease;
- coyote jump should only be accepted inside the configured window;
- successful coyote jump should consume coyote availability.

Expected configured duration:

    0.10 seconds

#### Jump buffering

When jump is pressed shortly before landing:

- jump buffer should become active;
- its remaining duration should be visible;
- landing within the valid window should consume the buffer and trigger jump;
- an expired buffer must not cause a later jump.

Expected configured duration:

    0.10 seconds

Do not change the M07 tuning values solely to make these tests easier.

### Camera validation

Use the metrics panel to observe Milestone 08 state.

Verify that:

- desired target remains stationary while the player is inside a dead zone;
- desired target changes when the player exceeds a dead-zone boundary;
- smoothed target converges toward desired target rather than snapping;
- camera target values remain finite and stable.

Do not change Milestone 08 camera behavior unless a regression is discovered.

### Rendering integration

The game world must render normally.

Dear ImGui rendering should happen after normal scene rendering and before
the end of the raylib frame, according to the rlImGui lifecycle requirements.

Debug UI must not own world rendering.

Renderer must not become responsible for gameplay metrics.

### Out of scope

Do NOT implement:

- editable gameplay values
- sliders for tuning
- buttons that modify Player state
- graphs/history plots
- performance profiler
- memory profiler
- entity inspector
- ECS inspector
- scene hierarchy
- asset browser
- developer console
- logging window
- collision visualization
- camera visualization
- gizmos
- level editor
- docking/editor layout
- saving UI layout/preferences
- Jolt integration
- animation
- audio
- Milestone 10

### Acceptance criteria

1. `cmake --preset windows-vs2022` succeeds.
2. `cmake --build --preset windows-debug` succeeds.
3. `cmake --build --preset windows-development` succeeds.
4. `cmake --build --preset windows-release` succeeds.
5. Debug build contains functional debug UI.
6. Development build contains functional debug UI.
7. Release build contains no active debug UI.
8. Debug UI initializes after the graphics/window context exists.
9. Debug UI shuts down before the graphics/window context is destroyed.
10. F1 toggles the metrics panel.
11. F1 is not part of gameplay `InputState`.
12. The world continues rendering normally behind the UI.
13. Player position X/Y/Z is visible.
14. Horizontal velocity is visible.
15. Vertical velocity is visible.
16. Grounded state is visible.
17. Semantic `moveX` is visible.
18. `jumpPressed` is observable.
19. Coyote timer/state is visible.
20. Jump-buffer remaining time/state is visible.
21. Current M07 movement constants are visible.
22. Camera desired target is visible.
23. Camera smoothed target is visible.
24. M08 camera constants are visible.
25. FPS is visible.
26. delta time is visible.
27. Dear ImGui/rlImGui/raylib headers do not leak into gameplay headers.
28. Player does not depend on debug UI.
29. PlatformerCamera does not depend on debug UI.
30. Collision does not depend on debug UI.
31. Metrics are read-only.
32. No gameplay tuning values were changed as part of this milestone.
33. Existing movement/collision behavior remains intact.
34. Existing camera behavior remains intact.
35. Resize still works.
36. X and ESC still close normally.
37. No Jolt integration exists.
38. Milestone 10 has not been started.

### Deferred-validation completion

After successful manual validation using the metrics panel, the deferred
Milestone 07 validation note may be marked as completed or updated with the
observed results.

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

## Milestone 13 — Jolt Moving Platform [ACTIVE]

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

## Milestone 14 — Asset Pipeline
- Python environment and cooker skeleton.
- Source/cooked asset separation.
- Incremental cooking foundation.

## Later milestones
Animation, enemies, collectibles, level editor, audio, save system, profiling/optimization, Raspberry Pi validation, Android port, iOS feasibility/backend work.
