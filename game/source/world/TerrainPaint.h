#pragma once

// Milestone 90: deterministic Terrain material-weight painting. Mutates only
// TerrainSpec extra-layer palette consumption via materialWeights. Reuses M87
// linear falloff and spatial stamp spacing. Brush parameters, stroke state,
// and GPU synchronization live elsewhere. Not a command framework, eraser
// tool, or Undo stack.

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

// Mixes the local 4-weight mixture toward a one-hot target for `layer`:
//   next = current * (1 - influence) + target * influence
//   influence = clamp(strength * falloff, 0, 1)
// Then renormalizes. Repeated stamps converge to layer L. A zero influence
// leaves weights unchanged.
inline void MixTerrainSampleWeightsTowardLayer(float* weights, int layer, float influence)
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
    if (layer < 0 || layer >= kMaxTerrainMaterialLayers)
    {
        return;
    }
    float target[kMaxTerrainMaterialLayers]{};
    target[layer] = 1.0f;
    for (int index = 0; index < kMaxTerrainMaterialLayers; ++index)
    {
        weights[index] = weights[index] * (1.0f - mix) + target[index] * mix;
    }
    NormalizeTerrainSampleWeights(weights);
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
    bool changed = false;
    for (int iz = 0; iz < terrain.resolutionZ; ++iz)
    {
        for (int ix = 0; ix < terrain.resolutionX; ++ix)
        {
            const core::Vec3 sample = TerrainSamplePosition(terrain, ix, iz);
            const float falloff = TerrainPaintFalloff(
                TerrainSculptDistanceXZ(sample.x, sample.z, request.centerX, request.centerZ),
                radius);
            if (!(falloff > 0.0f))
            {
                continue;
            }

            const int sampleIndex = TerrainHeightIndex(terrain, ix, iz);
            float current[kMaxTerrainMaterialLayers];
            ReadTerrainSampleWeights(terrain, sampleIndex, current);
            float next[kMaxTerrainMaterialLayers];
            for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
            {
                next[layer] = current[layer];
            }
            MixTerrainSampleWeightsTowardLayer(next, request.layer, strength * falloff);
            bool sampleChanged = false;
            for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
            {
                if (next[layer] != current[layer])
                {
                    sampleChanged = true;
                    break;
                }
            }
            if (sampleChanged)
            {
                WriteTerrainSampleWeights(terrain, sampleIndex, next);
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
