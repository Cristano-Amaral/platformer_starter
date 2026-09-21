#include "world/Terrain.h"
#include "world/TerrainGeometry.h"
#include "world/TerrainSculpt.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name.c_str());
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b, float epsilon = 1.0e-5f)
{
    return std::fabs(a - b) <= epsilon;
}

world::TerrainSpec MakeGrid(int resolutionX, int resolutionZ, float sizeX, float sizeZ)
{
    world::TerrainSpec terrain = world::MakeDefaultTerrain();
    terrain.origin = {0.0f, 1.0f, 0.0f};
    terrain.sizeX = sizeX;
    terrain.sizeZ = sizeZ;
    terrain.resolutionX = resolutionX;
    terrain.resolutionZ = resolutionZ;
    world::ResizeTerrainHeights(terrain);
    return terrain;
}

int ChangedSampleCount(const world::TerrainSpec& before, const world::TerrainSpec& after)
{
    int count = 0;
    const int n = static_cast<int>(before.heights.size());
    for (int index = 0; index < n; ++index)
    {
        if (before.heights[static_cast<std::size_t>(index)]
            != after.heights[static_cast<std::size_t>(index)])
        {
            ++count;
        }
    }
    return count;
}
}

int main()
{
    Expect(world::kDefaultTerrainSculptRadius == 2.0f, "default radius matches default spacing");
    Expect(world::kMinTerrainSculptRadius > 0.0f, "radius is positive");
    Expect(world::kMaxTerrainSculptRadius >= world::kMinTerrainSculptRadius, "radius bounded");
    Expect(world::kDefaultTerrainSculptStrength > 0.0f, "default strength positive");
    Expect(
        world::SanitizeTerrainSculptRadius(0.0f) == world::kMinTerrainSculptRadius,
        "zero radius clamps to min");
    Expect(
        world::SanitizeTerrainSculptRadius(1000.0f) == world::kMaxTerrainSculptRadius,
        "huge radius clamps to max");
    Expect(
        world::SanitizeTerrainSculptRadius(std::nanf("")) == world::kDefaultTerrainSculptRadius,
        "NaN radius uses default");
    Expect(
        world::SanitizeTerrainSculptStrength(0.0f) == world::kMinTerrainSculptStrength,
        "zero strength clamps to min");
    Expect(
        world::SanitizeTerrainSculptStrength(8.0f) == world::kMaxTerrainSculptStrength,
        "huge strength clamps to max");

    Expect(world::TerrainSculptFalloff(0.0f, 2.0f) == 1.0f, "center falloff is 1");
    Expect(world::TerrainSculptFalloff(1.0f, 2.0f) == 0.5f, "linear falloff at half radius");
    Expect(world::TerrainSculptFalloff(2.0f, 2.0f) == 0.0f, "boundary falloff is 0");
    Expect(world::TerrainSculptFalloff(2.1f, 2.0f) == 0.0f, "outside radius falloff is 0");
    Expect(
        world::TerrainSculptStampSpacing(2.0f)
            == 2.0f * world::kTerrainSculptStampSpacingFactor,
        "stamp spacing is 0.25 * radius");

    {
        world::TerrainSpec terrain = MakeGrid(5, 5, 8.0f, 8.0f);
        const world::TerrainSpec before = terrain;
        const core::Vec3 center = world::TerrainSamplePosition(terrain, 2, 2);
        world::TerrainSculptStampRequest raise{};
        raise.operation = world::TerrainSculptOperation::Raise;
        raise.centerX = center.x;
        raise.centerZ = center.z;
        raise.radius = 2.0f;
        raise.strength = 0.5f;
        Expect(world::ApplyTerrainSculptStamp(terrain, raise), "Raise mutates in-radius samples");
        Expect(ChangedSampleCount(before, terrain) == 1, "Raise radius 2 hits only the center sample");
        const int centerIndex = world::TerrainHeightIndex(terrain, 2, 2);
        Expect(
            NearlyEqual(terrain.heights[static_cast<std::size_t>(centerIndex)], 0.5f),
            "Raise increases the center by strength * falloff 1");
        for (int iz = 0; iz < 5; ++iz)
        {
            for (int ix = 0; ix < 5; ++ix)
            {
                if (ix == 2 && iz == 2)
                {
                    continue;
                }
                const int index = world::TerrainHeightIndex(terrain, ix, iz);
                Expect(
                    terrain.heights[static_cast<std::size_t>(index)] == 0.0f,
                    "Raise leaves samples at or beyond radius unchanged");
            }
        }
        Expect(terrain.heights[static_cast<std::size_t>(centerIndex)] > before.heights[static_cast<std::size_t>(centerIndex)],
            "Raise increases affected samples");
    }

    {
        world::TerrainSpec terrain = MakeGrid(5, 5, 8.0f, 8.0f);
        const core::Vec3 center = world::TerrainSamplePosition(terrain, 2, 2);
        world::TerrainSculptStampRequest raise{};
        raise.operation = world::TerrainSculptOperation::Raise;
        raise.centerX = center.x;
        raise.centerZ = center.z;
        raise.radius = 4.0f;
        raise.strength = 1.0f;
        Expect(world::ApplyTerrainSculptStamp(terrain, raise), "falloff Raise mutates");
        const int centerIndex = world::TerrainHeightIndex(terrain, 2, 2);
        const int neighborIndex = world::TerrainHeightIndex(terrain, 1, 2);
        Expect(
            NearlyEqual(terrain.heights[static_cast<std::size_t>(centerIndex)], 1.0f),
            "center uses full strength");
        Expect(
            NearlyEqual(terrain.heights[static_cast<std::size_t>(neighborIndex)], 0.5f),
            "neighbor at half radius uses linear falloff 0.5");
        const int farIndex = world::TerrainHeightIndex(terrain, 0, 0);
        Expect(
            terrain.heights[static_cast<std::size_t>(farIndex)] == 0.0f,
            "corner outside radius is unchanged");
    }

    {
        world::TerrainSpec terrain = MakeGrid(5, 5, 8.0f, 8.0f);
        const world::TerrainSpec before = terrain;
        const core::Vec3 center = world::TerrainSamplePosition(terrain, 2, 2);
        world::TerrainSculptStampRequest lower{};
        lower.operation = world::TerrainSculptOperation::Lower;
        lower.centerX = center.x;
        lower.centerZ = center.z;
        lower.radius = 2.0f;
        lower.strength = 0.5f;
        Expect(world::ApplyTerrainSculptStamp(terrain, lower), "Lower mutates");
        const int centerIndex = world::TerrainHeightIndex(terrain, 2, 2);
        Expect(
            NearlyEqual(terrain.heights[static_cast<std::size_t>(centerIndex)], -0.5f),
            "Lower decreases the center by strength");
        Expect(ChangedSampleCount(before, terrain) == 1, "Lower only affects in-radius samples");
        Expect(
            terrain.heights[static_cast<std::size_t>(centerIndex)]
                < before.heights[static_cast<std::size_t>(centerIndex)],
            "Lower decreases affected samples");
    }

    {
        world::TerrainSpec terrain = MakeGrid(3, 3, 4.0f, 4.0f);
        for (float& height : terrain.heights)
        {
            height = world::kMaxTerrainHeightAbs;
        }
        const core::Vec3 center = world::TerrainSamplePosition(terrain, 1, 1);
        world::TerrainSculptStampRequest raise{};
        raise.operation = world::TerrainSculptOperation::Raise;
        raise.centerX = center.x;
        raise.centerZ = center.z;
        raise.radius = 8.0f;
        raise.strength = 1.0f;
        Expect(!world::ApplyTerrainSculptStamp(terrain, raise), "Raise at max height is a no-op");
        Expect(
            terrain.heights[static_cast<std::size_t>(world::TerrainHeightIndex(terrain, 1, 1))]
                == world::kMaxTerrainHeightAbs,
            "Raise clamps to max authored height");

        for (float& height : terrain.heights)
        {
            height = -world::kMaxTerrainHeightAbs;
        }
        world::TerrainSculptStampRequest lower{};
        lower.operation = world::TerrainSculptOperation::Lower;
        lower.centerX = center.x;
        lower.centerZ = center.z;
        lower.radius = 8.0f;
        lower.strength = 1.0f;
        Expect(!world::ApplyTerrainSculptStamp(terrain, lower), "Lower at min height is a no-op");
        Expect(
            terrain.heights[static_cast<std::size_t>(world::TerrainHeightIndex(terrain, 1, 1))]
                == -world::kMaxTerrainHeightAbs,
            "Lower clamps to min authored height");
    }

    {
        world::TerrainSpec terrain = MakeGrid(3, 3, 4.0f, 4.0f);
        const world::TerrainSpec before = terrain;
        const core::Vec3 center = world::TerrainSamplePosition(terrain, 1, 1);
        world::TerrainSculptStampRequest smooth{};
        smooth.operation = world::TerrainSculptOperation::Smooth;
        smooth.centerX = center.x;
        smooth.centerZ = center.z;
        smooth.radius = 8.0f;
        smooth.strength = 1.0f;
        Expect(!world::ApplyTerrainSculptStamp(terrain, smooth), "Smooth on flat is a semantic no-op");
        Expect(world::TerrainSpecEqual(before, terrain), "no-op Smooth leaves heights identical");
    }

    {
        world::TerrainSpec terrain = MakeGrid(3, 3, 4.0f, 4.0f);
        terrain.heights = {0.0f, 0.0f, 0.0f, 0.0f, 9.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        const std::vector<float> snapshot = terrain.heights;
        const core::Vec3 center = world::TerrainSamplePosition(terrain, 1, 1);
        world::TerrainSculptStampRequest smooth{};
        smooth.operation = world::TerrainSculptOperation::Smooth;
        smooth.centerX = center.x;
        smooth.centerZ = center.z;
        smooth.radius = 8.0f;
        smooth.strength = 1.0f;
        Expect(world::ApplyTerrainSculptStamp(terrain, smooth), "Smooth mutates a spike");

        world::TerrainSpec expected = MakeGrid(3, 3, 4.0f, 4.0f);
        expected.heights = snapshot;
        for (int iz = 2; iz >= 0; --iz)
        {
            for (int ix = 2; ix >= 0; --ix)
            {
                const core::Vec3 sample = world::TerrainSamplePosition(expected, ix, iz);
                const float weight = world::TerrainSculptFalloff(
                    world::TerrainSculptDistanceXZ(sample.x, sample.z, center.x, center.z),
                    8.0f);
                const float target = world::TerrainSculptSmoothTarget(snapshot, expected, ix, iz);
                const int index = world::TerrainHeightIndex(expected, ix, iz);
                const float current = snapshot[static_cast<std::size_t>(index)];
                expected.heights[static_cast<std::size_t>(index)] =
                    world::ClampAuthoredTerrainHeight(
                        current + (target - current) * world::TerrainSculptBlendFactor(1.0f, weight));
            }
        }
        Expect(world::TerrainSpecEqual(terrain, expected),
            "Smooth is independent of mutation iteration order");
        const float centerHeight =
            terrain.heights[static_cast<std::size_t>(world::TerrainHeightIndex(terrain, 1, 1))];
        Expect(centerHeight < 9.0f && centerHeight > 0.0f, "Smooth blends the spike toward neighbors");
        Expect(NearlyEqual(centerHeight, 1.0f), "3x3 mean of a center-9 spike is 1");
    }

    {
        world::TerrainSpec terrain = MakeGrid(5, 5, 8.0f, 8.0f);
        const core::Vec3 a = world::TerrainSamplePosition(terrain, 1, 2);
        const core::Vec3 b = world::TerrainSamplePosition(terrain, 3, 2);
        const float capturedWorldY = a.y + 2.0f;
        world::TerrainSculptStampRequest flatten{};
        flatten.operation = world::TerrainSculptOperation::Flatten;
        flatten.centerX = a.x;
        flatten.centerZ = a.z;
        flatten.radius = 2.0f;
        flatten.strength = 1.0f;
        flatten.flattenWorldY = capturedWorldY;
        Expect(world::ApplyTerrainSculptStamp(terrain, flatten), "Flatten stamp A");
        flatten.centerX = b.x;
        flatten.centerZ = b.z;
        Expect(world::ApplyTerrainSculptStamp(terrain, flatten), "Flatten stamp B uses same target");
        const float authoredTarget = capturedWorldY - terrain.origin.y;
        Expect(
            NearlyEqual(
                terrain.heights[static_cast<std::size_t>(world::TerrainHeightIndex(terrain, 1, 2))],
                authoredTarget),
            "Flatten converts world Y relative to origin Y");
        Expect(
            NearlyEqual(
                terrain.heights[static_cast<std::size_t>(world::TerrainHeightIndex(terrain, 3, 2))],
                authoredTarget),
            "Flatten keeps one target for the whole stroke");
        Expect(
            terrain.heights[static_cast<std::size_t>(world::TerrainHeightIndex(terrain, 2, 2))] == 0.0f,
            "Flatten does not snap samples outside each dab");
    }

    {
        world::TerrainSpec terrain = MakeGrid(5, 5, 8.0f, 8.0f);
        const core::Vec3 start = world::TerrainSamplePosition(terrain, 1, 2);
        world::TerrainSculptStampRequest raise{};
        raise.operation = world::TerrainSculptOperation::Raise;
        raise.centerX = start.x;
        raise.centerZ = start.z;
        raise.radius = 2.0f;
        raise.strength = 0.25f;
        world::TerrainSculptStroke stroke{};
        Expect(
            world::BeginTerrainSculptStroke(stroke, terrain, raise, start.y),
            "stroke begin stamps once");
        const world::TerrainSpec afterBegin = terrain;
        Expect(
            !world::ContinueTerrainSculptStroke(stroke, terrain, raise, start.x, start.z),
            "stationary continue does not restamp");
        Expect(world::TerrainSpecEqual(afterBegin, terrain), "held cursor is not frame-rate dependent");
        const core::Vec3 end = world::TerrainSamplePosition(terrain, 3, 2);
        Expect(
            world::ContinueTerrainSculptStroke(stroke, terrain, raise, end.x, end.z),
            "cursor travel produces additional stamps");
        Expect(!world::TerrainSpecEqual(afterBegin, terrain), "drag mutates further samples");
        world::EndTerrainSculptStroke(stroke);
        Expect(!stroke.active, "stroke ends on release");
    }

    {
        world::TerrainSpec active = world::MakeDefaultTerrain();
        world::TerrainSpec working = active;
        const core::Vec3 center = world::TerrainSamplePosition(working, 4, 2);
        world::TerrainSculptStampRequest raise{};
        raise.operation = world::TerrainSculptOperation::Raise;
        raise.centerX = center.x;
        raise.centerZ = center.z;
        raise.radius = 2.0f;
        raise.strength = 0.5f;
        Expect(world::ApplyTerrainSculptStamp(working, raise), "workingCopy Raise");
        Expect(!world::TerrainSpecEqual(working, active), "sculpted workingCopy differs from active");
        Expect(world::TerrainSpecEqual(active, world::MakeDefaultTerrain()),
            "unapplied sculpt does not mutate active Terrain");
        const world::TerrainSpec promoted = working;
        Expect(world::TerrainSpecEqual(promoted, working), "Apply promotes sculpted heights");

        world::TerrainGeometry geometry{};
        Expect(world::GenerateTerrainGeometry(promoted, geometry), "geometry from sculpted heights");
        const int centerIndex = world::TerrainHeightIndex(promoted, 4, 2);
        Expect(
            NearlyEqual(
                geometry.positions[static_cast<std::size_t>(centerIndex)].y,
                world::TerrainSamplePosition(promoted, 4, 2).y),
            "generated geometry reflects sculpted heights");
        Expect(
            geometry.normals[static_cast<std::size_t>(centerIndex)].y < 1.0f
                || geometry.normals[static_cast<std::size_t>(world::TerrainHeightIndex(promoted, 3, 2))].y
                    < 0.999f,
            "generated normals tilt with sculpted relief");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Terrain sculpt test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Terrain sculpt tests passed.\n");
    return 0;
}
