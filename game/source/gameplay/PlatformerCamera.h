#pragma once

#include "core/Vec3.h"

namespace gameplay
{
class PlatformerCamera
{
public:
    // Tunable framing: mostly side-on (large +Z), slightly off-axis (+X).
    core::Vec3 offset{2.0f, 3.5f, 12.0f};
    float fieldOfViewY = 40.0f;

    static constexpr float kHorizontalDeadZone = 1.5f;
    static constexpr float kVerticalDeadZone = 0.75f;
    // Higher = snappier follow. Units: 1/seconds. ~95% catch-up in 3/k seconds.
    static constexpr float kFollowSharpness = 8.0f;

    void Initialize(core::Vec3 playerPosition);
    void Update(core::Vec3 playerPosition, float deltaSeconds);

    core::Vec3 Target() const;

private:
    void UpdateDesiredTarget(core::Vec3 playerPosition);

    core::Vec3 desiredTarget{};
    core::Vec3 smoothedTarget{};
};
}
