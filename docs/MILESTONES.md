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

# Milestone 20 — Checkpoint + Fall/Respawn Loop [implementation complete / awaiting final manual validation]

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

**Phase C:** Implementation is complete and awaiting final user manual validation. `cmake --preset windows-vs2022` plus Debug/Development/Release builds succeed. Structural isolation holds. Development and Release smoke-launched without crash, ERROR, or WARNING; Release from an unrelated CWD still loads M15–M19 assets executable-relative and has no metrics-panel strings. Manual gameplay cases A–L remain for the user. Do not start Milestone 21. Do not commit until those manual tests are approved.

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

## Milestone 21— Another Commit
- Python environment and cooker skeleton.
- Source/cooked asset separation.
- Incremental cooking foundation.

## Later milestones
Animation, enemies, collectibles, level editor, audio, save system, profiling/optimization, Raspberry Pi validation, Android port, iOS feasibility/backend work.
