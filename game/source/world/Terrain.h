#pragma once

// Milestone 86/87/88/90/92/93/96/97: optional singleton authored Terrain. Regular XZ
// heightfield. Not a repeatable prop category, tile set, GUID, or generic
// mesh editor. M87 sculpts these authored heights[] only; brush parameters
// are not Level data. M88 adds one optional base surface texture identity
// plus planar XZ tiling. M90 painted up to three extra layers as per-sample
// RGBA weights. M92 replaces that with an ordered palette (practical cap 16)
// and dedicated RGBA weight maps whose resolution is independent of the
// heightfield. Four layers share one packed map. M93 adds a vegetation
// palette and a separate occupancy grid; derived transforms are not stored.
// M96 adds a dedicated Ground Cover density grid. It is not vegetation
// occupancy and does not store per-blade transforms. M97 adds optional
// Normal and Roughness identities to each material layer; they use the
// same M92 paint weights and are not separate paint layers.
// Not a generic Material
// asset, engine-wide PBR authoring, or external splat editor.
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
// textures/<file>.png; never an absolute path. Weight texels use the same
// min-corner convention on their own grid:
//   texel(ix, iz) XZ =
//     (origin.x + ix * sizeX / (weightResolutionX - 1),
//      origin.z + iz * sizeZ / (weightResolutionZ - 1))
// Changing heights does not move weights. Sculpt does not edit weights.

#include "assets/RuntimePng.h"
#include "core/Vec3.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
// Four weights share one RGBA map. Four maps is the practical palette cap:
// 16 layers, well above the old M90 limit, and small enough for the 64 KiB /
// 256-line Level guard at the default weight resolution.
inline constexpr int kTerrainWeightsPerMap = 4;
inline constexpr int kMaxTerrainWeightMaps = 4;
inline constexpr int kMaxTerrainMaterialLayers = kTerrainWeightsPerMap * kMaxTerrainWeightMaps;
inline constexpr int kLegacyTerrainPaintLayers = 4;
inline constexpr int kTerrainMaterialWeightQuantum = 255;
inline constexpr int kDefaultTerrainWeightResolutionX = 32;
inline constexpr int kDefaultTerrainWeightResolutionZ = 32;
inline constexpr int kMinTerrainWeightResolution = 2;
inline constexpr int kMaxTerrainWeightResolution = 32;

// Vegetation placement is its own grid. It is not the heightfield and not
// the M92 weight-map resolution. 24x24 keeps a fully painted map inside the
// 256-line Level guard beside a maximum material palette.
inline constexpr int kMinTerrainVegetationResolution = 4;
inline constexpr int kMaxTerrainVegetationResolution = 24;
inline constexpr int kDefaultTerrainVegetationResolutionX = 16;
inline constexpr int kDefaultTerrainVegetationResolutionZ = 16;
inline constexpr int kMaxTerrainVegetationEntries = 8;
inline constexpr int kMaxTerrainVegetationInstancesPerCell = 4;
inline constexpr float kDefaultTerrainVegetationDensity = 1.0f;
inline constexpr float kMinTerrainVegetationDensity = 0.05f;
inline constexpr float kMaxTerrainVegetationDensity = 8.0f;
inline constexpr float kDefaultTerrainVegetationMinScale = 0.8f;
inline constexpr float kDefaultTerrainVegetationMaxScale = 1.2f;
inline constexpr float kMinTerrainVegetationScale = 0.05f;
inline constexpr float kMaxTerrainVegetationScale = 8.0f;
inline constexpr std::uint32_t kDefaultTerrainVegetationSeed = 1u;

// Ground cover is a separate dense-detail domain. 8x8 keeps a fully painted
// 4-entry map inside a handful of Level v1 lines beside max materials and
// max vegetation. Palette 4 matches RGBA occupancy bits and editor cards.
inline constexpr int kMinTerrainGroundCoverResolution = 4;
inline constexpr int kMaxTerrainGroundCoverResolution = 8;
inline constexpr int kDefaultTerrainGroundCoverResolutionX = 8;
inline constexpr int kDefaultTerrainGroundCoverResolutionZ = 8;
inline constexpr int kMaxTerrainGroundCoverEntries = 4;
inline constexpr int kMaxTerrainGroundCoverInstancesPerCell = 32;
inline constexpr int kMaxTerrainGroundCoverInstances = 16384;
inline constexpr float kDefaultTerrainGroundCoverDensity = 12.0f;
inline constexpr float kMinTerrainGroundCoverDensity = 0.5f;
inline constexpr float kMaxTerrainGroundCoverDensity = 48.0f;
inline constexpr float kDefaultTerrainGroundCoverMinWidth = 0.22f;
inline constexpr float kDefaultTerrainGroundCoverMaxWidth = 0.55f;
inline constexpr float kMinTerrainGroundCoverWidth = 0.05f;
inline constexpr float kMaxTerrainGroundCoverWidth = 1.5f;
inline constexpr float kDefaultTerrainGroundCoverMinHeight = 0.18f;
inline constexpr float kDefaultTerrainGroundCoverMaxHeight = 0.48f;
inline constexpr float kMinTerrainGroundCoverHeight = 0.05f;
inline constexpr float kMaxTerrainGroundCoverHeight = 2.0f;
inline constexpr std::uint32_t kDefaultTerrainGroundCoverSeed = 1u;
inline constexpr int kTerrainGroundCoverStyleHexDigits = 2;
inline constexpr std::string_view kTerrainGroundCoverDataKeyword = "terrain_cover_data";
inline constexpr int kTerrainGroundCoverDataHexPerRecord = 492;

struct TerrainMaterialLayer
{
    std::string textureIdentity{};
    std::string normalIdentity{};
    std::string roughnessIdentity{};
    float textureTiling = kDefaultTerrainTextureTiling;
};

struct TerrainVegetationEntry
{
    std::string modelIdentity{};
    float density = kDefaultTerrainVegetationDensity;
    float minScale = kDefaultTerrainVegetationMinScale;
    float maxScale = kDefaultTerrainVegetationMaxScale;
    bool randomYaw = true;
    bool alignToNormal = false;
};

struct TerrainGroundCoverEntry
{
    std::string textureIdentity{};
    float density = kDefaultTerrainGroundCoverDensity;
    float minWidth = kDefaultTerrainGroundCoverMinWidth;
    float maxWidth = kDefaultTerrainGroundCoverMaxWidth;
    float minHeight = kDefaultTerrainGroundCoverMinHeight;
    float maxHeight = kDefaultTerrainGroundCoverMaxHeight;
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
    // Layer 0 (M88). Extra layers are palette entries 1..N.
    // M97 optional Normal/Roughness belong to the same layer as albedo.
    std::string textureIdentity{};
    std::string normalIdentity{};
    std::string roughnessIdentity{};
    float textureTiling = kDefaultTerrainTextureTiling;
    std::vector<TerrainMaterialLayer> extraLayers{};
    int weightResolutionX = kDefaultTerrainWeightResolutionX;
    int weightResolutionZ = kDefaultTerrainWeightResolutionZ;
    // Empty means implicit defaults: layer 0 = 1, others = 0 per weight texel.
    // Non-empty size is weightTexelCount * kMaxTerrainMaterialLayers.
    std::vector<float> materialWeights{};
    // M93. Resolution 0 and empty entries/cells mean no authored vegetation.
    // Each cell is a bitmask. Bit i set means vegetationEntries[i] occupies that cell.
    // Slot (cell, entry) stores Paint-captured parameters:
    // vegetationDensityQuanta is 8-bit density across [0.05, 8];
    // vegetationPaintParams packs 5-bit min scale, 5-bit max scale, Random Yaw,
    // and Align Normal. Palette entry fields are Next-Paint only.
    int vegetationResolutionX = 0;
    int vegetationResolutionZ = 0;
    std::uint32_t vegetationSeed = kDefaultTerrainVegetationSeed;
    std::vector<TerrainVegetationEntry> vegetationEntries{};
    std::vector<unsigned char> vegetationCells{};
    std::vector<unsigned char> vegetationDensityQuanta{};
    std::vector<std::uint16_t> vegetationPaintParams{};
    // M96. Resolution 0 and empty entries/cells mean no authored ground cover.
    // Each cell is a 4-bit occupancy mask. Bit i set means groundCoverEntries[i]
    // occupies that cell. Slot (cell, entry) stores Paint-captured density
    // (8-bit) and packed min/max width/height (16-bit). Palette entry fields
    // are Next-Paint only. Derived blade/tuft transforms are not stored.
    int groundCoverResolutionX = 0;
    int groundCoverResolutionZ = 0;
    std::uint32_t groundCoverSeed = kDefaultTerrainGroundCoverSeed;
    std::vector<TerrainGroundCoverEntry> groundCoverEntries{};
    std::vector<unsigned char> groundCoverCells{};
    std::vector<unsigned char> groundCoverDensityQuanta{};
    std::vector<std::uint16_t> groundCoverPaintParams{};
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

inline bool TerrainOptionalChannelIdentityIsValid(std::string_view identity)
{
    return identity.empty() || assets::RuntimePngIdentityIsValid(identity);
}

// Terrain-specific picker convention. Not a generalized asset tag database.
// Content Browser still lists every runtime PNG. Normal/Roughness pickers
// keep identities whose filename stem uses these suffixes.
inline bool TerrainNormalMapIdentityIsCompatible(std::string_view identity)
{
    if (!assets::RuntimePngIdentityIsValid(identity))
    {
        return false;
    }
    return identity.ends_with("_NormalGL.png") || identity.ends_with("_NormalDX.png")
        || identity.ends_with("_Normal.png") || identity.ends_with("_normal.png")
        || identity.ends_with("_nrm.png");
}

// Shader/TBN is OpenGL-style (green = +bitangent). DirectX maps invert Y.
inline bool TerrainNormalMapIdentityIsDirectX(std::string_view identity)
{
    return assets::RuntimePngIdentityIsValid(identity) && identity.ends_with("_NormalDX.png");
}

inline bool TerrainRoughnessMapIdentityIsCompatible(std::string_view identity)
{
    if (!assets::RuntimePngIdentityIsValid(identity))
    {
        return false;
    }
    return identity.ends_with("_Roughness.png") || identity.ends_with("_roughness.png")
        || identity.ends_with("_rough.png");
}

inline bool TerrainLayerOptionalChannelsShouldWrite(
    std::string_view normalIdentity,
    std::string_view roughnessIdentity)
{
    return !normalIdentity.empty() || !roughnessIdentity.empty();
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

inline int TerrainWeightMapCountForLayers(int layerCount)
{
    if (layerCount < 1)
    {
        layerCount = 1;
    }
    if (layerCount > kMaxTerrainMaterialLayers)
    {
        layerCount = kMaxTerrainMaterialLayers;
    }
    return (layerCount + kTerrainWeightsPerMap - 1) / kTerrainWeightsPerMap;
}

inline int TerrainPackedWeightMapCount(const TerrainSpec& terrain)
{
    return TerrainWeightMapCountForLayers(TerrainMaterialLayerCount(terrain));
}

inline bool TerrainWeightResolutionIsValid(int resolution)
{
    return resolution >= kMinTerrainWeightResolution && resolution <= kMaxTerrainWeightResolution;
}

inline int TerrainWeightTexelCount(const TerrainSpec& terrain)
{
    if (!TerrainWeightResolutionIsValid(terrain.weightResolutionX)
        || !TerrainWeightResolutionIsValid(terrain.weightResolutionZ))
    {
        return 0;
    }
    return terrain.weightResolutionX * terrain.weightResolutionZ;
}

inline int TerrainWeightTexelIndex(const TerrainSpec& terrain, int ix, int iz)
{
    return iz * terrain.weightResolutionX + ix;
}

inline int TerrainMaterialWeightIndex(int texelIndex, int layer)
{
    return texelIndex * kMaxTerrainMaterialLayers + layer;
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
            || !TerrainOptionalChannelIdentityIsValid(layer.normalIdentity)
            || !TerrainOptionalChannelIdentityIsValid(layer.roughnessIdentity)
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

inline float TerrainTexelLayerWeight(const TerrainSpec& terrain, int texelIndex, int layer)
{
    if (layer < 0 || layer >= kMaxTerrainMaterialLayers || texelIndex < 0)
    {
        return 0.0f;
    }
    if (terrain.materialWeights.empty())
    {
        return layer == 0 ? 1.0f : 0.0f;
    }
    const int index = TerrainMaterialWeightIndex(texelIndex, layer);
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
    const int texels = TerrainWeightTexelCount(terrain);
    if (static_cast<int>(terrain.materialWeights.size()) != texels * kMaxTerrainMaterialLayers)
    {
        return false;
    }
    for (int texel = 0; texel < texels; ++texel)
    {
        if (TerrainTexelLayerWeight(terrain, texel, 0) != 1.0f)
        {
            return false;
        }
        for (int layer = 1; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            if (TerrainTexelLayerWeight(terrain, texel, layer) != 0.0f)
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
        || terrain.textureTiling != kDefaultTerrainTextureTiling
        || TerrainLayerOptionalChannelsShouldWrite(
            terrain.normalIdentity, terrain.roughnessIdentity);
}

inline bool TerrainWeightHeaderShouldWrite(const TerrainSpec& terrain)
{
    return TerrainPaintRecordsShouldWrite(terrain)
        || terrain.weightResolutionX != kDefaultTerrainWeightResolutionX
        || terrain.weightResolutionZ != kDefaultTerrainWeightResolutionZ;
}

inline bool TerrainVegetationResolutionIsValid(int resolution)
{
    return resolution >= kMinTerrainVegetationResolution
        && resolution <= kMaxTerrainVegetationResolution;
}

inline bool TerrainVegetationDensityIsValid(float density)
{
    return std::isfinite(density) && density >= kMinTerrainVegetationDensity
        && density <= kMaxTerrainVegetationDensity;
}

inline bool TerrainVegetationScaleIsValid(float scale)
{
    return std::isfinite(scale) && scale >= kMinTerrainVegetationScale
        && scale <= kMaxTerrainVegetationScale;
}

inline bool TerrainVegetationStemIsReservedDevice(std::string_view stem)
{
    std::string upper;
    upper.reserve(stem.size());
    for (char ch : stem)
    {
        const unsigned char byte = static_cast<unsigned char>(ch);
        upper.push_back(static_cast<char>(byte >= 'a' && byte <= 'z' ? byte - 32 : byte));
    }
    const auto isDevice = [&](std::string_view name) {
        if (upper == name)
        {
            return true;
        }
        if (upper.size() == name.size() + 1 && upper.starts_with(name) && upper.back() >= '1'
            && upper.back() <= '9')
        {
            return name == "COM" || name == "LPT";
        }
        return false;
    };
    return isDevice("CON") || isDevice("PRN") || isDevice("AUX") || isDevice("NUL")
        || isDevice("COM") || isDevice("LPT");
}

// Same structural rules as assets::TryParseStaticModelIdentity. Inlined so
// TerrainSpecIsValid stays header-only for the existing terrain test targets.
inline bool TerrainVegetationModelIdentityIsValid(std::string_view identity)
{
    constexpr std::string_view kPrefix = "models/";
    if (!identity.starts_with(kPrefix) || identity.find('\\') != std::string_view::npos)
    {
        return false;
    }
    const std::string_view fileName = identity.substr(kPrefix.size());
    constexpr std::string_view kExtension = ".glb";
    if (fileName.size() <= kExtension.size() || !fileName.ends_with(kExtension)
        || fileName.find('/') != std::string_view::npos)
    {
        return false;
    }
    const std::string_view stem = fileName.substr(0, fileName.size() - kExtension.size());
    if (stem.empty() || stem.front() == '.' || stem.front() == ' ' || stem.back() == ' '
        || stem.back() == '.' || fileName.find(".importing.tmp") != std::string_view::npos)
    {
        return false;
    }
    for (char ch : fileName)
    {
        const unsigned char byte = static_cast<unsigned char>(ch);
        if (byte < 32 || ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' || ch == '>'
            || ch == '|')
        {
            return false;
        }
    }
    return !TerrainVegetationStemIsReservedDevice(stem);
}

inline bool TerrainVegetationEntryIsValid(const TerrainVegetationEntry& entry)
{
    return TerrainVegetationModelIdentityIsValid(entry.modelIdentity)
        && TerrainVegetationDensityIsValid(entry.density)
        && TerrainVegetationScaleIsValid(entry.minScale)
        && TerrainVegetationScaleIsValid(entry.maxScale) && entry.minScale <= entry.maxScale;
}

inline bool TerrainVegetationIsAbsent(const TerrainSpec& terrain)
{
    return terrain.vegetationEntries.empty() && terrain.vegetationCells.empty()
        && terrain.vegetationDensityQuanta.empty() && terrain.vegetationPaintParams.empty()
        && terrain.vegetationResolutionX == 0 && terrain.vegetationResolutionZ == 0;
}

inline int TerrainVegetationDensitySlot(int cellIndex, int entryIndex)
{
    return cellIndex * kMaxTerrainVegetationEntries + entryIndex;
}

inline unsigned char QuantizeTerrainVegetationDensity(float density)
{
    if (!TerrainVegetationDensityIsValid(density))
    {
        return 0;
    }
    const float span = kMaxTerrainVegetationDensity - kMinTerrainVegetationDensity;
    const float normalized = (density - kMinTerrainVegetationDensity) / span;
    const int quantum = static_cast<int>(std::lround(normalized * 255.0f));
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

inline float DequantizeTerrainVegetationDensity(unsigned char quantum)
{
    const float span = kMaxTerrainVegetationDensity - kMinTerrainVegetationDensity;
    return kMinTerrainVegetationDensity + (static_cast<float>(quantum) / 255.0f) * span;
}

inline constexpr int kTerrainVegetationStyleScaleMaxQuantum = 31;
inline constexpr int kTerrainVegetationStyleHexDigits = 3;
inline constexpr std::string_view kTerrainVegetationStyleKeyword = "terrain_veg_style";
inline constexpr int kTerrainVegetationStyleHexPerRecord = 492;

inline unsigned char QuantizeTerrainVegetationScale(float scale)
{
    if (!TerrainVegetationScaleIsValid(scale))
    {
        return 0;
    }
    const float span = kMaxTerrainVegetationScale - kMinTerrainVegetationScale;
    const float normalized = (scale - kMinTerrainVegetationScale) / span;
    const int quantum = static_cast<int>(
        std::lround(normalized * static_cast<float>(kTerrainVegetationStyleScaleMaxQuantum)));
    if (quantum <= 0)
    {
        return 0;
    }
    if (quantum >= kTerrainVegetationStyleScaleMaxQuantum)
    {
        return static_cast<unsigned char>(kTerrainVegetationStyleScaleMaxQuantum);
    }
    return static_cast<unsigned char>(quantum);
}

inline float DequantizeTerrainVegetationScale(unsigned char quantum)
{
    const float span = kMaxTerrainVegetationScale - kMinTerrainVegetationScale;
    const float t = static_cast<float>(quantum) / static_cast<float>(kTerrainVegetationStyleScaleMaxQuantum);
    return kMinTerrainVegetationScale + t * span;
}

inline std::uint16_t PackTerrainVegetationPaint(
    unsigned char minScaleQuantum,
    unsigned char maxScaleQuantum,
    bool randomYaw,
    bool alignToNormal)
{
    unsigned char minQ = minScaleQuantum;
    unsigned char maxQ = maxScaleQuantum;
    if (minQ > kTerrainVegetationStyleScaleMaxQuantum)
    {
        minQ = static_cast<unsigned char>(kTerrainVegetationStyleScaleMaxQuantum);
    }
    if (maxQ > kTerrainVegetationStyleScaleMaxQuantum)
    {
        maxQ = static_cast<unsigned char>(kTerrainVegetationStyleScaleMaxQuantum);
    }
    if (minQ > maxQ)
    {
        maxQ = minQ;
    }
    return static_cast<std::uint16_t>(
        (minQ & 31) | ((maxQ & 31) << 5) | ((randomYaw ? 1 : 0) << 10) | ((alignToNormal ? 1 : 0) << 11));
}

inline std::uint16_t PackTerrainVegetationPaintFromEntry(const TerrainVegetationEntry& entry)
{
    return PackTerrainVegetationPaint(
        QuantizeTerrainVegetationScale(entry.minScale),
        QuantizeTerrainVegetationScale(entry.maxScale),
        entry.randomYaw,
        entry.alignToNormal);
}

inline unsigned char TerrainVegetationPaintMinScaleQuantum(std::uint16_t packed)
{
    return static_cast<unsigned char>(packed & 31u);
}

inline unsigned char TerrainVegetationPaintMaxScaleQuantum(std::uint16_t packed)
{
    return static_cast<unsigned char>((packed >> 5) & 31u);
}

inline bool TerrainVegetationPaintRandomYaw(std::uint16_t packed)
{
    return (packed & (1u << 10)) != 0;
}

inline bool TerrainVegetationPaintAlignToNormal(std::uint16_t packed)
{
    return (packed & (1u << 11)) != 0;
}

inline int TerrainVegetationCellIndex(const TerrainSpec& terrain, int ix, int iz)
{
    return iz * terrain.vegetationResolutionX + ix;
}

inline unsigned char TerrainVegetationEntryBit(int entryIndex)
{
    if (entryIndex < 0 || entryIndex >= kMaxTerrainVegetationEntries)
    {
        return 0;
    }
    return static_cast<unsigned char>(1u << entryIndex);
}

inline bool TerrainVegetationCellHasEntry(unsigned char cell, int entryIndex)
{
    const unsigned char bit = TerrainVegetationEntryBit(entryIndex);
    return bit != 0 && (cell & bit) != 0;
}

inline unsigned char TerrainVegetationAllowedCellMask(int entryCount)
{
    if (entryCount <= 0)
    {
        return 0;
    }
    if (entryCount >= kMaxTerrainVegetationEntries)
    {
        return 0xFFu;
    }
    return static_cast<unsigned char>((1u << entryCount) - 1u);
}

inline bool TerrainVegetationDataIsValid(const TerrainSpec& terrain)
{
    if (TerrainVegetationIsAbsent(terrain))
    {
        return true;
    }
    const int entryCount = static_cast<int>(terrain.vegetationEntries.size());
    if (entryCount < 1 || entryCount > kMaxTerrainVegetationEntries
        || !TerrainVegetationResolutionIsValid(terrain.vegetationResolutionX)
        || !TerrainVegetationResolutionIsValid(terrain.vegetationResolutionZ))
    {
        return false;
    }
    const int cells = terrain.vegetationResolutionX * terrain.vegetationResolutionZ;
    if (static_cast<int>(terrain.vegetationCells.size()) != cells
        || static_cast<int>(terrain.vegetationDensityQuanta.size())
            != cells * kMaxTerrainVegetationEntries
        || static_cast<int>(terrain.vegetationPaintParams.size())
            != cells * kMaxTerrainVegetationEntries)
    {
        return false;
    }
    for (const TerrainVegetationEntry& entry : terrain.vegetationEntries)
    {
        if (!TerrainVegetationEntryIsValid(entry))
        {
            return false;
        }
    }
    const unsigned char allowed = TerrainVegetationAllowedCellMask(entryCount);
    for (int cellIndex = 0; cellIndex < cells; ++cellIndex)
    {
        const unsigned char cell = terrain.vegetationCells[static_cast<std::size_t>(cellIndex)];
        if ((cell & static_cast<unsigned char>(~allowed)) != 0)
        {
            return false;
        }
        for (int entryIndex = 0; entryIndex < kMaxTerrainVegetationEntries; ++entryIndex)
        {
            const unsigned char quantum = terrain.vegetationDensityQuanta[static_cast<std::size_t>(
                TerrainVegetationDensitySlot(cellIndex, entryIndex))];
            const std::uint16_t paint = terrain.vegetationPaintParams[static_cast<std::size_t>(
                TerrainVegetationDensitySlot(cellIndex, entryIndex))];
            const bool occupied = TerrainVegetationCellHasEntry(cell, entryIndex);
            if (!occupied && (quantum != 0 || paint != 0))
            {
                return false;
            }
        }
    }
    return true;
}

inline bool TerrainVegetationShouldWrite(const TerrainSpec& terrain)
{
    return !terrain.vegetationEntries.empty();
}

inline bool TerrainVegetationRowIsOccupied(const TerrainSpec& terrain, int row)
{
    if (!TerrainVegetationShouldWrite(terrain) || row < 0 || row >= terrain.vegetationResolutionZ
        || static_cast<int>(terrain.vegetationCells.size())
            != terrain.vegetationResolutionX * terrain.vegetationResolutionZ)
    {
        return false;
    }
    for (int column = 0; column < terrain.vegetationResolutionX; ++column)
    {
        if (terrain.vegetationCells[static_cast<std::size_t>(
                TerrainVegetationCellIndex(terrain, column, row))]
            != 0)
        {
            return true;
        }
    }
    return false;
}

inline int TerrainVegetationOccupiedSlotCount(const TerrainSpec& terrain)
{
    if (!TerrainVegetationShouldWrite(terrain))
    {
        return 0;
    }
    int slots = 0;
    for (unsigned char cell : terrain.vegetationCells)
    {
        for (int entryIndex = 0; entryIndex < kMaxTerrainVegetationEntries; ++entryIndex)
        {
            if (TerrainVegetationCellHasEntry(cell, entryIndex))
            {
                ++slots;
            }
        }
    }
    return slots;
}

inline int TerrainVegetationStyleRecordCount(const TerrainSpec& terrain)
{
    const int slots = TerrainVegetationOccupiedSlotCount(terrain);
    if (slots <= 0)
    {
        return 0;
    }
    const int slotsPerRecord = kTerrainVegetationStyleHexPerRecord / kTerrainVegetationStyleHexDigits;
    return (slots + slotsPerRecord - 1) / slotsPerRecord;
}

inline int TerrainVegetationOccupiedRowCount(const TerrainSpec& terrain)
{
    if (!TerrainVegetationShouldWrite(terrain))
    {
        return 0;
    }
    int rows = 0;
    for (int row = 0; row < terrain.vegetationResolutionZ; ++row)
    {
        if (TerrainVegetationRowIsOccupied(terrain, row))
        {
            ++rows;
        }
    }
    return rows;
}

inline int TerrainVegetationRecordLineCount(const TerrainSpec& terrain)
{
    if (!TerrainVegetationShouldWrite(terrain))
    {
        return 0;
    }
    return 1 + static_cast<int>(terrain.vegetationEntries.size())
        + TerrainVegetationOccupiedRowCount(terrain) + TerrainVegetationStyleRecordCount(terrain);
}

inline bool TerrainVegetationEqual(const TerrainSpec& a, const TerrainSpec& b)
{
    if (a.vegetationResolutionX != b.vegetationResolutionX
        || a.vegetationResolutionZ != b.vegetationResolutionZ || a.vegetationSeed != b.vegetationSeed
        || a.vegetationEntries.size() != b.vegetationEntries.size()
        || a.vegetationCells.size() != b.vegetationCells.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < a.vegetationEntries.size(); ++index)
    {
        const TerrainVegetationEntry& left = a.vegetationEntries[index];
        const TerrainVegetationEntry& right = b.vegetationEntries[index];
        if (left.modelIdentity != right.modelIdentity || left.density != right.density
            || left.minScale != right.minScale || left.maxScale != right.maxScale
            || left.randomYaw != right.randomYaw || left.alignToNormal != right.alignToNormal)
        {
            return false;
        }
    }
    return a.vegetationCells == b.vegetationCells
        && a.vegetationDensityQuanta == b.vegetationDensityQuanta
        && a.vegetationPaintParams == b.vegetationPaintParams;
}

inline bool TerrainGroundCoverResolutionIsValid(int resolution)
{
    return resolution >= kMinTerrainGroundCoverResolution
        && resolution <= kMaxTerrainGroundCoverResolution;
}

inline bool TerrainGroundCoverDensityIsValid(float density)
{
    return std::isfinite(density) && density >= kMinTerrainGroundCoverDensity
        && density <= kMaxTerrainGroundCoverDensity;
}

inline bool TerrainGroundCoverWidthIsValid(float width)
{
    return std::isfinite(width) && width >= kMinTerrainGroundCoverWidth
        && width <= kMaxTerrainGroundCoverWidth;
}

inline bool TerrainGroundCoverHeightIsValid(float height)
{
    return std::isfinite(height) && height >= kMinTerrainGroundCoverHeight
        && height <= kMaxTerrainGroundCoverHeight;
}

inline bool TerrainGroundCoverTextureIdentityIsValid(std::string_view identity)
{
    return assets::RuntimePngIdentityIsValid(identity);
}

inline bool TerrainGroundCoverEntryIsValid(const TerrainGroundCoverEntry& entry)
{
    return TerrainGroundCoverTextureIdentityIsValid(entry.textureIdentity)
        && TerrainGroundCoverDensityIsValid(entry.density)
        && TerrainGroundCoverWidthIsValid(entry.minWidth)
        && TerrainGroundCoverWidthIsValid(entry.maxWidth) && entry.minWidth <= entry.maxWidth
        && TerrainGroundCoverHeightIsValid(entry.minHeight)
        && TerrainGroundCoverHeightIsValid(entry.maxHeight)
        && entry.minHeight <= entry.maxHeight;
}

inline bool TerrainGroundCoverIsAbsent(const TerrainSpec& terrain)
{
    return terrain.groundCoverEntries.empty() && terrain.groundCoverCells.empty()
        && terrain.groundCoverDensityQuanta.empty() && terrain.groundCoverPaintParams.empty()
        && terrain.groundCoverResolutionX == 0 && terrain.groundCoverResolutionZ == 0;
}

inline int TerrainGroundCoverDensitySlot(int cellIndex, int entryIndex)
{
    return cellIndex * kMaxTerrainGroundCoverEntries + entryIndex;
}

inline unsigned char QuantizeTerrainGroundCoverDensity(float density)
{
    if (!TerrainGroundCoverDensityIsValid(density))
    {
        return 0;
    }
    const float span = kMaxTerrainGroundCoverDensity - kMinTerrainGroundCoverDensity;
    const int quantum = static_cast<int>(
        std::lround(((density - kMinTerrainGroundCoverDensity) / span) * 255.0f));
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

inline float DequantizeTerrainGroundCoverDensity(unsigned char quantum)
{
    const float span = kMaxTerrainGroundCoverDensity - kMinTerrainGroundCoverDensity;
    return kMinTerrainGroundCoverDensity + (static_cast<float>(quantum) / 255.0f) * span;
}

inline constexpr int kTerrainGroundCoverStyleMaxQuantum = 15;

inline unsigned char QuantizeTerrainGroundCoverWidth(float width)
{
    if (!TerrainGroundCoverWidthIsValid(width))
    {
        return 0;
    }
    const float span = kMaxTerrainGroundCoverWidth - kMinTerrainGroundCoverWidth;
    const int quantum = static_cast<int>(
        std::lround(((width - kMinTerrainGroundCoverWidth) / span)
            * static_cast<float>(kTerrainGroundCoverStyleMaxQuantum)));
    if (quantum <= 0)
    {
        return 0;
    }
    if (quantum >= kTerrainGroundCoverStyleMaxQuantum)
    {
        return static_cast<unsigned char>(kTerrainGroundCoverStyleMaxQuantum);
    }
    return static_cast<unsigned char>(quantum);
}

inline float DequantizeTerrainGroundCoverWidth(unsigned char quantum)
{
    const float span = kMaxTerrainGroundCoverWidth - kMinTerrainGroundCoverWidth;
    const float t = static_cast<float>(quantum) / static_cast<float>(kTerrainGroundCoverStyleMaxQuantum);
    return kMinTerrainGroundCoverWidth + t * span;
}

inline unsigned char QuantizeTerrainGroundCoverHeight(float height)
{
    if (!TerrainGroundCoverHeightIsValid(height))
    {
        return 0;
    }
    const float span = kMaxTerrainGroundCoverHeight - kMinTerrainGroundCoverHeight;
    const int quantum = static_cast<int>(
        std::lround(((height - kMinTerrainGroundCoverHeight) / span)
            * static_cast<float>(kTerrainGroundCoverStyleMaxQuantum)));
    if (quantum <= 0)
    {
        return 0;
    }
    if (quantum >= kTerrainGroundCoverStyleMaxQuantum)
    {
        return static_cast<unsigned char>(kTerrainGroundCoverStyleMaxQuantum);
    }
    return static_cast<unsigned char>(quantum);
}

inline float DequantizeTerrainGroundCoverHeight(unsigned char quantum)
{
    const float span = kMaxTerrainGroundCoverHeight - kMinTerrainGroundCoverHeight;
    const float t = static_cast<float>(quantum) / static_cast<float>(kTerrainGroundCoverStyleMaxQuantum);
    return kMinTerrainGroundCoverHeight + t * span;
}

inline std::uint16_t PackTerrainGroundCoverPaint(
    unsigned char minWidthQuantum,
    unsigned char maxWidthQuantum,
    unsigned char minHeightQuantum,
    unsigned char maxHeightQuantum)
{
    unsigned char minW = minWidthQuantum;
    unsigned char maxW = maxWidthQuantum;
    unsigned char minH = minHeightQuantum;
    unsigned char maxH = maxHeightQuantum;
    if (minW > kTerrainGroundCoverStyleMaxQuantum)
    {
        minW = static_cast<unsigned char>(kTerrainGroundCoverStyleMaxQuantum);
    }
    if (maxW > kTerrainGroundCoverStyleMaxQuantum)
    {
        maxW = static_cast<unsigned char>(kTerrainGroundCoverStyleMaxQuantum);
    }
    if (minH > kTerrainGroundCoverStyleMaxQuantum)
    {
        minH = static_cast<unsigned char>(kTerrainGroundCoverStyleMaxQuantum);
    }
    if (maxH > kTerrainGroundCoverStyleMaxQuantum)
    {
        maxH = static_cast<unsigned char>(kTerrainGroundCoverStyleMaxQuantum);
    }
    if (minW > maxW)
    {
        maxW = minW;
    }
    if (minH > maxH)
    {
        maxH = minH;
    }
    return static_cast<std::uint16_t>(
        (minW & 15) | ((maxW & 15) << 4) | ((minH & 15) << 8) | ((maxH & 15) << 12));
}

inline std::uint16_t PackTerrainGroundCoverPaintFromEntry(const TerrainGroundCoverEntry& entry)
{
    return PackTerrainGroundCoverPaint(
        QuantizeTerrainGroundCoverWidth(entry.minWidth),
        QuantizeTerrainGroundCoverWidth(entry.maxWidth),
        QuantizeTerrainGroundCoverHeight(entry.minHeight),
        QuantizeTerrainGroundCoverHeight(entry.maxHeight));
}

inline unsigned char TerrainGroundCoverPaintMinWidthQuantum(std::uint16_t packed)
{
    return static_cast<unsigned char>(packed & 15u);
}

inline unsigned char TerrainGroundCoverPaintMaxWidthQuantum(std::uint16_t packed)
{
    return static_cast<unsigned char>((packed >> 4) & 15u);
}

inline unsigned char TerrainGroundCoverPaintMinHeightQuantum(std::uint16_t packed)
{
    return static_cast<unsigned char>((packed >> 8) & 15u);
}

inline unsigned char TerrainGroundCoverPaintMaxHeightQuantum(std::uint16_t packed)
{
    return static_cast<unsigned char>((packed >> 12) & 15u);
}

inline int TerrainGroundCoverCellIndex(const TerrainSpec& terrain, int ix, int iz)
{
    return iz * terrain.groundCoverResolutionX + ix;
}

inline unsigned char TerrainGroundCoverEntryBit(int entryIndex)
{
    if (entryIndex < 0 || entryIndex >= kMaxTerrainGroundCoverEntries)
    {
        return 0;
    }
    return static_cast<unsigned char>(1u << entryIndex);
}

inline bool TerrainGroundCoverCellHasEntry(unsigned char cell, int entryIndex)
{
    const unsigned char bit = TerrainGroundCoverEntryBit(entryIndex);
    return bit != 0 && (cell & bit) != 0;
}

inline unsigned char TerrainGroundCoverAllowedCellMask(int entryCount)
{
    if (entryCount <= 0)
    {
        return 0;
    }
    if (entryCount >= kMaxTerrainGroundCoverEntries)
    {
        return 0x0Fu;
    }
    return static_cast<unsigned char>((1u << entryCount) - 1u);
}

inline bool TerrainGroundCoverDataIsValid(const TerrainSpec& terrain)
{
    if (TerrainGroundCoverIsAbsent(terrain))
    {
        return true;
    }
    const int entryCount = static_cast<int>(terrain.groundCoverEntries.size());
    if (entryCount < 1 || entryCount > kMaxTerrainGroundCoverEntries
        || !TerrainGroundCoverResolutionIsValid(terrain.groundCoverResolutionX)
        || !TerrainGroundCoverResolutionIsValid(terrain.groundCoverResolutionZ))
    {
        return false;
    }
    const int cells = terrain.groundCoverResolutionX * terrain.groundCoverResolutionZ;
    if (static_cast<int>(terrain.groundCoverCells.size()) != cells
        || static_cast<int>(terrain.groundCoverDensityQuanta.size())
            != cells * kMaxTerrainGroundCoverEntries
        || static_cast<int>(terrain.groundCoverPaintParams.size())
            != cells * kMaxTerrainGroundCoverEntries)
    {
        return false;
    }
    for (const TerrainGroundCoverEntry& entry : terrain.groundCoverEntries)
    {
        if (!TerrainGroundCoverEntryIsValid(entry))
        {
            return false;
        }
    }
    const unsigned char allowed = TerrainGroundCoverAllowedCellMask(entryCount);
    for (int cellIndex = 0; cellIndex < cells; ++cellIndex)
    {
        const unsigned char cell = terrain.groundCoverCells[static_cast<std::size_t>(cellIndex)];
        if ((cell & static_cast<unsigned char>(~allowed)) != 0)
        {
            return false;
        }
        for (int entryIndex = 0; entryIndex < kMaxTerrainGroundCoverEntries; ++entryIndex)
        {
            const unsigned char quantum = terrain.groundCoverDensityQuanta[static_cast<std::size_t>(
                TerrainGroundCoverDensitySlot(cellIndex, entryIndex))];
            const std::uint16_t paint = terrain.groundCoverPaintParams[static_cast<std::size_t>(
                TerrainGroundCoverDensitySlot(cellIndex, entryIndex))];
            const bool occupied = TerrainGroundCoverCellHasEntry(cell, entryIndex);
            if (!occupied && (quantum != 0 || paint != 0))
            {
                return false;
            }
        }
    }
    return true;
}

inline bool TerrainGroundCoverShouldWrite(const TerrainSpec& terrain)
{
    return !terrain.groundCoverEntries.empty();
}

inline int TerrainGroundCoverOccupiedSlotCount(const TerrainSpec& terrain)
{
    if (!TerrainGroundCoverShouldWrite(terrain))
    {
        return 0;
    }
    int slots = 0;
    for (unsigned char cell : terrain.groundCoverCells)
    {
        for (int entryIndex = 0; entryIndex < kMaxTerrainGroundCoverEntries; ++entryIndex)
        {
            if (TerrainGroundCoverCellHasEntry(cell, entryIndex))
            {
                ++slots;
            }
        }
    }
    return slots;
}

inline int TerrainGroundCoverDataHexDigitCount(const TerrainSpec& terrain)
{
    if (!TerrainGroundCoverShouldWrite(terrain)
        || static_cast<int>(terrain.groundCoverCells.size())
            != terrain.groundCoverResolutionX * terrain.groundCoverResolutionZ)
    {
        return 0;
    }
    int digits = 0;
    const int cells = terrain.groundCoverResolutionX * terrain.groundCoverResolutionZ;
    for (int cellIndex = 0; cellIndex < cells; ++cellIndex)
    {
        const unsigned char cell = terrain.groundCoverCells[static_cast<std::size_t>(cellIndex)];
        digits += 1;
        for (int entryIndex = 0; entryIndex < kMaxTerrainGroundCoverEntries; ++entryIndex)
        {
            if (TerrainGroundCoverCellHasEntry(cell, entryIndex))
            {
                digits += 6;
            }
        }
    }
    bool anyOccupied = false;
    for (unsigned char cell : terrain.groundCoverCells)
    {
        if (cell != 0)
        {
            anyOccupied = true;
            break;
        }
    }
    return anyOccupied ? digits : 0;
}

inline int TerrainGroundCoverDataRecordCount(const TerrainSpec& terrain)
{
    const int digits = TerrainGroundCoverDataHexDigitCount(terrain);
    if (digits <= 0)
    {
        return 0;
    }
    return (digits + kTerrainGroundCoverDataHexPerRecord - 1) / kTerrainGroundCoverDataHexPerRecord;
}

inline int TerrainGroundCoverRecordLineCount(const TerrainSpec& terrain)
{
    if (!TerrainGroundCoverShouldWrite(terrain))
    {
        return 0;
    }
    return 1 + static_cast<int>(terrain.groundCoverEntries.size())
        + TerrainGroundCoverDataRecordCount(terrain);
}

inline bool TerrainGroundCoverEqual(const TerrainSpec& a, const TerrainSpec& b)
{
    if (a.groundCoverResolutionX != b.groundCoverResolutionX
        || a.groundCoverResolutionZ != b.groundCoverResolutionZ
        || a.groundCoverSeed != b.groundCoverSeed
        || a.groundCoverEntries.size() != b.groundCoverEntries.size()
        || a.groundCoverCells.size() != b.groundCoverCells.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < a.groundCoverEntries.size(); ++index)
    {
        const TerrainGroundCoverEntry& left = a.groundCoverEntries[index];
        const TerrainGroundCoverEntry& right = b.groundCoverEntries[index];
        if (left.textureIdentity != right.textureIdentity || left.density != right.density
            || left.minWidth != right.minWidth || left.maxWidth != right.maxWidth
            || left.minHeight != right.minHeight || left.maxHeight != right.maxHeight)
        {
            return false;
        }
    }
    return a.groundCoverCells == b.groundCoverCells
        && a.groundCoverDensityQuanta == b.groundCoverDensityQuanta
        && a.groundCoverPaintParams == b.groundCoverPaintParams;
}

inline bool TerrainReferencesGroundCoverTexture(
    const TerrainSpec& terrain,
    std::string_view identity)
{
    if (identity.empty())
    {
        return false;
    }
    for (const TerrainGroundCoverEntry& entry : terrain.groundCoverEntries)
    {
        if (entry.textureIdentity == identity)
        {
            return true;
        }
    }
    return false;
}

inline void CollectTerrainGroundCoverTextureIdentities(
    const TerrainSpec& terrain,
    std::vector<std::string>& out)
{
    for (const TerrainGroundCoverEntry& entry : terrain.groundCoverEntries)
    {
        if (assets::RuntimePngIdentityIsValid(entry.textureIdentity))
        {
            out.push_back(entry.textureIdentity);
        }
    }
}

inline bool TerrainReferencesVegetationModel(
    const TerrainSpec& terrain,
    std::string_view identity)
{
    if (identity.empty())
    {
        return false;
    }
    for (const TerrainVegetationEntry& entry : terrain.vegetationEntries)
    {
        if (entry.modelIdentity == identity)
        {
            return true;
        }
    }
    return false;
}

inline int TerrainRecordLineCount(const TerrainSpec& terrain)
{
    if (terrain.resolutionZ < kMinTerrainResolution)
    {
        return 0;
    }
    int lines = 1 + terrain.resolutionZ
        + (TerrainMaterialRecordShouldWrite(terrain) ? 1 : 0)
        + static_cast<int>(terrain.extraLayers.size());
    if (TerrainWeightHeaderShouldWrite(terrain))
    {
        ++lines;
    }
    if (TerrainPaintRecordsShouldWrite(terrain))
    {
        lines += TerrainPackedWeightMapCount(terrain) * terrain.weightResolutionZ;
    }
    lines += TerrainVegetationRecordLineCount(terrain);
    lines += TerrainGroundCoverRecordLineCount(terrain);
    return lines;
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
        || !TerrainOptionalChannelIdentityIsValid(terrain.normalIdentity)
        || !TerrainOptionalChannelIdentityIsValid(terrain.roughnessIdentity)
        || !TerrainTextureTilingIsValid(terrain.textureTiling)
        || !TerrainExtraLayersAreValid(terrain)
        || !TerrainWeightResolutionIsValid(terrain.weightResolutionX)
        || !TerrainWeightResolutionIsValid(terrain.weightResolutionZ)
        || !TerrainVegetationDataIsValid(terrain)
        || !TerrainGroundCoverDataIsValid(terrain))
    {
        return false;
    }
    if (!terrain.materialWeights.empty())
    {
        const int texels = TerrainWeightTexelCount(terrain);
        if (static_cast<int>(terrain.materialWeights.size())
            != texels * kMaxTerrainMaterialLayers)
        {
            return false;
        }
        const int assigned = TerrainMaterialLayerCount(terrain);
        for (int texel = 0; texel < texels; ++texel)
        {
            float sum = 0.0f;
            for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
            {
                const float weight = TerrainTexelLayerWeight(terrain, texel, layer);
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
    if (a.weightResolutionX != b.weightResolutionX || a.weightResolutionZ != b.weightResolutionZ)
    {
        return false;
    }
    const int texels = TerrainWeightTexelCount(a);
    if (TerrainWeightTexelCount(b) != texels)
    {
        return false;
    }
    if (TerrainMaterialWeightsAreDefault(a) && TerrainMaterialWeightsAreDefault(b))
    {
        return true;
    }
    for (int texel = 0; texel < texels; ++texel)
    {
        for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            if (TerrainTexelLayerWeight(a, texel, layer) != TerrainTexelLayerWeight(b, texel, layer))
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
            || a.extraLayers[index].normalIdentity != b.extraLayers[index].normalIdentity
            || a.extraLayers[index].roughnessIdentity != b.extraLayers[index].roughnessIdentity
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
        && TerrainHeightsEqual(a, b);
}

inline bool TerrainSpecEqual(const TerrainSpec& a, const TerrainSpec& b)
{
    return a.enabled == b.enabled && TerrainMeshDataEqual(a, b)
        && a.textureIdentity == b.textureIdentity && a.normalIdentity == b.normalIdentity
        && a.roughnessIdentity == b.roughnessIdentity && TerrainExtraLayersEqual(a, b)
        && TerrainMaterialWeightsEqual(a, b) && TerrainVegetationEqual(a, b)
        && TerrainGroundCoverEqual(a, b);
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

inline const std::string& TerrainLayerNormalIdentity(const TerrainSpec& terrain, int layer)
{
    static const std::string kEmpty{};
    if (layer <= 0)
    {
        return terrain.normalIdentity;
    }
    const int extra = layer - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size()))
    {
        return kEmpty;
    }
    return terrain.extraLayers[static_cast<std::size_t>(extra)].normalIdentity;
}

inline const std::string& TerrainLayerRoughnessIdentity(const TerrainSpec& terrain, int layer)
{
    static const std::string kEmpty{};
    if (layer <= 0)
    {
        return terrain.roughnessIdentity;
    }
    const int extra = layer - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size()))
    {
        return kEmpty;
    }
    return terrain.extraLayers[static_cast<std::size_t>(extra)].roughnessIdentity;
}

inline bool TerrainReferencesAlbedoIdentity(const TerrainSpec& terrain, std::string_view identity)
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

inline bool TerrainReferencesTextureIdentity(const TerrainSpec& terrain, std::string_view identity)
{
    if (identity.empty())
    {
        return false;
    }
    if (terrain.textureIdentity == identity || terrain.normalIdentity == identity
        || terrain.roughnessIdentity == identity)
    {
        return true;
    }
    for (const TerrainMaterialLayer& layer : terrain.extraLayers)
    {
        if (layer.textureIdentity == identity || layer.normalIdentity == identity
            || layer.roughnessIdentity == identity)
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
    const auto appendIfValid = [&](const std::string& identity) {
        if (!identity.empty() && assets::RuntimePngIdentityIsValid(identity))
        {
            out.push_back(identity);
        }
    };
    appendIfValid(terrain.textureIdentity);
    appendIfValid(terrain.normalIdentity);
    appendIfValid(terrain.roughnessIdentity);
    for (const TerrainMaterialLayer& layer : terrain.extraLayers)
    {
        appendIfValid(layer.textureIdentity);
        appendIfValid(layer.normalIdentity);
        appendIfValid(layer.roughnessIdentity);
    }
}

inline void NormalizeTerrainWeights(float* weights, int layerCount)
{
    if (weights == nullptr)
    {
        return;
    }
    if (layerCount < 1)
    {
        layerCount = 1;
    }
    if (layerCount > kMaxTerrainMaterialLayers)
    {
        layerCount = kMaxTerrainMaterialLayers;
    }
    float sum = 0.0f;
    for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
    {
        float value = layer < layerCount ? weights[layer] : 0.0f;
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
    for (int layer = 0; layer < layerCount; ++layer)
    {
        weights[layer] *= inv;
    }
}

inline void EnsureTerrainMaterialWeights(TerrainSpec& terrain)
{
    const int texels = TerrainWeightTexelCount(terrain);
    const int expected = texels * kMaxTerrainMaterialLayers;
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
    for (int texel = 0; texel < texels; ++texel)
    {
        terrain.materialWeights[static_cast<std::size_t>(TerrainMaterialWeightIndex(texel, 0))] = 1.0f;
    }
}

inline void CompactDefaultTerrainMaterialWeights(TerrainSpec& terrain)
{
    if (TerrainMaterialWeightsAreDefault(terrain))
    {
        terrain.materialWeights.clear();
    }
}

inline void ReadTerrainTexelWeights(const TerrainSpec& terrain, int texelIndex, float* out)
{
    if (out == nullptr)
    {
        return;
    }
    for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
    {
        out[layer] = TerrainTexelLayerWeight(terrain, texelIndex, layer);
    }
}

inline void WriteTerrainTexelWeights(TerrainSpec& terrain, int texelIndex, const float* weights)
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
    NormalizeTerrainWeights(normalized, TerrainMaterialLayerCount(terrain));
    for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
    {
        const int index = TerrainMaterialWeightIndex(texelIndex, layer);
        if (index >= 0 && index < static_cast<int>(terrain.materialWeights.size()))
        {
            terrain.materialWeights[static_cast<std::size_t>(index)] = normalized[layer];
        }
    }
}

inline void QuantizeTerrainTexelWeights(const float* weights, int layerCount, int* out)
{
    if (layerCount < 1)
    {
        layerCount = 1;
    }
    if (layerCount > kMaxTerrainMaterialLayers)
    {
        layerCount = kMaxTerrainMaterialLayers;
    }
    std::array<float, kMaxTerrainMaterialLayers> scaled{};
    std::array<int, kMaxTerrainMaterialLayers> quantized{};
    int sum = 0;
    for (int layer = 0; layer < layerCount; ++layer)
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
        bool found = false;
        for (int layer = 0; layer < layerCount; ++layer)
        {
            const float whole = static_cast<float>(quantized[static_cast<std::size_t>(layer)]);
            const float frac = scaled[static_cast<std::size_t>(layer)] - whole;
            if (frac > bestFrac
                && quantized[static_cast<std::size_t>(layer)] < kTerrainMaterialWeightQuantum)
            {
                bestFrac = frac;
                best = layer;
                found = true;
            }
        }
        if (!found)
        {
            break;
        }
        quantized[static_cast<std::size_t>(best)] += 1;
        --remainder;
    }
    while (remainder < 0)
    {
        int best = 0;
        int bestValue = -1;
        for (int layer = 0; layer < layerCount; ++layer)
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
            out[layer] = layer < layerCount ? quantized[static_cast<std::size_t>(layer)] : 0;
        }
    }
}

inline void DequantizeTerrainTexelWeights(const int* quantized, int layerCount, float* out)
{
    if (layerCount < 1)
    {
        layerCount = 1;
    }
    if (layerCount > kMaxTerrainMaterialLayers)
    {
        layerCount = kMaxTerrainMaterialLayers;
    }
    float weights[kMaxTerrainMaterialLayers]{};
    if (quantized != nullptr)
    {
        for (int layer = 0; layer < layerCount; ++layer)
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
    NormalizeTerrainWeights(weights, layerCount);
    if (out != nullptr)
    {
        for (int layer = 0; layer < kMaxTerrainMaterialLayers; ++layer)
        {
            out[layer] = weights[layer];
        }
    }
}

inline char TerrainWeightHexDigit(int nibble)
{
    return static_cast<char>(nibble < 10 ? ('0' + nibble) : ('a' + (nibble - 10)));
}

inline void AppendTerrainWeightMapHex(const int* quantized, int mapIndex, std::string& out)
{
    const int base = mapIndex * kTerrainWeightsPerMap;
    for (int channel = 0; channel < kTerrainWeightsPerMap; ++channel)
    {
        int value = 0;
        if (quantized != nullptr && base + channel < kMaxTerrainMaterialLayers)
        {
            value = quantized[base + channel];
        }
        if (value < 0)
        {
            value = 0;
        }
        if (value > kTerrainMaterialWeightQuantum)
        {
            value = kTerrainMaterialWeightQuantum;
        }
        out.push_back(TerrainWeightHexDigit((value >> 4) & 15));
        out.push_back(TerrainWeightHexDigit(value & 15));
    }
}

inline bool ParseTerrainWeightHexNibble(char digit, int& out)
{
    if (digit >= '0' && digit <= '9')
    {
        out = digit - '0';
        return true;
    }
    if (digit >= 'a' && digit <= 'f')
    {
        out = 10 + (digit - 'a');
        return true;
    }
    return false;
}

inline core::Vec3 TerrainWeightTexelPosition(const TerrainSpec& terrain, int ix, int iz)
{
    const float spanX = terrain.weightResolutionX > 1
        ? terrain.sizeX / static_cast<float>(terrain.weightResolutionX - 1)
        : 0.0f;
    const float spanZ = terrain.weightResolutionZ > 1
        ? terrain.sizeZ / static_cast<float>(terrain.weightResolutionZ - 1)
        : 0.0f;
    return {
        terrain.origin.x + static_cast<float>(ix) * spanX,
        terrain.origin.y,
        terrain.origin.z + static_cast<float>(iz) * spanZ};
}

inline void BuildTerrainWeightMapRgba(const TerrainSpec& terrain, std::vector<unsigned char>& out)
{
    const int maps = TerrainPackedWeightMapCount(terrain);
    const int width = terrain.weightResolutionX;
    const int height = terrain.weightResolutionZ;
    const int texels = TerrainWeightTexelCount(terrain);
    out.assign(static_cast<std::size_t>(maps * width * height * kTerrainWeightsPerMap), 0);
    if (texels <= 0 || maps <= 0)
    {
        return;
    }
    const int layerCount = TerrainMaterialLayerCount(terrain);
    for (int texel = 0; texel < texels; ++texel)
    {
        float weights[kMaxTerrainMaterialLayers];
        ReadTerrainTexelWeights(terrain, texel, weights);
        int quantized[kMaxTerrainMaterialLayers]{};
        QuantizeTerrainTexelWeights(weights, layerCount, quantized);
        const int x = texel % width;
        const int z = texel / width;
        for (int map = 0; map < maps; ++map)
        {
            const std::size_t offset = static_cast<std::size_t>(
                ((map * height + z) * width + x) * kTerrainWeightsPerMap);
            const int base = map * kTerrainWeightsPerMap;
            for (int channel = 0; channel < kTerrainWeightsPerMap; ++channel)
            {
                int value = quantized[base + channel];
                if (value < 0)
                {
                    value = 0;
                }
                if (value > 255)
                {
                    value = 255;
                }
                out[offset + static_cast<std::size_t>(channel)] = static_cast<unsigned char>(value);
            }
        }
    }
}

inline float LegacyTerrainPaintWeight(const float* legacy, int resolutionX, int x, int z, int layer)
{
    const int sample = z * resolutionX + x;
    return legacy[sample * kLegacyTerrainPaintLayers + layer];
}

inline void MigrateLegacyTerrainPaintWeights(TerrainSpec& terrain, const std::vector<float>& legacy)
{
    const int samples = TerrainSampleCount(terrain);
    const int expected = samples * kLegacyTerrainPaintLayers;
    terrain.weightResolutionX = kDefaultTerrainWeightResolutionX;
    terrain.weightResolutionZ = kDefaultTerrainWeightResolutionZ;
    if (static_cast<int>(legacy.size()) != expected || samples <= 0)
    {
        terrain.materialWeights.clear();
        return;
    }
    EnsureTerrainMaterialWeights(terrain);
    const float geoSpanX = static_cast<float>(terrain.resolutionX - 1);
    const float geoSpanZ = static_cast<float>(terrain.resolutionZ - 1);
    const float weightSpanX = static_cast<float>(terrain.weightResolutionX - 1);
    const float weightSpanZ = static_cast<float>(terrain.weightResolutionZ - 1);
    for (int iz = 0; iz < terrain.weightResolutionZ; ++iz)
    {
        for (int ix = 0; ix < terrain.weightResolutionX; ++ix)
        {
            const float gx = weightSpanX > 0.0f
                ? (static_cast<float>(ix) / weightSpanX) * geoSpanX
                : 0.0f;
            const float gz = weightSpanZ > 0.0f
                ? (static_cast<float>(iz) / weightSpanZ) * geoSpanZ
                : 0.0f;
            const int x0 = static_cast<int>(std::floor(gx));
            const int z0 = static_cast<int>(std::floor(gz));
            const int x1 = x0 + 1 < terrain.resolutionX ? x0 + 1 : terrain.resolutionX - 1;
            const int z1 = z0 + 1 < terrain.resolutionZ ? z0 + 1 : terrain.resolutionZ - 1;
            const float tx = gx - static_cast<float>(x0);
            const float tz = gz - static_cast<float>(z0);
            float weights[kMaxTerrainMaterialLayers]{};
            for (int layer = 0; layer < kLegacyTerrainPaintLayers; ++layer)
            {
                const float v00 = LegacyTerrainPaintWeight(legacy.data(), terrain.resolutionX, x0, z0, layer);
                const float v10 = LegacyTerrainPaintWeight(legacy.data(), terrain.resolutionX, x1, z0, layer);
                const float v01 = LegacyTerrainPaintWeight(legacy.data(), terrain.resolutionX, x0, z1, layer);
                const float v11 = LegacyTerrainPaintWeight(legacy.data(), terrain.resolutionX, x1, z1, layer);
                const float alongX0 = v00 + (v10 - v00) * tx;
                const float alongX1 = v01 + (v11 - v01) * tx;
                weights[layer] = alongX0 + (alongX1 - alongX0) * tz;
            }
            WriteTerrainTexelWeights(
                terrain, TerrainWeightTexelIndex(terrain, ix, iz), weights);
        }
    }
    CompactDefaultTerrainMaterialWeights(terrain);
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
        || TerrainReferencesAlbedoIdentity(terrain, identity))
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
    if (!TerrainMaterialWeightsAreDefault(terrain))
    {
        EnsureTerrainMaterialWeights(terrain);
        const int texels = TerrainWeightTexelCount(terrain);
        for (int texel = 0; texel < texels; ++texel)
        {
            float weights[kMaxTerrainMaterialLayers];
            ReadTerrainTexelWeights(terrain, texel, weights);
            weights[0] += weights[layerIndex];
            for (int layer = layerIndex; layer < kMaxTerrainMaterialLayers - 1; ++layer)
            {
                weights[layer] = weights[layer + 1];
            }
            weights[kMaxTerrainMaterialLayers - 1] = 0.0f;
            WriteTerrainTexelWeights(terrain, texel, weights);
        }
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

inline bool TryAssignTerrainLayerNormal(TerrainSpec& terrain, int layerIndex, std::string_view identity)
{
    if (!assets::RuntimePngIdentityIsValid(identity))
    {
        return false;
    }
    if (layerIndex <= 0)
    {
        if (terrain.normalIdentity == identity)
        {
            return false;
        }
        terrain.normalIdentity = std::string(identity);
        return true;
    }
    const int extra = layerIndex - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size()))
    {
        return false;
    }
    std::string& stored = terrain.extraLayers[static_cast<std::size_t>(extra)].normalIdentity;
    if (stored == identity)
    {
        return false;
    }
    stored = std::string(identity);
    return true;
}

inline bool TryClearTerrainLayerNormal(TerrainSpec& terrain, int layerIndex)
{
    if (layerIndex <= 0)
    {
        if (terrain.normalIdentity.empty())
        {
            return false;
        }
        terrain.normalIdentity.clear();
        return true;
    }
    const int extra = layerIndex - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size())
        || terrain.extraLayers[static_cast<std::size_t>(extra)].normalIdentity.empty())
    {
        return false;
    }
    terrain.extraLayers[static_cast<std::size_t>(extra)].normalIdentity.clear();
    return true;
}

inline bool TryAssignTerrainLayerRoughness(
    TerrainSpec& terrain,
    int layerIndex,
    std::string_view identity)
{
    if (!assets::RuntimePngIdentityIsValid(identity))
    {
        return false;
    }
    if (layerIndex <= 0)
    {
        if (terrain.roughnessIdentity == identity)
        {
            return false;
        }
        terrain.roughnessIdentity = std::string(identity);
        return true;
    }
    const int extra = layerIndex - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size()))
    {
        return false;
    }
    std::string& stored = terrain.extraLayers[static_cast<std::size_t>(extra)].roughnessIdentity;
    if (stored == identity)
    {
        return false;
    }
    stored = std::string(identity);
    return true;
}

inline bool TryClearTerrainLayerRoughness(TerrainSpec& terrain, int layerIndex)
{
    if (layerIndex <= 0)
    {
        if (terrain.roughnessIdentity.empty())
        {
            return false;
        }
        terrain.roughnessIdentity.clear();
        return true;
    }
    const int extra = layerIndex - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size())
        || terrain.extraLayers[static_cast<std::size_t>(extra)].roughnessIdentity.empty())
    {
        return false;
    }
    terrain.extraLayers[static_cast<std::size_t>(extra)].roughnessIdentity.clear();
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
