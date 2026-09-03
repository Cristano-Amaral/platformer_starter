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
    // True only on the frame Restart was pressed, not while held.
    bool restartPressed = false;
    // True only on the frame the Development level editor toggle was pressed.
    // Consumers with no editor simply ignore it.
    bool toggleLevelEditorPressed = false;
};
}
