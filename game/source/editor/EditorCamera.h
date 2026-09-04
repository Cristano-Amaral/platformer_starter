#pragma once

// Session-only editor/navigation camera. Not authored, not serialized, and
// not PlatformerCamera. Mutating this must never write LevelDefinition.camera.

#include "core/Vec3.h"
#include "editor/EditorInput.h"
#include "render/CameraView.h"

namespace editor
{
struct EditorCamera
{
    core::Vec3 position{};
    // Yaw 0 / pitch 0 looks along -Z, matching a typical +Z gameplay offset.
    float yawDegrees = 0.0f;
    float pitchDegrees = 0.0f;
    float movementSpeed = 8.0f;
    float fieldOfViewY = 40.0f;
    // False until the first F2 activation seeds from the gameplay view.
    // Subsequent F2 toggles in the same process keep the pose.
    bool initialized = false;
};

inline constexpr float kEditorCameraMinPitchDegrees = -89.0f;
inline constexpr float kEditorCameraMaxPitchDegrees = 89.0f;
inline constexpr float kEditorCameraMinSpeed = 1.0f;
inline constexpr float kEditorCameraMaxSpeed = 40.0f;
inline constexpr float kEditorCameraLookDegreesPerPixel = 0.15f;
inline constexpr float kEditorCameraFastMultiplier = 2.0f;
inline constexpr float kEditorCameraWheelSpeedStep = 1.0f;

core::Vec3 EditorCameraForward(const EditorCamera& camera);
core::Vec3 EditorCameraRight(const EditorCamera& camera);
core::Vec3 EditorCameraTarget(const EditorCamera& camera);
render::CameraView MakeCameraView(const EditorCamera& camera);

void ClampEditorCamera(EditorCamera& camera);

// Seeds yaw/pitch/position/FOV from the current gameplay look
// (position = target + offset). Does nothing when already initialized, so
// F2 close/reopen keeps the last editor pose for this process.
void SeedEditorCameraFromGameplay(
    EditorCamera& camera,
    core::Vec3 gameplayTarget,
    core::Vec3 gameplayOffset,
    float fieldOfViewY);

// Session navigation. Does not write LevelDefinition.camera. Callers gate
// look/move/wheel using ImGui capture so typing and panel clicks stay inert.
void UpdateEditorCamera(
    EditorCamera& camera,
    const EditorInputState& input,
    float deltaSeconds,
    bool applyLook,
    bool applyMove,
    bool applyWheel);
}
