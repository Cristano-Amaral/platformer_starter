#include "assets/RuntimePng.h"
#include "world/Terrain.h"
#include "world/TerrainGeometry.h"
#include "world/TerrainSculpt.h"

#include <cmath>
#include <cstdio>
#include <limits>
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

bool TexNear(world::TerrainTexCoord a, world::TerrainTexCoord b, float epsilon = 1.0e-5f)
{
    return NearlyEqual(a.u, b.u, epsilon) && NearlyEqual(a.v, b.v, epsilon);
}

world::TerrainGeometry MakeGeometry(const world::TerrainSpec& terrain)
{
    world::TerrainGeometry geometry{};
    Expect(world::GenerateTerrainGeometry(terrain, geometry), "generate geometry fixture");
    return geometry;
}
}

int main()
{
    Expect(
        std::string(assets::kRuntimeTexturesLogicalDirectory) == "textures",
        "M84 texture directory is textures/");
    Expect(
        assets::RuntimePngIdentityIsValid("textures/test_checker.png"),
        "valid M84-style texture identity");
    Expect(!assets::RuntimePngIdentityIsValid(""), "empty identity is not a texture reference");
    Expect(
        !assets::RuntimePngIdentityIsValid("C:/textures/test_checker.png"),
        "absolute path is rejected");
    Expect(
        !assets::RuntimePngIdentityIsValid("textures\\test_checker.png"),
        "backslash identity is rejected");
    Expect(
        !assets::RuntimePngIdentityIsValid("textures/../test_checker.png"),
        "parent-directory identity is rejected");
    Expect(
        !assets::RuntimePngIdentityIsValid("models/test_static.glb"),
        "model identity is not a Terrain texture");
    Expect(
        !assets::RuntimePngIdentityIsValid("textures/test_checker.PNG"),
        "extension is case-sensitive .png");
    Expect(
        !assets::RuntimePngIdentityIsValid("textures/my texture.png"),
        "spaces are not a single Level token");
    Expect(
        world::TerrainTextureIdentityIsValid(""),
        "no assignment is a valid empty identity");
    Expect(
        world::TerrainTextureIdentityIsValid("textures/test_checker.png"),
        "assigned identity follows M84 convention");
    Expect(
        !world::TerrainTextureIdentityIsValid("textures/missing space.png"),
        "invalid assigned identity is rejected");

    Expect(world::TerrainTextureTilingIsValid(world::kDefaultTerrainTextureTiling), "default tiling");
    Expect(world::TerrainTextureTilingIsValid(world::kMinTerrainTextureTiling), "min tiling");
    Expect(world::TerrainTextureTilingIsValid(world::kMaxTerrainTextureTiling), "max tiling");
    Expect(!world::TerrainTextureTilingIsValid(0.0f), "zero tiling rejected");
    Expect(!world::TerrainTextureTilingIsValid(-1.0f), "negative tiling rejected");
    Expect(!world::TerrainTextureTilingIsValid(world::kMaxTerrainTextureTiling + 1.0f),
        "over-max tiling rejected");
    Expect(!world::TerrainTextureTilingIsValid(std::numeric_limits<float>::quiet_NaN()),
        "non-finite tiling rejected");
    Expect(!world::TerrainTextureTilingIsValid(std::numeric_limits<float>::infinity()),
        "infinite tiling rejected");

    world::TerrainSpec terrain = world::MakeDefaultTerrain();
    Expect(world::TerrainSpecIsValid(terrain), "default Terrain remains valid");
    Expect(terrain.textureIdentity.empty(), "default has no texture assignment");
    Expect(terrain.textureTiling == world::kDefaultTerrainTextureTiling, "default tiling 0.25");
    Expect(
        !world::TerrainMaterialRecordShouldWrite(terrain),
        "default material is omitted from Level text");

    const world::TerrainSpec beforeAssign = terrain;
    Expect(
        world::TryAssignTerrainTextureIdentity(terrain, "textures/test_checker.png"),
        "first valid assignment changes authored data");
    Expect(terrain.textureIdentity == "textures/test_checker.png", "assignment stored");
    Expect(world::TerrainHeightsEqual(beforeAssign, terrain), "assignment does not change heights");
    Expect(
        world::TerrainMeshDataEqual(beforeAssign, terrain),
        "assignment does not change mesh/UV data");
    Expect(!world::TerrainSpecEqual(beforeAssign, terrain), "assignment is a semantic change");
    Expect(
        !world::TryAssignTerrainTextureIdentity(terrain, "textures/test_checker.png"),
        "same-assignment is a no-op");
    Expect(
        !world::TryAssignTerrainTextureIdentity(terrain, "C:/abs/grass.png"),
        "invalid identity is not stored");
    Expect(terrain.textureIdentity == "textures/test_checker.png", "invalid assign leaves identity");

    const world::TerrainSpec textured = terrain;
    Expect(world::TryClearTerrainTextureIdentity(terrain), "clear assigned texture");
    Expect(!world::TryClearTerrainTextureIdentity(terrain), "clear empty is a no-op");
    Expect(terrain.textureIdentity.empty(), "cleared identity is empty");
    Expect(world::TerrainHeightsEqual(textured, terrain), "clear does not change heights");

    terrain = world::MakeDefaultTerrain();
    Expect(!world::TrySetTerrainTextureTiling(terrain, world::kDefaultTerrainTextureTiling),
        "same tiling is a no-op");
    Expect(world::TrySetTerrainTextureTiling(terrain, 0.5f), "tiling change is authored");
    Expect(terrain.textureTiling == 0.5f, "tiling stored");
    Expect(terrain.heights == world::MakeDefaultTerrain().heights, "tiling does not change heights");
    Expect(!world::TrySetTerrainTextureTiling(terrain, 0.0f), "invalid tiling is a no-op");
    Expect(terrain.textureTiling == 0.5f, "invalid tiling leaves value");
    Expect(world::TerrainMaterialRecordShouldWrite(terrain), "custom tiling is written");

    world::TerrainSpec fallbackInvalid = world::MakeDefaultTerrain();
    fallbackInvalid.textureIdentity = "not-a-texture";
    Expect(!world::TerrainSpecIsValid(fallbackInvalid), "invalid identity fails spec validation");
    fallbackInvalid.textureIdentity.clear();
    fallbackInvalid.textureTiling = 0.0f;
    Expect(!world::TerrainSpecIsValid(fallbackInvalid), "out-of-bounds tiling fails spec validation");

    const world::TerrainSpec flat = world::MakeDefaultTerrain();
    world::TerrainGeometry flatGeometry = MakeGeometry(flat);
    Expect(static_cast<int>(flatGeometry.texcoords.size()) == 45, "flat UV count");
    Expect(
        TexNear(flatGeometry.texcoords[0], {0.0f, 0.0f}),
        "sample(0,0) UV is origin-relative zero");
    Expect(
        TexNear(
            world::TerrainSampleTexCoord(flat, 8, 4),
            {flat.sizeX * flat.textureTiling, flat.sizeZ * flat.textureTiling}),
        "opposite corner UV uses world XZ * tiling");
    Expect(
        TexNear(
            flatGeometry.texcoords[static_cast<std::size_t>(world::TerrainHeightIndex(flat, 8, 4))],
            world::TerrainSampleTexCoord(flat, 8, 4)),
        "generated UVs match the XZ formula");

    world::TerrainSpec rectangular = world::MakeDefaultTerrain();
    rectangular.sizeX = 16.0f;
    rectangular.sizeZ = 8.0f;
    rectangular.textureTiling = 0.25f;
    world::TerrainGeometry rectangularGeometry = MakeGeometry(rectangular);
    const world::TerrainTexCoord rectU = world::TerrainSampleTexCoord(rectangular, 8, 0);
    const world::TerrainTexCoord rectV = world::TerrainSampleTexCoord(rectangular, 0, 4);
    Expect(NearlyEqual(rectU.u, 4.0f) && NearlyEqual(rectU.v, 0.0f), "rectangular U along +X");
    Expect(NearlyEqual(rectV.u, 0.0f) && NearlyEqual(rectV.v, 2.0f), "rectangular V along +Z");
    Expect(
        NearlyEqual(rectU.u / rectangular.sizeX, rectV.v / rectangular.sizeZ),
        "planar mapping keeps isotropic world scale on rectangular Terrain");

    world::TerrainSpec raised = flat;
    raised.heights[static_cast<std::size_t>(world::TerrainHeightIndex(flat, 4, 2))] = 1.5f;
    world::TerrainGeometry raisedGeometry = MakeGeometry(raised);
    Expect(static_cast<int>(raisedGeometry.texcoords.size()) == 45, "raised UV count");
    for (int iz = 0; iz < flat.resolutionZ; ++iz)
    {
        for (int ix = 0; ix < flat.resolutionX; ++ix)
        {
            const int index = world::TerrainHeightIndex(flat, ix, iz);
            Expect(
                TexNear(
                    flatGeometry.texcoords[static_cast<std::size_t>(index)],
                    raisedGeometry.texcoords[static_cast<std::size_t>(index)]),
                "UVs stay stable when only heights change");
        }
    }

    world::TerrainSpec material = world::MakeDefaultTerrain();
    const std::vector<float> heightsBefore = material.heights;
    Expect(world::TryAssignTerrainTextureIdentity(material, "textures/test_checker.png"),
        "material assign fixture");
    Expect(world::TrySetTerrainTextureTiling(material, 1.0f), "material tiling fixture");
    Expect(material.heights == heightsBefore, "material edits do not modify heights");

    const std::string identityBefore = material.textureIdentity;
    const float tilingBefore = material.textureTiling;
    const core::Vec3 center = world::TerrainSamplePosition(material, 4, 2);
    world::TerrainSculptStampRequest raise{};
    raise.operation = world::TerrainSculptOperation::Raise;
    raise.centerX = center.x;
    raise.centerZ = center.z;
    raise.radius = 2.0f;
    raise.strength = 0.5f;
    Expect(world::ApplyTerrainSculptStamp(material, raise), "sculpt textured Terrain");
    Expect(material.textureIdentity == identityBefore, "sculpt does not change texture identity");
    Expect(material.textureTiling == tilingBefore, "sculpt does not change tiling");
    Expect(material.heights != heightsBefore, "sculpt still changes heights");

    world::TerrainSpec active = world::MakeDefaultTerrain();
    world::TerrainSpec working = active;
    Expect(world::TryAssignTerrainTextureIdentity(working, "textures/test_checker.png"),
        "workingCopy material edit");
    Expect(world::TrySetTerrainTextureTiling(working, 0.5f), "workingCopy tiling edit");
    Expect(!world::TerrainSpecEqual(working, active), "workingCopy differs before Apply");
    Expect(active.textureIdentity.empty(), "active identity unchanged before Apply");
    Expect(
        active.textureTiling == world::kDefaultTerrainTextureTiling,
        "active tiling unchanged before Apply");
    const world::TerrainSpec promoted = working;
    Expect(promoted.textureIdentity == working.textureIdentity, "Apply promotes identity");
    Expect(promoted.textureTiling == working.textureTiling, "Apply promotes tiling");
    Expect(world::TerrainHeightsEqual(promoted, working), "Apply preserves heights");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Terrain material test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Terrain material tests passed.\n");
    return 0;
}
