#pragma once

// Milestone 97: Terrain-specific tangent basis, weighted normal blending,
// and bounded roughness lighting. CPU-testable independently of GL.
// Not a general PBR or material framework.

#include "core/Vec3.h"

#include <cmath>

namespace world
{
inline constexpr float kDefaultTerrainRoughness = 0.72f;
inline constexpr float kMinTerrainRoughness = 0.04f;
inline constexpr float kMaxTerrainRoughness = 1.0f;
inline constexpr float kTerrainSpecularIntensity = 0.18f;
inline constexpr float kTerrainSpecularPowerMin = 8.0f;
inline constexpr float kTerrainSpecularPowerMax = 128.0f;
inline constexpr int kTerrainArrayMaxDimension = 512;

inline core::Vec3 TerrainFlatTangentNormal()
{
    return {0.0f, 0.0f, 1.0f};
}

inline unsigned char TerrainDefaultRoughnessQuantum()
{
    const int quantum = static_cast<int>(std::lround(kDefaultTerrainRoughness * 255.0f));
    if (quantum <= 0)
    {
        return 0;
    }
    if (quantum >= 255)
    {
        return 255;
    }
    return static_cast<unsigned char>(quantum);
}

inline core::Vec3 TerrainNormalizeOr(core::Vec3 value, core::Vec3 fallback)
{
    const float lenSq = value.x * value.x + value.y * value.y + value.z * value.z;
    if (!(lenSq > 1.0e-12f) || !std::isfinite(lenSq))
    {
        return fallback;
    }
    const float inv = 1.0f / std::sqrt(lenSq);
    return {value.x * inv, value.y * inv, value.z * inv};
}

inline core::Vec3 DecodeTerrainNormalMapRgb(float red, float green, float blue)
{
    return TerrainNormalizeOr(
        {red * 2.0f - 1.0f, green * 2.0f - 1.0f, blue * 2.0f - 1.0f},
        TerrainFlatTangentNormal());
}

inline float SanitizeTerrainRoughnessSample(float roughness)
{
    if (!std::isfinite(roughness))
    {
        return kDefaultTerrainRoughness;
    }
    if (roughness < kMinTerrainRoughness)
    {
        return kMinTerrainRoughness;
    }
    if (roughness > kMaxTerrainRoughness)
    {
        return kMaxTerrainRoughness;
    }
    return roughness;
}

struct TerrainTangentBasis
{
    core::Vec3 tangent{1.0f, 0.0f, 0.0f};
    core::Vec3 bitangent{0.0f, 0.0f, 1.0f};
    core::Vec3 normal{0.0f, 1.0f, 0.0f};
};

// Planar XZ UVs: U increases with +X, V increases with +Z. TBN is
// orthonormalized against the geometric normal so sculpted slopes keep a
// stable basis without NaNs. T follows U and B follows V.
inline TerrainTangentBasis BuildTerrainTangentBasis(core::Vec3 geometricNormal)
{
    TerrainTangentBasis basis{};
    basis.normal = TerrainNormalizeOr(geometricNormal, {0.0f, 1.0f, 0.0f});
    const float ndx = basis.normal.x;
    core::Vec3 tangent = {1.0f - basis.normal.x * ndx, -basis.normal.y * ndx, -basis.normal.z * ndx};
    if (tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z < 1.0e-8f)
    {
        const float ndz = basis.normal.z;
        tangent = {-basis.normal.x * ndz, -basis.normal.y * ndz, 1.0f - basis.normal.z * ndz};
    }
    basis.tangent = TerrainNormalizeOr(tangent, {1.0f, 0.0f, 0.0f});
    basis.bitangent = TerrainNormalizeOr(
        {basis.tangent.y * basis.normal.z - basis.tangent.z * basis.normal.y,
            basis.tangent.z * basis.normal.x - basis.tangent.x * basis.normal.z,
            basis.tangent.x * basis.normal.y - basis.tangent.y * basis.normal.x},
        {0.0f, 0.0f, 1.0f});
    return basis;
}

inline core::Vec3 TerrainTangentToWorld(const TerrainTangentBasis& basis, core::Vec3 tangentNormal)
{
    return TerrainNormalizeOr(
        {basis.tangent.x * tangentNormal.x + basis.bitangent.x * tangentNormal.y
                + basis.normal.x * tangentNormal.z,
            basis.tangent.y * tangentNormal.x + basis.bitangent.y * tangentNormal.y
                + basis.normal.y * tangentNormal.z,
            basis.tangent.z * tangentNormal.x + basis.bitangent.z * tangentNormal.y
                + basis.normal.z * tangentNormal.z},
        basis.normal);
}

// Weighted average of tangent-space normals, then normalize. Layers without a
// Normal map must pass the flat (0,0,1) vector so they do not perturb lighting.
inline core::Vec3 BlendTerrainTangentNormals(
    const float* weights,
    const core::Vec3* tangentNormals,
    int layerCount)
{
    if (weights == nullptr || tangentNormals == nullptr || layerCount <= 0)
    {
        return TerrainFlatTangentNormal();
    }
    core::Vec3 blended{0.0f, 0.0f, 0.0f};
    float weightSum = 0.0f;
    for (int layer = 0; layer < layerCount; ++layer)
    {
        float weight = weights[layer];
        if (!std::isfinite(weight) || weight <= 0.0f)
        {
            continue;
        }
        blended.x += tangentNormals[layer].x * weight;
        blended.y += tangentNormals[layer].y * weight;
        blended.z += tangentNormals[layer].z * weight;
        weightSum += weight;
    }
    if (!(weightSum > 1.0e-6f))
    {
        return TerrainFlatTangentNormal();
    }
    blended.x /= weightSum;
    blended.y /= weightSum;
    blended.z /= weightSum;
    return TerrainNormalizeOr(blended, TerrainFlatTangentNormal());
}

inline float BlendTerrainRoughness(const float* weights, const float* roughness, int layerCount)
{
    if (weights == nullptr || roughness == nullptr || layerCount <= 0)
    {
        return kDefaultTerrainRoughness;
    }
    float blended = 0.0f;
    float weightSum = 0.0f;
    for (int layer = 0; layer < layerCount; ++layer)
    {
        float weight = weights[layer];
        if (!std::isfinite(weight) || weight <= 0.0f)
        {
            continue;
        }
        blended += SanitizeTerrainRoughnessSample(roughness[layer]) * weight;
        weightSum += weight;
    }
    if (!(weightSum > 1.0e-6f))
    {
        return kDefaultTerrainRoughness;
    }
    return SanitizeTerrainRoughnessSample(blended / weightSum);
}

inline float TerrainGlossFromRoughness(float roughness)
{
    const float sanitized = SanitizeTerrainRoughnessSample(roughness);
    const float gloss = 1.0f - sanitized;
    return gloss < 0.0f ? 0.0f : gloss;
}

inline float TerrainSpecularPowerFromRoughness(float roughness)
{
    const float gloss = TerrainGlossFromRoughness(roughness);
    const float t = gloss * gloss;
    return kTerrainSpecularPowerMin + (kTerrainSpecularPowerMax - kTerrainSpecularPowerMin) * t;
}

inline float TerrainSpecularIntensityFromRoughness(float roughness)
{
    const float gloss = TerrainGlossFromRoughness(roughness);
    return kTerrainSpecularIntensity * gloss * gloss;
}

inline bool TerrainNormalBlendIsStable(core::Vec3 blended)
{
    return std::isfinite(blended.x) && std::isfinite(blended.y) && std::isfinite(blended.z)
        && std::fabs(blended.x * blended.x + blended.y * blended.y + blended.z * blended.z - 1.0f)
        <= 1.0e-4f;
}

inline unsigned char FlipDirectXNormalGreenQuantum(unsigned char green)
{
    return static_cast<unsigned char>(255 - static_cast<int>(green));
}
}
