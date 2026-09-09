#pragma once

// Authored Static Prop instance (Milestone 49). Visual only: no Jolt body.
// References a reusable Static Model Asset by canonical identity
// models/<file>.glb. Not a Transform component framework.

#include "core/Vec3.h"

#include <cmath>
#include <string>
#include <string_view>

namespace world
{
struct LevelDefinition;
inline constexpr core::Vec3 kDefaultStaticPropScale{1.0f, 1.0f, 1.0f};
inline constexpr core::Vec3 kDefaultStaticPropRotationDegrees{0.0f, 0.0f, 0.0f};

struct StaticPropSpec
{
    std::string modelIdentity;
    core::Vec3 position{};
    core::Vec3 rotationDegrees{};
    core::Vec3 scale = kDefaultStaticPropScale;
};

inline bool StaticPropPositionIsValid(core::Vec3 position)
{
    return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z);
}

inline bool StaticPropRotationIsValid(core::Vec3 rotationDegrees)
{
    return std::isfinite(rotationDegrees.x) && std::isfinite(rotationDegrees.y)
        && std::isfinite(rotationDegrees.z);
}

inline bool StaticPropScaleIsValid(core::Vec3 scale)
{
    return std::isfinite(scale.x) && std::isfinite(scale.y) && std::isfinite(scale.z)
        && scale.x > 0.0f && scale.y > 0.0f && scale.z > 0.0f;
}

// Interactive Scale-gizmo floor. Apply/parse still accept any finite axis > 0.
inline constexpr float kMinStaticPropScale = 0.01f;

inline float ClampStaticPropScaleAxis(float value)
{
    if (!std::isfinite(value) || value < kMinStaticPropScale)
    {
        return kMinStaticPropScale;
    }
    return value;
}

inline core::Vec3 ClampStaticPropScale(core::Vec3 scale)
{
    return {
        ClampStaticPropScaleAxis(scale.x),
        ClampStaticPropScaleAxis(scale.y),
        ClampStaticPropScaleAxis(scale.z)};
}

inline bool StaticPropTransformIsValid(const StaticPropSpec& spec)
{
    return StaticPropPositionIsValid(spec.position) && StaticPropRotationIsValid(spec.rotationDegrees)
        && StaticPropScaleIsValid(spec.scale);
}

bool StaticPropIdentityIsValid(std::string_view identity);
bool StaticPropSpecIsValid(const StaticPropSpec& spec);

bool LevelReferencesStaticPropIdentity(
    const LevelDefinition& level,
    std::string_view identity);
}
