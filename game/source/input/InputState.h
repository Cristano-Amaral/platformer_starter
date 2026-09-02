#pragma once

namespace input
{
struct InputState
{
    // -1 left, 0 idle, +1 right. Independent of which physical keys were held.
    float moveX = 0.0f;
    // True only on the frame Jump was pressed, not while held.
    bool jumpPressed = false;
    // True only on the frame Respawn was pressed, not while held.
    bool respawnPressed = false;
};
}
