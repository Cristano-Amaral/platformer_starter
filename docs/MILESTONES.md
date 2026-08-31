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

## Milestone 03 — Semantic Input and Player Movement [ACTIVE]

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

## Milestone 04 — Physics Integration
- Introduce Jolt only if the prototype shows it is justified.
- Preserve gameplay-facing collision/movement abstraction.

## Milestone 05 — Menus + Debug Tools
- Main menu, pause menu, Development-only diagnostics.
- Basic frame-time/FPS display.

## Milestone 06 — Asset Pipeline
- Python environment and cooker skeleton.
- Source/cooked asset separation.
- Incremental cooking foundation.

## Later milestones
Animation, enemies, collectibles, level editor, audio, save system, profiling/optimization, Raspberry Pi validation, Android port, iOS feasibility/backend work.
