#pragma once

#include "core/Vec3.h"

namespace gameplay
{
class PlatformerCamera
{
public:
    // Framing comes from LevelDefinition via ApplyLevelFraming. Dead zone and
    // sharpness remain controller policy, not level authoring.
    core::Vec3 offset{};
    float fieldOfViewY = 0.0f;

    static constexpr float kHorizontalDeadZone = 1.5f;
    static constexpr float kVerticalDeadZone = 0.75f;
    // Higher = snappier follow. Units: 1/seconds. ~95% catch-up in 3/k seconds.
    static constexpr float kFollowSharpness = 8.0f;

    void ApplyLevelFraming(core::Vec3 framingOffset, float framingFieldOfViewY);
    void Initialize(core::Vec3 playerPosition);
    void SnapToTarget(core::Vec3 playerPosition);
    void Update(core::Vec3 playerPosition, float deltaSeconds);

    core::Vec3 Target() const;
    core::Vec3 DesiredTarget() const;

private:
    void UpdateDesiredTarget(core::Vec3 playerPosition);

    core::Vec3 desiredTarget{};
    core::Vec3 smoothedTarget{};
};
}
