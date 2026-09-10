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

#include "editor/AuthoredObjectLifecycle.h"
#include "physics/PhysicsCapacity.h"
#include "physics/PhysicsWorld.h"
#include "physics/PhysicsWorldTestAccess.h"
#include "gameplay/Inventory.h"
#include "world/DynamicBox.h"
#include "world/GreyboxWorld.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"

#include <cmath>
#include <cstddef>
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

constexpr float kStepSeconds = 1.0f / 60.0f;

bool NearlyEqual(float a, float b, float epsilon = 0.05f)
{
    return std::fabs(a - b) < epsilon;
}

bool VecNear(core::Vec3 a, core::Vec3 b, float epsilon = 0.05f)
{
    return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
        && NearlyEqual(a.z, b.z, epsilon);
}

bool VelocityNearZero(core::Vec3 velocity, float epsilon = 0.05f)
{
    return VecNear(velocity, {}, epsilon);
}

bool RotationIsIdentity(const physics::DynamicBoxRuntimeState& box, float epsilon = 0.02f)
{
    return std::fabs(box.rotationX) < epsilon && std::fabs(box.rotationY) < epsilon
        && std::fabs(box.rotationZ) < epsilon && std::fabs(std::fabs(box.rotationW) - 1.0f) < epsilon;
}

void ArrangeDynamicBoxRuntimeMotion(
    physics::PhysicsWorld& world,
    std::size_t index,
    core::Vec3 center,
    core::Vec3 linearVelocity,
    core::Vec3 angularVelocity,
    float rotationX = 0.0f,
    float rotationY = 0.0f,
    float rotationZ = 0.0f,
    float rotationW = 1.0f)
{
    physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
        world,
        index,
        center,
        linearVelocity,
        angularVelocity,
        rotationX,
        rotationY,
        rotationZ,
        rotationW);
}

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
    Expect(world.DynamicBodyCount() == 0, tag + ": canonical Dynamic Boxes stay empty");

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

    Expect(world.DynamicBodyCount() == static_cast<int>(level.dynamicBoxes.size()),
        tag + ": Dynamic Box body count matches authored collection");

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

    {
        physics::PhysicsWorld live;
        Expect(live.Initialize(parsed.level), "live Initialize for TryRebuild");
        Expect(
            live.InitializePlayer(parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "live InitializePlayer for TryRebuild");
        Expect(live.StaticBodyCount() == 9, "live static count before TryRebuild");

        world::LevelDefinition moved = parsed.level;
        moved.elevatedPlatforms[0].center.x = 9.0f;
        moved.camera.fieldOfViewY = 55.0f;
        Expect(
            live.TryRebuild(
                moved, moved.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild succeeds without Shutdown first");
        Expect(live.IsInitialized(), "TryRebuild leaves world initialized");
        Expect(live.StaticBodyCount() == 9, "TryRebuild rebuilds the same static count");
        Expect(
            live.GetMovingPlatform().position.x == moved.movingPlatform.startX,
            "TryRebuild resets moving platform to authored start");
        Expect(live.DynamicBodyCount() == 0, "canonical rebuild has zero Dynamic Box bodies");

        physics::PhysicsWorld probe;
        Expect(probe.Initialize(parsed.level), "second world Initialize while live exists");
        Expect(
            probe.InitializePlayer(parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "second world InitializePlayer");
        Expect(live.IsInitialized() && probe.IsInitialized(), "two PhysicsWorld instances coexist");
        probe.Shutdown();
        Expect(live.IsInitialized(), "shutting down the second world preserves the live world");
        Expect(live.StaticBodyCount() == 9, "live static bodies survive the other world's Shutdown");
        Expect(
            live.GetPlayerPhysicsState().characterInitialized,
            "live CharacterVirtual survives the other world's Shutdown");
    }

    {
        physics::PhysicsWorld countWorld;
        Expect(countWorld.Initialize(parsed.level), "count Initialize");
        Expect(
            countWorld.InitializePlayer(
                parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "count InitializePlayer");
        Expect(countWorld.StaticBodyCount() == 9, "canonical 6 platforms -> 9 static");

        world::LevelDefinition fewer = parsed.level;
        fewer.elevatedPlatforms.pop_back();
        fewer.goalPlatformIndex = 4;
        Expect(
            countWorld.TryRebuild(
                fewer, fewer.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild fewer platforms");
        Expect(countWorld.IsInitialized(), "fewer platforms leaves world initialized");
        Expect(countWorld.StaticBodyCount() == 8, "5 platforms -> 8 static");

        world::LevelDefinition more = parsed.level;
        more.elevatedPlatforms.push_back({{30.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        Expect(
            countWorld.TryRebuild(
                more, more.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild more platforms");
        Expect(countWorld.StaticBodyCount() == 10, "7 platforms -> 10 static");
    }

    {
        physics::PhysicsWorld applyWorld;
        Expect(applyWorld.Initialize(parsed.level), "apply Initialize");
        Expect(
            applyWorld.InitializePlayer(
                parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "apply InitializePlayer");
        world::LevelDefinition added = parsed.level;
        Expect(
            editor::AddPlatform(added, {4.0f, 1.0f, 0.0f}).succeeded, "lifecycle Add before Apply");
        Expect(
            applyWorld.TryRebuild(
                added, added.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "Apply after Add rebuilds");
        Expect(applyWorld.StaticBodyCount() == 10, "Apply after Add body count");
        Expect(added.checkpoint1PlatformIndex == parsed.level.checkpoint1PlatformIndex, "Add keeps support indices");

        world::LevelDefinition deleted = parsed.level;
        Expect(
            editor::DeleteSelected(deleted, {editor::EditorObjectKind::ElevatedPlatform, 1}).succeeded,
            "lifecycle Delete before Apply");
        Expect(deleted.elevatedPlatforms.size() == 5, "delete unreferenced platform");
        Expect(deleted.checkpoint1PlatformIndex == 1, "cp1 remapped 2 -> 1");
        Expect(deleted.checkpoint2PlatformIndex == 3, "cp2 remapped 4 -> 3");
        Expect(deleted.goalPlatformIndex == 4, "goal remapped 5 -> 4");
        Expect(
            applyWorld.TryRebuild(
                deleted, deleted.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "Apply after Delete rebuilds");
        Expect(applyWorld.StaticBodyCount() == 8, "Apply after Delete body count");
    }

    {
        physics::PhysicsWorld capacityWorld;
        Expect(capacityWorld.Initialize(parsed.level), "capacity Initialize");
        Expect(
            capacityWorld.InitializePlayer(
                parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "capacity InitializePlayer");
        world::LevelDefinition seventeen = parsed.level;
        while (seventeen.elevatedPlatforms.size() < 17)
        {
            const float x = 40.0f + static_cast<float>(seventeen.elevatedPlatforms.size());
            seventeen.elevatedPlatforms.push_back({{x, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        }
        Expect(
            capacityWorld.TryRebuild(
                seventeen, seventeen.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "17 platforms rebuild succeeds");
        Expect(
            capacityWorld.StaticBodyCount() == 1 + 17 + world::kLevel01SlopeCount,
            "17 platforms static body count");

        world::LevelDefinition atBudget = parsed.level;
        atBudget.elevatedPlatforms.resize(
            static_cast<std::size_t>(physics::kMaxPhysicsElevatedPlatformCount),
            {{40.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        Expect(
            capacityWorld.TryRebuild(
                atBudget, atBudget.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "physics platform budget rebuild succeeds");

        world::LevelDefinition overBudget = atBudget;
        overBudget.elevatedPlatforms.push_back({{90.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        physics::PhysicsWorld rejected;
        Expect(
            !rejected.Initialize(overBudget)
                || !rejected.InitializePlayer(
                    overBudget.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "over-budget platform count is rejected by physics");
    }

    {
        physics::PhysicsWorld boxWorld;
        world::LevelDefinition withBox = parsed.level;
        world::DynamicBoxSpec spec{};
        // x=8 is over Ground, clear of the start-pose moving platform and of
        // platform 0 (x=5). Authored y=2 is high enough to fall under gravity.
        spec.center = {8.0f, 2.0f, 0.0f};
        spec.size = {1.0f, 1.0f, 1.0f};
        spec.massKg = 30.0f;
        withBox.dynamicBoxes.push_back(spec);
        Expect(boxWorld.Initialize(withBox), "Initialize with one Dynamic Box");
        Expect(
            boxWorld.InitializePlayer(withBox.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "InitializePlayer with Dynamic Box");
        Expect(boxWorld.DynamicBodyCount() == 1, "one definition creates one dynamic body");
        std::vector<physics::DynamicBoxRuntimeState> boxes = boxWorld.GetDynamicBoxes();
        Expect(boxes.size() == 1 && boxes[0].valid, "runtime query has one valid box");
        Expect(boxes[0].massKg == 30.0f, "authored mass maps to runtime");
        Expect(boxes[0].size.x == 1.0f && boxes[0].size.y == 1.0f && boxes[0].size.z == 1.0f,
            "authored size maps to runtime");
        const core::Vec3 start = boxes[0].center;
        for (int frame = 0; frame < 45; ++frame)
        {
            boxWorld.Update(kStepSeconds);
        }
        boxes = boxWorld.GetDynamicBoxes();
        Expect(boxes[0].center.y < start.y - 0.2f, "gravity lowers the Dynamic Box");
        Expect(boxes[0].center.y > withBox.ground.center.y, "Ground collision keeps the box above ground");
        const float fallenY = boxes[0].center.y;
        boxWorld.ResetCharacter(withBox.initialSpawnVisualCenter, {});
        boxes = boxWorld.GetDynamicBoxes();
        Expect(
            std::fabs(boxes[0].center.y - fallenY) < 0.2f,
            "checkpoint-style ResetCharacter does not reset Dynamic Boxes");

        boxWorld.ResetDynamicBoxes();
        boxes = boxWorld.GetDynamicBoxes();
        Expect(
            std::fabs(boxes[0].center.x - spec.center.x) < 0.05f
                && std::fabs(boxes[0].center.y - spec.center.y) < 0.05f,
            "reset restores authored pose");
        Expect(
            std::fabs(boxes[0].linearVelocity.x) < 0.05f
                && std::fabs(boxes[0].linearVelocity.y) < 0.05f
                && std::fabs(boxes[0].linearVelocity.z) < 0.05f,
            "reset clears linear velocity");

        const core::Vec3 authored = spec.center;
        for (int frame = 0; frame < 30; ++frame)
        {
            boxWorld.Update(kStepSeconds);
        }
        boxes = boxWorld.GetDynamicBoxes();
        Expect(std::fabs(boxes[0].center.y - authored.y) > 0.1f, "simulation moved the box");
        Expect(withBox.dynamicBoxes[0].center.x == authored.x, "authored center remains X=8");

        Expect(
            boxWorld.TryRebuild(
                withBox, withBox.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "rebuild after simulation");
        boxes = boxWorld.GetDynamicBoxes();
        Expect(
            std::fabs(boxes[0].center.x - authored.x) < 0.05f
                && std::fabs(boxes[0].center.y - authored.y) < 0.05f,
            "rebuild restores authored pose");

        world::LevelDefinition deleted = withBox;
        deleted.dynamicBoxes.clear();
        Expect(
            boxWorld.TryRebuild(
                deleted, deleted.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "delete + Apply removes body");
        Expect(boxWorld.DynamicBodyCount() == 0, "no dynamic bodies after delete rebuild");
        Expect(boxWorld.IsInitialized(), "world remains initialized after delete rebuild");

        world::LevelDefinition preserved = withBox;
        physics::PhysicsWorld failed;
        Expect(failed.Initialize(preserved), "preserve-world Initialize");
        Expect(
            failed.InitializePlayer(preserved.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "preserve-world InitializePlayer");
        world::LevelDefinition invalid = preserved;
        invalid.elevatedPlatforms.resize(
            static_cast<std::size_t>(physics::kMaxAuthoredPhysicsBodies) + 1,
            {{40.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        Expect(
            !failed.TryRebuild(
                invalid, invalid.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "failed rebuild rejects over-budget level");
        Expect(failed.IsInitialized(), "failed rebuild preserves previous world");
        Expect(failed.DynamicBodyCount() == 1, "failed rebuild keeps previous Dynamic Box");

        world::LevelDefinition mixed = parsed.level;
        mixed.elevatedPlatforms.resize(50, {{40.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        mixed.dynamicBoxes.assign(
            9, world::DynamicBoxSpec{{2.0f, 2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 30.0f});
        physics::PhysicsWorld mixedWorld;
        Expect(mixedWorld.Initialize(mixed), "50 platforms + 9 boxes Initialize");
        Expect(
            mixedWorld.InitializePlayer(mixed.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "50+9 InitializePlayer");
        Expect(mixedWorld.DynamicBodyCount() == 9, "nine Dynamic Box bodies");
        mixed.dynamicBoxes.push_back({{8.0f, 2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 30.0f});
        Expect(
            !mixedWorld.TryRebuild(
                mixed, mixed.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "50 platforms + 10 boxes exceeds shared budget");
        Expect(mixedWorld.DynamicBodyCount() == 9, "over-budget rebuild keeps previous 9 boxes");
    }

    {
        physics::PhysicsWorld pushWorld;
        world::LevelDefinition pushLevel = parsed.level;
        world::DynamicBoxSpec pushBox{};
        pushBox.center = {1.4f, 0.5f, 0.0f};
        pushBox.size = {1.0f, 1.0f, 1.0f};
        pushBox.massKg = 5.0f;
        pushLevel.dynamicBoxes.push_back(pushBox);
        Expect(pushWorld.Initialize(pushLevel), "push Initialize");
        Expect(
            pushWorld.InitializePlayer(pushLevel.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "push InitializePlayer");
        for (int frame = 0; frame < 90; ++frame)
        {
            pushWorld.UpdateMovingPlatform(kStepSeconds);
            pushWorld.MovePlayer({4.0f, -1.0f}, kStepSeconds);
            pushWorld.Update(kStepSeconds);
        }
        const std::vector<physics::DynamicBoxRuntimeState> pushed = pushWorld.GetDynamicBoxes();
        Expect(
            pushed.size() == 1 && pushed[0].center.x > pushBox.center.x + 0.05f,
            "player contact can push a Dynamic Box");
        Expect(
            pushLevel.dynamicBoxes[0].center.x == 1.4f,
            "push simulation does not rewrite authored center");
    }

    {
        physics::PhysicsWorld recoveryWorld;
        world::LevelDefinition recoveryLevel = parsed.level;
        world::DynamicBoxSpec boxA{};
        boxA.center = {2.0f, 4.0f, 0.0f};
        boxA.size = {1.0f, 4.0f, 1.0f};
        boxA.massKg = 30.0f;
        world::DynamicBoxSpec boxB{};
        boxB.center = {6.0f, 3.0f, 0.0f};
        boxB.size = {1.0f, 1.0f, 1.0f};
        boxB.massKg = 5.0f;
        recoveryLevel.dynamicBoxes.push_back(boxA);
        recoveryLevel.dynamicBoxes.push_back(boxB);

        world::LevelDefinition workingCopy = recoveryLevel;
        workingCopy.dynamicBoxes[0].center.x = 99.0f;
        const world::LevelDefinition savedSourceBaseline = recoveryLevel;
        const bool modifiedBefore =
            !world::AuthoredLevelDataEqual(workingCopy, recoveryLevel);
        const bool dirtyBefore =
            !world::AuthoredLevelDataEqual(recoveryLevel, savedSourceBaseline);

        Expect(recoveryWorld.Initialize(recoveryLevel), "recovery Initialize");
        Expect(
            recoveryWorld.InitializePlayer(
                recoveryLevel.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "recovery InitializePlayer");
        Expect(recoveryWorld.DynamicBodyCount() == 2, "recovery starts with two Dynamic Boxes");
        Expect(recoveryLevel.killPlaneY == -8.0f, "canonical active kill plane is -8");

        const float halfExtentY = boxA.size.y * 0.5f;
        Expect(halfExtentY == 2.0f, "threshold fixture half-extent is 2");

        const core::Vec3 aboveCenter{-10.0f, -7.9f, 1.0f};
        Expect(aboveCenter.y >= recoveryLevel.killPlaneY, "threshold high sample is not below kill plane");
        Expect(
            aboveCenter.y - halfExtentY < recoveryLevel.killPlaneY,
            "AABB minimum Y of high sample is below kill plane");
        ArrangeDynamicBoxRuntimeMotion(recoveryWorld,
            0,
            aboveCenter,
            {3.0f, -2.0f, 1.0f},
            {0.0f, 0.0f, 4.0f},
            0.0f,
            0.0f,
            0.70710678f,
            0.70710678f);
        ArrangeDynamicBoxRuntimeMotion(recoveryWorld,
            1,
            {8.0f, 1.5f, 0.25f},
            {1.25f, 0.5f, -0.75f},
            {0.4f, -0.2f, 0.6f},
            0.0f,
            0.70710678f,
            0.0f,
            0.70710678f);

        recoveryWorld.RecoverFallenDynamicBoxes();
        std::vector<physics::DynamicBoxRuntimeState> recovered = recoveryWorld.GetDynamicBoxes();
        Expect(recovered.size() == 2, "recovery query still has two boxes");
        Expect(
            VecNear(recovered[0].center, aboveCenter, 0.02f),
            "center Y >= kill plane does not recover even when AABB min Y is below");
        Expect(!RotationIsIdentity(recovered[0]), "unrecovered box keeps arranged orientation");
        Expect(
            !VelocityNearZero(recovered[0].linearVelocity) && !VelocityNearZero(recovered[0].angularVelocity),
            "unrecovered box keeps arranged velocities");

        const core::Vec3 belowCenter{-10.0f, -8.1f, 1.0f};
        Expect(belowCenter.y < recoveryLevel.killPlaneY, "threshold low sample is below kill plane");
        Expect(
            boxA.center.y >= recoveryLevel.killPlaneY,
            "authored center Y is not the detection value");
        ArrangeDynamicBoxRuntimeMotion(recoveryWorld,
            0,
            belowCenter,
            {3.0f, -2.0f, 1.0f},
            {0.0f, 0.0f, 4.0f},
            0.0f,
            0.0f,
            0.70710678f,
            0.70710678f);
        const std::vector<physics::DynamicBoxRuntimeState> beforeFallen =
            recoveryWorld.GetDynamicBoxes();
        Expect(beforeFallen[0].center.y < recoveryLevel.killPlaneY, "arranged runtime center is below kill plane");
        Expect(!RotationIsIdentity(beforeFallen[0]), "fallen box starts tumbled");
        Expect(
            !VelocityNearZero(beforeFallen[0].linearVelocity)
                && !VelocityNearZero(beforeFallen[0].angularVelocity),
            "fallen box starts with motion");
        const physics::DynamicBoxRuntimeState preservedB = beforeFallen[1];

        recoveryWorld.RecoverFallenDynamicBoxes();
        recovered = recoveryWorld.GetDynamicBoxes();
        Expect(VecNear(recovered[0].center, boxA.center), "fallen box recovers to active authored center");
        Expect(VelocityNearZero(recovered[0].linearVelocity), "recovered linear velocity is zero");
        Expect(VelocityNearZero(recovered[0].angularVelocity), "recovered angular velocity is zero");
        Expect(RotationIsIdentity(recovered[0]), "recovered orientation is authored identity");
        Expect(VecNear(recovered[1].center, preservedB.center, 0.02f), "other box keeps independent center");
        Expect(
            VecNear(recovered[1].linearVelocity, preservedB.linearVelocity, 0.02f),
            "other box keeps independent linear velocity");
        Expect(
            VecNear(recovered[1].angularVelocity, preservedB.angularVelocity, 0.02f),
            "other box keeps independent angular velocity");
        Expect(
            NearlyEqual(recovered[1].rotationX, preservedB.rotationX, 0.02f)
                && NearlyEqual(recovered[1].rotationY, preservedB.rotationY, 0.02f)
                && NearlyEqual(recovered[1].rotationZ, preservedB.rotationZ, 0.02f)
                && NearlyEqual(recovered[1].rotationW, preservedB.rotationW, 0.02f),
            "other box keeps independent orientation");

        Expect(
            world::AuthoredLevelDataEqual(recoveryLevel, savedSourceBaseline),
            "recovery does not mutate active authored LevelDefinition");
        Expect(
            workingCopy.dynamicBoxes[0].center.x == 99.0f,
            "recovery does not mutate workingCopy");
        Expect(
            !world::AuthoredLevelDataEqual(workingCopy, recoveryLevel) == modifiedBefore,
            "Modified semantics unchanged by recovery");
        Expect(
            !world::AuthoredLevelDataEqual(recoveryLevel, savedSourceBaseline) == dirtyBefore,
            "Dirty semantics unchanged by recovery");
        Expect(modifiedBefore, "pending workingCopy transform keeps Modified true");
        Expect(!dirtyBefore, "matching baseline keeps Dirty false");

        ArrangeDynamicBoxRuntimeMotion(recoveryWorld,
            0,
            {-12.0f, -20.0f, 0.0f},
            {2.0f, -8.0f, 0.0f},
            {1.0f, 0.0f, 1.0f},
            0.0f,
            0.0f,
            0.70710678f,
            0.70710678f);
        recoveryWorld.Update(kStepSeconds);
        recovered = recoveryWorld.GetDynamicBoxes();
        Expect(
            VecNear(recovered[0].center, boxA.center),
            "PhysicsWorld::Update recovers a fallen box without a full rebuild");
        Expect(VelocityNearZero(recovered[0].linearVelocity), "Update recovery zeros linear velocity");
        Expect(VelocityNearZero(recovered[0].angularVelocity), "Update recovery zeros angular velocity");
        Expect(RotationIsIdentity(recovered[0]), "Update recovery restores identity orientation");

        ArrangeDynamicBoxRuntimeMotion(recoveryWorld,
            0, {3.5f, 2.0f, 0.0f}, {0.8f, 0.0f, 0.0f}, {0.0f, 0.3f, 0.0f});
        ArrangeDynamicBoxRuntimeMotion(recoveryWorld,
            1, {9.5f, 1.25f, 0.0f}, {-0.4f, 0.2f, 0.0f}, {0.0f, 0.0f, 0.5f});
        recoveryWorld.ResetCharacter(recoveryLevel.initialSpawnVisualCenter, {});
        recovered = recoveryWorld.GetDynamicBoxes();
        Expect(
            VecNear(recovered[0].center, {3.5f, 2.0f, 0.0f}, 0.05f)
                && VecNear(recovered[1].center, {9.5f, 1.25f, 0.0f}, 0.05f),
            "checkpoint-style ResetCharacter still does not reset valid Dynamic Boxes");

        recoveryWorld.ResetDynamicBoxes();
        recovered = recoveryWorld.GetDynamicBoxes();
        Expect(VecNear(recovered[0].center, boxA.center), "Restart Run resets all boxes, including A");
        Expect(VecNear(recovered[1].center, boxB.center), "Restart Run resets all boxes, including B");
        Expect(VelocityNearZero(recovered[0].linearVelocity) && VelocityNearZero(recovered[0].angularVelocity),
            "Restart Run zeros box A motion");
        Expect(VelocityNearZero(recovered[1].linearVelocity) && VelocityNearZero(recovered[1].angularVelocity),
            "Restart Run zeros box B motion");
        Expect(RotationIsIdentity(recovered[0]) && RotationIsIdentity(recovered[1]),
            "Restart Run restores identity orientation for every box");

        world::LevelDefinition applied = recoveryLevel;
        applied.dynamicBoxes[0].center = {5.0f, 4.5f, 0.0f};
        Expect(
            recoveryWorld.TryRebuild(
                applied, applied.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "Apply-style rebuild with new authored Dynamic Box transform");
        ArrangeDynamicBoxRuntimeMotion(recoveryWorld,
            0,
            {-9.0f, -12.0f, 0.0f},
            {4.0f, -3.0f, 0.0f},
            {0.0f, 2.0f, 0.0f},
            0.0f,
            0.70710678f,
            0.0f,
            0.70710678f);
        recoveryWorld.RecoverFallenDynamicBoxes();
        recovered = recoveryWorld.GetDynamicBoxes();
        Expect(
            VecNear(recovered[0].center, applied.dynamicBoxes[0].center),
            "post-Apply recovery uses the new active authored transform");
        Expect(
            !VecNear(recovered[0].center, boxA.center),
            "post-Apply recovery does not use the pre-Apply transform");
        Expect(
            !VecNear(recovered[0].center, workingCopy.dynamicBoxes[0].center),
            "post-Apply recovery does not use unapplied workingCopy");
        Expect(workingCopy.dynamicBoxes[0].center.x == 99.0f, "Apply rebuild does not consume this pending copy");

        world::LevelDefinition staged = applied;
        staged.dynamicBoxes[0].center = {11.0f, 3.25f, 0.0f};
        Expect(
            recoveryWorld.TryRebuild(
                staged, staged.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "staged-reload-style rebuild from new authored definition");
        ArrangeDynamicBoxRuntimeMotion(recoveryWorld,
            0, {-4.0f, -15.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 3.0f});
        recoveryWorld.RecoverFallenDynamicBoxes();
        recovered = recoveryWorld.GetDynamicBoxes();
        Expect(
            VecNear(recovered[0].center, staged.dynamicBoxes[0].center),
            "post-reload recovery uses the newly active staged authored transform");
        Expect(
            !VecNear(recovered[0].center, applied.dynamicBoxes[0].center),
            "stale pre-reload recovery transform does not survive");
    }

    {
        physics::PhysicsWorld zeroWorld;
        Expect(zeroWorld.Initialize(parsed.level), "zero-box Initialize");
        Expect(
            zeroWorld.InitializePlayer(parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "zero-box InitializePlayer");
        Expect(zeroWorld.DynamicBodyCount() == 0, "canonical Level 01 still has zero Dynamic Boxes");
        zeroWorld.RecoverFallenDynamicBoxes();
        Expect(zeroWorld.IsInitialized(), "zero-box recovery is a no-op");
        Expect(zeroWorld.GetDynamicBoxes().empty(), "zero-box query stays empty");
        zeroWorld.Update(kStepSeconds);
        Expect(zeroWorld.DynamicBodyCount() == 0, "zero-box Update stays stable");
        Expect(zeroWorld.GetPlayerPhysicsState().characterInitialized, "zero-box player remains");
    }

    {
        world::LevelDefinition withProps = parsed.level;
        world::StaticPropSpec prop{};
        prop.modelIdentity = "models/test_static.glb";
        prop.position = {4.0f, 1.0f, 0.0f};
        prop.scale = world::kDefaultStaticPropScale;
        withProps.staticProps.push_back(prop);
        withProps.staticProps.push_back(prop);
        physics::PhysicsWorld propWorld;
        Expect(propWorld.Initialize(withProps), "Static Props Initialize");
        Expect(
            propWorld.InitializePlayer(withProps.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "Static Props InitializePlayer");
        Expect(propWorld.StaticBodyCount() == 9, "Static Props do not add static Jolt bodies");
        Expect(propWorld.DynamicBodyCount() == 0, "Static Props do not add dynamic Jolt bodies");
        Expect(
            propWorld.TryRebuild(
                withProps, withProps.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild with Static Props");
        Expect(propWorld.StaticBodyCount() == 9, "rebuild still ignores Static Props");
        Expect(propWorld.DynamicBodyCount() == 0, "rebuild still has zero Dynamic Boxes");
    }

    {
        world::LevelDefinition withPlates = parsed.level;
        world::PressurePlateSpec plate{};
        plate.center = {8.0f, 0.1f, 0.0f};
        plate.size = world::kDefaultPressurePlateSize;
        withPlates.pressurePlates.push_back(plate);
        withPlates.pressurePlates.push_back(plate);
        physics::PhysicsWorld plateWorld;
        Expect(plateWorld.Initialize(withPlates), "Pressure Plates Initialize");
        Expect(
            plateWorld.InitializePlayer(withPlates.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "Pressure Plates InitializePlayer");
        Expect(plateWorld.StaticBodyCount() == 9, "Pressure Plates do not add static Jolt bodies");
        Expect(plateWorld.DynamicBodyCount() == 0, "Pressure Plates do not add dynamic Jolt bodies");
        Expect(plateWorld.GetPressurePlates().size() == 2, "rebuild stores two applied plates");
        Expect(!plateWorld.GetPressurePlates()[0].active, "zero boxes stay Inactive");
        Expect(
            plateWorld.TryRebuild(
                withPlates, withPlates.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild with Pressure Plates");
        Expect(plateWorld.StaticBodyCount() == 9, "rebuild still ignores Pressure Plates for bodies");
        Expect(plateWorld.DynamicBodyCount() == 0, "rebuild still has zero Dynamic Boxes");
        Expect(plateWorld.GetPressurePlates().size() == 2, "rebuild keeps two plates");
    }

    {
        world::LevelDefinition withDoors = parsed.level;
        world::DoorSpec door{};
        door.center = {8.5f, 1.5f, 0.0f};
        door.size = world::kDefaultDoorSize;
        door.openDistance = world::kDefaultDoorOpenDistance;
        withDoors.doors.push_back(door);
        withDoors.doors.push_back(door);
        physics::PhysicsWorld doorWorld;
        Expect(doorWorld.Initialize(withDoors), "Doors Initialize");
        Expect(
            doorWorld.InitializePlayer(withDoors.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "Doors InitializePlayer");
        Expect(doorWorld.StaticBodyCount() == 9, "Doors do not add static Jolt bodies");
        Expect(doorWorld.DynamicBodyCount() == 0, "Doors are not dynamic bodies");
        Expect(doorWorld.DoorBodyCount() == 2, "two kinematic Door bodies");
        Expect(doorWorld.GetDoors().size() == 2, "runtime stores two Doors");
        Expect(!doorWorld.GetDoors()[0].desiredOpen, "unlinked Door stays closed");
        Expect(
            doorWorld.TryRebuild(
                withDoors, withDoors.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild with Doors");
        Expect(doorWorld.DoorBodyCount() == 2, "rebuild still two Door bodies");
        Expect(doorWorld.StaticBodyCount() == 9, "rebuild keeps canonical static count");
        Expect(
            doorWorld.TryRebuild(
                withDoors, withDoors.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "second Door rebuild");
        Expect(doorWorld.DoorBodyCount() == 2, "repeated Door rebuild does not leak bodies");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 3), "inventory seed before TryRebuild");
        physics::PhysicsWorld rebuildWorld;
        Expect(rebuildWorld.Initialize(parsed.level), "inventory isolation Initialize");
        Expect(
            rebuildWorld.InitializePlayer(
                parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "inventory isolation InitializePlayer");
        Expect(
            rebuildWorld.TryRebuild(
                parsed.level,
                parsed.level.initialSpawnVisualCenter,
                world::kPlayerVisualSize),
            "TryRebuild alone does not own Inventory");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild);
        Expect(inventory.GetQuantity("key") == 3, "PhysicsWorld rebuild preserves Inventory");
        Expect(inventory.Entries().size() == 1, "rebuild does not duplicate inventory entries");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d physics rebuild test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Physics rebuild tests passed.\n");
    return 0;
}
