#include "input/Input.h"
#include "editor/EditorInput.h"

#include "raylib.h"

namespace input
{
InputState Poll()
{
    InputState state;

    const bool moveLeft = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    const bool moveRight = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    if (moveLeft)
    {
        state.moveX -= 1.0f;
    }
    if (moveRight)
    {
        state.moveX += 1.0f;
    }

    state.jumpPressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP);
    state.respawnPressed = IsKeyPressed(KEY_R);
    state.restartPressed = IsKeyPressed(KEY_ENTER);
    state.toggleLevelEditorPressed = IsKeyPressed(KEY_F2);
    return state;
}

void SetMouseLookActive(bool active)
{
    static bool captured = false;
    if (active == captured)
    {
        return;
    }

    captured = active;
    if (active)
    {
        DisableCursor();
    }
    else
    {
        EnableCursor();
    }
}
}

namespace editor
{
EditorInputState PollEditorInput()
{
    EditorInputState state;

    if (IsKeyDown(KEY_W))
    {
        state.moveForward += 1.0f;
    }
    if (IsKeyDown(KEY_S))
    {
        state.moveForward -= 1.0f;
    }
    if (IsKeyDown(KEY_D))
    {
        state.moveRight += 1.0f;
    }
    if (IsKeyDown(KEY_A))
    {
        state.moveRight -= 1.0f;
    }
    if (IsKeyDown(KEY_E))
    {
        state.moveUp += 1.0f;
    }
    if (IsKeyDown(KEY_Q))
    {
        state.moveUp -= 1.0f;
    }
    state.faster = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    state.lookHeld = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    state.selectPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    state.selectHeld = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    state.selectReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    state.mouseDeltaX = GetMouseDelta().x;
    state.mouseDeltaY = GetMouseDelta().y;
    state.wheelDelta = GetMouseWheelMove();
    const Vector2 mouse = GetMousePosition();
    state.mouseX = mouse.x;
    state.mouseY = mouse.y;

    state.ctrlHeld = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    state.altHeld = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
    state.nudgePrecision = state.ctrlHeld && state.faster;
    if (state.ctrlHeld)
    {
        const auto pressed = [](int key) {
            return IsKeyPressed(key) || IsKeyPressedRepeat(key);
        };
        if (pressed(KEY_RIGHT))
        {
            state.nudgeX += 1;
        }
        if (pressed(KEY_LEFT))
        {
            state.nudgeX -= 1;
        }
        if (pressed(KEY_PAGE_UP))
        {
            state.nudgeY += 1;
        }
        if (pressed(KEY_PAGE_DOWN))
        {
            state.nudgeY -= 1;
        }
        if (pressed(KEY_UP))
        {
            state.nudgeZ += 1;
        }
        if (pressed(KEY_DOWN))
        {
            state.nudgeZ -= 1;
        }
    }
    if (state.altHeld)
    {
        state.dollyWheelDelta = state.wheelDelta;
    }
    return state;
}
}
