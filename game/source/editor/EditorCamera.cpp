#include "editor/EditorCamera.h"

#include "editor/EditorMath.h"

#include <algorithm>
#include <cmath>

namespace editor
{
namespace
{
core::Vec3 ForwardFromYawPitch(float yawDegrees, float pitchDegrees)
{
    const float yaw = yawDegrees * kDegreesToRadians;
    const float pitch = pitchDegrees * kDegreesToRadians;
    const float cosinePitch = std::cos(pitch);
    return NormalizeOr(
        {cosinePitch * std::sin(yaw), std::sin(pitch), -cosinePitch * std::cos(yaw)},
        {0.0f, 0.0f, -1.0f});
}
}

core::Vec3 EditorCameraForward(const EditorCamera& camera)
{
    return ForwardFromYawPitch(camera.yawDegrees, camera.pitchDegrees);
}

core::Vec3 EditorCameraRight(const EditorCamera& camera)
{
    return NormalizeOr(
        Cross(EditorCameraForward(camera), {0.0f, 1.0f, 0.0f}), {1.0f, 0.0f, 0.0f});
}

core::Vec3 EditorCameraTarget(const EditorCamera& camera)
{
    return camera.position + EditorCameraForward(camera);
}

render::CameraView MakeCameraView(const EditorCamera& camera)
{
    render::CameraView view{};
    view.position = camera.position;
    view.target = EditorCameraTarget(camera);
    view.up = {0.0f, 1.0f, 0.0f};
    view.fieldOfViewY = camera.fieldOfViewY;
    return view;
}

void ClampEditorCamera(EditorCamera& camera)
{
    camera.pitchDegrees = std::clamp(
        camera.pitchDegrees, kEditorCameraMinPitchDegrees, kEditorCameraMaxPitchDegrees);
    camera.movementSpeed =
        std::clamp(camera.movementSpeed, kEditorCameraMinSpeed, kEditorCameraMaxSpeed);
    if (!(camera.fieldOfViewY > 0.0f) || camera.fieldOfViewY >= 180.0f)
    {
        camera.fieldOfViewY = 40.0f;
    }
}

void SeedEditorCameraFromGameplay(
    EditorCamera& camera,
    core::Vec3 gameplayTarget,
    core::Vec3 gameplayOffset,
    float fieldOfViewY)
{
    if (camera.initialized)
    {
        return;
    }

    camera.position = gameplayTarget + gameplayOffset;
    const core::Vec3 forward = NormalizeOr(Scale(gameplayOffset, -1.0f), {0.0f, 0.0f, -1.0f});
    camera.pitchDegrees = std::asin(std::clamp(forward.y, -1.0f, 1.0f)) * kRadiansToDegrees;
    camera.yawDegrees = std::atan2(forward.x, -forward.z) * kRadiansToDegrees;
    camera.fieldOfViewY = fieldOfViewY;
    camera.initialized = true;
    ClampEditorCamera(camera);
}

void UpdateEditorCamera(
    EditorCamera& camera,
    const EditorInputState& input,
    float deltaSeconds,
    bool applyLook,
    bool applyMove,
    bool applyWheel)
{
    if (applyLook && input.lookHeld)
    {
        camera.yawDegrees += input.mouseDeltaX * kEditorCameraLookDegreesPerPixel;
        camera.pitchDegrees -= input.mouseDeltaY * kEditorCameraLookDegreesPerPixel;
    }

    if (applyWheel)
    {
        camera.movementSpeed += input.wheelDelta * kEditorCameraWheelSpeedStep;
    }

    if (applyMove && deltaSeconds > 0.0f)
    {
        const float speed = camera.movementSpeed
            * (input.faster ? kEditorCameraFastMultiplier : 1.0f);
        core::Vec3 forward = EditorCameraForward(camera);
        forward.y = 0.0f;
        forward = NormalizeOr(forward, {0.0f, 0.0f, -1.0f});
        core::Vec3 right = EditorCameraRight(camera);
        right.y = 0.0f;
        right = NormalizeOr(right, {1.0f, 0.0f, 0.0f});
        camera.position = camera.position + Scale(forward, input.moveForward * speed * deltaSeconds);
        camera.position = camera.position + Scale(right, input.moveRight * speed * deltaSeconds);
        camera.position.y += input.moveUp * speed * deltaSeconds;
    }

    ClampEditorCamera(camera);
}
}
