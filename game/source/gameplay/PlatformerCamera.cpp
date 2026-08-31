#include "gameplay/PlatformerCamera.h"

#include <cmath>

namespace gameplay
{
namespace
{
float ExpSmooth(float current, float target, float deltaSeconds, float sharpness)
{
    if (deltaSeconds <= 0.0f)
    {
        return current;
    }

    const float t = 1.0f - std::exp(-sharpness * deltaSeconds);
    return current + (target - current) * t;
}
}

void PlatformerCamera::Initialize(core::Vec3 playerPosition)
{
    desiredTarget = playerPosition;
    smoothedTarget = playerPosition;
}

void PlatformerCamera::UpdateDesiredTarget(core::Vec3 playerPosition)
{
    if (playerPosition.x > desiredTarget.x + kHorizontalDeadZone)
    {
        desiredTarget.x = playerPosition.x - kHorizontalDeadZone;
    }
    else if (playerPosition.x < desiredTarget.x - kHorizontalDeadZone)
    {
        desiredTarget.x = playerPosition.x + kHorizontalDeadZone;
    }

    if (playerPosition.y > desiredTarget.y + kVerticalDeadZone)
    {
        desiredTarget.y = playerPosition.y - kVerticalDeadZone;
    }
    else if (playerPosition.y < desiredTarget.y - kVerticalDeadZone)
    {
        desiredTarget.y = playerPosition.y + kVerticalDeadZone;
    }

    desiredTarget.z = playerPosition.z;
}

void PlatformerCamera::Update(core::Vec3 playerPosition, float deltaSeconds)
{
    UpdateDesiredTarget(playerPosition);
    smoothedTarget.x = ExpSmooth(smoothedTarget.x, desiredTarget.x, deltaSeconds, kFollowSharpness);
    smoothedTarget.y = ExpSmooth(smoothedTarget.y, desiredTarget.y, deltaSeconds, kFollowSharpness);
    smoothedTarget.z = ExpSmooth(smoothedTarget.z, desiredTarget.z, deltaSeconds, kFollowSharpness);
}

core::Vec3 PlatformerCamera::Target() const
{
    return smoothedTarget;
}
}
