#pragma once

// Authored Dynamic Box (Milestone 45). Box-shaped dynamic rigid body only.
// Jolt types stay out of this header. Runtime recovery (Milestone 46) restores
// the live body to this authored center with identity orientation; it does not
// add an authored orientation field.

#include "core/Vec3.h"

#include <cmath>

namespace world
{
// Matches editor::kMinAuthoredBoxExtent. Parser and physics share this so a
// Dynamic Box never becomes a zero-volume Jolt shape.
inline constexpr float kMinDynamicBoxExtent = 0.12f;
inline constexpr core::Vec3 kDefaultDynamicBoxSize{1.0f, 1.0f, 1.0f};
inline constexpr float kDefaultDynamicBoxMassKg = 30.0f;
// Safety cap for editor/parser usability and simulation stability. Not a
// gameplay-design quota. CharacterVirtual remains 70 kg / 100 N.
inline constexpr float kMaxDynamicBoxMassKg = 10000.0f;

struct DynamicBoxSpec
{
    core::Vec3 center{};
    core::Vec3 size{};
    float massKg = 0.0f;
};

inline bool DynamicBoxMassIsValid(float massKg)
{
    return std::isfinite(massKg) && massKg > 0.0f && massKg <= kMaxDynamicBoxMassKg;
}

inline bool DynamicBoxSizeIsValid(core::Vec3 size)
{
    return std::isfinite(size.x) && std::isfinite(size.y) && std::isfinite(size.z)
        && size.x >= kMinDynamicBoxExtent && size.y >= kMinDynamicBoxExtent
        && size.z >= kMinDynamicBoxExtent;
}

inline bool DynamicBoxCenterIsValid(core::Vec3 center)
{
    return std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z);
}

inline bool DynamicBoxSpecIsValid(const DynamicBoxSpec& spec)
{
    return DynamicBoxCenterIsValid(spec.center) && DynamicBoxSizeIsValid(spec.size)
        && DynamicBoxMassIsValid(spec.massKg);
}
}
