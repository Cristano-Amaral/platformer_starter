#include "physics/PhysicsWorld.h"
#include "physics/PhysicsWorldTestAccess.h"
#include "world/LevelFile.h"
#include "world/Terrain.h"
#include "world/TerrainGeometry.h"
#include "world/TerrainSculpt.h"

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

bool NearlyEqual(float a, float b, float epsilon = 0.08f)
{
    return std::fabs(a - b) <= epsilon;
}

world::LevelDefinition LoadCanonical()
{
    const world::ParseLevelFileResult parsed =
        world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
    Expect(parsed.status == world::LoadLevelFileStatus::Loaded, "load canonical");
    return parsed.level;
}

world::TerrainSpec MakeNonFlatTerrain()
{
    world::TerrainSpec terrain = world::MakeDefaultTerrain();
    terrain.resolutionX = 3;
    terrain.resolutionZ = 3;
    world::ResizeTerrainHeights(terrain);
    // Offset from canonical spawn/platforms/ground so rays hit Terrain only.
    terrain.origin = {32.0f, 1.0f, 0.0f};
    terrain.sizeX = 4.0f;
    terrain.sizeZ = 4.0f;
    terrain.heights = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f};
    return terrain;
}

bool HitY(
    physics::PhysicsWorld& world,
    core::Vec3 origin,
    core::Vec3 target,
    float& outY)
{
    float fraction = 0.0f;
    if (!physics::PhysicsWorldTestAccess::CastWorldRay(world, origin, target, fraction))
    {
        return false;
    }
    outY = origin.y + (target.y - origin.y) * fraction;
    return true;
}
}

int main()
{
    world::LevelDefinition none = LoadCanonical();
    Expect(!none.hasTerrain, "canonical has no Terrain");

    physics::PhysicsWorld world;
    Expect(world.Initialize(none), "initialize without Terrain");
    Expect(world.InitializePlayer(none.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "player without Terrain");
    const int baseStatic = world.StaticBodyCount();
    Expect(baseStatic == 9, "canonical static count");

    world::LevelDefinition withTerrain = none;
    withTerrain.hasTerrain = true;
    withTerrain.terrain = world::MakeDefaultTerrain();
    Expect(
        world.TryRebuild(
            withTerrain, withTerrain.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "Apply none -> Terrain");
    Expect(world.StaticBodyCount() == baseStatic + 1, "Terrain adds one static body");

    const core::Vec3 flatSample = world::TerrainSamplePosition(withTerrain.terrain, 1, 1);
    float hitY = 0.0f;
    Expect(
        HitY(
            world,
            {flatSample.x, flatSample.y + 8.0f, flatSample.z},
            {flatSample.x, flatSample.y - 8.0f, flatSample.z},
            hitY),
        "ground/ray sees flat Terrain");
    Expect(NearlyEqual(hitY, flatSample.y), "flat authored height correspondence");
    Expect(
        NearlyEqual(world::TerrainSampleSpacingX(withTerrain.terrain), 2.0f)
            && NearlyEqual(world::TerrainSampleSpacingZ(withTerrain.terrain), 2.0f),
        "horizontal spacing correspondence");

    world::LevelDefinition sloped = none;
    sloped.hasTerrain = true;
    sloped.terrain = MakeNonFlatTerrain();
    Expect(
        world.TryRebuild(sloped, sloped.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "Apply Terrain -> changed Terrain");
    Expect(world.StaticBodyCount() == baseStatic + 1, "rebuild keeps one Terrain body");

    const core::Vec3 highSample = world::TerrainSamplePosition(sloped.terrain, 1, 2);
    const core::Vec3 originSample = world::TerrainSamplePosition(sloped.terrain, 0, 0);
    Expect(
        HitY(
            world,
            {highSample.x, highSample.y + 8.0f, highSample.z},
            {highSample.x, highSample.y - 8.0f, highSample.z},
            hitY),
        "ray hits non-flat sample");
    Expect(NearlyEqual(hitY, highSample.y), "non-flat authored height correspondence");
    Expect(
        HitY(
            world,
            {originSample.x, originSample.y + 8.0f, originSample.z},
            {originSample.x, originSample.y - 8.0f, originSample.z},
            hitY),
        "origin sample correspondence");
    Expect(NearlyEqual(hitY, originSample.y), "origin height correspondence");

    world::LevelDefinition withBox = sloped;
    world::DynamicBoxSpec box{};
    box.center = {highSample.x, highSample.y + 1.5f, highSample.z};
    box.size = {0.6f, 0.6f, 0.6f};
    box.massKg = 30.0f;
    withBox.dynamicBoxes.push_back(box);
    Expect(
        world.TryRebuild(withBox, withBox.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "rebuild with Dynamic Box on Terrain");
    Expect(world.DynamicBodyCount() == 1, "Dynamic Box created");
    for (int frame = 0; frame < 90; ++frame)
    {
        world.Update(1.0f / 60.0f);
    }
    const std::vector<physics::DynamicBoxRuntimeState> boxes = world.GetDynamicBoxes();
    Expect(!boxes.empty() && boxes[0].valid, "Dynamic Box still valid");
    Expect(boxes[0].center.y > highSample.y, "Dynamic Box rests above Terrain");
    Expect(boxes[0].center.y < highSample.y + 2.0f, "Dynamic Box did not fall through Terrain");

    Expect(
        world.TryRebuild(none, none.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "Apply Terrain -> no Terrain");
    Expect(world.StaticBodyCount() == baseStatic, "Terrain body removed");
    Expect(
        !HitY(
            world,
            {highSample.x, highSample.y + 8.0f, highSample.z},
            {highSample.x, highSample.y - 0.1f, highSample.z},
            hitY),
        "no stale Terrain collision after removal");

    world::LevelDefinition terrainA = none;
    terrainA.hasTerrain = true;
    terrainA.terrain = world::MakeDefaultTerrain();
    world::LevelDefinition terrainB = none;
    terrainB.hasTerrain = true;
    terrainB.terrain = MakeNonFlatTerrain();
    Expect(
        world.TryRebuild(terrainA, terrainA.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "transition none -> Terrain A");
    Expect(
        world.TryRebuild(none, none.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "transition Terrain A -> none");
    Expect(world.StaticBodyCount() == baseStatic, "no stale body after A -> none");
    Expect(
        world.TryRebuild(terrainB, terrainB.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "transition none -> Terrain B");
    Expect(
        world.TryRebuild(terrainA, terrainA.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "transition Terrain B -> Terrain A");
    Expect(world.StaticBodyCount() == baseStatic + 1, "Terrain A has one body");
    const core::Vec3 aSample = world::TerrainSamplePosition(terrainA.terrain, 1, 1);
    Expect(
        HitY(
            world,
            {aSample.x, aSample.y + 8.0f, aSample.z},
            {aSample.x, aSample.y - 8.0f, aSample.z},
            hitY),
        "destination Terrain A collision");
    Expect(NearlyEqual(hitY, aSample.y), "destination uses only Terrain A heights");

    Expect(
        world.TryRebuild(terrainA, terrainA.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "rebuild same Terrain is safe");
    Expect(world.StaticBodyCount() == baseStatic + 1, "no duplicate Terrain body");

    world.ResetCharacter(terrainA.initialSpawnVisualCenter, {});
    Expect(world.StaticBodyCount() == baseStatic + 1, "Restart/respawn does not duplicate Terrain");
    Expect(terrainA.hasTerrain && world::TerrainSpecEqual(terrainA.terrain, world::MakeDefaultTerrain()),
        "Restart does not mutate authored Terrain");

    world::LevelDefinition sculpted = none;
    sculpted.hasTerrain = true;
    sculpted.terrain = world::MakeDefaultTerrain();
    const core::Vec3 sculptSample = world::TerrainSamplePosition(sculpted.terrain, 1, 1);
    world::TerrainSculptStampRequest raise{};
    raise.operation = world::TerrainSculptOperation::Raise;
    raise.centerX = sculptSample.x;
    raise.centerZ = sculptSample.z;
    raise.radius = 2.0f;
    raise.strength = 1.0f;
    Expect(world::ApplyTerrainSculptStamp(sculpted.terrain, raise), "sculpted collision fixture");
    Expect(
        world.TryRebuild(sculpted, sculpted.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "Apply sculpted Terrain rebuilds collision");
    Expect(world.StaticBodyCount() == baseStatic + 1, "sculpted Terrain keeps one body");
    const core::Vec3 hill = world::TerrainSamplePosition(sculpted.terrain, 1, 1);
    Expect(
        HitY(world, {hill.x, hill.y + 8.0f, hill.z}, {hill.x, hill.y - 8.0f, hill.z}, hitY),
        "ground/ray sees sculpted Terrain");
    Expect(NearlyEqual(hitY, hill.y), "Jolt after Apply reflects sculpted heights");
    Expect(
        world.TryRebuild(sculpted, sculpted.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "repeated Apply of sculpted Terrain");
    Expect(world.StaticBodyCount() == baseStatic + 1, "repeated Apply does not duplicate Terrain");

    world.Shutdown();
    Expect(!world.IsInitialized(), "shutdown clears world");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Terrain collision test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Terrain collision tests passed.\n");
    return 0;
}
