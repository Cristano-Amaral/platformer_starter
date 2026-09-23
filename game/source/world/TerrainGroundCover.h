#pragma once

// Milestone 96: authored Terrain grass / ground cover. Palette + density grid
// are Level data. Derived crossed-card instances are transient. This is not
// M93 vegetation occupancy, a Static Prop, a GUID, or a GPU resource.

#include "world/Terrain.h"
#include "world/TerrainVegetation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace world
{
inline constexpr float kDefaultTerrainGroundCoverRadius = kDefaultTerrainSculptRadius;
inline constexpr float kMinTerrainGroundCoverRadius = kMinTerrainSculptRadius;
inline constexpr float kMaxTerrainGroundCoverRadius = kMaxTerrainSculptRadius;
inline constexpr float kTerrainGroundCoverStampSpacingFactor = kTerrainSculptStampSpacingFactor;

enum class TerrainGroundCoverBrushOperation
{
    Paint,
    Erase,
};

inline float SanitizeTerrainGroundCoverRadius(float radius)
{
    return SanitizeTerrainSculptRadius(radius);
}

inline float TerrainGroundCoverStampSpacing(float radius)
{
    return SanitizeTerrainGroundCoverRadius(radius) * kTerrainGroundCoverStampSpacingFactor;
}

inline float TerrainGroundCoverCellSizeX(const TerrainSpec& terrain)
{
    if (terrain.groundCoverResolutionX <= 0)
    {
        return 0.0f;
    }
    return terrain.sizeX / static_cast<float>(terrain.groundCoverResolutionX);
}

inline float TerrainGroundCoverCellSizeZ(const TerrainSpec& terrain)
{
    if (terrain.groundCoverResolutionZ <= 0)
    {
        return 0.0f;
    }
    return terrain.sizeZ / static_cast<float>(terrain.groundCoverResolutionZ);
}

inline void ClearTerrainGroundCover(TerrainSpec& terrain)
{
    terrain.groundCoverResolutionX = 0;
    terrain.groundCoverResolutionZ = 0;
    terrain.groundCoverSeed = kDefaultTerrainGroundCoverSeed;
    terrain.groundCoverEntries.clear();
    terrain.groundCoverCells.clear();
    terrain.groundCoverDensityQuanta.clear();
    terrain.groundCoverPaintParams.clear();
}

inline bool EnsureTerrainGroundCoverGrid(TerrainSpec& terrain)
{
    if (!TerrainGroundCoverResolutionIsValid(terrain.groundCoverResolutionX)
        || !TerrainGroundCoverResolutionIsValid(terrain.groundCoverResolutionZ))
    {
        terrain.groundCoverResolutionX = kDefaultTerrainGroundCoverResolutionX;
        terrain.groundCoverResolutionZ = kDefaultTerrainGroundCoverResolutionZ;
        terrain.groundCoverSeed = kDefaultTerrainGroundCoverSeed;
        const int cells = terrain.groundCoverResolutionX * terrain.groundCoverResolutionZ;
        terrain.groundCoverCells.assign(static_cast<std::size_t>(cells), 0);
        terrain.groundCoverDensityQuanta.assign(
            static_cast<std::size_t>(cells * kMaxTerrainGroundCoverEntries), 0);
        terrain.groundCoverPaintParams.assign(
            static_cast<std::size_t>(cells * kMaxTerrainGroundCoverEntries), 0);
        return true;
    }
    const int cells = terrain.groundCoverResolutionX * terrain.groundCoverResolutionZ;
    const int quanta = cells * kMaxTerrainGroundCoverEntries;
    if (static_cast<int>(terrain.groundCoverCells.size()) != cells)
    {
        terrain.groundCoverCells.assign(static_cast<std::size_t>(cells), 0);
        terrain.groundCoverDensityQuanta.assign(static_cast<std::size_t>(quanta), 0);
        terrain.groundCoverPaintParams.assign(static_cast<std::size_t>(quanta), 0);
    }
    else
    {
        if (static_cast<int>(terrain.groundCoverDensityQuanta.size()) != quanta)
        {
            terrain.groundCoverDensityQuanta.assign(static_cast<std::size_t>(quanta), 0);
        }
        if (static_cast<int>(terrain.groundCoverPaintParams.size()) != quanta)
        {
            terrain.groundCoverPaintParams.assign(static_cast<std::size_t>(quanta), 0);
        }
    }
    return true;
}

inline bool TryAddTerrainGroundCoverEntry(TerrainSpec& terrain, std::string_view textureIdentity)
{
    if (!TerrainSpecIsValid(terrain)
        || static_cast<int>(terrain.groundCoverEntries.size()) >= kMaxTerrainGroundCoverEntries
        || !TerrainGroundCoverTextureIdentityIsValid(textureIdentity))
    {
        return false;
    }
    EnsureTerrainGroundCoverGrid(terrain);
    TerrainGroundCoverEntry entry{};
    entry.textureIdentity = std::string(textureIdentity);
    terrain.groundCoverEntries.push_back(std::move(entry));
    return TerrainGroundCoverDataIsValid(terrain);
}

inline bool TryRemoveTerrainGroundCoverEntry(TerrainSpec& terrain, int entryIndex)
{
    if (entryIndex < 0 || entryIndex >= static_cast<int>(terrain.groundCoverEntries.size()))
    {
        return false;
    }
    const unsigned char removedBit = TerrainGroundCoverEntryBit(entryIndex);
    const unsigned char lowMask = static_cast<unsigned char>(removedBit - 1u);
    const int stride = kMaxTerrainGroundCoverEntries;
    const bool quantaMatch = terrain.groundCoverDensityQuanta.size()
        == terrain.groundCoverCells.size() * static_cast<std::size_t>(stride);
    const bool paintMatch = terrain.groundCoverPaintParams.size()
        == terrain.groundCoverCells.size() * static_cast<std::size_t>(stride);
    for (std::size_t cellIndex = 0; cellIndex < terrain.groundCoverCells.size(); ++cellIndex)
    {
        unsigned char& cell = terrain.groundCoverCells[cellIndex];
        const unsigned char low = static_cast<unsigned char>(cell & lowMask);
        const unsigned char high = static_cast<unsigned char>(
            cell & static_cast<unsigned char>(~(removedBit | lowMask)));
        cell = static_cast<unsigned char>(low | static_cast<unsigned char>(high >> 1));
        const std::size_t base = cellIndex * static_cast<std::size_t>(stride);
        if (quantaMatch)
        {
            for (int slot = entryIndex; slot < stride - 1; ++slot)
            {
                terrain.groundCoverDensityQuanta[base + static_cast<std::size_t>(slot)] =
                    terrain.groundCoverDensityQuanta[base + static_cast<std::size_t>(slot + 1)];
            }
            terrain.groundCoverDensityQuanta[base + static_cast<std::size_t>(stride - 1)] = 0;
        }
        if (paintMatch)
        {
            for (int slot = entryIndex; slot < stride - 1; ++slot)
            {
                terrain.groundCoverPaintParams[base + static_cast<std::size_t>(slot)] =
                    terrain.groundCoverPaintParams[base + static_cast<std::size_t>(slot + 1)];
            }
            terrain.groundCoverPaintParams[base + static_cast<std::size_t>(stride - 1)] = 0;
        }
    }
    terrain.groundCoverEntries.erase(
        terrain.groundCoverEntries.begin() + static_cast<std::ptrdiff_t>(entryIndex));
    if (terrain.groundCoverEntries.empty())
    {
        ClearTerrainGroundCover(terrain);
    }
    return TerrainGroundCoverDataIsValid(terrain);
}

inline bool TrySetTerrainGroundCoverDensity(TerrainSpec& terrain, int entryIndex, float density)
{
    if (!TerrainGroundCoverDensityIsValid(density) || entryIndex < 0
        || entryIndex >= static_cast<int>(terrain.groundCoverEntries.size()))
    {
        return false;
    }
    TerrainGroundCoverEntry& entry = terrain.groundCoverEntries[static_cast<std::size_t>(entryIndex)];
    if (entry.density == density)
    {
        return false;
    }
    entry.density = density;
    return true;
}

inline bool TrySetTerrainGroundCoverWidthRange(
    TerrainSpec& terrain,
    int entryIndex,
    float minWidth,
    float maxWidth)
{
    if (!TerrainGroundCoverWidthIsValid(minWidth) || !TerrainGroundCoverWidthIsValid(maxWidth)
        || minWidth > maxWidth || entryIndex < 0
        || entryIndex >= static_cast<int>(terrain.groundCoverEntries.size()))
    {
        return false;
    }
    TerrainGroundCoverEntry& entry = terrain.groundCoverEntries[static_cast<std::size_t>(entryIndex)];
    if (entry.minWidth == minWidth && entry.maxWidth == maxWidth)
    {
        return false;
    }
    entry.minWidth = minWidth;
    entry.maxWidth = maxWidth;
    return true;
}

inline bool TrySetTerrainGroundCoverHeightRange(
    TerrainSpec& terrain,
    int entryIndex,
    float minHeight,
    float maxHeight)
{
    if (!TerrainGroundCoverHeightIsValid(minHeight) || !TerrainGroundCoverHeightIsValid(maxHeight)
        || minHeight > maxHeight || entryIndex < 0
        || entryIndex >= static_cast<int>(terrain.groundCoverEntries.size()))
    {
        return false;
    }
    TerrainGroundCoverEntry& entry = terrain.groundCoverEntries[static_cast<std::size_t>(entryIndex)];
    if (entry.minHeight == minHeight && entry.maxHeight == maxHeight)
    {
        return false;
    }
    entry.minHeight = minHeight;
    entry.maxHeight = maxHeight;
    return true;
}

struct TerrainGroundCoverStampRequest
{
    TerrainGroundCoverBrushOperation operation = TerrainGroundCoverBrushOperation::Paint;
    int entryIndex = 0;
    float centerX = 0.0f;
    float centerZ = 0.0f;
    float radius = kDefaultTerrainGroundCoverRadius;
};

inline bool TerrainGroundCoverCenterIsInside(const TerrainSpec& terrain, float centerX, float centerZ)
{
    return centerX >= terrain.origin.x && centerZ >= terrain.origin.z
        && centerX <= terrain.origin.x + terrain.sizeX && centerZ <= terrain.origin.z + terrain.sizeZ;
}

inline bool ApplyTerrainGroundCoverStamp(
    TerrainSpec& terrain,
    const TerrainGroundCoverStampRequest& request)
{
    if (!TerrainSpecIsValid(terrain) || !terrain.enabled || !TerrainGroundCoverShouldWrite(terrain))
    {
        return false;
    }
    if (!TerrainGroundCoverCenterIsInside(terrain, request.centerX, request.centerZ))
    {
        return false;
    }
    if (request.entryIndex < 0
        || request.entryIndex >= static_cast<int>(terrain.groundCoverEntries.size()))
    {
        return false;
    }

    const float radius = SanitizeTerrainGroundCoverRadius(request.radius);
    if (!(radius > 0.0f))
    {
        return false;
    }

    const float cellSizeX = TerrainGroundCoverCellSizeX(terrain);
    const float cellSizeZ = TerrainGroundCoverCellSizeZ(terrain);
    if (!(cellSizeX > 0.0f) || !(cellSizeZ > 0.0f))
    {
        return false;
    }

    const unsigned char bit = TerrainGroundCoverEntryBit(request.entryIndex);
    const bool erase = request.operation == TerrainGroundCoverBrushOperation::Erase;
    bool changed = false;
    for (int iz = 0; iz < terrain.groundCoverResolutionZ; ++iz)
    {
        for (int ix = 0; ix < terrain.groundCoverResolutionX; ++ix)
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
            const int index = TerrainGroundCoverCellIndex(terrain, ix, iz);
            const int densitySlot = TerrainGroundCoverDensitySlot(index, request.entryIndex);
            if (index < 0 || densitySlot < 0
                || densitySlot >= static_cast<int>(terrain.groundCoverDensityQuanta.size())
                || densitySlot >= static_cast<int>(terrain.groundCoverPaintParams.size()))
            {
                continue;
            }
            unsigned char& cell = terrain.groundCoverCells[static_cast<std::size_t>(index)];
            unsigned char& quantum =
                terrain.groundCoverDensityQuanta[static_cast<std::size_t>(densitySlot)];
            std::uint16_t& paint = terrain.groundCoverPaintParams[static_cast<std::size_t>(densitySlot)];
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
                const TerrainGroundCoverEntry& entry =
                    terrain.groundCoverEntries[static_cast<std::size_t>(request.entryIndex)];
                const unsigned char painted = QuantizeTerrainGroundCoverDensity(entry.density);
                const std::uint16_t paintedStyle = PackTerrainGroundCoverPaintFromEntry(entry);
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

struct TerrainGroundCoverStroke
{
    bool active = false;
    float lastStampX = 0.0f;
    float lastStampZ = 0.0f;
};

inline void EndTerrainGroundCoverStroke(TerrainGroundCoverStroke& stroke)
{
    stroke.active = false;
}

inline bool BeginTerrainGroundCoverStroke(
    TerrainGroundCoverStroke& stroke,
    TerrainSpec& terrain,
    TerrainGroundCoverStampRequest request)
{
    stroke.active = true;
    stroke.lastStampX = request.centerX;
    stroke.lastStampZ = request.centerZ;
    return ApplyTerrainGroundCoverStamp(terrain, request);
}

inline bool ContinueTerrainGroundCoverStroke(
    TerrainGroundCoverStroke& stroke,
    TerrainSpec& terrain,
    TerrainGroundCoverStampRequest request,
    float cursorX,
    float cursorZ)
{
    if (!stroke.active)
    {
        return false;
    }
    const float spacing = TerrainGroundCoverStampSpacing(request.radius);
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
        changed = ApplyTerrainGroundCoverStamp(terrain, request) || changed;
        dx = cursorX - stroke.lastStampX;
        dz = cursorZ - stroke.lastStampZ;
        remaining = std::sqrt(dx * dx + dz * dz);
    }
    return changed;
}

inline std::uint32_t TerrainGroundCoverHash(
    std::uint32_t seed,
    int ix,
    int iz,
    int instance,
    int channel)
{
    return TerrainVegetationHash(seed, ix, iz, instance, channel);
}

inline float TerrainGroundCoverHash01(
    std::uint32_t seed,
    int ix,
    int iz,
    int instance,
    int channel)
{
    return TerrainVegetationHash01(seed, ix, iz, instance, channel);
}

struct TerrainGroundCoverInstance
{
    int entryIndex = 0;
    core::Vec3 position{};
    core::Vec3 axisX{1.0f, 0.0f, 0.0f};
    core::Vec3 axisY{0.0f, 1.0f, 0.0f};
    core::Vec3 axisZ{0.0f, 0.0f, 1.0f};
    float width = 1.0f;
    float height = 1.0f;
};

inline constexpr float kTerrainGroundCoverPlacementMargin = 0.22f;
inline constexpr float kTerrainGroundCoverR2X = 0.7548776662466927f;
inline constexpr float kTerrainGroundCoverR2Z = 0.5698402909980532f;

inline int TerrainGroundCoverInstanceCountForCell(
    const TerrainSpec& terrain,
    float density,
    int ix,
    int iz)
{
    const float area = TerrainGroundCoverCellSizeX(terrain) * TerrainGroundCoverCellSizeZ(terrain);
    const float expected = density * area;
    if (!(expected > 0.0f) || !std::isfinite(expected))
    {
        return 0;
    }
    int whole = static_cast<int>(std::floor(expected));
    if (whole >= kMaxTerrainGroundCoverInstancesPerCell)
    {
        return kMaxTerrainGroundCoverInstancesPerCell;
    }
    const float fraction = expected - static_cast<float>(whole);
    if (TerrainGroundCoverHash01(terrain.groundCoverSeed, ix, iz, 0, 0) < fraction)
    {
        ++whole;
    }
    if (whole > kMaxTerrainGroundCoverInstancesPerCell)
    {
        whole = kMaxTerrainGroundCoverInstancesPerCell;
    }
    return whole < 0 ? 0 : whole;
}

inline void TerrainGroundCoverChooseWorldXZ(
    const TerrainSpec& terrain,
    int ix,
    int iz,
    int instance,
    int channelBase,
    float& outX,
    float& outZ)
{
    const float cellSizeX = TerrainGroundCoverCellSizeX(terrain);
    const float cellSizeZ = TerrainGroundCoverCellSizeZ(terrain);
    const float margin = kTerrainGroundCoverPlacementMargin;
    const float span = 1.0f + 2.0f * margin;
    const float phaseX = TerrainGroundCoverHash01(terrain.groundCoverSeed, ix, iz, 0, channelBase + 6);
    const float phaseZ = TerrainGroundCoverHash01(terrain.groundCoverSeed, ix, iz, 0, channelBase + 7);
    const float jitterX =
        TerrainGroundCoverHash01(terrain.groundCoverSeed, ix, iz, instance, channelBase + 16) - 0.5f;
    const float jitterZ =
        TerrainGroundCoverHash01(terrain.groundCoverSeed, ix, iz, instance, channelBase + 17) - 0.5f;
    const float sampleX = phaseX
        + (static_cast<float>(instance) + 0.5f) * kTerrainGroundCoverR2X + jitterX * 0.18f;
    const float sampleZ = phaseZ
        + (static_cast<float>(instance) + 0.5f) * kTerrainGroundCoverR2Z + jitterZ * 0.18f;
    const float unitX = sampleX - std::floor(sampleX);
    const float unitZ = sampleZ - std::floor(sampleZ);
    const float alongX = -margin + span * unitX;
    const float alongZ = -margin + span * unitZ;
    float worldX = terrain.origin.x + (static_cast<float>(ix) + alongX) * cellSizeX;
    float worldZ = terrain.origin.z + (static_cast<float>(iz) + alongZ) * cellSizeZ;
    const float minX = terrain.origin.x;
    const float maxX = terrain.origin.x + terrain.sizeX;
    const float minZ = terrain.origin.z;
    const float maxZ = terrain.origin.z + terrain.sizeZ;
    if (worldX < minX)
    {
        worldX = minX;
    }
    if (worldX > maxX)
    {
        worldX = maxX;
    }
    if (worldZ < minZ)
    {
        worldZ = minZ;
    }
    if (worldZ > maxZ)
    {
        worldZ = maxZ;
    }
    outX = worldX;
    outZ = worldZ;
}

inline void BuildTerrainGroundCoverInstances(
    const TerrainSpec& terrain,
    std::vector<TerrainGroundCoverInstance>& out)
{
    out.clear();
    if (!terrain.enabled || !TerrainGroundCoverDataIsValid(terrain)
        || !TerrainGroundCoverShouldWrite(terrain))
    {
        return;
    }
    const int entryCount = static_cast<int>(terrain.groundCoverEntries.size());
    out.reserve(static_cast<std::size_t>(
        terrain.groundCoverResolutionX * terrain.groundCoverResolutionZ * 4));
    for (int entryIndex = 0; entryIndex < entryCount; ++entryIndex)
    {
        const int channelBase = entryIndex * 8;
        for (int iz = 0; iz < terrain.groundCoverResolutionZ; ++iz)
        {
            for (int ix = 0; ix < terrain.groundCoverResolutionX; ++ix)
            {
                const unsigned char cell = terrain.groundCoverCells[static_cast<std::size_t>(
                    TerrainGroundCoverCellIndex(terrain, ix, iz))];
                if (!TerrainGroundCoverCellHasEntry(cell, entryIndex))
                {
                    continue;
                }
                const int cellIndex = TerrainGroundCoverCellIndex(terrain, ix, iz);
                const int densitySlot = TerrainGroundCoverDensitySlot(cellIndex, entryIndex);
                if (densitySlot < 0
                    || densitySlot >= static_cast<int>(terrain.groundCoverDensityQuanta.size())
                    || densitySlot >= static_cast<int>(terrain.groundCoverPaintParams.size()))
                {
                    continue;
                }
                const float density = DequantizeTerrainGroundCoverDensity(
                    terrain.groundCoverDensityQuanta[static_cast<std::size_t>(densitySlot)]);
                const std::uint16_t paint =
                    terrain.groundCoverPaintParams[static_cast<std::size_t>(densitySlot)];
                const float minWidth =
                    DequantizeTerrainGroundCoverWidth(TerrainGroundCoverPaintMinWidthQuantum(paint));
                const float maxWidth =
                    DequantizeTerrainGroundCoverWidth(TerrainGroundCoverPaintMaxWidthQuantum(paint));
                const float minHeight =
                    DequantizeTerrainGroundCoverHeight(TerrainGroundCoverPaintMinHeightQuantum(paint));
                const float maxHeight =
                    DequantizeTerrainGroundCoverHeight(TerrainGroundCoverPaintMaxHeightQuantum(paint));
                const int count = TerrainGroundCoverInstanceCountForCell(terrain, density, ix, iz);
                for (int instance = 0; instance < count; ++instance)
                {
                    if (static_cast<int>(out.size()) >= kMaxTerrainGroundCoverInstances)
                    {
                        return;
                    }
                    float worldX = terrain.origin.x;
                    float worldZ = terrain.origin.z;
                    TerrainGroundCoverChooseWorldXZ(
                        terrain, ix, iz, instance, channelBase, worldX, worldZ);
                    float worldY = terrain.origin.y;
                    core::Vec3 normal{0.0f, 1.0f, 0.0f};
                    if (!SampleTerrainSurface(terrain, worldX, worldZ, worldY, normal))
                    {
                        continue;
                    }
                    const float widthT = minWidth == maxWidth
                        ? 0.0f
                        : TerrainGroundCoverHash01(
                              terrain.groundCoverSeed, ix, iz, instance, channelBase + 3);
                    const float heightT = minHeight == maxHeight
                        ? 0.0f
                        : TerrainGroundCoverHash01(
                              terrain.groundCoverSeed, ix, iz, instance, channelBase + 4);
                    const float yaw = TerrainGroundCoverHash01(
                                          terrain.groundCoverSeed, ix, iz, instance, channelBase + 5)
                        * 360.0f;
                    TerrainGroundCoverInstance derived{};
                    derived.entryIndex = entryIndex;
                    derived.position = {worldX, worldY, worldZ};
                    derived.width = minWidth + (maxWidth - minWidth) * widthT;
                    derived.height = minHeight + (maxHeight - minHeight) * heightT;
                    TerrainVegetationBasis(
                        true, yaw, normal, derived.axisX, derived.axisY, derived.axisZ);
                    out.push_back(derived);
                }
            }
        }
    }
}

struct TerrainGroundCoverRenderGroup
{
    int entryIndex = 0;
    std::size_t begin = 0;
    std::size_t count = 0;
};

struct TerrainGroundCoverRenderPlan
{
    std::vector<int> instanceOrder;
    std::vector<TerrainGroundCoverRenderGroup> groups;
};

// Groups instances that share a texture identity. One group is one GPU
// texture plus the shared crossed-card mesh. Draw submits that group once.
inline void BuildTerrainGroundCoverRenderPlan(
    const TerrainSpec& terrain,
    const std::vector<TerrainGroundCoverInstance>& instances,
    TerrainGroundCoverRenderPlan& out)
{
    out.instanceOrder.clear();
    out.groups.clear();
    if (instances.empty() || terrain.groundCoverEntries.empty())
    {
        return;
    }

    std::vector<int> groupOfInstance(instances.size(), -1);
    for (std::size_t index = 0; index < instances.size(); ++index)
    {
        const int entryIndex = instances[index].entryIndex;
        if (entryIndex < 0 || entryIndex >= static_cast<int>(terrain.groundCoverEntries.size()))
        {
            continue;
        }
        const std::string& identity =
            terrain.groundCoverEntries[static_cast<std::size_t>(entryIndex)].textureIdentity;
        int groupIndex = -1;
        for (std::size_t group = 0; group < out.groups.size(); ++group)
        {
            const int representative = out.groups[group].entryIndex;
            if (terrain.groundCoverEntries[static_cast<std::size_t>(representative)].textureIdentity
                == identity)
            {
                groupIndex = static_cast<int>(group);
                break;
            }
        }
        if (groupIndex < 0)
        {
            TerrainGroundCoverRenderGroup created{};
            created.entryIndex = entryIndex;
            out.groups.push_back(created);
            groupIndex = static_cast<int>(out.groups.size() - 1);
        }
        groupOfInstance[index] = groupIndex;
        ++out.groups[static_cast<std::size_t>(groupIndex)].count;
    }

    std::size_t cursor = 0;
    for (TerrainGroundCoverRenderGroup& group : out.groups)
    {
        group.begin = cursor;
        cursor += group.count;
        group.count = 0;
    }
    out.instanceOrder.assign(cursor, 0);
    for (std::size_t index = 0; index < instances.size(); ++index)
    {
        const int groupIndex = groupOfInstance[index];
        if (groupIndex < 0)
        {
            continue;
        }
        TerrainGroundCoverRenderGroup& group = out.groups[static_cast<std::size_t>(groupIndex)];
        out.instanceOrder[group.begin + group.count] = static_cast<int>(index);
        ++group.count;
    }
}

inline std::size_t TerrainGroundCoverInstancedSubmissionCount(
    const TerrainGroundCoverRenderPlan& plan)
{
    std::size_t submissions = 0;
    for (const TerrainGroundCoverRenderGroup& group : plan.groups)
    {
        if (group.count > 0)
        {
            ++submissions;
        }
    }
    return submissions;
}

inline std::uint64_t TerrainGroundCoverDeriveSignature(const TerrainSpec& terrain)
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
    mix(terrain.groundCoverSeed);
    mix(static_cast<std::uint64_t>(terrain.groundCoverResolutionX));
    mix(static_cast<std::uint64_t>(terrain.groundCoverResolutionZ));
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
    for (unsigned char cell : terrain.groundCoverCells)
    {
        mix(cell);
    }
    for (unsigned char quantum : terrain.groundCoverDensityQuanta)
    {
        mix(quantum);
    }
    for (std::uint16_t paint : terrain.groundCoverPaintParams)
    {
        mix(paint);
    }
    for (const TerrainGroundCoverEntry& entry : terrain.groundCoverEntries)
    {
        for (unsigned char byte : entry.textureIdentity)
        {
            mix(byte);
        }
    }
    return hash;
}

inline char TerrainGroundCoverHexDigit(int value)
{
    return TerrainWeightHexDigit(value & 15);
}

inline void AppendTerrainGroundCoverDataHex(const TerrainSpec& terrain, std::string& out)
{
    if (!TerrainGroundCoverShouldWrite(terrain)
        || static_cast<int>(terrain.groundCoverCells.size())
            != terrain.groundCoverResolutionX * terrain.groundCoverResolutionZ)
    {
        return;
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
    if (!anyOccupied)
    {
        return;
    }
    const int cells = terrain.groundCoverResolutionX * terrain.groundCoverResolutionZ;
    for (int cellIndex = 0; cellIndex < cells; ++cellIndex)
    {
        const unsigned char cell = terrain.groundCoverCells[static_cast<std::size_t>(cellIndex)];
        out.push_back(TerrainGroundCoverHexDigit(static_cast<int>(cell) & 0xF));
        for (int entryIndex = 0; entryIndex < kMaxTerrainGroundCoverEntries; ++entryIndex)
        {
            if (!TerrainGroundCoverCellHasEntry(cell, entryIndex))
            {
                continue;
            }
            const int slot = TerrainGroundCoverDensitySlot(cellIndex, entryIndex);
            const unsigned char quantum =
                slot >= 0 && slot < static_cast<int>(terrain.groundCoverDensityQuanta.size())
                ? terrain.groundCoverDensityQuanta[static_cast<std::size_t>(slot)]
                : 0;
            const std::uint16_t paint =
                slot >= 0 && slot < static_cast<int>(terrain.groundCoverPaintParams.size())
                ? terrain.groundCoverPaintParams[static_cast<std::size_t>(slot)]
                : 0;
            out.push_back(TerrainGroundCoverHexDigit((static_cast<int>(quantum) >> 4) & 0xF));
            out.push_back(TerrainGroundCoverHexDigit(static_cast<int>(quantum) & 0xF));
            out.push_back(TerrainGroundCoverHexDigit((static_cast<int>(paint) >> 12) & 0xF));
            out.push_back(TerrainGroundCoverHexDigit((static_cast<int>(paint) >> 8) & 0xF));
            out.push_back(TerrainGroundCoverHexDigit((static_cast<int>(paint) >> 4) & 0xF));
            out.push_back(TerrainGroundCoverHexDigit(static_cast<int>(paint) & 0xF));
        }
    }
}
}
