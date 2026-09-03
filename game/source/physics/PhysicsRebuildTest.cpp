// Focused harness for the highest-risk Milestone 32 Phase B mechanism: the
// editor Apply Preview rebuild cycle
//
//     PhysicsWorld::Shutdown()
//     PhysicsWorld::Initialize(levelDefinition)
//     PhysicsWorld::InitializePlayer(spawn, playerSize)
//
// run repeatedly in one process. Inspection alone cannot prove that the global
// Jolt Factory / RegisterTypes lifecycle survives repeated teardown, so this
// exercises it directly. Not shipped, and it never writes any file.

#include "physics/PhysicsWorld.h"
#include "world/GreyboxWorld.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"

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

constexpr float kStepSeconds = 1.0f / 60.0f;

void SettleFrames(physics::PhysicsWorld& world, int frames)
{
    for (int frame = 0; frame < frames; ++frame)
    {
        world.UpdateMovingPlatform(kStepSeconds);
        world.MovePlayer({0.0f, -1.0f}, kStepSeconds);
        world.Update(kStepSeconds);
    }
}

// Rebuild exactly like Application::ApplyLevelEditorPreview, then drop the
// character at probePosition and report whether the rebuilt collision held it.
bool StandsAt(
    physics::PhysicsWorld& world,
    const world::LevelDefinition& level,
    core::Vec3 probePosition,
    float& restingCenterY)
{
    world.Shutdown();
    if (!world.Initialize(level)
        || !world.InitializePlayer(level.initialSpawnVisualCenter, world::kPlayerVisualSize))
    {
        restingCenterY = 0.0f;
        return false;
    }

    world.ResetCharacter(probePosition, {});
    SettleFrames(world, 40);
    const physics::PlayerPhysicsState state = world.GetPlayerPhysicsState();
    restingCenterY = state.visualCenter.y;
    return state.supported;
}

// Mirrors Application::ApplyLevelEditorPreview: rebuild from the candidate,
// then simulate a few frames so contacts and the character actually settle.
bool ApplyCycle(physics::PhysicsWorld& world, const world::LevelDefinition& level, int cycle)
{
    const std::string tag = "cycle " + std::to_string(cycle);

    world.Shutdown();
    Expect(!world.IsInitialized(), tag + ": shutdown clears initialized");

    if (!world.Initialize(level))
    {
        Expect(false, tag + ": Initialize");
        return false;
    }
    Expect(world.IsInitialized(), tag + ": initialized after Initialize");
    // Ground + 6 elevated platforms + 2 slopes.
    Expect(world.StaticBodyCount() == 9, tag + ": static body count");
    Expect(world.IsDynamicTestBodyValid(), tag + ": dynamic box valid");

    if (!world.InitializePlayer(level.initialSpawnVisualCenter, world::kPlayerVisualSize))
    {
        Expect(false, tag + ": InitializePlayer");
        return false;
    }
    Expect(
        world.GetPlayerPhysicsState().characterInitialized,
        tag + ": character initialized");

    const physics::MovingPlatformState platformAtRebuild = world.GetMovingPlatform();
    Expect(platformAtRebuild.valid, tag + ": moving platform valid");
    Expect(
        platformAtRebuild.position.x == level.movingPlatform.startX,
        tag + ": moving platform reset to authored startX");
    Expect(platformAtRebuild.direction == 1.0f, tag + ": moving platform direction reset");

    const physics::DynamicTestBox boxAtRebuild = world.GetDynamicTestBox();
    Expect(boxAtRebuild.valid, tag + ": dynamic box present after rebuild");
    Expect(
        boxAtRebuild.position.y == level.dynamicBox.center.y,
        tag + ": dynamic box reset to authored center");

    SettleFrames(world, 30);

    const physics::PlayerPhysicsState player = world.GetPlayerPhysicsState();
    Expect(player.characterInitialized, tag + ": character alive after stepping");
    Expect(
        player.visualCenter.y > level.killPlaneY,
        tag + ": player did not fall through rebuilt collision");
    Expect(player.supported, tag + ": player supported by rebuilt ground");
    Expect(
        world.GetMovingPlatform().position.x != platformAtRebuild.position.x,
        tag + ": moving platform advances after rebuild");

    return true;
}
}

int main()
{
    const world::ParseLevelFileResult parsed =
        world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
    if (parsed.status != world::LoadLevelFileStatus::Loaded)
    {
        std::fprintf(
            stderr,
            "canonical level load failed: %s line %d %s\n",
            world::LoadLevelFileStatusName(parsed.status),
            parsed.errorLine,
            parsed.error.c_str());
        return 1;
    }

    // Initial build, then three editor-style Apply cycles with authored edits
    // of the kind the M32 editor allows.
    physics::PhysicsWorld world;
    Expect(world.Initialize(parsed.level), "initial Initialize");
    Expect(
        world.InitializePlayer(parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "initial InitializePlayer");

    world::LevelDefinition candidate = parsed.level;
    for (int cycle = 1; cycle <= 3; ++cycle)
    {
        candidate.camera.fieldOfViewY += 1.0f;
        candidate.elevatedPlatforms[0].center.x += 0.5f;
        candidate.initialSpawnVisualCenter.x += 0.1f;
        if (!ApplyCycle(world, candidate, cycle))
        {
            break;
        }
    }

    // Collision must follow an authored platform edit. Platform 0 is a 4x0.5x3
    // box centred at (5, 0.75, 0), so its top is y = 1.0 and a character
    // standing on it rests with its visual centre at y = 1.8. The ground top is
    // y = 0.0, so a character that fell rests at y = 0.8.
    {
        constexpr float kOnPlatformCenterY = 1.8f;
        constexpr float kOnGroundCenterY = 0.8f;
        const core::Vec3 originalProbe{5.0f, kOnPlatformCenterY + 0.1f, 0.0f};
        const core::Vec3 movedProbe{9.0f, kOnPlatformCenterY + 0.1f, 0.0f};

        world::LevelDefinition original = parsed.level;
        float restingY = 0.0f;
        Expect(
            StandsAt(world, original, originalProbe, restingY),
            "platform 0 supports the character at its authored position");
        Expect(
            restingY > kOnPlatformCenterY - 0.1f,
            "character rests on platform 0, not on the ground");

        world::LevelDefinition moved = parsed.level;
        moved.elevatedPlatforms[0].center.x = 9.0f;

        Expect(
            !StandsAt(world, moved, originalProbe, restingY)
                || restingY < kOnGroundCenterY + 0.2f,
            "old platform 0 collision is gone after the edit");
        Expect(
            StandsAt(world, moved, movedProbe, restingY),
            "moved platform 0 collision exists at the new position");
        Expect(
            restingY > kOnPlatformCenterY - 0.1f,
            "character rests on the moved platform");
    }

    // Shutdown twice: the destructor also calls it, so it must be idempotent.
    world.Shutdown();
    world.Shutdown();
    Expect(!world.IsInitialized(), "repeated shutdown is safe");

    // And the world must still be reusable afterwards.
    Expect(world.Initialize(parsed.level), "Initialize after repeated shutdown");
    Expect(
        world.InitializePlayer(parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "InitializePlayer after repeated shutdown");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d physics rebuild test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Physics rebuild tests passed.\n");
    return 0;
}
