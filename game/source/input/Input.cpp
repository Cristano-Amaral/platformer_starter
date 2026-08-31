#include "input/Input.h"

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

    return state;
}
}
