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
    // True only on the frame Grab / Drop was pressed, not while held.
    bool grabDropPressed = false;
    // True only on the frame the player Inventory toggle was pressed.
    bool toggleInventoryPressed = false;
    // True only on the frame Inventory previous/next was pressed, not held.
    bool inventoryPreviousPressed = false;
    bool inventoryNextPressed = false;
    // True only on the frame Cancel / Esc was pressed. Gameplay uses this to
    // close Inventory before the window-exit key can fire.
    bool cancelPressed = false;
    // True only on the frame the Development level editor toggle was pressed.
    // Consumers with no editor simply ignore it.
    bool toggleLevelEditorPressed = false;
};
}
