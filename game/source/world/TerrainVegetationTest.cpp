#include "editor/TerrainVegetation.h"
#include "world/TerrainSculpt.h"
#include "world/TerrainVegetation.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++gFailures;
    }
}

world::TerrainSpec MakeTerrain()
{
    return world::MakeDefaultTerrain();
}

bool AddTree(world::TerrainSpec& terrain, const char* identity = "models/test_static.glb")
{
    return world::TryAddTerrainVegetationEntry(terrain, identity);
}

bool SameInstances(
    const std::vector<world::TerrainVegetationInstance>& first,
    const std::vector<world::TerrainVegetationInstance>& second)
{
    if (first.size() != second.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < first.size(); ++index)
    {
        const world::TerrainVegetationInstance& a = first[index];
        const world::TerrainVegetationInstance& b = second[index];
        if (a.entryIndex != b.entryIndex || a.position.x != b.position.x || a.position.y != b.position.y
            || a.position.z != b.position.z || a.uniformScale != b.uniformScale || a.axisX.x != b.axisX.x
            || a.axisY.y != b.axisY.y || a.axisZ.z != b.axisZ.z)
        {
            return false;
        }
    }
    return true;
}

world::TerrainVegetationStampRequest CellStamp(
    const world::TerrainSpec& terrain,
    int ix,
    int iz,
    int entryIndex,
    float radius)
{
    world::TerrainVegetationStampRequest request{};
    request.entryIndex = entryIndex;
    request.radius = radius;
    request.centerX =
        terrain.origin.x + (static_cast<float>(ix) + 0.5f) * world::TerrainVegetationCellSizeX(terrain);
    request.centerZ =
        terrain.origin.z + (static_cast<float>(iz) + 0.5f) * world::TerrainVegetationCellSizeZ(terrain);
    return request;
}

unsigned char EntryQuantum(const world::TerrainSpec& terrain, int cellIndex, int entryIndex)
{
    return terrain.vegetationDensityQuanta[static_cast<std::size_t>(
        world::TerrainVegetationDensitySlot(cellIndex, entryIndex))];
}

std::uint16_t EntryPaint(const world::TerrainSpec& terrain, int cellIndex, int entryIndex)
{
    return terrain.vegetationPaintParams[static_cast<std::size_t>(
        world::TerrainVegetationDensitySlot(cellIndex, entryIndex))];
}

int OccupiedCells(const world::TerrainSpec& terrain)
{
    int count = 0;
    for (unsigned char cell : terrain.vegetationCells)
    {
        if (cell != 0)
        {
            ++count;
        }
    }
    return count;
}

world::TerrainVegetationStampRequest CenterStamp(const world::TerrainSpec& terrain)
{
    world::TerrainVegetationStampRequest request{};
    request.operation = world::TerrainVegetationBrushOperation::Paint;
    request.entryIndex = 0;
    request.radius = 2.0f;
    request.centerX = terrain.origin.x + terrain.sizeX * 0.5f;
    request.centerZ = terrain.origin.z + terrain.sizeZ * 0.5f;
    return request;
}
}

int main()
{
    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(world::TerrainVegetationIsAbsent(terrain), "default terrain has no vegetation");
        Expect(!AddTree(terrain, "textures/grass.png"), "texture identity is not a vegetation model");
        Expect(!AddTree(terrain, "C:/temp/tree.glb"), "absolute model path is rejected");
        Expect(AddTree(terrain), "palette accepts models/test_static.glb");
        Expect(terrain.vegetationResolutionX == world::kDefaultTerrainVegetationResolutionX,
            "vegetation grid is independent of the heightfield");
        Expect(terrain.vegetationResolutionX != terrain.resolutionX,
            "default vegetation resolution is not the geometry resolution");
        Expect(terrain.vegetationResolutionX != terrain.weightResolutionX,
            "default vegetation resolution is not the weight-map resolution");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain), "paint fixture entry");
        terrain.vegetationEntries[0].density = 4.0f;
        const int before = OccupiedCells(terrain);
        world::TerrainVegetationStampRequest outside = CenterStamp(terrain);
        outside.centerX = terrain.origin.x - 20.0f;
        Expect(!world::ApplyTerrainVegetationStamp(terrain, outside), "paint outside Terrain changes nothing");
        Expect(OccupiedCells(terrain) == before, "outside stamp leaves the grid empty");
        Expect(world::ApplyTerrainVegetationStamp(terrain, CenterStamp(terrain)), "paint on Terrain adds occupancy");
        const int painted = OccupiedCells(terrain);
        Expect(painted > 0, "paint adds vegetation cells");
        world::TerrainVegetationStampRequest erase = CenterStamp(terrain);
        erase.operation = world::TerrainVegetationBrushOperation::Erase;
        erase.radius = 64.0f;
        Expect(world::ApplyTerrainVegetationStamp(terrain, erase), "erase removes painted cells");
        Expect(OccupiedCells(terrain) == 0, "erase clears the brush region");

        for (unsigned char& cell : terrain.vegetationCells)
        {
            cell = 0;
        }
        world::TerrainVegetationStroke stroke{};
        world::TerrainVegetationStampRequest strokeRequest = CenterStamp(terrain);
        Expect(
            world::BeginTerrainVegetationStroke(stroke, terrain, strokeRequest),
            "stroke begins with one stamp");
        const int afterBegin = OccupiedCells(terrain);
        Expect(
            !world::ContinueTerrainVegetationStroke(
                stroke, terrain, strokeRequest, strokeRequest.centerX, strokeRequest.centerZ),
            "stationary pointer does not stamp again");
        Expect(OccupiedCells(terrain) == afterBegin, "stationary stroke does not add cells");
        const float spacing = world::TerrainVegetationStampSpacing(strokeRequest.radius);
        Expect(
            world::ContinueTerrainVegetationStroke(
                stroke,
                terrain,
                strokeRequest,
                strokeRequest.centerX + spacing * 4.0f,
                strokeRequest.centerZ),
            "travel beyond spacing stamps again");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain), "height-follow entry");
        terrain.vegetationEntries[0].density = 8.0f;
        terrain.vegetationEntries[0].minScale = 0.4f;
        terrain.vegetationEntries[0].maxScale = 1.6f;
        terrain.vegetationEntries[0].randomYaw = true;
        terrain.vegetationEntries[0].alignToNormal = false;
        Expect(world::ApplyTerrainVegetationStamp(terrain, CenterStamp(terrain)), "paint before sculpt");
        const std::uint16_t captured =
            world::PackTerrainVegetationPaintFromEntry(terrain.vegetationEntries[0]);
        const float capturedMin =
            world::DequantizeTerrainVegetationScale(world::TerrainVegetationPaintMinScaleQuantum(captured));
        const float capturedMax =
            world::DequantizeTerrainVegetationScale(world::TerrainVegetationPaintMaxScaleQuantum(captured));
        std::vector<world::TerrainVegetationInstance> before;
        world::BuildTerrainVegetationInstances(terrain, before);
        Expect(before.size() >= 4, "density produces several instances");
        bool scaleInRange = true;
        bool yawVaries = false;
        bool upright = true;
        for (const world::TerrainVegetationInstance& instance : before)
        {
            scaleInRange = scaleInRange && instance.uniformScale + 1.0e-4f >= capturedMin
                && instance.uniformScale <= capturedMax + 1.0e-4f;
            upright = upright && std::fabs(instance.axisY.x) < 1.0e-4f
                && std::fabs(instance.axisY.y - 1.0f) < 1.0e-4f
                && std::fabs(instance.axisY.z) < 1.0e-4f;
            if (std::fabs(instance.axisX.x - 1.0f) > 1.0e-3f)
            {
                yawVaries = true;
            }
        }
        Expect(scaleInRange, "randomized scale stays within min/max");
        Expect(yawVaries, "random yaw changes some instances");
        Expect(upright, "alignment disabled keeps world up");

        for (float& height : terrain.heights)
        {
            height += 1.25f;
        }
        std::vector<world::TerrainVegetationInstance> after;
        world::BuildTerrainVegetationInstances(terrain, after);
        Expect(after.size() == before.size(), "sculpt keeps the same derived instances");
        bool followed = after.size() == before.size();
        for (std::size_t index = 0; followed && index < before.size(); ++index)
        {
            followed = after[index].position.x == before[index].position.x
                && after[index].position.z == before[index].position.z
                && std::fabs(after[index].position.y - (before[index].position.y + 1.25f)) < 1.0e-3f;
        }
        Expect(followed, "vegetation Y follows sculpted Terrain and XZ stays put");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain), "yaw-disabled entry");
        terrain.vegetationEntries[0].density = 8.0f;
        terrain.vegetationEntries[0].randomYaw = false;
        terrain.vegetationEntries[0].minScale = 1.0f;
        terrain.vegetationEntries[0].maxScale = 1.0f;
        Expect(world::ApplyTerrainVegetationStamp(terrain, CenterStamp(terrain)), "paint yaw-disabled");
        const float expectedScale = world::DequantizeTerrainVegetationScale(
            world::QuantizeTerrainVegetationScale(1.0f));
        std::vector<world::TerrainVegetationInstance> instances;
        world::BuildTerrainVegetationInstances(terrain, instances);
        Expect(!instances.empty(), "yaw-disabled paint still places instances");
        bool fixed = true;
        for (const world::TerrainVegetationInstance& instance : instances)
        {
            fixed = fixed && std::fabs(instance.uniformScale - expectedScale) < 1.0e-5f
                && std::fabs(instance.axisX.x - 1.0f) < 1.0e-4f
                && std::fabs(instance.axisX.z) < 1.0e-4f
                && std::fabs(instance.axisZ.z - 1.0f) < 1.0e-4f;
        }
        Expect(fixed, "random yaw disabled keeps identity yaw and exact scale");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain), "alignment entry");
        terrain.vegetationEntries[0].density = 8.0f;
        terrain.vegetationEntries[0].randomYaw = false;
        terrain.vegetationEntries[0].alignToNormal = true;
        for (int iz = 0; iz < terrain.resolutionZ; ++iz)
        {
            for (int ix = 0; ix < terrain.resolutionX; ++ix)
            {
                terrain.heights[static_cast<std::size_t>(world::TerrainHeightIndex(terrain, ix, iz))] =
                    static_cast<float>(iz) * 0.75f;
            }
        }
        Expect(world::ApplyTerrainVegetationStamp(terrain, CenterStamp(terrain)), "paint on a slope");
        std::vector<world::TerrainVegetationInstance> instances;
        world::BuildTerrainVegetationInstances(terrain, instances);
        Expect(!instances.empty(), "sloped terrain still places vegetation");
        bool tilted = false;
        for (const world::TerrainVegetationInstance& instance : instances)
        {
            if (std::fabs(instance.axisY.z) > 0.05f || std::fabs(instance.axisY.y - 1.0f) > 0.05f)
            {
                tilted = true;
            }
        }
        Expect(tilted, "alignment enabled tilts instances with the Terrain normal");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain, "models/test_static.glb"), "remove fixture first entry");
        Expect(AddTree(terrain, "models/test_authored.glb"), "remove fixture second entry");
        terrain.vegetationEntries[0].density = 1.0f;
        terrain.vegetationEntries[1].density = world::kMaxTerrainVegetationDensity;
        world::TerrainVegetationStampRequest first = CenterStamp(terrain);
        first.entryIndex = 0;
        first.radius = 1.0f;
        world::TerrainVegetationStampRequest second = first;
        second.entryIndex = 1;
        second.centerX += 3.0f;
        Expect(world::ApplyTerrainVegetationStamp(terrain, first), "paint first entry");
        Expect(world::ApplyTerrainVegetationStamp(terrain, second), "paint second entry");
        Expect(world::TryRemoveTerrainVegetationEntry(terrain, 0), "removing an entry succeeds");
        Expect(terrain.vegetationEntries.size() == 1, "one palette entry remains");
        Expect(
            terrain.vegetationEntries[0].modelIdentity == "models/test_authored.glb",
            "later entry keeps its model");
        const unsigned char allowed =
            world::TerrainVegetationAllowedCellMask(static_cast<int>(terrain.vegetationEntries.size()));
        bool dangling = false;
        int surviving = 0;
        for (unsigned char cell : terrain.vegetationCells)
        {
            if ((cell & static_cast<unsigned char>(~allowed)) != 0)
            {
                dangling = true;
            }
            if (world::TerrainVegetationCellHasEntry(cell, 0))
            {
                ++surviving;
            }
        }
        Expect(!dangling, "palette removal leaves no dangling cell references");
        Expect(surviving > 0, "removing an entry keeps the other entry's occupancy");
        const unsigned char keptDensity =
            world::QuantizeTerrainVegetationDensity(world::kMaxTerrainVegetationDensity);
        const std::uint16_t keptPaint = world::PackTerrainVegetationPaintFromEntry(
            world::TerrainVegetationEntry{});
        bool kept = surviving > 0;
        bool strayQuantum = false;
        for (int cellIndex = 0; cellIndex < static_cast<int>(terrain.vegetationCells.size()); ++cellIndex)
        {
            const bool occupied = world::TerrainVegetationCellHasEntry(terrain.vegetationCells[static_cast<std::size_t>(cellIndex)], 0);
            const unsigned char quantum = EntryQuantum(terrain, cellIndex, 0);
            const std::uint16_t paint = EntryPaint(terrain, cellIndex, 0);
            if (occupied)
            {
                kept = kept && quantum == keptDensity && paint == keptPaint;
            }
            else if (quantum != 0 || paint != 0)
            {
                strayQuantum = true;
            }
            for (int entryIndex = 1; entryIndex < world::kMaxTerrainVegetationEntries; ++entryIndex)
            {
                if (EntryQuantum(terrain, cellIndex, entryIndex) != 0
                    || EntryPaint(terrain, cellIndex, entryIndex) != 0)
                {
                    strayQuantum = true;
                }
            }
        }
        Expect(kept, "palette removal keeps the surviving entry's authored density");
        Expect(!strayQuantum, "palette removal drops the removed entry's density");
        Expect(world::TryRemoveTerrainVegetationEntry(terrain, 0), "removing the last entry succeeds");
        Expect(world::TerrainVegetationIsAbsent(terrain), "last removal clears the vegetation grid");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain), "high-instance entry");
        terrain.vegetationResolutionX = world::kMaxTerrainVegetationResolution;
        terrain.vegetationResolutionZ = world::kMaxTerrainVegetationResolution;
        terrain.vegetationEntries[0].density = world::kMaxTerrainVegetationDensity;
        const int fullCells = terrain.vegetationResolutionX * terrain.vegetationResolutionZ;
        terrain.vegetationCells.assign(static_cast<std::size_t>(fullCells), 1);
        terrain.vegetationDensityQuanta.assign(
            static_cast<std::size_t>(fullCells * world::kMaxTerrainVegetationEntries), 0);
        terrain.vegetationPaintParams.assign(
            static_cast<std::size_t>(fullCells * world::kMaxTerrainVegetationEntries), 0);
        const unsigned char fullDensity =
            world::QuantizeTerrainVegetationDensity(world::kMaxTerrainVegetationDensity);
        const std::uint16_t fullPaint =
            world::PackTerrainVegetationPaintFromEntry(terrain.vegetationEntries[0]);
        for (int cellIndex = 0; cellIndex < fullCells; ++cellIndex)
        {
            terrain.vegetationDensityQuanta[static_cast<std::size_t>(
                world::TerrainVegetationDensitySlot(cellIndex, 0))] = fullDensity;
            terrain.vegetationPaintParams[static_cast<std::size_t>(
                world::TerrainVegetationDensitySlot(cellIndex, 0))] = fullPaint;
        }
        Expect(world::TerrainVegetationDataIsValid(terrain), "full vegetation grid stays valid");
        std::vector<world::TerrainVegetationInstance> instances;
        world::BuildTerrainVegetationInstances(terrain, instances);
        Expect(instances.size() >= 512, "a full grid derives hundreds of instances");
        world::TerrainVegetationRenderPlan plan;
        world::BuildTerrainVegetationRenderPlan(terrain, instances, plan);
        Expect(plan.groups.size() == 1, "one model identity is one render group");
        Expect(plan.groups[0].count == instances.size(), "the group covers every instance");
        const std::size_t submissions = world::TerrainVegetationInstancedSubmissionCount(plan, 1);
        Expect(submissions == 1, "one mesh is one instanced submission");
        Expect(submissions < instances.size(), "instances are not one submission each");
        std::vector<world::TerrainVegetationInstance> again;
        world::BuildTerrainVegetationInstances(terrain, again);
        Expect(again.size() == instances.size() && SameInstances(instances, again),
            "high-instance derivation is repeatable");
        int outsideOldInset = 0;
        const float cellX = world::TerrainVegetationCellSizeX(terrain);
        const float cellZ = world::TerrainVegetationCellSizeZ(terrain);
        for (const world::TerrainVegetationInstance& instance : instances)
        {
            const float unitX = (instance.position.x - terrain.origin.x) / cellX;
            const float unitZ = (instance.position.z - terrain.origin.z) / cellZ;
            const float fracX = unitX - std::floor(unitX);
            const float fracZ = unitZ - std::floor(unitZ);
            if (fracX < 0.18f || fracX > 0.82f || fracZ < 0.18f || fracZ > 0.82f)
            {
                ++outsideOldInset;
            }
        }
        Expect(
            outsideOldInset * 10 >= static_cast<int>(instances.size()),
            "placement reaches the lanes the old cell inset left empty");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain, "models/test_static.glb"), "coexistence entry A");
        Expect(AddTree(terrain, "models/test_authored.glb"), "coexistence entry B");
        terrain.vegetationEntries[0].density = 4.0f;
        terrain.vegetationEntries[1].density = 4.0f;
        world::TerrainVegetationStampRequest paintA = CenterStamp(terrain);
        paintA.entryIndex = 0;
        world::TerrainVegetationStampRequest paintB = paintA;
        paintB.entryIndex = 1;
        Expect(world::ApplyTerrainVegetationStamp(terrain, paintA), "paint A");
        const int entryA = [&]() {
            int count = 0;
            for (unsigned char cell : terrain.vegetationCells)
            {
                if (world::TerrainVegetationCellHasEntry(cell, 0))
                {
                    ++count;
                }
            }
            return count;
        }();
        Expect(entryA > 0, "A occupies cells before B is painted");
        Expect(world::ApplyTerrainVegetationStamp(terrain, paintB), "paint B over A");
        int overlap = 0;
        int entryAAfter = 0;
        int entryB = 0;
        for (unsigned char cell : terrain.vegetationCells)
        {
            const bool hasA = world::TerrainVegetationCellHasEntry(cell, 0);
            const bool hasB = world::TerrainVegetationCellHasEntry(cell, 1);
            if (hasA)
            {
                ++entryAAfter;
            }
            if (hasB)
            {
                ++entryB;
            }
            if (hasA && hasB)
            {
                ++overlap;
            }
        }
        Expect(entryAAfter == entryA, "painting B leaves A's occupancy");
        Expect(entryB > 0 && overlap > 0, "B occupies the same cells as A");
        std::vector<world::TerrainVegetationInstance> together;
        world::BuildTerrainVegetationInstances(terrain, together);
        bool derivedA = false;
        bool derivedB = false;
        for (const world::TerrainVegetationInstance& instance : together)
        {
            derivedA = derivedA || instance.entryIndex == 0;
            derivedB = derivedB || instance.entryIndex == 1;
        }
        Expect(derivedA && derivedB, "both palette entries derive instances in the overlap");
        world::TerrainVegetationStampRequest eraseB = paintB;
        eraseB.operation = world::TerrainVegetationBrushOperation::Erase;
        Expect(world::ApplyTerrainVegetationStamp(terrain, eraseB), "erase B");
        int entryAFinal = 0;
        int entryBFinal = 0;
        for (unsigned char cell : terrain.vegetationCells)
        {
            if (world::TerrainVegetationCellHasEntry(cell, 0))
            {
                ++entryAFinal;
            }
            if (world::TerrainVegetationCellHasEntry(cell, 1))
            {
                ++entryBFinal;
            }
        }
        Expect(entryBFinal == 0, "erase B clears B in the brush");
        Expect(entryAFinal == entryA, "erase B leaves A unchanged");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain), "spatial density entry");
        terrain.vegetationEntries[0].density = 1.0f;
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 4, 8, 0, 1.5f)),
            "paint low-density region A1");
        const unsigned char lowQuantum = world::QuantizeTerrainVegetationDensity(1.0f);
        const unsigned char highQuantum = world::QuantizeTerrainVegetationDensity(8.0f);
        const unsigned char repaintQuantum = world::QuantizeTerrainVegetationDensity(4.0f);
        Expect(lowQuantum != highQuantum && highQuantum != repaintQuantum, "authored densities quantize apart");
        const int a1Cell = world::TerrainVegetationCellIndex(terrain, 4, 8);
        Expect(EntryQuantum(terrain, a1Cell, 0) == lowQuantum, "A1 captures the low paint density");
        const std::vector<unsigned char> paintedLow = terrain.vegetationDensityQuanta;
        std::vector<world::TerrainVegetationInstance> regionA1;
        world::BuildTerrainVegetationInstances(terrain, regionA1);
        Expect(!regionA1.empty(), "low-density A1 still generates instances");
        terrain.vegetationEntries[0].density = 8.0f;
        std::vector<world::TerrainVegetationInstance> afterSlider;
        world::BuildTerrainVegetationInstances(terrain, afterSlider);
        Expect(
            terrain.vegetationDensityQuanta == paintedLow && SameInstances(regionA1, afterSlider),
            "changing Density without painting leaves A1 unchanged");
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 12, 8, 0, 1.0f)),
            "paint high-density region A2");
        const int a2Cell = world::TerrainVegetationCellIndex(terrain, 12, 8);
        Expect(EntryQuantum(terrain, a1Cell, 0) == lowQuantum, "A2 paint leaves A1 at low density");
        Expect(EntryQuantum(terrain, a2Cell, 0) == highQuantum, "A2 captures the high paint density");
        int lowCells = 0;
        int highCells = 0;
        for (int cellIndex = 0; cellIndex < static_cast<int>(terrain.vegetationCells.size()); ++cellIndex)
        {
            if (!world::TerrainVegetationCellHasEntry(terrain.vegetationCells[static_cast<std::size_t>(cellIndex)], 0))
            {
                continue;
            }
            const unsigned char quantum = EntryQuantum(terrain, cellIndex, 0);
            lowCells += quantum == lowQuantum ? 1 : 0;
            highCells += quantum == highQuantum ? 1 : 0;
        }
        Expect(lowCells > 0 && highCells > 0, "one entry keeps two authored densities");
        terrain.vegetationEntries[0].density = 4.0f;
        const std::vector<unsigned char> beforeThird = terrain.vegetationDensityQuanta;
        std::vector<world::TerrainVegetationInstance> beforeRepaint;
        world::BuildTerrainVegetationInstances(terrain, beforeRepaint);
        std::vector<world::TerrainVegetationInstance> untouched;
        world::BuildTerrainVegetationInstances(terrain, untouched);
        Expect(
            terrain.vegetationDensityQuanta == beforeThird && SameInstances(beforeRepaint, untouched),
            "a later Density change still leaves both regions unchanged");
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 4, 8, 0, 0.25f)),
            "repaint only the center of A1");
        Expect(EntryQuantum(terrain, a1Cell, 0) == repaintQuantum, "the repainted cell takes the new density");
        Expect(EntryQuantum(terrain, a2Cell, 0) == highQuantum, "repainting A1 leaves A2 unchanged");
        int changedCells = 0;
        for (int cellIndex = 0; cellIndex < static_cast<int>(terrain.vegetationCells.size()); ++cellIndex)
        {
            if (EntryQuantum(terrain, cellIndex, 0) != beforeThird[static_cast<std::size_t>(
                    world::TerrainVegetationDensitySlot(cellIndex, 0))])
            {
                ++changedCells;
                Expect(cellIndex == a1Cell, "only the repainted A1 cell changes density");
            }
        }
        Expect(changedCells == 1, "partial repaint changes one vegetation cell");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain, "models/test_static.glb"), "independent density entry A");
        Expect(AddTree(terrain, "models/test_authored.glb"), "independent density entry B");
        terrain.vegetationEntries[0].density = 1.0f;
        terrain.vegetationEntries[1].density = 8.0f;
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 8, 8, 0, 2.0f)),
            "paint A at low density");
        world::TerrainVegetationStampRequest paintB = CellStamp(terrain, 8, 8, 1, 2.0f);
        Expect(world::ApplyTerrainVegetationStamp(terrain, paintB), "paint B at high density over A");
        const unsigned char quantumA = world::QuantizeTerrainVegetationDensity(1.0f);
        const unsigned char quantumB = world::QuantizeTerrainVegetationDensity(8.0f);
        const int sharedCell = world::TerrainVegetationCellIndex(terrain, 8, 8);
        Expect(
            world::TerrainVegetationCellHasEntry(terrain.vegetationCells[static_cast<std::size_t>(sharedCell)], 0)
                && world::TerrainVegetationCellHasEntry(
                    terrain.vegetationCells[static_cast<std::size_t>(sharedCell)], 1),
            "A and B share the stamped cell");
        Expect(
            EntryQuantum(terrain, sharedCell, 0) == quantumA && EntryQuantum(terrain, sharedCell, 1) == quantumB,
            "coexisting entries keep independent densities");
        std::vector<unsigned char> shrub = terrain.vegetationDensityQuanta;
        std::vector<std::uint16_t> shrubPaint = terrain.vegetationPaintParams;
        terrain.vegetationEntries[0].density = 4.0f;
        terrain.vegetationEntries[0].minScale = 1.2f;
        terrain.vegetationEntries[0].maxScale = 1.8f;
        terrain.vegetationEntries[0].randomYaw = false;
        terrain.vegetationEntries[0].alignToNormal = true;
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 8, 8, 0, 0.25f)),
            "repaint A inside the shared cell");
        Expect(
            EntryQuantum(terrain, sharedCell, 0) == world::QuantizeTerrainVegetationDensity(4.0f),
            "repainting A updates A's density");
        Expect(
            EntryPaint(terrain, sharedCell, 0)
                == world::PackTerrainVegetationPaintFromEntry(terrain.vegetationEntries[0]),
            "repainting A updates A's scale, yaw, and align");
        bool shrubUntouched = true;
        for (int cellIndex = 0; cellIndex < static_cast<int>(terrain.vegetationCells.size()); ++cellIndex)
        {
            const std::size_t slot = static_cast<std::size_t>(
                world::TerrainVegetationDensitySlot(cellIndex, 1));
            if (EntryQuantum(terrain, cellIndex, 1) != shrub[slot]
                || EntryPaint(terrain, cellIndex, 1) != shrubPaint[slot])
            {
                shrubUntouched = false;
            }
        }
        Expect(shrubUntouched, "repainting A leaves B's density unchanged");
        world::TerrainVegetationStampRequest eraseA = CellStamp(terrain, 8, 8, 0, 64.0f);
        eraseA.operation = world::TerrainVegetationBrushOperation::Erase;
        Expect(world::ApplyTerrainVegetationStamp(terrain, eraseA), "erase A");
        bool clearedA = true;
        bool keptB = true;
        for (int cellIndex = 0; cellIndex < static_cast<int>(terrain.vegetationCells.size()); ++cellIndex)
        {
            const unsigned char cell = terrain.vegetationCells[static_cast<std::size_t>(cellIndex)];
            if (world::TerrainVegetationCellHasEntry(cell, 0) || EntryQuantum(terrain, cellIndex, 0) != 0
                || EntryPaint(terrain, cellIndex, 0) != 0)
            {
                clearedA = false;
            }
            if (world::TerrainVegetationCellHasEntry(cell, 1)
                && EntryQuantum(terrain, cellIndex, 1) != quantumB)
            {
                keptB = false;
            }
        }
        Expect(clearedA, "erasing A drops A's occupancy and density");
        Expect(
            keptB && world::TerrainVegetationCellHasEntry(
                terrain.vegetationCells[static_cast<std::size_t>(sharedCell)], 1),
            "erasing A leaves B and its density");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain), "spatial scale entry");
        terrain.vegetationEntries[0].minScale = 0.5f;
        terrain.vegetationEntries[0].maxScale = 0.8f;
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 4, 8, 0, 1.5f)),
            "paint small-scale region A");
        const std::uint16_t smallPaint =
            world::PackTerrainVegetationPaintFromEntry(terrain.vegetationEntries[0]);
        const int aCell = world::TerrainVegetationCellIndex(terrain, 4, 8);
        const int bCell = world::TerrainVegetationCellIndex(terrain, 12, 8);
        Expect(EntryPaint(terrain, aCell, 0) == smallPaint, "region A captures the small scale range");
        std::vector<world::TerrainVegetationInstance> regionA;
        world::BuildTerrainVegetationInstances(terrain, regionA);
        Expect(!regionA.empty(), "small-scale region A still generates instances");
        bool smallRange = true;
        for (const world::TerrainVegetationInstance& instance : regionA)
        {
            smallRange = smallRange && instance.uniformScale + 1.0e-4f >= 0.3f && instance.uniformScale <= 0.9f;
        }
        Expect(smallRange, "region A instances stay in the small scale range");
        terrain.vegetationEntries[0].minScale = 1.2f;
        terrain.vegetationEntries[0].maxScale = 1.8f;
        std::vector<world::TerrainVegetationInstance> afterSlider;
        world::BuildTerrainVegetationInstances(terrain, afterSlider);
        Expect(SameInstances(regionA, afterSlider), "changing Min/Max Scale without painting leaves A unchanged");
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 12, 8, 0, 1.0f)),
            "paint large-scale region B");
        const std::uint16_t largePaint =
            world::PackTerrainVegetationPaintFromEntry(terrain.vegetationEntries[0]);
        Expect(smallPaint != largePaint, "small and large scale ranges quantize apart");
        Expect(EntryPaint(terrain, aCell, 0) == smallPaint, "painting B leaves A's scale");
        Expect(EntryPaint(terrain, bCell, 0) == largePaint, "region B captures the large scale range");
        std::vector<world::TerrainVegetationInstance> both;
        world::BuildTerrainVegetationInstances(terrain, both);
        bool sawSmall = false;
        bool sawLarge = false;
        const float smallMin =
            world::DequantizeTerrainVegetationScale(world::TerrainVegetationPaintMinScaleQuantum(smallPaint));
        const float smallMax =
            world::DequantizeTerrainVegetationScale(world::TerrainVegetationPaintMaxScaleQuantum(smallPaint));
        const float largeMin =
            world::DequantizeTerrainVegetationScale(world::TerrainVegetationPaintMinScaleQuantum(largePaint));
        const float largeMax =
            world::DequantizeTerrainVegetationScale(world::TerrainVegetationPaintMaxScaleQuantum(largePaint));
        for (const world::TerrainVegetationInstance& instance : both)
        {
            sawSmall = sawSmall
                || (instance.uniformScale + 1.0e-4f >= smallMin && instance.uniformScale <= smallMax + 1.0e-4f);
            sawLarge = sawLarge
                || (instance.uniformScale + 1.0e-4f >= largeMin && instance.uniformScale <= largeMax + 1.0e-4f);
        }
        Expect(sawSmall && sawLarge, "A stays small and B uses the larger range");
        const std::vector<std::uint16_t> beforePartial = terrain.vegetationPaintParams;
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 4, 8, 0, 0.25f)),
            "repaint only the center of A");
        Expect(EntryPaint(terrain, aCell, 0) == largePaint, "the repainted A cell takes the new scale");
        Expect(EntryPaint(terrain, bCell, 0) == largePaint, "repainting A leaves B's scale");
        int changedCells = 0;
        for (int cellIndex = 0; cellIndex < static_cast<int>(terrain.vegetationCells.size()); ++cellIndex)
        {
            if (EntryPaint(terrain, cellIndex, 0)
                != beforePartial[static_cast<std::size_t>(world::TerrainVegetationDensitySlot(cellIndex, 0))])
            {
                ++changedCells;
                Expect(cellIndex == aCell, "only the repainted A cell changes scale");
            }
        }
        Expect(changedCells == 1, "partial scale repaint changes one vegetation cell");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain), "spatial yaw entry");
        terrain.vegetationEntries[0].density = 8.0f;
        terrain.vegetationEntries[0].randomYaw = true;
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 4, 8, 0, 1.5f)),
            "paint random-yaw region A");
        const int aCell = world::TerrainVegetationCellIndex(terrain, 4, 8);
        const int bCell = world::TerrainVegetationCellIndex(terrain, 12, 8);
        Expect(world::TerrainVegetationPaintRandomYaw(EntryPaint(terrain, aCell, 0)), "region A stores Random Yaw on");
        std::vector<world::TerrainVegetationInstance> yawOn;
        world::BuildTerrainVegetationInstances(terrain, yawOn);
        Expect(!yawOn.empty(), "random-yaw region A generates instances");
        terrain.vegetationEntries[0].randomYaw = false;
        std::vector<world::TerrainVegetationInstance> afterYawOff;
        world::BuildTerrainVegetationInstances(terrain, afterYawOff);
        Expect(SameInstances(yawOn, afterYawOff), "disabling Random Yaw without painting leaves A unchanged");
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 12, 8, 0, 1.0f)),
            "paint fixed-yaw region B");
        Expect(world::TerrainVegetationPaintRandomYaw(EntryPaint(terrain, aCell, 0)), "painting B leaves A's yaw");
        Expect(
            !world::TerrainVegetationPaintRandomYaw(EntryPaint(terrain, bCell, 0)),
            "region B stores Random Yaw off");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddTree(terrain), "spatial align entry");
        terrain.vegetationEntries[0].density = 8.0f;
        terrain.vegetationEntries[0].alignToNormal = false;
        for (int iz = 0; iz < terrain.resolutionZ; ++iz)
        {
            for (int ix = 0; ix < terrain.resolutionX; ++ix)
            {
                terrain.heights[static_cast<std::size_t>(world::TerrainHeightIndex(terrain, ix, iz))] =
                    static_cast<float>(iz) * 0.75f;
            }
        }
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 4, 8, 0, 1.5f)),
            "paint upright region A on a slope");
        const int aCell = world::TerrainVegetationCellIndex(terrain, 4, 8);
        const int bCell = world::TerrainVegetationCellIndex(terrain, 12, 8);
        Expect(
            !world::TerrainVegetationPaintAlignToNormal(EntryPaint(terrain, aCell, 0)),
            "region A stores Align off");
        std::vector<world::TerrainVegetationInstance> upright;
        world::BuildTerrainVegetationInstances(terrain, upright);
        Expect(!upright.empty(), "upright region A generates instances");
        terrain.vegetationEntries[0].alignToNormal = true;
        std::vector<world::TerrainVegetationInstance> afterAlignOn;
        world::BuildTerrainVegetationInstances(terrain, afterAlignOn);
        Expect(SameInstances(upright, afterAlignOn), "enabling Align without painting leaves A unchanged");
        Expect(
            world::ApplyTerrainVegetationStamp(terrain, CellStamp(terrain, 12, 8, 0, 1.0f)),
            "paint aligned region B");
        Expect(
            !world::TerrainVegetationPaintAlignToNormal(EntryPaint(terrain, aCell, 0)),
            "painting B leaves A's align");
        Expect(
            world::TerrainVegetationPaintAlignToNormal(EntryPaint(terrain, bCell, 0)),
            "region B stores Align on");
        std::vector<world::TerrainVegetationInstance> mixed;
        world::BuildTerrainVegetationInstances(terrain, mixed);
        bool aUpright = false;
        bool bTilted = false;
        for (const world::TerrainVegetationInstance& instance : mixed)
        {
            const bool tilted = std::fabs(instance.axisY.z) > 0.05f || std::fabs(instance.axisY.y - 1.0f) > 0.05f;
            if (!tilted)
            {
                aUpright = true;
            }
            else
            {
                bTilted = true;
            }
        }
        Expect(aUpright && bTilted, "A stays upright and B follows the Terrain normal");
        for (float& height : terrain.heights)
        {
            height += 0.8f;
        }
        std::vector<world::TerrainVegetationInstance> afterSculpt;
        world::BuildTerrainVegetationInstances(terrain, afterSculpt);
        Expect(afterSculpt.size() == mixed.size(), "sculpting keeps the same vegetation count");
        bool yFollowed = !afterSculpt.empty();
        bool aStillUpright = false;
        bool bStillTilted = false;
        for (std::size_t index = 0; index < afterSculpt.size(); ++index)
        {
            yFollowed = yFollowed && afterSculpt[index].position.y > mixed[index].position.y + 0.4f;
            const bool tilted = std::fabs(afterSculpt[index].axisY.z) > 0.05f
                || std::fabs(afterSculpt[index].axisY.y - 1.0f) > 0.05f;
            if (!tilted)
            {
                aStillUpright = true;
            }
            else
            {
                bStillTilted = true;
            }
        }
        Expect(yFollowed, "all vegetation follows Terrain Y after sculpt");
        Expect(aStillUpright && bStillTilted, "only Align-on vegetation follows the Terrain normal");
    }

    {
        world::TerrainSpec low = MakeTerrain();
        Expect(AddTree(low), "low-density count entry");
        low.vegetationEntries[0].density = 1.0f;
        const int cells = low.vegetationResolutionX * low.vegetationResolutionZ;
        low.vegetationCells.assign(static_cast<std::size_t>(cells), 1);
        low.vegetationDensityQuanta.assign(
            static_cast<std::size_t>(cells * world::kMaxTerrainVegetationEntries), 0);
        low.vegetationPaintParams.assign(
            static_cast<std::size_t>(cells * world::kMaxTerrainVegetationEntries), 0);
        const unsigned char lowQuantum = world::QuantizeTerrainVegetationDensity(1.0f);
        const unsigned char highQuantum = world::QuantizeTerrainVegetationDensity(8.0f);
        const std::uint16_t lowPaint =
            world::PackTerrainVegetationPaintFromEntry(low.vegetationEntries[0]);
        for (int cellIndex = 0; cellIndex < cells; ++cellIndex)
        {
            const int slot = world::TerrainVegetationDensitySlot(cellIndex, 0);
            low.vegetationDensityQuanta[static_cast<std::size_t>(slot)] = lowQuantum;
            low.vegetationPaintParams[static_cast<std::size_t>(slot)] = lowPaint;
        }
        std::vector<world::TerrainVegetationInstance> lowInstances;
        world::BuildTerrainVegetationInstances(low, lowInstances);
        world::TerrainSpec high = low;
        high.vegetationEntries[0].density = 8.0f;
        for (int cellIndex = 0; cellIndex < cells; ++cellIndex)
        {
            high.vegetationDensityQuanta[static_cast<std::size_t>(
                world::TerrainVegetationDensitySlot(cellIndex, 0))] = highQuantum;
        }
        std::vector<world::TerrainVegetationInstance> highInstances;
        world::BuildTerrainVegetationInstances(high, highInstances);
        Expect(!lowInstances.empty() && highInstances.size() >= lowInstances.size() * 2,
            "high authored density generates substantially more instances than low density");

        world::TerrainSpec seeded = high;
        seeded.vegetationSeed = 1u;
        std::vector<world::TerrainVegetationInstance> seedA;
        std::vector<world::TerrainVegetationInstance> seedAAgain;
        world::BuildTerrainVegetationInstances(seeded, seedA);
        world::BuildTerrainVegetationInstances(seeded, seedAAgain);
        Expect(SameInstances(seedA, seedAAgain), "the same seed repeats the same distribution");
        seeded.vegetationSeed = 99u;
        std::vector<world::TerrainVegetationInstance> seedB;
        std::vector<world::TerrainVegetationInstance> seedBAgain;
        world::BuildTerrainVegetationInstances(seeded, seedB);
        world::BuildTerrainVegetationInstances(seeded, seedBAgain);
        Expect(SameInstances(seedB, seedBAgain), "a second seed is stable");
        Expect(!SameInstances(seedA, seedB), "a different seed changes the distribution");

        world::TerrainSpec shared = MakeTerrain();
        Expect(AddTree(shared, "models/test_static.glb"), "shared model entry A");
        Expect(AddTree(shared, "models/test_static.glb"), "shared model entry B");
        shared.vegetationResolutionX = world::kMaxTerrainVegetationResolution;
        shared.vegetationResolutionZ = world::kMaxTerrainVegetationResolution;
        const int sharedCells = shared.vegetationResolutionX * shared.vegetationResolutionZ;
        shared.vegetationCells.assign(static_cast<std::size_t>(sharedCells), 3);
        shared.vegetationDensityQuanta.assign(
            static_cast<std::size_t>(sharedCells * world::kMaxTerrainVegetationEntries), 0);
        shared.vegetationPaintParams.assign(
            static_cast<std::size_t>(sharedCells * world::kMaxTerrainVegetationEntries), 0);
        const unsigned char sharedDensity = world::QuantizeTerrainVegetationDensity(8.0f);
        for (int entryIndex = 0; entryIndex < 2; ++entryIndex)
        {
            const std::uint16_t paint =
                world::PackTerrainVegetationPaintFromEntry(shared.vegetationEntries[static_cast<std::size_t>(entryIndex)]);
            for (int cellIndex = 0; cellIndex < sharedCells; ++cellIndex)
            {
                const int slot = world::TerrainVegetationDensitySlot(cellIndex, entryIndex);
                shared.vegetationDensityQuanta[static_cast<std::size_t>(slot)] = sharedDensity;
                shared.vegetationPaintParams[static_cast<std::size_t>(slot)] = paint;
            }
        }
        std::vector<world::TerrainVegetationInstance> sharedInstances;
        world::BuildTerrainVegetationInstances(shared, sharedInstances);
        world::TerrainVegetationRenderPlan sharedPlan;
        world::BuildTerrainVegetationRenderPlan(shared, sharedInstances, sharedPlan);
        Expect(sharedInstances.size() >= 512, "two entries of one model still generate many instances");
        Expect(sharedPlan.groups.size() == 1, "one model identity stays one render group");
        Expect(
            world::TerrainVegetationInstancedSubmissionCount(sharedPlan, 1) == 1,
            "shared model identity is one instanced submission per mesh");

        world::TerrainSpec split = shared;
        split.vegetationEntries[1].modelIdentity = "models/test_authored.glb";
        world::TerrainVegetationRenderPlan splitPlan;
        world::BuildTerrainVegetationRenderPlan(split, sharedInstances, splitPlan);
        Expect(splitPlan.groups.size() == 2, "two model identities are two render groups");
        Expect(
            world::TerrainVegetationInstancedSubmissionCount(splitPlan, 1) == 2,
            "each model identity adds one instanced submission per mesh");
        Expect(
            world::TerrainVegetationInstancedSubmissionCount(splitPlan, 1) < sharedInstances.size(),
            "mixed models are still not one submission per instance");
    }

    {
        world::LevelDefinition level{};
        level.hasTerrain = true;
        level.terrain = MakeTerrain();
        Expect(AddTree(level.terrain), "editor paint entry");
        level.terrain.vegetationEntries[0].density = 8.0f;
        editor::TerrainVegetationState state{};
        state.mode = true;
        state.operation = world::TerrainVegetationBrushOperation::Paint;
        const editor::EditorSelection selection{editor::EditorObjectKind::Terrain, 0};
        editor::Ray3 miss{};
        miss.origin = {0.0f, 30.0f, 0.0f};
        miss.direction = {0.0f, 1.0f, 0.0f};
        const editor::TerrainVegetationFrameResult missed = editor::TickTerrainVegetation(
            state, level, selection, miss, true, true, false, false, false, false);
        Expect(!missed.mutatedWorkingCopy, "a ray that misses Terrain does not paint");
        Expect(OccupiedCells(level.terrain) == 0, "missed ray leaves vegetation empty");
        editor::Ray3 hit{};
        hit.origin = {
            level.terrain.origin.x + level.terrain.sizeX * 0.5f,
            30.0f,
            level.terrain.origin.z + level.terrain.sizeZ * 0.5f};
        hit.direction = {0.0f, -1.0f, 0.0f};
        const editor::TerrainVegetationFrameResult painted = editor::TickTerrainVegetation(
            state, level, selection, hit, true, true, false, false, false, false);
        Expect(painted.mutatedWorkingCopy, "a Terrain hit paints the working copy");
        Expect(OccupiedCells(level.terrain) > 0, "editor paint writes working-copy cells");
        state.operation = world::TerrainVegetationBrushOperation::Erase;
        state.radius = 64.0f;
        const editor::TerrainVegetationFrameResult erased = editor::TickTerrainVegetation(
            state, level, selection, hit, true, true, false, false, false, false);
        Expect(erased.mutatedWorkingCopy, "editor erase mutates the working copy");
        Expect(OccupiedCells(level.terrain) == 0, "editor erase clears the hit region");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Terrain vegetation test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Terrain vegetation tests passed.\n");
    return 0;
}
