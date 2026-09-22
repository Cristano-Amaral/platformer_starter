#pragma once

// Milestone 90/92: deterministic Terrain material-weight painting. Mutates
// dedicated weight-map texels, not heightfield samples. A stamp mixes the
// full palette, including layers stored in different packed RGBA maps.
// Reuses M87 linear falloff and spatial stamp spacing. Brush parameters,
// stroke state, and GPU synchronization live elsewhere. Not a command
// framework, eraser tool, or Undo stack.

#include "world/Terrain.h"
#include "world/TerrainSculpt.h"

#include <cmath>

namespace world
{
inline constexpr float kDefaultTerrainPaintRadius = kDefaultTerrainSculptRadius;
inline constexpr float kMinTerrainPaintRadius = kMinTerrainSculptRadius;
inline constexpr float kMaxTerrainPaintRadius = kMaxTerrainSculptRadius;
inline constexpr float kDefaultTerrainPaintStrength = kDefaultTerrainSculptStrength;
inline constexpr float kMinTerrainPaintStrength = kMinTerrainSculptStrength;
inline constexpr float kMaxTerrainPaintStrength = kMaxTerrainSculptStrength;
inline constexpr float kTerrainPaintStampSpacingFactor = kTerrainSculptStampSpacingFactor;

inline float SanitizeTerrainPaintRadius(float radius)
{
    return SanitizeTerrainSculptRadius(radius);
}

inline float SanitizeTerrainPaintStrength(float strength)
{
    return SanitizeTerrainSculptStrength(strength);
}

inline float TerrainPaintFalloff(float distanceXZ, float radius)
{
    return TerrainSculptFalloff(distanceXZ, radius);
}

inline float TerrainPaintStampSpacing(float radius)
{
    return SanitizeTerrainPaintRadius(radius) * kTerrainPaintStampSpacingFactor;
}

struct TerrainPaintStampRequest
{
    int layer = 0;
    float centerX = 0.0f;
    float centerZ = 0.0f;
    float radius = kDefaultTerrainPaintRadius;
    float strength = kDefaultTerrainPaintStrength;
};

// Mixes the full palette toward a one-hot target for `layer`:
//   next = current * (1 - influence) + target * influence
//   influence = clamp(strength * falloff, 0, 1)
// Then renormalizes across every active layer, including weights stored in
// other packed maps. Repeated stamps converge to layer L. A zero influence
// leaves weights unchanged.
inline void MixTerrainTexelWeightsTowardLayer(
    float* weights,
    int layer,
    int layerCount,
    float influence)
{
    if (weights == nullptr)
    {
        return;
    }
    float mix = influence;
    if (!std::isfinite(mix) || mix <= 0.0f)
    {
        return;
    }
    if (mix > 1.0f)
    {
        mix = 1.0f;
    }
    if (layer < 0 || layer >= layerCount || layer >= kMaxTerrainMaterialLayers)
    {
        return;
    }
    float target[kMaxTerrainMaterialLayers]{};
    target[layer] = 1.0f;
    for (int index = 0; index < kMaxTerrainMaterialLayers; ++index)
    {
        weights[index] = weights[index] * (1.0f - mix) + target[index] * mix;
    }
    NormalizeTerrainWeights(weights, layerCount);
}

inline bool ApplyTerrainPaintStamp(TerrainSpec& terrain, const TerrainPaintStampRequest& request)
{
    if (!TerrainSpecIsValid(terrain) || !terrain.enabled)
    {
        return false;
    }
    if (request.layer < 0 || request.layer >= TerrainMaterialLayerCount(terrain))
    {
        return false;
    }

    const float radius = SanitizeTerrainPaintRadius(request.radius);
    const float strength = SanitizeTerrainPaintStrength(request.strength);
    if (!(radius > 0.0f) || !(strength > 0.0f))
    {
        return false;
    }

    EnsureTerrainMaterialWeights(terrain);
    const int layerCount = TerrainMaterialLayerCount(terrain);
    bool changed = false;
    for (int iz = 0; iz < terrain.weightResolutionZ; ++iz)
    {
        for (int ix = 0; ix < terrain.weightResolutionX; ++ix)
        {
            const core::Vec3 texel = TerrainWeightTexelPosition(terrain, ix, iz);
            const float falloff = TerrainPaintFalloff(
                TerrainSculptDistanceXZ(texel.x, texel.z, request.centerX, request.centerZ),
                radius);
            if (!(falloff > 0.0f))
            {
                continue;
            }

            const int texelIndex = TerrainWeightTexelIndex(terrain, ix, iz);
            float current[kMaxTerrainMaterialLayers];
            ReadTerrainTexelWeights(terrain, texelIndex, current);
            float next[kMaxTerrainMaterialLayers];
            for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
            {
                next[layer] = current[layer];
            }
            MixTerrainTexelWeightsTowardLayer(next, request.layer, layerCount, strength * falloff);
            bool texelChanged = false;
            for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
            {
                if (next[layer] != current[layer])
                {
                    texelChanged = true;
                    break;
                }
            }
            if (texelChanged)
            {
                WriteTerrainTexelWeights(terrain, texelIndex, next);
                changed = true;
            }
        }
    }
    if (!changed)
    {
        CompactDefaultTerrainMaterialWeights(terrain);
    }
    return changed;
}

struct TerrainPaintStroke
{
    bool active = false;
    float lastStampX = 0.0f;
    float lastStampZ = 0.0f;
};

inline void EndTerrainPaintStroke(TerrainPaintStroke& stroke)
{
    stroke.active = false;
}

inline bool BeginTerrainPaintStroke(
    TerrainPaintStroke& stroke,
    TerrainSpec& terrain,
    TerrainPaintStampRequest request)
{
    stroke.active = true;
    stroke.lastStampX = request.centerX;
    stroke.lastStampZ = request.centerZ;
    return ApplyTerrainPaintStamp(terrain, request);
}

inline bool ContinueTerrainPaintStroke(
    TerrainPaintStroke& stroke,
    TerrainSpec& terrain,
    TerrainPaintStampRequest request,
    float cursorX,
    float cursorZ)
{
    if (!stroke.active)
    {
        return false;
    }

    const float spacing = TerrainPaintStampSpacing(request.radius);
    if (!(spacing > 0.0f))
    {
        return false;
    }

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
        changed = ApplyTerrainPaintStamp(terrain, request) || changed;
        dx = cursorX - stroke.lastStampX;
        dz = cursorZ - stroke.lastStampZ;
        remaining = std::sqrt(dx * dx + dz * dz);
    }
    return changed;
}
}
