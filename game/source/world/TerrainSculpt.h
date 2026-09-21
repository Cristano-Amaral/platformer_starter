#pragma once

// Milestone 87: deterministic Terrain height sculpt math. Mutates only
// TerrainSpec::heights. Brush parameters, stroke state, and GPU/Jolt
// synchronization live elsewhere. Not a command framework or Undo stack.

#include "world/Terrain.h"

#include <cmath>
#include <cstddef>
#include <vector>

namespace world
{
enum class TerrainSculptOperation
{
    Raise,
    Lower,
    Smooth,
    Flatten,
};

inline constexpr float kDefaultTerrainSculptRadius = 2.0f;
inline constexpr float kMinTerrainSculptRadius = 0.25f;
inline constexpr float kMaxTerrainSculptRadius = 64.0f;
inline constexpr float kDefaultTerrainSculptStrength = 0.25f;
inline constexpr float kMinTerrainSculptStrength = 0.01f;
inline constexpr float kMaxTerrainSculptStrength = 1.0f;
// Stamp spacing is a fraction of the current brush radius so travel, not
// frame count, produces additional dabs. A stationary cursor never retriggers.
inline constexpr float kTerrainSculptStampSpacingFactor = 0.25f;

inline float SanitizeTerrainSculptRadius(float radius)
{
    if (!std::isfinite(radius))
    {
        return kDefaultTerrainSculptRadius;
    }
    if (radius < kMinTerrainSculptRadius)
    {
        return kMinTerrainSculptRadius;
    }
    if (radius > kMaxTerrainSculptRadius)
    {
        return kMaxTerrainSculptRadius;
    }
    return radius;
}

inline float SanitizeTerrainSculptStrength(float strength)
{
    if (!std::isfinite(strength))
    {
        return kDefaultTerrainSculptStrength;
    }
    if (strength < kMinTerrainSculptStrength)
    {
        return kMinTerrainSculptStrength;
    }
    if (strength > kMaxTerrainSculptStrength)
    {
        return kMaxTerrainSculptStrength;
    }
    return strength;
}

inline float ClampAuthoredTerrainHeight(float height)
{
    if (!std::isfinite(height))
    {
        return 0.0f;
    }
    if (height > kMaxTerrainHeightAbs)
    {
        return kMaxTerrainHeightAbs;
    }
    if (height < -kMaxTerrainHeightAbs)
    {
        return -kMaxTerrainHeightAbs;
    }
    return height;
}

// Linear XZ falloff. weight = 1 - (d / radius) for d < radius, else 0.
// Samples at or beyond the radius are unchanged.
inline float TerrainSculptFalloff(float distanceXZ, float radius)
{
    if (!std::isfinite(distanceXZ) || !std::isfinite(radius) || !(radius > 0.0f)
        || !(distanceXZ < radius))
    {
        return 0.0f;
    }
    return 1.0f - (distanceXZ / radius);
}

inline float TerrainSculptDistanceXZ(float ax, float az, float bx, float bz)
{
    const float dx = ax - bx;
    const float dz = az - bz;
    return std::sqrt(dx * dx + dz * dz);
}

inline float TerrainSculptStampSpacing(float radius)
{
    return SanitizeTerrainSculptRadius(radius) * kTerrainSculptStampSpacingFactor;
}

inline float TerrainSculptSmoothTarget(
    const std::vector<float>& snapshot,
    const TerrainSpec& terrain,
    int ix,
    int iz)
{
    float sum = 0.0f;
    int count = 0;
    for (int jz = iz - 1; jz <= iz + 1; ++jz)
    {
        if (jz < 0 || jz >= terrain.resolutionZ)
        {
            continue;
        }
        for (int jx = ix - 1; jx <= ix + 1; ++jx)
        {
            if (jx < 0 || jx >= terrain.resolutionX)
            {
                continue;
            }
            const int index = TerrainHeightIndex(terrain, jx, jz);
            if (index < 0 || index >= static_cast<int>(snapshot.size()))
            {
                continue;
            }
            sum += snapshot[static_cast<std::size_t>(index)];
            ++count;
        }
    }
    if (count <= 0)
    {
        const int index = TerrainHeightIndex(terrain, ix, iz);
        if (index >= 0 && index < static_cast<int>(snapshot.size()))
        {
            return snapshot[static_cast<std::size_t>(index)];
        }
        return 0.0f;
    }
    return sum / static_cast<float>(count);
}

struct TerrainSculptStampRequest
{
    TerrainSculptOperation operation = TerrainSculptOperation::Raise;
    float centerX = 0.0f;
    float centerZ = 0.0f;
    float radius = kDefaultTerrainSculptRadius;
    float strength = kDefaultTerrainSculptStrength;
    // World-space Y captured at Flatten stroke begin. Converted to an
    // origin-relative authored height per stamp. Ignored by other operations.
    float flattenWorldY = 0.0f;
};

inline float TerrainSculptBlendFactor(float strength, float falloff)
{
    float blend = strength * falloff;
    if (!std::isfinite(blend) || blend < 0.0f)
    {
        return 0.0f;
    }
    if (blend > 1.0f)
    {
        return 1.0f;
    }
    return blend;
}

// Applies one dab. Smooth reads a pre-stamp height snapshot so mutation order
// cannot change the result. Returns true only when at least one sample changes.
inline bool ApplyTerrainSculptStamp(TerrainSpec& terrain, const TerrainSculptStampRequest& request)
{
    if (!TerrainSpecIsValid(terrain) || !terrain.enabled)
    {
        return false;
    }

    const float radius = SanitizeTerrainSculptRadius(request.radius);
    const float strength = SanitizeTerrainSculptStrength(request.strength);
    if (!(radius > 0.0f) || !(strength > 0.0f))
    {
        return false;
    }

    std::vector<float> snapshot;
    if (request.operation == TerrainSculptOperation::Smooth)
    {
        snapshot = terrain.heights;
    }

    const float flattenAuthored = ClampAuthoredTerrainHeight(request.flattenWorldY - terrain.origin.y);
    bool changed = false;
    for (int iz = 0; iz < terrain.resolutionZ; ++iz)
    {
        for (int ix = 0; ix < terrain.resolutionX; ++ix)
        {
            const core::Vec3 sample = TerrainSamplePosition(terrain, ix, iz);
            const float weight = TerrainSculptFalloff(
                TerrainSculptDistanceXZ(sample.x, sample.z, request.centerX, request.centerZ),
                radius);
            if (!(weight > 0.0f))
            {
                continue;
            }

            const int index = TerrainHeightIndex(terrain, ix, iz);
            const float current = terrain.heights[static_cast<std::size_t>(index)];
            float next = current;
            switch (request.operation)
            {
            case TerrainSculptOperation::Raise:
                next = current + strength * weight;
                break;
            case TerrainSculptOperation::Lower:
                next = current - strength * weight;
                break;
            case TerrainSculptOperation::Smooth:
            {
                const float target = TerrainSculptSmoothTarget(snapshot, terrain, ix, iz);
                next = current + (target - current) * TerrainSculptBlendFactor(strength, weight);
                break;
            }
            case TerrainSculptOperation::Flatten:
                next = current
                    + (flattenAuthored - current) * TerrainSculptBlendFactor(strength, weight);
                break;
            }
            next = ClampAuthoredTerrainHeight(next);
            if (next != current)
            {
                terrain.heights[static_cast<std::size_t>(index)] = next;
                changed = true;
            }
        }
    }
    return changed;
}

struct TerrainSculptStroke
{
    bool active = false;
    float lastStampX = 0.0f;
    float lastStampZ = 0.0f;
    float flattenWorldY = 0.0f;
};

inline void EndTerrainSculptStroke(TerrainSculptStroke& stroke)
{
    stroke.active = false;
}

inline bool BeginTerrainSculptStroke(
    TerrainSculptStroke& stroke,
    TerrainSpec& terrain,
    TerrainSculptStampRequest request,
    float flattenWorldY)
{
    stroke.active = true;
    stroke.lastStampX = request.centerX;
    stroke.lastStampZ = request.centerZ;
    stroke.flattenWorldY = flattenWorldY;
    request.flattenWorldY = flattenWorldY;
    return ApplyTerrainSculptStamp(terrain, request);
}

// Additional dabs are placed every StampSpacing(radius) of XZ travel from the
// last stamp. Holding a stationary cursor yields remaining == 0, so no extra
// deformation accumulates from extra rendered frames.
inline bool ContinueTerrainSculptStroke(
    TerrainSculptStroke& stroke,
    TerrainSpec& terrain,
    TerrainSculptStampRequest request,
    float cursorX,
    float cursorZ)
{
    if (!stroke.active)
    {
        return false;
    }

    const float spacing = TerrainSculptStampSpacing(request.radius);
    if (!(spacing > 0.0f))
    {
        return false;
    }

    request.flattenWorldY = stroke.flattenWorldY;
    bool changed = false;
    float dx = cursorX - stroke.lastStampX;
    float dz = cursorZ - stroke.lastStampZ;
    float remaining = std::sqrt(dx * dx + dz * dz);
    while (remaining >= spacing)
    {
        const float step = spacing / remaining;
        stroke.lastStampX += dx * step;
        stroke.lastStampZ += dz * step;
        request.centerX = stroke.lastStampX;
        request.centerZ = stroke.lastStampZ;
        changed = ApplyTerrainSculptStamp(terrain, request) || changed;
        dx = cursorX - stroke.lastStampX;
        dz = cursorZ - stroke.lastStampZ;
        remaining = std::sqrt(dx * dx + dz * dz);
    }
    return changed;
}
}
