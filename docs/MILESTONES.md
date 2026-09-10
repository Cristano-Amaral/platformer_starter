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

## Milestone 15 — Asset Pipeline Foundation

### Goal

Introduce the first real asset pipeline for Platformer3D.

Create a small Python asset cooker that transforms/copies authored source
assets into a deterministic cooked directory consumed by the game.

Validate the pipeline end-to-end with exactly one simple visual test asset.

This milestone establishes infrastructure only.

Do not build a general-purpose asset management system.

### Core pipeline

The intended flow is:

    game/assets/source
            |
            v
       Python cooker
            |
            v
    game/assets/cooked
            |
            v
          game

Source assets are authoring inputs.

Cooked assets are runtime inputs.

The game must not load the test asset directly from `assets/source`.

### Source versus cooked assets

`game/assets/source/` contains authored assets.

`game/assets/cooked/` contains runtime-ready output generated by the cooker.

Treat cooked output as generated data.

The runtime must consume cooked paths only.

Do not make gameplay code aware of the source asset directory.

### Asset cooker

Create a Python command-line tool under:

    tools/

Use Python 3.

The tool should be runnable from the repository root.

Prefer a command such as:

    python tools/cook_assets.py

Exact naming may differ if the existing repository conventions justify it.

Do not require Blender for Milestone 15.

Do not require external Python packages.

Prefer the Python standard library.

### First supported asset type

Support exactly one simple image/texture asset path through the cooker.

Use a small PNG test texture.

The cooker may initially copy the PNG unchanged into the cooked directory.

This is acceptable for Milestone 15.

The important validation is:

    authored source
        ->
    cooker
        ->
    cooked asset
        ->
    runtime load

Do not add texture compression yet.

Do not resize textures yet.

Do not generate mipmaps yet.

Do not implement platform-specific texture formats yet.

### Test asset

Add exactly one small project-owned visual test asset.

It should be easy to recognize visually.

Prefer a simple generated checker/test pattern rather than an externally
downloaded copyrighted asset.

Keep the asset small.

The asset exists only to prove the pipeline.

Do not start replacing the entire greybox environment with art assets.

### Runtime validation

Use the cooked test asset somewhere visible in the existing test scene.

Prefer applying it to one clearly identifiable existing test object or
displaying it on one simple dedicated visual surface.

Do not modify gameplay collision because of the visual asset.

The test asset must not become authoritative geometry.

Physics remains unchanged.

### Runtime path abstraction

Do not scatter literal runtime asset paths throughout rendering code.

Introduce the smallest project-owned runtime asset path helper/configuration
needed for this milestone.

The game should conceptually request something like:

    cooked asset path + logical relative asset path

rather than knowing the source directory.

Do not build a virtual filesystem.

Do not build asset bundles.

Do not build package archives.

### Working-directory robustness

The executable must not rely accidentally on being launched from one exact
current working directory.

Review how runtime asset paths are resolved.

Implement the smallest robust solution suitable for the current architecture.

Prefer a project-owned asset root/path abstraction.

Do not add Windows-only path logic.

Use:

    std::filesystem

where appropriate.

Keep path handling portable.

### CMake integration

Integrate the cooked assets with the existing CMake build in a minimal way.

The build should know where runtime assets are expected.

For Windows development builds, ensure the executable can locate the cooked
asset when launched through the normal project workflow.

Do not turn every source asset into a complex individual CMake rule.

Do not make normal C++ compilation depend on Blender.

### Cooker output

The cooker must create required output directories automatically.

It must produce deterministic output paths.

Running it twice without source changes should produce equivalent cooked
content.

Do not generate timestamps inside cooked asset content or manifest entries
that would cause meaningless diffs.

### Incremental cooking

Implement a minimal incremental mechanism.

The cooker should avoid rewriting an unchanged cooked asset unnecessarily.

For Milestone 15, use a simple deterministic content hash.

Prefer:

    SHA-256

from the Python standard library.

The cooker may compare source and cooked content hashes or maintain a small
manifest containing hashes.

Do not build a dependency graph.

Do not build parallel cooking.

Do not build a cache server.

### Manifest

Generate one small machine-readable manifest inside the cooked output.

Prefer:

    JSON

The manifest should contain enough information to identify the cooked test
asset and verify its source/cooked relationship.

Conceptually:

    version
    assets
        logical name/path
        source relative path
        cooked relative path
        source hash

Exact schema may differ.

Use stable ordering.

Use paths with portable separators in manifest data.

Do not store absolute machine-specific paths.

Do not store timestamps unless strictly required.

### Logical asset identity

Give the test asset a project-owned logical identity/path.

Example concept:

    textures/test_checker.png

The logical identity must not contain an absolute filesystem path.

Do not create UUID infrastructure.

Do not create a database.

### Clean/stale output behavior

The cooker should handle its own known generated output safely.

If an asset previously recorded in its manifest is removed from source, stale
cooked output owned by the cooker should be removable.

Do not recursively delete arbitrary unknown files.

Only clean files that the cooker can identify as outputs it owns.

If implementing safe stale cleanup would significantly expand the milestone,
document it explicitly and keep cleanup conservative.

### Error handling

The cooker must fail clearly when:

- a required source asset is missing;
- an input cannot be read;
- an output cannot be written;
- manifest generation fails.

Return a non-zero process exit code on failure.

Print concise actionable diagnostics.

Do not silently succeed with missing required content.

### Runtime missing-asset behavior

Runtime loading of the test asset must fail safely.

If the cooked asset is missing or cannot be loaded:

- print a clear diagnostic;
- keep the application stable where practical;
- use a simple fallback visual if easy within the current renderer.

Do not crash through a null/invalid texture handle.

Do not build a general fallback asset system.

### Renderer ownership

Keep raylib texture APIs inside the render/backend-facing layer.

Gameplay must not load textures.

Gameplay must not know filesystem asset paths.

Renderer or a small render-side resource owner should own the runtime
texture lifecycle.

Initialize the texture after the window/graphics context exists.

Unload it before the graphics context/window is destroyed.

Do not leak raylib Texture2D into gameplay headers.

### Asset lifetime

Explicitly manage:

    load
    use
    unload

Do not load the texture every frame.

Load it once during renderer/resource initialization.

Reuse it while running.

Unload it during shutdown.

### Existing gameplay

Preserve Milestones 07–14.

Do not change:

- Player movement constants;
- CharacterVirtual behavior;
- jump;
- gravity;
- coyote time;
- jump buffer;
- static platform collision;
- moving platform behavior;
- slope behavior;
- camera behavior.

Asset work must not affect physics.

### Existing test scene

Preserve:

- main ground;
- elevated platforms;
- moving platform;
- walkable slope;
- steep slope;
- cyan dynamic Jolt box.

The new asset should be a visual validation only.

Do not redesign the level.

### Debug metrics

Do not build a full asset inspector.

A very small read-only Asset section is allowed in Debug/Development.

If useful, expose:

- test texture loaded: true/false;
- cooked asset path;
- fallback active: true/false.

Do not expose editable paths.

Do not add asset hot reload.

### Release

The runtime asset-loading path must also work in Release.

Debug UI remains absent from Release.

Do not rely on Debug-only code for asset location.

### Portability

Do not introduce Windows-specific asset APIs.

Use portable C++ and `std::filesystem`.

Python cooker paths must work with pathlib.

Manifest paths should use portable relative representation.

Do not assume case-insensitive filesystems.

Keep exact filename casing consistent.

Do not introduce Android/iOS/Raspberry Pi packaging yet.

The pipeline should simply avoid preventing those future targets.

### Documentation

Update:

- AGENTS.md if appropriate;
- docs/ARCHITECTURE.md;
- docs/MILESTONES.md;
- README.md.

Document:

- source asset location;
- cooked asset location;
- how to run the cooker;
- how incremental cooking works;
- manifest purpose;
- how the runtime resolves cooked assets;
- how to verify the test texture.

### Dependencies

Do not change:

- raylib 6.0;
- Dear ImGui 1.92.7;
- rlImGui Raylib_6_0;
- Jolt Physics v5.6.0.

Do not add Python package dependencies.

### Out of scope

Do NOT implement:

- Blender automation;
- FBX import;
- glTF model pipeline;
- OBJ pipeline;
- model loading;
- skeletal animation;
- texture resizing;
- texture compression;
- mipmap generation;
- GPU-specific texture formats;
- asset bundles;
- PAK files;
- ZIP runtime packages;
- virtual filesystem;
- UUID asset database;
- asset registry framework;
- asynchronous loading;
- streaming;
- background loading;
- hot reload;
- file watching;
- dependency graph;
- parallel cooking;
- remote cache;
- shader cooking;
- audio cooking;
- localization;
- platform-specific cooking;
- Android packaging;
- iOS packaging;
- Raspberry Pi packaging;
- level loading;
- editor;
- Milestone 16.

### Acceptance criteria

1. CMake configure succeeds.
2. Debug build succeeds.
3. Development build succeeds.
4. Release build succeeds.
5. Existing dependency versions remain unchanged.
6. Python cooker exists under tools/.
7. Cooker uses Python standard library only.
8. Cooker runs successfully from repository root.
9. Source and cooked asset directories remain separate.
10. Exactly one PNG test asset validates the pipeline.
11. Runtime does not load the test asset from assets/source.
12. Cooker creates cooked directories automatically.
13. Cooker produces deterministic output path.
14. Cooker generates a JSON manifest.
15. Manifest contains no absolute machine-specific paths.
16. Manifest uses deterministic/stable ordering.
17. Source content is identified using SHA-256 or equivalent deterministic
    content hash.
18. Running cooker twice without changes does not unnecessarily rewrite the
    cooked texture.
19. Running cooker twice without changes does not produce meaningless
    manifest differences.
20. Changing the source asset causes the cooker to update the cooked asset.
21. Missing required source asset produces a clear non-zero cooker failure.
22. Runtime loads the cooked test texture.
23. Runtime texture is loaded once, not every frame.
24. Runtime texture is unloaded during shutdown.
25. Gameplay contains no raylib texture types.
26. Gameplay contains no source/cooked filesystem knowledge.
27. Runtime asset paths are project-owned and not scattered literals.
28. Runtime path handling does not use Windows-only APIs.
29. Runtime does not depend accidentally on one specific current working
    directory.
30. Missing cooked texture does not cause an uncontrolled crash.
31. Test asset is visibly identifiable in the scene.
32. Test asset does not alter physics/collision.
33. Player movement remains unchanged.
34. Moving platform remains unchanged.
35. Walkable/steep slopes remain unchanged.
36. Camera remains unchanged.
37. Dynamic cyan box remains functional.
38. F1/debug metrics remain functional.
39. Release contains no active debug UI.
40. Release can locate/load the cooked test asset through the intended
    runtime path strategy.
41. Resize still works.
42. X and ESC still close normally.
43. Documentation explains how to cook assets.
44. Documentation explains source versus cooked assets.
45. No external Python dependency was added.
46. No asset hot reload was added.
47. No model pipeline was added.
48. No platform-specific cooker was added.
49. No unrelated gameplay mechanic was added.
50. Milestone 16 was not started.

## Milestone 16 — First Static 3D Model Asset Pipeline

### Objective

Extend the Milestone 15 asset pipeline so it supports the first authored static 3D model asset in GLB format.

The pipeline must become:

text

game/assets/source/models/test_static.glb
                  ↓
         Python asset cooker
                  ↓
game/assets/cooked/models/test_static.glb
                  ↓
          CMake runtime staging
                  ↓
<exe>/assets/models/test_static.glb
                  ↓
              Renderer

The existing Milestone 15 texture pipeline must continue working unchanged.

This milestone is intentionally limited to proving:

text

GLB source
→ cooked
→ staged
→ loaded
→ rendered

The model is visual only and must not participate in collision or physics.

## Scope

Add exactly one authored static model:

    game/assets/source/models/test_static.glb

Logical asset identity:

    models/test_static.glb

Cooked output:

    game/assets/cooked/models/test_static.glb

Runtime staged path:

    <exe>/assets/models/test_static.glb

The existing Milestone 15 texture remains:

    textures/test_checker.png

After this milestone, the authored asset set should be:

    game/assets/source/
    ├─ textures/
    │  └─ test_checker.png
    └─ models/
   └─ test_static.glb


### Static GLB Test Asset

Create one small deterministic GLB/glTF 2.0 test model locally.

The model may be a simple:

- cube;
- pyramid;
- prism;
- or similarly simple low-poly shape.

Requirements:

- valid GLB/glTF 2.0;
- single self-contained `.glb` file;
- no external buffers;
- no external textures;
- no downloaded artwork;
- no copyrighted third-party model;
- small enough to remain a technical test asset;
- clearly visible as a 3D object at runtime.

The user must not need to provide or download any model manually.

A temporary implementation-time helper script may be used to generate the GLB using only the Python standard library.

However, the asset cooker itself must not become a procedural model generator.

The final authored source asset must exist at:

    game/assets/source/models/test_static.glb

### Asset Cooker

Continue using:

    tools/cook_assets.py

Do not create a second cooker.

The cooker must now process both known authored assets:

    textures/test_checker.png
    models/test_static.glb

A small explicit or declarative list of known assets is acceptable.

Do not implement a generic asset database.

Both assets must preserve the Milestone 15 incremental behavior:

- Python standard library only;
- `pathlib`;
- SHA-256 content identity;
- no mtime-based identity;
- deterministic output;
- no absolute paths in the manifest;
- no timestamps in the manifest;
- stable ordering;
- conservative stale-output cleanup;
- clear non-zero failure on missing required source assets.

If an asset has not changed, its cooked output must not be rewritten.

Example unchanged run:

    textures/test_checker.png → unchanged/skipped
    models/test_static.glb    → unchanged/skipped
    manifest.json             → unchanged/skipped

If only the GLB changes:

    textures/test_checker.png → unchanged/skipped
    models/test_static.glb    → cooked
    manifest.json             → updated


### Manifest

Reuse the current manifest structure.

If the Milestone 15 schema naturally supports multiple entries, keep:

    schemaVersion = 1

Do not increment the schema version merely because a second asset exists.

Expected structure:

    ```json
    {
    "assets": [
        {
        "cooked": "models/test_static.glb",
        "id": "models/test_static.glb",
        "source": "models/test_static.glb",
        "sourceSha256": "..."
        },
        {
        "cooked": "textures/test_checker.png",
        "id": "textures/test_checker.png",
        "source": "textures/test_checker.png",
        "sourceSha256": "..."
        }
    ],
    "schemaVersion": 1
    }
    ```

The exact ordering may follow the current deterministic strategy, but it must remain stable.

### CMake Runtime Staging

CMake must not perform asset cooking.

The user must continue running:

    ```powershell
    python tools/cook_assets.py
    ```

CMake may validate that the required cooked assets exist and fail with an actionable message when they do not.

Extend the existing runtime staging so both assets are copied to:

    ```text
    <TARGET_FILE_DIR>/assets/textures/test_checker.png
    <TARGET_FILE_DIR>/assets/models/test_static.glb
    ```

This must work for:

- Debug;
- Development;
- Release.

Expected runtime layout:

    ```text
    bin/Development/
    ├─ Platformer3D.exe
    └─ assets/
    ├─ textures/
    │  └─ test_checker.png
    └─ models/
        └─ test_static.glb
    ```

The same structure must exist for Debug and Release.

Use `copy_if_different` or equivalent behavior consistent with Milestone 15.

A small CMake list/helper is acceptable if it reduces duplication.

Do not introduce asset archives, bundles, installers, or packaging systems.

### Runtime Asset Paths

Reuse the Milestone 15 runtime path abstraction.

The model must be resolved through the executable directory.
Conceptually:

    ```cpp
    platform::RuntimeAssetPath("models/test_static.glb")
    ```

must resolve to:

    ```text
    <actual executable directory>/assets/models/test_static.glb
    ```

Do not:

- use `std::filesystem::current_path()`;
- depend on the current working directory;
- search upward for the repository root;
- load from `game/assets/source`;
- load directly from `game/assets/cooked`;
- introduce development-only path fallbacks.
- Preserve the current platform-specific executable-directory isolation.

### Renderer Ownership

The Renderer/render backend must own the raylib model resource.

Gameplay must not depend on raylib model types such as:

    ```cpp
    Model
    Mesh
    Material
    Texture2D
    ```

The model lifecycle must be:

    ```text
    graphics context initialized
            ↓
    LoadModel once
            ↓
    render each frame
            ↓
    UnloadModel once
            ↓
    graphics context shutdown
    ```

Do not load the model every frame.

Do not unload it after the graphics context has already shut down.

Preserve the existing Milestone 15 texture lifecycle.

### Visual Test

Render the static model in a dedicated visible location.

A position near:

    x = 2.5
    y ≈ 1.0
    z = 2.5

is acceptable, but may be adjusted slightly for visibility.

The model must:

- be clearly visible;
- appear three-dimensional;
- not significantly overlap the Player;
- not hide the checker texture test surface;
- not affect gameplay.

A simple render-owned position/scale/rotation is sufficient.

Do not introduce:

- generic Transform components;
- scene graph;
- ECS;
- entity abstraction.

### No Collision

The GLB is visual only.

Do not:

- add it to `GreyboxWorld`;
- create a Jolt body for it;
- create mesh collision;
- derive collision from the GLB;
- make `CharacterVirtual` interact with it.

Rendering geometry and physics geometry remain intentionally separate.

### Runtime Failure Handling

If the staged runtime model is missing or cannot be loaded:

- emit a clear diagnostic including:
    models/test_static.glb
- do not crash;
- keep the game running;
- do not fall back to source or cooked development paths.

A simple visual fallback cube, wireframe, or marker is acceptable.

The fallback must remain visual only.

### Debug Metrics

In Debug and Development only, extend the existing read-only Assets diagnostics.

Example:

    Assets

    Texture
      loaded: true
      fallback: false

    Static Model
    loaded: true
    fallback: false
    id: models/test_static.glb

Do not:

- expose editable paths;
- add asset browsing;
- add hot reload.
- Release must remain free of Dear ImGui/debug UI.

### Gameplay and Physics Preservation

Do not change gameplay or physics tuning.
Preserve:

    max horizontal speed = 6
    acceleration         = 40
    deceleration         = 50
    jump speed           = 8
    gravity              = 20
    coyote time          = 0.10 s
    jump buffer          = 0.10 s

Also preserve:

- CharacterVirtual behavior;
- fixed gameplay Z behavior;
- moving platform behavior and carry;
- walkable 30° slope;
- steep 60° slope;
- camera behavior;
- cyan dynamic Jolt test box;
- existing greybox world;
- existing checker texture test.

No unrelated gameplay, physics, or camera refactors.

### Documentation

Update relevant documentation minimally.

Document:

- source model path;
- logical model identity;
- cooked model path;
- runtime staged path;
- cooker command;
- that the GLB is currently copied unchanged;
- that model collision is intentionally unsupported;
- that the GLB is a technical pipeline test asset.

Keep the terminology consistent:

    source
    cooked
    runtime staged

### Explicitly Out of Scope

Do not implement:

- Milestone 17;
- Blender automation;
- Blender Python integration;
- `.blend` source pipeline;
- OBJ;
- FBX;
- animation;
- skeletal animation;
- bones;
- skinning;
- morph targets;
- material pipeline;
- advanced PBR pipeline;
- normal maps;
- custom shaders;
- model LOD;
- mesh optimization;
- mesh compression;
- Draco;
- KTX;
- texture compression;
- mipmap generation;
- collision generated from model meshes;
- Jolt mesh collision;
- scene files;
- scene graph;
- ECS;
- generic asset handles;
- shared-ownership asset manager;
- asynchronous asset loading;
- background loading;
- hot reload;
- VFS;
- archives;
- asset packs;
- runtime asset registry;
- level loading.

### Build Validation

Run the cooker:

```powershell
    python tools/cook_assets.py
```

Run it again without changing anything:

```powershell
    python tools/cook_assets.py
```

The second run must report both assets as unchanged/skipped and must not meaningfully rewrite the manifest.

Then run:

```powershell
    cmake --preset windows-vs2022
    cmake --build --preset windows-debug
    cmake --build --preset windows-development
    cmake --build --preset windows-release
```

All configurations must succeed.

### Manual Validation

**Normal Development Test**

Run:

```powershell
    .\build\windows-vs2022\bin\Development\Platformer3D.exe
```

Verify:

- checker texture appears;
- GLB model appears;
- F1 Assets metrics report the model as loaded;
- Player movement works;
- jump works;
- moving platform works;
- slopes behave as before;
- cyan dynamic box behaves as before;
- resize works;
- X and ESC close the game.

**Release Test From an Unrelated Working Directory**

Run:

```powershell
    $exe = (Resolve-Path ".\build\windows-vs2022\bin\Release\Platformer3D.exe").Path
    cd $env:TEMP
    & $exe
```

Verify:

- checker texture loads;
- GLB model loads;
- runtime behavior does not depend on CWD;
- gameplay remains functional;
- no debug UI is present.
- Missing Runtime Model Test

Temporarily remove:

    build/windows-vs2022/bin/Development/assets/models/test_static.glb

Launch the Development executable.

Verify:

- clear missing-model diagnostic;
- no crash;
- game continues;
- fallback visual appears if implemented;
- no source/cooked fallback loading occurs.

Rebuild afterward to restore the staged asset.

### Acceptance Criteria

1. `cmake --preset windows-vs2022` succeeds.
2. Debug builds successfully.
3. Development builds successfully.
4. Release builds successfully.
5. External dependency versions remain unchanged.
6. Milestone 15 PNG pipeline still works.
7. Exactly one authored test model is added.
8. The model source format is GLB.
9. The GLB is created locally.
10. No external model is downloaded.
11. No third-party copyrighted model is introduced.
12. The GLB is valid glTF 2.0.
13. The GLB is self-contained.
14. Source model is under `game/assets/source/models/`.
15. Cooked model is under `game/assets/cooked/models/`.
16. Runtime model is staged under `<exe>/assets/models/`.
17. Runtime never loads the source model.
18. Runtime never loads directly from `game/assets/cooked`.
19. Existing `tools/cook_assets.py` remains the single cooker.
20. Cooker remains Python-standard-library-only.
21. Cooker processes both PNG and GLB.
22. Both assets use SHA-256 content identity.
23. Unchanged PNG is skipped.
24. Unchanged GLB is skipped.
25. Manifest is not rewritten without changes.
26. Changing only GLB does not recook PNG.
27. Manifest remains deterministic.
28. Manifest contains no absolute paths.
29. Manifest contains no timestamps.
30. Manifest asset ordering is stable.
31. Stale cleanup remains conservative.
32. Missing source GLB produces clear non-zero failure.
33. CMake does not run the cooker.
34. CMake only validates/stages cooked assets.
35. Debug receives the staged GLB.
36. Development receives the staged GLB.
37. Release receives the staged GLB.
38. Runtime model paths use the executable directory.
39. Model loading is independent of CWD.
40. Model loads only once.
41. Model is not loaded every frame.
42. Model unloads before graphics shutdown.
43. raylib model resource types do not leak into gameplay.
44. The GLB model is visibly rendered.
45. The GLB has no gameplay collision.
46. The GLB is not added to `GreyboxWorld`.
47. The GLB creates no Jolt body.
48. Missing runtime model does not crash.
49. Debug/Development Assets metrics expose model state.
50. Release contains no Dear ImGui/debug UI.
51. Player behavior remains unchanged.
52. Moving platform behavior remains unchanged.
53. Slope behavior remains unchanged.
54. Cyan dynamic box remains unchanged.
55. Milestone 17 scope is not implemented.

### Definition of Done

Milestone 16 is complete when:

    GLB source
    → cooker
    → deterministic manifest
    → cooked GLB
    → CMake staging
    → executable-relative runtime path
    → one-time Renderer load
    → visible static model
    → safe unload

works in Debug, Development, and Release while preserving all gameplay and physics behavior from the previous milestone.

No Milestone 17 functionality may be included.

## Milestone 17 — Blender Authored Asset Workflow

### Objective

Introduce Blender as the official 3D model authoring tool without changing the M15/M16 runtime architecture.

```text
Blender
  ↓
.blend authored source
  ↓
manual GLB export
  ↓
existing Python cooker
  ↓
cooked GLB
  ↓
CMake runtime staging
  ↓
Renderer
```

### Scope

Add exactly one Blender source and its exported GLB:

```text
game/assets/source/blender/test_authored.blend
game/assets/source/models/test_authored.glb
```

Logical identity and outputs:

```text
models/test_authored.glb
game/assets/cooked/models/test_authored.glb
<exe>/assets/models/test_authored.glb
```

Preserve `textures/test_checker.png` and `models/test_static.glb`.

### Source Policy

`.blend` is editable authored source. `.glb` is exported runtime source. The cooker consumes the GLB, never the `.blend`. The runtime never loads `.blend`, source assets, or cooked assets directly.

Expected layout:

```text
game/assets/source/
├─ blender/
│  └─ test_authored.blend
├─ models/
│  ├─ test_static.glb
│  └─ test_authored.glb
└─ textures/
   └─ test_checker.png
```

### Blender Role

Blender is an authoring dependency only, not a build or runtime dependency. A machine without Blender must still be able to configure, build, cook existing exported assets, and run the game.

### Blender Test Model

Create one simple static low-poly model, such as a column, pedestal, simple arch, beveled crate, or small decorative structure.

Requirements:

- static and low-poly;
- sensible scale and pivot;
- authored near the Blender origin;
- no animation, armature, bones, or skinning;
- no external texture dependency;
- simple material is acceptable;
- no final-art requirement.

### Coordinate Convention

Document:

```text
Blender: X = horizontal, Y = depth, Z = up
Project: X = horizontal, Y = up, Z = depth
```

Do not create a generic coordinate conversion framework. Document actual glTF exporter axis/scale behavior used.

### Manual Blender Export

Use a manual workflow:

```text
open game/assets/source/blender/test_authored.blend
→ File → Export → glTF 2.0
→ Format: GLB
→ game/assets/source/models/test_authored.glb
```

Do not invoke Blender from CMake or the cooker. No Blender CLI/background/Python export automation in M17.

Document at least:

```text
Format: GLB
Animations: disabled/not exported
External textures: none
```

Use Selected Objects if practical. Exported GLB must be self-contained.

### Asset Cooker

Continue using `tools/cook_assets.py`. Known runtime assets become:

```text
textures/test_checker.png
models/test_static.glb
models/test_authored.glb
```

Preserve Python stdlib only, pathlib, SHA-256 identity, deterministic manifest, stable ordering, no timestamps/absolute paths, conservative stale cleanup, and clear non-zero errors.

The `.blend` must not appear in the runtime manifest.

### Manifest

Keep `schemaVersion: 1` if no schema change is required. It should contain the three runtime-source assets only, including `models/test_authored.glb`, and never `blender/test_authored.blend`.

### CMake Runtime Staging

CMake remains independent of Blender and does not cook assets. Stage:

```text
<TARGET_FILE_DIR>/assets/models/test_authored.glb
<TARGET_FILE_DIR>/assets/models/test_static.glb
<TARGET_FILE_DIR>/assets/textures/test_checker.png
```

for Debug, Development, and Release.

### Runtime Paths

Reuse executable-relative lookup:

```cpp
platform::RuntimeAssetPath("models/test_authored.glb")
```

No CWD dependency, repository-root search, source/cooked fallback, or `.blend` loading.

### Renderer

Follow M16:

```text
RuntimeAssetPath
→ LoadModel once
→ DrawModel each frame
→ UnloadModel once before graphics shutdown
```

No generic asset manager, scene graph, ECS, or generic handles.

### Visual Validation

Render the authored model in a dedicated visible location, approximately:

```text
x = -2.5
y ≈ 1.0
z = 2.5
```

It must be distinguishable from the checker, M16 model, Player, and greybox.

### No Collision

The model is visual only. No GreyboxWorld entry, Jolt body, mesh collision, collision derivation, or CharacterVirtual interaction.

### Failure Handling

If `models/test_authored.glb` is missing/invalid, log clearly, keep running, never try `.blend`/source/cooked fallbacks, and use a visual-only fallback if consistent with M16.

### Debug Metrics

Debug/Development only:

```text
Blender Authored Model
  loaded
  fallback
  id: models/test_authored.glb
```

No editable paths, browser, hot reload, or Blender controls. Release has no debug UI.

### Git Policy

Version:

```text
game/assets/source/blender/test_authored.blend
game/assets/source/models/test_authored.glb
```

Blender backups may be ignored:

```gitignore
*.blend1
*.blend2
```

Do not ignore `*.blend`.

### Documentation

Add `docs/BLENDER_WORKFLOW.md` documenting Blender version, source/export locations, naming and coordinate conventions, export settings, manual export steps, cooker command, source/runtime-source/cooked/runtime-staged terminology, authoring-only Blender role, Git policy, and no model collision.

### Gameplay and Physics Preservation

Do not change:

```text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Preserve CharacterVirtual, fixed gameplay Z, moving platform/carry, slopes, camera, cyan Jolt box, greybox, checker texture, and M16 static model.

### Implementation Phases

#### Phase A — Project Infrastructure

Cursor prepares `source/blender/`, cooker declaration, CMake staging, Renderer support, Debug Metrics, and docs. Cursor must not fabricate a `.blend` binary.

**Phase A:** `game/assets/source/blender/` exists. Cooker/CMake/Renderer did not yet require the authored GLB.

**Phase B:** Manual Blender 5.2.1 export of `test_authored.blend` → `test_authored.glb` (GLB, Selected Objects, +Y Up). See `docs/BLENDER_WORKFLOW.md`.

**Phase C:** Authored GLB is a required cooker/CMake/Renderer asset.

#### Phase B — Manual Blender Authoring

Using installed Blender:

1. Create the simple static model.
2. Save `game/assets/source/blender/test_authored.blend`.
3. Export `game/assets/source/models/test_authored.glb` manually.
4. Record Blender version and actual export settings.

#### Phase C — Integration and Validation

Run cooker twice, validate incremental behavior, build all configs, verify staging/rendering/CWD independence/fallback, and finalize docs.

### Explicitly Out of Scope

No M18, Blender automation/CLI/Python export, CMake/cooker invoking Blender, automatic export detection, dependency graph, material pipeline, texture extraction, PBR/normal maps, animation/rigging/skinning, scene export, prefabs, collision meshes/Jolt mesh collision, navmesh, LOD, mesh optimization/compression, hot reload, VFS, bundles, async loading, ECS, scene graph, or editor tooling.

### Build Validation

After manual export:

```powershell
python tools/cook_assets.py
python tools/cook_assets.py
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Second cook should report all three runtime assets and manifest unchanged/skipped.

### Manual Validation

Development:

```powershell
.\build\windows-vs2022\bin\Development\Platformer3D.exe
```

Verify checker, M16 model, M17 authored model, F1 asset metrics, Player/jump, moving platform, slopes, cyan box, resize, X/ESC.

Release from unrelated CWD:

```powershell
$exe = (Resolve-Path ".\build\windows-vs2022\bin\Release\Platformer3D.exe").Path
cd $env:TEMP
& $exe
```

Verify all assets load and Release has no debug UI.

Missing model test: temporarily remove `build/windows-vs2022/bin/Development/assets/models/test_authored.glb`, launch Development, verify clear diagnostic/no crash/no source fallback, then rebuild.

### Acceptance Criteria

- [ ] Exactly one authored `.blend` test asset exists and is versioned.
- [ ] Blender backup files may be ignored without ignoring `.blend`.
- [ ] Exactly one corresponding Blender-authored GLB exists under `source/models`.
- [ ] GLB is self-contained, static, and requires no external textures.
- [ ] Blender is neither build nor runtime dependency.
- [ ] CMake and cooker do not invoke Blender.
- [ ] GLB export is manual and settings are documented.
- [ ] Coordinate conventions/export behavior are documented.
- [ ] Cooker ignores `.blend` and processes `models/test_authored.glb` incrementally with SHA-256.
- [ ] M15 PNG and M16 GLB remain incremental.
- [ ] Second unchanged cook does not meaningfully rewrite outputs.
- [ ] Manifest remains deterministic, excludes `.blend`, and stays schemaVersion 1 if appropriate.
- [ ] CMake stages M17 GLB in Debug, Development, and Release without cooking.
- [ ] Runtime paths remain executable-relative and CWD-independent.
- [ ] Runtime never loads `.blend` or source assets directly.
- [ ] Authored model loads once and unloads before graphics shutdown.
- [ ] Authored model is visibly distinguishable from M16 model.
- [ ] Authored model has no collision, Jolt body, or GreyboxWorld entry.
- [ ] Missing staged M17 GLB does not crash.
- [ ] Debug/Development metrics expose M17 model status; Release has no debug UI.
- [ ] Player, moving platform, slopes, camera, and cyan box remain unchanged.
- [ ] Milestone 18 scope is not implemented.

### Definition of Done

```text
Blender-authored .blend
→ manual GLB export
→ versioned runtime-source GLB
→ existing SHA-256 cooker
→ deterministic cooked output
→ CMake staging
→ executable-relative runtime lookup
→ one-time Renderer load
→ visible Blender-authored model
→ safe unload
```

The project must build and run without Blender installed once exported assets exist. No Milestone 18 functionality may be included.

## Milestone 18 — Material + Embedded Texture Asset Workflow

## Objective

Extend the proven Blender-authored GLB workflow from Milestone 17 so one authored static model uses a simple textured material whose image data is embedded inside the exported GLB.

The goal is to prove this content path:

```text
Blender authored model + UVs + image texture
        ↓
.blend authored source
        ↓
manual GLB export
        ↓
self-contained textured GLB
        ↓
existing SHA-256 cooker
        ↓
deterministic cooked GLB
        ↓
CMake runtime staging
        ↓
executable-relative runtime loading
        ↓
raylib Model + embedded material/texture
```

Milestone 18 is deliberately **not** a general material system or texture dependency pipeline. It proves exactly one simple textured Blender-authored GLB while preserving the architecture and behavior established through Milestone 17.

---

## Why This Milestone

Milestones 15–17 have already proven:

- one standalone PNG runtime asset;
- one technical static GLB;
- one real Blender-authored static GLB;
- deterministic SHA-256 cooking;
- CMake staging;
- executable-relative runtime lookup;
- safe renderer ownership/lifetime;
- Blender as an authoring-only dependency.

The next useful content-pipeline step is to prove that a Blender-authored model can carry its own simple material and texture through the same GLB path without introducing runtime sidecar dependencies.

This keeps the asset contract friendly to future packaging and constrained targets:

```text
one runtime model asset
→ one staged GLB
→ no runtime search for authoring textures
```

---

## Scope

Create exactly one new Blender-authored textured test asset.

Authored Blender source:

```text
game/assets/source/blender/test_textured.blend
```

Exported runtime source:

```text
game/assets/source/models/test_textured.glb
```

Logical runtime identity:

```text
models/test_textured.glb
```

Cooked output:

```text
game/assets/cooked/models/test_textured.glb
```

Runtime staged path:

```text
<exe>/assets/models/test_textured.glb
```

Preserve all existing assets:

```text
textures/test_checker.png
models/test_static.glb
models/test_authored.glb
```

The M18 model is an additional visual test asset. Do not replace the M15, M16, or M17 assets.

---

## Authored Texture Source

Use exactly one small technical image texture for the M18 Blender asset.

Preferred authored texture source path:

```text
game/assets/source/textures/test_textured_basecolor.png
```

This PNG is an **authoring input** for Blender and must be versioned with the `.blend` source.

It is not a standalone runtime asset in this milestone.

It must **not** be added to the runtime cooker known-assets list and must **not** be staged separately next to the executable.

The exported GLB must contain the image data internally.

Use a small technical texture, preferably:

```text
128x128 or 256x256
```

Do not use an external downloaded/copyrighted texture. Create a simple local test pattern such as stripes, quadrants, arrows, or a small color grid so orientation and UV mapping are visually obvious.

The existing M15 `test_checker.png` remains a separate runtime texture test and should not be repurposed as the M18 authoring texture unless there is a strong reason.

---

## Expected Source Layout

Conceptually:

```text
game/assets/source/
├─ blender/
│  ├─ test_authored.blend
│  └─ test_textured.blend
├─ models/
│  ├─ test_static.glb
│  ├─ test_authored.glb
│  └─ test_textured.glb
└─ textures/
   ├─ test_checker.png
   └─ test_textured_basecolor.png
```

Terminology:

```text
.blend under source/blender          = editable authoring source
PNG under source/textures            = authoring texture source (not a runtime asset)
GLB under source/models              = exported runtime source
asset under assets/cooked            = cooked runtime asset
asset under <exe>/assets              = runtime-staged asset
```

The runtime must never load from `source/blender`.

---

## Blender Model

Create one simple static low-poly object that makes UV orientation easy to inspect.

Good choices:

- beveled crate;
- short pillar;
- rectangular sign/block;
- simple low-poly prop.

A beveled crate or rectangular block is preferred because the texture mapping is easy to inspect visually.

The model must:

- be static;
- remain low-poly;
- use a sensible origin/pivot;
- use a sensible scale;
- have applied scale before final export;
- have no animation;
- have no armature;
- have no bones;
- have no skinning;
- have no collision authority.

---

## UV Requirement

The M18 model must have an explicit UV map.

The purpose is to prove that authored UV data survives:

```text
Blender
→ GLB
→ cooker
→ runtime
```

Use a simple UV unwrap suitable for the chosen test mesh.

Do not introduce automatic UV tooling or a project-wide UV policy.

The technical texture should make obvious if the UV orientation is mirrored, rotated, stretched, or otherwise incorrect.

---

## Material Requirement

Use exactly one simple Blender material for the M18 model.

The intended material graph is minimal:

```text
Image Texture
    ↓
Principled BSDF Base Color
    ↓
Material Output
```

No advanced material authoring is required.

Do not add project-level abstractions for:

- material instances;
- material definitions;
- shaders;
- PBR presets;
- texture slots;
- material databases.

This milestone relies on the material data exported in the GLB and loaded by the existing raylib model loader.

---

## Texture Requirements

The M18 technical texture must:

- be locally created;
- be small;
- use a common PNG format;
- contain no copyrighted third-party artwork;
- be visually distinctive;
- make UV orientation easy to inspect;
- be used as the Base Color texture;
- require no external runtime file after GLB export.

Do not add:

- normal map;
- metallic map;
- roughness map;
- AO map;
- emissive map;
- height map;
- multiple texture sets.

Exactly one authored Base Color image texture is enough.

---

## Embedded GLB Texture Contract

The exported:

```text
game/assets/source/models/test_textured.glb
```

must be self-contained.

The image texture must be embedded in the GLB binary payload rather than referenced by an external URI/file.

The final runtime staging must therefore require only:

```text
assets/models/test_textured.glb
```

for the M18 asset.

Do not stage:

```text
assets/textures/test_textured_basecolor.png
```

The authoring PNG remains under `source/textures/` only. It is not a runtime asset.

The final validation/report must inspect the GLB using existing/local tooling or a small standard-library validation script and confirm that the GLB has no external URI dependency.

Do not add a permanent general-purpose glTF parser to the project merely for this check.

---

## Coordinate Convention

Preserve the M17 Blender export contract.

Blender authoring:

```text
X = horizontal
Y = depth
Z = up
```

Project/runtime:

```text
X = horizontal
Y = up
Z = depth
```

Use the same confirmed Blender export behavior from M17:

```text
Format: glTF Binary (.glb)
Selected Objects: ON
+Y Up: ON
Apply Modifiers: ON
UVs: ON
Normals: ON
Tangents: OFF unless demonstrated necessary
Animation: none
```

Do not manually rotate the model merely to compensate for axes.

---

## Manual Blender Workflow

Milestone 18 keeps Blender export manual.

Do not automate Blender.

Suggested workflow:

```text
create technical PNG
↓
open Blender
↓
create simple mesh
↓
UV unwrap
↓
create one material
↓
connect PNG to Base Color
↓
save .blend
↓
export GLB manually
↓
verify GLB is self-contained
↓
run existing cooker
```

Save authored Blender source as:

```text
game/assets/source/blender/test_textured.blend
```

Export to:

```text
game/assets/source/models/test_textured.glb
```

---

## Asset Cooker

Continue using:

```text
tools/cook_assets.py
```

Final runtime known-assets list should contain exactly the existing runtime assets plus the new M18 GLB:

```text
textures/test_checker.png
models/test_static.glb
models/test_authored.glb
models/test_textured.glb
```

Do not add:

```text
blender/test_textured.blend
textures/test_textured_basecolor.png
```

to the runtime manifest.

Preserve:

- Python standard library only;
- `pathlib`;
- SHA-256 content identity;
- deterministic manifest;
- stable sorting;
- no timestamps;
- no absolute paths;
- conservative stale cleanup;
- required-source failure behavior.

No `pendingAuthored` bridge should be needed in the completed milestone.

---

## Manifest

Keep:

```text
schemaVersion: 1
```

unless the existing schema genuinely cannot represent the new GLB. No schema change is expected.

The manifest should contain four runtime assets and no Blender authoring files.

Conceptually:

```text
models/test_authored.glb
models/test_static.glb
models/test_textured.glb
textures/test_checker.png
```

in deterministic ID ordering.

The authored texture PNG under `source/textures/` must not appear in the runtime manifest.

---

## CMake Runtime Staging

Extend the existing runtime asset list with:

```text
models/test_textured.glb
```

Stage to:

```text
<TARGET_FILE_DIR>/assets/models/test_textured.glb
```

Preserve staging for all previous runtime assets.

CMake must not:

- invoke Blender;
- invoke the cooker;
- stage the M18 authoring PNG separately;
- search `source/blender` at runtime.

The M18 runtime asset is one GLB.

---

## Renderer

Load the new model through the same narrow renderer-side pattern used by M16/M17:

```text
RuntimeAssetPath("models/test_textured.glb")
→ LoadModel once
→ DrawModel each frame
→ UnloadModel once
```

Do not create a generic asset manager or material system.

The model's embedded texture/material should be handled by the existing raylib GLB loading path.

Do not manually load the authored PNG at runtime.

Do not replace the GLB material with a runtime-created texture/material merely to make the visual test pass.

The milestone specifically needs to prove the authored GLB material/texture path.

---

## Visual Placement

Place the M18 textured model at a dedicated visible location that does not overlap the M15/M16/M17 visual tests.

Choose a simple constant position based on the existing scene layout.

The exact position may be adjusted during implementation for visibility, but document the final value.

The model must be visually distinguishable from:

- M15 checker quad;
- M16 technical model;
- M17 Blender-authored untextured/simple-material model;
- Player;
- greybox world;
- slopes;
- moving platform;
- cyan Jolt test body.

---

## Renderer Resource Lifetime

The M18 model must:

- load once after the graphics context is valid;
- remain owned by Renderer/render-facing code;
- draw while loaded;
- unload exactly once before window/graphics shutdown.

Do not expose raylib `Model`, `Material`, `Texture2D`, or other raylib resource types to gameplay.

---

## Failure Handling

If:

```text
assets/models/test_textured.glb
```

is missing or fails to load:

- log a clear diagnostic containing `models/test_textured.glb`;
- keep the game running;
- use a simple visual fallback consistent with M16/M17;
- do not attempt to load the authored PNG;
- do not attempt to load the `.blend`;
- do not load directly from source;
- do not load directly from cooked;
- do not search the repository;
- do not depend on CWD.

The fallback is visual-only.

---

## Debug Metrics

In Debug/Development, extend the existing read-only Assets section with the M18 asset.

Conceptually:

```text
Textured Blender Model
  loaded: true/false
  fallback: true/false
  id: models/test_textured.glb
```

If it fits naturally without introducing new renderer abstractions, also expose a simple read-only diagnostic confirming the loaded model has material/texture data, for example:

```text
material count
```

or another already-available safe diagnostic.

Do not add complex material introspection solely for the metrics panel.

Do not add editable asset paths, asset browser UI, reload buttons, or material controls.

Release remains free of Dear ImGui.

---

## Documentation

Update:

```text
docs/BLENDER_WORKFLOW.md
```

with a concise section covering textured static assets.

Document:

- authored texture location;
- `.blend` location;
- exported GLB location;
- one-material/one-Base-Color-texture scope;
- UV requirement;
- manual texture assignment;
- GLB export settings;
- embedded/self-contained texture requirement;
- authoring PNG is versioned but not a runtime asset;
- runtime stages only the GLB for this model;
- how to re-export after changing the texture/model;
- Blender remains authoring-only;
- collision remains separate.

Update other project docs only where needed to keep terminology accurate. Avoid documentation churn unrelated to M18.

---

## Git Policy

Version authored sources:

```text
game/assets/source/blender/test_textured.blend
game/assets/source/textures/test_textured_basecolor.png
game/assets/source/models/test_textured.glb
```

Continue ignoring Blender backups:

```text
*.blend1
*.blend2
```

Do not ignore:

```text
*.blend
```

Cooked outputs remain governed by the existing cooked-asset policy.

---

## Gameplay and Physics Preservation

Do not change gameplay tuning:

```text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Preserve:

- CharacterVirtual behavior;
- fixed gameplay Z;
- moving-platform carry;
- 30° walkable slope;
- 60° steep slope;
- platformer camera;
- cyan dynamic Jolt box;
- greybox world;
- M15 checker;
- M16 static GLB;
- M17 Blender-authored GLB.

The M18 model is visual only.

Do not create a Jolt body or add it to `GreyboxWorld`.

---

## Implementation Phases

Use the same controlled workflow that worked for M17.

### Phase A — Infrastructure Preparation

Cursor should:

- inspect the completed M17 pipeline;
- prepare the new M18 logical asset identity;
- prepare cooker/CMake/Renderer/metrics integration without breaking the current build before the authored files exist;
- update Blender workflow documentation;
- provide the exact manual Phase B instructions;
- stop before fabricating `.blend` or final GLB content.

Avoid introducing a permanent generic optional/pending asset system merely for the Phase A bridge. If activation must wait until Phase C, prefer small clearly marked Phase C activation points.

**Phase C (current):** `models/test_textured.glb` is a required cooker/CMake/Renderer asset. `pendingAuthored` is removed. The Base Color PNG is not a cooker/runtime asset. See `docs/BLENDER_WORKFLOW.md`.

### Phase B — Manual Blender Authoring

The user should follow the exact 14-step checklist in `docs/BLENDER_WORKFLOW.md` (Blender 5.2.1, UV, 128×128 or 256×256 asymmetric Base Color PNG, one Principled BSDF material, embed on GLB export, M17 coordinate/export convention). Summary:

1. create the technical PNG as `game/assets/source/textures/test_textured_basecolor.png`;
2. create a simple Blender model;
3. UV unwrap it;
4. create one material;
5. connect the PNG to Base Color;
6. save `test_textured.blend`;
7. export `test_textured.glb` manually;
8. confirm the GLB is the only runtime-source file required by this M18 model;
9. report the Blender/export settings used;
10. stop before Phase C.

### Phase C — Integration and Validation

Cursor should then:

- activate the M18 GLB as a required cooker asset;
- cook it;
- verify deterministic incremental behavior;
- verify self-contained GLB/no external URI dependency;
- activate CMake staging;
- activate Renderer loading/drawing/unloading;
- activate Debug Metrics;
- build all configurations;
- prepare/perform runtime tests;
- remove temporary Phase A scaffolding;
- stop before commit so final manual approval can happen.

---

## Build Validation

After Phase C integration, run:

```powershell
python tools/cook_assets.py
python tools/cook_assets.py
```

On the second unchanged run, all four runtime assets and the manifest should be unchanged/skipped.

Then:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

All configurations must succeed.

Verify the M18 runtime stage contains:

```text
assets/models/test_textured.glb
```

and does **not** require:

```text
assets/textures/test_textured_basecolor.png
```

---

## Development Manual Validation

Run:

```powershell
.\build\windows-vs2022\bin\Development\Platformer3D.exe
```

Verify:

- M15 checker still renders;
- M16 static GLB still renders;
- M17 Blender-authored GLB still renders;
- M18 textured GLB renders;
- the M18 Base Color texture is visible on the model;
- UV orientation looks sensible;
- the texture is not unexpectedly mirrored/stretched;
- model orientation is sensible;
- model scale is sensible;
- Debug Assets metrics show the M18 model loaded;
- fallback is false;
- Player movement/jump remain correct;
- moving platform remains correct;
- slopes remain correct;
- cyan dynamic box remains correct.

---

## Runtime Sidecar Validation

Inspect the executable's staged M18 files.

For this asset, the runtime should need:

```text
assets/models/test_textured.glb
```

There must be no requirement for a separately staged:

```text
assets/textures/test_textured_basecolor.png
```

This is a key acceptance condition.

---

## Release CWD Independence

From the repository root:

```powershell
$exe = (Resolve-Path ".\build\windows-vs2022\bin\Release\Platformer3D.exe").Path
cd $env:TEMP
& $exe
```

Verify:

- M15 asset loads;
- M16 model loads;
- M17 model loads;
- M18 textured model loads with its texture;
- runtime remains independent of CWD;
- no authoring PNG lookup occurs;
- no Debug/ImGui UI appears.

---

## Missing Runtime Model Validation

Temporarily remove:

```text
build/windows-vs2022/bin/Development/assets/models/test_textured.glb
```

Run Development.

Verify:

- diagnostic names `models/test_textured.glb`;
- game continues;
- fallback visual is used;
- runtime does not attempt to load `test_textured_basecolor.png`;
- runtime does not search `.blend`, source, or cooked directories.

Rebuild afterward to restore the staged GLB.

---

## Explicitly Out of Scope

Do not implement:

- Milestone 19;
- Blender CLI/background automation;
- Blender Python export automation;
- CMake invoking Blender;
- cooker invoking Blender;
- generic material system;
- generic texture dependency graph;
- external runtime texture dependency for the M18 model;
- multiple materials on the M18 model;
- multiple image textures on the M18 model;
- normal maps;
- metallic maps;
- roughness maps;
- AO maps;
- emissive maps;
- height maps;
- PBR tuning framework;
- custom shaders;
- shader hot reload;
- material editor;
- texture editor;
- runtime material swapping;
- animation;
- armatures;
- bones;
- skinning;
- rigging;
- skeletal animation;
- morph targets;
- collision meshes;
- Jolt mesh collision;
- navmesh;
- LOD;
- mesh optimization/compression;
- texture compression;
- mipmap pipeline changes;
- VFS;
- asset bundles;
- async asset loading;
- ECS;
- scene graph;
- editor tooling.

---

## Acceptance Criteria

- [ ] 1. One new M18 `.blend` authored source exists.
- [ ] 2. One small locally created M18 authoring PNG exists.
- [ ] 3. One exported `test_textured.glb` exists.
- [ ] 4. The M18 `.blend` is versioned.
- [ ] 5. The M18 authoring PNG is versioned.
- [ ] 6. The exported M18 GLB is versioned.
- [ ] 7. The M18 model is static.
- [ ] 8. The M18 model is low-poly/simple.
- [ ] 9. The M18 model has an explicit UV map.
- [ ] 10. The M18 model uses exactly one simple material.
- [ ] 11. The material uses exactly one Base Color image texture for the milestone test.
- [ ] 12. No normal/metallic/roughness/AO/emissive/height texture pipeline is added.
- [ ] 13. The GLB is self-contained.
- [ ] 14. The GLB has no external runtime URI dependency.
- [ ] 15. The authoring PNG is not a standalone runtime asset.
- [ ] 16. The authoring PNG is absent from the runtime manifest.
- [ ] 17. The `.blend` is absent from the runtime manifest.
- [ ] 18. Cooker processes `models/test_textured.glb`.
- [ ] 19. M18 GLB uses SHA-256 content identity.
- [ ] 20. M15 PNG remains incremental.
- [ ] 21. M16 GLB remains incremental.
- [ ] 22. M17 GLB remains incremental.
- [ ] 23. M18 GLB is incremental.
- [ ] 24. Second unchanged cook does not rewrite outputs meaningfully.
- [ ] 25. Manifest remains deterministic.
- [ ] 26. Manifest remains schemaVersion 1.
- [ ] 27. CMake stages `models/test_textured.glb`.
- [ ] 28. CMake does not stage the M18 authoring PNG separately.
- [ ] 29. Debug receives the M18 GLB.
- [ ] 30. Development receives the M18 GLB.
- [ ] 31. Release receives the M18 GLB.
- [ ] 32. Runtime lookup remains executable-relative.
- [ ] 33. Runtime remains independent of CWD.
- [ ] 34. Runtime never loads the M18 `.blend`.
- [ ] 35. Runtime never loads the M18 authoring PNG directly.
- [ ] 36. M18 model loads once.
- [ ] 37. M18 model unloads correctly before graphics shutdown.
- [ ] 38. M18 model renders visibly.
- [ ] 39. Embedded Base Color texture renders visibly.
- [ ] 40. UV orientation is visually sensible.
- [ ] 41. Model orientation is visually sensible.
- [ ] 42. Model scale is visually sensible.
- [ ] 43. M18 model is visually distinguishable from M15/M16/M17 tests.
- [ ] 44. Missing staged M18 GLB does not crash.
- [ ] 45. Missing staged M18 GLB uses safe visual fallback.
- [ ] 46. Debug/Development metrics expose M18 model status.
- [ ] 47. Release remains free of Dear ImGui/debug UI.
- [ ] 48. No Jolt body or gameplay collision is added for M18.
- [ ] 49. Existing gameplay/physics/camera behavior remains unchanged.
- [ ] 50. Blender remains authoring-only.
- [ ] 51. CMake does not invoke Blender.
- [ ] 52. Cooker does not invoke Blender.
- [ ] 53. No generic material system is introduced.
- [ ] 54. No generic texture dependency system is introduced.
- [ ] 55. No Milestone 19 scope is implemented.

---

## Definition of Done

Milestone 18 is complete when this workflow is proven end-to-end:

```text
technical authored PNG
        +
Blender-authored UV mesh/material
        ↓
versioned .blend authoring source
        ↓
manual self-contained GLB export
        ↓
versioned runtime-source GLB
        ↓
existing deterministic SHA-256 cooker
        ↓
cooked GLB
        ↓
CMake stages one M18 GLB
        ↓
executable-relative runtime lookup
        ↓
Renderer loads GLB once
        ↓
embedded Base Color texture renders correctly
        ↓
Renderer unloads model safely
```

The game must not require Blender or the authored texture PNG at build/runtime once the exported GLB exists.

All M15–M17 asset tests and all existing gameplay, physics, camera, and debug behavior must remain intact.

No Milestone 19 functionality may be included.

# Milestone 19 — Asset Cooker Texture Optimization Foundation

## Status

**Complete.** Standalone runtime PNGs use cooker recipe `runtime_png.max512.lanczos.v1` with cooker-only Pillow 12.3.0. GLBs remain opaque copies. Embedded GLB images and Blender authoring PNGs are not transformed.

## Goal

Extend the existing asset pipeline with the first conservative **texture-processing step** while preserving the architecture proven in Milestones 15–18.

Milestone 19 introduces deterministic image validation and optional size normalization for runtime PNG texture assets handled by the cooker. It does **not** introduce GPU texture compression, platform-specific formats, a general material system, or changes to textures embedded inside GLB files.

The milestone should prove this pipeline:

```text
runtime-source PNG
      ↓
Python cooker
      ↓
validate dimensions / format
      ↓
conservative deterministic resize when required
      ↓
cooked PNG
      ↓
CMake staging
      ↓
runtime
```

The existing Blender-authored textured GLB workflow remains unchanged:

```text
Base Color PNG (authoring only)
      ↓
Blender
      ↓ embedded into GLB
self-contained GLB
      ↓
existing cooker as opaque runtime asset
```

The cooker must **not** extract, resize, recompress, or otherwise modify images embedded inside GLB files in M19.

---

## Why this milestone now

Milestones 15–18 established:

- deterministic SHA-256 asset cooking;
- source → cooked → staged → runtime separation;
- executable-relative runtime paths;
- PNG runtime assets;
- static GLB runtime assets;
- Blender-authored GLB assets;
- embedded Base Color textures inside self-contained GLBs.

The next useful pipeline step is to establish a small, measurable texture policy before introducing more assets.

This also prepares for constrained targets such as Raspberry Pi-class hardware without prematurely implementing DDS, KTX2, Basis Universal, GPU compression, texture streaming, or platform-specific asset variants.

---

# 1. Scope

M19 applies **only to standalone runtime PNG textures that are explicit cooker assets**.

Initially this includes the existing runtime texture:

```text
game/assets/source/textures/test_checker.png
```

The M18 authoring texture:

```text
game/assets/source/textures/test_textured_basecolor.png
```

is **not** a runtime cooker asset and must remain excluded from the manifest/staging/runtime lookup.

Images embedded inside:

```text
models/test_authored.glb
models/test_textured.glb
```

must remain opaque to the cooker.

---

# 2. Texture policy

Introduce an explicit project-owned runtime PNG policy.

For M19:

```text
preferred maximum dimension: 512 px
supported source format: PNG
output format: PNG
aspect ratio: preserved
upscaling: forbidden
```

If both source dimensions are already `<= 512`, preserve the original dimensions.

If either source dimension is `> 512`, resize proportionally so the largest output dimension is exactly `512` pixels.

Examples:

```text
256 × 256   → 256 × 256
512 × 256   → 512 × 256
1024 × 1024 → 512 × 512
1024 × 512  → 512 × 256
400 × 800   → 256 × 512
```

Do not crop.
Do not stretch.
Do not upscale.

---

# 3. Dependency policy

The existing cooker is Python standard-library-only.

Do **not** silently add Pillow, ImageMagick, Blender, or another dependency merely to implement resizing.

Phase A must first determine whether the required PNG processing can be implemented cleanly with the current dependency policy.

If reliable PNG resizing would require a new dependency, stop and report that architectural decision rather than adding it automatically.

The preferred outcome is to keep the cooker dependency-light, but correctness is more important than writing a fragile custom PNG/image implementation.

Do not implement a custom PNG decoder/resampler from scratch merely to preserve the standard-library-only rule.

A dependency change, if ultimately needed, requires explicit approval before implementation.

---

# 4. Asset metadata

Extend the cooker/reporting only as much as needed to make texture processing observable.

For standalone runtime PNG assets, the cooker should be able to report at least:

```text
logical id
source dimensions
cooked dimensions
whether resize was required
source SHA-256 / content identity behavior consistent with existing cooker
```

Do not turn the manifest into a general asset database.

Keep `schemaVersion: 1` if the existing schema can accommodate the required information without ambiguity. If a manifest schema change is genuinely required, explain why before changing it.

Determinism remains mandatory:

- stable ordering;
- no timestamps;
- no absolute paths;
- no machine-specific metadata.

---

# 5. Incremental cooking

Preserve the existing incremental behavior.

The cooker must not rewrite a cooked PNG when:

- source content is unchanged; and
- the relevant texture-processing policy is unchanged.

If processing policy affects cooked output, the cooker must account for that so a future policy change cannot incorrectly reuse stale output.

Do not rely on file modification time as content identity.

The implementation should remain simple and explicit.

---

# 6. Test asset strategy

Do not modify the visual appearance of the existing M15 checker merely to force a resize test.

Use the smallest clean technical validation strategy.

Acceptable approaches include:

- a temporary/generated test fixture used only by cooker tests; or
- a small deterministic source test PNG specifically introduced for M19 if it is clearly justified.

Do not add unnecessary runtime-visible assets solely for automated testing.

The test must prove at least:

1. PNG already within limit remains the same dimensions.
2. PNG above the limit is reduced proportionally.
3. Aspect ratio is preserved.
4. No upscaling occurs.
5. Re-running unchanged input does not rewrite output.
6. Invalid/unsupported input fails clearly rather than producing corrupt output.

---

# 7. Runtime behavior

M19 must not require a new runtime texture API.

The existing M15 checker should continue to load from:

```text
assets/textures/test_checker.png
```

through the established executable-relative runtime path abstraction.

The runtime does not need to know whether the cooker resized a texture.

No gameplay code should know about texture dimensions or cooker policy.

---

# 8. Debug/Development diagnostics

Extend the existing read-only Assets metrics only if useful and cheap.

A suitable M19 diagnostic for the existing standalone checker texture could include:

```text
Standalone Runtime Texture
  loaded: true/false
  id: textures/test_checker.png
  runtime width: ...
  runtime height: ...
  fallback: true/false
```

Do not add editable controls.
Do not add texture reload.
Do not add an asset browser.
Do not expose cooker internals to gameplay.

Release remains free of Dear ImGui.

---

# 9. Documentation

Update the asset documentation to clearly distinguish three categories:

### A. Standalone runtime texture

Example:

```text
textures/test_checker.png
```

Consumed directly by the cooker and staged as a runtime PNG.

### B. Blender authoring texture

Example:

```text
textures/test_textured_basecolor.png
```

Used by Blender and versioned as source, but not directly cooked/staged for runtime.

### C. Texture embedded inside GLB

Example:

```text
models/test_textured.glb
```

The cooker treats the GLB as an opaque runtime asset in M19. Embedded image data is not independently processed.

Document the M19 maximum-dimension policy and the fact that it is a **foundation policy**, not the final multiplatform texture-compression solution.

---

# 10. Preserve existing architecture

Do not change gameplay tuning:

```text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10
jump buffer          = 0.10
```

Preserve:

- CharacterVirtual;
- fixed gameplay Z;
- moving platform and carry behavior;
- 30-degree walkable slope;
- 60-degree steep slope;
- platformer camera;
- cyan dynamic Jolt test box;
- greybox world;
- M15 checker;
- M16 static GLB;
- M17 Blender-authored GLB;
- M18 textured self-contained GLB;
- executable-relative runtime paths;
- current CMake staging architecture.

No unrelated gameplay, physics, camera, rendering, or architecture refactors.

---

# 11. Explicitly out of scope

Do **not** implement in M19:

- Milestone 20;
- DDS;
- KTX/KTX2;
- Basis Universal;
- BCn/ETC/ASTC compression;
- GPU texture compression;
- platform-specific texture variants;
- automatic texture atlases;
- texture arrays;
- texture streaming;
- async loading;
- virtual textures;
- generic texture manager;
- generic material system;
- custom shaders;
- normal-map pipeline;
- metallic/roughness pipeline;
- emissive pipeline;
- occlusion pipeline;
- extraction of textures from GLB;
- modification of embedded GLB images;
- Blender automation;
- Blender CLI integration;
- cooker invoking Blender;
- CMake invoking Blender/cooker;
- hot reload;
- VFS;
- PAK/bundles;
- LOD;
- billboards;
- instancing;
- mesh optimization;
- editor tooling.

---

# 12. Recommended implementation phases

## Phase A — Design and dependency feasibility

Cursor should:

1. Inspect the existing cooker and asset contracts.
2. Define the smallest texture-processing abstraction needed.
3. Determine whether reliable resize support is possible without a new Python dependency.
4. Prepare documentation/tests/contracts without changing runtime behavior unnecessarily.
5. Stop for review if a new image-processing dependency is required.

Phase A must not silently install dependencies.

**Phase A result:** Python stdlib can read PNG IHDR dimensions but cannot decode/resample/encode PNG. raylib's stb is a runtime/build FetchContent dependency, not a cooker API. No suitable resize capability exists in-tree. Do not implement a custom PNG codec. Recommended Phase B dependency (needs explicit approval): pinned Pillow, cooker-only, not a CMake/runtime dependency. `test_checker.png` is 16×16 and cannot prove downscale; Phase B should add a dedicated oversized standalone PNG such as `textures/test_large_checker.png` (1024×512 or 1024×1024), not the M18 authoring PNG. Incremental skip must recompute cook output under the current `runtime_png.max512.v1` recipe so a max-dimension policy change recooks. Keep `schemaVersion` 1; optional `recipe`/dimension fields may be added later without bumping schema. Embedded GLB images stay opaque copies.

## Phase B — Texture cooker implementation

After Phase A approval:

1. Implement validated PNG dimension handling.
2. Implement deterministic proportional downscaling only if the approved dependency strategy supports it.
3. Add focused cooker tests/fixtures.
4. Preserve SHA-256 incremental behavior.
5. Keep GLB processing opaque.

**Phase B result:** Pillow `12.3.0` is a cooker-only pin in `tools/requirements.txt`. Recipe `runtime_png.max512.lanczos.v1` downscales declared runtime PNGs with LANCZOS. Within-limit PNGs stay byte-identical. Test fixture `tools/fixtures/textures/test_large_checker.png` (1024×512) is not a runtime asset. Embedded GLB images remain opaque copies.

## Phase C — Integration and final validation

1. Cook assets twice.
2. Configure/build Debug, Development, Release.
3. Verify staging.
4. Run Development.
5. Verify standalone checker still renders correctly.
6. Verify M18 embedded-texture GLB remains correct.
7. Verify Release from an unrelated CWD.
8. Verify no unintended authoring PNG is staged.
9. Review Debug/Development metrics if added.
10. Perform final Git review before commit.

**Phase C result:** Tooling tests pass. Two cooker runs skip all runtime assets and the manifest. `test_checker.png` remains 16×16 byte-identical. Fixture and M18 Base Color PNG stay out of runtime staging. Debug/Development/Release build. Pillow remains cooker-only.

---

# 13. Acceptance criteria

Milestone 19 is complete when all of the following are true:

- standalone runtime PNG texture policy is documented;
- maximum dimension is explicitly `512 px`;
- aspect ratio is preserved;
- no upscaling occurs;
- PNG source/output policy is explicit;
- processing is deterministic;
- incremental cooking remains correct;
- source-content changes trigger recooking;
- relevant processing-policy changes cannot silently reuse stale cooked output;
- invalid inputs fail clearly;
- cooker tests cover within-limit and over-limit images;
- M15 checker still renders;
- M16/M17 GLBs still render;
- M18 embedded Base Color GLB still renders;
- M18 source Base Color PNG remains absent from runtime staging;
- embedded GLB images remain untouched by the cooker;
- Debug, Development, and Release build successfully;
- Release remains independent of current working directory;
- no gameplay/physics tuning changes;
- no unapproved image-processing dependency was introduced;
- no Milestone 20 work was started.

---

# 14. Final report requirements

At completion, report:

1. implementation summary;
2. files created/modified;
3. final standalone runtime texture policy;
4. dependency decision;
5. PNG validation implementation;
6. resize implementation, if approved/implemented;
7. resizing algorithm/library used;
8. deterministic-output behavior;
9. incremental-cooking behavior;
10. how processing-policy changes invalidate stale output;
11. test assets/fixtures used;
12. within-limit test result;
13. over-limit test result;
14. aspect-ratio test result;
15. no-upscale test result;
16. invalid-input test result;
17. first cooker result;
18. second cooker result;
19. manifest behavior/schema;
20. confirmation authoring Base Color PNG remains outside runtime manifest;
21. confirmation GLB embedded images are untouched;
22. CMake staging result;
23. Debug build result;
24. Development build result;
25. Release build result;
26. runtime validation performed;
27. M15 validation;
28. M16 validation;
29. M17 validation;
30. M18 embedded-texture validation;
31. Release unrelated-CWD validation;
32. Debug Metrics changes, if any;
33. confirmation Release has no ImGui;
34. confirmation gameplay/physics tuning unchanged;
35. confirmation no unrelated dependency was added;
36. confirmation Milestone 20 was not started.

Do not commit, push, or merge until manual validation and approval are complete.

# Milestone 20 — Checkpoint + Fall/Respawn Loop [COMPLETE]

## Goal

Return focus to core platformer gameplay after the M15–M19 asset-pipeline milestones.

Milestone 20 adds the first complete failure/recovery loop:

1. Player starts at the initial spawn.
2. Player can activate exactly one checkpoint.
3. Falling below a kill plane triggers a respawn.
4. Before checkpoint activation, respawn returns to the initial spawn.
5. After checkpoint activation, respawn returns to the checkpoint.
6. Manual respawn is available through semantic input.
7. CharacterVirtual, movement state, moving-platform carry, and camera state are reset coherently.

No lives, savegame, death screen, scene reload, or generic checkpoint framework.

## Architectural Intent

Respawn must not be implemented as a raw visual position assignment.

The authoritative physical player representation is Jolt `CharacterVirtual`, while `Player` owns gameplay movement policy/state. A respawn therefore needs to reset the relevant state across the existing boundaries without leaking Jolt types.

Keep the existing ownership rules:

- `Player` owns gameplay movement policy and timers.
- `PhysicsWorld` owns Jolt and CharacterVirtual physical state.
- gameplay/world code owns checkpoint and respawn meaning.
- Renderer only visualizes state.
- no Jolt types outside `PhysicsWorld.cpp`.
- no raylib input constants in gameplay.
- no general-purpose checkpoint/trigger framework.

## World / Respawn Data

Add the smallest project-owned representation for:

- initial spawn;
- kill-plane Y;
- exactly one checkpoint trigger volume;
- checkpoint respawn position.

A focused file such as `game/source/world/RespawnWorld.h` is acceptable if it fits the repository conventions.

Conceptually:

```cpp
struct CheckpointSpec
{
    core::Vec3 center;
    core::Vec3 size;
    core::Vec3 respawnPosition;
};
```

Preserve the existing CharacterVirtual position convention established in M11. Verify it before choosing exact spawn/checkpoint coordinates.

Initial kill-plane target: `Y = -8.0`.

Place the checkpoint in an area requiring traversal, preferably near/on the existing elevated right platform. Inspect actual current geometry before selecting final coordinates.

Checkpoint activation may use a simple project-owned AABB/point-volume test. Do not add a Jolt sensor.

## Runtime Respawn State

Keep state minimal, conceptually:

```cpp
struct RespawnState
{
    bool checkpointActive;
    core::Vec3 respawnPosition;
    int deathCount;
};
```

Optional diagnostic: last respawn reason (`Fall` or `Manual`).

PhysicsWorld must not know what a checkpoint means. Renderer must not own checkpoint gameplay state.

## Manual Respawn

Add semantic input:

```cpp
bool respawnPressed;
```

Map `R -> respawnPressed` in the platform/input layer. Gameplay must not query `KEY_R` directly.

Before checkpoint activation, R returns to initial spawn. After activation, R returns to checkpoint. Manual respawn does not increment `deathCount`.

## CharacterVirtual Reset

Expose the smallest project-owned, Jolt-free PhysicsWorld API needed to safely reset the character, preferably a cohesive operation such as:

```cpp
void ResetCharacter(const core::Vec3& position, const core::Vec3& velocity);
```

On respawn, coherently reset:

1. CharacterVirtual position.
2. CharacterVirtual linear velocity to zero.
3. Player relative horizontal velocity to zero.
4. Player vertical velocity to zero.
5. Moving-platform horizontal carry to zero.
6. Coyote timer to zero.
7. Jump buffer timer to zero.
8. stale grounded/support state safely.
9. fixed gameplay Z.
10. project-facing Player position synchronized from CharacterVirtual.

The moving platform itself must not reset.

## Camera Snap

M08 smoothing must not interpolate across the teleport. Add the smallest camera operation needed to immediately synchronize desired and smoothed target state with the respawn position, e.g. `SnapToTarget(...)`.

After the snap, normal dead-zone and exponential smoothing resume.

## Checkpoint Visual

Use existing Renderer primitives only. Render a simple marker/beacon with clearly distinguishable inactive and active states.

No new asset, shader, particles, or animation system.

## Debug / Development Metrics

Extend the existing Dear ImGui metrics panel with read-only checkpoint/respawn data, such as:

- Checkpoint active
- Respawn position X/Y/Z
- Player Y
- Kill plane Y
- Death count
- Last respawn reason (optional)

No debug UI in Release.

## Existing Gameplay Constants

Do not change:

```text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Preserve CharacterVirtual, moving platform, slopes, cyan Jolt box, camera behavior except respawn snap, and M15–M19 asset behavior.

## Manual Validation

| Test | Expected result |
|---|---|
| Fall before checkpoint | Respawn at initial spawn |
| Activate checkpoint | Marker changes to active |
| Fall after checkpoint | Respawn at checkpoint |
| R before checkpoint | Initial spawn |
| R after checkpoint | Checkpoint |
| Fall with horizontal velocity | Respawn stationary |
| Fall after jumping from moving platform | No residual platform carry |
| Jump input around death | No stale automatic jump |
| Respawn | Camera snaps immediately |
| Normal movement/jump after respawn | M07 behavior remains normal |
| Moving platform | Still works |
| Walkable/steep slopes | Still work |
| Cyan dynamic box | Still works |
| Development metrics | Correct state |
| Release | No debug UI |

## Explicitly Out of Scope

Do not implement multiple checkpoints, persistence/savegames, lives, health, damage, enemies, death animation/screen, fade transition, audio, particles, collectibles, score, full level restart, scene reload, Jolt checkpoint sensors, generic trigger/respawn framework, event bus, ECS, generic level manager/state machine, new art assets, new dependencies, asset-cooker changes, texture-pipeline changes, or Milestone 21.

## Completion Criteria

- [ ] Initial spawn explicitly represented.
- [ ] Kill plane is project-owned gameplay/world data.
- [ ] Exactly one checkpoint exists.
- [ ] Activation is independent of raylib/Jolt APIs.
- [ ] Checkpoint visual has inactive/active states.
- [ ] CharacterVirtual reset uses project-owned PhysicsWorld API.
- [ ] Physical and gameplay velocities reset.
- [ ] Moving-platform carry resets.
- [ ] Coyote and jump-buffer timers reset.
- [ ] Fixed gameplay Z remains correct.
- [ ] Camera snaps on respawn.
- [ ] R is semantic input.
- [ ] Fall increments death count.
- [ ] Manual respawn does not increment death count.
- [ ] Debug/Development metrics expose useful state.
- [ ] Release contains no debug UI.
- [ ] M01–M19 remain intact.
- [ ] No new dependency or generic framework.
- [ ] Milestone 21 not started.

## Recommended Phases

### Phase A — Architecture / State Integration

Inspect current Player, PhysicsWorld, Camera, Input, moving-platform and application update flow. Define ownership, reset APIs, semantic input, world data and exact update ordering. Stop for review before full feature implementation.

**Phase A:** `world/RespawnWorld.h` and `gameplay::RespawnState` exist. Semantic `respawnPressed` (`R`) is mapped. `ResetCharacter`, `ResetMovementState`, and `SnapToTarget` compile. Kill-plane, checkpoint activation, death count, checkpoint visuals, and the respawn update-order wiring were not active yet.

### Phase B — Gameplay Implementation
Implement checkpoint activation/visual, kill-plane and manual respawn, CharacterVirtual reset, transient-state cleanup, camera snap, death count and debug metrics.

**Phase B:** After `Player::Update`, Application evaluates Fall then Manual, performs at most one coherent CharacterVirtual/Player/camera reset, and skips checkpoint activation on a respawn frame. Checkpoint overlap uses visual-center vs `world::kCheckpoint`. Renderer draws a post+beacon from `checkpointActive` only. Debug metrics expose Respawn / Checkpoint. R and kill-plane are live.

### Phase C — Regression / Runtime Validation (current)
Validate all manual cases, all configurations, Release behavior, and M01–M19 regressions before Git closure.

**Phase C:** User-validated. Milestone 20 is complete. Checkpoint, kill-plane, manual/fall respawn, death count, camera snap, and marker visuals are live.

## Git Branch

```powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/20-checkpoint-respawn
git branch --show-current
```

Expected: `milestone/20-checkpoint-respawn`.

## Recommended Model

Use **Grok 4.6 High — Fast OFF** because this milestone touches interacting CharacterVirtual state, movement timers, moving-platform carry, input semantics, update ordering and camera state.

## Workflow

`main clean → define milestone → update docs/MILESTONES.md → create branch → Phase A → review → Phase B → review → Phase C/manual tests → approve → commit → push branch → merge main → push main → verify clean main`

# Milestone 21 — Level Goal + Completion Loop [COMPLETE]

## Goal

Build the first explicit level-completion condition for the platformer.

Milestone 20 established the failure/recovery loop. Milestone 21 adds the successful end of that loop:

```text
spawn → traversal → checkpoint/platforming → goal volume → levelCompleted = true → completion feedback
```

This milestone adds exactly one level goal, a project-owned completion state, a simple visual goal marker, and minimal player-facing completion feedback.

It does **not** add multiple levels, scene transitions, menus, save data, scoring, collectibles, or a generic game-state framework.

## Core Design

- `Application` owns the current run-level completion state.
- Project-owned world data defines the goal volume and marker placement.
- `PhysicsWorld` does not know what a goal means.
- `Player` does not own level completion.
- `Renderer` visualizes the goal and completion feedback but does not decide completion.
- Jolt is not required for goal detection.
- Do not generalize checkpoint + goal into a trigger framework.

## Level Goal World Data

Add a focused project-owned goal specification, e.g. `game/source/world/LevelGoal.h`.

Conceptually:

```cpp
struct LevelGoalSpec
{
    core::Vec3 center;
    core::Vec3 size;
};
```

The exact final coordinates must be chosen only after inspecting the current greybox, slopes, moving platform, and checkpoint layout. The goal must require meaningful traversal, must not overlap the M20 checkpoint trigger, and must be reachable with the existing movement constants.

## Goal Detection

Use a simple project-owned test against the player's current visual-center convention, conceptually `PointInsideGoal(player.Position())`.

Do not use Jolt sensors, collision callbacks, raylib collision helpers in gameplay, generic trigger volumes, or event buses.

## Completion State

Keep state minimal:

```cpp
struct LevelCompletionState
{
    bool completed = false;
};
```

Own it alongside other run-level state in `Application` unless inspection reveals an already-existing better project-owned location.

## Completion Semantics

On first entry into the goal:

```text
levelCompleted = true
```

Completion is one-way for the current application run. After completion, the player may continue moving; physics, moving platform, checkpoint and respawn remain functional; `R` remains manual respawn; falling still increments `deathCount`; respawning does not clear completion.

Do not introduce a pause/game-state machine in M21.

## Goal vs Checkpoint

Checkpoint and goal remain separate concepts. Do not merge them into a generic trigger abstraction.

The goal must not change `respawnPosition`, activate the checkpoint, reset `deathCount`, reset Player movement, reset CharacterVirtual, or snap the camera.

## Goal Visual

Render a clear marker using existing primitives only, for example two posts plus a top bar/beacon. It must have obvious not-completed and completed visual states.

No model assets, textures, shaders, particles, animation systems, or new dependencies.

## Player-Facing Completion Feedback

When completed, show minimal visible feedback such as:

```text
LEVEL COMPLETE
```

Keep raylib rendering details behind the existing render/UI boundary. Gameplay must not call raylib text functions directly.

No menu, buttons, transition, score screen, animation sequence, fade, or input lock.

The message may remain visible for the rest of the run.

## Input

Add no new input. Preserve movement, jump, and manual respawn (`R`). Do not overload `R` with level restart.

## Physics

No new Jolt body/sensor/callback/layer. `PhysicsWorld` remains unaware of level completion. CharacterVirtual behavior remains unchanged.

## Update Ordering

Inspect the actual M20 loop first. Intended relationship:

```text
Poll input
↓
UpdateMovingPlatform
↓
Player::Update
↓
Fall / Manual respawn decision
↓
if respawn:
    PerformRespawn
else:
    checkpoint activation
    goal completion test
↓
PhysicsWorld::Update
↓
camera update/snap behavior
↓
render
```

Rules:

1. A respawn frame must not complete the goal because of teleport.
2. Goal detection occurs only when no respawn happened that frame.
3. Checkpoint behavior remains unchanged.
4. Completion does not interrupt physics.
5. Completion does not teleport CharacterVirtual.
6. Completion does not change camera behavior.

Do not reorder the M13/M14 physics-sensitive pipeline unnecessarily.

## Respawn After Completion

Completion remains true after later manual/fall respawns:

```text
reach goal → completed = true → R/fall → normal M20 respawn → completed remains true
```

Do not implement level-reset semantics yet.

## Debug / Development Metrics

Add a small read-only Level Goal section:

```text
Level completed: true/false
Goal center: X/Y/Z
Goal size: X/Y/Z
Player inside goal: true/false
```

No Dear ImGui metrics in Release.

## Release Behavior

Release must include the gameplay goal marker and `LEVEL COMPLETE` feedback, while Dear ImGui diagnostics remain absent.

## Existing Systems to Preserve

Do not change:

```text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Preserve M08 camera, M11 CharacterVirtual, M13 moving-platform carry, M14 slopes, M15–M19 assets, M20 checkpoint/kill plane/respawn/death count/camera snap, and the cyan dynamic Jolt box.

## Manual Validation

| Test | Expected |
|---|---|
| Start fresh | Level not completed |
| Approach goal without entering | No completion |
| Enter goal | `levelCompleted = true` |
| Goal marker | Changes to completed visual |
| Completion feedback | `LEVEL COMPLETE` visible |
| Stay inside goal | No repeated completion behavior |
| Leave goal | Completion remains true |
| R after completion | Normal M20 respawn; completion remains true |
| Fall after completion | Normal M20 fall/deathCount; completion remains true |
| Checkpoint | Still activates and controls respawn |
| Moving platform | Still carries player correctly |
| Slopes | 30°/60° behavior unchanged |
| Camera | Normal follow and M20 respawn snap unchanged |
| Cyan box | Still behaves normally |
| Release | Goal/message present, no debug UI |

## Explicitly Out of Scope

Do not implement Milestone 22, multiple levels, level loading, scene transitions/reload, level restart, next-level button, menus, savegames, persistence, times/leaderboards, score/stars/medals, collectibles, enemies, health/lives, audio, particles, completion animation system, fades, Jolt goal sensor, generic trigger/event/game-state frameworks, ECS, generic level manager, new assets/dependencies, or asset-pipeline changes.

## Completion Criteria

- [ ] Exactly one project-owned level goal exists.
- [ ] Goal is reachable and does not overlap checkpoint trigger.
- [ ] Goal detection uses project-owned logic.
- [ ] No Jolt sensor/body is used for the goal.
- [ ] Run-level owner owns completion state.
- [ ] Player/PhysicsWorld/Renderer ownership boundaries remain correct.
- [ ] Goal completes only once per run.
- [ ] Completion survives later M20 respawns.
- [ ] Checkpoint/death count/moving platform/camera remain unchanged.
- [ ] Goal has inactive/completed visual states.
- [ ] `LEVEL COMPLETE` is visible to the player.
- [ ] Release includes completion feedback but no Dear ImGui metrics.
- [ ] Gameplay constants remain unchanged.
- [ ] M15–M19 asset behavior remains unchanged.
- [ ] No new dependency or generic framework is added.
- [ ] Milestone 22 is not started.

## Recommended Phases

### Phase A — Goal Architecture and Placement

Inspect M20 world/update/render architecture. Define exact goal location/dimensions, world-data representation, completion-state ownership, overlap helper, update insertion point, goal visual, minimal completion-message rendering path, and debug metrics. Small foundational APIs/data may be added, but do not wire the complete loop yet.

**Phase A:** `world/LevelGoal.h` (`kLevelGoal` center `{-4.5, 3.3, 0}`, size `{2.0, 1.6, 1.8}`) and `gameplay::LevelCompletionState` exist and are owned by `Application`. `PointInsideGoal` tests visual center. Renderer draws an incomplete two-post + bar marker on the left platform. Goal detection, `completed = true`, `LEVEL COMPLETE`, and Level Goal ImGui metrics were not wired yet.

### Phase B — Completion Gameplay

Implement goal detection, one-way completion state, visual states, `LEVEL COMPLETE`, metrics, and persistence of completion across M20 respawns.

**Phase B:** On non-respawn frames, Application sets `completed` once via `PointInsideGoal` after checkpoint evaluation. Renderer consumes `levelCompleted` for marker colors and draws `LEVEL COMPLETE` after `EndMode3D` in all configurations. Debug/Development metrics expose the Level Goal section. Completion is not cleared by `PerformRespawn`.

### Phase C — Regression and Manual Validation (current)

Validate goal completion, Release behavior, checkpoint/respawn, moving platform, slopes, camera, cyan box, and M15–M19 runtime assets.

**Phase C:** User-validated. Milestone 21 is complete. Goal detection, marker states, `LEVEL COMPLETE`, and completion persistence across M20 respawns are live.

Do not commit/push/merge until final approval.

## Git Branch

```powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/21-level-completion
git branch --show-current
```

Expected: `milestone/21-level-completion`.

## Recommended Model

Use **Grok 4.6 High — Fast OFF** for Phase A. If Phase A confirms a straightforward integration, Phase B may use Grok 4.6 High Fast.

## Workflow

```text
main clean → define M21 → update docs/MILESTONES.md → create branch → Phase A → review → Phase B → review → Phase C/manual tests → approve → commit → push → merge main → push main → verify clean main
```

# Milestone 22 --- Level Restart + Run-State Reset [COMPLETE]

## Goal

Add the first explicit restart loop after level completion, without
introducing multiple levels or a generic game-state framework.

M20 established failure/recovery:

``` text
fall / manual respawn -> continue current run
```

M21 established success:

``` text
reach goal -> levelCompleted = true
```

M22 adds a deliberate new-run action:

``` text
reach goal
    ↓
LEVEL COMPLETE
    ↓
press semantic Restart
    ↓
reset run-level state
    ↓
return to initial spawn
    ↓
play the same level again
```

The level is not reloaded from disk. Static world geometry and runtime
assets remain loaded.

------------------------------------------------------------------------

## Scope

Implement exactly one restart action for the current single level.

Restart must reset the gameplay state that belongs to the current run:

-   Player/CharacterVirtual position and transient movement state
-   checkpoint activation
-   respawn destination
-   death count
-   last respawn reason
-   level completion state
-   camera target/smoothing state

Restart must also reset the moving platform to its canonical initial
motion state so the new run begins deterministically.

The cyan dynamic Jolt test box should also return to its canonical
initial transform/velocity if the current PhysicsWorld architecture can
do so through a small project-owned reset API.

Do not reload the process, scene, assets, or physics world.

------------------------------------------------------------------------

## Restart Availability

The restart action is only meaningful after:

``` text
levelCompleted == true
```

Before completion, pressing the restart key should do nothing.

This prevents the new action from becoming a second manual-respawn
shortcut.

------------------------------------------------------------------------

## Semantic Input

Add one semantic action:

``` cpp
bool restartPressed;
```

Recommended keyboard binding:

``` text
Enter -> restartPressed
```

The raylib key constant must remain isolated in the platform/input
backend.

Gameplay/Application sees only `restartPressed`.

Existing input remains unchanged:

-   A/D or arrows -\> movement
-   Space/Up -\> jump
-   R -\> manual respawn
-   Enter -\> restart completed run

Do not overload R.

------------------------------------------------------------------------

## Completion Feedback

After completion, retain:

``` text
LEVEL COMPLETE
```

and add a small instruction such as:

``` text
PRESS ENTER TO RESTART
```

Both are gameplay UI and must exist in Release.

They must be rendered through the existing Renderer/backend boundary,
not through Dear ImGui.

Before completion, neither completion message should be visible.

------------------------------------------------------------------------

## Run-State Ownership

Application remains the owner/coordinator of run-level meaning.

A focused helper such as:

``` text
RestartRun()
```

or equivalent is acceptable in `Application`.

Do not create:

-   GameStateManager
-   LevelManager
-   RestartManager
-   generic state machine
-   event bus
-   ECS

M22 is still a single-level game.

------------------------------------------------------------------------

## Canonical Restart State

After restart, the run should match a fresh launch as closely as
practical.

Expected gameplay state:

``` text
Player visual center      = {0.0, 0.8, 0.0}
Player velocities         = zero
Player carry              = zero
Player grounded/support   = refreshed/clean
jump buffer               = clear
coyote state              = clear

checkpointActive          = false
respawnPosition           = initial spawn
deathCount                = 0
lastRespawnReason         = None

levelCompleted            = false

camera                    = snapped to initial spawn

moving platform           = canonical initial position/direction/target
dynamic cyan box          = canonical initial transform/velocity
```

The exact canonical moving-platform state and cyan-box state must be
derived from the current implementation rather than guessed.

------------------------------------------------------------------------

## Restart vs Respawn

Restart and respawn are different operations.

### Respawn (M20)

``` text
R or fall
    -> teleport Player to current respawnPosition
    -> preserve checkpoint activation
    -> preserve levelCompleted
    -> fall may increment deathCount
    -> moving platform continues
```

### Restart (M22)

``` text
Enter after completion
    -> begin a fresh run of the same level
    -> initial spawn
    -> checkpoint cleared
    -> deathCount cleared
    -> completion cleared
    -> moving platform reset
    -> test dynamic body reset
```

Do not implement restart by abusing the existing manual-respawn path
alone.

Reuse low-level reset helpers where appropriate, but keep the gameplay
semantics explicit.

------------------------------------------------------------------------

## Physics Boundary

PhysicsWorld remains responsible only for physical reset operations.

Small project-owned APIs are acceptable, for example:

``` cpp
ResetCharacter(...)
ResetMovingPlatform()
ResetTestDynamicBody()
```

or one narrowly scoped physical-run reset method if inspection shows
that is cleaner.

Public APIs must use project-owned types only.

No Jolt types in project headers.

PhysicsWorld must not know:

-   checkpointActive
-   deathCount
-   levelCompleted
-   restartPressed

Application coordinates those meanings.

------------------------------------------------------------------------

## Moving Platform Reset

M13 currently lets the moving platform continue through normal respawns.

M22 restart is different: a fresh run should reset it.

Inspect its actual canonical initial values and restore:

-   initial position
-   initial target/direction
-   physical kinematic transform
-   any carried-ground velocity state relevant to the CharacterVirtual

The platform must resume normal M13 movement on subsequent frames.

Do not alter its speed/path/tuning.

------------------------------------------------------------------------

## Cyan Dynamic Box Reset

The existing cyan Jolt box is a technical physics object.

For deterministic restart, reset it to the same initial transform used
at application initialization and clear its linear/angular velocity.

If a tiny focused reset API is required, add it behind PhysicsWorld.

Do not expose Jolt body IDs/types publicly.

Do not redesign the test body into a gameplay entity.

------------------------------------------------------------------------

## Update Ordering

Inspect the actual M21 loop first.

Restart should be evaluated after input is available and after the
current frame's Player update/respawn/completion logic has established
the state, but it must not allow a second Player movement after the
restart teleport.

A likely safe relationship is:

``` text
Poll input
↓
UpdateMovingPlatform
↓
Player::Update
↓
Fall / Manual respawn
↓
if no respawn:
    checkpoint activation
    goal completion
↓
if levelCompleted && restartPressed:
    RestartRun
    mark restartThisFrame
↓
PhysicsWorld::Update
↓
camera.Update only if neither respawn nor restart snapped it
↓
render
```

Phase A must inspect whether restart should occur before or after the
normal moving-platform step for the frame and define the exact
invariant.

Critical rules:

-   at most one restart per frame
-   restart only when completed
-   no Player::Update after restart in the same frame
-   no goal re-completion in the restart frame
-   no checkpoint activation caused by restart teleport
-   camera performs a hard snap
-   the next frame resumes normal gameplay

------------------------------------------------------------------------

## Interaction with Same-Frame Inputs

Define deterministic priority.

Recommended:

``` text
Fall
    > Manual Respawn
    > normal checkpoint/goal evaluation
    > Restart if completed
```

Because restart is only allowed after completion, an already-completed
run may receive R and Enter on the same frame.

Phase A must inspect the cleanest policy.

Preferred behavior:

``` text
if a respawn occurred this frame:
    do not restart this frame
```

This preserves the existing M20 respawn semantics and avoids two
teleports/reset operations in one frame.

Restart may occur on a later fresh Enter press.

------------------------------------------------------------------------

## Camera

Restart must hard-snap the M08 camera to the initial spawn.

No interpolation across the map.

Normal dead zone/smoothing resumes on the next frame.

Do not change:

``` text
horizontal dead zone = 1.5
vertical dead zone   = 0.75
sharpness            = 8
```

------------------------------------------------------------------------

## Debug / Development Metrics

Add a small read-only restart/run section or extend the existing
relevant metrics.

Useful values:

``` text
Level completed
Checkpoint active
Death count
Restart available
Restart pressed
```

Avoid duplicating every existing metric.

Dear ImGui remains Debug/Development only.

------------------------------------------------------------------------

## Release Behavior

Release must contain:

-   LEVEL COMPLETE
-   PRESS ENTER TO RESTART
-   restart gameplay

Release must not contain Dear ImGui/debug metrics.

No restart behavior may depend on `PLATFORMER_ENABLE_DEBUG_UI`.

------------------------------------------------------------------------

## Asset Pipeline

No asset changes.

Do not modify:

-   source assets
-   cooked assets
-   cooker recipes
-   Pillow dependency
-   Blender workflow
-   CMake runtime asset staging

M15--M19 behavior must remain unchanged.

------------------------------------------------------------------------

## Existing Systems to Preserve

Do not change gameplay tuning:

``` text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Preserve:

-   M08 camera behavior
-   M11 CharacterVirtual
-   M13 moving-platform behavior during normal gameplay/respawn
-   M14 slopes
-   M20 checkpoint/fall/manual respawn
-   M21 one-way completion within a run
-   M15--M19 asset/runtime behavior

------------------------------------------------------------------------

## Manual Validation

M22 is complete when the following are manually validated:

  -----------------------------------------------------------------------
  Test                                Expected
  ----------------------------------- -----------------------------------
  Fresh run + Enter                   No restart effect

  Complete level                      `LEVEL COMPLETE` + restart
                                      instruction

  Enter after completion              Fresh run at initial spawn

  Restart Player                      Position/velocities/carry clean

  Restart checkpoint                  Inactive, respawn back to initial
                                      spawn

  Restart death count                 `0`, reason `None`

  Restart completion                  `false`, completion UI disappears

  Restart camera                      Immediate snap to initial spawn

  Restart moving platform             Canonical initial motion state

  Restart cyan box                    Canonical initial
                                      transform/velocity

  R before completion                 Existing M20 manual respawn

  R after completion                  Existing M20 respawn; completion
                                      preserved unless Enter restart is
                                      later used

  Fall after completion               Existing M20 fall; completion
                                      preserved

  Complete again after restart        Goal can be completed again in new
                                      run

  Moving platform after restart       Normal carry/reversal works

  Slopes after restart                30°/60° behavior unchanged

  Release                             Completion/restart UI and behavior
                                      work, no ImGui
  -----------------------------------------------------------------------

------------------------------------------------------------------------

## Explicitly Out of Scope

Do not implement:

-   Milestone 23
-   multiple levels
-   next level
-   level selection
-   scene loading
-   scene reload from disk
-   process restart
-   main menu
-   pause menu
-   completion menu
-   savegame
-   persistence
-   score
-   timer
-   leaderboard
-   collectibles
-   enemies
-   health
-   lives
-   audio
-   particles
-   fade transitions
-   animation system
-   generic trigger system
-   generic event bus
-   ECS
-   generic game-state/state-machine framework
-   generic level manager
-   new assets
-   new dependencies
-   asset cooker changes

------------------------------------------------------------------------

## Completion Criteria

-   [ ] Semantic `restartPressed` exists.
-   [ ] Enter/key constant is isolated in Input backend.
-   [ ] Restart is available only after level completion.
-   [ ] Completion UI includes restart instruction.
-   [ ] Restart returns Player to canonical initial spawn.
-   [ ] CharacterVirtual velocity/state is clean.
-   [ ] Player transient movement state is clean.
-   [ ] Checkpoint is cleared.
-   [ ] Respawn destination returns to initial spawn.
-   [ ] Death count returns to zero.
-   [ ] Last respawn reason returns to None.
-   [ ] Level completion returns to false.
-   [ ] Camera snaps to initial spawn.
-   [ ] Moving platform resets deterministically.
-   [ ] Cyan dynamic box resets deterministically.
-   [ ] Normal M20 respawn does not become restart.
-   [ ] R behavior remains unchanged.
-   [ ] Fall behavior remains unchanged.
-   [ ] Goal can be completed again after restart.
-   [ ] No second Player update occurs on restart frame.
-   [ ] No goal/checkpoint activation occurs because of restart
    teleport.
-   [ ] PhysicsWorld remains free of gameplay restart meaning.
-   [ ] No Jolt types leak into public headers.
-   [ ] Release contains restart gameplay/UI.
-   [ ] Release contains no Dear ImGui metrics.
-   [ ] Gameplay constants remain unchanged.
-   [ ] M15--M19 remain unchanged.
-   [ ] No new dependency is added.
-   [ ] No generic game-state/level framework is introduced.
-   [ ] Milestone 23 is not started.

------------------------------------------------------------------------

## Recommended Phases

### Phase A --- Restart Contract and Reset Inventory

Inspect the real M21 code and define:

-   exact input binding/semantic action
-   exact restart priority/order
-   all run-level state that must reset
-   canonical moving-platform initial state
-   canonical cyan-box initial state
-   PhysicsWorld reset APIs
-   Application restart coordinator
-   camera reset
-   completion/restart UI path
-   metrics
-   regression risks

Small foundational APIs may be added, but do not wire the complete
restart loop yet.

**Phase A:** `restartPressed` maps `Enter` in `Input.cpp` only. `PhysicsWorld::ResetMovingPlatform` / `ResetDynamicTestBox` exist. `Application::RestartRun` coordinates the full reset. Approved.

### Phase B --- Restart Gameplay

**Phase B:** After poll, Application captures `restartAvailableAtFrameStart`. After checkpoint/goal, if no respawn and restart was already available and `restartPressed`, it calls `RestartRun()`. Same-frame goal + Enter completes and does not restart. Renderer draws `PRESS ENTER TO RESTART` under `LEVEL COMPLETE` while completed. Approved.

Implemented:

-   semantic restart input
-   completion restart instruction
-   Application restart coordination
-   physical resets
-   run-state resets
-   camera snap
-   metrics
-   Release behavior

### Phase C --- Regression and Manual Validation

**Phase C:** Implementation complete and manually validated. Restart gameplay, UI, and M20/M21 regressions accepted. Do not start Milestone 23 from this section.

Inspection confirmed:

-   restart only after completion already true at frame start
-   same-frame goal + Enter completes and does not restart
-   Fall > Manual > Restart
-   RestartRun canonical Player/M20/M21/platform/box/camera state
-   M20 respawn does not become restart
-   M21 goal geometry unchanged
-   Release contains restart gameplay/UI and omits Dear ImGui
-   cooker/assets/constants unchanged

------------------------------------------------------------------------

## Git Branch

After `main` is clean and the milestone definition is accepted:

``` powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/22-level-restart
git branch --show-current
```

Expected:

``` text
milestone/22-level-restart
```

------------------------------------------------------------------------

## Recommended Model

Use **Grok 4.6 High --- Fast OFF** for Phase A.

Restart crosses Application run-state ownership, CharacterVirtual reset,
moving-platform kinematic state, the dynamic Jolt body, camera snapping,
input priority, and Release UI. The architectural inspection matters
more than raw implementation speed.

If Phase A produces a clean narrow reset contract, Phase B may use Grok
4.6 High Fast.

------------------------------------------------------------------------

## Workflow

``` text
main clean
    ↓
define M22
    ↓
update docs/MILESTONES.md
    ↓
create milestone branch
    ↓
Phase A
    ↓
review
    ↓
Phase B
    ↓
review
    ↓
Phase C / manual tests
    ↓
approve
    ↓
commit
    ↓
push branch
    ↓
merge main
    ↓
push main
    ↓
verify clean main
```

# Milestone 23 --- Dynamic Body Interaction Safety

## Goal

Harden the existing Player ↔ dynamic-body interaction so the cyan Jolt
test box cannot leave the CharacterVirtual permanently wedged or unable
to move when the box, player, moving platform, and static world geometry
converge.

This milestone is based on the manually observed issue after M22:

``` text
dynamic cyan box moves/falls to the right
        +
player enters the same constrained area
        +
moving platform / static geometry closes space
        ↓
player can become physically stuck
        ↓
manual R respawn is currently required to recover
```

M23 must diagnose the actual cause in the current Jolt/CharacterVirtual
integration before choosing a fix.

The goal is **interaction safety**, not a generic physics redesign.

------------------------------------------------------------------------

## Why M23 Comes Next

M22 completed the single-level run loop:

``` text
spawn -> traversal -> checkpoint -> goal -> restart -> new run
```

Before expanding the level or introducing multiple checkpoints, the
current physics sandbox should be robust enough that an existing dynamic
object does not create an avoidable gameplay hard-lock.

The later idea of a longer level and multiple checkpoints is
intentionally deferred.

------------------------------------------------------------------------

## Primary Success Condition

After M23:

-   the cyan dynamic box remains a real Jolt dynamic body;
-   the player can still physically interact with it;
-   the box can still fall, move, collide, sleep/wake, and reset on M22
    restart;
-   the moving platform still works;
-   but the player should not become permanently trapped in the observed
    box/platform/static-geometry situation during normal traversal.

A temporary collision block is acceptable.

A state requiring `R` solely because the CharacterVirtual became
physically wedged by the technical dynamic box is not.

------------------------------------------------------------------------

## Phase A Must Diagnose First

Do not assume the fix.

Inspect the current implementation and determine:

1.  how CharacterVirtual contacts dynamic bodies;
2.  whether the CharacterVirtual currently applies impulses/forces to
    dynamic bodies;
3.  whether dynamic bodies can push the CharacterVirtual;
4.  how `UpdateGroundVelocity`, `SetLinearVelocity`,
    `CharacterVirtual::Update`, and `PhysicsSystem::Update` interact;
5.  whether the observed lock is:
    -   expected geometric blocking,
    -   penetration/recovery failure,
    -   stale support/contact state,
    -   insufficient player push strength,
    -   dynamic-body mass/response issue,
    -   moving-platform compression,
    -   or another cause;
6.  which Jolt CharacterVirtual facilities are already available in the
    pinned Jolt version;
7.  whether the current cyan-box mass/body settings contribute to the
    problem.

Do not tune blindly.

------------------------------------------------------------------------

## Preferred Fix Philosophy

Use the smallest solution supported by the actual diagnosis.

Preferred order:

1.  correct misuse/incomplete use of CharacterVirtual dynamic-body
    interaction if present;
2.  add a narrowly scoped CharacterVirtual contact/push policy if Jolt
    expects one;
3.  adjust only the technical cyan-box physical properties if they are
    clearly inappropriate;
4.  add a small anti-wedge safety measure only if the physics
    integration itself cannot reasonably prevent the observed trap.

Do not begin with teleport-based unstuck logic.

Manual respawn remains a gameplay feature, not the primary physics fix.

------------------------------------------------------------------------

## CharacterVirtual / Jolt Boundary

All Jolt-specific implementation remains inside:

``` text
PhysicsWorld.cpp
```

Public project headers must continue to expose only project-owned types.

Do not leak:

-   JPH::BodyID
-   JPH::CharacterVirtual
-   JPH::ContactSettings
-   JPH::Vec3
-   JPH::RVec3
-   JPH::Quat
-   Jolt listener types

If a listener/callback is required, keep it private to the PhysicsWorld
implementation.

------------------------------------------------------------------------

## Player Ownership

Player continues to own:

-   movement intent;
-   acceleration/deceleration;
-   jump policy;
-   gravity policy;
-   coyote time;
-   jump buffer;
-   relative horizontal velocity.

PhysicsWorld continues to own:

-   CharacterVirtual;
-   physical contacts;
-   static/dynamic/kinematic bodies;
-   Jolt-specific collision response;
-   moving-platform physical state.

Do not move gameplay movement policy into Jolt callbacks.

------------------------------------------------------------------------

## Dynamic Box

The cyan box remains a technical dynamic physics body.

Current canonical restart state from M22:

``` text
position         = {0.0, 5.0, 0.0}
size             = {1.0, 1.0, 1.0}
rotation         = identity
linear velocity  = zero
angular velocity = zero
activation       = active
```

M23 may change its physical material/mass/inertia-related setup only if
Phase A demonstrates that the existing setup is part of the problem.

Do not change its visual identity or convert it to static/kinematic
merely to avoid interaction.

------------------------------------------------------------------------

## Moving Platform

Preserve M13:

``` text
start            = {0.0, 1.3, 0.0}
path X           = [-6, +6]
speed            = 2.5
size             = {4.0, 0.4, 3.0}
initial direction= +X
```

Preserve:

-   standing carry;
-   airborne carry;
-   reversal;
-   normal M20 respawn behavior;
-   deterministic M22 restart.

Do not solve the box problem by disabling moving-platform collision.

------------------------------------------------------------------------

## Static World / Slopes

Preserve existing M14 geometry and slope behavior.

Do not move platforms, slopes, checkpoint, goal, or kill plane as the
primary M23 fix.

The observed issue should first be solved at the dynamic-body
interaction layer.

------------------------------------------------------------------------

## No New Gameplay Mechanic

M23 does not add:

-   attack;
-   grab;
-   push button;
-   dash;
-   crouch;
-   wall jump;
-   damage;
-   health;
-   lives;
-   enemy logic.

The player should interact with the box using the existing movement
only.

------------------------------------------------------------------------

## Debug Diagnostics

Phase B may add small Debug/Development-only diagnostics if they
materially help verify the fix.

Useful candidates:

``` text
dynamic contact count
dynamic body contact this frame
support body type
player world velocity
box position / velocity
box active/sleeping
```

Only add values that are actually useful to the diagnosis.

Do not build a generic physics inspector.

Release remains free of Dear ImGui.

------------------------------------------------------------------------

## Optional Debug Visualization

If the diagnosis genuinely benefits from it, a minimal
Debug/Development-only visualization may show:

-   CharacterVirtual bounds;
-   cyan-box bounds;
-   contact normal(s);
-   support/contact point.

This is optional.

Do not create a general collision-debug renderer.

Do not add Release visualization.

------------------------------------------------------------------------

## Anti-Wedge Safety Rule

If, after correct Jolt dynamic-body interaction is implemented, a
residual compression case still exists, Phase A/B may define one
narrowly scoped safety invariant.

Example concept:

``` text
the CharacterVirtual must retain a physically valid escape direction
instead of accumulating unrecoverable penetration/contact state
```

Any safety mechanism must:

-   be deterministic;
-   not teleport during ordinary contacts unless absolutely justified;
-   not trigger during normal wall/platform blocking;
-   not bypass the kill-plane/respawn system;
-   not create upward boosts;
-   not alter jump/coyote/buffer semantics;
-   not become a generic "unstuck system" unless evidence requires it.

------------------------------------------------------------------------

## Update Order

Preserve the proven M22 ordering unless diagnosis shows a concrete
defect:

``` text
Poll
↓
capture restart availability
↓
UpdateMovingPlatform
↓
Player::Update / CharacterVirtual movement
↓
Fall / Manual
↓
checkpoint / goal
↓
Restart if eligible
↓
PhysicsWorld::Update
↓
camera update when allowed
↓
render
```

Do not reorder CharacterVirtual and PhysicsSystem steps speculatively.

If Phase A finds that ordering is the actual cause, report the evidence
and propose the smallest correction before implementing it.

------------------------------------------------------------------------

## M20 / M21 / M22 Preservation

M23 must preserve:

### M20

-   R manual respawn;
-   kill-plane fall;
-   checkpoint;
-   death count;
-   camera snap;
-   moving-platform carry semantics.

### M21

-   one goal;
-   one-way completion within a run;
-   `LEVEL COMPLETE`.

### M22

-   Enter restart only after prior-frame completion;
-   restart clears run state;
-   moving platform reset;
-   cyan-box reset;
-   second completion;
-   `PRESS ENTER TO RESTART`.

The box-interaction fix must not change restart/respawn semantics.

------------------------------------------------------------------------

## Gameplay Constants

Do not change:

``` text
max horizontal speed = 6
acceleration         = 40
deceleration         = 50
jump speed           = 8
gravity              = 20
coyote time          = 0.10 s
jump buffer          = 0.10 s
```

Do not use movement retuning to hide the physics issue.

------------------------------------------------------------------------

## Asset Pipeline

No asset work.

Do not modify:

-   source assets;
-   cooked assets;
-   Blender workflow;
-   Pillow;
-   cooker;
-   runtime PNG recipe;
-   CMake asset staging.

No new dependency.

------------------------------------------------------------------------

## Manual Validation

M23 is complete when the following are manually validated:

  -----------------------------------------------------------------------
  Test                                Expected
  ----------------------------------- -----------------------------------
  Player meets cyan box on open       Physical interaction remains stable
  ground                              

  Player pushes/runs into box         No unrecoverable CharacterVirtual
                                      lock

  Box approaches static geometry      Player can back away when space
                                      permits

  Box + moving platform interaction   No persistent player wedge in
                                      reproduced problem area

  Platform compresses box near player Stable response; no permanent stuck
                                      state

  Repeated attempts                   Issue does not reproduce under the
                                      known scenario

  Jump near box                       No abnormal boost / inherited
                                      velocity

  Stand near/on valid support         Ground classification remains
                                      correct

  R while interacting with box        M20 respawn still clears player
                                      state

  Fall                                M20 death/respawn unchanged

  Complete level                      M21 unchanged

  Enter restart                       M22 resets box/platform/player
                                      correctly

  After restart                       Box begins normal simulation again

  Moving platform                     carry / jump-carry / reversal
                                      unchanged

  Slopes                              30° walkable / 60° steep unchanged

  Release                             physics fix works without debug UI
  -----------------------------------------------------------------------

------------------------------------------------------------------------

## Known Scenario to Reproduce

Before declaring success, deliberately attempt the user-observed
scenario:

1.  allow the cyan box to fall and move toward the right side;
2.  move the Player into the same area;
3.  time the interaction with the moving platform;
4.  create the constrained box/player/platform/static-geometry
    situation;
5.  attempt to move both left and right;
6.  repeat multiple times.

Record whether the result is:

``` text
normal blocking
temporary compression
recoverable contact
permanent wedge
```

The milestone is specifically intended to eliminate the last category.

------------------------------------------------------------------------

## Explicitly Out of Scope

Do not implement:

-   Milestone 24;
-   longer level;
-   larger main platform as level-design expansion;
-   multiple checkpoints;
-   checkpoint arrays/system;
-   multiple levels;
-   enemies;
-   combat;
-   health;
-   lives;
-   collectibles;
-   score;
-   timer;
-   audio;
-   particles;
-   generic unstuck manager;
-   generic physics event bus;
-   generic contact framework;
-   ECS;
-   scene manager;
-   level manager;
-   new assets;
-   new dependencies;
-   cooker changes.

The user's longer-platform / multiple-checkpoint experiment is
deliberately deferred to a later milestone.

------------------------------------------------------------------------

## Completion Criteria

-   [ ] Observed dynamic-box wedge scenario is deliberately
    investigated.
-   [ ] Root cause is documented from the actual implementation.
-   [ ] Fix uses Jolt/CharacterVirtual correctly rather than blind
    tuning.
-   [ ] Cyan box remains dynamic.
-   [ ] Player ↔ dynamic-box interaction remains physical.
-   [ ] Player is not permanently wedged in the reproduced scenario.
-   [ ] No inappropriate teleport-based workaround is introduced.
-   [ ] Player movement constants remain unchanged.
-   [ ] CharacterVirtual remains behind PhysicsWorld.
-   [ ] No Jolt types leak into public headers.
-   [ ] Moving-platform carry remains correct.
-   [ ] Moving-platform reversal remains correct.
-   [ ] M20 respawn remains correct.
-   [ ] M21 completion remains correct.
-   [ ] M22 restart remains correct.
-   [ ] Cyan box still resets on restart.
-   [ ] Slopes remain correct.
-   [ ] Debug diagnostics are Debug/Development-only.
-   [ ] Release contains no Dear ImGui.
-   [ ] Asset pipeline remains unchanged.
-   [ ] No dependency is added.
-   [ ] No generic physics/gameplay framework is introduced.
-   [x] Milestone 24 is not started. (superseded: M24 Phase A is active)

------------------------------------------------------------------------

## Recommended Phases

### Phase A --- Reproduce, Inspect, Diagnose

**Phase A:** Diagnosis complete and approved. CharacterVirtual already pushes dynamic bodies with `mMaxStrength * dt` in `HandleContact` (no listener required for impulses). No inner body and no `SetListener` in Phase A. Cyan box used Jolt default density 1000 kg/m³ (~1000 kg) vs 100 N max strength. Kinematic platform can carry/compress the box after `CharacterVirtual::Update`. Debug/Development metrics expose box velocity, support body kind, dynamic contact, and CharacterVirtual world velocity.

### Phase B --- Implement Narrow Physics Fix

**Phase B:** CharacterVirtual inner body enabled (0.9-scale translated capsule, layer Moving). Cyan box mass overridden to 30 kg via `CalculateInertia`. Character mass 70 / maxStrength 100 unchanged. No listener, no anti-wedge, no update-order change. Approved.

### Phase C --- Regression + Manual Stress Test

**Phase C:** Complete and manually validated. No listener, tuning, or anti-wedge added. Player is a physical barrier, the 30 kg box is pushable, and the previously observed persistent wedge is no longer reproduced. Milestone 24 is now the active milestone.

------------------------------------------------------------------------

## Git Branch

``` powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/23-dynamic-body-safety
git branch --show-current
```

Expected:

``` text
milestone/23-dynamic-body-safety
```

------------------------------------------------------------------------

## Recommended Model

Use **Grok 4.6 High --- Fast OFF** for Phase A.

This milestone is diagnosis-heavy and depends on understanding the
current CharacterVirtual/Jolt contact semantics rather than simply
adding gameplay code.

Keep Fast OFF for Phase B as well if the approved fix touches
CharacterVirtual contact listeners, body impulses, collision response,
or update ordering.

------------------------------------------------------------------------

## Workflow

``` text
main clean
    ↓
define M23
    ↓
update docs/MILESTONES.md
    ↓
create milestone branch
    ↓
Phase A diagnosis
    ↓
review
    ↓
Phase B narrow fix
    ↓
review
    ↓
Phase C stress/regression tests
    ↓
approve
    ↓
commit
    ↓
push branch
    ↓
merge main
    ↓
push main
    ↓
verify clean main
```

# Milestone 24 --- Extended Traversal + Multiple Checkpoints

## Goal

Expand the current greybox into a longer platforming route and evolve
the M20 single-checkpoint implementation into a small ordered system
with **exactly two checkpoints**.

Target run:

``` text
Initial Spawn
  -> Checkpoint 1
  -> Extended Traversal / Moving Platform
  -> Checkpoint 2
  -> Final Traversal
  -> Goal
  -> LEVEL COMPLETE
  -> Enter
  -> Fresh Run
```

M24 implements the previously deferred experiment: a more extended
traversal area and more than one checkpoint. It is deliberately **not**
a generic level/checkpoint framework.

## Core Requirements

-   Exactly two checkpoints total.
-   Checkpoints activate in order.
-   Checkpoint 2 cannot activate before Checkpoint 1.
-   The furthest checkpoint reached is the current respawn destination.
-   Backtracking through an earlier checkpoint never downgrades
    progress.
-   Manual R and Fall use the latest activated checkpoint.
-   M22 Enter restart clears all checkpoint progress.
-   Both checkpoint respawn volumes must be physically safe.
-   The moving platform must never occupy a checkpoint respawn volume.
-   M23 CharacterVirtual inner-body behavior and 30 kg cyan dynamic box
    are preserved.
-   No new gameplay movement tuning.
-   No assets or dependencies.

## World Expansion

Phase A must inspect the exact M23 geometry before choosing coordinates.
Expand the greybox horizontally enough to make the route meaningfully
longer while remaining readable with the existing M08 camera.

Prefer a small number of additional/extended static greybox platforms.
Reuse the existing moving platform and slopes where useful.

Do not blindly scale all existing coordinates and do not create a large
level.

## Player Reach

Design geometry around the existing movement:

``` text
max speed     = 6
acceleration  = 40
deceleration  = 50
jump speed    = 8
gravity       = 20
coyote        = 0.10 s
jump buffer   = 0.10 s
```

Ideal maximum jump rise is approximately 1.6 world units. Phase A must
check horizontal and vertical gaps before approving geometry.

## Checkpoint Data

Replace the single-checkpoint assumption with a fixed project-owned
collection containing exactly two `CheckpointSpec` values.

Preferred concept:

``` cpp
std::array<CheckpointSpec, 2>
```

No heap allocation, runtime registration, checkpoint manager, trigger
framework, ECS, or level manager.

Each checkpoint has a stable index: 0 and 1.

## Ordered Progression

Conceptual state:

``` text
activeCheckpointIndex = none
respawnPosition = initial spawn
```

Entering Checkpoint 1:

``` text
activeCheckpointIndex = 0
respawnPosition = checkpoint 1 respawn
```

Entering Checkpoint 2 after Checkpoint 1:

``` text
activeCheckpointIndex = 1
respawnPosition = checkpoint 2 respawn
```

Entering Checkpoint 1 again after Checkpoint 2 does nothing.

Checkpoint 2 must not activate while `activeCheckpointIndex` is none.

## RespawnState Migration

Phase A must inspect all consumers of the current:

``` text
checkpointActive
respawnPosition
deathCount
lastRespawnReason
```

Prefer replacing redundant `checkpointActive` with an optional/sentinel
checkpoint index rather than keeping contradictory state.

Keep the representation simple and project-owned.

## Detection

Continue using Player visual-center point vs checkpoint AABB.

No Jolt sensor/body.

No checkpoint meaning in PhysicsWorld.

Checkpoint evaluation occurs only on frames without Fall/Manual respawn,
preserving M20 same-frame rules.

## Safe Respawns

For both checkpoints verify:

-   visual AABB is non-penetrating;
-   CharacterVirtual feet position is valid;
-   M23 inner body synchronizes safely;
-   fixed Z is valid;
-   no static geometry occupies the player volume;
-   no moving platform can sweep through the respawn volume;
-   checkpoint is not inside the goal;
-   checkpoint is not at the cyan-box canonical spawn.

Phase A must explicitly compare both respawn volumes against the
moving-platform swept volume.

## Checkpoint Placement

The existing M20 checkpoint at approximately `{5, 1.8, 0}` may remain
Checkpoint 1 only if it is still useful and safe in the expanded route.

Do not preserve it blindly.

Checkpoint 2 must be later in traversal, safely reachable, and useful
before the final goal section.

## Goal

Preserve one M21 goal and its semantics. Phase A may relocate it if
necessary so it becomes the actual endpoint of the longer route.

Preserve:

-   `LevelGoalSpec`;
-   point/AABB detection;
-   one-way completion per run;
-   `LEVEL COMPLETE`;
-   `PRESS ENTER TO RESTART`;
-   completion surviving R/Fall;
-   Enter restart clearing completion.

No generic goal system.

## Visual States

Render both checkpoint markers with three semantic states:

``` text
Future
Current
PreviouslyActivated
```

Use existing primitives only.

Example visual intent:

-   Future: muted/steel.
-   Current: bright green.
-   Previously activated: subdued green.

Renderer consumes simple project-owned visual state only. It does not
perform overlap/progression logic.

## Manual R

Expected destinations:

``` text
no checkpoint -> initial spawn
checkpoint 1  -> checkpoint 1
checkpoint 2  -> checkpoint 2
```

R does not increment `deathCount`.

## Fall

Fall increments `deathCount` exactly once and respawns at the latest
checkpoint, or initial spawn if none is active.

## Restart

M22 Enter restart resets:

``` text
active checkpoint = none
respawn position  = initial spawn
deathCount        = 0
last reason       = None
completed         = false
```

It must continue resetting Player/CharacterVirtual/inner body, moving
platform, cyan box and camera.

After restart both checkpoint markers are Future again.

## M23 Preservation

Preserve:

``` text
CharacterVirtual:
  inner body enabled
  mass = 70 kg
  maxStrength = 100 N

cyan box:
  Dynamic
  size = 1x1x1
  mass = 30 kg
  spawn = {0,5,0}
```

The cyan box remains a technical physics object, not a required
traversal puzzle.

Do not introduce a new compression/wedge trap through level geometry.

## Moving Platform

Preserve M13 physics semantics: kinematic `MoveKinematic`, carry,
airborne carry, reversal and restart reset.

Prefer keeping size `{4,0.4,3}`, speed `2.5`, and current path. If Phase
A proposes relocation, it must justify coordinates without changing
physics semantics.

## Slopes

Preserve max slope 50°, 30° walkable and 60° steep.

## Camera

Preserve M08 dead zones 1.5/0.75 and smoothing sharpness 8.

The longer level is a camera stress test, not a camera-redesign
milestone.

## Debug Metrics

Debug/Development should show at minimum:

``` text
active checkpoint index / none
respawn position
death count
last respawn reason

checkpoint 1:
  inside
  visual/progression state

checkpoint 2:
  inside
  visual/progression state
```

Keep M23 physics diagnostics. Release remains free of Dear ImGui.

## Architecture

Preserve ownership:

``` text
Application  -> run/checkpoint/goal meaning
Player       -> movement policy
PhysicsWorld -> physical simulation
Renderer     -> visual consumption only
world/*      -> project world specifications
```

No Jolt checkpoint sensor, Renderer mutation, or generic trigger/level
framework.

## Asset Pipeline

No changes to cooker, Pillow, Blender workflow, source/cooked assets,
runtime PNG recipe, or CMake staging. No new dependency.

## Phase A --- Inspect + Design

Phase A must:

1.  inspect the exact M23 world;
2.  inventory all static geometry;
3.  map moving-platform swept volume;
4.  map cyan-box spawn/interaction region;
5.  map current checkpoint and goal;
6.  calculate Player reach;
7.  propose the longer greybox;
8.  propose Checkpoint 1 and Checkpoint 2;
9.  verify both safe respawn volumes;
10. define minimal `RespawnState` migration;
11. define checkpoint visual states;
12. define final update ordering;
13. create only minimal scaffolding if useful;
14. build Debug/Development/Release;
15. stop before full multi-checkpoint wiring.

**Phase A:** Complete. Design and scaffolding recorded and approved. The live M23 single checkpoint at `{5, 1.8, 0}` was rejected because its respawn player AABB overlaps the moving-platform swept volume.

### Phase A design record

Movement envelope (unchanged): jump rise 1.6, apex 0.4 s, same-height airtime 0.8 s, max-speed air distance 4.8.

Live M23 checkpoint `{5, 1.8, 0}` is **not** reused: its respawn player AABB overlaps the moving-platform swept volume (X [-8, 8], Y [1.1, 1.5]).

Proposed route (Phase B):

``` text
Spawn {0, 0.8, 0}
  -> early right / optional 30-degree slope
  -> Checkpoint 1 {16.5, 1.8, 0} on new platform top Y = 1.0
  -> back to moving platform (unchanged path)
  -> left mid landing (existing)
  -> new mid-left step
  -> Checkpoint 2 {-15.5, 2.8, 0} on new platform top Y = 2.0
  -> Goal {-21, 3.8, 0} on new platform top Y = 3.0
```

Ground resized to `{48, 0.5, 8}` at `{0, -0.25, 0}`. Steep 60-degree slope relocated to `{22, 0.966, 0}` (classification dead-end). Walkable 30-degree slope stays. Kill plane stays Y = -8.

`RespawnState` Phase B: `int activeCheckpointIndex = -1` replaces `checkpointActive`. Activation: `expectedIndex = active + 1` against `kCheckpoints[expectedIndex]` on the existing no-respawn branch.

Renderer Phase B: two `CheckpointVisualState` values (Future / Current / PreviouslyActivated). Do not pass `RespawnState` wholesale.

See `docs/ARCHITECTURE.md` and `world/RespawnWorld.h`.

## Phase B --- Implement

After Phase A approval:

-   implement approved greybox expansion;
-   implement exactly two ordered checkpoints;
-   update respawn progression;
-   render both marker states;
-   update diagnostics;
-   preserve M20--M23;
-   build all configs;
-   stop before final approval.

**Phase B:** Implementation recorded. Live world used the approved M24 greybox. `kCheckpoints` has exactly two ordered specs. `RespawnState::activeCheckpointIndex` replaced `checkpointActive`. Renderer consumes two `CheckpointVisualState` values.

Manual play found a **traversal geometry** defect, not a checkpoint-order defect: CP2 correctly refuses to activate before CP1; CP1 on the right activates. After CP1 the 30-degree ramp blocked return to the center.

## Phase B.1 --- Traversal Geometry Correction

The M14 30-degree box at `{10.90, 1.6732, 0}` is rotated **+30 degrees about Z**. Approximate world ends:

``` text
low  (left)  top:    X ~ 8.20,  Y ~ 0.35
high (right) top:    X ~ 13.40, Y ~ 3.35
```

The high end sat immediately left of CP1 (platform X [14.5, 18.5], top Y = 1.0). Players could walk up and drop onto CP1. Return failed: max jump rise 1.6 cannot clear a ~2.35 m face, and the underside wedges anyone walking back on the ground. The 60-degree slope at X=22 did not cause this trap.

Correction (one intent: clear the center <-> CP1 corridor):

- 30-degree test moved to `{21.70, 1.6732, 0}` (optional dead-end past CP1).
- 60-degree test moved to `{25.60, 0.966, 0}` so it does not overlap the 30-degree box.
- Ground expanded to size `{56, 0.5, 8}` (X [-28, 28]) so both tests rest on support.

CP1/CP2/goal specs and ordered activation are unchanged.

**Phase B.1:** Complete. The user manually verified spawn -> CP1 activation and CP1 -> central/moving-platform return without R, death, clipping, or frame-perfect play. Center -> CP1 remains comfortable.

## Phase C --- Manual Validation (current)

Required manual cases:

A. Fresh run: R -\> initial spawn.

B. Activate Checkpoint 1: R -\> CP1.

C. Attempt CP2 before CP1 if physically possible: CP2 must not activate.

D. Activate CP2 after CP1: R -\> CP2.

E. Backtrack through CP1: active remains CP2.

F. Fall after CP1: death +1, respawn CP1.

G. Fall after CP2: death +1, respawn CP2.

H. Reach goal: completion UI appears.

I. R after completion: latest checkpoint, completion remains true.

J. Fall after completion: death +1, latest checkpoint, completion
remains true.

K. Enter restart: initial spawn, checkpoint progress cleared, deaths 0,
goal incomplete, platform/box reset, camera snap.

L. Second run: both checkpoints activate again in order.

M. M23 cyan box: pushable, Player solid, no persistent wedge.

N. Moving platform: standing carry, airborne carry, reversal.

O. Slopes: 30° walkable, 60° steep.

P. Camera: longer traversal remains readable; respawn/restart snap
correct.

Q. Release: full gameplay works without ImGui.

**Phase C:** Complete and manually validated. Longer greybox, two ordered checkpoints, spawn → CP1 → center return → moving platform → left landing → mid-left step → CP2 → goal, plus R/Fall/Enter and M23 regressions, are accepted. Milestone 25 is now the active milestone.

## Completion Criteria

-   [x] Traversal is meaningfully longer.
-   [x] Exactly two checkpoints exist.
-   [x] CP2 requires CP1.
-   [x] Latest checkpoint controls respawn.
-   [x] Backtracking cannot downgrade progression.
-   [x] Both respawn volumes are safe.
-   [x] Moving platform cannot occupy either respawn volume.
-   [x] R/Fall semantics are correct at none/CP1/CP2.
-   [x] Goal remains reachable.
-   [x] Completion survives R/Fall.
-   [x] Enter restart clears checkpoint progression.
-   [x] Second run works.
-   [x] Markers communicate Future/Current/Previous.
-   [x] Renderer owns no checkpoint logic.
-   [x] PhysicsWorld owns no checkpoint meaning.
-   [x] No Jolt checkpoint sensor/body.
-   [x] M23 inner body and 30 kg box remain correct.
-   [x] No persistent wedge regression.
-   [x] Moving platform behavior remains correct.
-   [x] Slopes remain correct.
-   [x] Camera behavior remains M08.
-   [x] Gameplay constants remain unchanged.
-   [x] Debug metrics support two checkpoints.
-   [x] Release has no ImGui.
-   [x] Asset pipeline remains unchanged.
-   [x] No dependency added.
-   [x] No generic level/checkpoint framework.
-   [x] Milestone 25 is not started. (superseded: M25 Phase A is active)

## Explicitly Out of Scope

Do not add Milestone 25, more than two checkpoints, multiple levels,
save/load, serialization, JSON level data, LevelManager, SceneManager,
generic trigger manager, checkpoint registry, ECS/event bus, enemies,
combat, health, lives, collectibles, score, timer, audio, particles, new
assets, dependencies, or cooker changes.

## Git Branch

``` powershell
git checkout main
git pull origin main
git status

git checkout -b milestone/24-extended-traversal-checkpoints
git branch --show-current
```

## Recommended Model

Use **Grok 4.6 High --- Fast OFF** for Phase A and preferably Phase B.
Geometry reachability, safe CharacterVirtual/inner-body respawns,
moving-platform swept volume, checkpoint-state migration, and existing
M20--M23 ordering interact here.

## Workflow

``` text
main clean
 -> M24 branch
 -> Phase A inspect/design
 -> review
 -> Phase B implementation
 -> review
 -> Phase C manual traversal/regression
 -> approve
 -> commit
 -> push branch
 -> merge main
 -> push main
 -> verify clean main
```

Do not commit, push, or merge before manual Phase C approval.


# Milestone 25 — Static Hazards + Hazard Respawn

## Status

**Complete (manually approved).** Milestone 26 is now the active milestone.

## Branch

`milestone/25-static-hazards`

## Recommended Cursor model

**Grok 4.6 High — Fast OFF**

Use the non-Fast mode for Phase A because this milestone touches the established respawn priority contract, checkpoint progression, CharacterVirtual positioning, world geometry, and Release-visible gameplay markers.

---

## 1. Goal

Add the first explicit non-fall gameplay hazard to the platformer while preserving all M20–M24 respawn/checkpoint/restart semantics.

M25 introduces a **small fixed set of static hazard volumes** placed in the existing M24 greybox route. Touching a hazard causes a respawn at the latest valid checkpoint and increments the death counter exactly like falling below the kill plane.

Conceptual loop:

```text
Traversal
  -> touch static hazard
  -> deathCount +1
  -> respawn at latest checkpoint
  -> continue run
```

This milestone is intentionally **not** a health/combat system and **not** a generic trigger framework.

---

## 2. Why M25

M24 established a longer route with two ordered checkpoints. The next useful gameplay pressure is to make those checkpoints matter for more than falling off the world.

Static hazards exercise the existing systems without introducing enemies, health, combat, animation, audio, or arbitrary level scripting.

M25 should prove that the current run-state architecture can support another respawn cause cleanly.

---

## 3. Scope

M25 adds:

- a project-owned `HazardSpec` or equally small fixed data type;
- a small fixed-size collection of static hazard volumes;
- primitive hazard rendering;
- point/AABB hazard detection using the existing Player visual-center convention unless Phase A proves a better already-established project convention is required;
- a new `RespawnReason::Hazard`;
- hazard death semantics integrated into the existing single-respawn-per-frame contract;
- Debug/Development hazard metrics;
- manual regression validation across M20–M24 behavior.

M25 does **not** add:

- health;
- damage points;
- invulnerability frames;
- knockback;
- enemies;
- combat;
- lives;
- game over;
- moving hazards;
- Jolt sensor bodies;
- a generic trigger/event framework;
- arbitrary runtime hazard registration;
- new assets;
- new dependencies.

---

## 4. Baseline that must remain intact

### Player movement

- max horizontal speed: `6`
- acceleration: `40`
- deceleration: `50`
- jump speed: `8`
- gravity: `20`
- coyote time: `0.10`
- jump buffer: `0.10`

### CharacterVirtual / M23

- Character mass: `70 kg`
- maxStrength: `100 N`
- inner body enabled
- inner body remains private to `PhysicsWorld.cpp`
- cyan box remains Dynamic, `30 kg`, spawn `{0,5,0}`
- Player remains a physical barrier to the cyan box
- Player can push the cyan box
- no persistent wedge regression

### Moving platform / M13

- size `{4,0.4,3}`
- Y `1.3`
- path X `[-6,+6]`
- speed `2.5`
- initial direction `+X`
- carry/reversal semantics unchanged
- ordinary respawn does not reset it
- Enter restart resets it

### Slopes

- max slope `50°`
- 30° slope remains walkable and optional
- 60° slope remains steep and optional

### M24 world

- ground center `{0,-0.25,0}`, size `{56,0.5,8}`
- CP1 platform center `{16.5,0.75,0}`
- CP2 platform center `{-15.5,1.75,0}`
- goal platform center `{-21,2.75,0}`
- 30° slope center `{21.70,1.6732,0}`
- 60° slope center `{25.60,0.966,0}`
- exactly two ordered checkpoints
- CP1 before CP2
- no downgrade after CP2
- goal center `{-21,3.8,0}`
- kill plane `Y=-8`

---

## 5. Hazard representation

Prefer a minimal project-owned world type such as:

```cpp
struct HazardSpec
{
    core::Vec3 center;
    core::Vec3 size;
};
```

Use a fixed-size collection, preferably `std::array<HazardSpec, N>` with a deliberately small `N` selected in Phase A.

The target is **2–3 hazards**, not an arbitrary system.

Do not use:

- `std::vector` for runtime registration;
- heap allocation;
- Jolt IDs as gameplay identity;
- polymorphic hazard classes;
- a `HazardManager`;
- a `TriggerManager`.

---

## 6. Hazard placement design

Phase A must inspect the final M24 route before choosing coordinates.

Hazards should:

- make checkpoints useful;
- be visually readable;
- be avoidable with normal movement;
- not require frame-perfect jumps;
- not overlap checkpoint respawn volumes;
- not overlap the goal trigger;
- not occupy the moving-platform swept volume in a way that creates unavoidable deaths;
- not create a new Player/cyan-box/moving-platform compression trap;
- not make the cyan box a required puzzle object;
- not turn the optional slope tests into mandatory traversal.

Preferred distribution:

- one hazard after CP1 but before CP2;
- one hazard after CP2 or in the final approach to the goal;
- optionally one early hazard only if the route remains readable and fair.

Exact coordinates are recorded in the Phase A design section below. Do not treat this definition's earlier "not approved" note as current; Phase A chose the live volumes in `world/HazardWorld.h`.

---

## 7. Detection

Prefer the same simple project-owned overlap style already used by checkpoint/goal logic.

Phase A must inspect whether the existing code uses Player visual-center point/AABB checks and determine the smallest consistent implementation.

No Jolt sensor bodies.

No collision-response hazard body is required.

The hazard visual may be a primitive while the gameplay volume remains project-owned world data.

---

## 8. RespawnReason

Extend the existing reason enum with:

```text
Hazard
```

Final conceptual reasons:

```text
None
Fall
Manual
Hazard
```

Do not add Restart as a respawn reason unless the existing architecture already deliberately models it that way. M22 restart remains a distinct fresh-run action.

---

## 9. Death-count semantics

Hazard contact is a death event.

Therefore:

```text
Hazard -> deathCount +1
Fall   -> deathCount +1
Manual R -> deathCount unchanged
```

A hazard death must respawn at:

- initial spawn if no checkpoint is active;
- CP1 if CP1 is current;
- CP2 if CP2 is current.

Checkpoint progress is preserved.

Level completion is preserved if a hazard is touched after completion, matching the existing Fall behavior.

---

## 10. Same-frame priority

M25 must preserve the one-reset-per-frame rule.

Phase A must inspect and explicitly propose the final priority among:

- Fall
- Manual R
- Hazard
- checkpoint activation
- goal completion
- Enter restart

The existing established rule already gives Fall priority over Manual and keeps checkpoint/goal evaluation out of respawn frames.

M25 must not guess a hazard priority casually. The preferred policy is that **automatic lethal causes are resolved before Manual R**, but Phase A must verify the current control flow and propose one deterministic order.

Whatever is chosen must guarantee:

- at most one teleport/reset per frame;
- deathCount increments at most once per frame;
- a hazard respawn frame cannot activate a checkpoint;
- a hazard respawn frame cannot newly complete the goal;
- Enter restart still obeys M22's `restartAvailableAtFrameStart` contract.

---

## 11. Hazard contact after completion

If the level is already complete and the Player touches a hazard:

- `deathCount += 1`;
- respawn at latest checkpoint;
- `completed` remains `true`;
- `LEVEL COMPLETE` remains visible;
- `PRESS ENTER TO RESTART` remains visible.

Only Enter performs the fresh-run reset.

---

## 12. Enter restart

M22 restart must reset hazard-related transient state automatically.

Hazards themselves are static world specs, so they do not need physical reset.

After Enter:

- Player returns to initial spawn;
- checkpoint progression clears;
- deathCount returns to `0`;
- last reason returns to `None`;
- completion clears;
- moving platform resets;
- cyan box resets;
- camera snaps;
- hazard world remains in the same canonical positions.

---

## 13. Rendering

Hazards are visible gameplay geometry in all configurations.

Use raylib primitives through the existing Renderer boundary.

Preferred visual language:

- clearly dangerous shape/color;
- simple repeated teeth/spikes/bars or low boxes;
- no textures/models required;
- no animation required.

Do not add new authored assets.

Renderer only consumes hazard specs and draws them. It does not decide whether the Player dies.

---

## 14. Physics boundary

Hazard gameplay meaning belongs outside `PhysicsWorld`.

Do not add Jolt hazard sensors.

If hazard visuals are intentionally non-solid, they do not need Jolt bodies.

If Phase A finds a compelling reason for solid supporting geometry around a hazard, that supporting geometry must remain ordinary canonical static world geometry; the lethal volume still stays project-owned gameplay data.

---

## 15. Debug metrics

Debug/Development should show a small hazard section, for example:

- `Inside hazard: None / 1 / 2 / ...`
- `Hazard contact this frame`
- existing `Last respawn reason` must display `Hazard`

Preserve M23/M24 metrics.

Release remains free of ImGui.

Do not expose Jolt BodyIDs.

---

## 16. Architecture

Preserve:

```text
world/*
    fixed hazard specifications

Application
    hazard detection and run-state meaning

Player
    movement policy

PhysicsWorld
    physical simulation only

Renderer
    hazard visualization only
```

Do not introduce a generic hazard subsystem unless a concrete requirement appears later.

---

## 17. Phase plan

### Phase A — Hazard placement + respawn-priority design

Complete. Specs live in `world/HazardWorld.h`. Coordinates and Fall > Hazard > Manual policy approved.

Inspected live M24 geometry, Player envelope, and Application reset order. Scaffolding is in `world/HazardWorld.h` and `RespawnReason::Hazard`. Detection/rendering/death are **not** wired.

#### Chosen count: exactly 2

Hazard 1 makes CP1 useful (death on the CP1 ↔ center corridor). Hazard 2 makes CP2 useful (missed CP2 → goal jump). A third early hazard would crowd spawn, the moving-platform sweep, and the cyan-box region without adding checkpoint value.

#### Hazard 1 — corridor spikes (index 0)

- center `{11.5, 0.5, 0}`, size `{1.4, 1.0, 2.0}`
- AABB X [10.8, 12.2], Y [0, 1.0], Z [-1, 1]
- On ground between CP1 platform left (14.5) and moving-platform swept max X (8)
- Jump over on the way to CP1 and on the return. Width 1.4 at speed 6 ≈ 0.23 s; airborne center Y ≈ 2.12 vs lethal top 1.0 (margin ≈ 1.1). Rise 1.6 / air 4.8 m.

#### Hazard 2 — goal-gap spikes (index 1)

- center `{-18.5, 0.5, 0}`, size `{1.2, 1.0, 2.0}`
- AABB X [-19.1, -17.9], Y [0, 1.0], Z [-1, 1]
- In the 2.0 m ground gap between CP2 left (−17.5) and goal-platform right (−19.5)
- Successful CP2 → goal jump stays airborne above the bar. Landing in the gap is a Hazard death and returns to CP2.

#### Proposed Phase B priority

```text
Fall > Hazard > Manual R > checkpoint / goal > Enter (M22 restartAvailableAtFrameStart)
```

Fall stays first because that is the live first branch and ground hazards cannot also be below Y = −8. Multiple overlapping hazard volumes still produce one Hazard death. Hazard frames skip checkpoint/goal (already in the no-respawn else). Hazard wins over Enter that frame.

Compile-time invariants: respawn visual AABBs, checkpoint triggers, goal trigger, and moving-platform swept AABB do not overlap either hazard.

Build all configurations and stop for review. Do not mark M25 complete. Do not start Milestone 26.

### Phase B — Implementation

Complete. Live detection, Fall > Hazard > Manual priority, primitive visuals, and Debug/Development hazard metrics.

### Phase C — Manual validation

**Phase C:** Complete and manually approved. Hazard death +1, latest-checkpoint respawn (spawn / CP1 / CP2), and Enter restart deathCount 0 are accepted. Milestone 26 is now the active milestone.

---

## 18. Manual acceptance targets

M25 is complete only after manual validation confirms:

1. Fresh run can avoid hazards using normal movement.
2. Hazard before any checkpoint respawns at initial spawn and adds exactly one death.
3. Hazard after CP1 respawns at CP1 and adds exactly one death.
4. Hazard after CP2 respawns at CP2 and adds exactly one death.
5. Hazard contact does not downgrade checkpoint progress.
6. Hazard respawn does not reset the moving platform.
7. Hazard respawn does not reset the cyan box.
8. Manual R still does not add a death.
9. Fall still adds exactly one death.
10. Hazard after completion preserves completion UI/state.
11. Enter restart clears run state and death count but leaves static hazards in their canonical positions.
12. Second run behaves identically.
13. Cyan box remains pushable/solid without persistent wedge.
14. Moving-platform carry/reversal remains correct.
15. 30°/60° slope behavior remains correct.
16. Camera remains usable across the M24 world.
17. Release contains gameplay hazard visuals but no ImGui.

---

## 19. Explicitly out of scope

Do not implement in M25:

- Milestone 26;
- health points;
- damage amounts;
- invulnerability;
- knockback;
- lives/game over;
- enemies;
- combat;
- projectiles;
- moving hazards;
- animated hazards;
- hazard audio;
- particles;
- collectibles;
- score;
- timer;
- save/load;
- serialization;
- generic trigger system;
- generic hazard manager;
- LevelManager;
- SceneManager;
- ECS/event bus;
- JSON level format;
- new assets;
- new dependencies;
- cooker changes;
- camera redesign;
- Player movement tuning;
- CharacterVirtual tuning.

---

## 20. Completion definition

M25 is complete when:

- a small fixed set of static hazards exists in the M24 route;
- hazards are visible and avoidable;
- touching one produces exactly one hazard death/respawn;
- latest-checkpoint semantics are correct;
- `RespawnReason::Hazard` is visible in Debug/Development;
- same-frame priority is deterministic;
- checkpoint/goal/restart contracts remain intact;
- M23 dynamic-body behavior remains intact;
- all three builds pass;
- Release remains free of ImGui;
- user manually validates the gameplay;
- Git milestone workflow is completed;
- Milestone 26 has not started prematurely.


# Milestone 26 — Collectibles + Run Counter

## Status

**Complete (manually approved).** Do not reopen M26. Milestone 27 is the active milestone.

## Goal
Add the first non-lethal collectible gameplay loop to the existing platformer level while preserving all M00–M25 behavior.

The level will contain exactly **3 fixed collectibles**. Collecting one removes it for the current run and increments an Application-owned run counter. Collectibles are restored only by the existing full **Enter restart** after level completion; ordinary Manual/Fall/Hazard respawns do not restore them.

This milestone intentionally remains a small hardcoded gameplay feature. It does **not** introduce score, inventory, save/load, generic item systems, ECS, level serialization, audio, particles, or new assets.

## Player-facing loop

```text
Traverse level
    ↓
Touch collectible
    ↓
Collectible disappears
    ↓
Collected: N / 3
    ↓
Respawn from R / Fall / Hazard
    ↓
Already collected items remain collected
    ↓
Complete level
    ↓
Enter restart
    ↓
Fresh run: 0 / 3 and all collectibles restored
```

## Architectural intent

- `world` owns immutable fixed collectible specifications.
- `Application` owns per-run collected state and detection semantics.
- `Renderer` draws only collectibles that are still available.
- `Player` remains movement-only.
- `PhysicsWorld` remains physics-only.
- Collectibles use project-owned point-vs-AABB gameplay detection, not Jolt sensors.
- Exactly 3 collectibles; use a fixed-size representation such as `std::array` / bitset-like value state.
- No `CollectibleManager`, `ItemManager`, generic trigger abstraction, event bus, or ECS.

## Phase A — Inventory, placement design, and scaffolding

Phase A must inspect the final M25 world before choosing live coordinates.

Deliverables:

1. Confirm final M25 geometry, checkpoints, hazards, goal, moving-platform sweep, slopes, Player dimensions, camera behavior, and update order.
2. Propose a minimal `CollectibleSpec` and exactly 3 compile-time collectible specs.
3. Choose exact positions only after proving they are reachable, readable, and do not overlap respawn/checkpoint/goal/hazard volumes.
4. At least one collectible should reward the right-side/CP1 portion of the route.
5. At least one collectible should reward the moving-platform/central traversal.
6. At least one collectible should reward the left-side/CP2-to-goal portion of the route.
7. Collection must be optional: the goal remains completable even if the player has collected 0/3.
8. Design Application-owned run state, preferably fixed-size booleans/bitset plus a derived or maintained count.
9. Define same-frame semantics for collectible + Fall/Hazard/Manual/checkpoint/goal/Enter.
10. No live collection behavior in Phase A unless tiny scaffolding is required for compilation.

### Phase A design record

Exactly three static hop collectibles (`kCollectibleSize = {1.0, 1.2, 1.0}`). Hover = support top + 1.5 so standing center stays ~0.1 below the AABB.

- Collectible 1: `{5.0, 2.5, 0}` on the right platform (optional vs ground → CP1).
- Collectible 2: `{-4.5, 4.0, 0}` on the left landing (reached via moving platform; hop optional).
- Collectible 3: `{-10.0, 3.75, 0}` on the middle-left step (intended route; hop optional).

`CollectibleRunState.collected` is the source of truth; `CollectedCount` is derived. Proposed Phase B: collect in the no-respawn branch after checkpoint/goal, skip collection when `restartAvailableAtFrameStart && Enter`, then existing RestartRun. Fall/Hazard/Manual still win over collection. One uncollected match per frame.

Scaffolding: `world/CollectibleWorld.h`, `gameplay/CollectibleRunState.h`. Do not mark M26 complete. Do not start Milestone 27.

## Approved semantic direction to validate in Phase A

Collection should be evaluated only on a frame in which no respawn occurred. Therefore:

```text
Fall / Hazard / Manual respawn
    > collectible collection
```

Within a normal non-respawn frame, collectible collection should occur before or alongside checkpoint/goal evaluation with deterministic semantics. Enter restart must clear all collected state and restore all collectibles.

A collectible can be collected at most once per run. Multiple collectibles touched in one frame may each be collected if the geometry genuinely permits it, but the final design should spatially separate them so this is not a normal gameplay case.

## Phase B — Implementation (current)

Live:

- `FindAvailableCollectibleIndexContaining(Player::Position(), collectibleRunState)`;
- collection in the no-respawn branch after checkpoint/goal;
- skip collection when `restartAvailableAtFrameStart && Enter`;
- `RestartRun` clears `collectibleRunState`;
- Renderer gold 0.45 cubes for available items;
- `COLLECTED N / 3` upper-right in all configurations;
- Debug/Development Collectibles metrics.

Phase B implementation is complete. Phase C was manually approved.

## Phase C — Manual validation

Final validation must cover at least:

- Collect each of the 3 individually.
- Counter progresses 0/3 → 1/3 → 2/3 → 3/3.
- Collected item disappears and cannot increment twice.
- R after collection preserves it.
- Fall after collection preserves it.
- Hazard death after collection preserves it.
- CP1/CP2 progression remains correct.
- Goal can complete with fewer than 3 collectibles.
- Goal can complete with 3/3.
- Completion UI and collectible counter coexist correctly.
- Enter after completion restores all 3 and resets counter to 0/3.
- Second run can collect all 3 again.
- Moving platform, cyan dynamic box, slopes, hazards, camera, and Release behavior regressions pass.

## Completion criteria

M26 is complete only when:

- Exactly 3 fixed collectibles exist.
- All 3 are reachable through normal validated movement.
- Collection is non-lethal and one-time per run.
- Counter is correct and Release-visible.
- Ordinary respawns preserve collection progress.
- Enter fresh-run restart resets collection progress and restores visuals.
- Goal completion does not require collectibles.
- No new dependency or asset is introduced.
- No generic item/trigger/level system is introduced.
- Debug, Development, and Release build successfully.
- User manually approves Phase C.

## Explicitly out of scope

- Milestone 27
- score/points
- lives/game over
- health/damage changes
- inventory
- currency/economy
- collectible effects/power-ups
- required collectible gate for goal
- save/load/persistence
- achievements
- audio
- particles
- animation system
- new textures/models
- moving collectibles
- procedural placement
- generic collectible/item manager
- generic trigger framework
- ECS/event bus
- JSON level data
- camera redesign
- Player tuning
- physics redesign
- asset cooker changes
- new dependencies

## Git branch

`milestone/26-collectibles`

## Recommended Cursor model

Grok 4.6 High — Fast OFF for Phase A architecture/placement analysis.


# Milestone 27 — Run Timer + Completion Time

### Status
**Complete (manually approved).** Do not reopen M27. Milestone 28 is the active milestone.

### Branch
`milestone/27-run-timer`

## Recommended Cursor model
Grok 4.6 High — Fast OFF

### Goal
Add a minimal run timer that measures the current run from fresh start until the first level completion, displays it in gameplay UI, preserves the completed time after `LEVEL COMPLETE`, and resets only on a fresh run via Enter.

This milestone introduces time measurement only. It does **not** introduce best-time persistence, leaderboards, score, ranking, medals, save/load, ghost data, or online services.

### Player-facing contract

Fresh run:

```text
TIME 00:00.000
```

During gameplay:

```text
TIME 00:12.347
```

First goal completion freezes the run time:

```text
TIME 00:38.912
LEVEL COMPLETE
PRESS ENTER TO RESTART
```

After completion:
- the displayed completion time remains frozen;
- movement continues as in M21;
- collectibles can still be collected as in M26;
- R / Fall / Hazard do not resume or reset the timer;
- Enter starts a fresh run and resets time to zero.

### Scope

#### Add
- minimal Application-owned run-timer state;
- monotonic frame-delta accumulation using the existing game loop timing;
- timer starts at fresh-run start;
- timer freezes on the first transition `completed: false -> true`;
- Release-visible `TIME MM:SS.mmm` HUD;
- Debug/Development run-timer metrics;
- fresh-run reset integration;
- manual regression validation.

#### Preserve
- M20 respawn semantics;
- M21 completion semantics;
- M22 fresh-run restart;
- M23 dynamic body safety;
- M24 ordered checkpoints;
- M25 hazards;
- M26 collectibles and `COLLECTED N / 3`;
- Player movement constants;
- camera;
- moving platform;
- cyan physics box;
- slopes;
- asset pipeline.

### Explicitly out of scope
- best time;
- persistent records;
- save/load;
- file I/O for timer;
- leaderboards;
- rankings;
- medals;
- score;
- currency;
- combo systems;
- split times;
- pause menu;
- countdown;
- time limit;
- time bonuses/penalties;
- collectible time bonuses;
- hazard time penalties;
- online services;
- ghost/replay;
- achievements;
- audio/particles;
- generic RunManager / TimerManager;
- ECS/event bus;
- new assets/dependencies;
- Milestone 28.

### Ownership

#### Application
Owns run timing state and decides when time advances/freezes/resets.

Suggested minimal state:

```cpp
struct RunTimerState
{
    double elapsedSeconds = 0.0;
    bool frozen = false;
};
```

A separate stored completion-time field is unnecessary if `elapsedSeconds` simply stops changing once completion occurs.

#### Renderer
Receives a small read-only elapsed time value and renders `TIME MM:SS.mmm`.

Renderer must not own or advance time.

#### PhysicsWorld
No timer knowledge.

#### Player
No timer knowledge.

#### world
No timer knowledge.

### Timing source
Use the existing per-frame delta time already used by the game loop / Application.

Do not add:
- wall-clock calendar time;
- system clock timestamps;
- platform-specific timing APIs;
- a second independent timing subsystem.

The timer should accumulate gameplay frame delta:

```text
elapsedSeconds += dt
```

while the run is active and not completed.

### Start semantics
A fresh run begins with:

```text
elapsedSeconds = 0
frozen = false
```

The timer advances from the first normal gameplay update of that run.

No pre-start countdown.

### Completion semantics
The timer freezes on the first frame that changes:

```text
completed == false
```

to:

```text
completed == true
```

After that:
- timer value remains unchanged;
- later goal overlap does nothing;
- R / Fall / Hazard do not alter time;
- collecting remaining collectibles does not alter time;
- only Enter fresh restart clears it.

### Same-frame goal rule
The frame that first completes the goal must produce one deterministic final time.

Preferred implementation:
- advance active timer once for the frame using the same frame-delta policy used throughout the run;
- process gameplay;
- if goal transitions to complete this frame, freeze after that frame's accumulated delta.

Do not create sub-frame interpolation.

### Respawn semantics
Manual R, Fall, and Hazard preserve accumulated time. If the level is not complete, timing continues after respawn. No time penalty is added.

### Completion + respawn
After `LEVEL COMPLETE`:
- timer is frozen;
- R/Fall/Hazard preserve frozen time;
- completion remains true;
- collectible progress remains preserved;
- completion UI remains visible.

### Enter restart
Existing fresh-run Enter restart must additionally:

```text
elapsedSeconds = 0
frozen = false
```

It must preserve all existing restart behavior:
- Player spawn;
- checkpoint reset;
- deathCount reset;
- respawn reason reset;
- completion reset;
- collectible reset to 0/3;
- moving platform reset;
- cyan box reset;
- CharacterVirtual reset;
- camera snap.

### HUD
Add Release-visible text:

```text
TIME MM:SS.mmm
```

Recommended placement:
- upper-left;
- `COLLECTED N / 3` remains upper-right;
- completion UI remains centered.

### Formatting
Use fixed-width time formatting:

```text
TIME 00:00.000
TIME 01:05.432
TIME 12:34.567
```

Minimum:
- 2-digit minutes;
- 2-digit seconds;
- 3-digit milliseconds.

If elapsed time exceeds 99 minutes, do not wrap. Allow additional minute digits.

No hours field is required.

### Numerical behavior
Store time in `double`.

Do not quantize internal state to milliseconds. Format milliseconds only for display.

### Debug metrics
Debug/Development should expose at least:
- Run time seconds;
- Timer state: Running / Frozen;
- Formatted run time;
- completion state;
- existing collectible/checkpoint/hazard/death metrics remain.

Metrics are read-only.

### Release
Release must contain:
- `TIME MM:SS.mmm`;
- collectible HUD;
- goal completion UI;
- all gameplay.

Release must not contain Dear ImGui/debug metrics.

### Architecture rules
- No `TimerManager`.
- No `RunManager`.
- No singleton.
- No event bus.
- No platform-specific clock API.
- No timer ownership in Renderer.
- No timer ownership in PhysicsWorld.
- No save file.
- No persistent best time.

### Phase A
Design/scaffolding only:
- inspect current dt/update/completion/restart flow;
- define exact timer state and ownership;
- define precise update/freeze/reset order;
- define formatting helper boundary;
- define HUD coexistence;
- build all configs;
- no live timer required yet.

Scaffolding added (compile-only; M26 gameplay unchanged):
- `gameplay/RunTimerState.h` (`elapsedSeconds`, `frozen`); Application member initialized to zero;
- `core/RunTimeFormat.h` (floor-to-millisecond display helper + integer `static_assert` cases);
- no `elapsedSeconds += dt` yet;
- no TIME HUD yet;
- `RestartRun` does not reset the timer yet (still always zero);
- DebugMetrics does not show timer fields yet.

### Phase B
Implementation complete / awaiting Phase C:
- live accumulation after `restartAvailableAtFrameStart` using `deltaSeconds`;
- first-completion freeze inside the existing goal transition;
- ordinary respawn preserves time (`PerformRespawn` unchanged);
- `RestartRun` resets `runTimerState`;
- Release-visible `TIME MM:SS.mmm` upper-left;
- Debug/Development Run timer metrics;
- docs/build/smoke.

Do not mark complete.

### Phase C manual validation
At minimum:
1. Fresh run starts near `TIME 00:00.000`.
2. Time increases while playing.
3. Manual R does not reset time.
4. Fall does not reset time.
5. Hazard does not reset time.
6. Checkpoints do not reset time.
7. Collectibles do not reset or otherwise modify time.
8. Goal works with less than 3/3 collectibles.
9. First goal completion freezes timer.
10. Walking after completion leaves time frozen.
11. Collecting an item after completion leaves time frozen.
12. R after completion leaves time frozen.
13. Hazard/Fall after completion leave time frozen.
14. Enter starts fresh run at zero.
15. Collectibles restore to 0/3 on new run.
16. deathCount/checkpoints/completion reset as before.
17. moving platform resets only on Enter.
18. cyan box resets only on Enter.
19. second run timer works again.
20. Release shows TIME + COLLECTED HUD and no ImGui.

### Completion criteria
M27 is complete only when:
- all three configs build;
- manual Phase C passes;
- timer behavior matches contracts above;
- no timer manager/persistence scope creep exists;
- docs are updated;
- user approves;
- feature branch is committed/pushed/merged;
- `main` is clean and synchronized.

### Git closure
After approval:

```powershell
git status
git add .
git commit -m "Milestone 27 - Run timer and completion time"
git push -u origin milestone/27-run-timer

git checkout main
git pull origin main
git merge milestone/27-run-timer
git push origin main
git status
```

Do not start Milestone 28 before `main` is clean and synchronized.


# Milestone 28 — Best Time (Session Record)

### Status
**Complete (manually approved).** Do not reopen M28. Milestone 29 is the active milestone.

### Branch
`milestone/28-session-best-time`

### Recommended Cursor model
Grok 4.6 High — Fast OFF

### Goal
Build on M27 with a session-only best completion time. On each run's first level completion, compare the frozen M27 run time with the best completed run since process launch. Display the record in gameplay UI and preserve it across Enter fresh-run restarts.

M28 deliberately has no disk persistence.

### Player-facing contract
Before any completed run:
```text
TIME 00:12.345
BEST --:--.---
COLLECTED 0 / 3
```

First completion:
```text
TIME 00:40.500
BEST 00:40.500
LEVEL COMPLETE
PRESS ENTER TO RESTART
```

After Enter:
```text
TIME 00:00.000
BEST 00:40.500
```

A slower later run leaves BEST unchanged. A faster later run replaces BEST.

### Core rule
BEST changes only on the first `completed: false -> true` transition of a run.

Candidate value is exactly `runTimerState.elapsedSeconds`, after the M27 completion-frame `dt` has been accumulated. Compare underlying `double` values, not formatted strings.

### Minimal state
Application owns session state. Preferred:
```cpp
struct SessionBestTimeState
{
    bool hasBestTime = false;
    double bestSeconds = 0.0;
};
```
`std::optional<double>` is acceptable if it fits existing conventions better. Phase A must choose the smallest clear representation.

Renderer and DebugMetrics receive read-only data. Player, PhysicsWorld, and world remain unaware.

### Comparison
No existing best:
```text
best = current completion time
```

Later completion:
```text
if current < best:
    replace best
else:
    preserve best
```

Exact ties need not rewrite the record. Do not quantize stored time to milliseconds.

### Reset/lifetime semantics
Manual R / Fall / Hazard:
- preserve current TIME;
- preserve BEST;
- never update BEST.

Checkpoints/collectibles:
- no effect on BEST.

After completion:
- TIME remains frozen;
- BEST remains unchanged;
- post-completion movement/collecting/respawns remain allowed.

Enter fresh restart:
- resets current run state exactly as M27;
- resets TIME to zero;
- **preserves BEST**.

Full executable relaunch:
- BEST returns to no-best-yet;
- no persistence.

### HUD
Preserve:
- TIME upper-left;
- COLLECTED upper-right;
- completion UI centered.

Add below TIME:
```text
BEST --:--.---
```
or:
```text
BEST 00:40.500
```

Visible in Debug, Development, and Release. Reuse M27 `RunTimeFormat`; no second formatter.

### Debug
Debug/Development:
- Has session best: true/false
- Session best seconds when available
- Formatted session best / placeholder
- preserve M27 timer and all existing metrics.

### Explicitly out of scope
No persistent record, save/load, files, JSON/config/registry, leaderboards, rankings, medals, score, run history, previous-time list, ghost/replay, splits, achievements, reset-best input, profile, networking, cloud, BestTimeManager, StatisticsManager, RunManager, event bus, ECS, new assets/dependencies, or M29.

### Phase A
Design/scaffolding only:
- inspect exact M27 accumulation/freeze/completion order;
- inspect RestartRun;
- define session lifetime and ownership;
- choose no-best representation;
- define exact best-update insertion point;
- define Renderer boundary/HUD placement;
- define debug metrics;
- prove Enter preserves BEST while process relaunch clears it;
- build Debug/Development/Release.

Scaffolding added (compile-only; M27 gameplay unchanged):
- `gameplay/SessionBestTimeState.h` (`hasBestTime`, `bestSeconds`) plus raw-double `IsBetterSessionCompletion`;
- Application member default/Initialize no-best; `RestartRun` does not reset it;
- `FormatSessionBestTime` in `core/RunTimeFormat.h` (`--:--.---` when no record);
- no live comparison at goal;
- no BEST HUD yet;
- DebugMetrics does not show session-best fields yet.

BEST need not be live yet.

### Phase B
Implementation complete / awaiting Phase C:
- first-completion record via `IsBetterSessionCompletion` in the existing goal branch;
- faster-run replacement; slower/tie preservation;
- `RestartRun` still does not reset `sessionBestTimeState`;
- Release-visible `BEST` HUD below TIME (y = 46);
- Debug/Development Session best metrics;
- docs/build/runtime smoke.
Do not mark complete.

### Phase C manual validation
1. Fresh executable: `BEST --:--.---`.
2. TIME runs normally.
3. R/Fall/Hazard before completion do not affect BEST.
4. Complete Run 1: BEST equals frozen Run 1 TIME.
5. Post-completion wait/move/collect/R/Fall/Hazard: BEST unchanged.
6. Enter: TIME resets; BEST remains.
7. Complete deliberately slower Run 2: BEST remains Run 1.
8. Enter again: BEST remains.
9. Complete deliberately faster Run 3: BEST updates to Run 3 TIME.
10. Goal remains independent of collectible count.
11. BEST updates at most once per run.
12. Release shows TIME + BEST + COLLECTED; no ImGui.
13. Fully close/relaunch executable: BEST returns to `--:--.---`.
14. Confirm no save/config/record file is created.

### Completion criteria
All configs build; Phase C passes; comparison uses raw `double`; Enter preserves BEST; process relaunch clears it; no persistence/scope creep; docs updated; user approves; branch merged; main clean/synchronized.

### Git closure
```powershell
git status
git add .
git commit -m "Milestone 28 - Session best time"
git push -u origin milestone/28-session-best-time

git checkout main
git pull origin main
git merge milestone/28-session-best-time
git push origin main
git status
```

Do not start M29 before main is clean and synchronized.


# Milestone 29 --- Persistent Best Time (Save File v1) [ACTIVE]

### Status

**Phase B — implementation complete / awaiting Phase C manual validation.** Do not mark complete until Phase C manual validation and Git closure are finished. Do not start Milestone 30.

### Branch

`milestone/29-persistent-best-time`

### Recommended Cursor model

Grok 4.6 High --- Fast OFF

### Goal

Extend M28 so the session BEST can survive a full executable restart
through one tiny versioned save file.

M29 is the project's **first persistent player-data milestone**. The
goal is not a generic save system. Persist exactly one datum: the best
completion time.

The design must preserve portability boundaries and CWD independence.

### Player-facing contract

First-ever launch / no valid save:

``` text
BEST --:--.---
```

Complete a run:

``` text
TIME 00:40.500
BEST 00:40.500
```

Close the game completely and launch again:

``` text
BEST 00:40.500
```

Complete a slower run:

``` text
BEST 00:40.500
```

Complete a faster run:

``` text
BEST 00:37.250
```

Close/relaunch:

``` text
BEST 00:37.250
```

If the save is missing, malformed, unsupported, non-finite, negative, or
otherwise invalid, the game must recover safely to:

``` text
BEST --:--.---
```

### Core architecture rule

Gameplay/Application owns the meaning of BEST.

Persistence only loads/stores a small data value. It must not decide
whether a run is a record.

Do not create a generic SaveManager.

### Persistence scope

Persist exactly: - whether a valid best exists; - the raw best time in
seconds.

Do not persist: - current TIME; - completion; - checkpoints; - death
count; - collectibles; - moving platform; - cyan box; - player
transform; - camera; - run history; - previous run; - settings.

### Save format

Use a tiny project-owned, human-readable, versioned text format.

Recommended v1:

``` text
PLATFORMER_SAVE 1
best_seconds 40.500123456
```

Requirements: - explicit magic/header; - explicit version `1`; - one
`best_seconds` value; - enough precision to round-trip the stored
`double`; - strict validation; - no JSON dependency.

The exact whitespace may follow a simple documented parser contract.

### Save location

M29 must not depend on the process current working directory.

Phase A must inspect the current platform/path layer and choose the
smallest writable user-data path boundary appropriate for Windows-first
development without leaking Windows APIs into gameplay.

Preferred architectural direction:

``` text
platform::UserDataDirectory()
```

Application/persistence code may use the returned project-owned path,
but OS-specific path discovery belongs behind `platform`.

Do not blindly write beside the executable if that would create an
install-permission problem.

Phase A must report the exact chosen Windows path policy before Phase B
writes files.

### Suggested save filename

``` text
best_time_v1.txt
```

Use a project-specific subdirectory if required by the chosen user-data
path policy.

### State ownership

Preserve M28:

``` cpp
struct SessionBestTimeState
{
    bool hasBestTime = false;
    double bestSeconds = 0.0;
};
```

Application still owns the live BEST state.

Persistence should expose a tiny data-oriented boundary, for example:

``` cpp
struct PersistentBestTimeData
{
    bool hasBestTime = false;
    double bestSeconds = 0.0;
};
```

A still-smaller equivalent is acceptable after Phase A inspection.

Do not make the persistence layer own gameplay state.

### Load semantics

Load once during Application initialization, before normal gameplay
rendering.

Valid save: - populate session BEST from disk.

Missing save: - normal first-run condition; - no warning/error
required; - BEST remains unavailable.

Invalid/corrupt save: - do not crash; - do not accept partial garbage; -
BEST remains unavailable; - Development/Debug may report a concise
diagnostic.

Unsupported future version: - do not guess; - do not parse as v1; - BEST
remains unavailable.

### Validation

A loaded `best_seconds` must be: - finite; - strictly greater than
zero; - representable as the project's `double`; - syntactically valid
under the documented v1 format.

Reject NaN, infinity, zero, negative values, missing fields, wrong
magic, wrong version, trailing malformed content, and parse failure.

### Save semantics

Write only when a new BEST is established: - first completed run; -
later faster completed run.

Do not write: - every frame; - on Enter; - on R; - on Fall; - on
Hazard; - on checkpoint; - on collectible; - on slower/tied
completion; - merely on shutdown.

### Failure semantics

A save-write failure must not invalidate the in-memory run result.

If the player sets a new BEST but persistence fails: - live BEST remains
the new record for the current process; - game continues; -
Debug/Development reports failure; - no crash.

Release must remain playable.

### File-write safety

Do not intentionally expose the save to obvious partial-write
corruption.

Corrected Phase A strategy (Windows-validated): 1. write complete v1
contents to a sibling `best_time_v1.tmp`; 2. flush/close successfully;
3. promote temp -> final through `platform::ReplaceFileWithTemporary`.
Do **not** `remove(final)` as a separate step before promotion.

Windows: `ReplaceFileW` when `best_time_v1.txt` already exists (old
final stays valid until that OS call succeeds); `MoveFileExW` with
`MOVEFILE_WRITE_THROUGH` when the final file is missing (first save).
POSIX: `std::filesystem::rename` for compile compatibility only.

Do not claim crash-proof/durable transactional storage. M29 requires
avoiding obvious partial final writes and delete-old-before-promote,
plus flush/close of the temp. It does not require fsync, directory
fsync, journaling, power-loss-proof transactions, or backup
generations.

Keep it narrow. Do not build journaling, backups, transactions, or a
generic atomic-file framework.

Temp leftover policy: load never reads `best_time_v1.tmp`. If temp
write or promotion fails, Phase B should best-effort `remove(temp)`.
If that cleanup fails, ignore it for gameplay; Save still reports
Error. No temp recovery, journal, or backup rotation.

### Precision

BEST comparison remains M28 raw `double`.

Persist enough decimal precision for a `double` round trip (for example
`std::numeric_limits<double>::max_digits10`).

HUD remains `MM:SS.mmm` via the existing formatter.

Do not store only formatted milliseconds.

### Restart semantics

Enter: - resets current run state; - preserves loaded/current BEST; -
performs no save unless a new record was just established at completion.

Full process relaunch: - loads persisted BEST.

### HUD

No new gameplay HUD element is required.

Preserve:

``` text
TIME MM:SS.mmm
BEST MM:SS.mmm
COLLECTED N / 3
```

Before any valid saved or newly completed BEST:

``` text
BEST --:--.---
```

### Debug/Development diagnostics

Add minimal persistence diagnostics, such as: - Save path; - Load
status: Missing / Loaded / Invalid / UnsupportedVersion / Error; - Save
status: NotAttempted / Saved / Error; - persisted BEST formatted value
when valid.

Do not expose a save editor.

### Release

Release: - loads/saves the BEST; - shows normal gameplay HUD; - no Dear
ImGui; - no verbose console spam required for normal missing-save
behavior.

### Portability

-   gameplay must not call Win32 APIs;
-   OS-specific writable-directory discovery stays in platform code;
-   persistence parsing/serialization should be standard C++ where
    practical;
-   use `std::filesystem`;
-   preserve case-sensitive path discipline;
-   do not introduce a Windows-only save design into Application.

### Explicitly out of scope

No generic save system, save slots, autosave framework, manual save/load
UI, current-run resume, settings persistence, checkpoint persistence,
collectible persistence, death persistence, run history, previous times,
leaderboard, ranking, medals, score, ghost/replay, cloud sync, Steam
integration, achievements, encryption, compression, checksum system,
backup rotation, migration framework, JSON library, database,
SaveManager, ProfileManager, ECS/event bus, or M30.

### Phase A --- Persistence design/scaffolding

Inspect and report: - current Application initialization; - M28 BEST
update point; - current platform/path abstractions; -
executable-relative asset path behavior; - available `std::filesystem`
utilities; - exact Windows writable user-data path policy; - exact save
filename/path; - v1 grammar; - parser validation; - double serialization
precision; - load result model; - save result model; - safe-write
strategy; - ownership boundaries; - load insertion point; - save
insertion point; - CWD independence; - failure behavior; - test
strategy.

Phase A may add compile-only structs/helpers/interfaces and
parser/serializer unit-like static/runtime checks, but must not make
persistence live.

Scaffolding added and approved in Phase A (safe-replacement correction included):
- `platform::UserDataDirectory()` (Windows `FOLDERID_LocalAppData`; POSIX stub empty);
- `platform::ReplaceFileWithTemporary` (`platform/FileReplace.h`; Windows `ReplaceFileW` / first-save `MoveFileExW`; POSIX `rename`);
- `persistence/BestTimeSave.h` + `.cpp`: v1 serialize/parse, path helpers, load/save status enums.

Safe-replacement correction: the earlier `remove(final)` then `rename(temp, final)` plan is rejected. Promotion goes through the platform boundary only.

### Phase B --- Implementation

Implemented: - user-data path boundary; - `LoadBestTime` / `SaveBestTime`; - load once in Initialize after `sessionBestTimeState = {}`; - save only on new record after in-memory update; - temp sibling + `ReplaceFileWithTemporary`; - nonfatal save failures; - Debug/Development persistence metrics; - docs; - all Windows configs; - Development/Release smoke from unrelated CWD.

Do not mark M29 complete until Phase C.

### Phase C --- Manual validation

Canonical save (manipulate **only** this file; do not delete unrelated LocalAppData content):

``` text
%LOCALAPPDATA%\Platformer3D\best_time_v1.txt
```

Temp sibling (never load; do not treat as recovery):

``` text
%LOCALAPPDATA%\Platformer3D\best_time_v1.tmp
```

At minimum: 1. Start with no save: `BEST --:--.---`, Load Missing, TIME fresh. 2. Complete Run 1: BEST = TIME, Save Saved, canonical file created (`PLATFORMER_SAVE 1` / `best_seconds <positive finite>`). 3. Close/relaunch from unrelated CWD: BEST restored, Load Loaded, TIME/completion/COLLECTED fresh. 4. Enter fresh run: BEST preserved; no save merely due to Enter. 5. Complete slower run: BEST unchanged; save mtime unchanged if timestamps are reliable; Save stays NotAttempted unless this process already saved a record. 6. Complete faster run: BEST updates, Save Saved, file updates. 7. Close/relaunch: faster BEST restored. 8. Delete only the canonical save: next launch returns to placeholder, Load Missing, no crash. 9. Corrupt save contents: Load Invalid, placeholder, no crash. 10. Wrong magic: Invalid. 11. Unsupported version (`PLATFORMER_SAVE 2`): UnsupportedVersion. 12. `best_seconds 0`, negative, NaN, infinity: Invalid. 13. Restore/create valid save: loads again. 14. R/Fall/Hazard/checkpoints/collectibles/Enter without a new record do not rewrite the save. 15. Release from unrelated CWD loads/saves correctly and has no ImGui. 16. Confirm only BEST persists (TIME, completion, checkpoints, deathCount, collectibles, platform, cyan box, camera are fresh).

### Completion criteria

M29 is complete only when: - all configs build; - valid BEST survives
process relaunch; - missing/invalid/unsupported saves fail safely; -
save writes occur only for a new record; - raw double precision is
preserved; - CWD independence is demonstrated; - gameplay remains
independent of OS-specific APIs; - no generic save framework/scope creep
exists; - Phase C passes; - user approves; - branch is
committed/pushed/merged; - main is clean and synchronized.

### Git closure

After approval:

``` powershell
git status
git add .
git commit -m "Milestone 29 - Persistent best time"
git push -u origin milestone/29-persistent-best-time

git checkout main
git pull origin main
git merge milestone/29-persistent-best-time
git push origin main
git status
```

Do not start Milestone 30 before `main` is clean and synchronized.


## Milestone 30 — Level Data v1: Data-Driven Single Level

### Status
**Phase A — repository inspection and Level Data design/scaffolding (current).** Do not mark complete until Phase B, Phase C manual validation, and Git closure are finished. Do not start Milestone 31.

### Branch
`milestone/30-level-data-v1`

### Recommended Cursor model
**Grok 4.6 High — Fast OFF**

### Goal
Introduce the first project-owned **data-driven level definition** without adding multiple playable levels or an editor yet.

M30 converts the current hard-coded M29 world layout into one explicit `LevelDefinition` loaded/constructed as project data, while preserving the exact gameplay behavior validated through M29.

This milestone is the architectural bridge toward:

- multiple levels;
- a Development level editor;
- configurable player/camera/world parameters;
- future enemy and boss spawn data;
- asset packaging by level;
- Web/Android/iOS/embedded portability.

M30 does **not** implement those future systems yet.

---

### Player-visible contract
The game should look and play the same as M29.

Expected current route and systems remain intact:

- player spawn;
- static platforms;
- slopes;
- moving platform;
- two checkpoints;
- two hazards;
- three collectibles;
- goal;
- cyan dynamic physics box;
- camera behavior;
- TIME;
- persistent BEST;
- Release HUD;
- restart/respawn semantics.

The architectural change should not require the player to learn any new controls.

---

### Core architectural objective
Create a project-owned level-data boundary so gameplay systems no longer define the current level by scattering canonical world coordinates/specifications across Application/Renderer/Physics initialization.

The canonical geometry/gameplay layout for the current level should come from one `LevelDefinition` or a comparably small set of level-data structures.

The definition should describe **what the level contains**. Runtime systems remain responsible for **how those objects behave**.

Examples:

- Level data says where a checkpoint is and its trigger/respawn positions.
- Application still decides checkpoint progression semantics.
- Level data says where a hazard is.
- Application still decides that hazard contact causes Hazard respawn.
- Level data says where the goal is.
- Application still owns completion state and BEST logic.
- Level data says moving-platform path/speed.
- Physics/runtime still performs kinematic motion and CharacterVirtual carry.

---

### M30 LevelDefinition scope
The exact structs should be decided after repository inspection, but M30 should centralize the current level's immutable authoring data, including at minimum:

1. player initial spawn;
2. static boxes/platforms;
3. slope definitions;
4. moving-platform definition;
5. checkpoint definitions;
6. hazard definitions;
7. collectible definitions;
8. goal definition;
9. cyan dynamic-box initial definition;
10. level camera tuning that is currently canonical for this level, if inspection confirms it is level-specific rather than global engine policy.

Do not blindly put every constant in `LevelDefinition`. Physics constants, player movement constants, global rendering policy, save paths, UI positions, etc. are not level authoring data.

---

### Level identity
Introduce a minimal stable identity for the current level, for example:

`level_01`

or an equivalent project-owned identifier.

M30 still has exactly **one playable level**.

Do not build a campaign, level-selection screen, level progression system, unlock system, world map, or next-level transition.

---

### Source format
M30 should first establish the **runtime data model**, not prematurely commit the project to a large external serialization/editor format.

Preferred M30 direction:

- project-owned C++ `LevelDefinition` data;
- one canonical factory/data source such as `CreateLevel01Definition()` or equivalent;
- runtime systems consume that definition;
- no JSON dependency;
- no external level parser required yet.

The structures must nevertheless be designed so a later editor/file format can populate the same conceptual data without changing gameplay semantics.

If repository inspection finds a compelling existing lightweight data mechanism, Cursor should report it during Phase A before changing this direction.

---

### Ownership boundaries
#### Level data
Owns immutable authoring/configuration data for the current level.

#### Application
Owns run state and gameplay meaning:

- checkpoint progression;
- deaths/respawn reason;
- completion;
- collectible collected flags;
- run timer;
- session/persistent BEST integration;
- restart priority/order.

#### PhysicsWorld
Consumes relevant level geometry/physics definitions and creates runtime bodies.

#### Renderer
Consumes level data/runtime state read-only for visuals.

#### Player
Keeps player movement/CharacterVirtual responsibilities.

#### Platform
Keeps OS/path/time/window/file-replacement responsibilities.

#### Persistence
Keeps M29 BEST save responsibilities only.

---

### Runtime state must remain separate from authoring data
Do not put mutable run state into `LevelDefinition`.

Examples that must remain outside immutable level data:

- active checkpoint index;
- collected flags;
- death count;
- completion flag;
- current moving-platform position/phase;
- cyan-box current transform;
- current player transform;
- current TIME;
- BEST;
- load/save statuses.

A fresh run should instantiate/reset runtime state from the immutable level definition where appropriate.

---

### Restart/respawn preservation
Preserve all M20–M29 semantics.

Ordinary respawn from R/Fall/Hazard:

- teleports/reset Player as before;
- preserves active checkpoint;
- preserves collectibles;
- preserves completion;
- preserves timer behavior;
- preserves BEST;
- does not reset moving platform;
- does not reset cyan box.

Enter fresh restart:

- resets run-owned state;
- restores Player from level initial spawn;
- restores moving platform from its level definition;
- restores cyan dynamic box from its level definition;
- clears checkpoints/deaths/completion/collectibles/current TIME;
- preserves persistent/session BEST.

---

### Static validation
Where useful, keep or migrate compile-time/runtime validation that protects authored data assumptions.

Examples:

- exactly two checkpoints for the current M30 level;
- exactly two hazards;
- exactly three collectibles;
- valid positive sizes;
- checkpoint indices/order consistent;
- spawn/respawn points valid;
- level identity non-empty;
- moving-platform path valid.

Do not build a generic schema-validation framework.

---

### Debug/Development
Add a small Level Data section to existing DebugMetrics/ImGui showing useful read-only information such as:

- level id/name;
- player initial spawn;
- static box count;
- slope count;
- checkpoint count;
- hazard count;
- collectible count;
- goal presence;
- moving platform presence;
- dynamic-box presence.

If camera tuning is moved into level data, show the effective values.

Do not create the level editor in M30.

---

### Release
Release behavior must remain equivalent to M29:

- no ImGui;
- same playable level;
- same HUD;
- persistent BEST works;
- runtime assets remain executable-relative;
- save remains user-data-relative;
- no dependency on CWD.

---

### Portability
Level data must be project-owned standard C++ and contain no:

- Win32 types/APIs;
- raylib types in gameplay/authoring structures;
- filesystem policy;
- Android/iOS/Web APIs.

Prefer existing project-owned math types such as `core::Vec3` where suitable.

This is important for the future Web/Android/iOS/Raspberry Pi targets and Development editor.

---

### Asset pipeline
M30 does not change the cooker or runtime asset packaging.

Do not implement packs yet.

Current cooked/staged assets continue working exactly as in M29.

---

### Save compatibility
M29 save v1 must remain valid without migration.

M30 must not change:

- `PLATFORMER_SAVE 1`;
- `best_seconds` semantics;
- `%LOCALAPPDATA%/Platformer3D/best_time_v1.txt` Windows policy;
- load/save timing.

The current save does not need a level id because M30 still has one playable level and persists only the global/session best established by M29.

Do not create save v2.

---

### Phase A — Repository inspection and Level Data design/scaffolding
Cursor must inspect the actual M29 code before implementation and report:

- every location that currently owns canonical level coordinates/specs;
- duplicate geometry/spec data between Application, PhysicsWorld, Renderer, GreyboxWorld or other files;
- exact M29 initialization/restart flow;
- what is immutable authoring data vs mutable runtime state;
- proposed LevelDefinition structures;
- exact ownership and file placement;
- how PhysicsWorld will consume level geometry;
- how Renderer will consume level data;
- how Application will consume checkpoints/hazards/collectibles/goal;
- moving-platform definition/runtime split;
- cyan-box definition/runtime split;
- whether camera settings should be level data now;
- migration plan preserving exact M29 coordinates/behavior;
- validation strategy;
- future editor compatibility without implementing editor/file parsing;
- build impact.

Phase A may add compile-only structs/factories and documentation, but the game must not yet switch to the new definition if doing so would make the migration partially live.

Phase A scaffolding (M29 gameplay remains live):
- `world/LevelDefinition.h`: immutable aggregate (`level_01`) reusing `Box`, `SlopeSpec`, `MovingPlatformSpec`, `CheckpointSpec`, `HazardSpec`, `CollectibleSpec`, `LevelGoalSpec`;
- `world/Level01.cpp`: `CreateLevel01Definition()` copies current live world constants (not a second authoring source);
- Initialize checks that the factory matches live greybox/spawn/checkpoints/hazards/collectibles/goal, PlatformerCamera framing defaults, and the cyan box initial pose;
- no Application/Renderer/PhysicsWorld consumer migration;
- no DebugMetrics Level Data panel yet (Phase B);
- no editor, JSON, multi-level, or LevelManager.

Build Debug/Development/Release.

Do not commit/push/merge.

---

### Phase B — Data-driven single-level implementation
After Phase A approval:

- create the canonical Level 01 definition;
- migrate current authored level constants into it;
- make Application/PhysicsWorld/Renderer consume the definition through clean boundaries;
- preserve runtime state separately;
- remove obsolete duplicated canonical level constants;
- preserve all M29 behavior;
- add DebugMetrics level-data diagnostics;
- update documentation;
- build all configurations;
- smoke Development and Release from unrelated CWD.

M30 remains incomplete until Phase C.

**Phase B:** Implementation complete, awaiting Phase C manual validation. `game/source/world/Level01.cpp` is the canonical authored Level 01 source. `CreateLevel01Definition()` populates literals directly (no copy from legacy `kGround` / `kCheckpoints` / similar globals). Application owns one immutable `LevelDefinition` constructed at member initialization. PhysicsWorld creates statics/slopes/moving platform/cyan box from that definition. Renderer draws from the same definition plus runtime state. Debug/Development expose a Level Data section. M29 save v1 is unchanged. No editor, external level file, LevelManager, or second level.

---

### Phase C — Manual validation
At minimum validate:

1. initial spawn unchanged;
2. complete traversal remains possible;
3. static platforms/slopes unchanged;
4. moving platform path/speed/carry unchanged;
5. cyan box behavior unchanged;
6. CP1/CP2 progression and respawns unchanged;
7. both hazards unchanged;
8. all three collectibles unchanged;
9. goal/completion unchanged;
10. R/Fall/Hazard priority unchanged;
11. Enter fresh restart resets from level definition correctly;
12. TIME behavior unchanged;
13. M29 BEST loads/saves unchanged;
14. existing M29 save v1 remains compatible;
15. Debug/Development Level Data metrics correct;
16. Release has no ImGui;
17. Development/Release work from unrelated CWD;
18. no new gameplay controls;
19. no external level file/editor exists yet;
20. only one playable level exists.

---

### Explicitly out of scope
M30 must NOT implement:

- Milestone 31;
- multiple playable levels;
- campaign progression;
- level selection;
- next-level transition;
- level editor/editor mode;
- gizmos;
- external JSON/YAML/TOML level files;
- generic serialization framework;
- enemies;
- enemy spawns;
- bosses;
- combat;
- health;
- audio/music systems;
- asset packs;
- asset streaming;
- Web build;
- Android/iOS port;
- Raspberry Pi port;
- save v2;
- per-level BEST records;
- ECS;
- event bus;
- LevelManager/GameManager/WorldManager unless a concrete current requirement proves one is necessary.

---

### Completion criteria
M30 is complete only when:

- the current level has one canonical project-owned data definition;
- runtime systems consume that definition instead of owning duplicated authoring coordinates;
- immutable level data and mutable run state are clearly separated;
- the game behaves equivalently to M29;
- M29 save v1 remains compatible;
- Debug/Development expose useful level-data diagnostics;
- Debug/Development/Release build successfully;
- unrelated-CWD smoke passes;
- Phase C manual validation passes;
- no M31 scope has been implemented.



## Milestone 31 — External Level File v1

### Status
Phase B implementation complete. Not complete. Awaiting Phase C manual validation.

### Branch
`milestone/31-external-level-file-v1`

### Goal
Move the authored data for the single playable `level_01` out of hard-coded C++ values and into one project-owned external level file, while preserving the M30 `LevelDefinition` as the runtime data model and preserving all M30 gameplay exactly.

This milestone establishes the first real **authoring file → validated LevelDefinition → runtime** path. It is the bridge toward the future Development Level Editor and multiple levels, but it does **not** implement either feature yet.

### Player-visible contract
The game should look and behave exactly like M30:

- same Level 01 geometry and route;
- same Player spawn;
- same checkpoints, hazards, collectibles and goal;
- same moving platform and cyan dynamic box;
- same camera framing;
- same TIME/BEST/COLLECTED/completion behavior;
- same persistent BEST save;
- same Release behavior.

The architectural difference is that Level 01 authored values are loaded from an external runtime level file rather than being compiled into `Level01.cpp`.

### Canonical data flow

```text
Level authoring file
        ↓
strict project-owned loader/parser
        ↓
validated world::LevelDefinition
        ↓
Application
   ├── gameplay meaning/state
   ├── PhysicsWorld
   └── Renderer
```

`world::LevelDefinition` remains the portable runtime representation introduced in M30.

### Scope

#### 1. One external Level 01 file
Create exactly one authored level file for `level_01`.

Preferred project layout:

```text
assets/source/levels/level_01.level
        ↓ cooker/staging
assets/cooked/levels/level_01.level
        ↓ runtime staging
<exe>/assets/levels/level_01.level
```

The exact extension/location may be refined during Phase A if repository inspection shows a cleaner fit, but the level must participate in the existing source → cooked → staged asset pipeline.

#### 2. Project-owned text format v1
Use a small, explicit, versioned, human-readable project format.

Do not add JSON/YAML/TOML/XML or a third-party serialization dependency.

The format must include:

- magic/header;
- format version 1;
- level ID;
- initial spawn;
- kill plane;
- ground;
- six elevated platforms;
- two slopes;
- moving platform;
- two checkpoints;
- two hazards;
- three collectibles;
- goal;
- cyan dynamic box;
- camera offset/FOV.

The final grammar must be documented and deterministic.

#### 3. Strict parser
Implement a project-owned parser that rejects malformed content rather than silently guessing.

Reject at minimum:

- wrong magic;
- unsupported version;
- missing required sections/fields;
- duplicate singleton fields;
- invalid numeric parsing;
- NaN/Inf;
- invalid sizes;
- invalid counts;
- invalid moving-platform path/speed;
- invalid cyan-box mass;
- invalid FOV;
- trailing malformed/unrecognized content according to the chosen grammar.

#### 4. Load status
Provide a small typed result/status suitable for Debug/Development diagnostics, conceptually distinguishing:

- Loaded;
- Missing;
- Invalid;
- UnsupportedVersion;
- Error.

Do not create a generic asset error framework.

#### 5. Failure policy
A missing or invalid canonical Level 01 file is a **fatal initialization error** for this milestone.

Do not silently fall back to a compiled copy of Level 01, because that would recreate two canonical authoring sources and could hide packaging/editor errors.

Failure should be reported clearly in Debug/Development and via the existing application/platform error path as appropriate, then initialization should fail cleanly.

#### 6. Remove compiled Level 01 authored values
After the migration, `Level01.cpp` must no longer contain a second full copy of the Level 01 coordinates.

It may be removed or reduced to a path/bootstrap helper if justified.

Changing CP1, the goal, cyan-box initial position, camera framing, etc. should require editing the external Level 01 authoring file only.

#### 7. Runtime model remains M30
Do not redesign `LevelDefinition` merely because it is now loaded.

The loader populates the existing project-owned types.

Runtime state remains separate:

- active checkpoint;
- respawn position;
- collected flags;
- completion;
- death count;
- moving-platform runtime pose/direction;
- cyan-box runtime physics state;
- timer;
- BEST.

#### 8. Existing asset pipeline
Integrate the level file with the existing cooker/staging process.

For v1, cooking may be a validated/canonicalized copy if no transformation is required, but it must be handled deliberately by the cooker and manifest rather than bypassing the pipeline.

The runtime must load the staged/cooked level file using executable-relative runtime asset paths, not the working directory.

#### 9. Incremental cooker
Preserve the existing incremental/hash behavior where applicable.

Changing the source Level 01 file should cause the cooked Level 01 output to update; unchanged input should not be unnecessarily rewritten if the existing cooker architecture supports that contract.

#### 10. Camera
Persist/load only the M30 level-authored camera values:

- offset;
- FOV.

Dead-zone and smoothing/sharpness remain global controller policy.

#### 11. Save compatibility
M29 save v1 remains unchanged:

```text
PLATFORMER_SAVE 1
best_seconds <double>
```

Do not add level ID and do not create save v2.

#### 12. Debug metrics
Development/Debug should expose read-only level loading information, including at minimum:

- level source/runtime path;
- load status;
- format version if loaded;
- Level ID;
- existing M30 Level Data metrics.

No editor controls yet.

#### 13. Release
Release must load the external staged Level 01 file and remain CWD-independent.

Release must not contain Dear ImGui.

A properly packaged Release therefore depends on its runtime asset content, including the level file.

### Architecture rules

- Application owns the active `LevelDefinition`.
- Loader/parser owns syntax and validation, not gameplay meaning.
- PhysicsWorld derives Jolt runtime representations from `LevelDefinition`.
- Renderer consumes `LevelDefinition` read-only.
- No raylib/Jolt/Win32 types in the level format/runtime data model.
- Filesystem/runtime path policy remains behind existing project/platform boundaries.
- No generic serialization framework.
- No LevelManager/SceneManager/EntityManager/ECS/event bus.

### Portability
The format and parser must use portable C++ and project-owned types so the same level content can later be used on Windows, Web, Raspberry Pi, Android and iOS.

Platform-specific differences in runtime filesystem/access can be handled behind platform/runtime-asset boundaries later without changing the level grammar or gameplay systems.

### Future editor compatibility
This milestone deliberately establishes the file format that a future Development editor can eventually write.

However, M31 does **not** implement:

- editor mode;
- gizmos;
- object selection;
- property panels;
- save-from-editor;
- undo/redo.

The loader should not depend on editor code.

### Future multi-level compatibility
The format must carry a level ID and be capable of representing another `LevelDefinition` later, but M31 still loads exactly one canonical `level_01` path.

Do not add:

- Level 02;
- registry;
- campaign;
- level selection;
- next-level transition.

### Validation
Build and validate:

- Debug;
- Development;
- Release;
- Development from unrelated CWD;
- Release from unrelated CWD.

Manual Phase C must regress the full M30 route and also validate:

- valid external level load;
- level load diagnostics;
- CWD independence;
- source edit → cooker → runtime effect using one safe temporary authored value change, then restore it;
- missing level file fails cleanly;
- malformed level file fails cleanly;
- unsupported version fails cleanly;
- restored valid file launches normally;
- no compiled fallback hides failures.

### Explicitly out of scope

- Milestone 32;
- multiple playable levels;
- campaign progression;
- level transitions;
- Development editor;
- gizmos;
- external editor application;
- enemies;
- bosses;
- combat;
- health;
- audio systems;
- asset packs;
- asset streaming;
- Web build;
- Android/iOS/Raspberry Pi ports;
- save v2;
- per-level BEST;
- JSON/YAML/TOML/XML;
- third-party serialization libraries;
- generic serialization/reflection;
- LevelManager/SceneManager/ECS/event bus.

### Completion criteria
M31 is complete only when:

1. the external Level 01 file is the sole canonical authored source for the current level;
2. it passes through the cooker/staging pipeline;
3. the runtime loads and strictly validates it into `LevelDefinition`;
4. missing/invalid/unsupported content fails cleanly without compiled fallback;
5. all M30 gameplay is manually regression-tested successfully;
6. Debug/Development diagnostics expose the load state;
7. Release remains CWD-independent and loads packaged level data;
8. M29 persistent BEST remains compatible;
9. no editor or multiple-level system has been prematurely introduced;
10. Phase C is approved before commit/merge.

### Phase A — Inspection, format design, parser scaffolding
**Phase A:** Complete. Parser, cooker/staging, canonical source file, and `LevelFileTest` were added while live gameplay still used `CreateLevel01Definition()`. That compiled factory was removed in Phase B.

### Phase B — Live runtime migration
**Phase B:** Implementation complete, awaiting Phase C manual validation.

The sole live authored source for Level 01 is:

```text
game/assets/source/levels/level_01.level
        → cooker (level_v1, header check + byte copy)
game/assets/cooked/levels/level_01.level
        → CMake POST_BUILD staging
<exe>/assets/levels/level_01.level
        → LoadLevelFile / ParseLevelText
world::LevelDefinition
```

`LevelDefinition.id` is an owning `std::string`. Application owns `levelDefinition{}` until Initialize. It loads `platform::RuntimeAssetPath("levels/level_01.level")` once, requires `Loaded` plus `id == "level_01"`, then initializes camera framing, respawn, PhysicsWorld, and Player from the loaded definition. Player is constructed at `{0,0,0}` only as a pre-load placeholder and is initialized from the file spawn before any playable frame. `Level01.cpp` and `CreateLevel01Definition()` are gone. Missing/invalid/unsupported/I/O/wrong-ID Level 01 fails Initialize with stderr diagnostics and no compiled fallback. BEST save remains nonfatal and still loads after successful required init. RestartRun and ordinary respawns do not reread the file. PhysicsWorld/Renderer remain data consumers. Debug/Development add a Level Loading section; Release HUD is unchanged. Format v1 is unchanged. No editor, no LevelManager, no second level.

Do not commit/push/merge until Phase C is approved. Do not mark M31 complete.

### Final status

Complete (manually approved) and merged as `Milestone 31 - External level file v1`.

---

---


## Milestone 32 --- Development Level Editor v1

### Status

Phase A complete: design resolved, Level Format v1 writer implemented and
tested, Development authoring boundary in place. Phase B complete: the
Development editor is live — F2 toggle, whole-simulation pause, editable
working copy, validated Apply Preview with PhysicsWorld rebuild, Modified and
Dirty tracking, and explicit source Save. M32 is complete and merged.

### Branch

`milestone/32-development-level-editor-v1`

### Goal

Create the first Development-only in-game level editor on top of M31's
external `PLATFORMER_LEVEL 1` data contract.

M32 proves that the Development build can enter an editor mode, inspect
the active `world::LevelDefinition`, edit a deliberately small set of
authored properties, preview changes consistently in rendering/physics,
and explicitly save a deterministic Level Format v1 file back to the
canonical project source.

This is the architectural foundation for the larger map/game editor
discussed for later milestones; it is not yet a complete editor.

### Development contract

-   M31 gameplay remains unchanged while the editor is closed.
-   `F2` is the preferred semantic toggle unless inspection finds a
    conflict.
-   Editor mode pauses gameplay simulation.
-   Dear ImGui exposes a Level Editor panel.
-   First editable set: initial spawn, camera offset/FOV, and
    static/elevated platform center/size.
-   A platform can be selected from the UI.
-   Preview must keep rendered geometry and Jolt collision consistent.
-   Save is explicit; no autosave.
-   Save targets the canonical source
    `game/assets/source/levels/level_01.level`, never the staged runtime
    copy.
-   Normal cooker/build remains responsible for source → cooked → staged
    propagation.
-   Release exposes no editor UI or authoring write path.

### Architecture

`Application` remains owner of the active `LevelDefinition`. A focused
`editor/LevelEditor` module may inspect/edit it but must not become
gameplay ownership.

Add a focused deterministic Level Format v1 serializer/writer as the
inverse companion to the M31 parser. Do not add generic serialization,
reflection, a scene manager, entity manager, ECS, or a general property
framework.

The source-authoring path must be resolved explicitly for Development
tooling and must not depend on CWD or heuristic parent-directory
searching. Phase A must inspect CMake/build configuration and design
this boundary before live writing is enabled.

### Simulation pause

While editor mode is active: - no gameplay timer advancement; - no
Player simulation; - no moving-platform progression; - no dynamic-box
simulation; - no checkpoint/hazard/collectible/goal processing; - no
deaths or BEST changes.

Editor UI remains responsive.

### Preview rules

-   Camera offset/FOV may preview immediately.
-   Spawn edits change authored future fresh-run spawn; they must not
    cause arbitrary teleports on every field edit.
-   Static/elevated platform edits must update rendering and collision
    together. Visual-only movement with stale Jolt collision is
    forbidden.
-   A controlled physics/world rebuild while paused is acceptable if it
    is the smallest safe solution.

### Writer/save rules

-   Canonical header remains `PLATFORMER_LEVEL 1`.
-   Deterministic canonical record ordering.
-   Locale-independent numeric formatting with enough precision for
    round-trip.
-   Existing semantic validation must pass before save.
-   Invalid/non-finite/zero-or-negative required values must not be
    saved.
-   Prefer temp-file + safe replacement rather than truncate-in-place.
-   Successful save clears dirty state.
-   Save failure is reported and does not pretend success.

### Dirty state

Expose at least Clean/Dirty and last save result. Merely opening the
editor must not mark the level dirty.

### Explicitly out of scope

Level 02, transitions, campaign, enemies, bosses, combat, full object
creation/deletion, 3D gizmos, world mouse picking, undo/redo,
copy/paste, hierarchy/scene graph, generic inspector/reflection, asset
browser, material/audio/animation/terrain editors, automatic
Python/CMake spawning from the game, asset packs, Web/mobile/Pi editor,
save v2, per-level BEST, LevelManager, SceneManager, ECS, event bus.

### Phases

#### Phase A --- Inspection + writer/editor scaffolding

Inspect configuration boundaries, frame loop, Dear ImGui,
LevelDefinition/parser/validation, physics rebuild constraints,
runtime/source paths and CMake. Design Development authoring-root
resolution and editor ownership. Implement deterministic Level Format v1
serialization/writer tests and non-live scaffolding as appropriate. Do
not enable live editing/saving yet.

**Phase A outcome (complete):**

Writer. `world/LevelWriter.h/.cpp` beside the M31 parser.
`SerializeLevelText` emits one canonical record order (documented in
`docs/LEVEL_FORMAT_V1.md`) using `std::to_chars` shortest round-trip form:
locale-independent, exactly reparsable, never NaN/Inf.
`IsWritableLevelDefinition` = `IsValidLevelIdToken` +
`LevelDefinitionHasRequiredAuthoredContent` gates serialization and saving, so
an invalid definition yields no text and touches no file. `SaveLevelFile`
requires an absolute path, writes `<target>.tmp`, flush/closes, then promotes
via `platform::ReplaceFileWithTemporary`. `WriteLevelFileStatus` is
`Saved`/`Invalid`/`Error`. `ReplaceFileWithTemporary` moved into its own
`FileReplace{Windows,Posix}.cpp` so the test target links the boundary without
ole32/shell32. `IsValidLevelIdToken` was lifted out of the parser's anonymous
namespace into `LevelFile.h` and is now shared; parser validation was not
weakened.

Authoring boundary. `editor/AuthoringPaths.h/.cpp` resolves
`game/assets/source/levels/level_01.level` from the CMake compile definition
`PLATFORMER_AUTHORING_SOURCE_ROOT`, injected for the **Development**
configuration only together with `PLATFORMER_ENABLE_LEVEL_AUTHORING`. No CWD,
no parent-directory searching. Release compiles neither, so the path literal
does not exist in the Release binary (verified by string search: present in
Development, absent in Debug and Release).

Configuration policy. `PLATFORMER_ENABLE_DEBUG_UI` (Debug + Development) means
Dear ImGui is present. `PLATFORMER_ENABLE_LEVEL_AUTHORING` (Development only)
means the build may write project source. Debug therefore compiles the editor
but cannot author.

Editor ownership. `editor/LevelEditor.h/.cpp` holds `LevelEditorState`
(`active`, `dirty`, `selectedPlatformIndex`, `lastSaveStatus`) and a read-only
scaffolding panel. Application still owns the active `LevelDefinition` and all
gameplay state, holds the editor state beside `DebugUi`, and passes both
read-only into `DebugUi::Draw`. No manager/ECS/event-bus/reflection.

Verification. Configure plus Debug/Development/Release builds pass with no new
warnings. `LevelFileTest` passes in all three configurations and covers
round-trip semantic equality across every authored field, byte-level
determinism, canonical header/record counts, a keyword whitelist proving no
runtime state is serialized, invalid-write rejection (FOV 0, negative size,
NaN, Inf, empty and non-grammar id), and safe-save behavior against a
disposable temp directory (new save, replacement save, invalid save leaves the
existing file, relative path rejected, replacement failure returns `Error`, no
leftover temp). Cooker tests pass. Development and Release both launch from an
unrelated CWD with no stderr. `game/assets/source/levels/level_01.level` is
byte-identical before and after
(`SHA256 a494c3b29a7db84661fc5ca32a079f13a9f73c3cf446fbca3edf06d9dc6a0791`).

Not in Phase A: F2 toggle, simulation pause, physics rebuild, field editing,
Apply Preview, and source Save from the running game.

#### Phase B --- Live Development editor

Implement semantic F2 editor mode, simulation pause, Level Editor UI,
supported property editing, safe preview/rebuild, dirty state, explicit
source Save and diagnostics. Preserve Release and M31 gameplay.

**Phase B outcome (complete):**

Toggle. `input::InputState::toggleLevelEditorPressed` is the semantic action,
mapped to the F2 edge press in `input/Input.cpp`; Application consumes only the
flag. `DebugUiBackend::WantsKeyboardCapture()` exposes
`ImGui::GetIO().WantCaptureKeyboard` through `DebugUi::WantsKeyboardCapture()`,
and Application ignores the toggle while it is true, so typing into a field
cannot close the editor. ImGui knowledge stays in the debug-UI backend.

Pause. One guard in the frame loop. Input polling, the toggle, rendering, the
debug UI, the editor panel and window close stay outside it; the whole M31
simulation block — run timer, moving platform, Player, gravity/CharacterVirtual,
hazard detection, fall/hazard/manual respawn, checkpoint activation, goal
completion, BEST comparison and save, collectible pickup, Enter restart,
`PhysicsWorld::Update`, camera follow — sits inside `if (!simulationPaused)`
with its M31 order and content unchanged. No time-scale system.

Ownership. Application still owns the active `LevelDefinition` and all gameplay
state. `editor::LevelEditorState` owns only authored data: `workingCopy` and
`savedSourceBaseline`. `Modified` (working vs active) and `Dirty` (active vs
last saved source) are derived every draw from `world::AuthoredLevelDataEqual`,
lifted out of the writer test into `world/LevelDefinition.h` so the editor and
the round-trip tests share one explicit field list. No reflection. The panel
returns an `editor::LevelEditorRequest` that Application executes after
`renderer.EndFrame()`, so the definition/physics swap happens between frames.

Apply Preview. Validate `IsWritableLevelDefinition` and the `level_01` identity
while the live world is intact, then `Shutdown` / `Initialize(candidate)` /
`InitializePlayer`, and commit `levelDefinition = candidate` only afterwards. A
rebuild failure after `Shutdown` is fatal by policy: stderr diagnostic, Apply
status `Error`, `Application::fatalError` exits the loop through the normal
`Shutdown()` and `Run()` returns 1. No rollback transaction. Success starts a
fresh preview run (Player at authored spawn, movement/respawn/completion/
collectible/timer reset, moving platform and cyan box reset via the rebuild,
`ApplyLevelFraming` + `camera.Initialize`). `SessionBestTimeState`, persisted
BEST, persistence statuses, loaded level id and level-loading diagnostics are
untouched, so the editor can never write BEST.

Editable set. Spawn, camera offset, camera FOV, and ground plus
`elevatedPlatforms[0..5]` center/size, with a plain combo selector and `%.6f`
field precision so authored values such as `1.6732` cannot be silently rounded.
Nothing else is editable; invalid values are reported, never clamped.

Saving. `editor::SaveLevelSource` compiles only under
`PLATFORMER_ENABLE_LEVEL_AUTHORING`; Debug and Release link a stub with no path
and no write call. It resolves `AuthoringLevel01SourcePath()` and delegates to
`world::SaveLevelFile`. Save always serializes the **active** definition and the
button is disabled while `Modified` is true. Success updates the baseline,
clearing `Dirty`; failure leaves both alone and the source intact. No cooking,
rebuilding, restarting, or process spawning.

Session semantics. Activation seeds the working copy and clears `Modified`
without touching gameplay or files; closing never auto-applies or auto-saves;
reopening re-seeds, discarding unapplied edits. `Dirty` is derived, so it
survives the toggle. The baseline is seeded in `Initialize` from the staged
level, so `Dirty` starts false and tracks only this session.

Verification. Configure plus Debug/Development/Release builds pass with no new
warnings. `LevelFileTest` (now covering authored-equality sensitivity for every
editable field) and the new `PhysicsRebuildTest` harness pass in all three
configurations; the harness proves three repeated
`Shutdown`/`Initialize`/`InitializePlayer` cycles with edited data, idempotent
double shutdown and reuse, correct static body count, moving platform and cyan
box reset to authored values, and that collision follows a platform edit (the
old position no longer supports the character, the new one does). Cooker tests
pass. Development and Debug were driven interactively: F2 opens the editor and
freezes the world (0.003% / 0.008% sampled pixel change over three seconds
against 23–24% while running, with `TIME` identical), editing a field sets
`Modified` with `Dirty` false and Save disabled behind "Apply Preview first",
`Revert Working Copy` restores the field, and Apply Preview of both a camera FOV
change and a ground-size change reported `Applied` with `Modified` false,
`Dirty` true, a reset timer, a preserved BEST and a visibly rebuilt world.
Debug reports `Authoring: Unavailable` with Save disabled and no source path.
Release shows no editor and ignores F2. The authoring root literal is present in
the Development binary and absent from Debug and Release. Canonical
`game/assets/source/levels/level_01.level` is byte-identical throughout
(`SHA256 a494c3b29a7db84661fc5ca32a079f13a9f73c3cf446fbca3edf06d9dc6a0791`).

Not in Phase B: real source writing to the canonical file (Phase C), interactive
proof of walking onto a moved platform, `Reload Runtime Level`, filesystem
watching, and any widening of the editable set.

#### Phase C --- Manual validation

Validate editor toggle/absence in Release, true simulation pause,
camera/spawn/platform edits, render/collision agreement, validation
failures, dirty/save behavior, source-file modification, parse
compatibility, cook/build propagation, relaunch, M31 gameplay, BEST
persistence, CWD independence, and restoration of canonical Level 01
values.

### Acceptance criteria

M32 completes only when Development has the first functional Level
Editor; Release has none; simulation pauses in editor mode; supported
fields can be edited and previewed safely; platform rendering/collision
never diverge; v1 serialization is deterministic and reparses; Save
writes the canonical source rather than staged data; cooker/build
propagates it; invalid data cannot be saved; normal M31 gameplay and M29
BEST persistence remain intact; no generic editor/serialization
framework is introduced; all canonical Level 01 values are restored
after validation; all configurations build; Phase C passes; and no
commit/push/merge occurs before approval.


## Milestone 33 --- Visual Level Editor v2: Viewport Navigation, Hierarchy & World Selection

### Status

Phase B implemented: live editor camera, Hierarchy, world picking, Inspector
routing and selection highlight. M33 is **not** complete. Phase C manual
validation remains. Milestone 34 has not started.

### Branch

`milestone/33-visual-level-editor-v2`

### Goal

Evolve the M32 property editor into the first genuinely visual
level-editing workflow.

M33 must let the developer enter editor mode, navigate the 3D level
independently of the gameplay camera, see an explicit hierarchy of
authored level objects, and select supported objects either from the
hierarchy or by clicking them in the 3D world. Selection must stay
synchronized with the Inspector.

This milestone deliberately does **not** add object creation/deletion or
transform gizmos yet. Its purpose is to establish the selection/viewport
architecture those features will build on.

### User experience target

``` text
Development
→ F2 opens editor and pauses gameplay
→ editor camera can navigate around the level
→ Hierarchy lists authored objects
→ click an object in Hierarchy OR click it in the world
→ same object becomes selected
→ Inspector shows that object's supported properties
→ existing M32 edit / Apply Preview / Save workflow continues
```

### Scene/level terminology

For the current project, the active external `LevelDefinition` is the
authored gameplay scene/level.

M33 must not introduce a generic `SceneManager` or engine-wide scene
graph merely to rename this concept. The editor may use the term
**Level** or **Scene/Level** in its UI, but runtime ownership remains
based on the existing `LevelDefinition`.

Multiple levels, New/Open/Save As, and level transitions remain future
work.

### In scope

#### Editor viewport/navigation

-   Development/Debug editor mode retains paused gameplay.
-   Independent editor/navigation camera while editor is active.
-   Mouse + keyboard navigation suitable for inspecting a 3D platformer
    level.
-   Gameplay camera state must be preserved/restored appropriately when
    leaving editor mode.
-   Navigation must not mutate authored camera offset/FOV unless the
    user edits those Inspector properties explicitly.

#### Hierarchy

A focused hierarchy/list of the currently authored LevelDefinition
objects, including at least: - Ground - Platform 0..5 - Slope 0..1 -
Moving Platform - Checkpoint 0..1 - Hazard 0..1 - Collectible 0..2 -
Goal - Dynamic Cyan Box - Player Spawn - Camera

Read-only/non-editable object types may still be selectable and
inspectable.

#### Stable editor selection

Introduce a small, explicit editor selection identity suitable for the
current fixed Level Format v1 categories.

It must identify object type/category plus index where required.

Examples conceptually: - Ground - ElevatedPlatform\[0\] - Slope\[1\] -
Hazard\[0\] - Collectible\[2\] - Goal - Spawn - Camera

Do not introduce a generic entity/component system, UUID database,
reflection system, or scene graph.

#### Hierarchy selection

Clicking an item in the hierarchy selects it.

The selected row/item is visibly highlighted.

#### World selection / picking

Clicking a selectable object in the 3D viewport selects the same object
represented in the hierarchy.

Use project-owned CPU-side ray construction/intersection or the smallest
renderer/input boundary consistent with the existing architecture.

Do not make gameplay code depend directly on raylib.

#### Selection synchronization

Selection is one state shared by: - world picking; - hierarchy; -
Inspector.

Selecting in any supported place updates the others.

#### Selection visualization

The selected world object must have a clear Development/Debug-only
visual indication such as: - wireframe bounds; - outline-like debug
bounds; - bounding box; - other simple non-production selection marker.

Prefer a simple robust debug visualization over a complex outline
shader.

#### Inspector integration

Reuse and improve the M32 Level Editor inspector.

For M32-editable selections: - Spawn - Camera - Ground - Platform 0..5

show the existing editable controls.

For newly selectable but not yet editable types: - show useful read-only
authored properties; - clearly label them read-only for M33.

Do not expand authoring scope merely because an object is selectable.

### Editor camera contract

Phase A must inspect the current camera/render/input abstractions before
choosing controls.

Preferred desktop editor navigation: - RMB held: mouse-look/orbit-style
free look; - WASD: horizontal/free movement; - Q/E or another simple
pair: vertical movement; - mouse wheel: speed adjustment or dolly if
useful; - optional Shift: faster movement.

Exact controls may be refined after repository inspection.

Requirements: - editor camera is not serialized into Level Format v1; -
editor navigation never changes authored gameplay camera framing; -
entering editor initializes the editor camera from a sensible current
view; - leaving editor returns to the gameplay camera without corrupting
its follow state; - no cursor trapping that makes normal window
interaction impossible.

### Picking contract

Phase A must determine the smallest correct picking strategy.

Preferred conceptual flow:

``` text
mouse screen position
→ ray through active editor camera
→ intersect authored selectable geometry/proxies
→ nearest valid hit
→ EditorSelection
```

Picking must account for the actual transforms of selectable objects.

For objects without obvious box geometry
(spawn/checkpoint/collectible/goal/camera), use focused editor-only
selection proxies/bounds consistent with their existing debug/render
representation.

Do not use physics BodyIDs as persistent authoring identity.

### UI capture contract

World navigation/picking must not fire while the mouse is interacting
with Dear ImGui.

Respect ImGui mouse/keyboard capture at the UI boundary.

Examples: - clicking an InputFloat must not select an object behind the
panel; - scrolling an ImGui panel must not zoom/change editor camera; -
typing in an editor field must not move the editor camera.

### M32 preservation

Preserve: - working copy; - Modified; - Dirty; - Apply Preview; - Revert
Working Copy; - Save Level Source; - deterministic Level Format v1
writer; - safe source replacement; - Development-only authoring; - Debug
no source write; - Release no editor; - BEST separation; - paused
simulation.

Selection should normally refer to the editor working copy while
editing.

Apply Preview must preserve a valid selection when the corresponding
authored object still exists.

### Explicitly out of scope

-   object creation;
-   object deletion;
-   duplicate object;
-   transform gizmos;
-   drag-to-move objects;
-   rotation gizmo;
-   scale gizmo;
-   undo/redo;
-   copy/paste;
-   multi-select;
-   box selection;
-   scene graph;
-   parent/child hierarchy;
-   generic entity system;
-   ECS;
-   reflection;
-   generic Inspector;
-   generic serialization;
-   Level 02;
-   New Level/Open Level/Save As;
-   level browser;
-   campaign/transitions;
-   enemies/bosses;
-   asset browser;
-   terrain/material/audio/animation editors;
-   runtime editor in Release;
-   Web/mobile/Pi editor.

### Phase A --- Inspection & selection/navigation architecture

Phase A must: 1. inspect M32 editor, camera, renderer, input, DebugUi
and LevelDefinition; 2. define exact editor camera ownership and
controls; 3. define cursor/mouse capture behavior; 4. define a focused
`EditorSelection` representation; 5. define the hierarchy item model
without generic scene/entity infrastructure; 6. inventory selectable
LevelDefinition objects and their picking proxies; 7. design screen-ray
construction and nearest-hit selection; 8. design selection
visualization; 9. define how selection maps into the existing M32
Inspector; 10. add focused math/picking tests where practical; 11. add
scaffolding only as appropriate; 12. not enable the full live visual
workflow until Phase B.

**Phase A outcome (complete):**

Camera. `editor::EditorCamera` is session tooling on `LevelEditorState`, not
`PlatformerCamera` and not `LevelDefinition.camera`. `render::CameraView` is
the project-owned view Renderer will consume in Phase B. First F2 seeds from
gameplay target+offset; later toggles keep the pose. Apply does not move the
editor camera. Exit will SnapToTarget the gameplay camera. Controls (not live):
RMB look, WASD, Q/E, Shift, wheel speed. Cursor stays visible.

Selection. `EditorSelection { EditorObjectKind, index }` is the only current
selection. Validated by `IsValidSelection`. Display names match the 21-entry
`kHierarchyEntries` table. Resolves against `workingCopy`. Apply/Revert/Save
keep identity. Selecting/navigating/picking do not mark Modified or Dirty.

Picking. Project-owned `Ray3`, slab AABB, inverse-rotated slope AABB,
`ScreenToWorldRay`, `PickNearest` (nearest positive t; exact ties keep earlier
hierarchy proxy). Moving platform and cyan box proxies take runtime pose via
`EditorPickingWorldState`. Camera is hierarchy-only. Runtime Player is not
selectable. Spawn uses `kPlayerVisualSize` (marker in Phase B). Empty click
clears. LMB picks, RMB looks. No Jolt raycasts.

Input. `editor::EditorInputState` polled by `PollEditorInput()` in the input
backend. `InputState` is unchanged. `DebugUi::WantsMouseCapture()` added
beside keyboard capture. `Window::Width/Height` expose the resizable viewport.

Not live: Application does not consume editor input, does not swap Renderer to
`CameraView`, does not world-pick, does not draw Hierarchy/highlight, and does
not route the Inspector by selection. `selectedPlatformIndex` remains the M32
geometry combo.

Tests. `EditorPickingTest` covers AABB (front/miss/inside/parallel/behind/
negative-dir), rotated slope hit/miss, nearer-candidate wins, hierarchy-order
ties, runtime-pose vs authored startX, Camera/Player exclusion, screen-to-world
center/right rays, seed-from-gameplay without mutating authored camera, and
selection validity/names/equality.

### Phase B --- Live visual editor

Implemented: editor navigation camera; Hierarchy; world picking; synchronized
selection; selection visualization; Inspector routing by selection; M32
editing for Spawn/Camera/Ground/Platforms; read-only inspection for other
selectable Level Format v1 objects; ImGui input capture; configuration
boundaries.

**Phase B outcome:** Development F2 opens Hierarchy + Inspector + Level Editor,
seeds the editor camera once per process, and renders from `CameraView`. RMB /
WASD / Q/E / Shift / wheel navigate without writing `LevelDefinition.camera`.
LMB world-picks via `ScreenToWorldRay` + `BuildPickingSet(active/applied
level, runtime poses)` + `PickNearest`. Unapplied working-copy transforms do
not move pick, highlight, or the spawn marker. Empty click clears. Camera is
hierarchy-only. Runtime Player is not selectable. Spawn uses a
Debug/Development `kPlayerVisualSize` marker matching the pick proxy.
Highlight uses `MakeHighlightRequest` from the same proxies.
`selectedPlatformIndex` is retired. Apply/Revert/Save preserve selection and
editor camera. Reopening F2 still resets `workingCopy = active`. Debug has
the visual editor but cannot save source. Release has none of this.

M33 remains incomplete until Phase C.

### Phase C --- Manual validation

Validate at minimum:

- F2 editor pause still works
- unapplied Inspector edits do not move pick/highlight/spawn marker
- clicking the visible (applied) location still selects; the unapplied location does not
- Apply then moves render, physics, pick and highlight together
- editor camera navigation (RMB, WASD, Q/E, Shift, wheel)
- editor camera session persistence across F2 close/reopen
- gameplay camera resume / SnapToTarget on exit
- Hierarchy content (21 authored selections)
- hierarchy selection (Ground, Platform 0, Platform 5, Slope 0, Hazard 0, Collectible 2, Goal, Camera)
- world picking (Ground, elevated platform, hazard, collectible, goal)
- nearest hit when proxies overlap
- empty-space click clears selection
- slope oriented picking
- moving platform runtime pick (frozen pose, not authored start)
- cyan box runtime pick
- spawn marker / pick
- Camera hierarchy-only (no world proxy)
- selection highlight follows the pick proxy
- hierarchy / world / Inspector sync
- UI mouse capture (click/scroll/type over panels)
- keyboard capture (InputFloat does not move camera; F2 ignored while typing)
- window resize picking alignment
- editable Inspector fields (Spawn, Camera, Ground, Platform)
- read-only Inspector fields (slope, moving platform, checkpoint, hazard, collectible, goal, dynamic box)
- Modified / Dirty semantics
- Apply preserves selection and editor camera
- Revert preserves selection and restores Inspector values
- Save preserves selection and editor camera
- BEST preservation
- Debug cannot save source
- Release has no editor
- unrelated-CWD Release/Development
- canonical source integrity (`game/assets/source/levels/level_01.level`)

### Acceptance criteria

M33 is complete only when: 1. editor mode provides useful independent 3D
navigation; 2. a visible hierarchy represents the authored objects of
Level 01; 3. one explicit editor selection state synchronizes hierarchy,
world and Inspector; 4. supported objects can be selected by clicking
them in the world; 5. nearest valid hit is selected; 6. selected objects
have a clear editor-only world highlight; 7. ImGui interaction cannot
accidentally navigate/select the world behind it; 8. M32 editable object
types remain editable through the selected-object Inspector; 9. other
M33 object types are inspectable read-only without expanding scope; 10.
Apply/Revert/Save semantics from M32 remain correct; 11. gameplay
simulation remains paused during editing; 12. gameplay camera authoring
is not modified by editor navigation; 13. Debug remains non-authoring;
14. Release remains editor-free; 15. Level Format v1 remains unchanged;
16. no generic scene/entity/reflection framework is introduced; 17. all
automated tests/builds pass; 18. Phase C manual validation passes; 19.
no commit/push/merge occurs before approval.

### Future direction

M33 establishes the selection and viewport foundation needed for later
milestones such as: - transform gizmos; - add/remove/duplicate
objects; - dynamic LevelDefinition collections; - New/Open/Save As; -
multiple levels/scenes; - richer gameplay object authoring; - enemy and
boss spawn authoring.

Those features are intentionally deferred until the M33
selection/navigation foundation is proven stable.


## Milestone 34 --- Visual Level Editor v3: Transform Gizmo + Persistent Editor Layout

### Status

Phase A complete: translation-gizmo math, pending-preview helpers, layout path
and ImGui ini ownership are implemented and tested. Phase B complete: live
axis drag, gizmo rendering, pending ghost, persistent layout, and Reset Editor
Layout. Phase C complete: overlay gizmo + canonical Level 01 restore. M34 is
**complete and merged**.

### Branch

`milestone/34-transform-gizmo-persistent-layout`

### Goal

Make the M33 visual editor materially faster to use by adding the first
direct-manipulation tool: a **world-space translation gizmo** for
supported authored objects.

M34 also adds **local persistence of the Dear ImGui editor window
layout**, so Metrics, Level Editor, Inspector and Hierarchy reopen where
the developer left them, with an explicit **Reset Editor Layout**
action.

The milestone preserves the established authoring contract:

``` text
Inspector / gizmo edits -> workingCopy
Viewport world          -> active/applied LevelDefinition
Apply Preview            -> commits workingCopy into visible/physical preview
Save Level Source        -> persists active/applied level source
```

### User experience target

``` text
F2
→ select Spawn, Ground or an Elevated Platform
→ X/Y/Z translation gizmo appears
→ drag one axis
→ Inspector updates immediately
→ Modified = true
→ active render/physics stay unchanged until Apply Preview
→ pending editor preview represents the working transform
→ Apply Preview moves render/physics/picking/highlight together
```

Window workflow:

``` text
Arrange Metrics / Level Editor / Inspector / Hierarchy
→ close application
→ reopen
→ positions/sizes restored

Reset Editor Layout
→ project default layout restored and persisted
```

### Translation Gizmo v1

#### Editable gizmo targets

Translation gizmo is available only for the existing positional
M33-editable world objects: - Player Spawn - Ground - Elevated Platform
0..5

Camera remains numerically editable in Inspector but has no world gizmo
because its authored data is gameplay-camera framing/offset rather than
a placed scene camera.

M33 read-only types remain read-only and receive no editable gizmo.

#### Modes

M34 implements **translation only**, world-space: - X - Y - Z

No rotation, scale, universal gizmo, snapping, or local/world mode
switching.

#### Working-copy contract

Gizmo manipulation edits the selected object's `workingCopy` position.

During drag: - workingCopy changes; - Inspector values change; -
Modified becomes true; - Dirty retains M32 semantics.

Normal active world rendering, physics, normal world picking and normal
M33 highlight do not move before Apply Preview.

#### Pending transform preview

Because active geometry remains unchanged before Apply, show a clear
editor-only pending/ghost representation at the working-copy transform
whenever the selected supported object's working transform differs from
active.

The active/applied object remains visible at its current position.

The gizmo follows the working-copy position.

The pending preview: - is editor-only; - does not affect physics; - is
visually distinguishable from active geometry; - does not replace the
active-world picking contract.

#### Selection semantics

Selection remains `EditorSelection { kind, index }`.

For a selected supported object: - Inspector → workingCopy; - gizmo
target → workingCopy; - pending preview → workingCopy; - normal world
picking/highlight → active/applied world.

No second selection system.

#### Interaction priority

Required conceptual priority:

``` text
ImGui interaction
    > active gizmo drag / gizmo handle
    > ordinary world picking
```

RMB editor-camera look must not manipulate the gizmo. A gizmo click must
not clear/change object selection. Dragging must remain constrained to
the chosen X/Y/Z axis.

Use focused project-owned ray/math helpers and robust behavior for
near-parallel camera/axis cases. Do not build a generic gizmo framework.

#### Inspector synchronization

Gizmo and Inspector are two interfaces over the same workingCopy.

Gizmo drag updates Inspector. Numeric Inspector edits reposition
gizmo/pending preview before Apply.

#### Apply/Revert/F2

Apply Preview: - commits/rebuilds as before; - active world moves to
working transform; - pending preview disappears when active ==
working; - selection preserved; - editor camera preserved; - gizmo
remains on selected object at applied position.

Revert Working Copy: - discards pending numeric/gizmo edits; - gizmo
returns to active position; - pending preview disappears; -
selection/editor camera preserved.

F2: - M33 rule remains: unapplied edits are discarded on reopen; - no
gizmo drag may remain latched across editor close.

### Persistent Editor Window Layout

#### Scope

Persist Dear ImGui layout for at least: - Metrics - Level Editor -
Inspector - Hierarchy

Use native Dear ImGui ini persistence where practical, including
position, size and naturally supported collapsed state.

#### Storage

Layout is developer-local tooling state.

Do not store it in: - LevelDefinition; - level_01.level; - BEST save; -
repository; - current working directory.

Preferred Windows path:

``` text
%LOCALAPPDATA%\Platformer3D\editor_layout.ini
```

Use existing platform/user-data boundaries where appropriate.

Development: persistence enabled. Debug: may use the same safe local
layout path. Release: no editor/layout persistence.

Do not allow Dear ImGui to create a default repository/CWD `imgui.ini`.

#### Default layout

Define a sensible project default arrangement for the current windows
while keeping the central 3D view useful.

Do not add docking.

#### Reset Editor Layout

Add a visible action:

`Reset Editor Layout`

It must restore the project-defined default positions/sizes and make
that layout the persisted layout for future launches.

An optional non-conflicting shortcut is allowed, but the visible UI
action is required.

It must not reset authored/gameplay data.

#### Robustness

Editor startup must remain safe when: - layout file is missing; - layout
file is malformed/unreadable; - application window size changes; -
stored positions are completely off-screen.

Use the smallest reasonable recovery behavior; no multi-monitor
workspace manager.

### Architecture constraints

Preserve: - C++20/CMake architecture; - gameplay never calls raylib
directly; - no editor state in LevelDefinition; - M33 single
selection; - M33 CPU world picking; - active-world pick/highlight
contract; - M32 Apply/Revert/Save semantics; - Development-only source
authoring; - Debug non-authoring; - Release editor-free; - BEST
independence; - portability boundaries.

Prefer a focused project-owned translation gizmo. Do not add a
third-party gizmo dependency without explicit approval.

### Phase A --- Architecture and math

Inspect the real M33 implementation and establish: 1. workingCopy
position access/mutation helpers; 2. exact gizmo-supported categories;
3. gizmo state ownership; 4. X/Y/Z handle geometry; 5. handle
hit-testing; 6. drag-start state; 7. constrained axis drag math; 8.
near-parallel stability policy; 9. ImGui/gizmo/world-pick/editor-camera
interaction priority; 10. pending-preview representation; 11.
active-vs-working rendering semantics; 12. Inspector/gizmo
synchronization; 13. Apply/Revert/F2 behavior; 14. focused gizmo math
tests; 15. ImGui ini persistence path; 16. default-layout mechanism; 17.
reset-layout mechanism; 18. missing/corrupt/off-screen recovery; 19.
Debug/Development/Release policy.

Phase A may implement focused math/state scaffolding and tests, but must
not enable the complete live gizmo workflow before review.

**Phase A outcome (complete):**

Gizmo. `editor::EditorAxis` / `GizmoInteractionState` on `LevelEditorState`.
`GetEditablePosition` mutates `workingCopy` for Spawn, Ground, Platform 0..5
only. Camera and M33 read-only kinds return null. Drag uses a camera-facing
plane containing the world axis; `|axis × viewForward| < 0.05` falls back to
closest-points; unusable rays keep the start pose. Position is
`start + (param - startParam) * axis` (no per-frame accumulation). Visual
length scales with distance/FOV and is clamped. Hit radius is 3× the visual
shaft fraction. `EditorGizmoTest` covers support, X/Y/Z isolation, continuity,
near-parallel finiteness, handle pick, pending preview vs active, and layout
path/defaults/clamp.

Layout. `IniFilename` is a `DebugUiBackend`-owned `std::string` at
`%LOCALAPPDATA%\Platformer3D\editor_layout.ini`. Debug and Development share it.
Release does not create or read it. First-run uses `ImGuiCond_FirstUseEver`
defaults (metrics/hierarchy left, inspector/level editor right). Reset UI is
Phase B (`ImGuiCond_Always` for one frame). No CWD `imgui.ini`.

Not live: Application does not hover/drag gizmo handles, Renderer does not
draw axes or a pending ghost, and there is no Reset Editor Layout button.

### Phase B --- Live implementation

**Phase B outcome (complete, awaiting Phase C):**

Live gizmo. After this-frame ImGui capture, `UpdateGizmoInteraction` hovers and
drags using Phase A `PickGizmoHandle` / constrained-axis math. Application
writes `workingCopy` only. Overlay order: active spawn/highlight, cyan pending
ghost, then X/Y/Z handles. RMB never starts a drag; active LMB drag suppresses
look. Apply/Revert/F2 clear gizmo state. Camera and M33 read-only kinds have no
gizmo and no pending world preview.

Layout. Reset Editor Layout in the Level Editor actions snaps Metrics,
Hierarchy, Inspector, and Level Editor (named `SetWindowPos` plus two frames of
`ImGuiCond_Always`) and `SaveIniSettingsToDisk`. One-shot off-screen clamp after
first Begin of Metrics and of the editor windows. Debug/Development share
`%LOCALAPPDATA%\Platformer3D\editor_layout.ini`. Release does not touch it.

### Phase C --- Manual validation

**Phase C correction:** gizmo X/Z handles were depth-tested `DrawLine3D`
geometry whose origin sits inside Ground/Platform/Spawn AABBs, so horizontal
axes disappeared. Renderer now draws the editor overlay *after* cooker probes,
uses a faint depth-tested pass then a depth-independent cylinder/cone overlay
(`rlDisableDepthTest` / `rlDisableDepthMask` / `rlDisableBackfaceCulling`), and
restores those states immediately. Handle picking is unchanged (world-space
shafts). Canonical `level_01.level` restored from M33 HEAD and recooked. M34
still awaits final approval. Milestone 35 has not started.

Validate at minimum:

#### Gizmo

-   Spawn, Ground and several Platforms show gizmo;
-   Camera has no world gizmo;
-   M33 read-only types have no editable gizmo;
-   X/Y/Z each change only their own coordinate;
-   multiple camera angles are stable;
-   gizmo clicks/drags do not world-pick another object;
-   RMB look does not drag gizmo;
-   ImGui interaction does not drag gizmo;
-   Inspector updates from gizmo;
-   numeric Inspector edits reposition gizmo/pending preview;
-   Modified/Dirty semantics remain correct;
-   active render/physics/picking/highlight remain unchanged before
    Apply;
-   pending preview represents working transform;
-   Apply moves render/physics/picking/highlight together;
-   pending preview disappears after Apply;
-   Apply preserves selection/editor camera;
-   Revert restores working transform;
-   F2 discards unapplied edits;
-   repeated drag/Apply cycles are stable.

#### Layout

-   arrange Metrics, Level Editor, Inspector and Hierarchy;
-   close and relaunch;
-   positions/sizes restore;
-   persistence path is outside repo/CWD;
-   unrelated-CWD launch restores layout;
-   Reset Editor Layout restores project defaults;
-   relaunch after reset keeps defaults;
-   missing layout file falls back safely;
-   malformed/unreadable layout does not crash;
-   resized window remains usable;
-   Debug policy works;
-   Release has no layout/editor behavior.

#### Regressions

-   M33 picking/navigation/selection;
-   Apply/Revert/Save;
-   source authoring;
-   BEST;
-   Level Format v1;
-   cooker;
-   unrelated CWD;
-   canonical Level 01 restored before closure.

### Acceptance criteria

M34 is complete only when: 1. Spawn, Ground and Elevated Platforms can
be translated with a world-space X/Y/Z gizmo. 2. Gizmo and Inspector
edit the same workingCopy. 3. Unapplied gizmo edits do not move active
render or physics. 4. Pending preview clearly represents working
transform. 5. Normal picking/highlight remain on active world before
Apply. 6. Apply moves render/physics/picking/highlight together. 7.
Revert/F2 preserve established semantics. 8. Gizmo does not conflict
with ImGui, world picking or editor camera. 9. M33 read-only types
remain read-only. 10. Camera remains numeric-only. 11. editor window
layout persists across launches. 12. persistence is developer-local and
CWD/repository independent. 13. Reset Editor Layout restores a project
default. 14. invalid/missing layout cannot prevent startup. 15. Release
remains editor-free. 16. Level Format v1 remains unchanged. 17.
LevelDefinition contains no editor state. 18. automated builds/tests
pass. 19. Phase C passes. 20. canonical gameplay values are restored
before Git closure. 21. no commit/push/merge occurs before approval.

### Explicitly out of scope

Do NOT implement: - rotation gizmo; - scale gizmo; - universal gizmo; -
snapping/grid snapping; - local/world mode switching; - pivot editing; -
object creation/deletion/duplication; - undo/redo; - copy/paste; -
multi-select; - box selection; - Level 02; - New/Open/Save As; - scene
graph/SceneManager/ECS/reflection; - generic Inspector/serialization; -
docking; - saved workspaces; - multi-monitor workspace management; -
enemies/bosses/combat; - asset browser; - Level Format v2; - save v2.

### Future direction

M34 establishes direct manipulation and local editor preferences. Later
milestones can build on it with rotation/scale where meaningful,
snapping, dynamic add/delete/duplicate, stable authored IDs when
required, and multiple levels/scenes.


## Milestone 35 — Visual Level Editor v4: Scale/Resize Gizmo + Orientation Widget + Precision Navigation

### Status
Phase B implemented; Phase C functional validation passed. One placement
correction: orientation widget moves to the upper-right of the 3D viewport
(left of the default Inspector column). Awaiting final approval.
M35 is **not** complete. Milestone 36 has not started.

### Branch
`milestone/35-scale-gizmo-orientation-navigation`

## Recommended Cursor model
**Grok 4.6 High — Fast OFF**

### Goal
Build on M34 without changing the authored-level architecture. M35 adds three focused editor-quality improvements:

1. **Visual resize/scale handles** for the existing box-shaped editable objects.
2. **Persistent top-left orientation widget** showing world X/Y/Z and allowing canonical editor-camera views.
3. **Precision editor navigation**, including selected-object keyboard nudge and modifier + mouse-wheel camera dolly.

M35 does **not** add/remove/duplicate objects and does not change Level Format v1.

### Architectural contract
The M32–M34 split remains mandatory:

- Inspector edits `workingCopy`.
- Translate and resize gizmos edit `workingCopy`.
- Pending preview represents `workingCopy`.
- Normal render, physics, normal world picking and M33 highlight use the active/applied `LevelDefinition`.
- `Apply Preview` commits working → active and rebuilds physics.
- `Save Level Source` writes active/applied data only.
- Selection, editor camera, gizmo mode and UI state are editor-only and never serialized into `LevelDefinition`.

### Part A — Resize/Scale Gizmo

#### Scope
Visual resize is available only for box geometry whose size is already authored/editable:

- Ground
- Elevated Platform 0..5

Player Spawn remains **translation-only** in M35. Camera and M33 read-only categories receive no resize handles.

#### Behavior
Add an explicit editor transform mode:

- Translate
- Resize

A compact visible UI control is required. Optional editor-only shortcuts may be added only if they do not conflict with camera/game controls.

Resize handles operate in world X/Y/Z. M35 edits authored **box size**, not a generic Transform scale. Do not introduce scale matrices or parent transforms.

Dragging an axis handle changes only the corresponding size component. The default resize policy should preserve the box center and expand/contract symmetrically around it. This keeps M35 small and avoids introducing face-anchor/pivot semantics prematurely.

All size components must remain finite and strictly positive. Define a small project-owned minimum dimension and clamp safely; no negative or zero-sized boxes.

The pending ghost must immediately reflect working center + working size. Active render/highlight/picking/physics remain unchanged until Apply.

#### Resize tests
Add focused tests for:

- Ground/Platform supported; Spawn/read-only unsupported.
- X resize changes only size.x.
- Y resize changes only size.y.
- Z resize changes only size.z.
- center remains unchanged.
- minimum-size clamp.
- no NaN/inf.
- representative camera angles.
- near-parallel stability.
- active-vs-working preview semantics.

### Part B — Orientation / View Widget

Add a small **screen-space orientation triad** in the upper-left area of the 3D viewport, positioned so it does not collide with the existing TIME/BEST HUD or editor windows.

It must:

- display X=red, Y=green, Z=blue consistently with the world gizmo;
- rotate/orient according to the editor camera;
- be editor-only;
- not be authored level data;
- remain visible regardless of world geometry/depth;
- remain below ImGui windows.

Make the widget clickable for canonical views if the implementation stays focused and robust:

- +X / Right
- -X / Left
- +Y / Top
- -Y / Bottom (optional if awkward for current platformer workflow)
- +Z / Front
- -Z / Back

Clicking a view direction changes **editor camera orientation only**. Preserve editor-camera position unless a focused orbit-distance policy is needed; do not alter gameplay camera authoring.

If clickable faces/axes become disproportionate in scope, Phase A must report that and propose the smallest subset before enabling it live.

### Part C — Selected-object keyboard nudge

Add precision translation for the currently selected object that already supports the translation gizmo:

- Player Spawn
- Ground
- Elevated Platform 0..5

Do not reuse bare WASD/QE because those belong to editor-camera movement.

Preferred policy: require a modifier plus directional keys, chosen after inspecting existing bindings and ImGui capture. The controls must be visible/documented in the editor UI.

Nudge edits `workingCopy` only and follows the same Modified/Dirty/Apply/Revert semantics as the translate gizmo.

Provide a normal increment and, if clean, a smaller precision increment with an additional modifier. No grid/snapping system is introduced.

Keyboard nudge must respect current-frame ImGui keyboard capture: typing in `InputFloat` must never move an object.

### Part D — Modifier + mouse-wheel camera dolly

M34 wheel behavior changes editor navigation speed. Preserve that behavior for unmodified wheel input.

Add a modifier + wheel gesture for **editor-camera dolly** along its forward vector:

- ordinary wheel → adjust editor navigation speed (M34 behavior)
- modifier + wheel → move editor camera forward/backward

Choose the modifier after checking conflicts. Dolly must not mutate gameplay camera authoring or `workingCopy`.

ImGui mouse capture must block both speed-wheel and dolly-wheel when the pointer is over UI.

Clamp/guard movement to avoid non-finite camera state or extreme single-step jumps.

### Hierarchy inventory
M34 identified four visible cooker probes outside the 21-entry Hierarchy:

- `textures/test_checker.png` quad
- `models/test_static.glb`
- `models/test_authored.glb`
- `models/test_textured.glb`

They remain technical/demo/render probes in M35. Do not add them to the authored Hierarchy and do not make them editable.

Player runtime, grid, editor overlays and HUD also remain outside authored Hierarchy for their existing reasons.

### Renderer / input boundaries
Renderer receives read-only draw requests. It does not own selection, transform mode, working data or interaction state.

Editor interaction priority remains:

1. current-frame ImGui capture;
2. active transform interaction / orientation widget;
3. normal world picking;
4. camera navigation where applicable.

A single click/gesture must not trigger multiple systems.

### Layout persistence
M34 persistent window layout remains unchanged. M35 must not regress:

- `%LOCALAPPDATA%\\Platformer3D\\editor_layout.ini`
- Metrics / Hierarchy / Inspector / Level Editor persistence
- Reset Editor Layout
- off-screen recovery
- Debug/Development sharing
- Release isolation
- unrelated-CWD behavior

The orientation widget itself is viewport overlay state and does not need its own persisted position in M35.

### Configurations

#### Debug
- visual editor
- translate + resize tools
- orientation widget
- precision navigation
- persistent editor layout
- source authoring unavailable

#### Development
Same as Debug plus existing source authoring.

#### Release
No ImGui editor, transform tools, orientation widget, editor layout interaction or editor navigation controls.

### Explicitly out of scope

- object add/delete/duplicate
- dynamic authored collections
- stable authored object IDs
- Level 02
- New/Open/Save As
- scene browser
- rotation gizmo
- arbitrary/local Transform scale
- local/world transform toggle
- pivot editing
- face-anchored resize
- grid snapping
- generic snapping system
- undo/redo
- copy/paste
- multiselect
- box select
- docking/workspaces
- scene graph / ECS / reflection
- generic Inspector/serialization
- adding cooker probes to Hierarchy
- gameplay/campaign/enemies/bosses/combat
- Level Format v2
- save v2
- Web/mobile/Raspberry Pi editor work

### Phase plan

#### Phase A — architecture/math/scaffolding

Inspect M34 live implementation; define resize math/state, transform-mode ownership, orientation-widget math/hit testing, nudge bindings and dolly behavior. Add focused no-window tests. Do not enable the complete live feature set yet.

**Phase A outcome (complete):**

Resize. `EditorTransformMode` on `LevelEditorState` (default Translate, unused by Application). `GetEditableSize` / `IsResizeSelection` for Ground and Platform 0..5. `GizmoInteractionState` stores `dragStartSize` and `dragHandleSign`. Handles at `± gizmoLength` cubes. `newSize = start + 2 * sign * axisDelta`, clamp `kMinAuthoredBoxExtent = 0.12`. M34 translation solver unchanged. `UpdateResizeInteraction` exists for tests; Application still calls only `UpdateGizmoInteraction`.

Orientation. `ProjectOrientationWidgetAxes`, layout `(364, 28)`, canonical Front/Back/Right/Left/Top. Position-preserving yaw/pitch. 2D tip hit test. Not drawn.

Nudge. Ctrl+arrows/PageUp/PageDown, Ctrl+Shift precision 0.01 vs 0.10. Polled into `EditorInputState`, not applied.

Dolly. `ApplyEditorCameraDolly`; Alt+wheel fills `dollyWheelDelta`. `UpdateEditorCamera` still uses ordinary wheel for speed.

Tests: `EditorGizmoTest` resize/nudge cases; new `EditorOrientationTest`.

#### Phase B — live integration

**Phase B outcome (implemented, awaiting Phase C):**

Translate/Resize radios in Level Editor (locked during drag). Default Translate, session-only. `UpdateResizeInteraction` live for Resize; `UpdateGizmoInteraction` for Translate. Six cube handles, depth-independent overlay, ghost is true size. Spawn in Resize: no handles, UI hint. Orientation widget drawn screen-space before ImGui; clickable Front/Back/Left/Right/Top. Nudge live in Translate only (`NudgeAllowed`). `ResolveEditorWheel` makes Alt+wheel exclusive dolly. ImGui capture remains current-frame.

#### Phase C — manual validation
Manually validate resize from multiple camera angles, minimum-size safety, working-vs-active semantics, Apply/Revert/F2, orientation widget and canonical views, nudge/input capture, dolly/speed wheel separation, layout regression, Debug/Release boundaries, unrelated CWD and canonical level-source integrity.

**Phase C correction (placement only):** the orientation widget uses adaptive upper-right layout (`originX = viewportWidth - defaultInspectorColumn - radius - 24`, `originY = 64 + radius`) so it sits left of the default Inspector column, ~100 px from the top on a 1280×720 view. Hit-test, canonical views, and interaction priority are unchanged. Not persisted. Awaiting final approval.

### Acceptance criteria
M35 is complete only when:

- Ground and Platforms can be resized visually on X/Y/Z.
- Resize modifies authored size in `workingCopy`, with center preserved.
- Pending ghost shows resize before Apply.
- Active world/physics/pick/highlight remain applied until Apply.
- Minimum-size and finite-value safety work.
- Translate M34 remains correct.
- Orientation widget is readable and correctly follows editor-camera orientation.
- Approved canonical view clicks work without touching gameplay camera authoring.
- Keyboard nudge works only for translation-editable selections and respects ImGui capture.
- Modifier+wheel dolly and ordinary wheel speed adjustment coexist predictably.
- M34 persistent layout remains correct.
- Debug/Development/Release boundaries remain correct.
- Level Format v1, BEST and canonical source remain unchanged except intentional source authoring after explicit Save.
- All automated tests/builds/cooker regressions pass.
- Phase C manual validation passes.
- No M36 work has started.


## Milestone 36 --- Editor Menu Bar & Workspace Controls

### Status

Phase B live integration implemented: F2 Dear ImGui main menu bar (View /
Transform / Level) shares workspace visibility, `transformMode`, and
`LevelEditorRequest` with the existing panels. Automated tests/builds are
part of Phase B. Milestone 36 is **not** complete until Phase C manual
validation. Cooker/build integration is deferred. Milestone 37 has not
started.

### Branch

`milestone/36-editor-menu-bar-workspace-controls`

### Recommended Cursor model

**Grok 4.6 High --- Fast OFF**

------------------------------------------------------------------------

### 1. Goal

Consolidate the Development/Debug visual editor controls introduced in
M32--M35 into a clear, persistent top editor menu bar without changing
the authored level model, gameplay, physics, level format, or existing
editor semantics.

M36 is an **editor UX consolidation milestone**, not a new
authoring-capability milestone.

When F2 editor mode is active, a top menu bar provides centralized
access to:

-   editor panel visibility;
-   transform mode selection;
-   existing level actions;
-   editor layout reset.

The existing M32--M35 behavior remains authoritative. The menu bar must
call the same state/actions rather than introducing parallel
implementations.

------------------------------------------------------------------------

### 2. Why M36 exists

The editor now has multiple independent surfaces:

-   Platformer3D Metrics;
-   Hierarchy;
-   Inspector;
-   Level Editor;
-   Translate / Resize;
-   Apply Preview;
-   Revert Working Copy;
-   Save Level Source;
-   Reset Editor Layout.

These controls are useful but increasingly distributed across panels.
Before adding structurally larger features such as object
creation/removal, multiple levels, rotation, undo/redo, or external
build tooling, M36 creates a stable editor workspace shell.

This keeps the editor evolution incremental and reduces future UI
duplication.

------------------------------------------------------------------------

### 3. In scope

#### 3.1 Top editor menu bar

Visible only while the F2 editor is active.

Required top-level menus:

-   `View`
-   `Transform`
-   `Level`

The menu bar is editor UI, not gameplay HUD.

#### 3.2 View menu

Provide checkable visibility controls for exactly these existing
windows:

-   `Metrics`
-   `Hierarchy`
-   `Inspector`
-   `Level Editor`

Each menu item manipulates the same authoritative visibility state used
to decide whether the corresponding window is drawn.

No duplicate visibility state is allowed.

#### 3.3 F1 integration

F1 remains the Metrics shortcut.

F1 and `View > Metrics` must modify the **same Metrics visibility
state**.

Examples:

-   Metrics visible → F1 hides it → menu becomes unchecked.
-   Metrics hidden → `View > Metrics` shows it → F1 state follows.

#### 3.4 Transform menu

Expose the existing M35 transform mode:

-   `Translate`
-   `Resize`

The menu must use the existing `EditorTransformMode`.

No second transform-mode state.

The existing transform controls inside the Level Editor panel may remain
during M36 unless removing them is clearly simpler and regression-free.
If both surfaces exist, they must stay synchronized.

#### 3.5 Level menu

Expose the existing actions:

-   `Apply Preview`
-   `Revert Working Copy`
-   `Save Level Source`
-   separator
-   `Reset Editor Layout`

These must invoke the existing M32--M35 action/request paths.

Do not duplicate Apply, Revert, Save, or Reset logic inside menu code.

#### 3.6 Enable/disable semantics

Menu actions must respect the same rules as their existing panel
equivalents.

At minimum:

-   `Apply Preview`: enabled only when the current existing Apply
    semantics allow it.
-   `Revert Working Copy`: enabled only when the current existing Revert
    semantics allow it.
-   `Save Level Source`: Development authoring only and subject to the
    existing Save rules.
-   Debug must not gain source authoring.
-   `Reset Editor Layout`: available in Debug and Development editor
    builds.

#### 3.7 Panel close buttons

Hierarchy, Inspector, Level Editor, and Metrics should support normal
ImGui close behavior where practical.

Closing a window through its close button must update the same
visibility state represented by `View`.

A panel hidden this way must be recoverable through the menu bar.

#### 3.8 Editor reopen behavior

F2 closes editor mode as before.

When reopening F2 during the same process:

-   existing editor camera session behavior remains unchanged;
-   selection behavior remains unchanged;
-   transform mode session behavior remains unchanged;
-   panel visibility should remain as the user last set it during that
    process.

Do not serialize panel visibility into `LevelDefinition`.

#### 3.9 Layout persistence

Continue using the existing M34 `editor_layout.ini` for ImGui window
geometry/collapse state.

M36 may persist panel open/closed visibility in the same
editor-preference domain **only if Dear ImGui naturally provides a
reliable solution without custom level/save data**.

Preferred M36 rule:

-   position/size/collapse: existing ImGui INI behavior;
-   panel visibility: session state.

Do not create a new general settings format merely for M36.

#### 3.10 Reset Editor Layout

`Reset Editor Layout` must continue to reset the four known editor
windows to the project defaults.

It must **not**:

-   mutate the active level;
-   mutate `workingCopy`;
-   change Modified/Dirty;
-   change BEST;
-   change selection;
-   change editor camera;
-   invoke Apply/Revert/Save;
-   invoke cooker/build tools.

Panel visibility after Reset should be deterministic. Preferred
behavior: restore all four editor panels visible.

#### 3.11 Menu bar layout impact

The top menu bar consumes screen space.

Update viewport/editor overlay placement where necessary so that:

-   the orientation widget does not collide with the menu bar;
-   editor 3D interaction remains aligned with the actual viewport;
-   mouse-ray picking remains correct;
-   gizmo hit testing remains correct;
-   ImGui capture remains authoritative.

Do not casually offset camera projection/picking without tracing the
actual raylib/ImGui coordinate relationship.

#### 3.12 Orientation widget

The M35 orientation widget remains viewport UI.

It must remain:

-   upper-right;
-   left of the default Inspector region;
-   below the menu bar with comfortable spacing;
-   adaptive to window resize;
-   non-persisted.

Its Front/Back/Left/Right/Top behavior is unchanged.

------------------------------------------------------------------------

### 4. Out of scope

M36 must **not** implement:

-   Cooker execution from the editor;
-   CMake/build execution from the editor;
-   Build Debug/Development/Release/All;
-   external process launching;
-   build console/log window;
-   Add Object;
-   Delete Object;
-   Duplicate Object;
-   dynamic object counts;
-   Level Format v2;
-   new level/scene creation;
-   Open/Save As;
-   multiple levels;
-   rotation gizmo;
-   generic scale system beyond M35 Resize;
-   snap;
-   undo/redo;
-   copy/paste;
-   docking;
-   SceneManager;
-   ECS/reflection;
-   asset browser;
-   runtime player authoring;
-   new gameplay systems.

External Cooker/build integration is intentionally deferred to a later
dedicated milestone because process execution/build orchestration is
architecturally different from editor workspace UI.

------------------------------------------------------------------------

### 5. Architecture rules

#### 5.1 One source of truth

Menu controls must route into existing editor state/actions.

Examples:

`View > Inspector` → existing Inspector visibility state.

`Transform > Resize` → existing `EditorTransformMode::Resize`.

`Level > Apply Preview` → existing Apply request path.

No menu-specific Apply implementation.

#### 5.2 UI requests, Application ownership

Preserve the existing M32 pattern where UI produces requests and
`Application` executes operations that own gameplay/physics/runtime
consequences after the frame at the appropriate point.

Do not let ImGui menu code directly rebuild physics or replace the
active level.

#### 5.3 Renderer boundaries

Renderer remains unaware of editor menu semantics.

Renderer may receive only the same read-only overlay requests/camera
data needed for rendering.

#### 5.4 Gameplay isolation

Gameplay input/state must not depend on menu bar state.

F2 remains the semantic editor toggle boundary.

Release gameplay remains unchanged.

#### 5.5 Configurations

**Debug** - F2 editor available; - menu bar available; - visual editor
tools available; - source authoring Save unavailable.

**Development** - full M36 menu bar; - existing source authoring Save
available under existing rules.

**Release** - no editor menu bar; - no ImGui editor; - F2 remains
no-op; - no editor layout behavior added.

------------------------------------------------------------------------

### 6. Preferred menu structure

``` text
View            Transform          Level
├─ [✓] Metrics  ├─ ● Translate     ├─ Apply Preview
├─ [✓] Hierarchy└─ ○ Resize        ├─ Revert Working Copy
├─ [✓] Inspector                   ├─ Save Level Source
└─ [✓] Level Editor                ├────────────────────
                                    └─ Reset Editor Layout
```

Exact spacing is not important. Semantics are.

------------------------------------------------------------------------

### 7. Menu bar behavior

-   Menu bar appears when F2 editor mode is active.
-   Menu bar disappears when F2 editor mode is inactive.
-   It should span the usable top area naturally.
-   It must not create a second OS window.
-   It must not require docking.
-   ImGui interaction with the menu bar must block world picking/gizmo
    interaction beneath it.
-   Clicking a menu item must never also select a world object.
-   Keyboard navigation/focus inside ImGui must continue to respect
    capture.

------------------------------------------------------------------------

### 8. Panel visibility model

Introduce the minimum project-owned editor workspace state needed, for
example conceptually:

``` cpp
struct EditorWorkspaceState
{
    bool showMetrics = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showLevelEditor = true;
};
```

The exact type/location should follow the existing code ownership
discovered in Phase A.

Do not blindly add this exact struct if the project already has a
cleaner authoritative location.

The important invariant is one visibility value per panel.

------------------------------------------------------------------------

### 9. Existing panel compatibility

#### Metrics

-   F1 still works.
-   `View > Metrics` stays synchronized.
-   Reset Layout can restore visibility.

#### Hierarchy

-   hidden panel must not affect selection ownership.
-   selected object remains selected while Hierarchy is hidden.

#### Inspector

-   hiding Inspector must not mutate `workingCopy`.
-   selection remains valid.

#### Level Editor

-   hiding it must not change transform mode.
-   hiding it must not Apply/Revert/Save.
-   menu actions remain available while the panel is hidden.

------------------------------------------------------------------------

### 10. Transform compatibility

Switching Translate/Resize from the menu must behave exactly like
switching from the existing M35 controls.

It must not:

-   Apply;
-   Revert;
-   clear Modified;
-   change Dirty;
-   change selection;
-   change editor camera.

If a gizmo drag is active when a transform-mode change is requested, use
the existing safe interaction policy. Do not redirect an active drag
into another mode.

------------------------------------------------------------------------

### 11. Level action compatibility

#### Apply Preview

Preserve: - validation; - physics rebuild; - active/working semantics; -
selection; - editor camera; - BEST isolation.

#### Revert Working Copy

Preserve: - working = active; - gizmo interaction cleanup; -
selection/camera preservation.

#### Save Level Source

Preserve: - Development-only authoring path; - active/applied definition
only; - disabled while current existing Save semantics disallow it; -
deterministic writer; - safe replacement; - no automatic
cooker/build/restart.

#### Reset Editor Layout

Preserve M34 behavior plus deterministic panel visibility restoration.

------------------------------------------------------------------------

### 12. Tests

Add focused no-window tests where useful for project-owned
workspace/menu decision logic.

At minimum preserve and rerun:

-   `EditorGizmoTest`
-   `EditorOrientationTest`
-   `EditorPickingTest`
-   `LevelFileTest`
-   `PhysicsRebuildTest`
-   cooker tests

Do not weaken M34/M35 tests.

If workspace visibility/action-enable logic is extracted into
project-owned pure helpers, add a focused test for:

-   default panel visibility;
-   F1/menu Metrics synchronization;
-   transform mode synchronization;
-   Debug vs Development Save availability;
-   Reset restoring default visibility without changing authored state.

Do not attempt to unit-test Dear ImGui itself.

------------------------------------------------------------------------

### 13. Manual validation

Phase C must manually validate at least:

1.  F2 shows menu bar.
2.  F2 hides menu bar.
3.  View toggles each of four panels independently.
4.  Closing a panel updates View check state.
5.  Hidden panel can be restored from View.
6.  F1 and View \> Metrics remain synchronized.
7.  Selection survives hiding/showing Hierarchy.
8.  Working edits survive hiding/showing Inspector.
9.  Transform mode survives hiding/showing Level Editor.
10. Translate menu selection works.
11. Resize menu selection works.
12. Existing panel transform controls, if retained, synchronize with
    menu.
13. Apply from menu works.
14. Revert from menu works.
15. Save from menu works in Development.
16. Save is unavailable/disabled in Debug.
17. Reset Layout from menu resets all four windows.
18. Reset restores deterministic panel visibility.
19. Reset does not change level/editor gameplay state.
20. Menu clicks do not world-pick.
21. Menu interaction does not start gizmo drag.
22. Orientation widget remains correctly placed below/right of menu bar.
23. Orientation widget remains clickable.
24. Resize/Translate gizmos remain correctly pickable.
25. Window resize preserves menu/widget/picking alignment.
26. layout persistence regression.
27. unrelated-CWD regression.
28. Debug editor regression.
29. Release has no menu/editor.
30. canonical `level_01.level` remains clean.

------------------------------------------------------------------------

### 14. Phase plan

#### Phase A --- Audit & workspace architecture

-   inspect current DebugUi/LevelEditor/Metrics ownership;
-   trace F1/F2 state;
-   trace M35 transform state;
-   trace LevelEditorRequest Apply/Revert/Save/Reset;
-   design one-source-of-truth workspace visibility;
-   design menu request routing;
-   identify menu-bar impact on viewport/orientation-widget coordinates;
-   add pure helpers/tests where useful;
-   do not wire the live menu yet.

#### Phase B --- Live menu integration

-   render top menu bar in Debug/Development F2 mode;
-   wire View;
-   wire F1 synchronization;
-   wire Transform;
-   wire Level actions through existing requests;
-   support panel close/reopen;
-   update Reset behavior;
-   adjust orientation widget placement for menu bar;
-   preserve capture/picking/gizmo behavior;
-   builds/tests/smokes.

#### Phase C --- Manual validation

-   full UX validation checklist above;
-   correct only M36 regressions;
-   restore canonical level source;
-   final builds/tests;
-   no commit until approval.

------------------------------------------------------------------------

### 15. Completion criteria

M36 is complete only when:

-   top menu bar is live in Debug/Development editor mode;
-   all four panel visibility toggles work from one authoritative state;
-   F1 Metrics synchronization works;
-   Translate/Resize menu controls use the existing transform mode;
-   Apply/Revert/Save/Reset use existing action paths;
-   Debug/Development authoring boundaries remain correct;
-   menu interaction cannot leak into world picking/gizmos;
-   orientation widget and picking remain correct after menu-bar
    introduction;
-   Release remains editor-free;
-   all tests/builds/cooker regressions pass;
-   manual Phase C passes;
-   canonical Level 01 source is clean;
-   M37 has not started.

------------------------------------------------------------------------

### 16. Deferred backlog

Not part of M36, but preserved for future milestones:

-   editor tool runner;
-   Cook Assets;
-   Build Debug;
-   Build Development;
-   Build Release;
-   Build All;
-   build/cooker output console;
-   object Add/Delete/Duplicate;
-   dynamic authored collections;
-   multiple levels/scenes;
-   New/Open/Save As;
-   rotation;
-   snap;
-   undo/redo;
-   focus/frame selected.

These must not be opportunistically implemented during M36.


## Milestone 37 --- Editor Tool Runner: Asset Cooker & Build Integration

### Status

Implementation, Phase C, and Git closure are complete. Milestone 37 is
merged. Milestone 38 has started on `milestone/38-runtime-asset-staging`.

### Branch

`milestone/37-editor-tool-runner`

### Recommended Cursor model

**Grok 4.6 High --- Fast OFF**

------------------------------------------------------------------------

### 1. Goal

Add a Development-only editor tool runner that can launch the project's
existing asset cooker and CMake build commands as **external child
processes**, while keeping all build/cooker logic outside gameplay and
preserving the existing command-line workflow as canonical.

M37 adds the infrastructure and UI needed to run:

-   `Cook Assets`
-   `Build Debug`
-   `Build Development`
-   `Build Release`
-   `Build All`

from the editor.

The editor does **not** reimplement CMake, Python, the cooker, or build
logic. It only launches the already-established project commands,
captures their output, reports status, and prevents unsafe concurrent
execution.

------------------------------------------------------------------------

### 2. Why M37 exists

M36 established a stable editor menu/workspace shell. M37 is the next
tooling layer: frequently used project commands can be invoked from the
Development editor without replacing the command-line workflow.

This milestone deliberately separates:

1.  editor UI;
2.  process execution;
3.  project command definitions;
4.  gameplay/runtime systems.

The resulting process-runner boundary should remain portable enough for
a future Linux development host without introducing platform-specific
process APIs into gameplay or generic editor logic.

------------------------------------------------------------------------

### 3. Core principles

#### 3.1 CLI remains canonical

The existing Python/CMake commands remain the source of truth.

The editor must launch them; it must not reproduce their behavior in
C++.

#### 3.2 External process boundary

Tool execution must live behind an editor/tooling process abstraction.

Windows-specific process APIs, if required, must be isolated in a
platform implementation.

#### 3.3 Development-only execution

Actual Cooker/build launching from the editor is **Development-only**
for M37.

Debug may keep the M36 editor/menu but must not launch project tool
processes.

Release remains editor-free.

#### 3.4 One active tool job

M37 supports at most one editor tool process/job at a time.

While a job is running, commands that would start another job are
disabled.

No parallel builds/cooks in M37.

#### 3.5 No self-rebuild assumption

A running Development executable may be locked on Windows.

M37 must not assume that `Build Development` can always replace/relink
the executable currently running the editor.

This condition must be detected/reported cleanly as a build failure or
prevented if the architecture can determine it safely.

Do not invent dangerous self-restart/hot-reload behavior.

------------------------------------------------------------------------

### 4. In scope

#### 4.1 New Build menu

Extend the M36 menu bar with:

`Build`

Required items:

-   `Cook Assets`
-   separator
-   `Build Debug`
-   `Build Development`
-   `Build Release`
-   `Build All`
-   separator
-   `Tool Output`

Exact wording may be adjusted slightly if required by existing UI
conventions, but semantics must remain explicit.

#### 4.2 Tool Output window

Add a Development editor window that displays:

-   current/last command label;
-   state: Idle / Running / Succeeded / Failed;
-   exit code when available;
-   elapsed time;
-   captured stdout/stderr text;
-   auto-scroll behavior suitable for build logs;
-   a clear-log action when not destructive to a running process.

The output window must be recoverable from the editor UI.

#### 4.3 Process execution abstraction

Introduce a small editor/tooling abstraction responsible for:

-   launching a child process;
-   setting a working directory;
-   capturing stdout;
-   capturing stderr;
-   non-blocking status polling from the game/editor loop;
-   obtaining exit code;
-   safe cleanup;
-   preventing leaked handles/process objects.

Do not block the render/game loop waiting for a build.

#### 4.4 Repository-root safety

Tool commands must execute relative to the resolved project/repository
root, not the user's current working directory.

Launching the game from `%TEMP%` must not cause cooker/build commands to
target `%TEMP%`.

Reuse existing authoring/runtime path knowledge where appropriate, but
do not hard-code a developer-specific absolute path.

#### 4.5 Cook Assets

`Cook Assets` must launch the project's existing cooker entry
point/command.

Phase A must inspect the repository and identify the canonical command
before implementation.

The editor must not embed cooker logic.

#### 4.6 Build commands

Use the existing CMake presets:

-   Debug → `windows-debug`
-   Development → `windows-development`
-   Release → `windows-release`

Use the existing configure preset if required by the current workflow:

-   `windows-vs2022`

Do not silently redesign the CMake preset structure.

#### 4.7 Build All semantics

`Build All` means:

1.  Build Debug
2.  Build Development
3.  Build Release

in a deterministic sequence.

It does **not** mean that `Build Release` implicitly builds all
configurations.

If one step fails, preferred M37 behavior is to stop the sequence and
report which step failed.

#### 4.8 Command status

Required conceptual states:

-   Idle
-   Running
-   Succeeded
-   Failed

Optional internal states such as Starting may be used if genuinely
useful.

#### 4.9 UI responsiveness

While a process runs:

-   rendering continues;
-   editor camera remains responsive;
-   panels remain responsive;
-   output updates incrementally;
-   the application must not freeze waiting for process completion.

#### 4.10 Output bounds

Build output can become large.

Implement a reasonable bounded log strategy so a long tool run cannot
grow editor memory without limit.

The exact cap should be documented and deterministic.

------------------------------------------------------------------------

### 5. Configuration policy

#### Development

Full M37 functionality:

-   Build menu visible;
-   Cooker execution enabled;
-   Build execution enabled;
-   Tool Output available.

#### Debug

M36 editor remains available, but external project tool execution is
unavailable in M37.

Preferred UX:

-   Build menu may be absent, or commands clearly disabled.

Choose one consistent policy in Phase A and document it.

Debug must not accidentally become an authoring/build host.

#### Release

No editor, no menu bar, no tool runner, no process execution code path.

------------------------------------------------------------------------

### 6. Process architecture

Preferred conceptual separation:

``` text
Editor menu/UI
    ↓ semantic request
EditorToolRunner
    ↓ command/job description
Platform process layer
    ↓
OS child process
```

Potential names are illustrative, not mandatory:

-   `EditorToolRunner`
-   `EditorToolJob`
-   `EditorToolProcess`
-   `PlatformProcess`
-   `WindowsProcess`

Use the smallest architecture that cleanly isolates OS process details.

Do not introduce a generic engine-wide job system.

------------------------------------------------------------------------

### 7. Command representation

A tool job should conceptually describe:

-   display name;
-   executable;
-   argument list;
-   working directory;
-   optional sequence step metadata.

Avoid building one large shell command string if a direct executable +
arguments API is practical.

Do not pass untrusted level/game data into a shell.

M37 commands are fixed project tooling commands.

------------------------------------------------------------------------

### 8. Windows implementation

The initial implementation targets Windows.

If Win32 APIs are used:

-   isolate them from gameplay;
-   avoid handle leaks;
-   capture stdout/stderr through pipes;
-   avoid blocking reads on the main thread;
-   correctly quote executable/arguments;
-   use Unicode-safe APIs where practical;
-   close process/thread/pipe handles deterministically.

Do not scatter `CreateProcess` calls through editor UI code.

Future Linux support should require replacing/adding a platform process
backend rather than rewriting menu logic.

------------------------------------------------------------------------

### 9. Non-blocking execution

The main editor frame must never perform a blocking wait for tool
completion.

Acceptable designs include:

-   background reader thread plus main-thread polling;
-   carefully implemented non-blocking pipe polling;
-   another minimal architecture justified by the current codebase.

Threading, if used, must have clear ownership and shutdown.

Application exit while a tool is running must be handled deliberately.

M37 must not leave orphaned reader threads or invalid handles.

------------------------------------------------------------------------

### 10. Process termination policy

M37 does not require a user-facing `Cancel Build` feature unless the
implementation makes it trivial and safe.

However, application shutdown with a running tool must be deterministic.

Phase A must choose and document whether shutdown:

-   waits briefly and terminates the child;
-   detaches intentionally;
-   or uses another safe policy.

Do not silently leak a child process.

------------------------------------------------------------------------

### 11. Tool Output window behavior

The Tool Output window is editor/tooling state, not level state.

It must not affect:

-   `LevelDefinition`;
-   workingCopy;
-   active level;
-   Modified;
-   Dirty;
-   BEST;
-   selection;
-   editor camera;
-   physics.

Suggested controls:

-   status line;
-   command/job name;
-   elapsed time;
-   exit code;
-   scrollable log;
-   `Clear` when appropriate.

No terminal emulator is required.

No command prompt text input is required.

Users cannot enter arbitrary shell commands in M37.

------------------------------------------------------------------------

### 12. Workspace integration

Extend M36 workspace state with Tool Output visibility if appropriate.

`View` should expose `Tool Output` if that matches the cleanest M36
pattern.

Alternatively, `Build > Tool Output` may toggle/open it, but there must
be a reliable recovery path after closing the window.

Preferred design:

-   `View > Tool Output`
-   `Build > Tool Output` may additionally focus/open the same window.

One authoritative visibility state only.

------------------------------------------------------------------------

### 13. Build menu enable rules

When no tool is running:

-   Cook Assets: enabled in Development.
-   Build Debug: enabled in Development.
-   Build Development: enabled in Development, subject to the self-build
    policy.
-   Build Release: enabled in Development.
-   Build All: enabled in Development, subject to the self-build policy.

When a tool is running:

-   all job-start commands disabled;
-   Tool Output remains usable;
-   existing editor Level/View/Transform functionality remains usable.

No overlapping tool jobs.

------------------------------------------------------------------------

### 14. Build Development / self-lock handling

Windows may prevent replacing the currently running Development
executable.

Phase A must inspect the actual build output layout and determine the
real risk.

M37 must choose a safe explicit policy.

Acceptable examples:

#### Policy A --- Allow and report

Launch the normal Development build and display the linker/file-lock
failure clearly.

#### Policy B --- Disable unsafe self-build

Disable `Build Development` while running the Development executable if
the build would necessarily overwrite it.

If Build All includes Development, the same policy must be reflected
consistently.

Do not create automatic executable swapping, self-restart, hot reload,
or launcher architecture in M37.

------------------------------------------------------------------------

### 15. Configure policy

Determine whether build commands should:

-   run only `cmake --build --preset ...`, assuming configured build
    tree;
-   or perform a configure step when necessary.

Do not run configure every frame or hide expensive work unnecessarily.

Preferred behavior: preserve the project's current explicit preset
workflow and report missing/not-configured errors clearly unless a small
deterministic preflight is justified.

Phase A must inspect and decide.

------------------------------------------------------------------------

### 16. Cooker behavior

The existing cooker remains authoritative.

M37 must:

-   launch it from repository root;
-   capture its normal output;
-   report exit code;
-   show success/failure;
-   leave source assets untouched except for the normal cooker-defined
    outputs.

No C++ asset conversion implementation.

No new pack format.

------------------------------------------------------------------------

### 17. Build success/failure

A process is successful only if its exit code indicates success.

Do not infer success merely because output contains a word such as
"success".

For Build All:

-   each step has its own label;
-   final status succeeds only if all required steps succeed;
-   failure identifies the failed step.

------------------------------------------------------------------------

### 18. Existing editor regression contract

M37 must preserve all M36 behavior:

-   F1 Metrics;
-   F2 editor;
-   View menu;
-   Transform menu;
-   Level menu;
-   panel close/reopen;
-   Reset Editor Layout;
-   orientation widget;
-   picking;
-   Translate;
-   Resize;
-   nudge;
-   dolly;
-   Development source Save;
-   Debug no source Save;
-   Release editor-free.

Tool running must not hijack these systems.

------------------------------------------------------------------------

### 19. Out of scope

M37 must **not** implement:

-   arbitrary shell/terminal input;
-   terminal emulator;
-   interactive stdin to child processes;
-   Cancel Build unless trivial and explicitly approved during
    implementation;
-   hot reload;
-   executable self-restart;
-   launcher application;
-   automatic game restart after build;
-   automatic source Save before cooker;
-   automatic Apply before cooker;
-   automatic Cooker before every build;
-   automatic Build after every Save;
-   file watching;
-   background continuous build;
-   parallel builds;
-   remote build;
-   Linux process backend unless required for compile hygiene;
-   Android build;
-   iOS build;
-   Web/Emscripten build;
-   deployment;
-   packaging/installer;
-   Add/Delete/Duplicate objects;
-   Level Format v2;
-   rotation gizmo;
-   undo/redo;
-   multiple levels/scenes.

------------------------------------------------------------------------

### 20. Testing strategy

Process launching is testable without launching the full game.

Add focused tests for project-owned logic where practical:

-   job state transitions;
-   only one active job;
-   command construction;
-   repository-root working directory;
-   exit code success/failure;
-   bounded log behavior;
-   Build All sequencing;
-   stop-on-failure sequencing;
-   self-build policy;
-   shutdown/cleanup logic where testable.

A small deterministic helper child process/test command may be used for
process-capture tests if it does not depend on network access.

Do not make unit tests compile the entire project repeatedly.

------------------------------------------------------------------------

### 21. Manual validation

Phase C must validate at least:

1.  Development Build menu visible.
2.  Debug execution policy matches Phase A decision.
3.  Release has no tool UI.
4.  Tool Output opens/closes/restores.
5.  Cook Assets launches.
6.  Cooker output appears incrementally.
7.  Cooker exit code/status correct.
8.  Build Debug launches.
9.  Build Debug output appears.
10. Build Debug success/failure correctly reported.
11. Build Release launches.
12. Build Release output appears.
13. Build Development follows approved self-build policy.
14. Build All follows approved sequence.
15. Build All stops on failure if a step fails.
16. second tool cannot start while one is running.
17. editor remains responsive while tool runs.
18. menu/panels remain usable while tool runs.
19. world picking/gizmos remain unaffected.
20. tool commands work when game was launched from unrelated CWD.
21. command working directory is repository root.
22. no arbitrary shell input exists.
23. no unexpected source level modification.
24. application shutdown with tool state is safe.
25. M36 regressions pass.

------------------------------------------------------------------------

### 22. Phase plan

#### Phase A --- Audit, process abstraction & tests

-   inspect canonical cooker command;
-   inspect CMake presets/build layout;
-   inspect Development executable lock/self-build risk;
-   define Development-only tool policy;
-   design tool job/state model;
-   design process abstraction;
-   define repository-root resolution;
-   define output capture/bounds;
-   define Build All sequence;
-   define shutdown policy;
-   implement/test process layer and pure job logic where appropriate;
-   no live Build menu yet.

#### Phase B --- Live editor integration

-   add Build menu;
-   add Tool Output window;
-   wire Cook Assets;
-   wire Build Debug;
-   wire Build Development according to approved policy;
-   wire Build Release;
-   wire Build All;
-   live incremental output;
-   busy-state disabling;
-   workspace visibility integration;
-   unrelated-CWD safety;
-   builds/tests/smokes.

#### Phase C --- Manual validation

Phase C passed. Live Development Build menu launched the canonical cooker
and CMake presets; Tool Output, one-active-job disabling, Policy A /
LNK1168 when a relink is required, incremental no-op success while the
Development exe is locked, unrelated-CWD root safety, and M36 regressions
were confirmed.

**Source vs cooked vs staged (Phase C finding).** Save Level Source writes
`game/assets/source/`. Cook Assets updates `game/assets/cooked/` only.
Runtime staging is the existing `Platformer3D` POST_BUILD copy into
`build/windows-vs2022/bin/<Config>/assets/`. Cook-only then restarting an
already-built Development exe still loads the previous staged level (FOV
55 vs authored 40 in the Phase C camera test). The current fresh-restart
workflow is Apply Preview → Save → Cook → Build Development → Restart.
Build Development may be a C++ no-op and still stage assets; the exe
timestamp need not change. This separation is intentional in M37.

Milestone 38 is now the dedicated Stage Runtime Assets / Cook & Stage
work. M37 Cook Assets remains cook-only; M37 Build Development remains
`cmake --build --preset windows-development`.

------------------------------------------------------------------------

### 23. Completion criteria

M37 is complete only when:

-   Development editor can launch the existing cooker externally;
-   Development editor can launch supported CMake build presets
    externally;
-   Build All sequences Debug/Development/Release according to approved
    self-build policy;
-   output is captured and visible without freezing the editor;
-   exit status is accurate;
-   concurrent jobs are prevented;
-   repository-root/CWD handling is safe;
-   OS-specific process code is isolated;
-   Debug/Release boundaries are preserved;
-   M36 editor functionality regresses cleanly;
-   automated tests/builds/cooker tests pass;
-   manual Phase C passes;
-   canonical level source remains clean;
-   no M38 work has started.

**Phase C result:** the criteria above are satisfied. Git closure
(commit/push/merge) has not been requested. The authored Level 01 camera
FOV Y is intentionally 40 (not a cooker accident).

------------------------------------------------------------------------

### 24. Deferred backlog

Future milestones may consider:

-   Add/Delete/Duplicate authored objects;
-   dynamic authored collections / Level Format v2;
-   rotation gizmo;
-   undo/redo;
-   multiple levels/scenes;
-   richer build profiles;
-   Linux editor tool process backend;
-   Android/Web build targets;
-   packaging/deployment;
-   optional cancel/restart workflows;
-   explicit Stage Runtime Assets or Cook & Stage (now Milestone 38).

These are not M37.


## Milestone 38 --- Runtime Asset Staging & Cook-and-Stage Workflow

**Branch:** `milestone/38-runtime-asset-staging`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF\
**Status:** Phase B implemented (live Development Build menu). Awaiting Phase C. Milestone 38 is not complete. Milestone 39 has not started.

### Goal

Make the Development editor's asset workflow explicit and convenient by
separating **cooking** from **runtime staging**, while preserving the
canonical CLI/CMake pipeline established through M37.

M38 addresses a UX finding from M37 Phase C:

-   `Save Level Source` writes authored data to `game/assets/source/`.
-   `Cook Assets` updates `game/assets/cooked/`.
-   the running/built game consumes staged assets from
    `build/windows-vs2022/bin/<Config>/assets/`.
-   today, refreshing those staged assets is coupled to the existing
    CMake build/POST_BUILD path.
-   therefore a pure asset edit can require invoking `Build Development`
    even when no C++ rebuild is needed.

M38 will introduce an explicit, deterministic staging operation and a
convenience `Cook & Stage` workflow for Development without pretending
that asset staging is a C++ build.

### Core principles

1.  Source, cooked, and staged assets remain distinct.
2.  The existing cooker remains canonical.
3.  CMake build presets remain canonical for C++ builds.
4.  Staging must not reimplement asset cooking.
5.  Staging must not compile or link C++.
6.  `Cook & Stage` is an explicit user-requested sequence, not an
    automatic reaction to Save/Apply.
7.  No hot reload or automatic game restart.
8.  No arbitrary shell/terminal UI.
9.  Repository-root and unrelated-CWD safety remain mandatory.
10. Tool execution remains Development-only.
11. One active external tool job at a time.
12. Existing M37 Tool Output remains the common status/log surface.
13. No gameplay or LevelDefinition state may be modified by staging.

### Intended Development menu

``` text
Build
├ Cook Assets
├ Stage Runtime Assets
├ Cook & Stage
├ ----------------
├ Build Debug
├ Build Development
├ Build Release
├ Build All
├ ----------------
└ Tool Output
```

Debug must continue to have no external-tool Build menu. Release must
continue to have no editor/tool UI.

### Phase A --- Audit and architecture

Before implementing live UI behavior:

-   Audit the existing CMake POST_BUILD asset staging commands exactly.
-   Identify every cooked runtime file/directory staged for
    `Platformer3D`.
-   Identify destination directories for Debug, Development, and
    Release.
-   Determine whether staging should reuse a CMake script/helper or
    introduce a small project-owned staging tool.
-   Prefer one canonical staging implementation that both the editor and
    CMake can invoke, if this can be achieved without unnecessary
    build-system complexity.
-   Avoid maintaining two independent lists of runtime assets.
-   Define Development staging semantics precisely.
-   Define `Cook & Stage` as a sequential runner job:
    1.  Cook Assets.
    2.  If cook succeeds, Stage Runtime Assets.
    3.  If cook fails, do not stage.
-   Define success/failure and exit-code behavior.
-   Confirm staging uses `copy_if_different` or equivalent incremental
    behavior.
-   Confirm stale/removed cooked runtime files are handled deliberately;
    do not silently invent deletion semantics.
-   Preserve repository-root safety and structured process invocation.
-   Add focused tests for command/job construction and sequence
    behavior.
-   Do not add the live menu items during Phase A.

### Phase B --- Live Development integration

Add:

-   `Build > Stage Runtime Assets`
-   `Build > Cook & Stage`

Use the existing M37 `EditorToolRunner` and Tool Output.

#### Stage Runtime Assets

The action must:

-   stage the canonical cooked runtime assets into the Development
    runtime asset directory;
-   not invoke a C++ compile/link;
-   not invoke the cooker;
-   not Apply, Revert, or Save;
-   not restart the game;
-   work from unrelated process CWD;
-   report useful output and exit status through Tool Output;
-   be disabled while another job is running.

The expected Development destination is based on the audited build
layout, currently:

`build/windows-vs2022/bin/Development/assets/`

but Phase A must verify rather than blindly hard-code assumptions.

#### Cook & Stage

The action must:

1.  run the canonical cooker;
2.  stop immediately if cooking fails;
3.  stage runtime assets if cooking succeeds;
4.  report both steps clearly in Tool Output;
5.  finish Succeeded only if both steps succeed.

Suggested log markers:

``` text
=== Cook & Stage: Step 1/2 - Cook Assets ===
=== Cook & Stage: Step 2/2 - Stage Runtime Assets ===
```

It must not build C++.

**Phase B outcome:** Development Build menu order is Cook Assets, Stage Runtime Assets, Cook & Stage, separator, C++ builds, separator, Tool Output. Both new items call `TryStart` (sequence owned by the runner). Tool Output uses generic `sequenceStep*` fields and `{displayLabel}: n/N label` so Cook & Stage is not shown as Build All. Debug still has no Build menu. No hot reload, restart, Save/Apply, or stale-file deletion.

### Tool Output

Reuse the M37 window and state.

For staging/sequences, display:

-   job label;
-   Running / Succeeded / Failed;
-   elapsed time;
-   exit code where applicable;
-   current sequence step;
-   bounded incremental log.

Do not add a second output system.

### Runtime behavior

M38 does **not** add hot reload.

If Development is already running and its runtime asset files can safely
be replaced, staging may update files on disk, but the currently loaded
level is not required to change in memory.

The validated workflow for authored level changes becomes:

``` text
Apply Preview
→ Save Level Source
→ Cook & Stage
→ Restart Development
```

or explicitly:

``` text
Apply Preview
→ Save Level Source
→ Cook Assets
→ Stage Runtime Assets
→ Restart Development
```

No C++ build should be necessary solely to propagate level-data changes.

### CMake integration

A key architectural goal is to avoid divergence between:

-   editor-triggered staging; and
-   build-triggered POST_BUILD staging.

Phase A must determine the smallest maintainable design.

Preferred outcome: one reusable staging definition/helper invoked by
both paths.

Do not make CMake builds dependent on the editor.

Existing Debug/Development/Release builds must continue to stage assets
as they do today unless the shared implementation deliberately replaces
the internals with equivalent behavior.

### Tests

Automated coverage should include, where practical:

-   staging command/path construction;
-   repository-root validation;
-   Development destination resolution;
-   unrelated-CWD behavior;
-   paths containing spaces;
-   successful staging;
-   staging failure;
-   incremental `copy_if_different` behavior;
-   `Cook & Stage` success sequence;
-   cook failure prevents staging;
-   staging failure makes `Cook & Stage` fail;
-   one-active-job guard;
-   bounded log remains intact;
-   existing M37 Build All behavior remains unchanged;
-   existing editor/workspace/gizmo/orientation/picking/level/physics
    tests remain green;
-   cooker regression tests remain green.

Avoid tests that repeatedly rebuild the whole project when a focused
temporary-directory staging test is sufficient.

### Manual Phase C acceptance

Manually validate at minimum:

-   Development shows the two new menu actions.
-   Debug still has no Build menu.
-   Release still has no tool UI.
-   `Stage Runtime Assets` does not build C++.
-   `Cook & Stage` clearly executes two sequential steps.
-   Tool Output remains responsive.
-   one-job-at-a-time disabling works.
-   close/reopen Tool Output does not affect the job.
-   unrelated-CWD launch still works.
-   a controlled FOV authoring test propagates with:
    `Apply Preview → Save → Cook & Stage → Restart`.
-   the Development executable timestamp does not need to change for
    asset-only edits.
-   no automatic Save/Apply/restart occurs.
-   M35/M36/M37 regressions remain healthy.

### Documentation

Update as appropriate:

-   `README.md`
-   `docs/ARCHITECTURE.md`
-   `docs/MILESTONES.md`
-   `AGENTS.md` only if a durable agent rule is needed.

Document the final canonical developer workflow and the distinction
between source, cooked, staged, and loaded runtime state.

### Explicitly out of scope

M38 must not add:

-   automatic Save → Cook;
-   automatic Apply → Cook;
-   automatic cook on file change;
-   file watching;
-   hot reload;
-   automatic game restart;
-   launcher/self-restart architecture;
-   arbitrary shell input;
-   terminal emulator;
-   interactive stdin;
-   parallel tool jobs;
-   remote builds;
-   Linux staging backend beyond portability-friendly structure;
-   Android/iOS/Web deployment;
-   packaging;
-   Add/Delete/Duplicate;
-   Level Format v2;
-   rotation gizmo;
-   undo/redo;
-   multiple levels/scenes.

### Completion condition

M38 is complete only after:

1.  Phase A audit/architecture is approved.
2.  Phase B implementation and automated validation are approved.
3.  Phase C manual validation passes.
4.  Documentation is accurate.
5.  No unintended authored-level changes remain.
6.  Git closure is performed through the normal project flow: test →
    approve → commit → push milestone branch → merge to main → push main
    → confirm clean/synchronized main.
7.  No M39 work begins before M38 is formally closed.


## Milestone 39 --- Development Runtime Level Reload

**Branch:** `milestone/39-runtime-level-reload`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF\
**Status:** Phase B implemented (live Development `Level > Reload Runtime Level`). Awaiting Phase C. Milestone 39 is not complete. Milestone 40 has not started.

### Goal

Add an explicit Development-only runtime level reload so staged Level
Format v1 data can be tested without closing and reopening the game.

M38 established `source → cooked → staged runtime assets`. M39 adds the
next explicit step:

`Apply Preview → Save Level Source → Cook & Stage → Reload Runtime Level`

Reload reads the staged Development runtime level through the normal
runtime loading path. It must not silently Save, Cook, Stage, Build,
restart the executable, or introduce file watching.

### Core principles

-   Explicit, user-triggered reload.
-   Staged runtime data is the reload authority.
-   Reuse the normal runtime parser/loading and world-rebuild paths.
-   Never load reload data directly from authored source or
    `game/assets/cooked/`.
-   No automatic Save/Cook/Stage/Build.
-   No file watching, executable restart, or general-purpose hot reload.
-   Failed reload must not leave a partially replaced world.
-   Preserve Development/Debug/Release boundaries.
-   Preserve persistent BEST unless the audited architecture gives a
    compelling reason otherwise.

### Intended Development workflow

``` text
Edit level
→ Apply Preview
→ Save Level Source
→ Cook & Stage
→ Reload Runtime Level
→ test immediately
```

Reload is independent: if the user does not cook/stage first, it reloads
the currently staged level.

### Preferred menu

``` text
Level
├ Apply Preview
├ Revert Working Copy
├ Save Level Source
├ ----------------
├ Reload Runtime Level
├ ----------------
└ Reset Editor Layout
```

Phase A must verify that Level is the correct semantic home. Reload is
an in-process editor/runtime action, not an external Build tool.

### Phase A

Audit before live UI: - startup level loading end-to-end; - exact staged
Development level path; - LevelDefinition ownership and active-world
rebuild; - Apply Preview rebuild path and reusable pieces; -
physics/static world reconstruction; - player/run/session reset; -
checkpoints, hazards, collectibles, goal and timer; - moving/dynamic
runtime objects; - camera authored state; - editor
workingCopy/active/savedSourceBaseline, Modified/Dirty; - selection and
editor camera; - persistent BEST; - transactional reload/failure
behavior.

Define the smallest safe reload architecture and focused tests. Do not
add the live menu item in Phase A.

**Phase A outcome:** Reload core is `PrepareRuntimeLevelReload` (absolute
staged path only) plus `PhysicsWorld::TryRebuild` (replacement world,
swap on success; Jolt Factory refcount so two worlds can exist briefly).
Policy A rejects reload while Modified. Development-only
(`CanReloadRuntimeLevel` / authoring flag). Apply Preview now uses
TryRebuild so physics failure is no longer fatal. No live Level menu
item. No EditorToolKind.

### Phase B --- Live editor integration

**Phase B outcome:** Development `Level > Reload Runtime Level` sits after Save Level Source and before Reset Editor Layout. The item emits `LevelEditorRequest::ReloadRuntimeLevel` only; Application owns `ReloadRuntimeLevelFromStaged`. Enable is `CanReloadRuntimeLevel(authoringAvailable, modified, toolRunner.IsRunning())` (Policy A: also disabled while any Build tool is Running). Debug omits the item. Release has no editor. One Level-action status/message in the Level Editor panel. Dirty does not block. No Cook/Stage/Build/restart/hot-reload chain. Awaiting Phase C. Milestone 39 is **not** complete. Milestone 40 has not started.

### Runtime source of truth

Reload must read the staged runtime level, conceptually:

`build/windows-vs2022/bin/Development/assets/levels/level_01.level`

The exact path must come from existing runtime-path mechanisms and be
verified.

Do not reload from: - `game/assets/source/levels/level_01.level` -
`game/assets/cooked/levels/level_01.level`

### Transactional safety

Preferred design: 1. Parse/validate staged level into a temporary
candidate. 2. On parse/validation failure, preserve the current active
world. 3. Commit/rebuild only after validation succeeds. 4. If later
reconstruction can fail, define the smallest safe rollback/commit
boundary supported by the current architecture.

### Successful reload policy

Phase A must audit and explicitly choose semantics. Preferred
direction: - active LevelDefinition becomes staged definition; -
physics/world rebuilds; - player/run state resets deterministically; -
checkpoints/collectibles/hazards/goal/timer reset coherently; -
moving/dynamic objects reset coherently; - authored camera values
update; - workingCopy reconciles to active; - Modified becomes false; -
Dirty remains coherent with saved authored-source baseline; - unsafe
selection is cleared; - editor navigation camera remains editor state
where possible; - Tool Output remains unrelated; - persistent BEST is
preserved.

Do not implement these assumptions blindly; report the exact policy.

### Failure policy

Missing/invalid staged level must: - show a clear Development
error/status; - not crash; - not partially replace the current world; -
not write source/cooked/staged files; - not invoke external tools; - not
erase persistent BEST.

Prefer an existing editor status/error surface rather than a new large
framework.

### Scope

M39 reloads Level Format v1 level data only. It does not add general
GLB, texture, shader, executable, DLL, or C++ hot reload.

M38 commands and semantics remain unchanged. There is no automatic
`Cook & Stage → Reload` chain.

### Tests

Where practical cover: - staged path resolution; - successful candidate
load; - invalid/missing staged level; - active world preserved on
pre-commit failure; - world/physics rebuild; - session reset policy; -
checkpoint/collectible/hazard/goal reset; - working-copy
reconciliation; - Modified/Dirty semantics; - selection policy; - BEST
preservation; - no source/cooked writes; - no external tool
invocation; - M37/M38 runner regressions; - existing
workspace/gizmo/orientation/picking/level/physics tests.

### Manual Phase C

Primary acceptance:

``` text
Camera FOV 40 → 55
Apply Preview
Save Level Source
Cook & Stage
Reload Runtime Level
```

Expected: loaded runtime FOV becomes 55 without Build Development and
without closing/reopening Development.

Then restore final intended FOV 40 through the same explicit workflow.

Also validate reload of old staged data when Cook & Stage is skipped,
safe invalid-level failure, approved session reset behavior,
editor-state reconciliation, BEST preservation, and M35--M38
regressions.

### Documentation

Update README, ARCHITECTURE, MILESTONES, and AGENTS only where
appropriate. Document source vs cooked vs staged vs currently loaded
in-memory state.

### Out of scope

No automatic Save→Cook, Stage→Reload, file watching, background hot
reload, automatic restart, launcher, C++/DLL hot reload, general
texture/model/shader reload, stale-file deletion, arbitrary shell UI,
parallel jobs, Add/Delete/Duplicate, Level Format v2, rotation gizmo,
undo/redo, multiple levels/scenes, mobile/Web deployment, or packaging.

### Completion

M39 completes only after Phase A approval, Phase B approval, Phase C
manual pass, final authored FOV 40, accurate docs, and normal Git
closure: test → approve → commit → push milestone branch → normal merge
to main → push main → confirm clean/synchronized main. M40 must not
start before closure.


## Milestone 40 --- Cook, Stage & Reload Workflow

**Branch:** `milestone/40-cook-stage-reload`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF\
**Status:** Phase B implemented (live Development `Build > Cook, Stage & Reload`). Awaiting Phase C. Milestone 40 is not complete. Milestone 41 has not started.

### Goal

Add a Development-only convenience workflow composing the proven
M37--M39 systems:

`Cook Assets → Stage Runtime Assets → Reload Runtime Level`

It reduces repetitive authoring steps without replacing the individual
canonical commands or weakening their ownership boundaries.

### Intended loop

Current:
`Edit → Apply Preview → Save Level Source → Cook & Stage → Reload Runtime Level → Test`

M40:
`Edit → Apply Preview → Save Level Source → Cook, Stage & Reload → Test`

No automatic Apply or Save.

### Preferred Development Build menu

``` text
Build
├ Cook Assets
├ Stage Runtime Assets
├ Cook & Stage
├ Cook, Stage & Reload
├ ----------------
├ Build Debug
├ Build Development
├ Build Release
├ Build All
├ ----------------
└ Tool Output
```

`Level > Reload Runtime Level` remains available and unchanged.

### Architecture

This is cross-boundary orchestration, not a monolithic external tool
job.

1.  UI requests the convenience workflow.
2.  Existing EditorToolRunner performs canonical Cook & Stage.
3.  Application/editor orchestration observes successful terminal
    completion.
4.  Only then it invokes the existing M39 in-process staged-level
    reload.
5.  Any Cook/Stage failure stops the chain.

Do not put reload in HostProcess, Python, CMake, or the cooker. Do not
duplicate cooking, staging, parsing, physics rebuild, or reconciliation.

### Preconditions

Unavailable/rejected when: - authoring unavailable; - Modified ==
true; - EditorToolRunner already running; - another convenience sequence
is pending.

Dirty alone does not block.

### Sequence

Conceptually:

``` text
Cook, Stage & Reload
  Step 1/3 — Cook Assets
  Step 2/3 — Stage Runtime Assets
  Step 3/3 — Reload Runtime Level
```

If EditorToolRunner only models external-process steps, do not force the
in-process reload into that abstraction. Phase A must design the
smallest clean orchestration state bridging asynchronous Cook & Stage to
synchronous semantic Reload.

### Failures

Cook failure: no Stage, no Reload.\
Stage failure: no Reload.\
Reload failure after successful Cook & Stage: cooked/staged files
remain; active world remains intact per M39; report failure; no asset
rollback.\
No automatic retry.

### Shutdown

Preserve M37/M38 process cleanup. If shutdown begins during Cook/Stage,
no later Reload may execute.

### Status

Cook/Stage output remains in Tool Output. Reload must not create a fake
external-process log. The UI should clearly expose running step and
final success/failure without a new logging framework.

### Existing commands remain canonical

Unchanged: - Cook Assets - Stage Runtime Assets - Cook & Stage - Build
Debug - Build Development - Build Release - Build All - Level \> Reload
Runtime Level

### Phase A

Audit before live UI: - EditorToolRunner sequence lifecycle and terminal
result polling; - LevelEditor/DebugUi/Application tool-request
ownership; - M39 reload request/guards; - clean owner for pending
Cook→Stage→Reload state; - exact state machine and terminal states; -
F2/editor visibility interaction; - shutdown behavior; - status
presentation; - testable orchestration helpers; - all M37--M39
regressions.

Do not add the live menu item in Phase A.

**Phase A outcome:** `CookStageReloadWorkflow` is the testable state machine (`Idle` / `WaitingForCookAndStage` / `ReloadPending`). Application owns it, observes the single runner Poll, and invokes M39 `ReloadRuntimeLevelFromStaged` exactly once after canonical `CookAndStage` Succeeded. `CanStartCookStageReload` / pending-workflow Reload guard are in `EditorWorkspace`. No live Build menu item. F2/window visibility does not own the workflow. Shutdown Cancels before runner cleanup.

### Phase B

After approval: - add Development-only menu action; - start canonical
Cook & Stage; - trigger M39 Reload only after successful completion; -
stop on failures; - preserve guards/boundaries; - integrate status/Tool
Output; - add regressions.

**Phase B outcome:** Development Build menu includes `Cook, Stage & Reload` after `Cook & Stage`. The item emits `LevelEditorRequest::CookStageAndReload` only. Application starts canonical `CookAndStage`, observes Poll snapshots, and invokes M39 reload exactly once. Debug has no Build menu. Awaiting Phase C. Milestone 40 is **not** complete. Milestone 41 has not started.

### Phase C

Primary acceptance: 1. baseline FOV 40; 2. change 40 → 55; 3. Apply
Preview; 4. Save Level Source; 5. choose Cook, Stage & Reload; 6. no
separate Cook & Stage; 7. no separate Reload; 8. no Build Development;
9. no restart.

Expected: Cook succeeds → Stage succeeds → Reload succeeds → loaded FOV
55 in the same Development process.

Restore 55 → 40 through the same workflow.

Also validate Modified guard, Dirty allowed, running-job guard, failure
stopping, individual M38/M39 commands, coherent status, F2 visibility,
and safe shutdown.

### Documentation

Update README, ARCHITECTURE, MILESTONES, and AGENTS only where
appropriate. Document this as convenience orchestration over canonical
commands.

### Out of scope

No automatic Apply, automatic Save, file watching, source-change
automation, general hot reload, texture/model/shader reload, restart,
C++/DLL hot reload, stale-file mirroring/deletion, parallel jobs,
arbitrary shell commands, undo/redo, Add/Delete/Duplicate, rotation
gizmo, Level Format v2, multiple levels/scenes, mobile/Web deployment,
packaging, or M41 work.

### Completion

M40 completes only after Phase A, Phase B, Phase C, final FOV 40 across
authored/cooked/staged/loaded state, M37--M39 regression validation,
documentation, and normal Git closure: test → approve → commit → push
branch → normal merge to main → push main → confirm clean/synchronized
main. M41 must not start before closure.


## Milestone 41 — Authored Object Add / Delete / Duplicate

### Status
**CLOSED.** Phase A, Phase B, and Phase C accepted. Milestone 41 is complete and merged.

M40 is closed. M41 is closed. Milestone 42 is Object Palette & Placement Workflow.

### Branch
`milestone/41-authored-object-lifecycle`

### Recommended Cursor model
**Grok 4.6 High — Fast OFF**

### Goal
Extend the Development Level Editor so repeatable Level Format v1 objects can be **added, duplicated, and deleted** without hand-editing `level_01.level`.

Supported lifecycle categories:
- Platforms
- Checkpoints
- Hazards
- Collectibles

Fixed/out of scope lifecycle categories:
- Spawn, Ground, Camera, Goal
- slopes, moving platform, dynamic cyan box
- technical/demo renderer probes

This is not a generic scene graph, prefab system, ECS conversion, undo/redo system, or Level Format v2.

### Authoring semantics
Lifecycle operations modify `workingCopy` only.

- **Add:** inserts a supported object into `workingCopy` using editor-camera X/Y and `workingCopy` spawn.z as the current gameplay-lane Z; new object becomes selected.
- **Duplicate:** copies the selected supported object, applies +1 world X, preserves source Z (no lane snap), and selects the copy.
- **Delete:** removes the selected supported object from `workingCopy` and safely clears/reconciles selection. Platforms require count > 1 and must not be referenced by checkpoint/goal support metadata.

Collected runtime visibility is separate from authored editor visibility. Opening F2 does not mutate `CollectibleRunState`. Collected authored Collectibles remain selectable/editable.

Pending-deleted objects remain in `active` until Apply, with a Development editor wireframe mark. A session-local active↔working index map keeps surviving same-category viewport picks valid. Delete key (Development) matches Edit > Delete Selected and is ignored while ImGui captures the keyboard.

Immediately after any lifecycle edit:
- `workingCopy` changes;
- `active` does not;
- `Modified = true`;
- runtime physics/gameplay does not change;
- no automatic Apply or Save occurs.

`Apply Preview` remains the transactional commit boundary. On success, rebuild/validate using the existing path and commit to `active`. On failure, preserve the active runtime world and keep `workingCopy` available for correction.

`Revert Working Copy` restores `workingCopy = active`, cancels pending additions/deletions/duplicates, clears unsafe transient gizmo state, and returns `Modified = false`.

`Save Level Source` remains Development-only and saves `active`, never unapplied `workingCopy`.

### Level Format v1
Level Format v1 remains unchanged. Phase A must audit whether repeatable categories currently use fixed-size arrays/assumptions and make the smallest safe change needed for variable counts.

Requirements:
- existing v1 files still load;
- parser/writer remain deterministic;
- existing v1 count grammar is preserved;
- cooker accepts authored variable counts;
- runtime state sizes safely follow authored counts;
- M39 Reload and M40 Cook, Stage & Reload continue to work;
- no silent migration to v2.

Prefer standard variable-length containers only for genuinely repeatable categories. Do not convert the whole level to a generic entity list.

### Selection and ordering
Current type+index selection may remain if safe invalidation rules are explicit and tested. Do not add GUIDs unless Phase A proves they are necessary.

Required:
- Add selects the new object;
- Duplicate selects the copy;
- Delete cannot leave a dangling index;
- Apply/Revert/Reload reconcile selection safely;
- deleting during a gizmo interaction clears transient manipulation state.

Insertion/order must be deterministic. Prefer Add-at-end. Phase A must choose and document whether Duplicate inserts after the selected object or appends. Do not reorder unrelated objects.

Checkpoint container/writer order remains gameplay checkpoint order unless the existing format already defines otherwise.

### Defaults
Phase A must define deterministic valid Add defaults for each category and a small deterministic Duplicate offset (preferably world +X if valid).

No drag-to-place, snapping, placement mode, surface raycast placement, or configurable duplicate offset in M41.

### UI
Preferred Development menu:

```text
Edit
├ Add
│  ├ Platform
│  ├ Checkpoint
│  ├ Hazard
│  └ Collectible
├ Duplicate Selected
└ Delete Selected
```

Existing View / Transform / Level / Build menus remain.

Optional shortcuts:
- `Ctrl+D` — Duplicate Selected
- `Delete` — Delete Selected

Only add shortcuts if they cleanly honor current-frame ImGui keyboard capture and do not conflict with navigation/gizmo/text input. Menu-only operation is acceptable.

### Hierarchy / Inspector / pending state
Hierarchy is an authoring view and should reflect `workingCopy` immediately.

Therefore before Apply:
- pending additions/duplicates may appear in Hierarchy/Inspector;
- pending deletions may disappear from Hierarchy;
- active runtime world remains unchanged.

World picking keeps the existing rule: **the active visible world is the authority**. Phase A must explicitly prevent a pending-deleted active object from producing an invalid working-copy selection.

Reuse the smallest existing pending/ghost visualization where practical for newly added/duplicated objects. Do not build a second full preview renderer or a tombstone system.

Inspector edits selected `workingCopy` objects. Deleted objects cannot leave stale Inspector references.

Existing gizmo capabilities remain category-dependent. New Platforms must support existing Translate/Resize. No rotation gizmo.

### Validation and limits
Audit existing validation before choosing count limits. Protect against:
- unsafe/pathological counts;
- parser/writer count mismatch;
- unsafe allocations/overflow;
- invalid dimensions;
- non-finite values.

A conservative shared per-category maximum is acceptable if justified by current runtime assumptions.

### Runtime audit
Phase A must inspect all fixed-count assumptions in at least:
- `LevelDefinition`;
- Level Format v1 parser/writer;
- cooker;
- hierarchy/inspector;
- selection/picking;
- gizmos/pending ghost;
- `PhysicsWorld::TryRebuild`;
- static platform creation;
- checkpoint progression/state;
- hazard state;
- collectible state/count;
- run reset/respawn;
- M39 runtime reload;
- M40 workflow;
- tests/docs.

Variable checkpoint, hazard, collectible, and platform counts must resize/rebuild runtime state safely.

### Configuration boundaries
**Development:** lifecycle authoring UI enabled.

**Debug:** preserve existing editor/debug behavior, but no source-authoring lifecycle operation that violates the established Development-only authoring boundary.

**Release:** no editor/lifecycle authoring UI.

### Phase A — architecture and fixed-count audit
1. Audit all fixed-count assumptions.
2. Map v1 parser/writer/count grammar and cooker assumptions.
3. Map runtime state sizing.
4. Map hierarchy/selection/picking/gizmo assumptions.
5. Choose smallest safe variable-count representation.
6. Define insertion ordering.
7. Define Add defaults.
8. Define Duplicate offset.
9. Define Delete selection behavior.
10. Resolve active-vs-working picking edge cases.
11. Define validation/count limits.
12. Define configuration gating.
13. Implement/refactor pure testable data operations where useful.
14. Add focused non-live tests.
15. Preserve M39/M40 behavior.
16. Update architecture docs.
17. Prepare exact Phase B plan.

**Do not add the live Edit menu in Phase A.**
No Phase C. No commit/push/merge.

Phase A implemented (this branch): `std::vector` for the four repeatable categories; singletons unchanged; append-only Add/Duplicate (existing platform-index references do not shift); Platform delete remaps `support_index_*` with `R > D`, rejects `R == D` and the last platform; delete success clears selection; duplicate offset +1 X; per-category `CategoryStructuralPending` so active picks are not remapped across index shifts; min 1 platform; no GUIDs. The original Phase A 16/8/8/16 caps were defensive policy only; Correction 7 replaces them with physics leftover for Platforms (58) and the shared 256-line / 64 KiB parser guard for Checkpoint / Hazard / Collectible.

Phase B implemented (this branch): Development Edit menu (Add/Duplicate/Delete, menu-only plus Delete key); `LevelEditorRequest` intents; `HandleAuthoredLifecycleRequest` mutates workingCopy only; Inspector edits for Checkpoint/Hazard/Collectible authored fields; Translate gizmo for those three categories (Resize remains Ground/Platform); Checkpoint world Translate moves `center` and `respawnPosition` by the same delta so the authored relative offset is preserved (Inspector Trigger Center / Respawn Position stay independent); category-local pick guard; cyan **wireframe** pending bounds plus Development pending **object** ghosts (checkpoint post/beacon, full hazard, collectible visual cube) from `workingCopy` so bounds do not hide the object; pending Add/Modify ghosts persist after deselection (`CollectPendingAuthoringVisuals`); selected pending uses stronger cyan, unselected pending uses softer cyan; Platform uses one cyan wire AABB (visual == bounds); Add uses editor-camera placement anchor (look-forward * 10), not Spawn; Duplicate remains +1 X; editor-only Checkpoint respawn marker from `workingCopy`; pending ghosts create no physics/gameplay; visible pending Add/Duplicate/Modify ghosts are viewport-pickable (`PendingPickProxy`, working index, priority over mapped active hits); pending deletes keep the full active object visible until Apply, drawn faded/desaturated with world depth and a subtle dusty-red outline (not a second entity); pending-deleted objects stay non-pickable; pending-delete visual precedence beats cyan pending Add/Modify and collected authored gold wire; Platform Add follows the Jolt body leftover (58) rather than a 16-object design cap; Checkpoint/Hazard/Collectible Add disable only at the v1 256-line file guard; M39/M40 unchanged. No live Edit menu in Debug. No editor in Release. Object Palette is Milestone 42, not part of M41.

### Phase B — live integration
After Phase A approval:
- add Development Edit menu;
- wire Add Platform/Checkpoint/Hazard/Collectible;
- wire Duplicate/Delete;
- integrate Hierarchy/Inspector;
- integrate pending visualization where appropriate;
- safely reconcile selection and gizmo state;
- preserve active-world picking authority;
- preserve Apply/Revert/Save semantics;
- verify M39/M40 compatibility;
- update tests/docs.

No commit/push/merge until Phase C approval.

### Phase C — manual acceptance
Primary scenario:
1. Start Development.
2. Add Platform.
3. Verify pending authoring state.
4. Edit position/size.
5. Apply Preview.
6. Verify active platform/collision.
7. Save Level Source.
8. Cook, Stage & Reload.
9. Verify it survives staged reload without build/restart.
10. Duplicate it and verify deterministic offset/selection.
11. Apply/Save/Cook-Stage-Reload.
12. Delete the duplicate.
13. Apply/Save/Cook-Stage-Reload.
14. Verify final level.

Also exercise lifecycle operations for Checkpoint, Hazard, and Collectible.

Validate Revert, Modified/Dirty semantics, selection safety, no stale gizmo, checkpoint ordering/state, collectible count/reset, hazard respawn, M39 manual Reload, M40 workflow, F2/editor visibility, unrelated-CWD behavior, and no CWD `imgui.ini`.

### Automated tests
Add focused coverage for:
- Add/Duplicate/Delete supported categories;
- rejection of unsupported singleton lifecycle;
- deterministic insertion/order and duplicate offset;
- selection after lifecycle operations;
- Revert after Add/Delete;
- variable-count parser round trip;
- deterministic writer;
- invalid/excessive counts;
- physics rebuild after platform count changes;
- checkpoint/hazard/collectible state resizing;
- reload with changed counts;
- M40 workflow regression.

Retain existing editor, physics, cooker, staging, reload, and tool-runner regressions.

### Documentation
Update as appropriate:
- `README.md`
- `docs/ARCHITECTURE.md`
- `docs/MILESTONES.md`
- `AGENTS.md`

Document supported categories, workingCopy-only lifecycle edits, Apply/Revert behavior, selection invalidation, ordering, v1 compatibility, runtime resizing, M39/M40 compatibility, and configuration gating.

### Out of scope
- undo/redo;
- generic scene graph/ECS/reflection;
- prefabs/GUID system unless strictly proven necessary;
- drag placement/surface or grid snapping;
- explicit placement plane / multi-lane snapping (spawn.z is the current v1 proxy);
- rotation gizmo;
- multi-select;
- clipboard copy/paste;
- grouping/parenting/rename;
- arbitrary object types;
- lifecycle for slopes/moving platform/dynamic body/Spawn/Ground/Camera/Goal;
- Level Format v2;
- multiple levels/scenes;
- general hot reload/file watching;
- mobile/Web deployment;
- packaging;
- M42;
- Object Palette / Object Browser (future editor UX candidate; not required for M41).

### Completion
M41 completes only after Phases A/B/C are approved, variable-count supported objects survive Apply/Save/Cook/Stage/Reload safely, regressions pass, `git diff --check` is clean, the branch is committed/pushed/merged through the normal workflow, `main` is clean/synchronized, and M42 has not started.


## Milestone 42 --- Object Palette & Placement Workflow

**Branch:** `milestone/42-object-palette-placement`\
**Status:** CLOSED. Merged to main. Milestone 43 is Editor Quick Toolbar.\
**Prerequisite:** Milestone 41 --- Authored Object Lifecycle --- CLOSED\
**Scope:** Development editor only

### Goal

Turn the M41 authored-object lifecycle into a faster level-design
workflow by adding a compact **Object Palette** and an explicit
**placement mode** for the four repeatable authored categories:

-   Platform
-   Checkpoint
-   Hazard
-   Collectible

M42 improves *how* designers create and place objects. It does not
change Level Format v1, runtime authority, gameplay rules, or the
lifecycle semantics established in M41.

### User-facing outcome

In Development/F2 editor mode, the designer can open an Object Palette,
choose an authored object type, and place new objects directly in the 3D
viewport.

Expected workflow:

1.  Open **Object Palette**.
2.  Choose Platform, Checkpoint, Hazard, or Collectible.
3.  Enter placement mode.
4.  Move the placement preview through the viewport.
5.  Click to place the object into `workingCopy`.
6.  The placed object becomes selected.
7.  Continue placing the same type or exit placement mode.
8.  Use the existing M41 Apply/Revert/Save workflow normally.

The existing **Edit \> Add** commands remain valid and are not removed.

### Non-goals

M42 must not introduce:

-   ECS or scene graph architecture.
-   GUIDs or persistent object IDs.
-   Level Format v2.
-   New authored object categories.
-   Arbitrary asset browser/content browser.
-   Prefab system.
-   Drag-and-drop asset importing.
-   Runtime spawning system.
-   Physics simulation for placement previews.
-   General undo/redo stack.
-   Multi-selection.
-   Copy/paste framework.
-   Object grouping/parenting.
-   Grid/surface snapping framework beyond the narrow placement rules
    defined here.
-   Terrain editing.
-   Streaming/LOD/partitioning.
-   Changes to M39/M40 Cook/Stage/Reload semantics.

### Existing M41 contracts that remain authoritative

M42 must preserve:

-   `active` = currently applied runtime world.
-   `workingCopy` = pending authored state.
-   Add/Duplicate/Delete mutate `workingCopy` only.
-   Apply remains transactional.
-   Revert restores `workingCopy = active`.
-   Save writes active state only.
-   `StructuralIndexMap` remains the transient active↔working identity
    mechanism.
-   Pending Add/Duplicate/Modify visuals remain cyan.
-   Selected pending visuals use stronger emphasis.
-   Unselected pending visuals remain visible with softer emphasis.
-   Pending Delete remains faded/desaturated with delete outline.
-   Pending workingCopy objects remain viewport-pickable.
-   Pending Delete remains non-pickable.
-   Platform support-index deletion/remap rules remain unchanged.
-   Capacity policy from M41 remains unchanged.
-   Debug has no authoring palette/placement workflow.
-   Release has no editor.

### Object Palette

Add a Development-only **Object Palette** editor window.

Minimum contents:

-   Platform
-   Checkpoint
-   Hazard
-   Collectible

Each entry should be compact and immediately actionable.

The palette is a creation tool, not a hierarchy replacement.

#### Workspace integration

Add **Object Palette** to the existing `View` menu and workspace state.

Preferred menu:

``` text
View
├ Metrics
├ Hierarchy
├ Inspector
├ Level Editor
└ Object Palette
```

Requirements:

-   Window visibility participates in the existing editor workspace
    authority.
-   Closing the palette window updates the corresponding View menu
    state.
-   View menu toggling updates the window.
-   F2 editor visibility behavior remains consistent with other editor
    windows.
-   Persistent layout uses the existing
    `%LOCALAPPDATA%\Platformer3D\editor_layout.ini`.
-   Do not create CWD `imgui.ini`.
-   Reset Editor Layout resets palette window placement/visibility
    according to the established workspace policy.

### Placement mode

Selecting a palette entry enters an editor-only placement mode for that
category. Clicking the **same** category again returns to `None` (tool
toggle). Clicking a different category switches mode. Esc still exits.
Placement mode is transient editor state and must not modify the level
until the user confirms placement.

Conceptually:

``` text
PlacementMode
├ None
├ Platform
├ Checkpoint
├ Hazard
└ Collectible
```

No generalized tool framework is required.

### Placement preview

While placement mode is active, draw a preview of the object at the
candidate position.

The preview:

-   Uses the selected category's default authored geometry.
-   Uses a distinct placement-preview treatment.
-   Must be visually distinguishable from already-created pending
    objects.
-   Does not exist in `workingCopy` yet.
-   Has no physics.
-   Has no trigger/gameplay behavior.
-   Is not included in Hierarchy.
-   Is not included in Save/Apply.
-   Is not viewport-pickable as an authored object.

Preferred visual language:

-   Placement candidate: brighter/cleaner cyan preview, optionally with
    a simple placement marker.
-   Existing pending object: M41 cyan selected/unselected styles.
-   Pending Delete: M41 faded/delete style.

Do not introduce shaders or a new rendering framework.

### Candidate position

Placement should be driven by the editor viewport ray.

Preferred narrow rule:

1.  Cast the editor mouse ray.
2.  If it hits an eligible active-world placement surface, place the
    candidate at the hit point with category-specific vertical offset as
    needed.
3.  If there is no eligible hit, use a fallback point in front of the
    editor camera.
4.  Preserve the current level traversal-lane convention where
    appropriate rather than creating a new Level Format field.

Keep this deterministic and testable.

#### Eligible surfaces

For M42, surface placement only needs to consider authored/static world
geometry already available to editor picking/geometry helpers.

Do not implement arbitrary mesh triangle placement or physics raycasts
if existing editor CPU geometry is sufficient.

#### Fallback

If no surface is hit, use the existing editor-camera working-region
concept rather than silently placing near Spawn.

The fallback must remain visible/reachable from the current editor
camera.

### Category defaults

Use the same canonical default values already defined by M41 lifecycle
helpers.

Do not create a second set of defaults in the palette.

#### Platform

Placement creates a Platform using the M41 default size.

The candidate position should represent the Platform center
consistently.

#### Checkpoint

Placement creates the entire Checkpoint assembly.

The initial:

-   trigger center
-   respawn position

must preserve the M41 default relation.

After creation, normal checkpoint assembly translation behavior remains
authoritative.

#### Hazard

Placement creates the existing M41 default Hazard.

#### Collectible

Placement creates the existing M41 default Collectible using its
authored collection bounds.

### Confirm placement

Primary viewport click confirms placement when:

-   placement mode is active,
-   mouse is over the viewport,
-   ImGui does not capture the pointer,
-   no gizmo interaction consumes the pointer,
-   capacity permits the object.

Confirmation must call the same M41 lifecycle Add authority rather than
duplicating lifecycle mutation logic.

The helper should accept the resolved placement position.

After successful placement:

-   append to `workingCopy`,
-   update structural mapping/session state as required by M41,
-   select the new working object,
-   show the normal selected pending visual,
-   expose it in Hierarchy and Inspector.

Do not Apply automatically.

Do not Save automatically.

Do not create physics automatically.

### Repeated placement

After a successful placement, remain in the same placement mode by
default so multiple objects of one type can be authored efficiently.

Example:

``` text
Choose Collectible
click
click
click
Esc
```

Each click creates one Collectible in `workingCopy`.

Each newly created object becomes the current selection.

Previously created pending objects remain visible with M41 unselected
emphasis.

### Exit placement mode

Support at minimum:

-   `Esc` exits placement mode.
-   Selecting another palette category switches placement type.
-   Closing the Object Palette does **not** have to cancel placement
    mode unless the implementation makes that behavior clearer and is
    documented consistently.
-   F2 closing the editor must cancel placement mode.
-   Apply/Revert/Reload must cancel placement mode.
-   Entering a conflicting gizmo interaction/tool must not accidentally
    place an object.

No right-click context-menu system is required.

### Cursor/input safety

Placement must honor current-frame ImGui capture.

Do not place objects when clicking:

-   menu bar,
-   Object Palette,
-   Hierarchy,
-   Inspector,
-   Tool Output,
-   other ImGui windows.

Existing editor camera controls must remain usable.

RMB camera navigation must not place objects.

Gizmo interaction retains priority over placement.

### Capacity behavior

Reuse the M41 capacity policy.

-   Platform placement stops at actual physics capacity.
-   Checkpoint/Hazard/Collectible use Level Format v1 defensive
    record/file capacity rather than old arbitrary 8/16 limits.
-   Disabled/unavailable placement should communicate the existing
    capacity reason.
-   Do not add new small category caps.

### Selection and picking

After placement, the object is a normal M41 pending workingCopy object.

Therefore it must:

-   be selectable from Hierarchy,
-   be selectable from viewport using M41 pending picking,
-   receive strong pending emphasis when selected,
-   receive soft pending emphasis when deselected.

Do not create a separate selection model for palette-created objects.

### Apply / Revert / Reload

#### Apply success

-   Commit `workingCopy` to active using existing transactional rebuild.
-   Clear pending authoring state as M41 already defines.
-   Cancel placement mode.
-   New objects become normal active objects.

#### Apply failure

-   Preserve active world.
-   Preserve workingCopy.
-   Preserve pending objects.
-   Placement mode may be cancelled for safety, but behavior must be
    deterministic and documented.

#### Revert

-   Restore `workingCopy = active`.
-   Remove pending palette-created objects.
-   Clear transient placement preview.
-   Cancel placement mode.

#### Runtime Reload

-   Preserve M39 authority.
-   Successful Reload clears pending editor state and cancels placement
    mode.
-   Do not alter M40 Cook, Stage & Reload orchestration.

### Edit \> Add compatibility

Keep existing M41:

``` text
Edit
└ Add
   ├ Platform
   ├ Checkpoint
   ├ Hazard
   └ Collectible
```

These commands remain quick-add commands using their existing
placement-anchor semantics.

Do not silently change Edit \> Add into placement mode.

Object Palette placement is an additional workflow.

This keeps M41 behavior stable and gives the designer both:

-   quick Add,
-   deliberate viewport placement.

### Testability

Keep placement math and decisions outside direct ImGui/raylib UI code
where practical.

Preferred small pure/testable helpers include concepts such as:

-   placement mode state,
-   candidate position resolution,
-   placement surface hit selection,
-   category default placement transform,
-   placement confirmation eligibility.

Do not build a generalized editor command framework.

### Automated acceptance

Add focused tests for:

1.  Palette/workspace visibility state.
2.  Placement mode enter/switch/exit.
3.  Esc cancellation.
4.  F2/editor-close cancellation.
5.  Candidate surface-hit placement.
6.  Candidate fallback placement.
7.  Platform placement default.
8.  Checkpoint assembly placement default.
9.  Hazard placement default.
10. Collectible placement default.
11. Confirm adds exactly one object.
12. Repeated placement adds multiple objects.
13. New object becomes selected.
14. Previous pending objects remain visible.
15. M41 pending viewport picking works on palette-created objects.
16. ImGui capture prevents placement.
17. RMB camera interaction prevents placement.
18. Gizmo-consumed pointer prevents placement.
19. Capacity rejection.
20. Apply clears placement state.
21. Revert clears placement state.
22. Reload clears placement state.
23. Existing Edit \> Add behavior unchanged.
24. Pending Delete behavior unchanged.
25. StructuralIndexMap behavior unchanged.
26. Level Format v1 round-trip unchanged.

Run the established M41/editor/runtime regression suites in all
applicable configurations.

### Manual acceptance

In Development build:

#### Platform

-   Open F2.
-   Open Object Palette.
-   Choose Platform.
-   Move cursor over an eligible surface.
-   Verify placement preview.
-   Click.
-   Verify Platform appears in Hierarchy and Inspector.
-   Verify it is selected.
-   Click elsewhere.
-   Verify soft pending visual remains.
-   Click Platform again in viewport.
-   Verify selection returns.
-   Test Translate and Resize.
-   Revert.

#### Checkpoint

-   Choose Checkpoint.
-   Place it.
-   Verify trigger/beacon/respawn representation.
-   Verify checkpoint assembly semantics.
-   Deselect/reselect from viewport.
-   Revert.

#### Hazard

-   Choose Hazard.
-   Place it.
-   Deselect/reselect.
-   Move it.
-   Revert.

#### Collectible

-   Choose Collectible.
-   Place at least three with repeated clicks.
-   Verify each new object becomes selected.
-   Verify older pending Collectibles remain visible.
-   Click each from viewport.
-   Revert.

#### Input safety

-   While placement mode is active, interact with Object Palette,
    Inspector and Hierarchy.
-   Verify no accidental object creation.
-   RMB navigate camera.
-   Verify no accidental placement.
-   Press Esc.
-   Verify preview disappears and no object is created.

### Mixed lifecycle

Before Apply:

-   Place one new object.
-   Modify one existing object.
-   Delete another existing object.

Verify simultaneously:

-   placement-created/pending = cyan family,
-   modified pending = cyan family,
-   pending Delete = faded/delete style.

Apply and verify all three changes become the active world correctly.

Then restore the canonical Level 01 before closure.

### Regression suites

Run:

``` text
AuthoredObjectLifecycleTest
AuthoredLifecycleIntegrationTest
EditorWorkspaceTest
EditorGizmoTest
EditorOrientationTest
EditorPickingTest
LevelFileTest
PhysicsRebuildTest
CookStageReloadWorkflowTest
EditorToolRunnerTest
```

plus new M42 Object Palette / placement tests.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Then:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

### Canonical safety

Before M42 closure, restore/confirm canonical Level 01:

-   Platforms = 6
-   Checkpoints = 2
-   Hazards = 2
-   Collectibles = 3
-   Camera FOV = 40

Run:

``` text
git diff -- game/assets/source/levels/level_01.level
git diff --check
```

Expected:

-   canonical level has no unintended semantic diff,
-   `git diff --check` is clean.

Do not normalize line endings merely to remove the known EOL warning. If
necessary, restore the worktree copy from HEAD as established in
previous milestones.

### Documentation

Update as appropriate:

-   `README.md`
-   `AGENTS.md`
-   `docs/ARCHITECTURE.md`
-   `docs/MILESTONES.md`

Document:

-   Object Palette scope.
-   Placement mode state.
-   Surface-hit/fallback placement rule.
-   Repeated placement.
-   Input/capture rules.
-   Relationship to M41 lifecycle.
-   Edit \> Add remains available and unchanged.
-   No runtime/physics entity until Apply.
-   No Level Format change.

### Completion criteria

M42 is complete only when:

-   Object Palette is integrated into Development workspace.
-   All four M41 repeatable categories can enter placement mode.
-   Placement preview is clear and non-authoritative.
-   Viewport click creates the object at the resolved candidate
    position.
-   Repeated placement works.
-   Pending objects use M41 lifecycle/visual/picking semantics.
-   Input capture prevents accidental placement.
-   Apply/Revert/Reload cleanup is correct.
-   Existing Edit \> Add remains unchanged.
-   No Level Format v1 change.
-   Automated tests pass.
-   Manual acceptance passes.
-   Debug/Development/Release builds pass.
-   Canonical Level 01 is restored.
-   Git diff checks are clean.
-   No unrelated scope was introduced.

### Git closure

Do not commit/push/merge until manual acceptance is approved.

After approval:

``` text
test
→ approve
→ inspect diff/status
→ commit once
→ push milestone branch
→ merge normally into main
→ push main
→ confirm clean/synced main
```

Do not use `--no-ff` automatically.


## Milestone 43 --- Editor Quick Toolbar

**Branch:** `milestone/43-editor-quick-toolbar`\
**Status:** CLOSED\
**Prerequisite:** Milestone 42 --- Object Palette & Placement Workflow
--- CLOSED\
**Scope:** Development editor UX only

### Goal

Add a compact, fixed **Quick Toolbar** directly below the existing menu
bar so the most frequently used editor actions are available without
keeping multiple windows open or repeatedly navigating menus.

The toolbar is an alternate presentation of existing editor commands. It
must reuse the same state and command authorities already established by
M36--M42 rather than introducing duplicate behavior.

Initial toolbar actions:

-   Translate
-   Resize
-   Apply Preview
-   Revert Working Copy
-   Save Level Source
-   Build configuration selector:
    -   Debug
    -   Development
    -   Release
    -   All
-   Run Build button

The toolbar can be shown or hidden through **View \> Quick Toolbar**.

### Design principles

1.  **One authority per action.** Toolbar buttons call the same commands
    as menus/windows.
2.  **Compact.** The toolbar should reduce window/menu friction, not
    become another large panel.
3.  **Stateful where useful.** Transform mode and selected build
    configuration must be visible.
4.  **Extensible, not generic.** Future milestones may add
    high-frequency shortcuts, but M43 must not create a generalized
    command registry/plugin toolbar framework.
5.  **Development-only authoring UI.** Do not expose authoring controls
    in Debug or Release.

### Layout

Preferred structure:

``` text
Menu Bar
File  Edit  View  Transform  Level  Build ...

Quick Toolbar
[Translate] [Resize] | [Apply] [Revert] [Save] | Build: [Development v] [Run]
```

The toolbar is fixed immediately below the menu bar and spans the
available editor width.

It is not an ordinary floating ImGui window.

When hidden, the viewport/editor content should reclaim its vertical
space.

### View menu integration

Add:

``` text
View
├ Metrics
├ Hierarchy
├ Inspector
├ Level Editor
├ Object Palette
├ Quick Toolbar
└ Tool Output
```

Exact ordering may follow the current project convention.

Requirements:

-   `View > Quick Toolbar` toggles visibility.
-   Visibility participates in the existing workspace authority.
-   Default visibility: **on**.
-   Reset Editor Layout restores the toolbar to its default visible
    state.
-   F2 hide/show behavior remains consistent with the editor.
-   No CWD `imgui.ini`.

### Transform shortcuts

Provide two mutually exclusive controls:

-   Translate
-   Resize

They must use the exact existing `TransformMode` authority.

Expected:

``` text
Translate active -> Translate appears pressed/active
Resize active    -> Resize appears pressed/active
```

Clicking Translate selects Translate mode.

Clicking Resize selects Resize mode.

The toolbar, `Transform` menu, and any existing mode UI must always
reflect the same state.

Do not create a second toolbar-specific transform state.

#### Availability

Respect existing selection/category rules.

If Resize is not valid for the current selection, the toolbar must
reflect the same availability policy as the existing editor behavior.

Do not enable an operation through the toolbar that the canonical
command path would reject.

### Apply Preview shortcut

Add a compact Apply action.

It must invoke the exact same Apply Preview command/handler used by:

-   Level menu
-   Level Editor window

Preserve all existing rules:

-   transactional physics rebuild,
-   working-copy validation,
-   Development/Debug policy already established,
-   pending lifecycle cleanup,
-   placement-mode cleanup from M42,
-   failure preservation semantics.

Do not create a toolbar-specific Apply implementation.

### Revert Working Copy shortcut

Add Revert.

It must call the existing Revert authority.

Preserve:

-   `workingCopy = active`,
-   pending lifecycle cleanup,
-   gizmo/transient cleanup,
-   M42 placement cancellation,
-   structural-map reset semantics.

### Save Level Source shortcut

Add Save.

It must call the existing Save Level Source authority.

Preserve:

-   Development-only authoring boundary,
-   Save writes active state only,
-   no implicit Apply,
-   existing validation/error handling,
-   deterministic Level Format v1 writer,
-   existing canonical source path behavior.

The toolbar must reflect whether Save is currently enabled.

### Toolbar iconography

Use compact icon-like controls where practical.

M43 does **not** require importing a new icon font, texture atlas, SVG
library, or asset dependency.

Preferred implementation order:

1.  Existing built-in/project icon resources, if already available.
2.  Small text/symbol labels that render reliably with the existing
    ImGui font.
3.  Compact labeled buttons if symbols would be ambiguous.

Tooltips are required for icon-only or abbreviated controls.

Examples:

-   Translate
-   Resize
-   Apply Preview
-   Revert Working Copy
-   Save Level Source
-   Run selected build

Do not use obscure Unicode glyphs if font coverage is uncertain.

### Build selector

Add a build configuration combo with:

-   Debug
-   Development
-   Release
-   All

Default on first use:

-   **Development**

The selected option must persist and be restored in later editor
sessions.

This selection is editor preference state, not level data.

It must never be serialized into Level Format v1.

### Build selection persistence

Persist the last selected build option using the existing editor
preference/layout persistence mechanism where appropriate.

Requirements:

-   First run/no saved value -\> Development.
-   User selects Debug -\> later editor session restores Debug.
-   User selects All -\> later editor session restores All.
-   Invalid/unknown persisted value -\> safely fall back to Development.
-   Reset Editor Layout behavior must be explicitly defined and tested.

Preferred M43 policy:

-   Reset Editor Layout resets window/workspace layout and toolbar
    visibility.
-   It does **not** need to reset the user's last build selection unless
    the existing preference architecture treats all such editor
    preferences as resettable together.

Choose one deterministic policy and document it.

Do not introduce a general settings database.

### Run Build button

The Run button executes the build currently selected in the combo.

Mapping:

``` text
Debug       -> existing Build Debug command
Development -> existing Build Development command
Release     -> existing Build Release command
All         -> existing Build All command
```

Use the existing `EditorToolRunner` command authority from M37.

Do not invoke CMake directly from toolbar code.

Do not duplicate process-launch logic.

Do not invent a new build orchestration layer.

### Build busy state

Preserve the existing one-job-at-a-time policy.

While a tool/build job is running:

-   Run Build must be disabled or otherwise prevented from starting a
    second job.
-   Existing Tool Output behavior remains authoritative.
-   Existing menu build commands must remain consistent.

The combo may remain changeable while a job is running only if doing so
does not alter the running job; otherwise disable it. Choose and test a
deterministic behavior.

### Tool Output interaction

Starting a build from the toolbar should behave like starting it from
the Build menu.

Do not force Tool Output open unless the existing command path already
does so.

Do not create separate toolbar logs.

### Toolbar geometry and editor viewport

The toolbar consumes vertical editor space below the menu bar.

Update any hard-coded editor geometry assumptions introduced by earlier
milestones, including where relevant:

-   orientation widget position,
-   viewport bounds,
-   mouse-to-viewport interpretation,
-   placement ray coordinates,
-   picking,
-   gizmo interaction,
-   camera navigation,
-   overlays.

Do not merely draw the toolbar over the existing viewport.

The viewport must start below the toolbar when the toolbar is visible.

When hidden, geometry must return to the menu-only layout.

This is a critical M43 acceptance requirement.

### M42 placement integration

Object Palette placement must remain correct with the toolbar visible or
hidden.

Verify:

-   candidate follows the correct viewport ray,
-   toolbar clicks never place objects,
-   toolbar does not shift ray/picking incorrectly,
-   placement preview remains aligned with the mouse,
-   gizmo interaction remains correct,
-   toolbar ImGui capture prevents accidental viewport actions.

### Toolbar and Object Palette

These are complementary:

-   Object Palette = object creation workflow.
-   Quick Toolbar = frequent global/editor commands.

Do not merge them into one window.

The user should be able to hide Object Palette and still use the
toolbar.

### Workspace persistence

Add toolbar visibility to the established editor workspace state.

Requirements:

-   default visible,
-   View menu sync,
-   persistence consistent with existing workspace policy,
-   Reset Editor Layout restores default,
-   no CWD `imgui.ini`.

Build selection persistence is separate from level data and should use
the smallest existing editor preference mechanism.

### Keyboard/input capture

Toolbar interactions must be treated as ImGui UI interactions.

Clicking toolbar controls must not:

-   place M42 objects,
-   viewport-pick,
-   begin gizmo drag,
-   rotate/move editor camera,
-   trigger Delete-selected behavior.

Preserve current-frame ImGui capture rules.

### Debug and Release boundaries

Development:

-   full Quick Toolbar.

Debug:

-   do not expose Development authoring toolbar controls unless the
    existing compile-time/editor policy explicitly supports the
    underlying action.
-   Preferred M43 behavior: Quick Toolbar is part of Development
    authoring workspace only.

Release:

-   no editor toolbar.

Do not broaden build-configuration feature boundaries accidentally.

### Non-goals

M43 must not add:

-   undo/redo,
-   copy/paste,
-   play/pause simulation controls,
-   prefab controls,
-   Object Palette category buttons inside toolbar,
-   Save All,
-   Cook/Stage/Reload toolbar buttons,
-   customizable toolbar ordering,
-   user-defined toolbar actions,
-   command palette,
-   keyboard shortcut editor,
-   icon/font dependency,
-   docking framework rewrite,
-   generic action registry.

Those may be evaluated later if actual usage justifies them.

### Automated acceptance

Add/update focused tests for:

1.  Toolbar default visible.
2.  View toggle hides/shows toolbar.
3.  Reset restores default visibility.
4.  Translate toolbar action changes canonical TransformMode.
5.  Resize toolbar action changes canonical TransformMode.
6.  Transform menu reflects toolbar change.
7.  Toolbar reflects menu transform change.
8.  Resize availability matches canonical selection rules.
9.  Apply toolbar request uses existing Apply action.
10. Revert toolbar request uses existing Revert action.
11. Save toolbar request uses existing Save action.
12. Apply enabled/disabled state matches canonical command.
13. Save enabled/disabled state matches canonical command.
14. Build selector defaults to Development.
15. Debug selection persists.
16. Development selection persists.
17. Release selection persists.
18. All selection persists.
19. Invalid persisted selection falls back safely.
20. Build Debug maps to existing command.
21. Build Development maps to existing command.
22. Build Release maps to existing command.
23. Build All maps to existing command.
24. Busy runner prevents second build.
25. Toolbar interaction consumes UI pointer.
26. Toolbar visible viewport top/bounds are correct.
27. Toolbar hidden viewport top/bounds are correct.
28. Orientation widget position accounts for toolbar height.
29. M42 placement ray remains correct with toolbar visible.
30. M42 placement ray remains correct with toolbar hidden.
31. Picking remains correct with toolbar visible/hidden.
32. Gizmo remains correct with toolbar visible/hidden.
33. F2 lifecycle does not leave stale toolbar interaction state.
34. Existing Build menu behavior remains unchanged.
35. Existing Level menu behavior remains unchanged.
36. Existing Transform menu behavior remains unchanged.
37. Level Format v1 unchanged.

### Manual acceptance

#### Toolbar visibility

1.  Run Development.
2.  Open F2.
3.  Confirm Quick Toolbar is visible below menu bar.
4.  Toggle `View > Quick Toolbar`.
5.  Confirm toolbar disappears and viewport reclaims space.
6.  Toggle it on again.

#### Transform

1.  Select a Platform.
2.  Click Translate toolbar control.
3.  Confirm Translate active and Resize inactive.
4.  Click Resize.
5.  Confirm Resize active and Translate inactive.
6.  Change mode through Transform menu.
7.  Confirm toolbar updates immediately.

#### Apply / Revert

1.  Create or modify a pending object.
2.  Use toolbar Apply.
3.  Confirm same behavior as Level \> Apply Preview.
4.  Create another pending modification.
5.  Use toolbar Revert.
6.  Confirm same behavior as Level \> Revert Working Copy.

Restore canonical state as necessary.

#### Save

Use only after ensuring active state is canonical or with a deliberate
temporary test that will be restored.

Verify:

-   Save availability is correct.
-   Toolbar Save invokes the existing Save behavior.
-   No implicit Apply occurs.

#### Build selector

1.  Confirm initial/default selection is Development when no preference
    exists.
2.  Select Debug and run.
3.  Verify Debug build command runs through Tool Runner.
4.  Select Development and run.
5.  Select Release and run.
6.  Select All and run.
7.  Confirm one-job-at-a-time behavior.

No need to repeat expensive builds manually if automated command-mapping
tests plus one or two real toolbar launches sufficiently validate the UI
path.

#### Build persistence

1.  Choose a non-default build option.
2.  Close/reopen the Development editor/application as appropriate.
3.  Confirm the last selection is restored.

#### M42 regression

With toolbar visible:

1.  Open Object Palette.
2.  Enter Collectible placement.
3.  Move mouse across Ground/Platform/empty space.
4.  Confirm candidate alignment remains correct.
5.  Click toolbar controls while placement mode is active.
6.  Confirm toolbar click does not create an object.
7.  Return to viewport and place normally.
8.  Test pending viewport picking.
9.  Revert.

Repeat the key alignment check with toolbar hidden.

#### Input safety

Verify toolbar clicks do not:

-   select world objects,
-   place objects,
-   manipulate gizmo,
-   move/rotate camera.

#### Layout

Verify:

-   toolbar does not overlap menu bar,
-   toolbar does not overlap viewport content,
-   orientation widget remains correctly positioned,
-   no CWD `imgui.ini`.

### Regression suites

Run all applicable configurations for:

``` text
AuthoredObjectLifecycleTest
AuthoredLifecycleIntegrationTest
EditorWorkspaceTest
EditorGizmoTest
EditorOrientationTest
EditorPickingTest
EditorPlacementTest
LevelFileTest
PhysicsRebuildTest
CookStageReloadWorkflowTest
EditorToolRunnerTest
```

plus new M43 toolbar tests.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Then:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

### Canonical safety

Before M43 closure confirm canonical Level 01:

-   Platforms = 6
-   Checkpoints = 2
-   Hazards = 2
-   Collectibles = 3
-   Camera FOV = 40

Run:

``` text
git diff -- game/assets/source/levels/level_01.level
git diff --check
```

Expected:

-   no intended canonical level diff,
-   diff check clean.

If only the known EOL/LF-CRLF artifact appears and there is no intended
semantic level change, use the established safe worktree restore:

``` text
git restore --worktree --source=HEAD -- game/assets/source/levels/level_01.level
```

Do not normalize EOL.

### Documentation

Update as appropriate:

-   `README.md`
-   `AGENTS.md`
-   `docs/ARCHITECTURE.md`
-   `docs/MILESTONES.md`

Document:

-   Quick Toolbar scope.
-   View toggle.
-   Shared command/state authority.
-   Transform mutual exclusivity.
-   Apply/Revert/Save mappings.
-   Build selector and persistence.
-   Build runner mapping.
-   Toolbar impact on viewport geometry.
-   Development-only boundary.
-   No Level Format change.

### Completion criteria

M43 is complete only when:

-   Quick Toolbar is fixed below the menu bar.
-   View toggle works.
-   Toolbar visibility participates in workspace state.
-   Translate/Resize share canonical TransformMode.
-   Apply/Revert/Save share canonical actions.
-   Build selector supports Debug/Development/Release/All.
-   Development is the first-run default.
-   Last build selection persists.
-   Run invokes existing EditorToolRunner build commands.
-   Busy-state behavior is safe.
-   Toolbar geometry correctly adjusts the viewport.
-   M42 placement/picking/gizmo behavior remains correct with toolbar
    visible/hidden.
-   Automated tests pass.
-   Manual acceptance passes.
-   Debug/Development/Release builds pass.
-   Canonical level is restored.
-   Git checks are clean.
-   No unrelated scope is introduced.

### Git closure

Do not commit/push/merge until manual acceptance is approved.

After approval:

``` text
test
-> approve
-> inspect diff/status
-> commit once
-> push milestone branch
-> merge normally into main
-> push main
-> confirm clean/synced main
```

Do not use `--no-ff` automatically.


## Milestone 44 --- Legacy Prototype Scene Cleanup

**Branch:** `milestone/44-legacy-prototype-scene-cleanup`\
**Status:** CLOSED. Merged to main. Milestone 45 is Authored Dynamic Physics Objects.\
**Prerequisite:** Milestone 43 --- Editor Quick Toolbar --- CLOSED\
**Scope:** Audit and remove obsolete prototype/test visuals from the
playable/editor scene while preserving useful automated asset-pipeline
coverage and intentional gameplay geometry.

### Goal

Clean the Level 01 runtime/editor scene of legacy visual probes and
prototype objects introduced during early rendering, asset-pipeline,
physics, and authoring milestones.

The cleanup must distinguish between obsolete scene instances, useful
automated test assets, intentional gameplay/editor geometry, and physics
probes. Do not delete an object merely because it looks temporary.

### Phase A --- provenance audit

Before changing runtime behavior, identify every suspicious prototype
object. For each candidate report its visible description, code/data
symbol, source file, runtime creation path, render path, physics path if
any, LevelDefinition/Level Format relationship, Hierarchy relationship,
automated-test dependencies, asset-pipeline dependencies, and proposed
disposition.

At minimum audit:

-   plain white/grey test cube;
-   pink/black checker-textured cube;
-   larger textured test cube;
-   orange wedge/ramp-like object;
-   `textures/test_checker.png`;
-   `models/test_static.glb`;
-   `models/test_authored.glb`;
-   `models/test_textured.glb`;
-   both canonical slopes;
-   moving platform;
-   dynamic cyan box;
-   the four historical cooker probes intentionally excluded from
    Hierarchy.

Allowed dispositions:

-   Keep in canonical scene
-   Remove scene instance, keep test asset
-   Remove scene instance and obsolete code
-   Defer because purpose remains intentional/uncertain

Do not perform broad deletion before this audit.

### Core policy

Separate **test coverage** from **canonical scene clutter**.

If an asset exists only to prove cooking/loading/texturing, prefer
keeping the asset and automated test while removing its always-visible
runtime/editor scene instance.

If an object is intentional gameplay geometry or required by current
semantics, keep it.

### Slopes

The canonical world historically has exactly two slopes, and M42
placement treats slopes as eligible placement surfaces.

Do not remove a slope merely because the orange wedge looks like a
prototype. First prove whether the marked wedge is a canonical slope. If
it is, keep it. If it is only an obsolete visual proxy, remove only that
proxy.

M44 must not silently change slope gameplay.

### Moving platform

Audit the moving platform and determine whether it remains an
intentional gameplay mechanic/regression fixture. Do not remove it
automatically.

### Dynamic cyan box

Audit the dynamic body introduced during physics validation. If it is
only a legacy runtime probe, prefer removing it from the canonical scene
while preserving focused physics coverage where useful.

Do not remove Jolt/dynamic-body infrastructure merely because a probe is
removed.

### Asset-pipeline probes

Trace:

-   `textures/test_checker.png`
-   `models/test_static.glb`
-   `models/test_authored.glb`
-   `models/test_textured.glb`

Determine which are visibly instantiated in Level 01.

Preferred policy:

-   automated cooker/staging/model/texture coverage remains;
-   test assets remain if tests require them;
-   visible canonical probe instances are removed when they have no
    gameplay/editor purpose.

The four historical cooker probes were intentionally excluded from
Hierarchy. Audit whether these correspond to the user-visible legacy
objects.

### Phase B --- minimal cleanup

After the audit, perform only justified cleanup:

-   remove obsolete always-visible probe instances;
-   keep useful test assets/tests;
-   preserve intentional slopes;
-   preserve/remove moving and dynamic probes only according to
    evidence;
-   remove dead scene-specific render/setup code made unreachable.

Do not turn M44 into a scene architecture rewrite.

### Level Format and lifecycle

Preferred outcome: **no Level Format v1 schema change**.

Do not add fields just to hide prototype probes. Do not introduce Level
Format v2.

Preserve all M41--M43 contracts:

-   active/workingCopy;
-   StructuralIndexMap;
-   Platforms/Checkpoints/Hazards/Collectibles;
-   Spawn/Ground/Camera/Goal;
-   support-index metadata;
-   Object Palette and placement;
-   pending visuals/picking;
-   Quick Toolbar.

Do not force legacy probes into the authored lifecycle system merely to
remove them.

### Runtime/resource cleanup

For obsolete instances:

-   remove always-visible draw calls;
-   remove scene-only transforms/constants no longer needed;
-   remove scene-only model handles if no remaining runtime/test path
    needs them;
-   preserve RAII/resource lifetime;
-   avoid loading an asset at runtime solely for a removed visual unless
    a current runtime test requires it.

### Physics/body-budget safety

If a removed probe has a physics body, audit whether the body is still
required.

M41 Platform capacity is tied to the Jolt body budget. If canonical
non-Platform body count changes:

-   recompute actual capacity;
-   keep accounting centralized/static-asserted;
-   update parser/lifecycle capacity consistently;
-   add regression coverage.

Do not change Platform capacity unless the actual body accounting
changes.

### Editor and placement safety

Removed probes must no longer clutter the viewport, intercept picking,
create hidden proxies, or affect placement surfaces.

Intentional Ground/Platform/Slope placement remains correct.

Verify M42 candidate ray, fallback, pending picking, and gizmo behavior.

Verify M43 Quick Toolbar remains unaffected, visible and hidden.

### Visual acceptance target

The canonical scene should read as an intentional platformer level
rather than an engine test room.

The user-marked obsolete cubes should disappear if the audit confirms
they are probes. Any retained unusual object must have a clear current
purpose documented in the report.

## Non-goals

Do not add new art, terrain, lighting overhaul, gameplay mechanics,
Level Format v2, prefabs, scene graph, asset browser, singleton deletion
UI, broad Jolt refactor, or asset-pack redesign.

### Automated validation

Add focused M44 coverage where appropriate for:

1.  removed probe instances no longer contributing to canonical
    rendering;
2.  retained slope count/behavior;
3.  moving-platform disposition;
4.  dynamic-body disposition;
5.  useful asset tests remaining covered;
6.  picking excluding removed probes;
7.  placement surfaces remaining Ground/Platform/Slope;
8.  Level Format v1 round-trip unchanged;
9.  body-budget/Platform capacity correctness if physics accounting
    changes;
10. M41 lifecycle regression;
11. M42 placement regression;
12. M43 toolbar regression.

Run all applicable configurations for:

    AuthoredObjectLifecycleTest
    AuthoredLifecycleIntegrationTest
    EditorWorkspaceTest
    EditorGizmoTest
    EditorOrientationTest
    EditorPickingTest
    EditorPlacementTest
    EditorQuickToolbarTest
    LevelFileTest
    PhysicsRebuildTest
    CookStageReloadWorkflowTest
    EditorToolRunnerTest

Also run existing asset/model/texture tests found during the audit.

Run:

    python tools/test_stage_runtime_assets.py
    python tools/test_cook_level_v1.py
    python tools/test_cook_runtime_png.py

and relevant existing GLB/asset cooker tests.

Then:

    cmake --preset windows-vs2022
    cmake --build --preset windows-debug
    cmake --build --preset windows-development
    cmake --build --preset windows-release

### Manual acceptance

#### Visual cleanup

Launch canonical Level 01 and inspect the region where old test objects
were visible. Confirm every removed object is gone and every retained
unusual object matches the audit.

#### Slopes

Locate both canonical slopes, traverse them, verify collision, and in F2
verify Object Palette surface placement over a slope.

#### Moving platform

If retained, verify motion and CharacterVirtual interaction. If removed,
confirm the audit justification and absence of stale render/physics
artifacts.

#### Dynamic body

If retained, document why. If removed, confirm no legacy dynamic probe
remains and physics regressions pass.

#### Asset probes

Confirm obsolete white/checker/textured test cubes no longer clutter the
scene if classified as probes. Verify cooker, staging and runtime
startup still work.

#### Editor

Verify Hierarchy, picking, Object Palette, pending Add/Modify/Delete
visuals, Quick Toolbar and orientation widget.

#### Gameplay

Perform a short run covering movement, jump, slopes, moving platform if
retained, checkpoint, hazard, collectible, goal and respawn/restart as
practical.

### Canonical safety

Unless the audit proves an authored record itself is obsolete, preserve:

-   Platforms = 6
-   Checkpoints = 2
-   Hazards = 2
-   Collectibles = 3
-   FOV = 40

Run:

    git diff -- game/assets/source/levels/level_01.level
    git diff --check

If there is no intended semantic level change and only the known EOL
artifact appears:

    git restore --worktree --source=HEAD -- game/assets/source/levels/level_01.level

Do not normalize EOL.

If the audit requires an intentional authored level change, STOP and
report it before treating it as accepted.

### Documentation

Update as appropriate:

-   `README.md`
-   `AGENTS.md`
-   `docs/ARCHITECTURE.md`
-   `docs/MILESTONES.md`

Document identified probes, removed scene instances, retained test
assets, slopes/moving-platform/dynamic-body disposition, body-budget
changes if any, and Level Format impact.

### Required report

Return:

1.  branch and baseline
2.  provenance audit
3.  white/grey cube identity/disposition
4.  checker cube identity/disposition
5.  larger textured cube identity/disposition
6.  orange wedge identity/disposition
7.  `test_checker.png` disposition
8.  `test_static.glb` disposition
9.  `test_authored.glb` disposition
10. `test_textured.glb` disposition
11. slope 0 disposition
12. slope 1 disposition
13. moving platform disposition
14. dynamic cyan box disposition
15. cooker-probe/Hierarchy relationship
16. removed render paths
17. removed runtime loads/resources
18. removed physics bodies, if any
19. retained test-only assets
20. staging inventory changes, if any
21. cooker-test changes, if any
22. body-budget before/after
23. Platform capacity before/after
24. Level Format v1 impact
25. LevelDefinition impact
26. editor picking impact
27. placement-surface impact
28. M41 lifecycle regression
29. M42 placement regression
30. M43 toolbar regression
31. focused M44 tests
32. regression-suite results
33. asset/model/texture regression results
34. Python staging
35. Python level cooker
36. Python PNG cooker
37. CMake configure
38. Debug build
39. Development build
40. Release build
41. manual visual cleanup status
42. manual slopes status
43. manual moving-platform status
44. manual dynamic-body status
45. manual editor status
46. manual gameplay status
47. canonical counts
48. FOV 40
49. canonical level diff
50. git diff --check
51. docs
52. no commit
53. no push
54. no merge
55. M44 incomplete pending manual approval
56. M45 not started
57. STOP

### Implemented (this branch)

Provenance audit classified the user-marked spawn-area cubes as the
M15–M19 cooker probes (hard-coded renderer instances, never Hierarchy,
never Level Format). They are removed from canonical load/draw. Source,
cooked, and staged files plus Python cooker/staging tests remain.

The spawn-area orange pyramid is `models/test_static.glb`, not a
gameplay slope. Slope 0 (30° walkable) and slope 1 (60° steep) stay.
The moving platform stays. Authored `dynamic_box` stays in Level Format
v1 for parser/writer compatibility; it is not a Hierarchy/Inspector
scene object, not drawn, not picked, and has no Jolt body. Platform leftover is 59. No Level Format schema change and
no semantic `level_01.level` edit.

Correction 1: remove Dynamic Cyan Box from Development Hierarchy and
the obsolete Inspector path. `IsValidSelection(DynamicBox)` is false.

Focused coverage: `CanonicalSceneCleanupTest`. M44 is not complete until
manual acceptance.

### Stop condition

After audit, minimal implementation, automated validation, builds and
report:

**STOP.**

Do not commit.\
Do not push.\
Do not merge.\
Do not start M46 from this closed milestone.

M44 is CLOSED and merged.


## Milestone 45 --- Authored Dynamic Physics Objects

**Branch:** `milestone/45-authored-dynamic-physics-objects` **Status:**
Implemented on `milestone/45-authored-dynamic-physics-objects`. Awaiting
manual acceptance. Not complete. Milestone 46 has not started.
**Prerequisite:** M44 CLOSED

### Goal

Replace the useful behavior once demonstrated by the legacy Dynamic Cyan
Box with a real authored feature: repeatable box-shaped dynamic rigid
bodies that can be created in the Development editor, persisted in the
level, instantiated through Jolt, and physically pushed by the player.

Each Dynamic Box has authored center, size, and mass in kilograms. M45
is intentionally box-only and is not a general physics-object framework.

### Authoring

Add `Dynamic Box` as a repeatable authored category alongside Platform,
Checkpoint, Hazard and Collectible.

Support: - Object Palette placement - Edit \> Add - Duplicate Selected -
Delete Selected - Translate - Resize - Inspector center/size/mass
editing - Apply/Revert/Save - pending Add/Modify/Delete visuals - active
and pending viewport picking

Hierarchy uses a `Dynamic Boxes` group. Do not restore the legacy
singleton `Dynamic Cyan Box`.

Preferred defaults: size `{1,1,1}`, mass `30 kg`. Object Palette
placement follows M42 Ground/Platform/Slope surface-hit and fallback
behavior.

### Data and validation

Use a value-level definition equivalent to:

`DynamicBoxDefinition { center, size, massKg }`

Do not expose Jolt internals in LevelDefinition.

Size components must be finite and positive; reuse the established 0.12
minimum extent where appropriate. Mass must be finite, \> 0, and have
one centralized defensible safety maximum. Reject zero, negative, NaN
and infinity.

### Level Format v1 migration

M44 retained the legacy singleton `dynamic_box` record only for
compatibility. M45 should evolve the existing syntax into a repeatable
record:

`dynamic_box <cx> <cy> <cz> <sx> <sy> <sz> <massKg>`

Prefer keeping Level Format v1. Allow zero, one or multiple records in
encounter order; no count field. Migrate the in-memory singleton to a
vector/list.

If a safe v1-compatible migration is impossible, STOP and report before
introducing Level Format v2.

The canonical M44 legacy `dynamic_box` line must not silently resurrect
the old probe. Preferred canonical Level 01 migration is Dynamic Boxes =
0. Its removal is an intentional semantic source diff and must be
reported; do not erase it with the historical EOL restore.

Preserve file safety bounds: 64 KiB, 256 lines, 512 chars/line.

### Runtime physics

Each authored Dynamic Box creates one Jolt dynamic rigid body with: -
box collision matching authored size - authored mass - gravity -
collision with intentional world geometry - ordinary contact-based
interaction with CharacterVirtual

Do not fake pushing with manual player-input translation.

After instantiation, Jolt pose is authoritative for runtime rendering.
Authored transform remains authoritative for rebuild/reset/reload/save.
Never continuously overwrite the simulated body pose from authored data.

### Rendering

Render from current Jolt pose with a simple intentional
greybox/debug-gameplay appearance. Do not restore the hard-coded cyan
probe renderer or any M15--M23 legacy scene probe.

### Physics rebuild

Integrate Dynamic Boxes into transactional `PhysicsWorld::TryRebuild`.
Apply reconstructs bodies from active authored definitions. Failure
preserves the previous active/runtime world.

### Shared body capacity

M44 state: - `kPhysicsMaxBodies = 64` - fixed non-Platform usage = 5 -
Platform-only capacity = 59

M45 makes Dynamic Box count variable. Replace the independent
Platform-only assumption with a shared authored-body budget derived from
real body accounting.

At minimum enforce semantically:

`fixedBodies + platformCount + dynamicBoxCount <= kPhysicsMaxBodies`

Lifecycle Add/Duplicate and level validation must reject overflow
deterministically. Reuse `AtLimit` if semantically sufficient. Do not
create arbitrary independent quotas that can exceed Jolt capacity.

### Player interaction and reset

Player must be able to push Dynamic Boxes through ordinary collision.

Define deterministic reset behavior. Preferred: - full run restart
resets boxes to authored poses and zero velocities - Apply/reload
reconstruct from authored definitions - checkpoint respawn alone does
not reset all boxes unless existing run semantics require it

Audit current restart boundaries and document/test the chosen policy.

### Editor semantics

Inspector edits workingCopy only. Gizmo Translate/Resize edits
workingCopy only. Do not drag the live Jolt body with editor gizmos.

Pending ghost represents authored proposal while the active body may
continue simulating.

Active picking must use current runtime physics pose, not stale authored
center. Extend M41 structural mapping to Dynamic Boxes; no GUIDs.

Save serializes active authored definitions, never transient simulated
poses. A box authored at X=2 and pushed to X=8 still saves X=2 unless an
authored edit was applied.

### Reload / cook / stage

M39 reload reconstructs boxes from staged authored definitions and
resets their runtime poses/velocities. No special hot reload system.

No new external asset is required. Preserve M40 Cook → Stage → Reload
workflow.

### Build boundaries

Runtime Dynamic Boxes work in Debug, Development and Release. Authoring
UI remains Development-only.

### Tests

Add focused tests for: - zero/one/multiple records - encounter order and
deterministic round-trip - malformed record - invalid mass/size
including NaN/Inf - exact shared body-budget limit accepted and one-over
rejected - one definition creates one Jolt dynamic body - authored size
and mass mapping - gravity/Ground collision - deterministic contact/push
behavior where practical - rebuild/restart reset - transactional rebuild
failure - body removal after authored Delete + Apply -
Add/Duplicate/Delete/Translate/Resize - Inspector mass edit - pending
lifecycle visuals/identity - active moving-body picking - pending
picking - Object Palette placement - capacity rejection - Save authored
pose rather than simulated pose

Run all applicable configs for: `AuthoredObjectLifecycleTest`,
`AuthoredLifecycleIntegrationTest`, `CanonicalSceneCleanupTest`,
`EditorWorkspaceTest`, `EditorGizmoTest`, `EditorOrientationTest`,
`EditorPickingTest`, `EditorPlacementTest`, `EditorQuickToolbarTest`,
`LevelFileTest`, `PhysicsRebuildTest`, `CookStageReloadWorkflowTest`,
`EditorToolRunnerTest`, plus new M45 tests.

Run: `python tools/test_stage_runtime_assets.py`
`python tools/test_cook_level_v1.py`
`python tools/test_cook_runtime_png.py`

Then configure and build Debug, Development and Release with the
existing Windows presets.

### Manual acceptance

In Development/F2: 1. Place Dynamic Box from Object Palette. 2. Verify
pending visual and Hierarchy. 3. Apply and verify a real dynamic body.
4. Walk into it and verify physical pushing/gravity/Ground collision. 5.
Compare same-size boxes with meaningfully different masses such as 5 kg
and 100 kg. 6. Resize, Apply and verify render/collision dimensions. 7.
Translate workingCopy and verify live body does not teleport until
Apply. 8. Push a box away, Save, and verify simulated pose is not baked
into authored source. 9. Test Duplicate/Delete with Apply/Revert. 10.
Push boxes and test documented restart reset. 11. Test staged runtime
reload reconstruction. 12. Verify slopes, moving platform, checkpoints,
hazards, collectibles, goal, Object Palette and Quick Toolbar. 13.
Verify no legacy white/checker/textured/orange probes or old singleton
Dynamic Cyan Box return.

### Canonical target

After intentional migration: - Platforms = 6 - Checkpoints = 2 - Hazards
= 2 - Collectibles = 3 - Dynamic Boxes = 0 preferred - FOV = 40

Run `git diff --check` and inspect the exact `level_01.level` diff. The
intentional legacy dynamic_box removal must not be hidden by
`git restore`.

### Documentation

Update README, AGENTS, architecture, milestones and LEVEL_FORMAT_V1 docs
as appropriate. Document repeated Dynamic Boxes, mass units/bounds,
shared body budget, authored-vs-runtime pose, reset semantics,
lifecycle/editor support and legacy migration.

### Non-goals

No spheres/capsules/cylinders, arbitrary convex or dynamic GLB
collision, friction/restitution editor, density calculation, joints,
ropes, ragdolls, grab/carry mechanic, physics-layer UI, prefabs, GUIDs,
undo/redo or Level Format v2 without explicit review.

### Completion

M45 requires repeatable authored Dynamic Boxes, validated mass/size,
shared body capacity, editor lifecycle/placement, real Jolt bodies,
player pushing, runtime-pose rendering, authored-pose saving,
deterministic restart/reload, no legacy probe regression, passing
tests/builds and manual acceptance.

### Stop

After implementation, automated validation and report: STOP.

Do not commit, push, merge or start M46. M45 remains incomplete until
explicit manual approval.


## Milestone 46 — Dynamic Box Runtime Recovery

### Status

**Implemented on `milestone/46-dynamic-box-runtime-recovery`. Awaiting mandatory manual user acceptance. Not CLOSED.**

This milestone remains OPEN until all of the following are complete:

1. implementation on the milestone branch;
2. automated validation;
3. manual user acceptance;
4. Git pre-closure inspection;
5. one meaningful milestone commit;
6. milestone branch push;
7. normal merge into `main`;
8. `main` push;
9. final confirmation that `main` is clean and synchronized.

A Cursor implementation report alone does **not** close M46.

---

### Official title

**M46 — Dynamic Box Runtime Recovery**

### Exact Git branch

`milestone/46-dynamic-box-runtime-recovery`

---

### Baseline / prerequisites

M46 starts from the post-M45 repository state.

Required baseline:

- M00–M45 are CLOSED.
- Canonical branch is `main`.
- `main` is clean and synchronized with its remote.
- M45 — Authored Dynamic Physics Objects is fully integrated.
- M45 Correction 1 is included.
- The current repository, current tests, and current documentation are the source of truth.
- `PROJECT_STATE_M45.md` is the architectural checkpoint.
- `DEVELOPMENT_WORKFLOW.md` defines the required milestone workflow.

Before changing code, inspect the actual current M45 Dynamic Box/Jolt implementation and identify the smallest existing runtime/physics authority where recovery can be implemented safely. Do not presume that any particular class, method, or API exists merely because this specification describes responsibilities.

---

### Post-M45 context

After M45, Dynamic Boxes are repeatable authored rigid bodies in Level Format v1.

Each Dynamic Box has authored properties including:

- center;
- size;
- mass.

Applied Dynamic Boxes create Jolt dynamic box bodies.

The key existing authority invariant is:

> Authored state and simulated runtime state are distinct.

The active authored Dynamic Box definition describes the authoritative applied authoring state. Jolt owns transient runtime pose and velocity during simulation.

Rendering and active picking use the current runtime Jolt pose. Saving continues to serialize authored state rather than silently capturing simulated physics state.

Current reset semantics established by M45:

- full Restart Run restores Dynamic Boxes from authored transforms and clears runtime velocities;
- checkpoint respawn does **not** reset Dynamic Boxes;
- Apply rebuilds from applied authored definitions;
- staged runtime reload rebuilds from staged authored definitions.

M46 extends this runtime behavior with **individual out-of-world recovery** while preserving all of those distinctions.

---

### Problem

A Dynamic Box can be pushed or fall out of the playable area.

Without a defined recovery rule, an authored physics object may become permanently unavailable during the current run even though its authored definition remains valid.

A naïve implementation could introduce regressions by:

- rebuilding the complete PhysicsWorld;
- resetting every Dynamic Box when only one fell;
- mutating editor authoring state from runtime simulation;
- changing Modified/Dirty semantics;
- persisting simulated positions through Save;
- conflating checkpoint respawn, Restart Run, Apply, reload, and runtime recovery.

M46 must solve this narrowly and deterministically.

---

### Goal

When an **active Dynamic Box runtime body** falls below the currently applied authored level kill plane, recover **only that Dynamic Box runtime instance** to its currently applied authored transform and clear its runtime motion.

The recovery must be:

- individual;
- runtime-only;
- deterministic;
- based on `active.killPlane`;
- based on the Dynamic Box's currently applied authored definition;
- independent from editor pending state;
- independent from other Dynamic Boxes;
- performed without rebuilding the entire PhysicsWorld.

---

### Core architectural decisions

#### 1. Reuse the existing authored kill plane

Recovery threshold is exactly the currently applied authored level kill plane:

`active.killPlane`

No additional margin is introduced.

No Dynamic-Box-specific recovery threshold is introduced.

No new Level Format field is introduced.

Level Format remains **v1**.

#### 2. Recovery is runtime-only

Crossing below `active.killPlane` is a runtime simulation event.

It must not perform an authoring operation.

It must not modify:

- `workingCopy`;
- active authored Dynamic Box definitions;
- `savedSourceBaseline`;
- Modified state;
- Dirty state;
- serialized Save output.

#### 3. Recover only the fallen Dynamic Box

If one Dynamic Box is below the kill plane, only its corresponding runtime body is reset.

Other Dynamic Boxes must preserve their own current runtime:

- position;
- orientation;
- linear velocity;
- angular velocity.

No global Dynamic Box reset and no full PhysicsWorld rebuild are allowed for individual recovery.

#### 4. Recovery transform comes from applied authored authority

The recovery transform must come from the Dynamic Box definition in the **currently applied authored state**.

Do not use:

- current `workingCopy` pending edits;
- the last saved source baseline unless it is also the active applied state;
- the current simulated transform;
- a hard-coded spawn position.

#### 5. Orientation returns to authored/initial state

The current Level Format v1 Dynamic Box record authors center, size, and mass and does not introduce a new orientation field in M46.

The implementation must inspect the actual current M45 body creation representation and restore the runtime orientation to the same authored/initial orientation used when that active Dynamic Box body was constructed.

M46 must not invent a new authored orientation representation or Level Format field.

#### 6. Runtime velocities are cleared

After individual recovery:

- linear velocity = zero;
- angular velocity = zero.

The body must not retain pre-fall momentum after being restored.

#### 7. No generic respawn framework

Implement the smallest coherent Dynamic Box runtime recovery path supported by the current architecture.

Do not introduce a generalized "respawnable physics entity" abstraction or unrelated framework.

---

### Authored authority vs runtime authority

The existing editor state model remains unchanged:

- `workingCopy`: pending authored edits;
- `active`: applied authored world;
- `savedSourceBaseline`: saved-source baseline;
- Modified = `workingCopy != active`;
- Dirty = `active != savedSourceBaseline`.

M46 adds no fourth authored authority.

For Dynamic Boxes:

- authored transform/data comes from `active`;
- simulated pose/velocity comes from Jolt;
- recovery uses the active authored transform as the reset target;
- after recovery, Jolt again owns subsequent transient motion.

Runtime recovery must not cause any authored-state comparison to change.

---

### Exact Dynamic Box Runtime Recovery behavior

For each active Dynamic Box runtime instance:

1. observe its actual current runtime Jolt pose using the established runtime authority;
2. read the current runtime Jolt body center/position Y;
3. if runtimeBodyCenterY < active.killPlane, recover that instance;
4. if the Dynamic Box is below `active.killPlane`, recover that instance;
5. restore its runtime position to the currently applied authored center/transform;
6. restore its runtime orientation to the authored/initial orientation represented by the current implementation;
7. set linear velocity to zero;
8. set angular velocity to zero;
9. preserve all other Dynamic Box runtime instances unchanged;
10. continue normal Jolt simulation after recovery.

The implementation must use the actual current project conventions for the precise body transform update and activation/wake semantics required by Jolt.

Do not invent an API in advance; inspect the repository and use the smallest existing authority boundary.

---

### Multiple Dynamic Boxes

M46 must work correctly with zero, one, or multiple Dynamic Boxes.

With multiple boxes:

- each runtime instance remains independent;
- recovery is determined per instance;
- one fallen box must not reset another valid box;
- another box's runtime pose and velocity must survive unchanged;
- capacity accounting remains shared with Platforms exactly as before.

Multiple boxes may recover independently if multiple bodies independently satisfy the recovery condition.

---

### PhysicsWorld / Jolt integration

The implementation must inspect the current M45 architecture and place:

- recovery detection; and
- individual runtime body reset

at the smallest existing runtime/physics authority that preserves ownership and avoids duplicated state.

Required constraints:

- no full PhysicsWorld rebuild for individual recovery;
- no alteration to transactional `PhysicsWorld::TryRebuild` semantics;
- no new arbitrary body quota;
- no authored-state mutation from physics;
- no opportunistic physics architecture rewrite.

The implementation should use existing body ownership/mapping structures where suitable rather than introducing speculative identity systems.

---

### Shared physics body budget

The current physics capacity invariant remains unchanged.

Current fixed body accounting after M45 is 5:

- Ground;
- two slopes;
- moving platform kinematic body;
- CharacterVirtual inner body.

Platforms and Dynamic Boxes share:

`fixedBodies + platformCount + dynamicBoxCount <= 64`

Therefore the current authored constraint remains conceptually:

`platformCount + dynamicBoxCount <= 59`

M46 must not:

- increase `kPhysicsMaxBodies`;
- create a second independent Dynamic Box quota;
- add extra persistent recovery bodies;
- weaken validation;
- alter transactional rebuild guarantees.

---

### Interaction with Apply

Apply semantics remain unchanged.

Apply validates and promotes `workingCopy` to `active`, then reconstructs the active/runtime world through the established transactional path.

After a successful Apply:

- Dynamic Box runtime bodies are rebuilt from the newly applied authored definitions;
- future runtime recovery targets use those newly active authored transforms;
- pending `workingCopy` state that has not been applied must never be used as a recovery target.

M46 must not make runtime recovery invoke Apply.

---

### Interaction with Revert

Revert semantics remain unchanged:

`workingCopy = active`

Revert affects pending authored edits only.

Runtime recovery must not:

- trigger Revert;
- change whether Revert is available;
- use Revert as a recovery mechanism;
- reset runtime bodies simply because Revert occurred unless the current established editor flow already rebuilds runtime state for another valid reason.

Do not change existing Revert behavior outside what is strictly necessary for M46.

---

### Interaction with Save

Save remains authored-only.

Runtime recovery must not:

- write files;
- dirty authored state;
- change active authored transforms;
- serialize recovered runtime transforms;
- serialize the simulated pre-recovery transform.

If a box is authored at X=2, simulated to X=8, then falls and recovers, Save must still serialize its authored X=2 definition.

---

### Interaction with Restart Run

Existing M45 Restart Run semantics must be preserved.

Restart Run resets **all** Dynamic Boxes from the current authoritative authored run state according to the existing implementation and clears their runtime linear/angular velocity.

This is deliberately distinct from M46 runtime recovery:

- runtime recovery resets only each box that individually falls below the kill plane;
- Restart Run resets all Dynamic Boxes as part of the established full-run reset semantics.

Do not replace Restart Run with the individual recovery path if that would change existing run-reset behavior.

---

### Interaction with checkpoint respawn

Existing checkpoint respawn semantics must be preserved.

Respawning the player at a checkpoint does **not** reset valid Dynamic Boxes.

Example:

- box A is pushed to a new valid runtime position;
- player dies and respawns at a checkpoint;
- box A remains at its runtime position;
- if box A later falls below `active.killPlane`, only then does M46 recovery restore it.

Do not conflate player respawn with Dynamic Box recovery.

---

### Interaction with staged runtime reload

Existing Development staged runtime reload remains authoritative.

A successful staged reload:

- reloads staged authored level data through the established path;
- transactionally rebuilds runtime state;
- reconstructs Dynamic Boxes from that staged authored definition.

M46 runtime recovery must not change staged reload authority or introduce source/cooked fallback.

After reload, recovery targets must correspond to the newly active staged authored state.

---

### Rendering and picking

Existing M45 behavior remains unchanged:

- active Dynamic Box rendering follows current Jolt pose;
- active Dynamic Box picking follows current Jolt pose;
- pending editor picking follows existing workingCopy proxy rules.

After recovery, rendering and picking naturally observe the recovered Jolt runtime pose.

Do not add a separate visual-only recovery position.

---

### Development editor

Development retains full authoring/editor functionality.

M46 does not introduce a new editor command or UI.

Required Development behavior:

- runtime recovery can occur while exercising the Development runtime;
- it does not alter Modified/Dirty state;
- it does not change selection/lifecycle authority unnecessarily;
- pending authored edits remain governed by `workingCopy`;
- active runtime bodies remain governed by applied state and Jolt;
- Add/Duplicate/Delete/Translate/Resize, Apply/Revert/Save, picking and gizmos remain regressions to protect.

No "Reset Dynamic Box" button or menu item is added.

---

### Debug

Debug must compile and preserve the runtime behavior available in that configuration under the current project architecture.

M46 must not introduce Development-editor dependencies into Debug runtime code.

No new Debug-only authoring feature is required.

---

### Release

Release must compile without Development editor dependencies and preserve runtime Dynamic Box recovery where Dynamic Boxes exist in authored runtime data.

M46 must not expose editor-only UI or authoring systems in Release.

---

### Level Format

Level Format remains **v1**.

Existing Dynamic Box syntax remains unchanged:

`dynamic_box <cx> <cy> <cz> <sx> <sy> <sz> <massKg>`

No new:

- recovery threshold;
- respawn flag;
- orientation field;
- recovery position;
- recovery count;
- format version

is introduced.

---

### Canonical Level 01 safety

The final canonical source Level 01 must remain semantically:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The intentional M45 deletion of:

`dynamic_box 0 5 0 1 1 1 30`

must remain deleted.

Manual testing may use temporary Dynamic Boxes, but those objects must **not** be left saved in canonical Level 01.

Before reporting completion, explicitly inspect the semantic diff of:

`game/assets/source/levels/level_01.level`

Do not mechanically run a worktree restore that could accidentally restore the legacy Dynamic Box line. Only restore proven unintended worktree/EOL noise, and only after confirming that doing so cannot undo intentional semantic state.

---

### Required automated tests

Tests must exercise the real runtime/physics authority boundary rather than only isolated helper logic.

The final test layout may extend existing tests or add a focused test according to current repository conventions. Do not invent a test architecture before inspecting the repository.

Automated coverage must prove at least:

1. one active Dynamic Box whose runtime position falls below the active kill plane recovers to its active authored transform;
2. recovered linear velocity is zero;
3. recovered angular velocity is zero;
4. recovered runtime orientation is restored to the correct authored/initial orientation;
5. with multiple Dynamic Boxes, another valid box preserves its independent runtime pose and velocity;
6. recovery does not modify authored `LevelDefinition` state;
7. recovery does not modify `workingCopy` / `savedSourceBaseline` / Modified / Dirty semantics where those authorities are available at the tested integration boundary;
8. recovery works correctly after Apply/rebuild so the recovery target is the newly active authored transform;
9. Restart Run still resets all Dynamic Boxes according to M45 semantics;
10. checkpoint respawn still does not reset valid Dynamic Boxes;
11. staged runtime reload still reconstructs Dynamic Boxes from staged authored state and subsequent recovery uses that new active state;
12. zero Dynamic Boxes remains valid;
13. shared Platform + Dynamic Box capacity validation remains unchanged;
14. transactional PhysicsWorld rebuild behavior remains unchanged.
15. recovery threshold semantics must be tested using runtime body center Y, not box bottom/AABB extent or authored center.

Where editor authority cannot reasonably be asserted inside a low-level physics test, cover it at the appropriate integration boundary rather than creating artificial coupling.

---

### Required integration validation

Run the most relevant current integration suites, including those that cover:

- authored object lifecycle;
- lifecycle owner dispatch/integration;
- Dynamic Box physics/runtime behavior;
- editor picking;
- level parsing/serialization;
- physics rebuild/capacity/transactionality;
- canonical scene cleanup;
- Apply/Revert/Save authority;
- staged Cook/Stage/Reload workflow;
- editor workspace/tooling paths affected by the changes.

Current known relevant suites from the post-M45 checkpoint include:

- `AuthoredObjectLifecycleTest`
- `AuthoredLifecycleIntegrationTest`
- `CanonicalSceneCleanupTest`
- `EditorWorkspaceTest`
- `EditorGizmoTest`
- `EditorOrientationTest`
- `EditorPickingTest`
- `EditorPlacementTest`
- `EditorQuickToolbarTest`
- `LevelFileTest`
- `PhysicsRebuildTest`
- `CookStageReloadWorkflowTest`
- `EditorToolRunnerTest`

Inspect the actual current test targets and run those that exist and are relevant. If names or organization have changed in the repository, follow the current repository rather than this historical list.

---

### Required regression protection

M46 must preserve at minimum:

- M41 authored lifecycle behavior;
- M42 Object Palette placement behavior;
- M43 Quick Toolbar behavior;
- M44 canonical legacy-scene cleanup;
- M45 repeatable Dynamic Boxes;
- M45 Correction 1 lifecycle request dispatch protection;
- Dynamic Box Add/Duplicate/Delete;
- Dynamic Box Translate/Resize;
- Apply/Revert/Save;
- runtime render and picking from Jolt pose;
- pending editor proxy/picking rules;
- restart semantics;
- checkpoint respawn semantics;
- staged reload semantics;
- body capacity validation;
- transactional rebuild;
- Level Format v1 round-trip behavior;
- zero/one/multiple Dynamic Box validity.

---

### Build validation

Perform canonical Windows build validation:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

If the repository's current canonical commands have changed, use the current documented equivalents and report that explicitly.

All required configurations must build successfully before Cursor stops.

---

### Python / tooling regressions

Run the relevant existing Python regressions, including when applicable:

```powershell
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Because Level Format must remain v1 and staged reload semantics are a required regression, the level cooker/staging paths should be verified unless current repository organization demonstrates a different canonical command.

Report each executed Python regression and its result.

---

### `git diff --check`

Before the implementation report:

```powershell
git diff --check
```

must pass.

Also inspect:

```powershell
git diff -- game/assets/source/levels/level_01.level
```

and verify canonical semantics explicitly.

---

### Detailed manual acceptance checklist

Manual acceptance is mandatory after Cursor completes automated validation.

Use Development for authoring/runtime observation unless a specific check targets another build configuration.

#### A. Basic individual recovery

1. Start from canonical Level 01 without saving test objects.
2. Add one temporary Dynamic Box through the established editor workflow.
3. Apply it.
4. Push or otherwise cause the box to fall below the active level kill plane.
5. Confirm the box returns to its applied authored position.
6. Confirm it does not continue with obvious pre-fall linear momentum.
7. Confirm it does not continue spinning with pre-fall angular momentum.
8. Confirm its orientation returns to the expected initial/authored orientation.

#### B. Multi-box independence

1. Create and Apply at least two temporary Dynamic Boxes at distinguishable positions.
2. Move/push both into different runtime states.
3. Cause only box A to fall below the kill plane.
4. Confirm box A recovers.
5. Confirm box B remains at its independent runtime pose.
6. Confirm box B does not lose its independent runtime motion merely because box A recovered.

#### C. Checkpoint respawn distinction

1. Put a Dynamic Box in a changed but valid runtime position above the kill plane.
2. Trigger player death/checkpoint respawn.
3. Confirm the Dynamic Box remains in its runtime position.
4. Then cause that box to fall below the kill plane.
5. Confirm only the kill-plane event recovers it.

#### D. Restart Run distinction

1. Move multiple Dynamic Boxes away from authored positions.
2. Use Restart Run.
3. Confirm all Dynamic Boxes reset according to established M45 run-reset behavior.
4. Confirm this remains distinct from individual kill-plane recovery.

#### E. Apply semantics

1. Make a pending authored transform change to a Dynamic Box.
2. Before Apply, confirm runtime recovery does not start using the pending `workingCopy` transform.
3. Apply the change.
4. Cause the box to fall below the kill plane.
5. Confirm recovery now targets the newly applied authored transform.

#### F. Revert semantics

1. Create a pending Dynamic Box edit without applying it.
2. Revert.
3. Confirm the pending edit is discarded according to existing behavior.
4. Confirm runtime recovery has not introduced unexpected authored or runtime side effects.

#### G. Save authority

1. Apply a Dynamic Box at a known authored position.
2. Move it through runtime physics.
3. Optionally cause it to recover.
4. Save using the established workflow only if testing on a safe noncanonical level or with a plan to avoid persisting temporary canonical data.
5. Confirm Save represents the authored state, not the simulated pre-recovery or post-recovery transient state.
6. Ensure no temporary box remains in canonical Level 01.

#### H. Staged reload

1. Exercise the existing Cook/Stage/Reload path using safe test data/workflow.
2. Confirm reload reconstructs Dynamic Boxes from staged authored definitions.
3. Move a reloaded box below the active kill plane.
4. Confirm recovery targets the newly loaded active authored transform.

#### I. Picking/rendering

1. Push a Dynamic Box to a runtime position different from its authored center.
2. Confirm rendering follows that Jolt pose.
3. Confirm active picking selects it at that runtime pose.
4. Cause recovery.
5. Confirm rendering/picking immediately follow the recovered runtime Jolt pose.
6. Confirm there is no separate stale visual or picking proxy for active physics bodies.

#### J. Zero-box canonical scene

1. Remove/avoid all temporary test boxes.
2. Return to canonical Level 01 state.
3. Confirm the level runs correctly with zero Dynamic Boxes.
4. Confirm editor Hierarchy/placement/lifecycle behavior remains stable with zero boxes.

#### K. Canonical-data final verification

Before approving M46:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40
- legacy `dynamic_box 0 5 0 1 1 1 30` remains absent.

No temporary authored test object may remain saved.

---

### Documentation to update

Update only documentation whose description of current runtime behavior becomes incomplete after M46.

Likely candidates, subject to inspection of the current repository:

- `README.md`
- `AGENTS.md`
- `docs/ARCHITECTURE.md`
- `docs/MILESTONES.md`
- physics/editor/runtime behavior documentation that currently defines Dynamic Box semantics.

`docs/LEVEL_FORMAT_V1.md` should not gain a new syntax field because the format is unchanged. It should only be edited if current documentation needs a behavior clarification that genuinely belongs there.

Prefer describing the resulting current system instead of adding stale chronological history.

---

### Explicit non-goals

M46 must **not** implement:

- Level Format v2;
- any new Dynamic Box syntax field;
- per-box recovery threshold;
- kill-plane recovery margin;
- separate Dynamic Box kill plane;
- authored recovery position;
- authored orientation support;
- new physics shapes;
- spheres;
- capsules;
- cylinders;
- arbitrary convex/dynamic GLB collision;
- friction UI;
- restitution UI;
- density-based mass;
- physics material editor;
- joints;
- constraints;
- ropes;
- ragdolls;
- grab/carry;
- pressure plates;
- switches;
- doors;
- generic gameplay interaction systems;
- generic "respawnable physics entity" framework;
- physics-layer editor;
- prefabs;
- GUID identity;
- ECS migration;
- undo/redo;
- multi-select;
- generic hot-reload watcher;
- unrelated editor UX;
- unrelated refactors;
- M47 work.

---

### Completion criteria

M46 implementation is ready for manual acceptance only when all of the following are true:

- runtime recovery uses exactly the active authored kill plane;
- an individual fallen Dynamic Box is restored to its active authored transform;
- its runtime orientation is reset correctly using the current representation;
- linear velocity is zeroed;
- angular velocity is zeroed;
- other Dynamic Boxes preserve independent runtime state;
- no full PhysicsWorld rebuild is used for individual recovery;
- `workingCopy` is unchanged by recovery;
- active authored definitions are unchanged by recovery;
- `savedSourceBaseline` is unchanged by recovery;
- Modified/Dirty state is unchanged by recovery;
- Save remains authored-only;
- Restart Run semantics remain intact;
- checkpoint respawn semantics remain intact;
- Apply/Revert semantics remain intact;
- staged reload semantics remain intact;
- active rendering and picking still follow Jolt runtime pose;
- zero Dynamic Boxes remains valid;
- body budget/capacity validation is unchanged;
- transactional rebuild remains unchanged;
- Level Format remains v1;
- focused automated tests pass;
- relevant M41–M45 regressions pass;
- required Python/tooling regressions pass;
- Debug build passes;
- Development build passes;
- Release build passes;
- `git diff --check` passes;
- canonical Level 01 is semantically unchanged from the approved post-M45 state;
- documentation affected by the behavior is updated;
- Cursor provides the required detailed report.

After that, the user performs mandatory manual acceptance.

M46 is **not CLOSED** until manual approval and the complete Git closure procedure are performed and `main` is clean/synchronized.

---

### Cursor implementation report requirements

At the end of implementation and automated validation, Cursor must report:

- root implementation approach;
- exact files changed;
- where recovery detection lives;
- where individual body reset lives;
- how active authored state is used as the recovery source;
- how runtime/editor authority separation is preserved;
- how multi-box independence is guaranteed;
- how orientation reset is represented;
- how linear/angular velocity reset is performed;
- how Restart Run remains distinct;
- how checkpoint respawn remains distinct;
- how Apply/Revert remain correct;
- how staged reload remains authoritative;
- tests added or changed;
- focused test results;
- relevant regression results;
- Python/tooling results;
- Debug build result;
- Development build result;
- Release build result;
- canonical Level 01 verification;
- `git diff --check` result;
- any remaining risks or limitations.

---

### STOP condition

After implementation, automated validation, build validation, canonical-data verification, documentation updates, and the detailed report:

- Do **NOT** commit.
- Do **NOT** push.
- Do **NOT** merge.
- Do **NOT** checkout or start M47.
- Do **NOT** declare M46 CLOSED.
- **STOP.**

M46 then waits for mandatory manual user acceptance and subsequent Git closure.


## Milestone 47 --- Static GLB Asset Import & Registry

**Status:** Implemented; awaiting manual acceptance.\
**Branch:** `milestone/47-static-glb-asset-import-registry`

### Baseline

M00--M46 are CLOSED. `main` is clean and synchronized. M47 starts from
the actual post-M46 repository. Current code, tests, and current
documentation are the source of truth.

Before changing code, inspect the real repository: existing
source/cooked/staged asset layout, static GLB loader/cooker assumptions,
staging scripts, Development editor/menu/tooling, `EditorToolRunner`,
path-validation conventions, tests, and asset documentation. Reuse the
existing pipeline. Do not invent paths, APIs, manifests, file-dialog
abstractions, or registry formats before inspection.

### Strategic role

M47 begins the transition from engine construction toward productive
game-content creation:

**Import → Visualize → Instantiate → Place → Interact**

M47 owns only **Import**. The eventual level-building workflow should
make reusable content easy to import, find, instantiate, place,
transform, duplicate, save, and test. The planning screenshot is a
productivity/workflow reference only, not a UI or architecture
specification.

M48 Content Browser, M49 Static Props, M50 Prop Placement, and later
gameplay systems remain future scope.

### Problem

The project has an external asset pipeline, but no official
Development-facing authority for bringing a compatible external model
into canonical project content. A user should not have to guess where to
copy a GLB, which path is canonical, whether it is supported, how
cook/stage sees it, or whether an import collides with existing content.

Future asset browsing and Static Prop authoring need a small,
deterministic content authority rather than ad-hoc filesystem
conventions.

### Objective

Create the first official Development workflow for importing
**already-supported static `.glb` models**.

A successful import must:

1.  accept/select an external static GLB through Development tooling;
2.  validate the input against actual current pipeline constraints;
3.  import/copy it into the existing canonical source-asset
    organization;
4.  assign a normalized canonical project-relative asset identity;
5.  make it discoverable through a minimal asset registry/catalog
    authority;
6.  integrate with existing cook/stage without bypassing
    source/cooked/staged separation;
7.  reject unsafe, invalid, unsupported, or colliding imports
    deterministically;
8.  report success/failure clearly;
9.  make no LevelDefinition or canonical level changes.

### Architectural decisions

#### Static GLB only

M47 supports static `.glb` files already compatible with the current
pipeline. It does not add FBX, OBJ, glTF JSON/external resources,
animation, skeletons, universal conversion, material authoring, shader
import, or a general texture importer.

Existing concrete GLB restrictions remain authoritative.

#### Preserve source → cooked → staged authority

Import establishes canonical **source** content. Cooked content remains
generated by the cooker. Staged content remains runtime/deployment
content.

External source paths and staged paths must not become the persistent
authored identity of an asset.

#### Path-based identity; no GUIDs

For M47, asset identity is the normalized canonical **project-relative
asset path**, following existing repository path/case rules.

No GUID system is introduced.

#### Minimal registry/catalog

The registry must expose only what is concretely needed now and to
provide a clean input for a later browser:

-   canonical project-relative path;
-   supported asset type (use existing terminology if present);
-   only minimal validity/status information actually required.

No speculative tags, favorites, thumbnails, dependency graphs,
import-settings databases, package systems, reflection metadata, or
future interaction data.

#### Prefer deterministic discovery

Inspect the repository first. If canonical source content can
reconstruct the catalog deterministically, prefer a derived
registry/catalog over a new hand-maintained database.

If an existing manifest/index already exists, reuse it.

Introduce a new persistent registry file only if the actual repository
demonstrates that it is necessary. If so, document/test its authority
and deterministic update/failure behavior.

#### No LevelDefinition change

Importing an asset does not instantiate it. Level Format remains v1. No
Static Prop or generic scene-object record is added.

#### Development-only authoring operation

Import UI/workflow belongs to Development. Debug and Release must not
depend on source-file import UI or external authoring paths.

#### No Content Browser yet

M47 may expose minimal feedback/inspection sufficient to verify registry
results, but no browser grid, thumbnail system, folder tree, search
experience, preview viewport, favorites, or drag/drop placement.

### Import workflow

Use names and boundaries found in the repository rather than inventing
APIs.

Conceptually:

1.  invoke **Import Static GLB** from an appropriate Development
    workflow;
2.  supply/select an external `.glb`;
3.  validate input path, extension, and current static-model
    compatibility;
4.  resolve the destination using existing canonical source
    organization;
5.  reject unsafe paths and destination collisions;
6.  perform the import safely;
7.  refresh/rebuild registry discovery as required;
8.  allow existing cook/stage to process the imported content;
9.  report success/failure through appropriate existing Development
    feedback/tool output.

Use the smallest existing file-selection/platform boundary. If a
file-dialog abstraction exists, reuse it. If none exists, add only the
narrow Development input mechanism needed here; do not create a
universal filesystem browser framework.

### Validation and collision policy

Reject at minimum:

-   missing source;
-   non-file input;
-   unsupported extension;
-   unsafe/out-of-root destination resolution;
-   destination traversal;
-   destination collision;
-   GLB failing actual current static-model compatibility checks.

Reuse existing loader/cooker validation where practical rather than
creating a competing GLB parser.

**Never silently overwrite an existing canonical asset.** Collision
fails clearly and preserves the existing file. No overwrite
confirmation, auto-rename, versioning, or hash deduplication in M47.

A failed import must not leave a registered half-imported asset.
Temporary/intermediate files, if required, must not become registry
entries.

### Registry/catalog invariants

The registry/catalog must answer, conceptually:

-   which supported project assets exist;
-   each asset's canonical project-relative identity;
-   each asset's supported type.

Exact class/API is deliberately unspecified pending repository
inspection.

Required properties:

-   deterministic ordering when exposed;
-   normalized project-relative paths;
-   no accidental current-working-directory dependency;
-   no persistent machine-specific absolute identity;
-   invalid/unsupported files excluded;
-   duplicate canonical identities impossible;
-   explicit/testable refresh or discovery behavior;
-   future Development UI can consume it without coupling registry
    authority to ImGui;
-   it is not a scene-object system.

### Cooker/staging integration

Existing cooker/stager remain authoritative. Make only narrowly
necessary discovery changes so imported static GLBs traverse the
established pipeline.

Do not stage directly from arbitrary external locations. Do not make
staged output authored authority. Do not add a generic watcher.

Import need not automatically Cook/Stage unless the current architecture
clearly makes that the correct existing workflow. If separate,
feedback/documentation must make the next existing Cook/Stage step
clear.

### Editor state

Import must not modify:

-   `workingCopy`;
-   `active`;
-   `savedSourceBaseline`;
-   Modified;
-   Dirty;
-   Level Save state.

Existing Apply/Revert/Save semantics remain unchanged.

No object is added to Hierarchy or viewport.

### Build configurations

**Development:** owns import workflow and minimal verification feedback.
Existing Tool Output, `EditorToolRunner`, Cook Assets, Stage Runtime
Assets, Cook & Stage, Cook Stage & Reload, and level-editor workflows
remain regressions.

**Debug:** must build and remain free of unnecessary Development
import/UI dependencies.

**Release:** must build and consume staged runtime assets through the
existing runtime path. It must not require external source locations,
import dialogs, or Development registry UI.

### Canonical-data safety

M47 must not semantically change
`game/assets/source/levels/level_01.level`.

Final Level 01 remains:

-   Platforms = 6
-   Checkpoints = 2
-   Hazards = 2
-   Collectibles = 3
-   Dynamic Boxes = 0
-   FOV = 40

The legacy M45 line `dynamic_box 0 5 0 1 1 1 30` remains absent.

Automated tests must use fixtures/temp locations where possible.
Manual-test imports that are not intentional repository content must be
removed before closure. Inspect source/cooked/staged diffs for
generated/test artifacts.

### Required automated tests

Follow actual repository test conventions. Cover the real
import/registry/cook-stage boundary and prove at least:

1.  valid supported static GLB imports into canonical source
    organization;
2.  identity is normalized and project-relative;
3.  imported asset appears exactly once in registry discovery;
4.  registry ordering is deterministic;
5.  unsupported extension is rejected;
6.  missing/non-file input is rejected;
7.  unsafe/path-traversal destination is rejected;
8.  destination collision is rejected without overwrite;
9.  invalid/incompatible GLB is rejected using actual pipeline
    constraints;
10. failure leaves no registered partial asset;
11. refresh/discovery reflects canonical source content correctly;
12. unsupported files do not become valid model entries;
13. imported content traverses relevant existing cook/stage path;
14. source identity remains distinct from staged/runtime path;
15. empty/zero user-import state is valid where applicable;
16. import does not mutate level/editor authored state at the
    appropriate integration boundary;
17. path/case behavior preserves current portability conventions.

Do not pollute canonical content during automated tests.

### Required regressions

Run relevant current regressions for:

-   static GLB/model loading;
-   asset cooking;
-   runtime staging;
-   `EditorToolRunner`;
-   Cook & Stage / Cook Stage & Reload;
-   editor menu/workspace if touched;
-   LevelFile;
-   canonical scene cleanup;
-   authored lifecycle if shared editor/application code is touched;
-   M45/M46 physics only if shared runtime/update code is touched.

Inspect current test targets instead of inventing names.

Run relevant Python tests, including current equivalents of:

``` powershell
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Run any existing static-GLB cooker tests. Add focused Python coverage if
Python import/cook/stage code changes.

### Build validation

``` powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Use current documented equivalents if commands have changed and report
the difference.

Also run `git diff --check`.

### Manual acceptance checklist

#### Successful import

-   Launch Development.
-   Invoke official Static GLB import.
-   Select/supply a known-compatible external `.glb`.
-   Confirm clear success feedback.
-   Confirm canonical source placement.
-   Confirm stable project-relative registry identity.
-   Confirm the project recognizes it without level-file editing.

#### Cook/stage

-   Run the established relevant Cook/Stage workflow.
-   Confirm the imported asset is processed.
-   Confirm source/cooked/staged remain distinct.
-   Confirm runtime output does not depend on the original external
    absolute path.

#### Collision

-   Attempt the same destination again.
-   Confirm clear failure.
-   Confirm existing canonical content is unchanged.

#### Invalid input

-   Try an unsupported file type.
-   Try an invalid/incompatible GLB where practical.
-   Confirm rejection and no registry pollution.

#### Existing editor

-   Confirm import does not make the level Modified/Dirty.
-   Confirm Apply/Revert/Save are unchanged.
-   Confirm existing Cook/Stage/Reload still works.

#### Scope boundary

Confirm import does **not** create a level object, viewport instance,
Static Prop, Hierarchy entry, Content Browser, or gameplay interaction.

#### Cleanup

-   Remove noncanonical manual-test imports.
-   Verify no temporary/generated artifacts remain unintentionally.
-   Verify canonical Level 01 is unchanged.

### Documentation

Update only affected current documentation, likely `README.md`,
`AGENTS.md`, `docs/ARCHITECTURE.md`, `docs/MILESTONES.md`, and existing
asset-pipeline docs if present.

Document supported import scope, source/cooked/staged authority,
registry identity, collision policy, Development workflow, and
limitations.

Do not document M48--M55 as implemented. Do not add Static Prop syntax
to Level Format docs.

### Explicit non-goals

M47 does not implement:

-   Content Browser / Asset Browser;
-   thumbnails/previews/search/favorites;
-   asset drag-and-drop;
-   Static Props / Generic Scene Objects;
-   viewport placement;
-   new level records;
-   prop rotate/scale workflow;
-   folders/parenting/scene graph redesign;
-   GUIDs;
-   prefabs;
-   ECS/components;
-   FBX/OBJ/glTF JSON;
-   animation/skeletal import;
-   material/shader editor;
-   universal texture importer;
-   overwrite/auto-versioning/hash deduplication;
-   generalized dependency/package database;
-   hot-reload watcher;
-   Grab/Carry;
-   Pressure Plates/triggers;
-   Game Feature Configuration;
-   Inventory/Pickup;
-   M48 work.

### Completion criteria

M47 is ready for manual acceptance only when:

-   compatible static GLB import works through Development;
-   validation preserves actual static-model constraints;
-   canonical source destination follows existing organization;
-   registry/catalog discovers content deterministically;
-   identity is normalized/project-relative and no GUID is added;
-   collisions never silently overwrite;
-   failed imports leave coherent state;
-   source/cooked/staged authority remains intact;
-   existing cook/stage handles imported content;
-   Level Format remains v1;
-   no level instance is created;
-   editor authored state is unchanged;
-   focused/regression/Python tests pass;
-   Debug/Development/Release build;
-   `git diff --check` passes;
-   canonical Level 01 and asset tree are clean from unintended test
    artifacts;
-   affected documentation is current;
-   Cursor supplies the required report.

Manual acceptance and Git closure are still mandatory. M47 is CLOSED
only after automated validation + explicit manual approval + Git
closure + clean synchronized `main`.

### Cursor final report requirements

Report:

1.  root implementation approach;
2.  exact files changed;
3.  actual source/cooked/staged paths discovered;
4.  where import authority lives;
5.  how external input is selected/supplied;
6.  exact validation performed;
7.  collision behavior;
8.  canonical identity formation;
9.  registry/catalog authority;
10. derived vs persistent registry decision and rationale;
11. exact registry metadata;
12. cook/stage discovery path;
13. partial-import/failure safety;
14. confirmation level/editor authored state is untouched;
15. Level Format v1 confirmation;
16. tests added/changed and focused results;
17. relevant regressions;
18. Python/tooling results;
19. Debug/Development/Release builds;
20. canonical Level 01 verification;
21. asset-tree/generated-artifact verification;
22. `git diff --check`;
23. remaining risks/limitations.

### STOP condition

After implementation, validation, builds, canonical-data verification,
documentation, and report:

-   Do **NOT** commit.
-   Do **NOT** push.
-   Do **NOT** merge.
-   Do **NOT** start M48.
-   Do **NOT** declare M47 CLOSED.
-   **STOP.**

M47 then waits for mandatory manual acceptance and Git closure.


## Milestone 48 — Content Browser / Asset Browser v1

**Status:** Implemented; awaiting manual acceptance.
**Branch:** `milestone/48-content-browser`

### Implemented (this branch)

Development F2 **Content Browser** is a view/controller over M47
`StaticModelCatalog`. It lists registered static GLB assets (filename,
`static_glb`, canonical `models/<file>.glb`), supports case-insensitive
search, Refresh, Import Static GLB (same `LevelEditorRequest::ImportStaticGlb`
authority), and confirmed Delete Asset.

Asset selection lives on `ContentBrowserState.selectedIdentity` and is
not Hierarchy/viewport selection. Import, refresh, search, and delete do
not mutate `workingCopy` / `active` / `savedSourceBaseline` / Modified /
Dirty.

Delete authority is `assets::DeleteStaticModel`. Identity must parse as
`models/<safe.glb>`. Source/cooked/staged paths are joined under explicit
absolute roots. Generated copies are removed first; source last. Missing
cooked/staged counterparts are not errors. Cancellation never calls
delete. No Static Props, placement, thumbnails, or Level Format change.

Focused coverage: `ContentBrowserTest`. M48 is not complete until manual
acceptance.

### Baseline

M00–M47 are CLOSED. `main` is clean and synchronized.

M47 established the official static-GLB import workflow, canonical source models, project-relative model identity, a derived `StaticModelCatalog`, and existing source → cooked → staged authority. Import does not instantiate a level object.

Before implementation, inspect the actual post-M47 repository. Current code, tests, and current documentation are the source of truth. Preserve actual current behavior if names/details differ from this planning document.

### Strategic role

The production workflow is progressing as:

**Import → Visualize/Find → Instantiate → Place → Interact**

M47 implemented **Import**. M48 implements **Visualize/Find and basic asset management**.

M48 must make registered static-model assets visible and manageable inside the Development editor, but must **not instantiate them into the level**. The planning screenshot is only a workflow/productivity reference, not a UI or architecture specification.

### Objective

Create a Development-only **Content Browser / Asset Browser v1** using the M47 catalog as authority, allowing the user to:

1. see available registered static GLB assets inside the editor;
2. identify them by useful human-readable information;
3. select an asset;
4. search/filter the catalog;
5. refresh after import/external changes;
6. invoke the existing M47 import workflow from a natural asset-management surface;
7. remove an asset through an explicit, safe project operation;
8. receive clear success/failure feedback;
9. do all this without modifying level authored state.

### Architectural decisions

#### Catalog remains authority

The Content Browser is a view/controller over the M47 catalog, not a second asset database. No GUIDs and no duplicated browser-owned registry. Small repository-consistent extensions to `StaticModelCatalog` are allowed if needed for refresh/querying, but keep them independent of ImGui.

#### Development-only

The browser belongs to Development editor tooling. Debug/Release must not expose or depend on browser UI. Runtime loading remains unchanged.

#### Static GLB scope remains

Browse only asset types actually supported by M47, initially static GLB/static models. Showing an asset type field is appropriate; broadening import pipelines is not.

#### Asset selection != scene selection

Selecting an asset means selecting a reusable project resource. It must not create a LevelDefinition object, select a Hierarchy object, place anything, mutate `workingCopy`, `active`, `savedSourceBaseline`, or set level Modified/Dirty.

#### No placement

No viewport drag/drop, double-click placement, ghost preview, Static Prop creation, or Add-to-Level action. Those belong to M49/M50.

#### Basic asset deletion belongs in M48

M48 adds an official removal workflow while no Static Prop references exist yet.

A successful Delete Asset operation must:

- target a selected canonical registered asset;
- require explicit confirmation;
- remove the canonical source asset;
- remove only its corresponding cooked/staged generated artifacts when present;
- refresh the catalog/browser;
- report success/failure clearly.

No arbitrary filesystem deletion, no paths outside authorized roots, and no future dependency/reference system.

##### DELETE SOURCE-OF-TRUTH CLARIFICATION

Delete Asset must treat the canonical SOURCE asset as the authored
project authority.

Before deleting anything, validate all source/cooked/staged mappings.

The operation must never reach outside the exact authorized roots for
the selected canonical asset identity.

If deleting generated counterparts fails, do not improvise recovery,
delete neighboring files, or broaden the operation.

The implementation must define and test a deterministic partial-failure
policy based on the actual repository filesystem behavior.

In particular, the final Cursor report must state clearly:

- whether source deletion occurs before or after generated cleanup;
- what happens if cooked deletion fails;
- what happens if staged deletion fails;
- what catalog state is exposed after any partial failure;
- whether the operation is retry-safe.

Do not claim filesystem transactionality if the underlying implementation
does not actually provide it.

#### Generated-artifact cleanup

Inspect actual M47 cooker/stager mapping first. Define one narrow deletion authority that understands current source/cooked/staged mapping. Do not leave stale generated copies that can masquerade as valid runtime content. Missing generated counterparts are acceptable. Document/test partial-failure semantics conservatively.

#### Presentation

At minimum show:

- display name / filename;
- asset type;
- canonical project-relative path.

Do not invent gameplay categories such as Pickup, Grab/Carry, Pressure Plate, etc.

#### Search/filter v1

Provide simple case-insensitive text filtering over useful existing fields, at minimum filename/display name and/or canonical path. No tags, favorites, smart collections, saved searches, or advanced query language.

#### List/grid and thumbnails

Use the smallest productive ImGui presentation consistent with the current editor. A selectable list/table/grid is acceptable.

M48 does **not require rendered thumbnails**. If no safe reusable thumbnail/preview infrastructure exists, use a clean textual/icon-like placeholder presentation. Do not create an offscreen renderer or thumbnail cache merely for M48.

### Editor integration

Add a persistent/toggleable Development editor window named consistently with current UI, preferably **Content Browser** unless existing terminology favors **Asset Browser**.

Integrate with existing menu/window persistence conventions. Provide:

- catalog entries;
- selection;
- search/filter;
- Refresh;
- Import Static GLB access/reuse;
- Delete/Remove selected asset;
- useful status/empty-state messaging.

Reuse M47 import authority and existing Tool Output/status infrastructure where appropriate.

### Import integration

The browser makes importing convenient but M47 remains import authority. After successful import, refresh automatically if practical so the asset becomes visible without restarting Development. Do not auto-place or auto-cook/stage. Existing **Assets > Import Static GLB** must continue working.

### Refresh behavior

Provide explicit Refresh. It must re-discover valid canonical source assets, preserve selection if the same canonical identity still exists when simple/safe, clear selection if it disappeared, and update filtered results deterministically. No filesystem watcher/hot-reload service.

### Delete Asset behavior

Expected flow:

1. select asset;
2. invoke Delete/Remove;
3. show explicit confirmation naming it;
4. validate canonical identity/path;
5. remove only authorized source/cooked/staged files mapped to it;
6. refresh catalog;
7. clear selection if deleted;
8. report result.

Cancellation makes no filesystem changes.

Reject no selection, malformed/noncanonical identity, traversal/out-of-root mapping, unsupported type, arbitrary-file deletion, or any mapping escaping authorized roots.

No recycle bin, undo, trash folder, restore, batch delete, multi-select delete, dependency graph, or force-delete.

### Cook/stage relationship

M48 preserves M47 policy: **Import does not automatically cook/stage.** Existing global Cook Assets / Stage Runtime Assets / Cook & Stage / Cook Stage & Reload remain authoritative.

Per-asset cook buttons or cooked/staged badges are not required. Deletion must safely clean corresponding generated counterparts so removed content is not resurrected later.

### Level/editor authority

Browser operations must not mutate `workingCopy`, `active`, `savedSourceBaseline`, level Modified/Dirty, Apply/Revert, or Save state. Asset selection remains independent from Hierarchy/viewport scene selection. Level Format remains v1.

### Window/layout persistence

Follow existing Development workspace/window persistence. If current editor windows persist open/closed state, Content Browser should participate consistently. Do not redesign workspace architecture.

### Empty state

Zero supported assets is valid. Show useful guidance to import a Static GLB without errors or fake placeholder assets.

### Canonical-data safety

M48 must not semantically modify `game/assets/source/levels/level_01.level`.

Final Level 01 remains:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The legacy `dynamic_box 0 5 0 1 1 1 30` remains absent.

Automated asset-management tests should use temporary directories/fixtures. Do not leave manual-test assets or generated artifacts unintentionally committed.

### Required automated tests

Follow actual post-M47 conventions. At minimum prove:

1. browser/query model exposes valid catalog entries deterministically;
2. filename/type/canonical identity are available;
3. case-insensitive search/filter works;
4. unmatched filter yields a valid empty result;
5. refresh discovers a newly added/imported valid asset;
6. refresh removes an absent asset;
7. selection survives refresh if identity still exists, if implemented;
8. selection clears safely if identity disappears;
9. successful M47 import becomes visible through refresh without restart;
10. browser import dispatch reuses M47 authority;
11. deletion rejects no-selection/invalid identity safely;
12. deletion rejects traversal/out-of-root mapping;
13. deletion cancellation makes no filesystem changes at the testable authority boundary;
14. successful deletion removes canonical source;
15. successful deletion removes only corresponding cooked/staged artifacts when present;
16. missing generated counterparts do not cause unrelated deletion;
17. neighboring assets remain untouched;
18. catalog no longer exposes deleted asset after refresh;
19. browser/import/delete operations do not mutate authored level state;
20. zero-assets state is valid;
21. Debug/Release remain free of browser UI dependencies as appropriate.

Keep filesystem authority tests outside ImGui where practical.

### Required regressions

Run relevant current tests for M47 import/catalog, `EditorToolRunner`, editor workspace/layout persistence, menu behavior if modified, authored lifecycle integration, LevelFile, CanonicalSceneCleanup, CookStageReloadWorkflow, and editor selection/picking/gizmo tests if shared selection code is touched.

Run current relevant Python tests, including equivalents of:

```powershell
python tools/test_import_static_glb.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Add focused deletion/cook-stage cleanup tests if Python/CMake tooling changes.

### Build validation

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

All configurations must succeed. Also run `git diff --check`.

### Required manual acceptance

#### A. Browser visibility
- Launch Development and open Content Browser.
- Confirm registered static models appear.
- Confirm filename/display name, type, and canonical path are understandable.
- Select different assets and confirm stable selection.

#### B. Search/filter
- Search with part of an asset filename/path using different casing.
- Confirm expected match.
- Enter unmatched text and confirm clean empty result.
- Clear search and confirm full catalog returns.

#### C. Import integration
- Invoke Import Static GLB from the browser.
- Import a compatible GLB.
- Confirm normal M47 validation/success feedback.
- Confirm it appears without restarting Development.
- Confirm it is not placed in the level.

#### D. Refresh
- Use Refresh and confirm catalog correctness and sensible deterministic selection behavior.

#### E. Delete cancellation
- Select a disposable asset, invoke Delete, cancel, and confirm source/cooked/staged and catalog entry remain unchanged.

#### F. Delete success
- Cook/stage a disposable asset so generated counterparts exist.
- Delete it through Content Browser and confirm.
- Confirm source and corresponding cooked/staged files are removed.
- Confirm unrelated assets remain and browser entry disappears.

#### G. Level/editor isolation
Throughout browser/import/delete/search/refresh, level must not become Modified/Dirty, Apply/Revert/Save remain unchanged, Hierarchy/viewport scene selection remains coherent, and no level object is created.

#### H. Scope boundary
Confirm no viewport placement, Static Prop, scene drag/drop, double-click instantiate, new rendered-thumbnail pipeline, gameplay classification, Grab/Carry, or Inventory behavior.

#### I. Cleanup
Remove disposable manual-test imports, verify no unintended generated/temp artifacts, canonical Level 01 unchanged, and inspect asset-tree diffs.

### Documentation

Update only affected current docs, likely `README.md`, `AGENTS.md`, `docs/ARCHITECTURE.md`, `docs/MILESTONES.md`, `game/assets/README.md`, and `tools/README.md` if deletion affects tooling.

Document Content Browser v1, catalog authority, asset-vs-scene selection, search/filter, refresh, import integration, deletion semantics, generated-artifact cleanup, and limitations. Do not document M49+ as implemented.

### Explicit non-goals

M48 does not implement:

- Static Props / Generic Scene Objects;
- Level Format asset records;
- viewport placement / drag-drop / double-click instantiate / ghost placement;
- prop Rotate/Scale workflow;
- rendered thumbnail generation unless a trivial reusable current facility already exists with no new architecture;
- model preview viewport / thumbnail cache;
- tags/favorites/recents/virtual folders;
- filesystem folder management;
- asset rename/move;
- overwrite/reimport;
- batch import;
- multi-select/batch delete;
- recycle bin/undo delete;
- dependency graph/reference checking for future Static Props;
- GUIDs/prefabs/ECS/components;
- new asset formats;
- Grab/Carry;
- Pressure Plate;
- Game Feature Configuration;
- Inventory;
- M49 work.

### Completion criteria

M48 is ready for manual acceptance when Development has a usable Content Browser v1 driven by the M47 catalog; filename/type/path are visible; asset selection is independent of scene selection; search/filter and refresh work; M47 import is reused and newly imported assets become visible; Delete Asset is confirmed/path-safe and cleans corresponding source/cooked/staged artifacts without touching unrelated content; level authored state is unchanged; no placement/Static Prop behavior exists; Level Format remains v1; tests/regressions/Python pass; Debug/Development/Release build; `git diff --check` passes; canonical Level 01 is unchanged; test artifacts are cleaned; docs are current; and Cursor supplies the required report.

Manual acceptance and Git closure remain mandatory. M48 is CLOSED only after automated validation + explicit manual approval + Git closure + clean synchronized `main`.

### Cursor final report requirements

Report:

1. root implementation approach;
2. exact files changed;
3. Content Browser window/menu integration;
4. catalog authority and any M47 catalog changes;
5. browser entry metadata;
6. selection model and separation from scene selection;
7. search/filter semantics;
8. refresh semantics;
9. import integration and M47 authority reuse;
10. delete confirmation flow;
11. delete authority/path-safety rules;
12. exact source/cooked/staged mapping and cleanup behavior;
13. partial-delete/failure semantics;
14. authored-level isolation;
15. workspace persistence behavior;
16. tests added/changed;
17. focused results;
18. M47/editor regressions;
19. Python/tooling results;
20. Debug/Development/Release builds;
21. canonical Level 01 verification;
22. asset-tree/generated-artifact verification;
23. `git diff --check`;
24. remaining risks/limitations.

### STOP condition

After implementation, validation, builds, canonical-data verification, documentation, and detailed report:

- Do **NOT** commit.
- Do **NOT** push.
- Do **NOT** merge.
- Do **NOT** start M49.
- Do **NOT** declare M48 CLOSED.
- **STOP.**

M48 then waits for mandatory manual user acceptance and Git closure.


## Milestone 48.1 — Content Browser Thumbnails

**Status:** Implemented; awaiting mandatory manual acceptance. Not CLOSED.
**Branch:** `milestone/48.1-content-browser-thumbnails`
**Cursor model:** Grok 4.6 High — Fast OFF

### Baseline

M00–M48 are CLOSED. `main` is clean and synchronized.

M47 established the static GLB import/catalog workflow.

M48 established the Development Content Browser with:

- `StaticModelCatalog` as asset authority;
- independent asset selection;
- search/filter;
- explicit Refresh;
- reuse of `Import Static GLB`;
- safe Delete Asset;
- source/cooked/staged authority;
- no Static Props or viewport placement;
- no authored-level state mutation.

Before implementation, inspect the actual post-M48 repository. Current code, tests, and current documentation are the source of truth. Do not assume class names, renderer architecture, cache facilities, GLB loading paths, texture APIs, or editor persistence details before inspection.

### Strategic role

The production workflow remains:

**Import → Visualize/Find → Instantiate → Place → Interact**

M48.1 deepens only the **Visualize/Find** stage.

It upgrades the Content Browser from a primarily textual navigator into a visual asset navigator by adding thumbnails for the static `.glb` assets already represented by `StaticModelCatalog`.

This milestone does not create authored scene instances.

### Objective

Evolve the M48 Content Browser so that:

1. static GLB assets can be browsed visually through generated thumbnails;
2. **Thumbnails** is the default view mode;
3. the user can switch between **Thumbnails** and **List**;
4. the view-mode preference persists consistently with the current editor workspace/layout system;
5. existing M48 search, selection, Refresh, Import, Delete, empty state, and status feedback continue to work;
6. thumbnails are generated/cached efficiently;
7. thumbnail failure produces a stable placeholder/fallback;
8. thumbnail work does not continuously reload/render every model every frame;
9. M47/M48 source/cooked/staged authority and authored-level isolation remain unchanged.

### Architectural constraint: inspect before choosing the thumbnail path

Before settling on the implementation, inspect the actual post-M48 repository and report what reusable infrastructure exists for:

- loading static GLB mesh data;
- obtaining model geometry/bounds;
- creating GPU textures;
- rendering meshes;
- render targets/framebuffers/offscreen passes;
- camera/view/projection helpers;
- lighting/shading;
- image encoding/decoding;
- Development-only resources;
- cache/directory helpers;
- file timestamp or content-change detection;
- editor texture ownership/lifetime.

Do not create a new renderer or generalized asset-preview framework unless inspection proves no smaller reuse path exists.

Prefer the smallest architecture consistent with the current engine.

### Thumbnail generation

A thumbnail must visually represent the corresponding static GLB.

The implementation must define, based on actual repository capabilities:

- how the GLB is loaded for thumbnail generation;
- how model bounds are computed or reused;
- how the camera is positioned/framed automatically;
- camera projection and orientation;
- lighting setup;
- background;
- output resolution;
- output image format, if persisted as an image;
- GPU/CPU resource ownership;
- failure behavior.

#### Bounds and framing

Use current mesh/model data when possible.

The thumbnail must frame the model based on its actual bounds rather than relying on arbitrary asset-specific constants.

The framing should:

- center the model visually;
- include the entire model with reasonable padding;
- behave safely for very small/large dimensions;
- handle degenerate/near-zero extents gracefully;
- avoid clipping through the model.

Do not add authored pivot editing or model-normalization tools.

#### Camera

Use one consistent thumbnail camera setup.

A three-quarter perspective view is preferred if it fits current renderer conventions, but exact angles must follow the minimal implementation after repository inspection.

The camera must be deterministic for a given asset/configuration.

Do not add interactive preview controls.

#### Lighting and background

Use a stable, readable setup with a neutral background and sufficient lighting to understand the shape.

Prefer reuse of current renderer/material/light conventions.

Do not build a full studio-lighting subsystem.

The objective is asset recognition, not photorealistic presentation.

### Thumbnail cache

Prefer a local generated cache that is **not versioned in Git**.

The exact location must be chosen only after inspecting current repository conventions.

Desired properties:

- local to the project/editor environment;
- clearly generated/derived;
- not part of source authority;
- safe to delete/rebuild;
- ignored by Git;
- does not pollute canonical source/cooked/staged asset roots;
- does not become runtime content authority.

#### Cache identity

Thumbnail identity must derive from the canonical project-relative asset identity used by M47/M48.

Do not introduce GUIDs.

The mapping from asset identity to cache entry must:

- be deterministic;
- avoid path traversal;
- avoid collisions;
- be safe across supported filesystem semantics;
- remain separate from user-facing display name.

If hashing is used for filenames, the canonical asset identity remains the authority; the hash is only a cache key.

#### Cache invalidation

Avoid regenerating thumbnails unnecessarily.

A cached thumbnail must be regenerated when the underlying source asset has changed enough that the cached image may be stale.

Choose the narrowest reliable invalidation strategy supported by current repository infrastructure.

Possible inputs may include source modification timestamp, source size, source content hash, thumbnail schema/version, or renderer/config version.

Do not add expensive hashing if current infrastructure already provides a reliable cheaper mechanism.

If a small cache metadata record is needed, it is derived cache data, not a new asset registry.

The final implementation must document the invalidation rule precisely.

#### CACHE VALIDATION SAFETY CLARIFICATION

The chosen invalidation mechanism must not rely on file modification
timestamp alone if the actual filesystem/tooling behavior can reasonably
preserve or reproduce the same timestamp after source replacement.

Prefer the smallest reliable fingerprint available in the current
repository.

The implementation does not need cryptographic content hashing unless
repository inspection shows it is necessary.

Whatever strategy is chosen must correctly distinguish, within the
supported project workflow:

- unchanged source -> cache remains valid;
- changed/replaced source -> cache becomes stale.

Document the known limitations of the chosen fingerprint in the final
Cursor report.

#### Cache cleanup

Do not build an aggressive cache-pruning subsystem unless needed.

If obsolete cache cleanup is simple, safe, and naturally fits Refresh/Delete, it may be implemented narrowly.

At minimum, deleting an asset should ensure its in-memory thumbnail state does not remain active.

If cache-file deletion for the removed asset is safe and straightforward, prefer removing it.

Do not scan/delete arbitrary unrelated files.

### When thumbnails are created

Do not eagerly regenerate every asset every frame.

The implementation should choose the smallest productive policy after inspection, such as lazy/on-demand generation.

Preferred behavior:

- visible/needed thumbnail missing or stale → schedule/generate it;
- valid cached thumbnail → reuse it;
- offscreen/unneeded assets should not cause continuous work;
- repeated frames should reuse loaded thumbnail texture/resources.

If the engine already has an appropriate lightweight queue or job system, reuse it. Do not introduce a generalized asynchronous task framework solely for M48.1.

Synchronous generation is acceptable if bounded and responsive enough for the expected Content Browser scale; otherwise use the smallest existing asynchronous mechanism.

The Cursor report must state whether generation is synchronous or asynchronous and why.

### Import behavior

Preserve M48 import authority.

After successful `Import Static GLB`:

- refresh the catalog as M48 already does;
- the imported asset should appear in the browser;
- thumbnail generation should follow the normal missing-cache policy;
- no automatic scene instantiation;
- no authored-level state mutation;
- no change to M47 cook/stage policy.

Do not duplicate import logic.

### Delete behavior

Preserve M48 Delete Asset authority and path-safety rules.

After successful deletion:

- remove the browser entry after normal refresh;
- release any in-memory thumbnail resource for that asset;
- remove its local derived thumbnail cache entry if safely mapped;
- do not touch unrelated thumbnail cache entries;
- preserve existing source/cooked/staged deletion semantics.

Delete cancellation must not modify cache or filesystem state.

### Refresh behavior

Refresh must preserve M48 catalog behavior.

In addition:

- valid cache entries remain reusable;
- stale cache entries are recognized according to the chosen invalidation rule;
- missing entries can regenerate when needed;
- selection behavior remains unchanged;
- filter behavior remains unchanged;
- Refresh must not synchronously rebuild the entire thumbnail cache unless repository scale/architecture makes that clearly the minimal safe design.

No filesystem watcher.

### View modes

Add two Content Browser view modes:

- **Thumbnails**
- **List**

#### Default

**Thumbnails is the default mode.**

#### Thumbnails mode

Display a simple productive grid of asset cards/items.

Each item should expose at least:

- thumbnail or fallback placeholder;
- display name / filename.

Asset type/path may appear in a tooltip, secondary label, or appropriate compact detail if useful.

Selection must be obvious.

The grid should adapt reasonably to available Content Browser width.

Do not add zoom sliders, masonry layout, folders, multi-select, drag/drop, or model preview.

#### List mode

Preserve the useful M48 textual/table view behavior.

At minimum retain:

- filename/display name;
- asset type;
- canonical project-relative identity/path;
- selection;
- search/filter;
- import;
- refresh;
- delete.

Do not regress the M48 list view.

#### Switching modes

Provide a simple toolbar/control to switch between Thumbnails and List.

Switching modes must preserve selection and current search query where possible, avoid unnecessary reload/regeneration, and never mutate authored-level state.

### View-mode persistence

Persist the Thumbnails/List preference using the same repository-consistent workspace/layout persistence approach used by M48.

Requirements:

- fresh/default editor state opens in Thumbnails mode;
- switching to List persists across the same kind of editor restart/layout persistence currently supported;
- Reset Editor Layout restores the project-defined default behavior consistently;
- persistence remains Development tooling state, not LevelDefinition data;
- no Level Format changes.

### Placeholder/fallback

If thumbnail generation, cache loading, texture upload, or asset preview loading fails:

- Content Browser remains usable;
- show a stable visual placeholder/fallback;
- asset remains selectable/searchable/deletable;
- failure is reported in Tool Output/logging as appropriate without spamming every frame;
- do not repeatedly retry every frame.

Define a reasonable retry policy, such as retry only after Refresh/source change/editor session, depending on the minimal architecture chosen.

### Zero-assets behavior

Zero assets remains a valid state.

In Thumbnails mode, show the same kind of useful empty-state guidance introduced in M48.

Do not generate placeholder sample assets.

### Performance expectations

The Content Browser must not:

- load every GLB every frame;
- render every GLB every frame;
- upload the same thumbnail texture every frame;
- regenerate valid cached thumbnails continuously;
- perform unbounded repeated failure work every frame.

Use cache/in-memory state to make steady-state browsing cheap.

Do not prematurely create a global asset streaming system, generalized render job scheduler, generalized content-addressable cache, or generalized thumbnail service for future asset types.

### Development / Debug / Release boundaries

#### Development

Owns Content Browser UI, thumbnail grid, thumbnail generation/cache consumption, and view-mode switching/persistence.

#### Debug

Must still configure/build successfully. Do not introduce Development ImGui thumbnail UI into Debug.

#### Release

Must still configure/build successfully. No Content Browser UI or Development thumbnail management should be required by Release runtime.

### Authored-level isolation

M48.1 must not mutate:

- `workingCopy`;
- `active`;
- `savedSourceBaseline`;
- Modified;
- Dirty;
- Apply/Revert state;
- Save state.

Thumbnail generation/cache operations are editor-derived asset visualization operations.

Asset selection remains independent from scene/Hierarchy selection.

### Level Format and canonical data

Level Format remains v1.

No new records.

Canonical `game/assets/source/levels/level_01.level` must remain semantically unchanged:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The legacy line `dynamic_box 0 5 0 1 1 1 30` remains absent.

### Required focused automated tests

Follow actual post-M48 test conventions. At minimum test the relevant non-ImGui/model boundaries for:

1. deterministic cache identity from canonical asset identity;
2. cache path safety;
3. valid cache hit avoids regeneration;
4. missing cache causes generation request/work;
5. changed source invalidates cache according to chosen rule;
6. unchanged source does not invalidate;
7. thumbnail schema/config version invalidates if such a version is used;
8. failed generation enters stable fallback/failure state;
9. failed item is not retried every frame;
10. Refresh can permit a sensible retry/invalidation;
11. Import makes a new asset eligible for thumbnail generation;
12. Delete releases/removes mapped thumbnail state/cache safely;
13. unrelated cache entries remain untouched;
14. zero-assets state works;
15. Thumbnails is default view mode;
16. switching to List works;
17. switching back to Thumbnails works;
18. view-mode persistence round-trip works;
19. reset/default layout returns to Thumbnails;
20. selection persists across view-mode changes;
21. search/filter behaves identically in both modes;
22. browser operations remain independent from scene selection;
23. authored-level state remains unchanged;
24. Debug/Release build boundaries remain valid.

If visual raster correctness cannot reasonably be asserted in existing unit-test infrastructure, test deterministic generation inputs, bounds/framing math, cache state, and failure behavior outside ImGui.

### Bounds/camera focused tests

If new bounds/framing helpers are introduced, add focused tests for:

- ordinary centered model bounds;
- off-origin model bounds;
- non-uniform extents;
- tiny/near-zero extents;
- large extents;
- deterministic camera target/distance;
- near/far plane safety if computed;
- finite/non-NaN outputs.

Do not expose production API solely for tests. Use existing test-access patterns if needed.

### Required regressions

Run relevant current M47/M48 and editor tests, including actual equivalents of:

- `StaticGlbImportTest`;
- `ContentBrowserTest`;
- `StaticModelCatalog` tests;
- `EditorToolRunnerTest`;
- `EditorWorkspaceTest`;
- layout persistence tests;
- `AuthoredLifecycleIntegrationTest`;
- `LevelFileTest`;
- `CanonicalSceneCleanupTest`;
- `CookStageReloadWorkflowTest`;
- editor selection/picking/gizmo tests if shared UI/selection paths are touched.

Run current Python tests:

```powershell
python tools/test_import_static_glb.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

### Build validation

Run:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

All must succeed.

Also run:

```powershell
git diff --check
```

### Required manual acceptance

Use a small set of compatible static GLBs with visibly different silhouettes.

#### A. Default thumbnails mode

1. Launch Development with default/reset layout state.
2. Open/observe Content Browser.
3. Confirm **Thumbnails** is the default mode.
4. Confirm assets appear as thumbnail cards.
5. Confirm each visible asset has a readable filename.
6. Confirm selection is visually clear.

#### B. Thumbnail quality

For multiple models, confirm full framing, no obvious clipping, consistent orientation/view, readable lighting/background, and placeholder behavior on failure.

#### C. View switching

Switch to List and back. Confirm M48 textual data remains correct, search/selection are preserved, and returning to Thumbnails does not trigger unnecessary full regeneration.

#### D. Persistence

Switch to List, restart according to current persistence behavior, confirm List persists, then Reset Editor Layout and confirm the default returns to Thumbnails.

#### E. Search/filter

Test the same case-insensitive searches in both modes. Results must match M48 behavior.

#### F. Cache reuse

Generate thumbnails, restart/reopen editor, and confirm valid thumbnails are reused rather than regenerated unnecessarily.

#### G. Cache invalidation

Using a disposable asset, generate its thumbnail, safely replace/update the source GLB, Refresh/reopen as required, and confirm the stale thumbnail is regenerated.

#### H. Import integration

Import Static GLB and confirm the asset appears, its thumbnail is produced by the normal policy, no level object is created, and the level remains unmodified.

#### I. Delete integration

Cancel Delete once and confirm nothing changes. Then delete the asset and confirm the browser entry and corresponding thumbnail state/cache disappear while unrelated thumbnails remain.

#### J. Performance sanity

With several assets visible, idle browsing must not continuously regenerate thumbnails; switching views must not reload/render every asset each frame; scrolling should remain responsive at current project scale.

#### K. Authored-state isolation

Throughout all tests confirm no Modified/Dirty, Apply/Revert, Hierarchy/viewport scene-selection, or Static Prop regressions.

#### L. Repository safety

Before approval:

- no generated thumbnail cache is staged for Git;
- local cache location is ignored appropriately;
- no disposable manual test assets remain unless intentionally kept;
- canonical Level 01 remains unchanged;
- `git diff --check` passes.

### Documentation

Update only affected current docs. Document:

- Thumbnails/List modes;
- Thumbnails default;
- thumbnail generation architecture actually chosen;
- bounds/camera/lighting/background behavior;
- cache location and non-authoritative nature;
- cache identity/invalidation policy;
- Import/Delete/Refresh interactions;
- fallback behavior;
- Development-only boundary;
- current limitations.

Do not document M49+ as implemented.

### Explicit non-goals

M48.1 does not implement:

- Static Props;
- Generic Scene Objects;
- viewport placement;
- drag-and-drop to viewport;
- double-click instantiate;
- ghost placement;
- new Level Format records;
- GUIDs;
- asset tags;
- favorites;
- recents;
- generalized asset metadata;
- rename/move;
- new import formats;
- interactive 3D Model Preview Editor;
- generic thumbnail service for future asset types;
- Grab/Carry;
- Pressure Plate;
- Inventory;
- M49 functionality.

Avoid unrelated refactors.

### Completion criteria

M48.1 is ready for manual acceptance when:

- Content Browser defaults to Thumbnails;
- List mode remains available and functional;
- view-mode switching is persistent;
- thumbnails are visually useful and automatically framed;
- cache is local/derived/non-versioned;
- cache identity maps deterministically from canonical asset identity;
- valid cache entries are reused;
- stale entries regenerate according to a documented rule;
- failures produce a stable fallback;
- browser steady state avoids continuous model loading/rendering;
- Import/Delete/Refresh/search/selection from M48 remain correct;
- authored-level state remains isolated;
- no Static Props/placement are introduced;
- focused tests pass;
- M47/M48 regressions pass;
- Python tests pass;
- Debug/Development/Release build;
- canonical Level 01 remains unchanged;
- thumbnail cache is not accidentally staged;
- `git diff --check` passes;
- Cursor provides the required detailed report.

M48.1 is CLOSED only after automated validation + explicit manual approval + Git closure + clean synchronized `main`.

### Cursor final report requirements

Report:

1. repository infrastructure discovered before architecture choice;
2. root implementation approach;
3. exact files changed;
4. thumbnail generation path;
5. model loading path reused;
6. bounds source/computation;
7. camera framing algorithm;
8. lighting/background;
9. thumbnail resolution/format;
10. cache location;
11. cache key/identity mapping;
12. cache metadata if any;
13. invalidation rule;
14. creation policy: lazy/eager/synchronous/asynchronous and why;
15. in-memory texture/resource reuse;
16. failure/fallback/retry behavior;
17. Import interaction;
18. Delete interaction and cache cleanup;
19. Refresh interaction;
20. Thumbnails/List UI behavior;
21. default mode;
22. persistence/reset behavior;
23. asset-selection vs scene-selection isolation;
24. authored-level isolation;
25. tests added/changed;
26. focused thumbnail/cache tests;
27. M47/M48/editor regressions;
28. Python results;
29. Debug/Development/Release builds;
30. canonical Level 01 verification;
31. cache/Git-ignore verification;
32. asset-tree safety;
33. `git diff --check`;
34. remaining risks/limitations.

### STOP condition

After implementation, tests, regressions, builds, cache/Git safety checks, canonical-data verification, documentation, and final report:

- Do **NOT** commit.
- Do **NOT** push.
- Do **NOT** merge.
- Do **NOT** start M49.
- Do **NOT** declare M48.1 CLOSED.
- **STOP.**

M48.1 then waits for mandatory manual user acceptance and Git closure.

### Implementation notes (post-inspection)

Inspection found no existing offscreen pass, thumbnail cache, GLB mesh loader in project code, or content-addressable asset store. `StaticGlb` validates containers only. The runtime renderer does not load catalog GLBs. Editor persistence already uses `%LOCALAPPDATA%\Platformer3D\` (`editor_layout.ini`, `editor_build_selection.txt`).

Chosen smallest path:

- Reuse raylib `LoadModel` / `GetModelBoundingBox` / `LoadRenderTexture` / `DrawModel` / `ExportImage` in `render::StaticModelThumbnailStore` (Development authoring only at the call site).
- 128×128 PNG cache under `%LOCALAPPDATA%\Platformer3D\thumbnails\`, keyed by FNV-1a 64 of the canonical identity `models/<file>.glb`.
- Meta: `schema=1`, identity, source mtime ticks, size. No content hash.
- Camera: three-quarter `normalize(1, 0.85, 1)`, perspective FOV 40°, padding 1.2 around bounds radius.
- Background `{56,60,72}`; lighting is `DrawModel(..., WHITE)`.
- Lazy synchronous generation, at most one GLB per frame; in-memory GPU texture reused while stamp matches.
- Failure placeholder; retry on Refresh (`AllowRetryAll`) or source stamp change.
- View modes Thumbnails (default) and List; persist `editor_content_browser_view.txt`; Reset Editor Layout restores Thumbnails.


## Milestone 48.2 — Interactive Static Model Preview

**Status:** CLOSED.
**Branch:** `milestone/48.2-interactive-static-model-preview`
**Cursor model:** Grok 4.6 High — Fast OFF

### Baseline

M00–M48.1 are CLOSED. `main` is clean and synchronized.

M47 established Static GLB import/catalog and canonical project-relative asset identity. M48 established the Development Content Browser, independent asset selection, search, Refresh, Import and Delete. M48.1 added cached static-model thumbnails and persistent Thumbnails/List modes.

Before implementation, inspect the actual post-M48.1 repository. Current code, tests and documentation are authoritative. In particular, inspect the thumbnail implementation before deciding which loading, bounds, camera, render-target and resource-lifetime pieces should be shared with an interactive preview.

### Objective

Add a persistent/toggleable Development **Model Preview** window that renders the real static `.glb` currently selected in the Content Browser at a larger interactive size, without instantiating it into the level.

The Preview must:

- use Content Browser asset selection as its authority;
- render the real model rather than enlarge the cached thumbnail;
- automatically frame the model from its bounds;
- support orbit;
- support zoom/dolly;
- support Frame/Reset View;
- consider pan only if it is simple and consistent with current controls;
- follow Content Browser selection changes;
- handle no selection, Refresh, Import and Delete safely;
- avoid unnecessary model reload/resource recreation every frame;
- remain Development-only;
- preserve M47, M48 and M48.1 behavior.

### Architectural principle: share reusable preview primitives, not thumbnail cache authority

The M48.1 thumbnail is a derived cached representation. The M48.2 Preview must render the actual selected asset through the appropriate rendering path.

Inspect whether M48.1 contains reusable low-level pieces for:

- validated static GLB loading;
- model lifetime;
- model bounds;
- framing math;
- camera construction;
- render targets;
- background/render setup;
- texture/resource ownership.

Refactor narrowly only when it removes real duplication between thumbnail rendering and preview rendering. Do not make the interactive Preview depend on thumbnail PNG/cache validity. Do not turn `StaticModelThumbnailCache` into a generic asset system.

### Model Preview window

Add a Development editor window named consistently with current terminology, preferably **Model Preview**.

It must participate in the current View menu/workspace/layout persistence conventions:

- toggleable like existing editor windows;
- persistent open/closed state/pose according to current workspace behavior;
- Reset Editor Layout restores the project-defined default state consistently.

Choose the default visible/hidden state only after inspecting current workspace conventions; report the choice. Do not redesign the workspace.

### Selection authority

The Preview consumes the selected canonical asset identity from the Content Browser.

It must not create a second selection authority.

When selection changes:

- if the same identity remains selected, reuse current preview resources;
- if a different valid static model is selected, replace preview resources safely;
- if selection becomes empty/invalid/deleted, release or clear model resources and show a useful no-selection state.

Asset selection remains independent of scene/Hierarchy selection.

### Real model rendering

The Preview must load/render the real selected static GLB from the canonical source path or the existing appropriate Development asset path established by M47/M48.1.

Do not display the thumbnail PNG as the model preview.

Reuse M48.1's validated model-loading/rendering path when architecturally appropriate, but separate:

- thumbnail disk cache state;
- interactive preview model/camera state.

The Preview is not runtime level authority and does not change source/cooked/staged authority.

### Bounds and automatic framing

Use actual model bounds. Prefer shared M48.1 framing/bounds helpers if they are cleanly reusable.

Automatic framing must handle:

- centered bounds;
- off-origin bounds;
- non-uniform extents;
- tiny/near-degenerate bounds;
- large bounds;
- finite camera outputs;
- clipping safety.

On first selection/load, frame the complete model with reasonable padding.

`Frame` / `Reset View` must return to a deterministic useful framing based on current model bounds.

Do not modify the asset pivot or source transform.

### Camera interaction

#### Orbit — required

Allow mouse-driven orbit around a sensible target, initially the framed model center unless repository conventions provide a better equivalent.

Orbit must:

- be deterministic and stable;
- avoid pathological pole behavior/gimbal-like flips;
- not affect the main editor viewport camera;
- only capture interaction when appropriate for the Preview region/window.

#### Zoom/dolly — required

Allow zoom/dolly with the current editor-consistent input convention, preferably mouse wheel when the Preview is hovered.

Clamp distance/zoom to safe values derived from model scale/bounds so the camera cannot collapse into invalid state or become unusably distant.

#### Pan — optional

Implement pan only if it is small, natural and consistent with existing editor controls. Its omission is acceptable and must not block M48.2.

Do not add a generalized camera-controller framework solely for this window.

### Frame / Reset View

Provide a clear control, e.g. **Frame** or **Reset View**, that restores:

- target;
- orbit orientation/default orientation;
- useful distance/zoom;
- any optional pan offset;

from current model bounds.

The exact control should fit current editor UI conventions.

### Render target and resizing

Render the model into a preview area appropriate to the current window size using existing raylib/render-target infrastructure where practical.

The Preview should respond sensibly when the window changes size.

Do not recreate the model every time the window size changes.

Recreate/resize only render-target resources that genuinely depend on dimensions, and avoid churn from tiny size oscillations if current ImGui/raylib integration makes that relevant.

Handle zero/minimized/invalid content dimensions safely.

### Lighting and background

Reuse M48.1 rendering conventions where they remain readable at larger preview size, or make the smallest preview-specific improvement justified by actual rendering infrastructure.

Keep lighting/background deterministic and neutral.

Do not build:

- material editor;
- HDRI browser;
- studio-light system;
- shader editor.

### Simple asset information

The Preview may display information already naturally available, such as:

- display name;
- canonical project-relative path;
- asset type;
- model bounds/dimensions.

Do not introduce new authoritative metadata merely to populate the window.

### Resource lifetime

The Preview must not reload the selected GLB every frame.

Maintain explicit state for the currently loaded preview asset/resources.

Expected behavior:

- no selection -> no model resource;
- first valid selection -> load once;
- same selection across frames -> reuse;
- selection changes -> unload old model and load new one;
- selected asset source changes and Refresh makes that relevant -> reload safely;
- selected asset is deleted -> unload/clear safely;
- editor shutdown -> release resources;
- render target resizes independently of model resource lifetime.

Do not leak GPU/model resources.

If loading fails, show a stable fallback/error state and do not retry every frame. A controlled retry on Refresh/source change/reselection is acceptable.

### Refresh

Preserve M48/M48.1 Refresh semantics.

After Refresh:

- if selected identity still exists and source stamp is unchanged, keep/reuse preview resources when safe;
- if the selected source changed, reload it according to the narrow change-detection mechanism already established or naturally available;
- if selection disappears, clear Preview;
- failed load may become retry-eligible according to the chosen policy.

No filesystem watcher.

### Import

Preserve the existing M47/M48 import authority.

After Import:

- catalog/browser behavior remains unchanged;
- if the imported asset becomes selected according to existing M48 behavior, Preview should load it through normal selection flow;
- otherwise Preview remains on current selection;
- no automatic level object;
- no automatic Cook/Stage change;
- no authored-level mutation.

### Delete

Preserve M48 Delete authority and M48.1 thumbnail cleanup.

On Delete cancellation, Preview state must remain unchanged.

On successful deletion of the selected asset:

- release its Preview model/resources;
- clear Preview selection-dependent state naturally when Content Browser selection clears;
- do not touch unrelated Preview/thumbnail/cache resources;
- preserve existing source/cooked/staged deletion semantics.

### Thumbnail interaction

M48.1 thumbnail cache remains independent derived state.

Preview may share low-level rendering/framing helpers with thumbnail generation, but must not require:

- thumbnail PNG existence;
- thumbnail cache metadata;
- thumbnail cache hit;
- thumbnail texture as source content.

Thumbnail failure must not inherently prevent Preview from trying to render the real asset, and Preview failure must not corrupt the thumbnail cache.

### Development / Debug / Release boundaries

#### Development

Owns the Model Preview UI and interactive rendering behavior.

#### Debug

Must configure/build successfully. No Model Preview editor UI is required.

#### Release

Must configure/build successfully. No Model Preview editor UI/resource management is required by runtime.

Keep Development-only dependencies behind current build boundaries. Shared low-level helpers are acceptable only when consistent with existing architecture.

### Authored-level isolation

M48.2 must not modify:

- `LevelDefinition`;
- `workingCopy`;
- `active`;
- `savedSourceBaseline`;
- Modified;
- Dirty;
- Apply/Revert;
- Save;
- Level Format.

Orbit, zoom, Frame, selection changes and Preview rendering are editor visualization state only.

The Preview must not affect the main editor viewport camera or authored transforms.

### Level Format / canonical data

Level Format remains v1.

Do not semantically modify `game/assets/source/levels/level_01.level`.

Final Level 01 remains:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The legacy line `dynamic_box 0 5 0 1 1 1 30` remains absent.

### Required focused tests

Follow actual post-M48.1 repository conventions. Keep rendering-independent state/math testable outside ImGui where practical.

At minimum cover:

1. Content Browser selected asset identity feeds Preview state correctly;
2. no selection produces valid empty Preview state;
3. changing selection requests/replaces the loaded asset exactly when required;
4. same selection does not reload every frame;
5. deleting/clearing selected asset releases/clears Preview state;
6. failed load enters stable fallback and is not retried every frame;
7. controlled retry after Refresh/source change/reselection works according to chosen policy;
8. ordinary bounds produce finite useful framing;
9. off-origin bounds frame around the correct target;
10. non-uniform/tiny/large bounds remain safe;
11. orbit state updates deterministically;
12. orbit clamping/pole handling remains safe;
13. zoom/dolly updates and clamps safely relative to model scale;
14. Frame/Reset restores deterministic default framing;
15. optional pan, if implemented, resets correctly;
16. Preview camera state is independent of main editor viewport camera state;
17. render-target size policy handles valid resize and zero/invalid dimensions safely;
18. model resource lifetime is independent from render-target resize lifetime;
19. Refresh with unchanged source avoids unnecessary model reload;
20. Refresh with changed selected source causes appropriate reload;
21. Import integrates through existing selection/catalog behavior;
22. Delete cancellation leaves Preview intact;
23. successful Delete of selected asset clears/releases Preview;
24. thumbnail cache validity is not required for Preview loading;
25. Preview operations do not mutate authored level state;
26. workspace visibility/layout persistence works according to current conventions;
27. Debug/Release boundaries remain valid.

Do not expose public production APIs solely for tests. Reuse existing test-access patterns if needed.

### Required regressions

Run relevant current M47/M48/M48.1/editor tests, including actual equivalents of:

- `StaticGlbImportTest`;
- `ContentBrowserTest`;
- `ContentBrowserThumbnailTest`;
- `StaticModelCatalog` tests;
- `EditorToolRunnerTest`;
- `EditorWorkspaceTest`;
- editor layout persistence tests;
- `AuthoredLifecycleIntegrationTest`;
- `LevelFileTest`;
- `CanonicalSceneCleanupTest`;
- `CookStageReloadWorkflowTest`;
- editor selection/picking/gizmo/orientation tests if shared input/camera paths are touched.

Run current Python tests:

```powershell
python tools/test_import_static_glb.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

### Build validation

Run:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

All must succeed.

Also run `git diff --check`.

### Manual acceptance

Use several static GLBs with visibly different shapes/scales.

#### A. Window/workspace

- Open/close Model Preview through current View/workspace UI.
- Confirm persistence follows current editor conventions.
- Confirm Reset Editor Layout behaves consistently.

#### B. Selection

- Select asset A in Content Browser; Preview shows A.
- Select B; Preview replaces A with B.
- Clear/remove selection; Preview shows a useful no-selection state.
- Scene/Hierarchy selection remains independent.

#### C. Real model rendering

- Confirm Preview renders the real model at larger size, not an enlarged thumbnail.
- Confirm it can work independently of thumbnail cache state.

#### D. Framing

- Test centered, off-origin, tall/wide and differently scaled models.
- Confirm complete model is visible without obvious clipping.
- Use Frame/Reset and confirm useful deterministic framing returns.

#### E. Orbit

- Orbit around model with intended input.
- Confirm smooth/stable behavior.
- Confirm no pole flip/pathological camera state.
- Confirm main viewport camera does not move.

#### F. Zoom/dolly

- Zoom in/out.
- Confirm useful clamps.
- Confirm Frame/Reset restores useful distance.

#### G. Pan, if implemented

- Confirm controls are natural and isolated to Preview.
- Confirm Reset clears pan offset.

#### H. Resize

- Resize Model Preview window substantially.
- Confirm rendering adapts.
- Confirm model is not reloaded simply because the window resized.
- Confirm minimized/tiny content does not break rendering.

#### I. Refresh/source change

- Refresh unchanged asset and confirm no visible unnecessary reload/churn.
- Safely replace/update a disposable selected source GLB.
- Refresh and confirm Preview updates according to implemented source-change rule.

#### J. Import

- Import a static GLB through existing workflow.
- Select it and confirm Preview loads normally.
- Confirm no level object is created.

#### K. Delete

- Cancel deletion of selected previewed asset: Preview remains.
- Delete for real: Preview clears/releases safely when selection disappears.
- Unrelated assets/thumbnails remain correct.

#### L. Failure

- Exercise a safe preview-load failure fixture/case if practical.
- Confirm stable fallback/error state.
- Confirm no per-frame error spam/reload loop.

#### M. Authored-state isolation

Throughout all Preview operations confirm:

- no Modified/Dirty;
- no Apply/Revert change;
- no Save-state change;
- no LevelDefinition change;
- no scene object creation;
- no authored transform changes.

#### N. Repository safety

Before approval:

- no disposable test assets remain unintentionally;
- no new preview-derived files are staged unless intentionally part of code/docs;
- thumbnail cache remains non-versioned;
- canonical Level 01 remains unchanged;
- `git diff --check` passes.

### Documentation

Update only affected current documentation. Document the architecture actually chosen:

- Model Preview window/workspace behavior;
- selection authority;
- real-model rendering path;
- what M48.1 infrastructure was reused/refactored;
- bounds/framing;
- orbit/zoom controls;
- optional pan if present;
- Frame/Reset;
- render-target resizing;
- resource lifetime/reload policy;
- Refresh/Import/Delete interactions;
- thumbnail-cache independence;
- Development-only boundary;
- limitations.

Do not document M49+ as implemented.

### Explicit non-goals

M48.2 does not implement:

- Static Props;
- Generic Scene Objects;
- viewport placement;
- drag-and-drop to level;
- double-click instantiate;
- ghost placement;
- asset transformation gizmos;
- mesh editing;
- pivot editing;
- material editing;
- texture editing;
- shader editing;
- animation;
- generic Asset Editor;
- generic property system;
- new asset formats;
- GUIDs;
- Grab/Carry;
- Pressure Plate;
- Inventory;
- M49 functionality.

Avoid unrelated refactors.

### Completion criteria

M48.2 is ready for manual acceptance when:

- a persistent/toggleable Development Model Preview window exists;
- it follows Content Browser asset selection;
- it renders the real selected static GLB;
- it does not depend on thumbnail PNG/cache validity;
- automatic bounds-based framing works;
- orbit works;
- zoom/dolly works with safe limits;
- Frame/Reset works;
- pan is either cleanly implemented or intentionally omitted;
- selection changes/clears replace/release resources correctly;
- model resources are not reloaded every frame;
- render-target resize does not reload the model;
- Refresh/Import/Delete behave correctly;
- authored-level state remains isolated;
- no Static Props/placement are introduced;
- focused tests pass;
- M47/M48/M48.1 regressions pass;
- Python tests pass;
- Debug/Development/Release build;
- canonical Level 01 remains unchanged;
- `git diff --check` passes;
- Cursor provides the required detailed report.

M48.2 is CLOSED only after automated validation + explicit manual approval + Git closure + clean synchronized `main`.

### Cursor final report requirements

Report:

1. repository infrastructure discovered before architecture choice;
2. root implementation approach;
3. exact files changed;
4. Model Preview window/workspace integration;
5. selection authority/data flow from Content Browser;
6. actual model loading/rendering path;
7. M48.1 infrastructure reused/refactored and what remained thumbnail-specific;
8. bounds source/computation;
9. automatic framing algorithm;
10. orbit implementation/input/clamps;
11. zoom/dolly implementation/input/clamps;
12. pan decision/implementation;
13. Frame/Reset behavior;
14. lighting/background;
15. render-target sizing/recreation policy;
16. model/resource lifetime and reload policy;
17. source-change detection/retry/failure behavior;
18. no-selection/fallback behavior;
19. Refresh interaction;
20. Import interaction;
21. Delete interaction;
22. thumbnail-cache independence;
23. simple asset information shown;
24. asset-selection vs scene-selection isolation;
25. authored-level isolation;
26. tests added/changed;
27. focused Preview/camera/resource tests;
28. M47/M48/M48.1/editor regressions;
29. Python results;
30. Debug/Development/Release builds;
31. canonical Level 01 verification;
32. asset/cache/repository safety;
33. `git diff --check`;
34. remaining risks/limitations.

### STOP condition

After implementation, tests, regressions, builds, repository/canonical-data verification, documentation and final report:

- Do **NOT** commit.
- Do **NOT** push.
- Do **NOT** merge.
- Do **NOT** start M49.
- Do **NOT** declare M48.2 CLOSED.
- **STOP.**

M48.2 then waits for mandatory manual user acceptance and Git closure.

### Implementation notes (post-inspection)

Reusable from M48.1: `ValidateStaticGlbFile`, raylib `LoadModel` / `GetModelBoundingBox` / `LoadRenderTexture` / `DrawModel`, source stamp (`mtime` + `size`), and bounds/framing math (moved to `editor/StaticModelFraming.h`). Thumbnail PNG/cache/GPU textures remain thumbnail-specific.

Preview: `render::StaticModelPreviewRenderer` keeps one loaded `Model` and a resizeable `RenderTexture`. Selection authority is `ContentBrowserState.selectedIdentity`. Orbit is LMB; zoom is wheel; pan is omitted. Default window visibility is **true** (same as Content Browser / Object Palette). Name/type/identity/bounds are in a collapsed-by-default **Asset Details** `CollapsingHeader` below the interactive preview (Correction 1).


## Milestone 49 — Authored Static Props

### Status
CLOSED. Milestone 50 is implemented and awaits manual acceptance.

### Branch
`milestone/49-authored-static-props`

### Goal
Introduce authored Static Prop instances that reference imported static GLB assets, allowing reusable project assets to become persistent visual objects in a level without yet implementing the Content Browser placement workflow.

### Core distinction
- **Static Model Asset**: reusable project asset discovered through the existing `StaticModelCatalog`, identified by its canonical project-relative asset identity.
- **Static Prop**: authored level instance that references one Static Model Asset and owns its authored transform/state.

M49 must preserve this distinction. An asset is not itself a scene object, and selecting an asset in the Content Browser is not scene selection.

### Repository-first architecture
Before implementation, inspect the real post-M48.2 repository and current conventions for `LevelDefinition`, Level Format v1, `workingCopy` / `active` / `savedSourceBaseline`, authored lifecycle requests, Hierarchy, Inspector, rendering, picking, gizmos, physics rebuild boundaries, cook/stage/reload, Content Browser Delete Asset, StaticModelCatalog/model loading, and all M47–M48.2 asset workflows.

Use the narrowest architecture that fits the current engine. Do not introduce ECS, GUIDs, prefabs, generic components, a generic scene-object framework, a generalized asset-instance system, generic dependency graph, generic asset database, or generic reference manager unless the existing repository already requires such an abstraction.

### Authored data
Add a repeatable authored Static Prop category to the current level-authoring model.

Each Static Prop must reference a canonical static model asset identity and must own a complete authored visual transform:

- `position`
- `rotation`
- `scale`

All three are part of the functional target of M49.

The exact Level Format v1 syntax and exact in-memory transform representation must be chosen only after inspecting the current parser/writer, rendering, Inspector, picking and gizmo architecture. Prefer the smallest backward-compatible Level Format v1 extension and the smallest representation consistent with the current repository.

Do not create a generic `Transform` component/framework merely to satisfy Static Props.

If `rotation` or `scale` would require a disproportionate architectural expansion that cannot be kept narrow and consistent with the actual repository, Cursor must **STOP and report the blocker to the user before omitting that capability or inventing a large framework**.

M49 must not silently degrade into a position-only Static Prop milestone without explicit user approval.

Static Prop transform is authored state. Runtime rendering must not mutate it.

### Authored transform vs gizmo scope
Authored transform data and manipulation tools are separate concerns.

M49 must own, persist and round-trip:

- `position`
- `rotation`
- `scale`

All three must be editable at minimum through the Inspector.

#### Gizmo requirements
- **Translate**: integrate with the existing translation gizmo when applicable.
- **Rotate**: implement a Rotate gizmo only if it can be added narrowly and coherently to the current gizmo architecture without redesigning the entire gizmo system.
- **Scale**: implement a Scale gizmo only if it can be added narrowly and coherently to the current gizmo architecture without redesigning the entire gizmo system.

Do not semantically reuse primitive `Resize` as if it were automatically equivalent to visual model `Scale`.

A justified absence of Rotate and/or Scale gizmos does **not** remove `rotation` or `scale` from the authored Static Prop data model. Those fields must still exist, persist, validate, render correctly and be editable through the Inspector.

The final Cursor report must state exactly which Static Prop transform operations are available through:
- Inspector;
- Translate gizmo;
- Rotate gizmo;
- Scale gizmo.

### Asset references and validation
Static Props reference assets by the existing canonical project-relative identity, for example:

`models/example.glb`

Do not add GUIDs.

On load/apply, validate references using the existing static-model/catalog/path-safety authority. A missing/invalid referenced asset must fail safely and diagnostically without arbitrary filesystem access or corrupting authored state.

Do not copy GLB data into the level file.

Do not serialize:
- absolute filesystem paths;
- cooked absolute paths;
- staged absolute paths;
- thumbnail hashes or PNGs;
- Model Preview state;
- runtime resource handles.

### M48 Delete Asset compatibility / reference safety
M49 introduces the first authored level references to Static Model Assets. Therefore the existing M48 **Delete Asset** operation must no longer be allowed to silently remove a Static Model Asset that is known to be referenced by authored Static Props in the currently relevant authored level context.

Before implementing the rule, inspect the real post-M48.2 authority model and determine which authored states are relevant for reference safety. The implementation must explicitly decide whether deletion checks:
- `workingCopy`;
- `active`;
- `savedSourceBaseline`;
- a narrow combination of these;
- or another existing authored authority that is actually canonical for the current editor flow.

Use the smallest rule consistent with the repository's real authority semantics.

At minimum, attempting to delete a Static Model Asset that the relevant authored authority knows is referenced by one or more Static Props must:
- fail safely;
- preserve the asset;
- preserve the referencing Static Props;
- preserve thumbnail/model-preview behavior;
- produce a clear diagnostic explaining that the asset is referenced.

Do not create:
- generic dependency graph;
- asset database;
- reference manager framework;
- force-delete;
- automatic retargeting;
- automatic deletion of referencing Static Props;
- automatic reference repair.

The check must be narrow and authored-level aware, not a speculative cross-project dependency system.

The final Cursor report must document precisely:
- which authored authority or authorities are checked;
- why that scope was chosen;
- whether unsaved pending references are protected;
- whether applied-but-unsaved references are protected;
- whether saved-baseline references are protected;
- how Delete Asset behaves on rejection.

### Editor integration
Integrate Static Props into the existing Development editor:
- Hierarchy category/entries;
- scene selection;
- Inspector;
- Add/Duplicate/Delete authored lifecycle;
- Apply/Revert/Save authority;
- Modified/Dirty semantics;
- editor viewport rendering;
- CPU/editor picking as appropriate;
- existing transform workflow where applicable.

Asset selection in Content Browser remains independent from Static Prop scene selection.

M49 may provide a narrow direct-add path needed to create a Static Prop for testing/authoring, but it must **not** implement the M50 Content Browser-to-viewport placement workflow. No drag/drop, ghost placement, click-to-place loop, repeated placement or double-click instantiate.

If a direct-add action needs an asset reference, use the smallest explicit UI consistent with current editor patterns; do not turn this into the M50 placement workflow.

### Rendering and resource lifetime
Render Static Props using the referenced real static GLB asset.

Reuse post-M48.2 model loading/rendering primitives where architecturally appropriate, but do not make level rendering depend on thumbnail cache or Model Preview state.

Avoid loading the same GLB independently every frame or once per instance if a narrow shared model-resource cache is clearly warranted by the actual repository. Any such cache must remain static-model resource infrastructure, not a speculative generic asset manager.

Multiple Static Props may reference the same model resource while owning independent authored transforms.

Static Props are visual authored objects in M49. They do not automatically become physics bodies, collision surfaces, gameplay interactables, placement surfaces, hazards, collectibles, triggers, inventory items, or grab/carry objects.

### Transform rendering behavior
The rendered Static Prop must apply its authored:
- position;
- rotation;
- scale.

The exact rotation representation may be Euler angles, quaternion-backed storage, matrix decomposition, or another narrow form only if it matches the actual repository architecture. Serialization must remain deterministic.

Scale must represent authored visual model scale, not primitive dimensions. Validate against non-finite, zero, negative or otherwise pathological values according to the narrowest rule consistent with the engine.

Picking must account for all implemented authored transform fields. If exact mesh picking is unnecessary, use the narrowest reliable transformed-bounds approach.

### Physics boundary
Do not add Static Props to Jolt merely because they are visible models.

Preserve existing physics body accounting and `PhysicsWorld::TryRebuild` behavior unless the current implementation genuinely requires an unrelated compatibility adjustment.

Static Props are non-physical by default in M49.

### Lifecycle and authority
Preserve the editor authority model:
- `workingCopy` contains pending authored edits;
- Apply validates/promotes to `active`;
- Revert restores the appropriate authored state;
- Save writes authored source state;
- runtime/transient rendering state is not authored state.

Add/Duplicate/Delete/transform changes to Static Props must follow the same authored lifecycle conventions as existing repeatable categories.

Expected behavior includes:
- Add Static Prop -> `workingCopy` changes;
- Duplicate -> `workingCopy` changes;
- Delete instance -> `workingCopy` changes;
- Inspector transform edit -> `workingCopy` changes;
- Apply -> validated props promote to `active`;
- Revert -> pending edits roll back according to current semantics;
- Save -> Static Props serialize;
- reload -> saved Static Props reconstruct.

### Hierarchy
Integrate Static Props into current Hierarchy conventions.

Use a repeatable category such as `Static Props` when consistent with existing category behavior.

Each instance must be individually selectable.

Reuse `StructuralIndexMap` or the actual current session-local mapping mechanism where appropriate.

Do not introduce folders, parenting, scene graph, or GUID-based hierarchy identity.

### Inspector
When a Static Prop is scene-selected, the Inspector must expose the minimum useful authored instance data:
- referenced static-model asset identity;
- `position`;
- `rotation`;
- `scale`.

All three transform fields must be editable through the Inspector in M49.

Use current Inspector patterns.

Do not create a generic property system, component inspector, material inspector, mesh inspector, shader inspector, or gameplay property system.

### Add / Duplicate / Delete instance semantics
#### Add
Creates one `workingCopy` Static Prop using a valid Static Model Asset reference and deterministic/default transform values. Does not change `active` until Apply.

#### Duplicate
Duplicates the selected Static Prop, preserving asset reference and complete authored transform, except for any already-established deterministic duplicate offset convention in the repository.

#### Delete instance
Removes only the selected authored Static Prop from `workingCopy`.

It must **not**:
- delete the underlying GLB asset;
- delete thumbnail cache;
- modify the StaticModelCatalog;
- alter Content Browser asset selection unless current scene-selection rules independently require it.

This is distinct from M48 Delete Asset.

### Content Browser selection vs scene selection
Preserve M48/M48.1/M48.2 boundaries.

Content Browser selection represents:
- reusable project asset selection.

Hierarchy/viewport selection represents:
- authored scene instance selection.

Creating a Static Prop may consume the currently selected asset identity if that is the chosen narrow direct-add workflow.

After creation, the Static Prop becomes a normal authored scene object. Content Browser selection remains independent.

Selecting a Static Prop must not silently change Content Browser selection.

Selecting an asset must not silently change scene selection.

Model Preview continues to follow Content Browser asset selection, not Static Prop scene selection.

### Runtime / reload
Applied Static Props must render from `active` authored data in the appropriate runtime/editor path.

Save and staged-runtime reload must preserve:
- asset reference;
- position;
- rotation;
- scale.

Do not introduce Development source fallback where staged runtime authority currently forbids it.

Inspect the cooker/stager and add only the minimum required handling so referenced static model assets are available through the correct runtime path.

### Cook / stage dependency handling
Inspect the post-M48.2 asset pipeline before deciding behavior.

If a level references a Static Prop asset, the existing cook/stage workflow must produce a runnable staged result without relying on Development source paths at runtime.

Implement only the narrow dependency mapping required for Static Props and existing static GLB assets.

Do not create a general dependency graph, package manager, registry database, GUID system, generic reference manager, or Asset Manager framework.

### Model Preview independence
M48.2 Model Preview remains an asset-inspection tool.

Static Prop scene rendering must not depend on:
- Model Preview window visibility;
- Model Preview current selected asset;
- Model Preview loaded model resource;
- Model Preview camera;
- Model Preview render target.

Selecting a Static Prop must not hijack Model Preview selection.

### Thumbnail independence
Static Props must not depend on:
- thumbnail PNG;
- thumbnail metadata;
- thumbnail cache hit;
- thumbnail GPU texture.

Thumbnail cache remains a derived Content Browser representation.

### Canonical data safety
Do not make semantic changes to:

`game/assets/source/levels/level_01.level`

merely to demonstrate M49.

Tests should use fixtures/disposable levels/assets.

The canonical Level 01 should remain unchanged unless the user explicitly approves a content change later.

Expected canonical state remains:
- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The legacy line:

`dynamic_box 0 5 0 1 1 1 30`

must remain absent.

### Development / Debug / Release
Development owns:
- Static Prop authoring UI;
- Hierarchy integration;
- Inspector transform editing;
- Add/Duplicate/Delete authoring;
- authoring-specific asset-selection bridge;
- gizmo manipulation where implemented.

Debug and Release must continue to build correctly and remain free of Development-only editor UI.

Runtime rendering/data code required by staged levels may exist outside Development-only code as appropriate.

### Required focused tests
Add focused tests for the actual architecture, including:
- Level Format v1 parse/write/round-trip for Static Props;
- multiple/repeatable Static Props;
- old level compatibility with zero Static Props;
- canonical asset identity/path validation;
- missing/invalid asset reference handling;
- position round-trip;
- rotation round-trip;
- scale round-trip;
- finite/valid transform validation;
- Add/Duplicate/Delete lifecycle;
- `workingCopy` / `active` / `savedSourceBaseline` authority;
- Apply/Revert/Save;
- Modified/Dirty semantics;
- Hierarchy/scene selection integration;
- Inspector integration for position/rotation/scale;
- viewport rendering/resource mapping at a testable boundary;
- resource reuse for multiple instances of the same GLB;
- picking with transformed Static Props;
- Translate gizmo behavior where applicable;
- Rotate gizmo behavior if implemented;
- Scale gizmo behavior if implemented;
- explicit confirmation that absence of Rotate/Scale gizmos does not remove authored rotation/scale;
- asset selection vs scene selection isolation;
- Model Preview isolation;
- no physics-body creation by default;
- cook/stage/reload of referenced static models;
- missing staged dependency behavior;
- thumbnail independence;
- M48 Delete Asset rejection when a referenced model is protected by the chosen authored authority;
- Delete Asset success for genuinely unreferenced assets;
- Delete Asset cancellation unchanged;
- reference-safety behavior across whichever of `workingCopy`, `active`, `savedSourceBaseline` are chosen as relevant;
- Debug/Development/Release boundaries;
- regressions for M47, M48, M48.1, M48.2 and existing authored object lifecycle.

Do not expose public production APIs solely for tests. Reuse existing test-access patterns.

### Manual acceptance targets
The user must be able to create at least one Static Prop through the narrow M49 authoring path, select it in Hierarchy/viewport as implemented, inspect its asset reference and complete authored transform, edit position/rotation/scale through the Inspector, Duplicate/Delete it, Apply/Revert/Save, reload it, and run the level with the real referenced GLB rendered at the authored transform.

Manual acceptance must also confirm:
- Content Browser asset selection remains separate;
- Model Preview remains functional and independent;
- position, rotation and scale persist;
- Translate gizmo integrates where appropriate;
- Rotate/Scale gizmo availability matches the documented narrow-scope decision;
- primitive Resize was not misrepresented as model Scale;
- no automatic physics/collision/gameplay behavior;
- no M50 placement workflow has appeared;
- Delete Asset refuses to delete a model that is referenced by Static Props according to the documented authored authority scope;
- Delete Asset still works for unreferenced disposable assets;
- canonical Level 01 remains unchanged after cleanup.

### Explicit non-goals
Do not implement:
- M50 viewport placement workflow;
- drag-and-drop to level;
- ghost placement;
- repeated click placement;
- double-click instantiate;
- automatic physics/collision for props;
- Grab/Carry;
- Pressure Plates;
- Inventory;
- generic interaction/component systems;
- ECS;
- GUIDs;
- prefabs;
- parenting/folders/scene graph;
- undo/redo;
- generic Transform/component framework;
- generic dependency graph;
- generic asset database;
- generic reference manager;
- force-delete;
- automatic retargeting;
- automatic deletion of referencing Static Props;
- material/mesh/texture/shader editing;
- generic Asset Editor;
- new asset formats;
- Level Format v2 unless absolutely forced by existing repository constraints and explicitly reported before implementation.

### Validation
Run all focused tests and relevant existing regressions, current Python asset/cook/stage tests, and:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
git diff --check
```

Verify canonical Level 01 has no unintended semantic diff.

### Cursor final report
Report:
1. repository infrastructure discovered before architecture choice;
2. root implementation approach;
3. exact files changed;
4. Static Model Asset vs Static Prop data model;
5. exact Level Format v1 syntax;
6. parser/writer/round-trip behavior;
7. asset identity/path validation;
8. missing/invalid asset policy;
9. complete authored transform representation;
10. position implementation;
11. rotation implementation;
12. scale implementation;
13. Inspector editability for position/rotation/scale;
14. Translate gizmo behavior;
15. Rotate gizmo behavior/decision;
16. Scale gizmo behavior/decision;
17. explanation proving primitive Resize was not incorrectly reused as model Scale;
18. `workingCopy` / `active` / `savedSourceBaseline` integration;
19. Modified/Dirty semantics;
20. direct-add workflow;
21. Hierarchy integration;
22. Inspector integration;
23. Duplicate/Delete instance semantics;
24. Content Browser asset selection isolation;
25. Model Preview isolation;
26. viewport rendering path;
27. model resource cache/lifetime/reuse;
28. thumbnail-cache independence;
29. viewport picking;
30. physics isolation;
31. cook mapping;
32. stage mapping;
33. runtime lookup/reload;
34. missing staged dependency behavior;
35. M48 Delete Asset compatibility;
36. exact authored authority/authorities checked for reference safety;
37. Delete Asset rejection behavior and diagnostic;
38. tests added/changed;
39. focused Static Prop test results;
40. M47–M48.2/editor/physics regressions;
41. Python results;
42. Debug/Development/Release builds;
43. canonical Level 01 verification;
44. asset/cache/repository safety;
45. `git diff --check`;
46. remaining risks/limitations.

### Stop rule
After implementation, automated validation, documentation and report: STOP.

Do not commit, push, merge, start M50, or declare M49 CLOSED.

If complete authored `position + rotation + scale` cannot be implemented without a disproportionate architectural expansion, STOP and report the blocker before reducing scope or inventing a large framework.

Manual acceptance and Git closure remain mandatory.

### Implementation notes (post-inspection)
- Data: `world::StaticPropSpec` (`modelIdentity`, `position`, Euler XYZ `rotationDegrees`, `scale`).
- Level Format v1: `static_prop <px> <py> <pz> <rx> <ry> <rz> <sx> <sy> <sz> <identity...>`. Identity last. No v2.
- Parse/Apply: grammar + finite transform + scale `> 0` + `TryParseStaticModelIdentity`. No on-disk existence check.
- Missing staged GLB: `StaticModelSceneStore` marks failed; renderer draws a fallback cube. No source fallback.
- Direct-add: Content Browser toolbar **Add Static Prop** and `Edit > Add > Static Prop` share `LevelEditorRequest::AddStaticProp` using Content Browser `selectedIdentity`. Not Object Palette / M50 placement.
- Correction 1: the menu item existed but was disabled unless `contentBrowser.selectedIdentity` already parsed as a valid Static Model identity, with no hover reason and no Content Browser action. Manual acceptance could not discover the workflow. The toolbar button uses the same `CanIssueAuthoredLifecycleRequest` / `HandleAuthoredLifecycleRequest` path; both surfaces now share disable-reason copy.
- Correction 2 (Edit menu): `Edit > Add > Static Prop` is wired correctly and routes the same `LevelEditorRequest::AddStaticProp` as the toolbar button; there is no enablement, submenu, routing, or gizmo-state defect. It is the only `Add` row whose enablement comes from another window, and that dependency was invisible in the menu bar, so a greyed row read as "the command does not exist". Kept (Option A) and made self-describing: the row's right-hand column shows the selected asset filename or `select asset` (`AddStaticPropMenuHint`), and the `Add` submenu carries a persistent `Static Prop uses the Content Browser selection.` note. The Content Browser button remains the primary direct-add UI.
- Correction 2 (staged diagnostic): the Inspector Static Prop panel and the Content Browser selected-asset line now report `Static model is not available in staged runtime assets, so the viewport draws a placeholder cube. Run Build > Cook & Stage.` Policy lives in `ClassifyStaticPropAsset` / `StaticPropAssetStateMessage`; the ImGui caller supplies the staged-file answer. No automatic cook/stage and no canonical-source fallback.
- Correction 2 (rendering): reproduced with all four models staged and applied. No renderer defect found. `DrawProp` pushes and pops its own matrix, `DrawModel` takes `Model` by value so no per-instance transform is written back to the shared resource, and the authored spec is read-only, so nothing accumulates across frames or contaminates another instance, identity, or later world/HUD drawing.
- Correction 2 (measured raw world bounds, Scale `(1,1,1)`): Barrel `3.43 x 3.01 x 3.48` (center `0.00, 0.87, 0.00`), `models/test_textured.glb` `2.00 x 2.00 x 2.00`, `models/test_authored.glb` `2.00 x 1.00 x 2.00`, `models/test_static.glb` `1.10 x 1.20 x 1.10`. The three test models are the *smallest* assets, so "unexpectedly large geometry" is not raw model size. Apparent size comes from proximity to the follow camera. Direct-add is unchanged and remains deterministic: camera-region X/Y with authored Z snapped to `spawn.z`; with the editor camera seeded from the gameplay pose that is the lane between the follow camera and the player, so repeated adds without moving the camera stack in one visible spot.
- Correction 2 (pre-Apply boxes): the pre-Apply representation is the M34/M41 cyan pending-Add ghost, sized from `kStaticPropDefaultLocalMin/Max` scaled by authored `scale`, which is why every pending prop looks like a similar unit box. `render::DebugWorldOverlay` is a per-frame local populated only while the editor is active, so it cannot reach Gameplay or mutate `active`. Not accidental fallback geometry, and no M50 ghost model rendering was added.
- Correction 2 (known limitation): picking and the selection highlight still use the unit local AABB proxy, not real GLB bounds, so a prop whose model is much larger or smaller than one unit has a pick box that does not match its silhouette.
- Correction 3 (Chest Gameplay): reproduced `models/Chest by Quaternius - O72u4Drp8k.glb` through source → cook → stage → LoadModel. The Chest **does load** (5 meshes, 4 materials, 0 textures, ~1.18×0.90×0.82 after raylib bakes the FBX `scale (100,100,100)` node transforms). It is smaller than the Barrel (~3.43×3.01×3.48). Default-add at camera-region X/Y and `spawn.z` (~`0.42, 1.54, 0`) does **not** put the gameplay camera `(2, 4.3, 12)` inside the Chest, and the look ray toward spawn does not immediately hit it. A hidden-window isolation pass showed that drawing Chest/Barrel cannot erase later greybox pixels or leave the next frame blank; HUD remaining visible matches ordinary 3D occlusion/sky, not a lost framebuffer. `DrawProp` now flushes the immediate batch before `DrawModel` and restores default shader/blend/depth/cull/texture afterward so a GLB without texcoords cannot leave `rlBegin` greybox draws unbound. Inspector reports loaded staged size at Scale `(1,1,1)`, current visual size, and warns when the gameplay camera/target is inside the loaded AABB. Scale is not auto-normalized. Direct-add placement is unchanged (not M50).
- Correction 4 (post-delete persistence): the user's Delete Chest → Apply → Gameplay still broken is not explained by a leftover instance. Apply copies `workingCopy` onto `active`; `Sync` already prunes unreferenced models with `UnloadModel`; draw submission is `active.staticProps` only. DrawMesh writes the last material `colDiffuse` onto the **default** shader (Chest Metal ≈ sky). Correction 3 restored state only after `DrawProp`, so a later zero-prop frame never healed. `RestoreGreyboxImmediateState` now runs at the start of every `DrawWorld` 3D pass (and after each prop): default shader, `colDiffuse` white, texture0. **Superseded:** user captures showed `colDiffuse = 1 1 1 1` in broken frames; leftover tint is same-pass hygiene, not the persistent blank world.
- Correction 5 (temporary diagnostic capture, removed): Development `Debug > Capture Next Gameplay Render Frame` was a one-shot framebuffer capture used to locate the blank Gameplay frame. It is not a shipped editor feature and was deleted in Correction 7 after the user confirmed the clip-plane fix.
- Correction 6 (persistent 3D clip planes): the divergent Gameplay state is `rlCullDistanceNear/Far`. Model Preview and thumbnails call `rlSetClipPlanes` for a tight model frame. Gameplay `BeginMode3D` builds the frustum from those getters; editor `BeginMode3DInRect` uses `RL_CULL_DISTANCE_NEAR/FAR` (0.05/4000), so the editor viewport can stay healthy while Gameplay is clear-only. A Chest-sized frame (~near 1.5, far 6) clips the gameplay camera (~13 units from origin); a Barrel-sized frame (~far 20) can still include the world. Ownership: Preview/thumbnail restore default clip planes after the offscreen pass; `DrawWorld` establishes `RL_CULL_DISTANCE_NEAR/FAR` again before `BeginMode3D`. `GreyboxImmediateStateTest` is the clip-plane regression (default framebuffer).
- Correction 7: removed Correction 5 capture UI/readback/triangle. Added Static Prop Scale gizmo (`EditorTransformMode::Scale`) editing `staticProp.scale` on independent X/Y/Z handles. Primitive Resize is unchanged. No uniform-scale hub, Rotate gizmo, physics, alias, or M50.
- Gizmos: Translate (position); Scale (Static Prop visual scale); Resize (primitive size). Rotation is Inspector-only.
- Delete Asset safety: union of `workingCopy` + `active` + `savedSourceBaseline` before `DeleteStaticModel`.
- Canonical Level 01 unchanged (0 Static Props).


## Milestone 50 --- Static Prop Placement Workflow

### Status

Implemented; awaiting mandatory manual acceptance. Not CLOSED. Milestone 49 is CLOSED. Milestone 51 has not started.

This milestone may begin only from a clean and synchronized `main` after
Milestone 49 is fully closed.

### Branch

`milestone/50-static-prop-placement`

### Recommended Cursor Model

**Grok 4.6 High --- Fast OFF**

------------------------------------------------------------------------

### 1. Goal

Milestone 50 turns the Static Model asset workflow completed in M47--M49
into a productive level-building loop.

The editor must allow a user to select an imported static model in the
Content Browser, enter a dedicated Static Prop placement mode, preview
the future instance in the 3D viewport, choose a valid authored
placement surface, and click to create the Static Prop.

The resulting object is the same authored Static Prop introduced in M49.
M50 must not create a second prop representation, placement-only object
type, asset registry, transform system, or lifecycle authority.

The intended production loop is:

**Import → Find → Preview → Place → Transform → Duplicate → Apply → Save
→ Cook/Stage → Run**

M50 focuses on the **Preview → Place** part and integrates it with the
existing M49 Static Prop lifecycle and editor tools.

------------------------------------------------------------------------

### 2. Existing Architecture to Reuse

M50 must inspect the current repository and reuse the actual
implementations and conventions established by previous milestones.

Relevant existing concepts include:

-   `StaticModelCatalog` as the static-model catalog authority.
-   Content Browser asset selection, independent from scene-object
    selection.
-   M48.1 thumbnails and List/Thumbnails view.
-   M48.2 Model Preview.
-   M49 authored Static Props.
-   M49 complete Static Prop authored transform:
    -   position
    -   rotation
    -   scale
-   M49 Static Prop Add/Duplicate/Delete lifecycle.
-   `workingCopy` / `active` / `savedSourceBaseline` authority.
-   Apply / Revert / Save / Modified behavior.
-   Hierarchy and Inspector integration.
-   Translate and Static Prop Scale gizmos.
-   Existing Object Palette / placement-surface concepts from earlier
    milestones where semantically appropriate.
-   Existing editor viewport picking/ray logic where appropriate.
-   Existing Ground / Platform / Slope placement surfaces.
-   M49 clip-plane isolation rules for Model Preview, thumbnails, and
    Gameplay.

Do not build parallel systems where an existing authority is adequate.

------------------------------------------------------------------------

### 3. Scope

#### 3.1 Placement entry point

The Content Browser must provide an explicit way to start placement for
the currently selected valid Static Model.

Use a clear action such as:

`Place Static Prop`

The exact UI placement should follow the current Content Browser
conventions.

The existing M49 direct-add behavior may remain available as a distinct
operation if it currently exists and remains useful.

**Direct Add and Placement are not the same operation.**

Direct Add creates immediately using its established deterministic
behavior.

Placement enters an interactive viewport placement mode and does not
author an object until placement is confirmed.

#### 3.2 Placement mode

Starting placement must create a transient editor placement state
containing only what is needed to place a Static Prop.

At minimum it needs the selected canonical static-model identity.

The placement state is **not** a `LevelDefinition` object.

Entering placement must not mutate:

-   `workingCopy`
-   `active`
-   `savedSourceBaseline`

until the user confirms a placement.

#### 3.3 Real-model placement preview

While Static Prop placement mode is active, render a transient
preview/ghost of the actual selected static model in the editor
viewport.

Do not use the old cyan primitive pending-add proxy as the primary
Static Prop placement preview.

The preview should make it obvious that it is not yet authored. Use the
narrowest visual treatment compatible with the current renderer, such as
tint/transparency/outline or another existing editor convention.

The preview must:

-   use the selected static-model identity;
-   use the same low-level model resource/bounds authority as the
    current Static Prop renderer where appropriate;
-   not reload the GLB every frame;
-   not mutate the shared model resource;
-   not become a scene object until confirmed;
-   not create physics;
-   not appear in Gameplay;
-   not affect Apply/Save/Dirty by merely moving the mouse.

#### 3.4 Placement surfaces

Reuse the existing authored placement-surface concept where appropriate.

For M50, valid placement surfaces are:

-   Ground
-   Platform
-   Slope

Static Props themselves are **not** placement surfaces in M50.

Dynamic Boxes are **not** placement surfaces.

Hazards, Collectibles, Checkpoints and other authored markers are not
placement surfaces.

Do not add arbitrary mesh-surface placement or
Static-Prop-on-Static-Prop placement.

#### 3.5 Surface hit and position

Use the current editor viewport ray/picking infrastructure or the
narrowest reusable part of the existing Object Palette placement
implementation.

The placement preview position must follow the valid surface under the
mouse.

The final authored position must be deterministic and correspond to the
preview shown at confirmation.

Avoid a second independent ray/surface algorithm if the existing
placement path can be safely reused.

#### 3.6 Surface contact / model bounds

Static Props have real model bounds.

The placement workflow should place the model so that its authored
preview rests sensibly on the selected surface rather than blindly
placing the model origin at the surface hit if that would visibly bury
or float normal models.

Use the existing loaded model bounds and authored transform where
practical.

For the initial M50 placement preview:

-   default rotation = the established Static Prop default;
-   default scale = the established Static Prop default;
-   use the model's local bounds to derive the vertical/support offset
    required for surface contact.

Do not modify the GLB or normalize its geometry.

Do not introduce pivots, custom origins, per-asset import settings, or
asset metadata.

If an unusual model has an unusual authored origin, M50 only needs
deterministic bounds-based placement consistent with the current model
representation.

#### 3.7 Confirmation

Primary viewport confirmation (normally left click, following current
editor conventions) creates exactly one authored Static Prop in:

`workingCopy`

using:

-   selected asset identity;
-   preview position;
-   default authored rotation;
-   default authored scale.

After creation:

-   the new Static Prop becomes the scene selection;
-   Hierarchy and Inspector reflect it;
-   the level becomes Modified according to existing authored lifecycle
    semantics;
-   `active` remains unchanged until Apply;
-   Save semantics remain unchanged.

#### 3.8 Repeated placement

A productive repeated-placement loop is in scope.

After confirming one Static Prop, placement mode should remain active
with the same asset selected so the user can place additional instances
without returning to the Content Browser after every click.

This behavior must be explicit and easy to exit.

The exact UI may follow existing Object Palette placement conventions.

Repeated placement must create one authored Static Prop per confirmed
click and must not accumulate duplicate requests from one click.

#### 3.9 Cancel / exit placement

The user must be able to cancel/exit Static Prop placement without
authoring the current preview.

At minimum support the editor's established cancel convention,
preferably `Esc` if that is already used by placement/tools.

Changing to an incompatible editor operation may also cancel placement
if that matches current editor conventions.

Cancel must:

-   remove the transient preview;
-   leave existing authored Static Props untouched;
-   not mutate `active`;
-   not mutate the saved baseline;
-   not mark the level Modified solely because placement was cancelled.

#### 3.10 Content Browser selection changes

Asset selection and scene selection remain separate.

If the user starts placement with asset A and then intentionally selects
asset B in the Content Browser while placement mode remains active,
choose the smallest coherent behavior consistent with the current UI
architecture:

-   either update the pending placement asset to B; or
-   require an explicit new `Place Static Prop` action.

Do not silently mix asset identity and scene selection.

Document and test the chosen behavior.

#### 3.11 Placement and existing transform tools

After a Static Prop is created, the normal M49 authored workflow takes
over.

The user may then use:

-   Inspector Position
-   Inspector Rotation
-   Inspector Scale
-   Translate gizmo
-   Scale gizmo
-   Duplicate
-   Delete
-   Apply
-   Revert
-   Save

M50 does not need to transform the transient preview with the normal
gizmos.

#### 3.12 Hierarchy / Inspector

The transient placement preview must not appear as a normal Hierarchy
object.

Only confirmed Static Props appear in Hierarchy.

Only confirmed Static Props are edited in Inspector as authored scene
instances.

Do not create a fake `workingCopy` entry just to drive the preview.

#### 3.13 Runtime and persistence

M50 does not change the Static Prop runtime representation or Level
Format syntax introduced by M49.

Once confirmed, a placed Static Prop must flow through the existing M49
path:

`workingCopy → Apply → active → Save → cook/stage → staged runtime rendering`

No new runtime placement concept is required.

------------------------------------------------------------------------

### 4. Placement UX Requirements

The workflow should support this sequence without unnecessary mode
switching:

1.  Open Content Browser.
2.  Find/select a static model.
3.  Click `Place Static Prop`.
4.  Move mouse over the editor viewport.
5.  See the real-model placement preview follow valid surfaces.
6.  Click to create.
7.  Move to another valid surface.
8.  Click to create another instance.
9.  Press `Esc` or the established cancel action to leave placement.
10. Select any created prop.
11. Translate/Scale or edit transform in Inspector.
12. Apply.
13. Save.

The user must not need to reopen the import dialog, Model Preview, or
Object Palette between repeated placements.

------------------------------------------------------------------------

### 5. Preview Validity Feedback

The user must be able to distinguish a valid placement from an invalid
one.

When the cursor has no valid Ground/Platform/Slope surface:

-   do not create a Static Prop on click;
-   do not guess a world position;
-   show the preview as invalid or hide it, following the narrowest
    current editor convention.

Do not place at camera target, origin, last valid hit, or another
fallback when the current cursor position is invalid.

------------------------------------------------------------------------

### 6. Resource and Renderer Safety

Placement preview rendering must preserve the renderer-state ownership
rules established by M49.

In particular:

-   no clip-plane leakage;
-   no shader/material state leakage into the editor or Gameplay;
-   no mutation of shared `Model` resources;
-   no per-frame `LoadModel`;
-   no resource double-unload;
-   preview lifetime must be deterministic;
-   switching/cancelling placement must not leave stale draw
    submissions.

If the current Static Model scene/resource store can be narrowly reused
for preview resources, prefer that over a second loader/cache.

Do not create a generalized asset manager.

------------------------------------------------------------------------

### 7. Authored-State Authority

Placement preview is transient.

Before confirmation:

-   `workingCopy` unchanged;
-   `active` unchanged;
-   `savedSourceBaseline` unchanged;
-   Modified/Dirty unchanged.

On confirmation:

-   exactly one Static Prop is added to `workingCopy`;
-   Modified becomes true according to current lifecycle;
-   `active` remains unchanged until Apply.

Apply/Revert/Save semantics remain those of M49.

------------------------------------------------------------------------

### 8. Direct Add Compatibility

M49 introduced direct-add of a selected Content Browser asset.

M50 must not accidentally convert Direct Add into interactive placement.

If Direct Add remains in the UI:

-   it continues to create immediately;
-   Placement is a separate explicit action;
-   both use the same Static Prop authored representation and
    validation;
-   both preserve asset-reference safety.

If repository inspection shows Direct Add has become redundant or
confusing, do not remove it opportunistically. Report the UX concern and
keep scope stable unless removal is necessary for correctness.

------------------------------------------------------------------------

### 9. Delete Asset Reference Safety

M49 prevents deletion of a static-model asset referenced by relevant
authored Static Prop authority.

M50 must preserve this.

A transient placement preview alone is not an authored level reference.

Do not expand M49 into a generalized dependency graph.

If Delete Asset is attempted while the same asset is only in transient
placement mode, use the narrowest safe behavior consistent with current
UI ownership, such as cancelling placement before deletion or rejecting
the action while placement is active.

Document the chosen behavior.

Confirmed Static Props must continue to protect their referenced asset
exactly as in M49.

------------------------------------------------------------------------

### 10. Level Format

No new Level Format version.

No new Static Prop syntax is required.

Placed props serialize through the M49 `static_prop` representation.

Do not introduce placement metadata into the level file.

Do not persist:

-   placement mode;
-   ghost state;
-   last mouse hit;
-   placement surface identity;
-   editor-only preview state.

------------------------------------------------------------------------

### 11. Development / Debug / Release

Interactive authored placement is a Development editor feature.

Debug and Release must remain free of Development editor UI.

Release runtime continues to render applied/staged Static Props through
the existing M49 runtime path.

------------------------------------------------------------------------

### 12. Explicitly Out of Scope

Do **not** implement in M50:

-   Static Prop collision;
-   Static Prop Jolt bodies;
-   physics configuration;
-   gravity;
-   mass/friction/restitution;
-   Grab/Carry;
-   pressure plates;
-   inventory;
-   interaction policies;
-   aliases/authored display names;
-   GUIDs;
-   ECS;
-   prefabs;
-   scene graph/parenting;
-   folders in Hierarchy;
-   undo/redo;
-   generic asset dependency graph;
-   drag-and-drop from Content Browser;
-   drag-and-drop from OS;
-   Static-Prop-on-Static-Prop placement;
-   arbitrary mesh-surface placement;
-   placement on Dynamic Boxes;
-   vertex/face snapping;
-   grid snapping framework;
-   rotation snapping;
-   scale snapping;
-   local/world transform-space framework;
-   Rotate gizmo;
-   Level Format v2;
-   generic placement framework for future gameplay objects.

M50 may reuse existing generic-enough placement helpers, but must not
create speculative abstractions for later milestones.

------------------------------------------------------------------------

### 13. Expected Tests

Cursor must inspect the actual test architecture and add focused
regression coverage at the narrowest practical boundaries.

At minimum cover equivalents of:

1.  placement cannot start without a valid selected static model;
2.  placement can start from a valid Content Browser static model;
3.  entering placement does not mutate `workingCopy`;
4.  entering placement does not mutate `active`;
5.  entering placement does not mutate saved baseline;
6.  moving preview does not mark Modified;
7.  valid Ground hit produces valid preview;
8.  valid Platform hit produces valid preview;
9.  valid Slope hit produces valid preview;
10. Dynamic Box is not a placement surface;
11. Static Prop is not a placement surface;
12. invalid/no hit cannot confirm;
13. confirmation creates exactly one Static Prop in `workingCopy`;
14. created identity equals selected asset identity;
15. created position matches preview result;
16. default rotation is correct;
17. default scale is correct;
18. bounds-based support offset is deterministic;
19. confirmation does not mutate `active` before Apply;
20. Apply promotes the placed prop;
21. Revert behavior remains correct;
22. repeated placement creates one instance per confirmation;
23. repeated instances remain independent;
24. cancel creates no object;
25. cancel clears transient preview;
26. cancel alone does not mark Modified;
27. confirmed prop becomes scene selection;
28. Content Browser asset selection remains independent from scene
    selection;
29. Hierarchy contains confirmed prop but not transient preview;
30. Direct Add behavior remains distinct and functional;
31. M49 Translate gizmo remains functional;
32. M49 Scale gizmo remains functional;
33. primitive Resize remains unchanged;
34. Delete Asset reference protection remains correct;
35. placement preview alone does not become a persistent authored
    dependency;
36. preview resource is not loaded every frame;
37. switching/cancelling does not leave stale preview submissions;
38. Preview/thumbnail/placement rendering does not leak clip planes into
    Gameplay;
39. Save/Level Format round-trip remains green;
40. no M50 placement state is serialized.

Do not expose production APIs solely for tests.

------------------------------------------------------------------------

### 14. Manual Acceptance

Manual acceptance must include at least:

#### A. Enter placement

-   Select Barrel or another valid imported static model in Content
    Browser.
-   Click `Place Static Prop`.
-   Confirm the editor enters placement mode without immediately
    creating a prop.

#### B. Real-model preview

-   Move over Ground.
-   Confirm the actual model preview follows the cursor/surface.
-   Confirm it visually rests on the surface rather than obviously using
    a primitive proxy.

#### C. Platform

-   Move over a Platform.
-   Confirm valid preview and placement.

#### D. Slope

-   Move over a Slope.
-   Confirm valid preview and placement orientation/position remains
    deterministic under M50 semantics.

M50 does not require automatic alignment of model rotation to the slope
normal unless the existing placement convention already does this
narrowly. Do not invent slope-normal rotation if not already supported.

#### E. Invalid target

-   Move over empty space or an invalid object.
-   Confirm click does not create a prop.

#### F. Repeated placement

-   Place several instances of the same asset without returning to
    Content Browser.
-   Confirm exactly one instance per click.

#### G. Cancel

-   Press the established cancel action.
-   Confirm preview disappears.
-   Confirm no extra prop is created.

#### H. Transform

-   Select a placed prop.
-   Use Translate.
-   Use Scale.
-   Edit Rotation in Inspector.
-   Confirm normal M49 transform workflow remains correct.

#### I. Apply / Revert

-   Place a prop.
-   Revert and confirm pending placement disappears according to
    existing authored semantics.
-   Place again.
-   Apply and confirm it becomes active.

#### J. Save / Reload

-   Save a disposable authored test if appropriate.
-   Reload.
-   Confirm the placed Static Prop persists with identity and transform.

#### K. Multiple assets

-   Place at least two different static-model assets.
-   Confirm each instance keeps the correct identity/resource.

#### L. Gameplay

-   Apply placed props.
-   Enter Gameplay.
-   Confirm they render correctly.
-   Confirm no preview/ghost appears in Gameplay.
-   Confirm the M49 Chest/clip-plane regression remains fixed.

#### M. Direct Add

-   If Direct Add remains available, confirm it still behaves as
    immediate add and is distinct from interactive placement.

#### N. Canonical cleanup

-   Remove manual test Static Props from canonical Level 01.
-   Apply and Save.
-   Confirm canonical Level 01 returns to 0 Static Props before closure.

------------------------------------------------------------------------

### 15. Validation

Run the current relevant C++ test suite, including actual equivalents
of:

-   Static Prop lifecycle tests
-   Static Prop render/transform tests
-   Static Prop scene/resource isolation tests
-   Content Browser tests
-   Content Browser thumbnail tests
-   Static Model Preview tests
-   Editor placement tests
-   Editor picking tests
-   Editor gizmo tests
-   Editor quick-toolbar tests
-   authored lifecycle integration tests
-   Level file tests
-   cook/stage/reload workflow tests
-   canonical scene cleanup tests
-   M49 clip-plane regression test

Run:

``` text
python tools/test_import_static_glb.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:

``` text
git diff --check
```

------------------------------------------------------------------------

### 16. Canonical Data Safety

`game/assets/source/levels/level_01.level` must not retain manual M50
placement fixtures at closure.

Expected final canonical state remains:

-   Static Props: 0 unless the user explicitly authorizes production
    authored props later.
-   Legacy `dynamic_box 0 5 0 1 1 1 30` remains absent.

Do not mechanically overwrite unrelated authored semantic changes.

------------------------------------------------------------------------

### 17. Completion Criteria

M50 is ready for user manual acceptance only when:

-   Content Browser can explicitly enter Static Prop placement mode.
-   Real selected model appears as transient viewport preview.
-   Ground / Platform / Slope placement works.
-   Invalid targets cannot create props.
-   Confirmation creates exactly one M49 Static Prop in `workingCopy`.
-   Repeated placement works.
-   Cancel works without authored mutation.
-   Bounds-based surface contact is deterministic.
-   Scene and asset selection remain separate.
-   Apply/Revert/Save remain correct.
-   Direct Add remains distinct if retained.
-   Translate and Scale gizmos remain correct after creation.
-   runtime rendering remains correct.
-   clip-plane regression remains fixed.
-   no physics/collision/interaction features were added.
-   no M51+ functionality was anticipated.
-   all required tests/builds pass.
-   canonical Level 01 is clean.

------------------------------------------------------------------------

### 18. STOP Rule

After implementation and validation, Cursor must:

-   report results;
-   identify any limitation or deviation;
-   STOP.

Cursor must **not**:

-   commit;
-   push;
-   merge;
-   start M51;
-   declare M50 closed.

M50 closes only after user manual acceptance and the separate Git
closure workflow.


## Milestone 51 --- Dynamic Box Grab / Carry

### Status

**Implemented, awaiting manual acceptance**

Milestone 50 is CLOSED. Do not declare M51 CLOSED until after user
manual acceptance and the separate Git closure workflow.

### Branch

`milestone/51-dynamic-box-grab-carry`

### Recommended Cursor Model

**Grok 4.6 High --- Fast OFF**

### 1. Goal

Introduce the first direct player-to-physics-object interaction: **Grab
/ Carry** for authored Dynamic Boxes.

The player can target a nearby Dynamic Box, grab it through one explicit
gameplay action, carry it in front of the player while preserving safe
physics/world collision, and release/drop it back into normal
simulation.

Gameplay loop:

**Approach Dynamic Box → Target → Grab → Carry → Move → Drop**

M51 is deliberately narrow. It is not a generic interaction framework.

### 2. Existing Architecture to Preserve

Inspect the current repository first and reuse its actual authorities: -
Jolt Physics and current CharacterVirtual/player architecture. -
Repeatable authored Dynamic Boxes. - Authored transform distinct from
transient simulated transform. - Runtime Dynamic Box rendering from
current Jolt pose. - Restart Run restores authored box transforms and
zeros velocities. - Checkpoint respawn does not globally reset Dynamic
Boxes. - M46 per-box recovery below `active.killPlane`. - Apply/reload
reconstruct runtime Dynamic Boxes from authored definitions. - Existing
input, collision-layer, physics-query, and runtime lifecycle
conventions. - Static Props remain non-physical and non-grabbable.

Do not replace adequate current architecture.

### 3. Core Behavior

#### 3.1 Grabbable scope

Only authored Dynamic Boxes are grabbable in M51.

Do not make Static Props or other authored/world objects grabbable. Do
not add a generic `Interactable` component or interaction-policy
framework.

#### 3.2 Runtime-only carry state

Carry state is transient runtime state and must not change Level Format
v1, `workingCopy`, `active` authored definitions, saved source,
Modified/Dirty, or authored Dynamic Box transforms.

Track only the runtime identity/reference needed to know whether and
which current Dynamic Box is carried. No GUIDs or serialized body
handles.

#### 3.3 Target acquisition

When not carrying, determine an eligible Dynamic Box reasonably in front
of the player/camera and within a finite maximum grab distance.

Requirements: - finite max range; - deterministic selection if several
candidates qualify; - only current runtime Dynamic Boxes; - aligned with
player/view intent; - no grabbing across the level.

Prefer an existing Jolt ray/query path if appropriate. Otherwise add the
narrowest Dynamic-Box-specific query. Do not create a generic
interaction targeting framework.

#### 3.4 Obstruction

Do not grab through obvious solid world geometry. Existing
Ground/Platform/Slope or equivalent solid collision between player and
candidate must prevent the grab where current physics authority can
establish this.

#### 3.5 Input

Inspect current bindings and add one clear action: **Grab / Drop**.

The same action may toggle: - not carrying → attempt Grab; - carrying →
Drop.

Follow current keyboard/gamepad conventions. No input-remapping system.

#### 3.6 Carry target

Carry the box at a stable, finite target position in front of the
player/camera. It follows player movement and relevant facing/view
direction.

Use existing fixed physics timing. Do not parent the body to the camera
or create a scene graph.

#### 3.7 Physics while carried

The carried box remains represented by its physics body. Drive it toward
the carry target with the narrowest stable Jolt-compatible mechanism
available in the current architecture.

Do not detach a visual-only model from physics. Do not permanently alter
authored state. Do not build a generalized joint/constraint framework
solely for M51.

#### 3.8 Collision policy

World collision must remain meaningful while carrying. The box must not
simply teleport through Ground/Platforms/Slopes/solid world geometry.

Prevent catastrophic player↔carried-box feedback. A narrow
suppression/adjustment of player-vs-current-carried-box collision is
acceptable if required, while preserving carried-box vs world collision.
Do not globally disable Dynamic Box collision.

Document the exact policy.

#### 3.9 Carry obstruction

If the desired carry point is obstructed, do not blindly teleport
through geometry. Let world collision constrain the body or narrowly
clamp/shorten the target using existing physics queries.

Prefer stable behavior over a complex gravity-gun simulation.

#### 3.10 Drop

Grab/Drop while carrying releases the box from its current runtime pose
into ordinary Dynamic Box simulation.

Normal Drop: - current runtime pose retained; - normal gravity/dynamics
resume; - authored transform unchanged; - no automatic authored reset; -
no arbitrary large release impulse.

#### 3.11 Lifecycle invalidation

Clear carry safely whenever runtime ownership/references can become
invalid, including relevant cases such as: - Restart Run; -
Apply/reload/rebuild; - runtime/level reconstruction; - carried-box
kill-plane recovery; - gameplay-state destruction/transition.

Never retain a dangling index/body reference.

#### 3.12 Checkpoint respawn

Preserve the established rule: checkpoint respawn does **not** globally
reset Dynamic Boxes.

Choose the narrowest safe carry policy during player checkpoint respawn.
Prefer releasing the carried box if continuing carry across player
teleport/reconstruction is ambiguous. Document/test it.

#### 3.13 Kill-plane recovery

M46 recovery remains authoritative and per-box. If the carried box is
recovered below `active.killPlane`, clear carrying safely before/with
recovery. Do not disable recovery while carrying or reset unrelated
boxes.

#### 3.14 Restart Run

Restart clears carry. All Dynamic Boxes still return to applied authored
transforms with zero velocities. No box remains implicitly carried
afterward.

#### 3.15 Apply/reload

Apply/reload clears carry before/as runtime Dynamic Boxes are
reconstructed. Do not preserve transient carry across reconstruction.

### 4. Player Feedback

Provide minimal readable runtime feedback for target/carry state using
current HUD/gameplay conventions.

A small prompt, reticle state, simple text, tint, or bounds indicator is
enough. No generic interaction UI and no inventory UI.

A lightweight target highlight may reuse existing rendering conventions;
do not build outline post-processing solely for this.

### 5. Runtime Identity Safety

Use the narrowest session/runtime identity compatible with current
Dynamic Box/physics architecture. No persistent GUIDs.

Any runtime index/body reference must be invalidated correctly on
rebuild, restart, reload, and recovery.

### 6. Physics Tuning

Use small explicit constants following repository conventions for
concepts such as max grab distance, carry distance, responsiveness,
correction velocity/force, and safety margin if needed.

Do not add these to Level Format or an editor physics panel.

Default expectation: existing authored Dynamic Boxes are grabbable
regardless of mass unless repository constraints prove this unsafe. If a
mass restriction becomes necessary, STOP and report rather than silently
inventing a gameplay rule.

### 7. Editor / Authoring

No new Dynamic Box Inspector property or `Grabbable` checkbox. Dynamic
Box itself is the explicit grabbable type in M51.

No Level Format change. Existing Dynamic Box authoring/lifecycle/gizmos
remain unchanged.

### 8. Explicitly Out of Scope

Do not implement: - Static Prop collision/physics/grabbing; - generic
Interactable/component/policy architecture; - inventory or inventory
pickup; - pressure plates/triggers/event graphs; - scripting; -
throwing/charged throw; - weapons/melee use of boxes; - held-object
rotation controls; - adjustable carry distance; - multiple carried
objects; - authored grab points/sockets; - mass/friction/restitution
editor; - new physics-layer editor; - GUIDs/ECS/prefabs/undo-redo; -
Level Format v2; - M52+ functionality.

### 9. Focused Tests

Inspect current tests and add narrow production-boundary coverage for
equivalents of: 1. no target without an eligible box; 2.
in-range/in-front Dynamic Box can target; 3. beyond-range box cannot
target; 4. non-Dynamic-Box cannot target; 5. deterministic
multiple-candidate choice; 6. solid obstruction prevents through-world
grab; 7. valid Grab enters carrying; 8. invalid Grab does nothing; 9.
exactly one box carried; 10. carried reference identifies correct
runtime box; 11. carry target follows movement; 12. carry target follows
facing/view; 13. carry distance bounded; 14. world collision remains
active; 15. no catastrophic player feedback; 16. carry does not mutate
authored transform; 17. no `workingCopy` mutation; 18. no authored
`active` mutation; 19. no Modified/Dirty mutation; 20. Drop clears
carry; 21. dropped box resumes simulation; 22. Drop does not
authored-reset; 23. no arbitrary large release impulse; 24. Restart
clears carry; 25. Restart still authored-resets all boxes; 26.
checkpoint respawn keeps non-global-reset semantics; 27. checkpoint
carry policy safe/deterministic; 28. Apply/reload clears carry; 29.
rebuild leaves no stale carry reference; 30. carried-box kill-plane
recovery clears carry; 31. recovery still resets only that box; 32.
other boxes unaffected; 33. repeated Grab/Drop stable; 34. grab another
box after drop; 35. feedback matches target; 36. feedback matches
carrying; 37. Static Props non-grabbable; 38. Dynamic Box authored
lifecycle unchanged; 39. Level Format unchanged; 40. physics body
accounting remains valid; 41. builds/configurations remain appropriate;
42. no M52+ architecture introduced.

Do not expose public production APIs solely for tests; reuse established
test-access conventions.

### 10. Manual Acceptance

Test at least: - nearby/in-front target feedback; - out-of-range
rejection; - Grab and stable carry; - movement/turning while carrying; -
carried-box collision against world geometry; - no catastrophic player
launch/push-through; - Drop and resumed gravity/dynamics; - repeated
re-grab/drop; - two Dynamic Boxes, only one carried; - obstruction
blocks through-wall grab; - Restart clears carry and restores all
boxes; - checkpoint respawn does not globally reset boxes and follows
documented carry policy; - kill-plane recovery remains per-box and
cannot leave stale carry; - Apply/reload cannot leave stale carry; -
runtime carry never marks authored level Modified and never persists
transient pose; - Static Props cannot be grabbed.

### 11. Regression Validation

Run relevant current C++ tests for Dynamic Box runtime/recovery, physics
rebuild/body budget, authored lifecycle, LevelFile, player movement,
restart/checkpoint behavior, rendering, editor reconstruction, canonical
cleanup, and preserve M49/M50 Static Prop/clip-plane regressions.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

If still part of the standard suite:

``` text
python tools/test_import_static_glb.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:

``` text
git diff --check
```

### 12. Canonical Data Safety

Do not leave manual M51 Dynamic Box fixtures in
`game/assets/source/levels/level_01.level` unless explicitly authorized.

The intentionally removed legacy line remains absent:

`dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

### 13. Completion Criteria

M51 is ready for manual acceptance when: - nearby/in-front Dynamic Boxes
can be targeted; - solid obstruction prevents through-world grab; -
exactly one box can be grabbed/carried; - carry follows player/view at
bounded distance; - world collision remains meaningful and stable; -
Drop safely resumes normal simulation; - authored state never changes
from carry; - Restart/checkpoint/recovery/rebuild semantics remain
correct; - no stale runtime reference survives reconstruction; - Static
Props remain non-grabbable; - no generic interaction framework was
introduced; - required tests/builds pass; - canonical Level 01 remains
safe.

### 14. STOP Rule

After implementation, validation, documentation, and report, STOP.

Do not commit, push, merge, start M52, or declare M51 CLOSED.

M51 closes only after user manual acceptance and the separate Git
closure workflow.


## Milestone 52 --- Pressure Plate / Dynamic Box Trigger

### Status

**Implemented — awaiting manual acceptance**

Milestone 51 is CLOSED. Runtime activation uses a direct AABB overlap
between the applied authored Pressure Plate region and current Dynamic
Box runtime bounds. Pressure Plates do not consume Jolt body slots.
Canonical Level 01 remains 0 Pressure Plates, 0 Dynamic Boxes, and 0
Static Props.

Do not declare M52 CLOSED until user manual acceptance.

M52 begins from clean synchronized `main`.

### Branch

`milestone/52-pressure-plate-box-trigger`

### Cursor

**Grok 4.6 High --- Fast OFF**

### Goal

Add the first repeatable authored gameplay trigger: a **Pressure Plate**
whose transient runtime state is active while at least one runtime
Dynamic Box overlaps its trigger region.

Puzzle loop: **Grab box → place on plate → plate activates → remove last
box → plate deactivates.**

M52 proves concrete detection/state only. It does not add doors,
outputs, event graphs, scripting, or generic trigger/action
architecture.

### Authored data

Add repeatable Pressure Plates to the existing LevelDefinition/authored
lifecycle with the smallest data required: - position/center; - trigger
size/extents.

Use current primitive-object conventions. No GUID, alias, target,
action, script, or serialized active state.

Extend **Level Format v1** with narrow repeatable syntax consistent with
the repository, conceptually:
`pressure_plate <cx> <cy> <cz> <sx> <sy> <sz>`

Validate finite position and positive/non-degenerate size; preserve
existing file/line/count limits and parse/save round-trip. No Level
Format v2.

### Lifecycle/editor

Integrate Pressure Plates with existing: - `workingCopy`, `active`,
`savedSourceBaseline`; - Add, Duplicate, Delete; - Apply, Revert,
Save; - Modified/Dirty; - Hierarchy and Inspector; - Position and Size
editing; - primitive Translate and Resize gizmos.

Integrate with existing Add/Object Palette placement conventions where
narrow and appropriate, reusing Ground/Platform/Slope placement
infrastructure rather than creating a new placement system. Do not alter
M50 Static Prop placement.

### Runtime

Runtime state is applied authored trigger region + transient activation
state.

Use the narrowest current Jolt-compatible representation. Prefer a
sensor/trigger if it integrates cleanly. A deterministic direct overlap
test between authored trigger AABB and current Dynamic Box runtime
bounds is also acceptable if substantially narrower/safer. Do not build
a generalized trigger framework.

A plate is **Active iff at least one current runtime Dynamic Box
overlaps it**.

Multiple boxes are supported: removing one keeps the plate active while
another remains; removing the last deactivates it. No latch/toggle.

Only Dynamic Boxes activate it. Player, Static Props, Platforms,
Collectibles, Hazards, Checkpoints and other plates do not.

M51 carry state is not itself an activation condition: a carried box
activates only through actual runtime overlap.

### Runtime lifecycle semantics

-   M46 kill-plane recovery: recompute overlap; no stale active state.
-   Restart Run: boxes restore as already defined; plate activation is
    recomputed, not restored from saved runtime state.
-   Checkpoint respawn: boxes are not globally reset; a box remaining on
    a plate keeps it active.
-   Apply/reload/rebuild: reconstruct/recompute without stale overlap
    references.
-   `workingCopy` changes do not affect runtime before Apply.
-   Runtime activation never marks Modified/Dirty and is never
    serialized.

### Visual feedback

Render a simple visible plate using current primitive world-render
conventions, with clear inactive vs active visual state (for example a
narrow tint/color convention). No animation/material/VFX/audio
framework.

Use current F1 metrics narrowly if appropriate (e.g. Pressure Plates /
Active Pressure Plates).

### Physics body budget

Preserve `kPhysicsMaxBodies = 64`. If plates use Jolt sensor bodies,
update transactional body accounting correctly. If direct overlap tests
use no body, explicitly preserve/test existing accounting.

### Out of scope

No doors, moving-platform control, hazard control, lights, target
references, channels, event bus, generic
Trigger/Activator/Action/Receiver components, scripting, visual
scripting, logic gates, timers, latching, player activation, Static Prop
activation, weight thresholds, required box counts, filters/tags, custom
plate assets, audio, animation framework, GUIDs, ECS, prefabs,
undo/redo, Level Format v2, or M53+ features.

### Tests

Add focused coverage for: - Level Format
parse/save/validation/repeatability; -
Add/Duplicate/Delete/Apply/Revert/Save; -
Hierarchy/Inspector/Translate/Resize; - inactive with zero boxes; -
active with one overlapping box; - deactivate when last box leaves; -
correct two-box occupancy; - player and Static Props do not activate; -
carried box activates only by overlap; - Drop-on-plate and re-grab-away
behavior; - kill-plane recovery removes stale activation; - Restart
recomputes; - checkpoint respawn preserves activation when box
remains; - Apply/reload has no stale overlap state; - pending
workingCopy edits do not affect active runtime; - activation does not
mutate authored state; - activation is not serialized; - multiple plates
are independent; - physics body accounting remains valid; - M51
Grab/Carry remains green; - M49/M50 Static Prop/clip-plane regressions
remain green; - no generic trigger/output system.

Do not expose public production APIs solely for tests; reuse established
test-access conventions.

### Manual acceptance

In Development: 1. Add Pressure Plate; verify
Hierarchy/Inspector/Translate/Resize; Apply. 2. Add/apply Dynamic Box.
3. Gameplay: use M51 Grab/Carry to put box on plate; plate becomes
active. 4. Remove box; plate becomes inactive. 5. Test two boxes: remove
one, still active; remove last, inactive. 6. Player standing on plate
alone must not activate it. 7. Static Prop must not activate it. 8.
Carried box activation follows actual overlap. 9. Checkpoint respawn
with box left on plate preserves box/activation. 10. Restart Run
recomputes from restored box positions. 11. Kill-plane recovery leaves
no stale active plate. 12. Apply/Revert/Save obey authored authority;
active state never persists. 13. M51 Grab/Carry still behaves normally.
14. Clean canonical Level 01 before closure.

Expected canonical closure state: 0 Pressure Plates, 0 Dynamic Boxes, 0
Static Props; legacy `dynamic_box 0 5 0 1 1 1 30` remains absent.

### Validation

Run relevant current C++ tests for LevelFile, authored lifecycle,
editor/gizmos, physics rebuild/body budget, Dynamic Box recovery, M51
Grab/Carry, restart/checkpoint, cook/stage/reload, canonical cleanup,
and M49/M50 regressions.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Run `python tools/test_import_static_glb.py` too if it remains in the
standard full suite.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check`.

### Completion

Ready for manual acceptance only when authored Pressure Plates fully
integrate, Dynamic Box overlap drives correct transient active/inactive
state, M51 works naturally with them, lifecycle/recovery/rebuild
semantics are correct, body budget is safe, no generalized
trigger/output architecture was introduced, tests/builds pass, and
canonical data is clean.

### STOP

After implementation and validation, report and STOP. Do not commit,
push, merge, start M53, or declare M52 CLOSED. Closure occurs only after
user manual acceptance and separate Git closure.


## Milestone 53 --- Authored Door & Pressure Plate Link

### Status

**Implemented — awaiting manual acceptance**

Milestone 52 is CLOSED. Do not declare M53 CLOSED.

### Branch

`milestone/53-pressure-plate-door`

### Recommended Cursor Model

**Grok 4.6 High --- Fast OFF**

------------------------------------------------------------------------

### 1. Goal

Close the first complete physical puzzle loop by adding a repeatable
authored **Door** and a narrow authored link from a Pressure Plate to
one Door:

**Grab Dynamic Box → place on Pressure Plate → plate becomes active →
linked Door opens → remove last box → plate becomes inactive → Door
closes.**

M53 deliberately implements one concrete producer/consumer relationship.
It must **not** introduce a generalized
trigger/receiver/event/channel/component framework yet.

The previous roadmap label "Game Feature Configuration" is
deferred/resequenced. The concrete Pressure Plate → Door use case comes
first so any later trigger abstraction is based on real requirements.

------------------------------------------------------------------------

### 2. Existing Architecture to Preserve

Inspect the repository first and preserve current authorities,
especially:

-   Level Format v1;
-   `workingCopy`, `active`, `savedSourceBaseline`;
-   authored repeatable-object lifecycle;
-   session-local structural indices;
-   Hierarchy / Inspector / scene selection;
-   primitive Translate / Resize gizmos;
-   Object Palette / placement conventions;
-   Jolt `PhysicsWorld` transactional rebuild and body budget;
-   Dynamic Box runtime simulation;
-   M46 kill-plane recovery;
-   M51 Grab / Carry;
-   M52 Pressure Plate authored/runtime overlap semantics;
-   Restart Run / checkpoint respawn;
-   Development/Debug/Release behavior;
-   staged runtime authority.

Do not build parallel authorities.

------------------------------------------------------------------------

### 3. Authored Door

Add a repeatable authored `Door` category.

Use the smallest authored data needed for M53:

-   closed position / center;
-   size;
-   opening travel distance (or equivalent narrow scalar/axis
    representation after repository inspection).

Default opening direction for M53 should be deterministic and simple.
Prefer **vertical +Y opening** unless current project conventions
strongly justify another fixed direction.

Do not add rotation, custom model identity, material, animation asset,
GUID, script, lock/key, health, interaction prompt, or generic receiver
component.

The authored position is the **closed** pose.

------------------------------------------------------------------------

### 4. Pressure Plate → Door Link

Extend the authored Pressure Plate with the narrowest reference needed
to control one Door.

Because the repository intentionally has no persistent GUID system, use
a Level Format v1-compatible **authored Door index/reference** if it can
be made safe and deterministic within current save/parse/lifecycle
conventions.

The reference must mean the Door's authored order in the same
LevelDefinition, not a Jolt BodyID or runtime pointer.

Use an explicit "no linked door" representation.

Requirements:

-   a Pressure Plate may link to zero or one Door in M53;
-   multiple Pressure Plates may link to the same Door;
-   one Pressure Plate does not control multiple Doors yet;
-   references are validated;
-   Delete/Duplicate/reordering semantics must not silently retarget
    links;
-   if the current lifecycle makes stable index maintenance unsafe, STOP
    and report the concrete architectural conflict rather than inventing
    GUIDs or a generic ID system.

Do not introduce named channels or persistent GUIDs.

------------------------------------------------------------------------

### 5. Level Format v1

Keep Level Format v1.

Add repeatable Door syntax consistent with current parser conventions,
conceptually:

`door <cx> <cy> <cz> <sx> <sy> <sz> <openDistance>`

Extend Pressure Plate syntax with its optional/narrow Door reference
using the safest repository-consistent form discovered during
inspection.

Requirements:

-   finite values;
-   positive/non-degenerate Door size;
-   finite positive opening distance within a sensible safety bound;
-   valid/no-link Pressure Plate reference;
-   parse/save round-trip;
-   current file/line/count limits;
-   no runtime open fraction/state serialized;
-   no Level Format v2.

Document the exact final syntax.

------------------------------------------------------------------------

### 6. Door Authored Lifecycle

Door must integrate with:

-   Add;
-   Duplicate;
-   Delete;
-   Apply;
-   Revert;
-   Save;
-   Modified/Dirty;
-   `workingCopy`;
-   `active`;
-   `savedSourceBaseline`.

Hierarchy group: `Doors`.

Inspector exposes only the authored fields actually chosen for M53,
expected:

-   Position;
-   Size;
-   Open Distance.

Reuse primitive Translate / Resize semantics where appropriate.

No Rotate gizmo.

Object Palette/direct Add should follow existing primitive
authored-object conventions and reuse Ground/Platform/Slope placement
infrastructure where appropriate.

------------------------------------------------------------------------

### 7. Pressure Plate Inspector Link

Pressure Plate Inspector should expose a narrow Door link editor.

Prefer a deterministic combo/select control:

-   `None`;
-   Door 0 / Door 1 / ... with useful current editor labeling.

Do not ask the user to type raw indices if current ImGui/editor
conventions allow a safer selector.

Changing the link edits `workingCopy` only and follows Apply/Revert/Save
authority.

No runtime link mutation.

------------------------------------------------------------------------

### 8. Reference Integrity

Door lifecycle operations must preserve semantic links.

At minimum:

-   deleting a Door clears links that pointed to that Door;
-   links to later Doors are remapped if authored indices shift;
-   duplicating a Door does not automatically steal or duplicate
    Pressure Plate links;
-   duplicating a Pressure Plate preserves its current Door link only if
    that is safe and deterministic under current lifecycle conventions;
-   Revert restores prior links;
-   Save/reload preserves links;
-   invalid file references fail validation rather than silently
    retarget.

Implement this narrowly for Pressure Plate → Door only. Do not create a
generic reference graph.

------------------------------------------------------------------------

### 9. Runtime Door Representation

A Door is a solid world obstacle.

Use the narrowest Jolt-compatible representation consistent with
existing architecture, preferably a **kinematic box body** driven
between closed and open positions.

It must collide meaningfully with:

-   player;
-   Dynamic Boxes;
-   carried Dynamic Boxes;
-   other existing solid-world participants as current collision layers
    permit.

Do not implement the Door as visual-only if that would allow the
player/boxes to pass through while closed.

Door body allocation must participate in `kPhysicsMaxBodies = 64`
transactional accounting.

------------------------------------------------------------------------

### 10. Door Activation Semantics

For each Door, derive a runtime desired-open state from applied Pressure
Plates linked to it.

Rule:

**Door desired-open = true if at least one linked Pressure Plate is
Active.**

Therefore:

-   zero linked active plates → close;
-   one linked active plate → open;
-   two linked plates active → open;
-   one of two deactivates while the other remains active → stay open;
-   final linked plate deactivates → close.

This gives useful OR semantics without introducing logic gates.

Do not add AND mode, inversion, latch, toggle, delay, timer, or authored
logic settings.

------------------------------------------------------------------------

### 11. Door Motion

Door moves smoothly between:

-   closed authored position;
-   open position = closed position + deterministic opening travel.

Use narrow tuning constants for speed/responsiveness.

Do not serialize runtime open fraction.

The Door should not teleport between states during ordinary gameplay.

Opening/closing must be deterministic and bounded.

Do not add an animation framework.

------------------------------------------------------------------------

### 12. Obstruction Safety

Closing a Door must not catastrophically crush, launch, tunnel through,
or destabilize the player or Dynamic Boxes.

Use current Jolt kinematic collision behavior and the narrowest safe
policy.

If a player or Dynamic Box blocks closing, prefer a safe behavior such
as pausing/reopening/remaining blocked rather than forcing the Door
through the object.

Do not add a generalized obstruction/door framework.

Report the exact policy.

------------------------------------------------------------------------

### 13. M51 / M52 Integration

M51 remains unchanged:

-   Dynamic Box Grab / Carry continues to work;
-   carried boxes remain physical;
-   Door collision must remain meaningful.

M52 remains unchanged in detection authority:

-   Pressure Plate Active comes only from actual Dynamic Box overlap;
-   player and Static Props do not activate;
-   multiple boxes behave correctly.

M53 consumes the M52 derived active state. It must not replace or
duplicate Pressure Plate overlap logic.

------------------------------------------------------------------------

### 14. Restart / Checkpoint / Recovery / Rebuild

#### Restart Run

Dynamic Boxes reset according to existing semantics. Door runtime
motion/state must recompute from resulting Pressure Plate states. No
stale open fraction/reference survives improperly.

#### Checkpoint respawn

Dynamic Boxes are not globally reset. If a box remains on a linked
plate, the Door should remain/open according to that plate. Player
respawn alone must not reset the puzzle.

#### Kill-plane recovery

If an activating box is recovered and leaves the plate, M52 deactivates
the plate and the linked Door responds accordingly.

#### Apply/reload/rebuild

Reconstruct Door bodies and links from `active`. No stale body IDs,
runtime pointers, open-state ownership, or invalid references.

Pending `workingCopy` changes do not affect Gameplay until Apply.

------------------------------------------------------------------------

### 15. Visual Representation

Use a simple primitive Door representation consistent with current
greybox rendering.

The visual transform must match the runtime collision body closely
enough for manual testing.

No custom GLB requirement, material system, VFX, audio, or animation
assets.

Pressure Plate M52 green/inactive feedback remains unchanged.

------------------------------------------------------------------------

### 16. Physics Body Budget

Doors are expected to consume Jolt bodies if implemented as solid
kinematic boxes.

Update body accounting transactionally.

Current known accounting before M53:

-   fixed bodies = 5;
-   Platforms + Dynamic Boxes share the remaining 59;
-   M52 Pressure Plates consume no bodies.

M53 must update the formula based on the actual implementation, expected
conceptually:

`fixedBodies + platformCount + dynamicBoxCount + doorCount <= 64`

Preserve `TryRebuild` transactional failure semantics.

No body leak across rebuilds.

------------------------------------------------------------------------

### 17. Explicitly Out of Scope

Do not implement:

-   generic Trigger system;
-   generic Receiver system;
-   event bus;
-   named channels;
-   GUIDs;
-   generic object references;
-   one plate → many doors;
-   arbitrary trigger → arbitrary receiver;
-   switches/buttons;
-   proximity triggers;
-   player-activated plates;
-   door interaction with E;
-   keys/locks;
-   inventory;
-   door rotation/hinges;
-   sliding-axis editor;
-   horizontal/custom opening direction;
-   animation curves;
-   audio;
-   VFX;
-   custom Door GLB;
-   logic gates;
-   AND/NOT/XOR;
-   delays/timers;
-   latch/toggle;
-   scripting;
-   visual scripting;
-   ECS;
-   prefabs;
-   undo/redo;
-   Level Format v2;
-   the deferred generalized Game Feature Configuration milestone;
-   M54/M55 functionality.

------------------------------------------------------------------------

### 18. Focused Tests

Add focused regression coverage for actual equivalents of:

1.  Door Level Format parse;
2.  Door save/round-trip;
3.  invalid Door position/size/open distance rejected;
4.  repeatable Doors;
5.  Pressure Plate no-link parse/save;
6.  Pressure Plate valid Door link parse/save;
7.  invalid Door link rejected;
8.  Add/Duplicate/Delete Door lifecycle;
9.  Apply/Revert/Save Door lifecycle;
10. Door Hierarchy/Inspector;
11. Door Translate/Resize;
12. Pressure Plate link selector edits workingCopy only;
13. deleting linked Door clears/remaps references correctly;
14. deleting an earlier Door remaps later references;
15. duplicating Door does not silently retarget plates;
16. duplicating linked Pressure Plate has deterministic documented
    behavior;
17. one inactive linked plate keeps Door closed;
18. active linked plate opens Door;
19. removing box/deactivating plate closes Door;
20. two plates linked to one Door use OR semantics;
21. one of two remaining active keeps Door open;
22. final deactivation closes;
23. unlinked active plate does not affect Door;
24. plate linked to Door A does not affect Door B;
25. smooth bounded opening motion;
26. smooth bounded closing motion;
27. closed Door collision blocks player;
28. closed Door collision blocks Dynamic Box;
29. carried Dynamic Box interacts safely with Door;
30. obstruction during closing follows safe documented policy;
31. runtime motion does not mutate authored Door position;
32. runtime open state is not serialized;
33. Restart recomputes puzzle state;
34. checkpoint respawn preserves puzzle when box remains on plate;
35. kill-plane recovery propagates plate deactivation to Door;
36. Apply/reload/rebuild has no stale references/body IDs;
37. pending workingCopy Door/link edits do not affect runtime before
    Apply;
38. body budget includes Doors correctly;
39. over-budget rebuild fails transactionally;
40. no body leak after repeated rebuild;
41. M51 Grab/Carry regressions;
42. M52 Pressure Plate regressions;
43. M49/M50 Static Prop regressions;
44. canonical Level 01 cleanup;
45. no generic trigger/receiver/event system introduced.

Do not expose public production APIs solely for tests. Reuse established
test-access patterns.

------------------------------------------------------------------------

### 19. Manual Acceptance

In Development, using disposable fixtures:

1.  Add a Door; verify Hierarchy, Inspector, Translate, Resize.
2.  Add a Pressure Plate and link it to the Door in Inspector.
3.  Add/apply a Dynamic Box.
4.  Gameplay: closed Door blocks passage.
5.  Grab/carry/drop box onto linked plate.
6.  Plate turns active and Door opens smoothly.
7.  Walk/pass through the opened doorway.
8.  Remove the box; Door closes smoothly.
9.  Verify an unlinked plate does not control the Door.
10. If practical, link two plates to one Door and verify OR semantics.
11. Put an object/player in the closing path and verify safe obstruction
    behavior.
12. Verify M51 Grab/Carry remains stable around Door collision.
13. Leave a box on plate and checkpoint-respawn; puzzle state remains
    derived from the box.
14. Restart Run; puzzle recomputes from restored box positions.
15. Exercise kill-plane recovery; no stale Door-open state.
16. Test Apply/Revert/Save/reload of Door and link.
17. Ensure runtime Door motion never changes saved closed position.
18. Remove all manual M53 fixtures before Git closure.

Expected canonical closure state:

-   Pressure Plates: 0;
-   Doors: 0;
-   Dynamic Boxes: 0;
-   Static Props: 0.

The intentionally removed legacy Dynamic Box line remains absent.

------------------------------------------------------------------------

### 20. Validation

Run all relevant C++ tests, including LevelFile, lifecycle, editor,
gizmo, physics rebuild/body budget, M46, M51, M52, restart/checkpoint,
cook/stage/reload, M49/M50 regressions and canonical cleanup.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

If still standard:

``` text
python tools/test_import_static_glb.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:

`git diff --check`

------------------------------------------------------------------------

### 21. Canonical Data Safety

Do not retain manual M53 Door/Pressure Plate/Dynamic Box fixtures in:

`game/assets/source/levels/level_01.level`

unless explicitly authorized.

Expected closure state: 0 Doors, 0 Pressure Plates, 0 Dynamic Boxes, 0
Static Props.

Keep the intentionally removed line absent:

`dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

------------------------------------------------------------------------

### 22. Completion Criteria

M53 is ready for manual acceptance when:

-   authored repeatable Doors exist;
-   Pressure Plates can safely reference one authored Door;
-   reference integrity survives lifecycle operations;
-   linked active plate opens the Door;
-   final linked plate deactivation closes it;
-   multiple linked plates provide simple OR semantics;
-   Door is a meaningful solid runtime obstacle;
-   motion is smooth and bounded;
-   closing obstruction is safe;
-   M51/M52 behavior remains authoritative;
-   runtime state never mutates authored state;
-   body accounting is transactional and safe;
-   no generalized trigger/receiver/event architecture was introduced;
-   tests/builds pass;
-   canonical data is clean.

------------------------------------------------------------------------

### 23. STOP Rule

After implementation, validation and report, Cursor must STOP.

Do not commit, push, merge, start the next milestone, or declare M53
CLOSED.

Closure occurs only after user manual acceptance and the separate Git
closure workflow.


## Milestone 54 --- Player Inventory System v1

### Status

**Implemented — awaiting manual acceptance**

Milestone 53 is CLOSED. M54 begins only from clean synchronized `main`.

### Branch

`milestone/54-player-inventory-v1`

### Cursor

**Grok 4.6 High --- Fast OFF**

### Goal

Introduce a small reliable **player runtime inventory** without
connecting it to world pickups yet:

**item identity → quantity → Add / Remove / Query → runtime player
inventory → Development inspection**

Inventory is runtime gameplay state, not authored Level state. M54 does
not implement world pickups, Static Prop interaction, `E` pickup,
polished inventory UI, equipment, item assets/catalogs, save-game
persistence, or generic interaction architecture. Those integrations
remain later scope, especially M55.

### Preserve current architecture

Inspect and reuse current player/session lifecycle, input/HUD/debug
conventions, Level Format v1 authority, editor
`workingCopy`/`active`/`savedSourceBaseline`, Restart Run, checkpoint
respawn, PhysicsWorld rebuild, M51 Grab/Carry, M52 Pressure Plates, M53
Doors, and Development/Debug/Release separation.

Do not put Inventory into LevelDefinition and do not create parallel
runtime authority.

### Inventory ownership

Add one runtime Inventory at the narrowest existing
gameplay/player-session authority. Contents are transient and never
mutate or serialize `workingCopy`, `active`, `savedSourceBaseline`,
Modified/Dirty, or Level Format.

### Item identity

Use a bounded canonical string/token `itemId`. Follow existing
repository token-validation conventions where possible. Make case policy
explicit and deterministic. Tests may use IDs such as `key`, `coin`,
`battery`.

Do not add GUIDs, ItemDefinition assets/catalog/database, icons, meshes,
descriptions, rarity, value, weight, or classes.

### Data model/API

Smallest model: unique canonical `itemId` + positive integral quantity.
No duplicate logical IDs.

Production operations, using repository naming: - query quantity; -
`Has(itemId, quantity)`; - `TryAdd(itemId, quantity)`; -
`TryRemove(itemId, quantity)`; - `Clear()`; - read-only deterministic
enumeration.

Missing item returns zero. Add positive N increases quantity. Remove
succeeds only when enough exists; exact removal removes the entry.
Invalid IDs/quantities, over-removal and overflow fail without mutation.
Use bounded safe arithmetic.

No slots, capacity, weight, per-stack limits, grid, hotbar or equipment.

### Runtime lifecycle

-   Initial/new gameplay session: empty inventory.
-   **Checkpoint respawn preserves Inventory.**
-   **Full Restart Run clears Inventory to empty.**
-   PhysicsWorld rebuild alone must not clear Inventory.
-   Apply/reload must not accidentally make Inventory physics-owned.
    Inspect actual lifecycle and document exact boundary; only an
    operation that semantically starts/restarts the full run may clear
    it.

### M51/M52/M53 isolation

Dynamic Boxes are not inventory items. Grab/Carry does not alter
inventory. Pressure Plates and Doors do not read inventory. Do not make
keys open Doors. No item-trigger integration.

### Development inspection

Add a narrow **Development-only** inspection/test harness, preferably in
an existing gameplay/debug/tool window rather than a large new UI.

It must show current item IDs/quantities and allow manual: - Add by
itemId + quantity; - Remove by itemId + quantity; - Clear.

Use the production mutation API. This is test tooling, not final
inventory UI. No icons, grid, drag/drop, inventory screen or polished
UX. It must not appear in Release; preserve current Debug/ImGui
separation.

### Safety/determinism

Canonical validation, no duplicate logical IDs, overflow-safe
operations, non-mutating failures, deterministic enumeration, no
dangling references, no physics BodyID or authored-index dependency.

### Explicitly out of scope

No world pickups; Collectible→inventory conversion; Static Prop pickup;
`E` collect interaction; generic Interactable/Interaction component;
item definition catalog/database or `.item` format;
icons/thumbnails/models/descriptions/rarity/value/weight;
capacity/slots/per-stack
max/grid/drag-drop/sorting/filtering/hotbar/equipment/weapons/consumable
use; dropping items into world; crafting; quests; keys/locks; save-game
or cross-launch persistence; checkpoint inventory snapshots;
multiplayer/replication; inventory events/event bus; trigger/receiver
integration; GUIDs; ECS; prefabs; undo/redo; Level Format v2;
generalized Game Feature Configuration; M55 functionality.

### Focused tests

Cover actual equivalents of: 1. new Inventory empty; 2. valid canonical
ID accepted; 3. empty/invalid ID rejected; 4. case policy enforced; 5.
Add one; 6. Add quantity; 7. repeated Add merges logical entry; 8.
missing query returns zero; 9. Has exact; 10. Has over-owned false; 11.
partial Remove; 12. exact Remove removes entry; 13. over-remove fails
unchanged; 14. zero/invalid quantity rejected; 15. overflow fails
unchanged; 16. invalid Add/Remove non-mutating; 17. Clear; 18.
deterministic unique enumeration; 19. Restart Run clears; 20. checkpoint
respawn preserves; 21. PhysicsWorld rebuild preserves; 22. M51/M52/M53
updates do not mutate inventory; 23. inventory mutation does not mark
editor Modified/Dirty; 24. inventory absent from Level serialization;
25. Apply does not make inventory authored; 26. Development
Add/Remove/Clear use production API; 27. Release has no ImGui inventory
tool; 28. M51 regressions; 29. M52 regressions; 30. M53 regressions; 31.
M49/M50 regressions; 32. canonical Level 01 unchanged; 33. no M55
pickup/interaction scope.

Do not expose public production APIs solely for tests; reuse established
test-access conventions.

### Manual acceptance

Development: 1. Start Gameplay; inventory empty. 2. Add `key` x1; then
x2; confirm one `key = 3`. 3. Add `coin` x5; identities independent. 4.
Remove `key` x1; confirm 2. Attempt over-remove; unchanged. Remove final
2; entry disappears. 5. Add items and Clear; inventory empty. 6. Add
item, checkpoint respawn; inventory preserved. 7. Add item, full Restart
Run; inventory cleared. 8. Exercise Grab/Carry, Pressure Plate and Door;
none mutate inventory. 9. Inventory operations do not mark level
Modified/Dirty and Save does not serialize contents. 10. Release has no
Development inventory tool.

No canonical Level 01 fixture should be needed.

### Validation

Run relevant C++ tests: Inventory, player/session lifecycle, Restart,
checkpoint, Physics rebuild, editor isolation, M51, M52, M53, M49/M50,
LevelFile/canonical cleanup.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

If still standard: `python tools/test_import_static_glb.py`.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check`.

### Canonical data safety

M54 should require no canonical level edits. Verify
`game/assets/source/levels/level_01.level` remains semantically
unchanged: 0 Doors, 0 Pressure Plates, 0 Dynamic Boxes, 0 Static Props.
Keep legacy `dynamic_box 0 5 0 1 1 1 30` absent.

### Completion

Ready for manual acceptance when runtime Player Inventory exists; item
IDs/quantities are safe; Add/Remove/Has/Query/Clear work; checkpoint
preserves; Restart clears; physics rebuild preserves; inventory is not
Level-authored state; Development has narrow inspection tooling; Release
has no Development inventory UI; M51/M52/M53 remain unaffected; no world
pickup/generic interaction system exists; tests/builds pass; canonical
data is unchanged.

### STOP

After implementation, validation and report, STOP. Do not commit, push,
merge, start M55, or declare M54 CLOSED. Closure occurs only after user
manual acceptance and separate Git closure.


## Milestone 55

Not started. Do not implement during Milestone 54.


## Later milestones
Animation, enemies, collectibles, level editor, audio, save system, profiling/optimization, Raspberry Pi validation, Android port, iOS feasibility/backend work.
