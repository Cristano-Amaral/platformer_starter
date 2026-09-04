#pragma once

// Tiny editor-only vector helpers. Not a math library and not gameplay API.
// Kept out of core::Vec3 so runtime code does not grow editor navigation math.

#include "core/Vec3.h"

#include <cmath>

namespace editor
{
inline constexpr float kPi = 3.14159265f;
inline constexpr float kDegreesToRadians = kPi / 180.0f;
inline constexpr float kRadiansToDegrees = 180.0f / kPi;

inline core::Vec3 Sub(core::Vec3 a, core::Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline core::Vec3 Scale(core::Vec3 value, float scalar)
{
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

inline float Dot(core::Vec3 a, core::Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline core::Vec3 Cross(core::Vec3 a, core::Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

inline float LengthSquared(core::Vec3 value)
{
    return Dot(value, value);
}

inline float Length(core::Vec3 value)
{
    return std::sqrt(LengthSquared(value));
}

inline core::Vec3 NormalizeOr(core::Vec3 value, core::Vec3 fallback)
{
    const float length = Length(value);
    if (!(length > 0.0f))
    {
        return fallback;
    }
    return Scale(value, 1.0f / length);
}

inline core::Vec3 RotateZ(core::Vec3 value, float degrees)
{
    const float radians = degrees * kDegreesToRadians;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {
        value.x * cosine - value.y * sine,
        value.x * sine + value.y * cosine,
        value.z};
}
}
