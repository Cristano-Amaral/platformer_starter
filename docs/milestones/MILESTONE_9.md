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
