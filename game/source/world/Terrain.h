#pragma once

// Milestone 86/87/88/90: optional singleton authored Terrain. Regular XZ
// heightfield. Not a repeatable prop category, tile set, GUID, or generic
// mesh editor. M87 sculpts these authored heights[] only; brush parameters
// are not Level data. M88 adds one optional base surface texture identity
// plus planar XZ tiling. M90 adds up to three extra texture layers and
// per-sample painted weights. Not a generic Material asset, splat texture,
// or PBR authoring.
//
// Origin convention (used by parse/write, render, Jolt, normals, picking,
// Inspector, and Translate):
//   sample(ix, iz) world position =
//     (origin.x + ix * sizeX / (resolutionX - 1),
//      origin.y + heights[iz * resolutionX + ix],
//      origin.z + iz * sizeZ / (resolutionZ - 1))
// sample(0,0) is the min-X / min-Z corner. Authored heights are relative to
// origin.y. World Y is up.
//
// Texture mapping (M88/M90) is planar on the regular XZ grid and independent
// of sample heights. Per layer L:
//   u = (worldX - origin.x) * layerTiling[L]
//   v = (worldZ - origin.z) * layerTiling[L]
// textureTiling / extraLayers[].textureTiling are repeats per world unit.
// Empty layer-0 textureIdentity is the solid-color fallback. Identities are
// textures/<file>.png; never an absolute path. Painted weights stay tied to
// the XZ sample topology; changing heights does not move them.

#include "assets/RuntimePng.h"
#include "core/Vec3.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace world
{
inline constexpr float kMinTerrainSize = 0.12f;
inline constexpr float kMaxTerrainSize = 256.0f;
inline constexpr float kMaxTerrainOriginAbs = 1024.0f;
inline constexpr float kMaxTerrainHeightAbs = 256.0f;
inline constexpr int kMinTerrainResolution = 2;
inline constexpr int kMaxTerrainResolution = 17;

inline constexpr bool kDefaultTerrainEnabled = true;
inline constexpr core::Vec3 kDefaultTerrainOrigin{-8.0f, 0.25f, -4.0f};
inline constexpr float kDefaultTerrainSizeX = 16.0f;
inline constexpr float kDefaultTerrainSizeZ = 8.0f;
inline constexpr int kDefaultTerrainResolutionX = 9;
inline constexpr int kDefaultTerrainResolutionZ = 5;
inline constexpr float kDefaultTerrainTextureTiling = 0.25f;
inline constexpr float kMinTerrainTextureTiling = 0.01f;
inline constexpr float kMaxTerrainTextureTiling = 16.0f;
inline constexpr int kMaxTerrainMaterialLayers = 4;
inline constexpr int kTerrainMaterialWeightQuantum = 255;

struct TerrainMaterialLayer
{
    std::string textureIdentity{};
    float textureTiling = kDefaultTerrainTextureTiling;
};

struct TerrainSpec
{
    bool enabled = kDefaultTerrainEnabled;
    core::Vec3 origin = kDefaultTerrainOrigin;
    float sizeX = kDefaultTerrainSizeX;
    float sizeZ = kDefaultTerrainSizeZ;
    int resolutionX = kDefaultTerrainResolutionX;
    int resolutionZ = kDefaultTerrainResolutionZ;
    std::vector<float> heights{};
    // Layer 0 (M88). Extra layers are M90 palette entries 1..3.
    std::string textureIdentity{};
    float textureTiling = kDefaultTerrainTextureTiling;
    std::vector<TerrainMaterialLayer> extraLayers{};
    // Empty means implicit defaults: layer 0 = 1, others = 0 per sample.
    // Non-empty size is sampleCount * kMaxTerrainMaterialLayers.
    std::vector<float> materialWeights{};
};

inline int TerrainSampleCount(int resolutionX, int resolutionZ)
{
    if (resolutionX < kMinTerrainResolution || resolutionZ < kMinTerrainResolution)
    {
        return 0;
    }
    return resolutionX * resolutionZ;
}

inline int TerrainSampleCount(const TerrainSpec& terrain)
{
    return TerrainSampleCount(terrain.resolutionX, terrain.resolutionZ);
}

inline bool TerrainTextureTilingIsValid(float tiling)
{
    return std::isfinite(tiling) && tiling >= kMinTerrainTextureTiling
        && tiling <= kMaxTerrainTextureTiling;
}

inline bool TerrainTextureIdentityIsNone(std::string_view identity)
{
    return identity.empty();
}

inline bool TerrainTextureIdentityIsValid(std::string_view identity)
{
    if (TerrainTextureIdentityIsNone(identity))
    {
        return true;
    }
    return assets::RuntimePngIdentityIsValid(identity);
}

inline int TerrainMaterialLayerCount(const TerrainSpec& terrain)
{
    const int extra = static_cast<int>(terrain.extraLayers.size());
    if (extra < 0)
    {
        return 1;
    }
    if (extra > kMaxTerrainMaterialLayers - 1)
    {
        return kMaxTerrainMaterialLayers;
    }
    return 1 + extra;
}

inline int TerrainMaterialWeightIndex(int sampleIndex, int layer)
{
    return sampleIndex * kMaxTerrainMaterialLayers + layer;
}

inline bool TerrainExtraLayersAreValid(const TerrainSpec& terrain)
{
    if (static_cast<int>(terrain.extraLayers.size()) > kMaxTerrainMaterialLayers - 1)
    {
        return false;
    }
    for (const TerrainMaterialLayer& layer : terrain.extraLayers)
    {
        if (TerrainTextureIdentityIsNone(layer.textureIdentity)
            || !assets::RuntimePngIdentityIsValid(layer.textureIdentity)
            || !TerrainTextureTilingIsValid(layer.textureTiling))
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < terrain.extraLayers.size(); ++index)
    {
        if (!TerrainTextureIdentityIsNone(terrain.textureIdentity)
            && terrain.extraLayers[index].textureIdentity == terrain.textureIdentity)
        {
            return false;
        }
        for (std::size_t other = index + 1; other < terrain.extraLayers.size(); ++other)
        {
            if (terrain.extraLayers[index].textureIdentity
                == terrain.extraLayers[other].textureIdentity)
            {
                return false;
            }
        }
    }
    return true;
}

inline float TerrainSampleLayerWeight(const TerrainSpec& terrain, int sampleIndex, int layer)
{
    if (layer < 0 || layer >= kMaxTerrainMaterialLayers || sampleIndex < 0)
    {
        return 0.0f;
    }
    if (terrain.materialWeights.empty())
    {
        return layer == 0 ? 1.0f : 0.0f;
    }
    const int index = TerrainMaterialWeightIndex(sampleIndex, layer);
    if (index < 0 || index >= static_cast<int>(terrain.materialWeights.size()))
    {
        return layer == 0 ? 1.0f : 0.0f;
    }
    return terrain.materialWeights[static_cast<std::size_t>(index)];
}

inline bool TerrainMaterialWeightsAreDefault(const TerrainSpec& terrain)
{
    if (terrain.materialWeights.empty())
    {
        return true;
    }
    const int samples = TerrainSampleCount(terrain);
    if (static_cast<int>(terrain.materialWeights.size()) != samples * kMaxTerrainMaterialLayers)
    {
        return false;
    }
    for (int sample = 0; sample < samples; ++sample)
    {
        if (TerrainSampleLayerWeight(terrain, sample, 0) != 1.0f)
        {
            return false;
        }
        for (int layer = 1; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            if (TerrainSampleLayerWeight(terrain, sample, layer) != 0.0f)
            {
                return false;
            }
        }
    }
    return true;
}

inline bool TerrainPaintRecordsShouldWrite(const TerrainSpec& terrain)
{
    return !TerrainMaterialWeightsAreDefault(terrain);
}

inline bool TerrainMaterialRecordShouldWrite(const TerrainSpec& terrain)
{
    return !TerrainTextureIdentityIsNone(terrain.textureIdentity)
        || terrain.textureTiling != kDefaultTerrainTextureTiling;
}

inline int TerrainRecordLineCount(const TerrainSpec& terrain)
{
    if (terrain.resolutionZ < kMinTerrainResolution)
    {
        return 0;
    }
    return 1 + terrain.resolutionZ
        + (TerrainMaterialRecordShouldWrite(terrain) ? 1 : 0)
        + static_cast<int>(terrain.extraLayers.size())
        + (TerrainPaintRecordsShouldWrite(terrain) ? terrain.resolutionZ : 0);
}

inline bool TerrainComponentFinite(float value)
{
    return std::isfinite(value);
}

inline bool TerrainVecFinite(core::Vec3 value)
{
    return TerrainComponentFinite(value.x) && TerrainComponentFinite(value.y)
        && TerrainComponentFinite(value.z);
}

inline bool TerrainOriginIsValid(core::Vec3 origin)
{
    if (!TerrainVecFinite(origin))
    {
        return false;
    }
    return std::fabs(origin.x) <= kMaxTerrainOriginAbs
        && std::fabs(origin.y) <= kMaxTerrainOriginAbs
        && std::fabs(origin.z) <= kMaxTerrainOriginAbs;
}

inline bool TerrainSizeIsValid(float size)
{
    return TerrainComponentFinite(size) && size >= kMinTerrainSize && size <= kMaxTerrainSize;
}

inline bool TerrainResolutionIsValid(int resolution)
{
    return resolution >= kMinTerrainResolution && resolution <= kMaxTerrainResolution;
}

inline bool TerrainHeightIsValid(float height)
{
    return TerrainComponentFinite(height) && std::fabs(height) <= kMaxTerrainHeightAbs;
}

inline void ResizeTerrainHeights(TerrainSpec& terrain)
{
    const int count = TerrainSampleCount(terrain);
    terrain.heights.assign(static_cast<std::size_t>(count), 0.0f);
}

inline TerrainSpec MakeDefaultTerrain()
{
    TerrainSpec terrain{};
    terrain.enabled = kDefaultTerrainEnabled;
    terrain.origin = kDefaultTerrainOrigin;
    terrain.sizeX = kDefaultTerrainSizeX;
    terrain.sizeZ = kDefaultTerrainSizeZ;
    terrain.resolutionX = kDefaultTerrainResolutionX;
    terrain.resolutionZ = kDefaultTerrainResolutionZ;
    ResizeTerrainHeights(terrain);
    return terrain;
}

inline bool TerrainSpecIsValid(const TerrainSpec& terrain)
{
    if (!TerrainOriginIsValid(terrain.origin) || !TerrainSizeIsValid(terrain.sizeX)
        || !TerrainSizeIsValid(terrain.sizeZ) || !TerrainResolutionIsValid(terrain.resolutionX)
        || !TerrainResolutionIsValid(terrain.resolutionZ))
    {
        return false;
    }
    if (static_cast<int>(terrain.heights.size()) != TerrainSampleCount(terrain))
    {
        return false;
    }
    for (float height : terrain.heights)
    {
        if (!TerrainHeightIsValid(height))
        {
            return false;
        }
    }
    if (!TerrainTextureIdentityIsValid(terrain.textureIdentity)
        || !TerrainTextureTilingIsValid(terrain.textureTiling)
        || !TerrainExtraLayersAreValid(terrain))
    {
        return false;
    }
    if (!terrain.materialWeights.empty())
    {
        const int samples = TerrainSampleCount(terrain);
        if (static_cast<int>(terrain.materialWeights.size())
            != samples * kMaxTerrainMaterialLayers)
        {
            return false;
        }
        for (int sample = 0; sample < samples; ++sample)
        {
            float sum = 0.0f;
            const int assigned = TerrainMaterialLayerCount(terrain);
            for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
            {
                const float weight = TerrainSampleLayerWeight(terrain, sample, layer);
                if (!TerrainComponentFinite(weight) || weight < 0.0f || weight > 1.0f)
                {
                    return false;
                }
                if (layer >= assigned && weight != 0.0f)
                {
                    return false;
                }
                sum += weight;
            }
            if (!TerrainComponentFinite(sum) || sum < 0.999f || sum > 1.001f)
            {
                return false;
            }
        }
    }
    return true;
}

inline bool TerrainHeightsEqual(const TerrainSpec& a, const TerrainSpec& b)
{
    if (a.heights.size() != b.heights.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < a.heights.size(); ++index)
    {
        if (a.heights[index] != b.heights[index])
        {
            return false;
        }
    }
    return true;
}

inline bool TerrainMaterialWeightsEqual(const TerrainSpec& a, const TerrainSpec& b)
{
    const int samples = TerrainSampleCount(a);
    if (TerrainSampleCount(b) != samples)
    {
        return false;
    }
    if (TerrainMaterialWeightsAreDefault(a) && TerrainMaterialWeightsAreDefault(b))
    {
        return true;
    }
    for (int sample = 0; sample < samples; ++sample)
    {
        for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            if (TerrainSampleLayerWeight(a, sample, layer)
                != TerrainSampleLayerWeight(b, sample, layer))
            {
                return false;
            }
        }
    }
    return true;
}

inline bool TerrainExtraLayersEqual(const TerrainSpec& a, const TerrainSpec& b)
{
    if (a.extraLayers.size() != b.extraLayers.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < a.extraLayers.size(); ++index)
    {
        if (a.extraLayers[index].textureIdentity != b.extraLayers[index].textureIdentity
            || a.extraLayers[index].textureTiling != b.extraLayers[index].textureTiling)
        {
            return false;
        }
    }
    return true;
}

inline bool TerrainMeshDataEqual(const TerrainSpec& a, const TerrainSpec& b)
{
    return a.origin.x == b.origin.x && a.origin.y == b.origin.y && a.origin.z == b.origin.z
        && a.sizeX == b.sizeX && a.sizeZ == b.sizeZ && a.resolutionX == b.resolutionX
        && a.resolutionZ == b.resolutionZ && a.textureTiling == b.textureTiling
        && TerrainHeightsEqual(a, b) && TerrainMaterialWeightsEqual(a, b);
}

inline bool TerrainSpecEqual(const TerrainSpec& a, const TerrainSpec& b)
{
    return a.enabled == b.enabled && TerrainMeshDataEqual(a, b)
        && a.textureIdentity == b.textureIdentity && TerrainExtraLayersEqual(a, b);
}

inline float TerrainSampleSpacingX(const TerrainSpec& terrain)
{
    if (terrain.resolutionX <= 1)
    {
        return 0.0f;
    }
    return terrain.sizeX / static_cast<float>(terrain.resolutionX - 1);
}

inline float TerrainSampleSpacingZ(const TerrainSpec& terrain)
{
    if (terrain.resolutionZ <= 1)
    {
        return 0.0f;
    }
    return terrain.sizeZ / static_cast<float>(terrain.resolutionZ - 1);
}

inline int TerrainHeightIndex(const TerrainSpec& terrain, int ix, int iz)
{
    return iz * terrain.resolutionX + ix;
}

inline core::Vec3 TerrainSamplePosition(const TerrainSpec& terrain, int ix, int iz)
{
    const int index = TerrainHeightIndex(terrain, ix, iz);
    const float height =
        (index >= 0 && index < static_cast<int>(terrain.heights.size()))
            ? terrain.heights[static_cast<std::size_t>(index)]
            : 0.0f;
    return {
        terrain.origin.x + static_cast<float>(ix) * TerrainSampleSpacingX(terrain),
        terrain.origin.y + height,
        terrain.origin.z + static_cast<float>(iz) * TerrainSampleSpacingZ(terrain)};
}

struct TerrainTexCoord
{
    float u = 0.0f;
    float v = 0.0f;
};

inline TerrainTexCoord TerrainSampleTexCoord(const TerrainSpec& terrain, int ix, int iz)
{
    return {
        static_cast<float>(ix) * TerrainSampleSpacingX(terrain) * terrain.textureTiling,
        static_cast<float>(iz) * TerrainSampleSpacingZ(terrain) * terrain.textureTiling};
}

inline const std::string& TerrainLayerTextureIdentity(const TerrainSpec& terrain, int layer)
{
    static const std::string kEmpty{};
    if (layer <= 0)
    {
        return terrain.textureIdentity;
    }
    const int extra = layer - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size()))
    {
        return kEmpty;
    }
    return terrain.extraLayers[static_cast<std::size_t>(extra)].textureIdentity;
}

inline float TerrainLayerTextureTiling(const TerrainSpec& terrain, int layer)
{
    if (layer <= 0)
    {
        return terrain.textureTiling;
    }
    const int extra = layer - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size()))
    {
        return kDefaultTerrainTextureTiling;
    }
    return terrain.extraLayers[static_cast<std::size_t>(extra)].textureTiling;
}

inline bool TerrainReferencesTextureIdentity(const TerrainSpec& terrain, std::string_view identity)
{
    if (identity.empty())
    {
        return false;
    }
    if (terrain.textureIdentity == identity)
    {
        return true;
    }
    for (const TerrainMaterialLayer& layer : terrain.extraLayers)
    {
        if (layer.textureIdentity == identity)
        {
            return true;
        }
    }
    return false;
}

inline void CollectTerrainTextureIdentities(
    const TerrainSpec& terrain,
    std::vector<std::string>& out)
{
    if (!TerrainTextureIdentityIsNone(terrain.textureIdentity)
        && assets::RuntimePngIdentityIsValid(terrain.textureIdentity))
    {
        out.push_back(terrain.textureIdentity);
    }
    for (const TerrainMaterialLayer& layer : terrain.extraLayers)
    {
        if (assets::RuntimePngIdentityIsValid(layer.textureIdentity))
        {
            out.push_back(layer.textureIdentity);
        }
    }
}

inline void NormalizeTerrainSampleWeights(float* weights)
{
    if (weights == nullptr)
    {
        return;
    }
    float sum = 0.0f;
    for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
    {
        float value = weights[layer];
        if (!std::isfinite(value) || value < 0.0f)
        {
            value = 0.0f;
        }
        weights[layer] = value;
        sum += value;
    }
    if (!(sum > 0.0f) || !std::isfinite(sum))
    {
        weights[0] = 1.0f;
        for (int layer = 1; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            weights[layer] = 0.0f;
        }
        return;
    }
    const float inv = 1.0f / sum;
    for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
    {
        weights[layer] *= inv;
    }
}

inline void EnsureTerrainMaterialWeights(TerrainSpec& terrain)
{
    const int samples = TerrainSampleCount(terrain);
    const int expected = samples * kMaxTerrainMaterialLayers;
    if (expected <= 0)
    {
        terrain.materialWeights.clear();
        return;
    }
    if (static_cast<int>(terrain.materialWeights.size()) == expected)
    {
        return;
    }
    terrain.materialWeights.assign(static_cast<std::size_t>(expected), 0.0f);
    for (int sample = 0; sample < samples; ++sample)
    {
        terrain.materialWeights[static_cast<std::size_t>(
            TerrainMaterialWeightIndex(sample, 0))] = 1.0f;
    }
}

inline void CompactDefaultTerrainMaterialWeights(TerrainSpec& terrain)
{
    if (TerrainMaterialWeightsAreDefault(terrain))
    {
        terrain.materialWeights.clear();
    }
}

inline void ReadTerrainSampleWeights(const TerrainSpec& terrain, int sampleIndex, float* out)
{
    if (out == nullptr)
    {
        return;
    }
    for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
    {
        out[layer] = TerrainSampleLayerWeight(terrain, sampleIndex, layer);
    }
}

inline void WriteTerrainSampleWeights(TerrainSpec& terrain, int sampleIndex, const float* weights)
{
    if (weights == nullptr)
    {
        return;
    }
    EnsureTerrainMaterialWeights(terrain);
    float normalized[kMaxTerrainMaterialLayers];
    for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
    {
        normalized[layer] = weights[layer];
    }
    NormalizeTerrainSampleWeights(normalized);
    for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
    {
        const int index = TerrainMaterialWeightIndex(sampleIndex, layer);
        if (index >= 0 && index < static_cast<int>(terrain.materialWeights.size()))
        {
            terrain.materialWeights[static_cast<std::size_t>(index)] = normalized[layer];
        }
    }
}

inline void QuantizeTerrainSampleWeights(const float* weights, int* out)
{
    std::array<float, kMaxTerrainMaterialLayers> scaled{};
    std::array<int, kMaxTerrainMaterialLayers> quantized{};
    int sum = 0;
    for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
    {
        float value = weights != nullptr ? weights[layer] : (layer == 0 ? 1.0f : 0.0f);
        if (!std::isfinite(value) || value < 0.0f)
        {
            value = 0.0f;
        }
        if (value > 1.0f)
        {
            value = 1.0f;
        }
        scaled[static_cast<std::size_t>(layer)] =
            value * static_cast<float>(kTerrainMaterialWeightQuantum);
        quantized[static_cast<std::size_t>(layer)] =
            static_cast<int>(std::floor(scaled[static_cast<std::size_t>(layer)]));
        if (quantized[static_cast<std::size_t>(layer)] < 0)
        {
            quantized[static_cast<std::size_t>(layer)] = 0;
        }
        if (quantized[static_cast<std::size_t>(layer)] > kTerrainMaterialWeightQuantum)
        {
            quantized[static_cast<std::size_t>(layer)] = kTerrainMaterialWeightQuantum;
        }
        sum += quantized[static_cast<std::size_t>(layer)];
    }
    int remainder = kTerrainMaterialWeightQuantum - sum;
    while (remainder > 0)
    {
        int best = 0;
        float bestFrac = -1.0f;
        for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            const float whole =
                static_cast<float>(quantized[static_cast<std::size_t>(layer)]);
            const float frac = scaled[static_cast<std::size_t>(layer)] - whole;
            if (frac > bestFrac
                && quantized[static_cast<std::size_t>(layer)] < kTerrainMaterialWeightQuantum)
            {
                bestFrac = frac;
                best = layer;
            }
        }
        quantized[static_cast<std::size_t>(best)] += 1;
        --remainder;
    }
    while (remainder < 0)
    {
        int best = 0;
        int bestValue = -1;
        for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            if (quantized[static_cast<std::size_t>(layer)] > bestValue)
            {
                bestValue = quantized[static_cast<std::size_t>(layer)];
                best = layer;
            }
        }
        if (quantized[static_cast<std::size_t>(best)] <= 0)
        {
            break;
        }
        quantized[static_cast<std::size_t>(best)] -= 1;
        ++remainder;
    }
    if (out != nullptr)
    {
        for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            out[layer] = quantized[static_cast<std::size_t>(layer)];
        }
    }
}

inline void DequantizeTerrainSampleWeights(const int* quantized, float* out)
{
    float weights[kMaxTerrainMaterialLayers]{};
    if (quantized != nullptr)
    {
        for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            int value = quantized[layer];
            if (value < 0)
            {
                value = 0;
            }
            if (value > kTerrainMaterialWeightQuantum)
            {
                value = kTerrainMaterialWeightQuantum;
            }
            weights[layer] =
                static_cast<float>(value) / static_cast<float>(kTerrainMaterialWeightQuantum);
        }
    }
    else
    {
        weights[0] = 1.0f;
    }
    NormalizeTerrainSampleWeights(weights);
    if (out != nullptr)
    {
        for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            out[layer] = weights[layer];
        }
    }
}

inline bool TryAssignTerrainTextureIdentity(TerrainSpec& terrain, std::string_view identity)
{
    if (!assets::RuntimePngIdentityIsValid(identity) || terrain.textureIdentity == identity)
    {
        return false;
    }
    for (const TerrainMaterialLayer& layer : terrain.extraLayers)
    {
        if (layer.textureIdentity == identity)
        {
            return false;
        }
    }
    terrain.textureIdentity = std::string(identity);
    return true;
}

inline bool TryAddTerrainMaterialLayer(TerrainSpec& terrain, std::string_view identity)
{
    if (static_cast<int>(terrain.extraLayers.size()) >= kMaxTerrainMaterialLayers - 1
        || !assets::RuntimePngIdentityIsValid(identity)
        || TerrainReferencesTextureIdentity(terrain, identity))
    {
        return false;
    }
    TerrainMaterialLayer layer{};
    layer.textureIdentity = std::string(identity);
    layer.textureTiling = kDefaultTerrainTextureTiling;
    terrain.extraLayers.push_back(layer);
    return true;
}

inline bool TryRemoveTerrainMaterialLayer(TerrainSpec& terrain, int layerIndex)
{
    if (layerIndex <= 0 || layerIndex >= TerrainMaterialLayerCount(terrain))
    {
        return false;
    }
    EnsureTerrainMaterialWeights(terrain);
    const int samples = TerrainSampleCount(terrain);
    for (int sample = 0; sample < samples; ++sample)
    {
        float weights[kMaxTerrainMaterialLayers];
        ReadTerrainSampleWeights(terrain, sample, weights);
        weights[0] += weights[layerIndex];
        for (int layer = layerIndex; layer < kMaxTerrainMaterialLayers - 1; ++layer)
        {
            weights[layer] = weights[layer + 1];
        }
        weights[kMaxTerrainMaterialLayers - 1] = 0.0f;
        WriteTerrainSampleWeights(terrain, sample, weights);
    }
    terrain.extraLayers.erase(
        terrain.extraLayers.begin() + static_cast<std::ptrdiff_t>(layerIndex - 1));
    CompactDefaultTerrainMaterialWeights(terrain);
    return true;
}

inline bool TrySetTerrainLayerTextureTiling(TerrainSpec& terrain, int layerIndex, float tiling)
{
    if (!TerrainTextureTilingIsValid(tiling))
    {
        return false;
    }
    if (layerIndex <= 0)
    {
        if (terrain.textureTiling == tiling)
        {
            return false;
        }
        terrain.textureTiling = tiling;
        return true;
    }
    const int extra = layerIndex - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size()))
    {
        return false;
    }
    if (terrain.extraLayers[static_cast<std::size_t>(extra)].textureTiling == tiling)
    {
        return false;
    }
    terrain.extraLayers[static_cast<std::size_t>(extra)].textureTiling = tiling;
    return true;
}

inline bool TryClearTerrainTextureIdentity(TerrainSpec& terrain)
{
    if (TerrainTextureIdentityIsNone(terrain.textureIdentity))
    {
        return false;
    }
    terrain.textureIdentity.clear();
    return true;
}

inline bool TrySetTerrainTextureTiling(TerrainSpec& terrain, float tiling)
{
    if (!TerrainTextureTilingIsValid(tiling) || terrain.textureTiling == tiling)
    {
        return false;
    }
    terrain.textureTiling = tiling;
    return true;
}
}
