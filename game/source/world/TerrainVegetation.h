#pragma once

// Milestone 93: authored Terrain vegetation. The palette and occupancy grid
// are Level data. Instance position, yaw, scale, and terrain alignment are
// derived from that data plus the current heightfield. Nothing here is a
// GPU resource, a Static Prop, or a GUID.

#include "world/Terrain.h"
#include "world/TerrainGeometry.h"
#include "world/TerrainSculpt.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace world
{
inline constexpr float kDefaultTerrainVegetationRadius = kDefaultTerrainSculptRadius;
inline constexpr float kMinTerrainVegetationRadius = kMinTerrainSculptRadius;
inline constexpr float kMaxTerrainVegetationRadius = kMaxTerrainSculptRadius;
inline constexpr float kTerrainVegetationStampSpacingFactor = kTerrainSculptStampSpacingFactor;

enum class TerrainVegetationBrushOperation
{
    Paint,
    Erase,
};

inline float SanitizeTerrainVegetationRadius(float radius)
{
    return SanitizeTerrainSculptRadius(radius);
}

inline float TerrainVegetationStampSpacing(float radius)
{
    return SanitizeTerrainVegetationRadius(radius) * kTerrainVegetationStampSpacingFactor;
}

inline float TerrainVegetationCellSizeX(const TerrainSpec& terrain)
{
    if (terrain.vegetationResolutionX <= 0)
    {
        return 0.0f;
    }
    return terrain.sizeX / static_cast<float>(terrain.vegetationResolutionX);
}

inline float TerrainVegetationCellSizeZ(const TerrainSpec& terrain)
{
    if (terrain.vegetationResolutionZ <= 0)
    {
        return 0.0f;
    }
    return terrain.sizeZ / static_cast<float>(terrain.vegetationResolutionZ);
}

inline void ClearTerrainVegetation(TerrainSpec& terrain)
{
    terrain.vegetationResolutionX = 0;
    terrain.vegetationResolutionZ = 0;
    terrain.vegetationSeed = kDefaultTerrainVegetationSeed;
    terrain.vegetationEntries.clear();
    terrain.vegetationCells.clear();
    terrain.vegetationDensityQuanta.clear();
    terrain.vegetationPaintParams.clear();
}

inline bool EnsureTerrainVegetationGrid(TerrainSpec& terrain)
{
    if (!TerrainVegetationResolutionIsValid(terrain.vegetationResolutionX)
        || !TerrainVegetationResolutionIsValid(terrain.vegetationResolutionZ))
    {
        terrain.vegetationResolutionX = kDefaultTerrainVegetationResolutionX;
        terrain.vegetationResolutionZ = kDefaultTerrainVegetationResolutionZ;
        terrain.vegetationSeed = kDefaultTerrainVegetationSeed;
        const int cells = terrain.vegetationResolutionX * terrain.vegetationResolutionZ;
        terrain.vegetationCells.assign(static_cast<std::size_t>(cells), 0);
        terrain.vegetationDensityQuanta.assign(
            static_cast<std::size_t>(cells * kMaxTerrainVegetationEntries), 0);
        terrain.vegetationPaintParams.assign(
            static_cast<std::size_t>(cells * kMaxTerrainVegetationEntries), 0);
        return true;
    }
    const int cells = terrain.vegetationResolutionX * terrain.vegetationResolutionZ;
    const int quanta = cells * kMaxTerrainVegetationEntries;
    if (static_cast<int>(terrain.vegetationCells.size()) != cells)
    {
        terrain.vegetationCells.assign(static_cast<std::size_t>(cells), 0);
        terrain.vegetationDensityQuanta.assign(static_cast<std::size_t>(quanta), 0);
        terrain.vegetationPaintParams.assign(static_cast<std::size_t>(quanta), 0);
    }
    else
    {
        if (static_cast<int>(terrain.vegetationDensityQuanta.size()) != quanta)
        {
            terrain.vegetationDensityQuanta.assign(static_cast<std::size_t>(quanta), 0);
        }
        if (static_cast<int>(terrain.vegetationPaintParams.size()) != quanta)
        {
            terrain.vegetationPaintParams.assign(static_cast<std::size_t>(quanta), 0);
        }
    }
    return true;
}

inline bool TryAddTerrainVegetationEntry(TerrainSpec& terrain, std::string_view modelIdentity)
{
    if (!TerrainSpecIsValid(terrain)
        || static_cast<int>(terrain.vegetationEntries.size()) >= kMaxTerrainVegetationEntries
        || !TerrainVegetationModelIdentityIsValid(modelIdentity))
    {
        return false;
    }
    EnsureTerrainVegetationGrid(terrain);
    TerrainVegetationEntry entry{};
    entry.modelIdentity = std::string(modelIdentity);
    terrain.vegetationEntries.push_back(std::move(entry));
    return TerrainVegetationDataIsValid(terrain);
}

inline bool TryRemoveTerrainVegetationEntry(TerrainSpec& terrain, int entryIndex)
{
    if (entryIndex < 0 || entryIndex >= static_cast<int>(terrain.vegetationEntries.size()))
    {
        return false;
    }
    const unsigned char removedBit = TerrainVegetationEntryBit(entryIndex);
    const unsigned char lowMask = static_cast<unsigned char>(removedBit - 1u);
    const int stride = kMaxTerrainVegetationEntries;
    const bool quantaMatch = terrain.vegetationDensityQuanta.size()
        == terrain.vegetationCells.size() * static_cast<std::size_t>(stride);
    const bool paintMatch = terrain.vegetationPaintParams.size()
        == terrain.vegetationCells.size() * static_cast<std::size_t>(stride);
    for (std::size_t cellIndex = 0; cellIndex < terrain.vegetationCells.size(); ++cellIndex)
    {
        unsigned char& cell = terrain.vegetationCells[cellIndex];
        const unsigned char low = static_cast<unsigned char>(cell & lowMask);
        const unsigned char high = static_cast<unsigned char>(
            cell & static_cast<unsigned char>(~(removedBit | lowMask)));
        cell = static_cast<unsigned char>(low | static_cast<unsigned char>(high >> 1));
        const std::size_t base = cellIndex * static_cast<std::size_t>(stride);
        if (quantaMatch)
        {
            for (int slot = entryIndex; slot < stride - 1; ++slot)
            {
                terrain.vegetationDensityQuanta[base + static_cast<std::size_t>(slot)] =
                    terrain.vegetationDensityQuanta[base + static_cast<std::size_t>(slot + 1)];
            }
            terrain.vegetationDensityQuanta[base + static_cast<std::size_t>(stride - 1)] = 0;
        }
        if (paintMatch)
        {
            for (int slot = entryIndex; slot < stride - 1; ++slot)
            {
                terrain.vegetationPaintParams[base + static_cast<std::size_t>(slot)] =
                    terrain.vegetationPaintParams[base + static_cast<std::size_t>(slot + 1)];
            }
            terrain.vegetationPaintParams[base + static_cast<std::size_t>(stride - 1)] = 0;
        }
    }
    terrain.vegetationEntries.erase(
        terrain.vegetationEntries.begin() + static_cast<std::ptrdiff_t>(entryIndex));
    if (terrain.vegetationEntries.empty())
    {
        ClearTerrainVegetation(terrain);
    }
    return TerrainVegetationDataIsValid(terrain);
}

inline bool TrySetTerrainVegetationDensity(TerrainSpec& terrain, int entryIndex, float density)
{
    if (!TerrainVegetationDensityIsValid(density) || entryIndex < 0
        || entryIndex >= static_cast<int>(terrain.vegetationEntries.size()))
    {
        return false;
    }
    TerrainVegetationEntry& entry = terrain.vegetationEntries[static_cast<std::size_t>(entryIndex)];
    if (entry.density == density)
    {
        return false;
    }
    entry.density = density;
    return true;
}

inline bool TrySetTerrainVegetationScaleRange(
    TerrainSpec& terrain,
    int entryIndex,
    float minScale,
    float maxScale)
{
    if (!TerrainVegetationScaleIsValid(minScale) || !TerrainVegetationScaleIsValid(maxScale)
        || minScale > maxScale || entryIndex < 0
        || entryIndex >= static_cast<int>(terrain.vegetationEntries.size()))
    {
        return false;
    }
    TerrainVegetationEntry& entry = terrain.vegetationEntries[static_cast<std::size_t>(entryIndex)];
    if (entry.minScale == minScale && entry.maxScale == maxScale)
    {
        return false;
    }
    entry.minScale = minScale;
    entry.maxScale = maxScale;
    return true;
}

struct TerrainVegetationStampRequest
{
    TerrainVegetationBrushOperation operation = TerrainVegetationBrushOperation::Paint;
    int entryIndex = 0;
    float centerX = 0.0f;
    float centerZ = 0.0f;
    float radius = kDefaultTerrainVegetationRadius;
};

inline bool TerrainVegetationCenterIsInside(const TerrainSpec& terrain, float centerX, float centerZ)
{
    return centerX >= terrain.origin.x && centerZ >= terrain.origin.z
        && centerX <= terrain.origin.x + terrain.sizeX && centerZ <= terrain.origin.z + terrain.sizeZ;
}

inline bool ApplyTerrainVegetationStamp(
    TerrainSpec& terrain,
    const TerrainVegetationStampRequest& request)
{
    if (!TerrainSpecIsValid(terrain) || !terrain.enabled || !TerrainVegetationShouldWrite(terrain))
    {
        return false;
    }
    if (!TerrainVegetationCenterIsInside(terrain, request.centerX, request.centerZ))
    {
        return false;
    }
    if (request.entryIndex < 0
        || request.entryIndex >= static_cast<int>(terrain.vegetationEntries.size()))
    {
        return false;
    }

    const float radius = SanitizeTerrainVegetationRadius(request.radius);
    if (!(radius > 0.0f))
    {
        return false;
    }

    const float cellSizeX = TerrainVegetationCellSizeX(terrain);
    const float cellSizeZ = TerrainVegetationCellSizeZ(terrain);
    if (!(cellSizeX > 0.0f) || !(cellSizeZ > 0.0f))
    {
        return false;
    }

    const unsigned char bit = TerrainVegetationEntryBit(request.entryIndex);
    const bool erase = request.operation == TerrainVegetationBrushOperation::Erase;
    bool changed = false;
    for (int iz = 0; iz < terrain.vegetationResolutionZ; ++iz)
    {
        for (int ix = 0; ix < terrain.vegetationResolutionX; ++ix)
        {
            const float cellX = terrain.origin.x + (static_cast<float>(ix) + 0.5f) * cellSizeX;
            const float cellZ = terrain.origin.z + (static_cast<float>(iz) + 0.5f) * cellSizeZ;
            const float falloff = TerrainSculptFalloff(
                TerrainSculptDistanceXZ(cellX, cellZ, request.centerX, request.centerZ),
                radius);
            if (!(falloff > 0.0f))
            {
                continue;
            }
            const int index = TerrainVegetationCellIndex(terrain, ix, iz);
            const int densitySlot = TerrainVegetationDensitySlot(index, request.entryIndex);
            if (index < 0 || densitySlot < 0
                || densitySlot >= static_cast<int>(terrain.vegetationDensityQuanta.size())
                || densitySlot >= static_cast<int>(terrain.vegetationPaintParams.size()))
            {
                continue;
            }
            unsigned char& cell = terrain.vegetationCells[static_cast<std::size_t>(index)];
            unsigned char& quantum =
                terrain.vegetationDensityQuanta[static_cast<std::size_t>(densitySlot)];
            std::uint16_t& paint = terrain.vegetationPaintParams[static_cast<std::size_t>(densitySlot)];
            if (erase)
            {
                if ((cell & bit) != 0)
                {
                    cell = static_cast<unsigned char>(cell & static_cast<unsigned char>(~bit));
                    changed = true;
                }
                if (quantum != 0)
                {
                    quantum = 0;
                    changed = true;
                }
                if (paint != 0)
                {
                    paint = 0;
                    changed = true;
                }
            }
            else
            {
                const TerrainVegetationEntry& entry =
                    terrain.vegetationEntries[static_cast<std::size_t>(request.entryIndex)];
                const unsigned char painted = QuantizeTerrainVegetationDensity(entry.density);
                const std::uint16_t paintedStyle = PackTerrainVegetationPaintFromEntry(entry);
                if ((cell & bit) == 0 || quantum != painted || paint != paintedStyle)
                {
                    cell = static_cast<unsigned char>(cell | bit);
                    quantum = painted;
                    paint = paintedStyle;
                    changed = true;
                }
            }
        }
    }
    return changed;
}

struct TerrainVegetationStroke
{
    bool active = false;
    float lastStampX = 0.0f;
    float lastStampZ = 0.0f;
};

inline void EndTerrainVegetationStroke(TerrainVegetationStroke& stroke)
{
    stroke.active = false;
}

inline bool BeginTerrainVegetationStroke(
    TerrainVegetationStroke& stroke,
    TerrainSpec& terrain,
    TerrainVegetationStampRequest request)
{
    stroke.active = true;
    stroke.lastStampX = request.centerX;
    stroke.lastStampZ = request.centerZ;
    return ApplyTerrainVegetationStamp(terrain, request);
}

inline bool ContinueTerrainVegetationStroke(
    TerrainVegetationStroke& stroke,
    TerrainSpec& terrain,
    TerrainVegetationStampRequest request,
    float cursorX,
    float cursorZ)
{
    if (!stroke.active)
    {
        return false;
    }
    const float spacing = TerrainVegetationStampSpacing(request.radius);
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
        changed = ApplyTerrainVegetationStamp(terrain, request) || changed;
        dx = cursorX - stroke.lastStampX;
        dz = cursorZ - stroke.lastStampZ;
        remaining = std::sqrt(dx * dx + dz * dz);
    }
    return changed;
}

inline std::uint32_t TerrainVegetationMix(std::uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

inline std::uint32_t TerrainVegetationHash(
    std::uint32_t seed,
    int ix,
    int iz,
    int instance,
    int channel)
{
    std::uint32_t value = seed;
    value ^= static_cast<std::uint32_t>(ix) * 0x9e3779b9u;
    value ^= static_cast<std::uint32_t>(iz) * 0x85ebca6bu;
    value ^= static_cast<std::uint32_t>(instance + 1) * 0xc2b2ae35u;
    value ^= static_cast<std::uint32_t>(channel + 1) * 0x27d4eb2fu;
    return TerrainVegetationMix(value);
}

inline float TerrainVegetationHash01(
    std::uint32_t seed,
    int ix,
    int iz,
    int instance,
    int channel)
{
    return static_cast<float>(TerrainVegetationHash(seed, ix, iz, instance, channel) >> 8)
        * (1.0f / 16777216.0f);
}

inline bool SampleTerrainSurface(
    const TerrainSpec& terrain,
    float worldX,
    float worldZ,
    float& outY,
    core::Vec3& outNormal)
{
    outY = terrain.origin.y;
    outNormal = {0.0f, 1.0f, 0.0f};
    if (terrain.resolutionX < 2 || terrain.resolutionZ < 2
        || static_cast<int>(terrain.heights.size()) != TerrainSampleCount(terrain))
    {
        return false;
    }
    const float spanX = TerrainSampleSpacingX(terrain);
    const float spanZ = TerrainSampleSpacingZ(terrain);
    if (!(spanX > 0.0f) || !(spanZ > 0.0f))
    {
        return false;
    }
    float gx = (worldX - terrain.origin.x) / spanX;
    float gz = (worldZ - terrain.origin.z) / spanZ;
    const float maxX = static_cast<float>(terrain.resolutionX - 1);
    const float maxZ = static_cast<float>(terrain.resolutionZ - 1);
    if (gx < -1.0e-3f || gz < -1.0e-3f || gx > maxX + 1.0e-3f || gz > maxZ + 1.0e-3f)
    {
        return false;
    }
    if (gx < 0.0f)
    {
        gx = 0.0f;
    }
    if (gz < 0.0f)
    {
        gz = 0.0f;
    }
    if (gx > maxX)
    {
        gx = maxX;
    }
    if (gz > maxZ)
    {
        gz = maxZ;
    }
    int x0 = static_cast<int>(std::floor(gx));
    int z0 = static_cast<int>(std::floor(gz));
    if (x0 >= terrain.resolutionX - 1)
    {
        x0 = terrain.resolutionX - 2;
    }
    if (z0 >= terrain.resolutionZ - 1)
    {
        z0 = terrain.resolutionZ - 2;
    }
    const float u = gx - static_cast<float>(x0);
    const float v = gz - static_cast<float>(z0);
    const core::Vec3 p00 = TerrainSamplePosition(terrain, x0, z0);
    const core::Vec3 p10 = TerrainSamplePosition(terrain, x0 + 1, z0);
    const core::Vec3 p01 = TerrainSamplePosition(terrain, x0, z0 + 1);
    const core::Vec3 p11 = TerrainSamplePosition(terrain, x0 + 1, z0 + 1);
    core::Vec3 point{};
    core::Vec3 normal{};
    if (u + v <= 1.0f)
    {
        const float w00 = 1.0f - u - v;
        point = {
            p00.x * w00 + p01.x * v + p10.x * u,
            p00.y * w00 + p01.y * v + p10.y * u,
            p00.z * w00 + p01.z * v + p10.z * u};
        normal = TerrainCross(TerrainSub(p01, p00), TerrainSub(p10, p00));
    }
    else
    {
        const core::Vec3 edge01 = TerrainSub(p01, p10);
        const core::Vec3 edge11 = TerrainSub(p11, p10);
        const float dot00 = TerrainDot(edge01, edge01);
        const float dot01 = TerrainDot(edge01, edge11);
        const float dot11 = TerrainDot(edge11, edge11);
        const core::Vec3 local = {worldX - p10.x, 0.0f, worldZ - p10.z};
        const core::Vec3 planar{local.x, 0.0f, local.z};
        const float dot02 = edge01.x * planar.x + edge01.z * planar.z;
        const float dot12 = edge11.x * planar.x + edge11.z * planar.z;
        const float denom = dot00 * dot11 - dot01 * dot01;
        float bary01 = 0.0f;
        float bary11 = 0.0f;
        if (std::fabs(denom) > 1.0e-8f)
        {
            bary01 = (dot11 * dot02 - dot01 * dot12) / denom;
            bary11 = (dot00 * dot12 - dot01 * dot02) / denom;
        }
        const float bary10 = 1.0f - bary01 - bary11;
        point = {
            p10.x * bary10 + p01.x * bary01 + p11.x * bary11,
            p10.y * bary10 + p01.y * bary01 + p11.y * bary11,
            p10.z * bary10 + p01.z * bary01 + p11.z * bary11};
        normal = TerrainCross(TerrainSub(p01, p10), TerrainSub(p11, p10));
    }
    if (!TerrainVecFinite(point))
    {
        return false;
    }
    outY = point.y;
    outNormal = TerrainNormalizeOrUp(normal);
    return TerrainVecFinite(outNormal);
}

struct TerrainVegetationInstance
{
    int entryIndex = 0;
    core::Vec3 position{};
    core::Vec3 axisX{1.0f, 0.0f, 0.0f};
    core::Vec3 axisY{0.0f, 1.0f, 0.0f};
    core::Vec3 axisZ{0.0f, 0.0f, 1.0f};
    float uniformScale = 1.0f;
};

inline void TerrainVegetationBasis(
    bool alignToNormal,
    float yawDegrees,
    core::Vec3 normal,
    core::Vec3& axisX,
    core::Vec3& axisY,
    core::Vec3& axisZ)
{
    const float yaw = yawDegrees * 0.017453292519943295f;
    const float cosine = std::cos(yaw);
    const float sine = std::sin(yaw);
    if (!alignToNormal)
    {
        axisX = {cosine, 0.0f, -sine};
        axisY = {0.0f, 1.0f, 0.0f};
        axisZ = {sine, 0.0f, cosine};
        return;
    }

    const core::Vec3 up = TerrainNormalizeOrUp(normal);
    auto project = [](core::Vec3 hint, core::Vec3 onto) {
        const float along = TerrainDot(hint, onto);
        return TerrainSub(hint, {onto.x * along, onto.y * along, onto.z * along});
    };
    core::Vec3 forward0 = project({0.0f, 0.0f, 1.0f}, up);
    if (!(TerrainDot(forward0, forward0) > 1.0e-8f))
    {
        forward0 = project({1.0f, 0.0f, 0.0f}, up);
    }
    forward0 = TerrainNormalizeOrUp(forward0);
    core::Vec3 right0 = TerrainNormalizeOrUp(TerrainCross(up, forward0));
    core::Vec3 forward = {
        forward0.x * cosine + right0.x * sine,
        forward0.y * cosine + right0.y * sine,
        forward0.z * cosine + right0.z * sine};
    forward = TerrainNormalizeOrUp(forward);
    axisX = TerrainNormalizeOrUp(TerrainCross(up, forward));
    axisY = up;
    axisZ = forward;
}

inline int TerrainVegetationInstanceCountForCell(
    const TerrainSpec& terrain,
    float density,
    int ix,
    int iz)
{
    const float area = TerrainVegetationCellSizeX(terrain) * TerrainVegetationCellSizeZ(terrain);
    const float expected = density * area;
    if (!(expected > 0.0f) || !std::isfinite(expected))
    {
        return 0;
    }
    int whole = static_cast<int>(std::floor(expected));
    if (whole >= kMaxTerrainVegetationInstancesPerCell)
    {
        return kMaxTerrainVegetationInstancesPerCell;
    }
    const float fraction = expected - static_cast<float>(whole);
    if (TerrainVegetationHash01(terrain.vegetationSeed, ix, iz, 0, 0) < fraction)
    {
        ++whole;
    }
    if (whole > kMaxTerrainVegetationInstancesPerCell)
    {
        whole = kMaxTerrainVegetationInstancesPerCell;
    }
    return whole < 0 ? 0 : whole;
}

inline void BuildTerrainVegetationInstances(
    const TerrainSpec& terrain,
    std::vector<TerrainVegetationInstance>& out)
{
    out.clear();
    if (!terrain.enabled || !TerrainVegetationDataIsValid(terrain) || !TerrainVegetationShouldWrite(terrain))
    {
        return;
    }
    const int entryCount = static_cast<int>(terrain.vegetationEntries.size());
    out.reserve(static_cast<std::size_t>(
        terrain.vegetationResolutionX * terrain.vegetationResolutionZ));
    for (int entryIndex = 0; entryIndex < entryCount; ++entryIndex)
    {
        const int channelBase = entryIndex * 8;
        for (int iz = 0; iz < terrain.vegetationResolutionZ; ++iz)
        {
            for (int ix = 0; ix < terrain.vegetationResolutionX; ++ix)
            {
                const unsigned char cell = terrain.vegetationCells[static_cast<std::size_t>(
                    TerrainVegetationCellIndex(terrain, ix, iz))];
                if (!TerrainVegetationCellHasEntry(cell, entryIndex))
                {
                    continue;
                }
                const int cellIndex = TerrainVegetationCellIndex(terrain, ix, iz);
                const int densitySlot = TerrainVegetationDensitySlot(cellIndex, entryIndex);
                if (densitySlot < 0
                    || densitySlot >= static_cast<int>(terrain.vegetationDensityQuanta.size())
                    || densitySlot >= static_cast<int>(terrain.vegetationPaintParams.size()))
                {
                    continue;
                }
                const float density = DequantizeTerrainVegetationDensity(
                    terrain.vegetationDensityQuanta[static_cast<std::size_t>(densitySlot)]);
                const std::uint16_t paint =
                    terrain.vegetationPaintParams[static_cast<std::size_t>(densitySlot)];
                const float minScale =
                    DequantizeTerrainVegetationScale(TerrainVegetationPaintMinScaleQuantum(paint));
                const float maxScale =
                    DequantizeTerrainVegetationScale(TerrainVegetationPaintMaxScaleQuantum(paint));
                const bool randomYaw = TerrainVegetationPaintRandomYaw(paint);
                const bool alignToNormal = TerrainVegetationPaintAlignToNormal(paint);
                const int count = TerrainVegetationInstanceCountForCell(terrain, density, ix, iz);
                for (int instance = 0; instance < count; ++instance)
                {
                    const float alongX = 0.2f
                        + 0.6f
                            * TerrainVegetationHash01(
                                terrain.vegetationSeed, ix, iz, instance, channelBase + 1);
                    const float alongZ = 0.2f
                        + 0.6f
                            * TerrainVegetationHash01(
                                terrain.vegetationSeed, ix, iz, instance, channelBase + 2);
                    const float worldX = terrain.origin.x
                        + (static_cast<float>(ix) + alongX) * TerrainVegetationCellSizeX(terrain);
                    const float worldZ = terrain.origin.z
                        + (static_cast<float>(iz) + alongZ) * TerrainVegetationCellSizeZ(terrain);
                    float worldY = terrain.origin.y;
                    core::Vec3 normal{0.0f, 1.0f, 0.0f};
                    if (!SampleTerrainSurface(terrain, worldX, worldZ, worldY, normal))
                    {
                        continue;
                    }
                    const float scaleT = minScale == maxScale
                        ? 0.0f
                        : TerrainVegetationHash01(
                              terrain.vegetationSeed, ix, iz, instance, channelBase + 3);
                    const float yaw = randomYaw
                        ? TerrainVegetationHash01(
                              terrain.vegetationSeed, ix, iz, instance, channelBase + 4)
                            * 360.0f
                        : 0.0f;
                    TerrainVegetationInstance derived{};
                    derived.entryIndex = entryIndex;
                    derived.position = {worldX, worldY, worldZ};
                    derived.uniformScale = minScale + (maxScale - minScale) * scaleT;
                    TerrainVegetationBasis(
                        alignToNormal, yaw, normal, derived.axisX, derived.axisY, derived.axisZ);
                    out.push_back(derived);
                }
            }
        }
    }
}

struct TerrainVegetationDrawBatch
{
    int entryIndex = 0;
    std::size_t begin = 0;
    std::size_t count = 0;
};

inline void BuildTerrainVegetationDrawBatches(
    const std::vector<TerrainVegetationInstance>& instances,
    std::vector<TerrainVegetationDrawBatch>& out)
{
    out.clear();
    for (std::size_t index = 0; index < instances.size();)
    {
        const int entryIndex = instances[index].entryIndex;
        std::size_t end = index + 1;
        while (end < instances.size() && instances[end].entryIndex == entryIndex)
        {
            ++end;
        }
        out.push_back(TerrainVegetationDrawBatch{entryIndex, index, end - index});
        index = end;
    }
}

inline std::uint64_t TerrainVegetationDeriveSignature(const TerrainSpec& terrain)
{
    std::uint64_t hash = 14695981039346656037ull;
    const auto mix = [&](std::uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ull;
    };
    const auto mixFloat = [&](float value) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        mix(bits);
    };
    mix(terrain.enabled ? 1u : 0u);
    mix(terrain.vegetationSeed);
    mix(static_cast<std::uint64_t>(terrain.vegetationResolutionX));
    mix(static_cast<std::uint64_t>(terrain.vegetationResolutionZ));
    mix(static_cast<std::uint64_t>(terrain.resolutionX));
    mix(static_cast<std::uint64_t>(terrain.resolutionZ));
    mixFloat(terrain.origin.x);
    mixFloat(terrain.origin.y);
    mixFloat(terrain.origin.z);
    mixFloat(terrain.sizeX);
    mixFloat(terrain.sizeZ);
    for (float height : terrain.heights)
    {
        mixFloat(height);
    }
    for (unsigned char cell : terrain.vegetationCells)
    {
        mix(cell);
    }
    for (unsigned char quantum : terrain.vegetationDensityQuanta)
    {
        mix(quantum);
    }
    for (std::uint16_t paint : terrain.vegetationPaintParams)
    {
        mix(paint);
    }
    for (const TerrainVegetationEntry& entry : terrain.vegetationEntries)
    {
        for (unsigned char byte : entry.modelIdentity)
        {
            mix(byte);
        }
    }
    return hash;
}

inline char TerrainVegetationHexDigit(int value)
{
    return TerrainWeightHexDigit(value & 15);
}
}
