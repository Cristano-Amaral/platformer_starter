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

## Milestone 08 — Platformer Camera Follow [ACTIVE]

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

## Milestone 09 — Asset Pipeline
- Python environment and cooker skeleton.
- Source/cooked asset separation.
- Incremental cooking foundation.

## Later milestones
Animation, enemies, collectibles, level editor, audio, save system, profiling/optimization, Raspberry Pi validation, Android port, iOS feasibility/backend work.
