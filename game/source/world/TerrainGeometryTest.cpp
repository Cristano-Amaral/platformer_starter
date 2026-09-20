#include "world/Terrain.h"
#include "world/TerrainGeometry.h"

#include <cmath>
#include <cstdio>
#include <string>

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

bool VecNear(core::Vec3 a, core::Vec3 b, float epsilon = 1.0e-5f)
{
    return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
        && NearlyEqual(a.z, b.z, epsilon);
}
}

int main()
{
    Expect(world::kMinTerrainResolution == 2, "minimum resolution forms a cell");
    Expect(world::kMaxTerrainResolution == 17, "maximum resolution is bounded");
    Expect(world::TerrainSampleCount(9, 5) == 45, "default sample count");
    Expect(world::TerrainVertexCount(9, 5) == 45, "vertex count matches samples");
    Expect(world::TerrainCellCount(9, 5) == 8 * 4, "cell count is (res-1)^2 analog");
    Expect(world::TerrainTriangleCount(9, 5) == 64, "two triangles per cell");
    Expect(world::TerrainIndexCount(9, 5) == 192, "three indices per triangle");

    const world::TerrainSpec flat = world::MakeDefaultTerrain();
    Expect(world::TerrainSpecIsValid(flat), "default terrain is valid");
    Expect(flat.enabled, "default terrain is enabled");
    Expect(VecNear(flat.origin, world::kDefaultTerrainOrigin), "default origin");
    Expect(flat.sizeX == world::kDefaultTerrainSizeX, "default sizeX");
    Expect(flat.sizeZ == world::kDefaultTerrainSizeZ, "default sizeZ");
    Expect(flat.resolutionX == 9 && flat.resolutionZ == 5, "default resolution");
    Expect(static_cast<int>(flat.heights.size()) == 45, "default height array");
    Expect(NearlyEqual(world::TerrainSampleSpacingX(flat), 2.0f), "default spacing X");
    Expect(NearlyEqual(world::TerrainSampleSpacingZ(flat), 2.0f), "default spacing Z");

    world::TerrainGeometry geometry{};
    Expect(world::GenerateTerrainGeometry(flat, geometry), "generate flat");
    Expect(static_cast<int>(geometry.positions.size()) == 45, "flat vertex count");
    Expect(static_cast<int>(geometry.triangles.size()) == 64, "flat triangle count");
    Expect(world::TerrainGeometryIsFinite(geometry), "flat geometry is finite");

    Expect(
        VecNear(world::TerrainSamplePosition(flat, 0, 0), {-8.0f, 0.25f, -4.0f}),
        "sample(0,0) maps to origin");
    Expect(
        VecNear(world::TerrainSamplePosition(flat, 8, 4), {8.0f, 0.25f, 4.0f}),
        "opposite corner maps to origin+size");
    Expect(
        VecNear(geometry.positions[0], world::TerrainSamplePosition(flat, 0, 0)),
        "vertex 0 is sample(0,0)");

    const world::TerrainTriangle first = geometry.triangles[0];
    Expect(first.i0 == 0 && first.i1 == 9 && first.i2 == 1, "first cell winding CCW from +Y");
    Expect(
        geometry.triangles[1].i0 == 1 && geometry.triangles[1].i1 == 9
            && geometry.triangles[1].i2 == 10,
        "second triangle of first cell");

    for (const core::Vec3& normal : geometry.normals)
    {
        Expect(VecNear(normal, {0.0f, 1.0f, 0.0f}, 1.0e-4f), "flat normals point +Y");
    }

    world::TerrainSpec sloped = world::MakeDefaultTerrain();
    sloped.resolutionX = 3;
    sloped.resolutionZ = 3;
    world::ResizeTerrainHeights(sloped);
    sloped.heights = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f};
    Expect(world::TerrainSpecIsValid(sloped), "non-flat terrain is valid");
    Expect(
        NearlyEqual(world::TerrainSamplePosition(sloped, 0, 2).y, sloped.origin.y + 2.0f),
        "non-flat sample uses authored height");

    world::TerrainGeometry slopedGeometry{};
    Expect(world::GenerateTerrainGeometry(sloped, slopedGeometry), "generate non-flat");
    Expect(static_cast<int>(slopedGeometry.positions.size()) == 9, "non-flat vertex count");
    Expect(static_cast<int>(slopedGeometry.triangles.size()) == 8, "non-flat triangle count");
    Expect(world::TerrainGeometryIsFinite(slopedGeometry), "non-flat geometry is finite");
    Expect(
        VecNear(slopedGeometry.positions[world::TerrainHeightIndex(sloped, 1, 1)],
            world::TerrainSamplePosition(sloped, 1, 1)),
        "non-flat vertex matches sample");

    const core::Vec3 midNormal = slopedGeometry.normals[world::TerrainHeightIndex(sloped, 1, 1)];
    Expect(midNormal.z < -0.2f, "non-flat normal tilts with increasing Z height");
    Expect(midNormal.y > 0.2f, "non-flat normal still has upward component");
    Expect(!(std::fabs(midNormal.z) < 0.01f && NearlyEqual(midNormal.y, 1.0f)),
        "non-flat is not a constant +Y normal");

    const core::Vec3 ab = world::TerrainSub(
        slopedGeometry.positions[static_cast<std::size_t>(slopedGeometry.triangles[0].i1)],
        slopedGeometry.positions[static_cast<std::size_t>(slopedGeometry.triangles[0].i0)]);
    const core::Vec3 ac = world::TerrainSub(
        slopedGeometry.positions[static_cast<std::size_t>(slopedGeometry.triangles[0].i2)],
        slopedGeometry.positions[static_cast<std::size_t>(slopedGeometry.triangles[0].i0)]);
    const core::Vec3 faceNormal = world::TerrainNormalizeOrUp(world::TerrainCross(ab, ac));
    Expect(faceNormal.y > 0.0f, "triangle winding produces upward face");

    world::TerrainSpec invalid = flat;
    invalid.sizeX = 0.0f;
    Expect(!world::TerrainSpecIsValid(invalid), "zero size rejected");
    invalid = flat;
    invalid.heights[0] = std::nanf("");
    Expect(!world::TerrainSpecIsValid(invalid), "NaN height rejected");
    invalid = flat;
    invalid.resolutionX = 1;
    Expect(!world::GenerateTerrainGeometry(invalid, geometry), "below-min resolution rejected");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Terrain geometry test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Terrain geometry tests passed.\n");
    return 0;
}
