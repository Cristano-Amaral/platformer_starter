#pragma once

// Milestone 86/87/88: optional singleton authored Terrain. Regular XZ
// heightfield. Not a repeatable prop category, tile set, GUID, or generic
// mesh editor. M87 sculpts these authored heights[] only; brush parameters
// are not Level data. M88 adds one optional base surface texture identity
// plus planar XZ tiling. Not painted layers, splat maps, or PBR authoring.
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
// Texture mapping (M88) is planar on the regular XZ grid and independent of
// sample heights:
//   u = (worldX - origin.x) * textureTiling
//   v = (worldZ - origin.z) * textureTiling
// textureTiling is repeats per world unit. Empty textureIdentity is the
// solid-color fallback. Identity is textures/<file>.png; never an absolute
// path.

#include "assets/RuntimePng.h"
#include "core/Vec3.h"

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

struct TerrainSpec
{
    bool enabled = kDefaultTerrainEnabled;
    core::Vec3 origin = kDefaultTerrainOrigin;
    float sizeX = kDefaultTerrainSizeX;
    float sizeZ = kDefaultTerrainSizeZ;
    int resolutionX = kDefaultTerrainResolutionX;
    int resolutionZ = kDefaultTerrainResolutionZ;
    std::vector<float> heights{};
    std::string textureIdentity{};
    float textureTiling = kDefaultTerrainTextureTiling;
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
    return 1 + terrain.resolutionZ + (TerrainMaterialRecordShouldWrite(terrain) ? 1 : 0);
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
        || !TerrainTextureTilingIsValid(terrain.textureTiling))
    {
        return false;
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
        && a.textureIdentity == b.textureIdentity;
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

inline bool TryAssignTerrainTextureIdentity(TerrainSpec& terrain, std::string_view identity)
{
    if (!assets::RuntimePngIdentityIsValid(identity) || terrain.textureIdentity == identity)
    {
        return false;
    }
    terrain.textureIdentity = std::string(identity);
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
