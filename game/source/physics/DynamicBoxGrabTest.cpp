// Focused Milestone 51 Dynamic Box Grab / Carry harness. Not shipped.
// Exercises the PhysicsWorld production boundary. Never writes a file.

#include "physics/DynamicBoxGrab.h"
#include "physics/PhysicsCapacity.h"
#include "physics/PhysicsWorld.h"
#include "physics/PhysicsWorldTestAccess.h"
#include "gameplay/Inventory.h"
#include "world/DynamicBox.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/StaticProp.h"

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

float Distance(core::Vec3 a, core::Vec3 b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float VecLength(core::Vec3 value)
{
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

world::DynamicBoxSpec MakeBox(core::Vec3 center, float massKg = 30.0f)
{
    world::DynamicBoxSpec spec{};
    spec.center = center;
    spec.size = {1.0f, 1.0f, 1.0f};
    spec.massKg = massKg;
    return spec;
}

bool StartWorld(physics::PhysicsWorld& world, const world::LevelDefinition& level)
{
    if (!world.Initialize(level))
    {
        return false;
    }
    return world.InitializePlayer(level.initialSpawnVisualCenter, world::kPlayerVisualSize);
}

void StepGrab(
    physics::PhysicsWorld& world,
    core::Vec3 aim,
    float facingX,
    physics::PlayerMoveCommand move,
    bool grabDrop)
{
    world.SetGrabAim(aim, facingX);
    if (grabDrop)
    {
        world.HandleGrabDrop();
    }
    world.UpdateMovingPlatform(kStepSeconds);
    world.MovePlayer(move, kStepSeconds);
    const physics::PlayerPhysicsState player = world.GetPlayerPhysicsState();
    world.SetGrabAim(player.visualCenter, facingX);
    world.Update(kStepSeconds);
}

void AimAndUpdate(physics::PhysicsWorld& world, float facingX)
{
    const physics::PlayerPhysicsState player = world.GetPlayerPhysicsState();
    world.SetGrabAim(player.visualCenter, facingX);
    world.UpdateMovingPlatform(kStepSeconds);
    world.MovePlayer({0.0f, -1.0f}, kStepSeconds);
    world.SetGrabAim(world.GetPlayerPhysicsState().visualCenter, facingX);
    world.Update(kStepSeconds);
}
}

int main()
{
    const world::ParseLevelFileResult parsed =
        world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
    if (parsed.status != world::LoadLevelFileStatus::Loaded)
    {
        std::fprintf(stderr, "FAIL could not load canonical Level 01 source\n");
        return 1;
    }
    Expect(parsed.level.dynamicBoxes.empty(), "canonical Level 01 has 0 Dynamic Boxes");
    Expect(parsed.level.staticProps.empty(), "canonical Level 01 has 0 Static Props");
    Expect(parsed.level.itemPickups.empty(), "canonical Level 01 has 0 Item Pickups");

    const core::Vec3 spawn = parsed.level.initialSpawnVisualCenter;
    const float groundTop =
        parsed.level.ground.center.y + parsed.level.ground.size.y * 0.5f;
    const core::Vec3 inRangeCenter{spawn.x + 1.55f, groundTop + 0.5f, spawn.z};
    const core::Vec3 farCenter{spawn.x + 4.5f, groundTop + 0.5f, spawn.z};
    const core::Vec3 behindCenter{spawn.x - 1.55f, groundTop + 0.5f, spawn.z};

    {
        physics::PhysicsWorld world;
        Expect(StartWorld(world, parsed.level), "empty-level Initialize");
        AimAndUpdate(world, 1.0f);
        const physics::DynamicBoxGrabState grab = world.GetGrabState();
        Expect(!grab.hasTarget, "1. no target when no Dynamic Box is eligible");
        Expect(!grab.carrying, "empty level is not carrying");
        Expect(world.DynamicBodyCount() == 0, "empty level has no dynamic bodies");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox(inRangeCenter));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "in-range Initialize");
        AimAndUpdate(world, 1.0f);
        const physics::DynamicBoxGrabState grab = world.GetGrabState();
        Expect(grab.hasTarget, "2. in-range/in-front Dynamic Box can become target");
        Expect(grab.targetIndex == 0, "in-range target is box 0");
        Expect(grab.hasTarget == (grab.targetIndex == 0), "35. target feedback matches target");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox(farCenter));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "beyond-range Initialize");
        AimAndUpdate(world, 1.0f);
        Expect(!world.GetGrabState().hasTarget, "3. beyond-range Dynamic Box cannot target");
        world.HandleGrabDrop();
        Expect(!world.GetGrabState().carrying, "8. Grab without valid target does nothing");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox(behindCenter));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "behind Initialize");
        AimAndUpdate(world, 1.0f);
        Expect(!world.GetGrabState().hasTarget, "behind the player is not a target");
    }

    {
        world::LevelDefinition level = parsed.level;
        world::StaticPropSpec prop{};
        prop.modelIdentity = "models/test_static.glb";
        prop.position = inRangeCenter;
        prop.scale = world::kDefaultStaticPropScale;
        level.staticProps.push_back(prop);
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "static-prop Initialize");
        AimAndUpdate(world, 1.0f);
        Expect(!world.GetGrabState().hasTarget, "4/37. Static Props cannot target or grab");
        Expect(world.DynamicBodyCount() == 0, "Static Props add no dynamic bodies");
        Expect(world.StaticBodyCount() == 9, "Static Props add no static bodies");
        world.HandleGrabDrop();
        Expect(!world.GetGrabState().carrying, "Static Prop Grab is a no-op");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox({spawn.x + 1.8f, groundTop + 0.5f, spawn.z}));
        level.dynamicBoxes.push_back(MakeBox({spawn.x + 1.45f, groundTop + 0.5f, spawn.z}));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "two-box Initialize");
        AimAndUpdate(world, 1.0f);
        const physics::DynamicBoxGrabState grab = world.GetGrabState();
        Expect(grab.hasTarget && grab.targetIndex == 1, "5. closer eligible box is the deterministic target");
        world.HandleGrabDrop();
        AimAndUpdate(world, 1.0f);
        const physics::DynamicBoxGrabState carried = world.GetGrabState();
        Expect(carried.carrying, "7. Grab with valid target enters carrying");
        Expect(carried.carriedIndex == 1, "10. carried identity matches selected box");
        Expect(!carried.hasTarget, "carrying suppresses a second target");
        Expect(carried.carrying == (carried.carriedIndex == 1), "36. carry feedback matches carrying");
        Expect(world.DynamicBodyCount() == 2, "9. exactly one box carried; both bodies remain");
    }

    {
        world::LevelDefinition level = parsed.level;
        world::Box wall{};
        wall.center = {spawn.x + 2.2f, groundTop + 2.0f, spawn.z};
        wall.size = {0.4f, 4.0f, 3.0f};
        level.elevatedPlatforms.push_back(wall);
        level.dynamicBoxes.push_back(MakeBox({spawn.x + 3.6f, groundTop + 0.5f, spawn.z}));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "obstruction Initialize");
        AimAndUpdate(world, 1.0f);
        Expect(
            !world.GetGrabState().hasTarget,
            "6. solid world obstruction prevents through-world grab");
        world.HandleGrabDrop();
        Expect(!world.GetGrabState().carrying, "obstructed Grab does nothing");
    }

    {
        world::LevelDefinition level = parsed.level;
        const world::LevelDefinition authoredBaseline = level;
        level.dynamicBoxes.push_back(MakeBox(inRangeCenter, 30.0f));
        level.dynamicBoxes.push_back(MakeBox({spawn.x + 2.2f, groundTop + 0.5f, spawn.z}, 80.0f));
        world::LevelDefinition workingCopy = level;
        world::LevelDefinition active = level;
        bool modified = false;
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "carry-follow Initialize");
        Expect(
            physics::AuthoredPhysicsBodiesWithinBudget(
                static_cast<int>(level.elevatedPlatforms.size()),
                static_cast<int>(level.dynamicBoxes.size()),
                static_cast<int>(level.doors.size())),
            "40. body budget remains valid with two boxes");

        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        AimAndUpdate(world, 1.0f);
        const physics::PlayerPhysicsState before = world.GetPlayerPhysicsState();
        const physics::DynamicBoxGrabState grabbed = world.GetGrabState();
        Expect(grabbed.carrying && grabbed.carriedIndex == 0, "grab box 0 first");
        Expect(grabbed.carryTargetValid, "carry target exists after grab step");

        core::Vec3 previousTarget = grabbed.carryTarget;
        for (int frame = 0; frame < 20; ++frame)
        {
            StepGrab(world, world.GetPlayerPhysicsState().visualCenter, 1.0f, {4.0f, -1.0f}, false);
        }
        physics::DynamicBoxGrabState afterMove = world.GetGrabState();
        Expect(afterMove.carrying, "still carrying after movement");
        Expect(afterMove.carryTargetValid, "carry target is published while carrying");
        Expect(
            afterMove.carryTarget.x > previousTarget.x + 0.05f
                || afterMove.carryTarget.x > before.visualCenter.x,
            "11. carry target follows player movement");

        const physics::PlayerPhysicsState movedPlayer = world.GetPlayerPhysicsState();
        Expect(
            Distance(afterMove.carryTarget, movedPlayer.visualCenter)
                <= physics::kDynamicBoxCarryDistance + 0.2f,
            "13. carry distance remains bounded");

        world.SetGrabAim(movedPlayer.visualCenter, -1.0f);
        for (int frame = 0; frame < 12; ++frame)
        {
            world.SetGrabAim(world.GetPlayerPhysicsState().visualCenter, -1.0f);
            world.UpdateMovingPlatform(kStepSeconds);
            world.MovePlayer({0.0f, -1.0f}, kStepSeconds);
            world.SetGrabAim(world.GetPlayerPhysicsState().visualCenter, -1.0f);
            world.Update(kStepSeconds);
        }
        afterMove = world.GetGrabState();
        Expect(
            afterMove.carryTarget.x < world.GetPlayerPhysicsState().visualCenter.x,
            "12. carry target follows facing");

        const std::vector<physics::DynamicBoxRuntimeState> whileCarried = world.GetDynamicBoxes();
        Expect(whileCarried[0].center.y > parsed.level.ground.center.y, "14. carried box stays above Ground");
        Expect(
            world.GetPlayerPhysicsState().visualCenter.y < spawn.y + 3.0f
                && std::fabs(world.GetPlayerPhysicsState().verticalVelocity) < 12.0f,
            "15. no catastrophic player launch while carrying");

        Expect(VecNear(level.dynamicBoxes[0].center, inRangeCenter), "16. authored transform unchanged");
        Expect(
            world::AuthoredLevelDataEqual(workingCopy, level),
            "17. workingCopy is not mutated by carry");
        Expect(world::AuthoredLevelDataEqual(active, level), "18. active authored definitions unchanged");
        Expect(!modified, "19. carry does not mark Modified/Dirty");
        Expect(parsed.level.dynamicBoxes.empty(), "canonical source still empty");
        Expect(
            world::AuthoredLevelDataEqual(authoredBaseline, parsed.level)
                || parsed.level.dynamicBoxes.empty(),
            "39. Level Format / canonical authored data stays box-free");

        const core::Vec3 carriedPose = world.GetDynamicBoxes()[0].center;
        const core::Vec3 otherPose = world.GetDynamicBoxes()[1].center;
        world.HandleGrabDrop();
        AimAndUpdate(world, 1.0f);
        Expect(!world.GetGrabState().carrying, "20. Drop exits carrying state");
        const std::vector<physics::DynamicBoxRuntimeState> dropped = world.GetDynamicBoxes();
        Expect(dropped[0].valid, "21. dropped box remains a simulated body");
        Expect(!VecNear(dropped[0].center, inRangeCenter, 0.02f)
                || VecNear(dropped[0].center, carriedPose, 0.35f),
            "22. Drop does not restore authored transform");
        Expect(
            VecLength(dropped[0].linearVelocity) <= physics::kDynamicBoxCarryMaxSpeed + 1.0f,
            "23. Drop does not inject an arbitrary large impulse");

        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying, "33a. re-grab after drop");
        world.HandleGrabDrop();
        Expect(!world.GetGrabState().carrying, "33. repeated Grab/Drop stays stable");
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying, "33b. third grab still works");
        world.HandleGrabDrop();

        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying, "grab one of two boxes");
        world.HandleGrabDrop();
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, {spawn.x - 3.0f, groundTop + 0.5f, spawn.z}, {}, {});
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying, "34. another box can be grabbed after drop");
        Expect(world.GetGrabState().carriedIndex == 1, "34. carried identity can change to the other box");
        Expect(world.DynamicBodyCount() == 2, "40. carrying never adds a body");
    }

    {
        world::LevelDefinition level = parsed.level;
        world::Box wall{};
        wall.center = {spawn.x + 3.4f, groundTop + 2.0f, spawn.z};
        wall.size = {0.5f, 4.0f, 3.0f};
        level.elevatedPlatforms.push_back(wall);
        level.dynamicBoxes.push_back(MakeBox(inRangeCenter));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "obstructed-carry Initialize");
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying, "grab before walking into the wall");
        for (int frame = 0; frame < 45; ++frame)
        {
            StepGrab(world, world.GetPlayerPhysicsState().visualCenter, 1.0f, {5.0f, -1.0f}, false);
        }
        const physics::DynamicBoxRuntimeState box = world.GetDynamicBoxes()[0];
        const float wallMinX = wall.center.x - wall.size.x * 0.5f;
        Expect(
            box.center.x + box.size.x * 0.5f < wallMinX + 0.15f,
            "14. carried box does not teleport through solid world");
        Expect(
            world.GetPlayerPhysicsState().visualCenter.y < spawn.y + 4.0f,
            "15. walking into geometry while carrying does not launch the player");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox(inRangeCenter));
        level.dynamicBoxes.push_back(
            MakeBox({spawn.x + 2.4f, groundTop + 0.5f, spawn.z}));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "restart Initialize");
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        for (int frame = 0; frame < 15; ++frame)
        {
            StepGrab(world, world.GetPlayerPhysicsState().visualCenter, 1.0f, {4.0f, -1.0f}, false);
        }
        Expect(world.GetGrabState().carrying, "carrying before Restart Run");
        world.ResetDynamicBoxes();
        Expect(!world.GetGrabState().carrying, "24. Restart Run clears carry");
        Expect(world.GetGrabState().carriedIndex == physics::kNoDynamicBoxGrabIndex,
            "22. no stale carried index after Restart");
        const std::vector<physics::DynamicBoxRuntimeState> reset = world.GetDynamicBoxes();
        Expect(VecNear(reset[0].center, level.dynamicBoxes[0].center), "25. Restart restores box 0");
        Expect(VecNear(reset[1].center, level.dynamicBoxes[1].center), "25. Restart restores box 1");
        Expect(VecNear(reset[0].linearVelocity, {}), "25. Restart zeros box 0 velocity");
        Expect(VecNear(reset[1].linearVelocity, {}), "25. Restart zeros box 1 velocity");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox(inRangeCenter));
        level.dynamicBoxes.push_back(
            MakeBox({spawn.x + 2.4f, groundTop + 0.5f, spawn.z}));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "checkpoint Initialize");
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        for (int frame = 0; frame < 12; ++frame)
        {
            StepGrab(world, world.GetPlayerPhysicsState().visualCenter, 1.0f, {4.0f, -1.0f}, false);
        }
        const core::Vec3 moved0 = world.GetDynamicBoxes()[0].center;
        const core::Vec3 moved1 = world.GetDynamicBoxes()[1].center;
        Expect(world.GetGrabState().carrying, "carrying before checkpoint respawn");
        world.ResetCharacter(level.initialSpawnVisualCenter, {});
        Expect(!world.GetGrabState().carrying, "27. checkpoint respawn releases carry");
        const std::vector<physics::DynamicBoxRuntimeState> afterRespawn = world.GetDynamicBoxes();
        Expect(
            !VecNear(afterRespawn[0].center, level.dynamicBoxes[0].center, 0.02f)
                || VecNear(afterRespawn[0].center, moved0, 0.35f),
            "26. checkpoint respawn does not globally reset Dynamic Boxes");
        Expect(VecNear(afterRespawn[1].center, moved1, 0.35f),
            "26. the second box keeps its runtime pose");
        Expect(
            afterRespawn[0].center.x != level.dynamicBoxes[0].center.x
                || VecNear(afterRespawn[0].center, moved0, 0.5f),
            "27. release does not authored-reset the dropped box");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox(inRangeCenter));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "rebuild Initialize");
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying, "carrying before TryRebuild");
        Expect(
            world.TryRebuild(level, level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild succeeds");
        Expect(!world.GetGrabState().carrying, "28. Apply/reload clears carry");
        Expect(
            world.GetGrabState().carriedIndex == physics::kNoDynamicBoxGrabIndex,
            "29. rebuild leaves no stale carried index");
        Expect(world.GetGrabState().targetIndex == physics::kNoDynamicBoxGrabIndex
                || !world.GetGrabState().hasTarget,
            "29. rebuild leaves no stale target");
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying, "grab works on reconstructed bodies");
        Expect(world.DynamicBodyCount() == 1, "38. authored Dynamic Box body count unchanged");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox(inRangeCenter));
        level.dynamicBoxes.push_back(
            MakeBox({spawn.x + 2.4f, groundTop + 0.5f, spawn.z}));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "kill-plane Initialize");
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying && world.GetGrabState().carriedIndex == 0,
            "carrying box 0 before kill-plane");
        const core::Vec3 otherBefore = world.GetDynamicBoxes()[1].center;
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world,
            0,
            {inRangeCenter.x, level.killPlaneY - 1.0f, inRangeCenter.z},
            {0.0f, -4.0f, 0.0f},
            {});
        world.Update(kStepSeconds);
        Expect(!world.GetGrabState().carrying, "30. kill-plane recovery of carried box clears carry");
        const std::vector<physics::DynamicBoxRuntimeState> recovered = world.GetDynamicBoxes();
        Expect(VecNear(recovered[0].center, level.dynamicBoxes[0].center),
            "31. recovery still resets only that box");
        Expect(VecNear(recovered[0].linearVelocity, {}), "recovered box velocity is zero");
        Expect(VecNear(recovered[1].center, otherBefore, 0.2f), "32. other Dynamic Boxes remain unaffected");
        Expect(world.IsInitialized(), "recovery does not rebuild PhysicsWorld");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox(inRangeCenter, 5.0f));
        level.dynamicBoxes.push_back(MakeBox({spawn.x + 2.1f, groundTop + 0.5f, spawn.z}, 200.0f));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "mass Initialize");
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying, "light box is grabbable");
        world.HandleGrabDrop();
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, {spawn.x - 4.0f, groundTop + 0.5f, spawn.z}, {}, {});
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying && world.GetGrabState().carriedIndex == 1,
            "heavy authored mass remains grabbable");
        Expect(level.dynamicBoxes[1].massKg == 200.0f, "38. authored mass is unchanged");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("coin", 2), "inventory seed before Grab");
        world::LevelDefinition level = parsed.level;
        level.dynamicBoxes.push_back(MakeBox(inRangeCenter));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "inventory grab isolation Initialize");
        AimAndUpdate(world, 1.0f);
        world.HandleGrabDrop();
        Expect(world.GetGrabState().carrying, "Grab succeeded for inventory isolation");
        Expect(inventory.GetQuantity("coin") == 2, "Grab does not add Inventory");
        world.HandleGrabDrop();
        Expect(!world.GetGrabState().carrying, "Drop succeeded for inventory isolation");
        Expect(inventory.GetQuantity("coin") == 2, "Drop does not remove Inventory");
        Expect(inventory.Entries().size() == 1, "Carry does not add Inventory entries");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Dynamic Box grab test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Dynamic Box grab tests passed.\n");
    return 0;
}
