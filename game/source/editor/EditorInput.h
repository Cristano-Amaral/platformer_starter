#pragma once

// Editor-only mouse/keyboard snapshot. Separate from input::InputState so
// gameplay semantic actions (MoveLeft, Jump, Respawn) stay unpolluted.
//
// Filled by editor::PollEditorInput() in the input backend. Application
// consumes this only while the level editor is active.

namespace editor
{
struct EditorInputState
{
    // Camera-relative axes: -1 / 0 / +1. Forward is look direction on XZ,
    // right is camera right, up is world +Y (Q/E).
    float moveForward = 0.0f;
    float moveRight = 0.0f;
    float moveUp = 0.0f;
    bool faster = false;

    bool lookHeld = false;
    bool selectPressed = false;
    bool selectHeld = false;
    bool selectReleased = false;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    float wheelDelta = 0.0f;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
};

EditorInputState PollEditorInput();
}
