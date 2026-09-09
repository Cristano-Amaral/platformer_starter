// Focused Milestone 52 Pressure Plate overlap harness. Not shipped.
// Exercises PhysicsWorld production overlap. Never writes a file.

#include "physics/DynamicBoxGrab.h"
#include "physics/PhysicsCapacity.h"
#include "physics/PhysicsWorld.h"
#include "physics/PhysicsWorldTestAccess.h"
#include "world/DynamicBox.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/PressurePlate.h"
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

world::DynamicBoxSpec MakeBox(core::Vec3 center, float massKg = 30.0f)
{
    world::DynamicBoxSpec spec{};
    spec.center = center;
    spec.size = {1.0f, 1.0f, 1.0f};
    spec.massKg = massKg;
    return spec;
}

world::PressurePlateSpec MakePlate(core::Vec3 center, core::Vec3 size = world::kDefaultPressurePlateSize)
{
    world::PressurePlateSpec spec{};
    spec.center = center;
    spec.size = size;
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

int ActiveCount(const std::vector<physics::PressurePlateRuntimeState>& plates)
{
    int count = 0;
    for (const physics::PressurePlateRuntimeState& plate : plates)
    {
        if (plate.active)
        {
            ++count;
        }
    }
    return count;
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
    Expect(parsed.level.pressurePlates.empty(), "canonical Level 01 has 0 Pressure Plates");
    Expect(parsed.level.dynamicBoxes.empty(), "canonical Level 01 has 0 Dynamic Boxes");
    Expect(parsed.level.staticProps.empty(), "canonical Level 01 has 0 Static Props");

    const float groundTop = parsed.level.ground.center.y + parsed.level.ground.size.y * 0.5f;
    const core::Vec3 plateCenter{8.0f, groundTop + world::kDefaultPressurePlateSize.y * 0.5f, 0.0f};
    const core::Vec3 onPlate{plateCenter.x, groundTop + 0.5f, 0.0f};
    const core::Vec3 offPlate{24.0f, groundTop + 0.5f, 0.0f};
    const core::Vec3 plateBCenter{16.0f, groundTop + world::kDefaultPressurePlateSize.y * 0.5f, 0.0f};

    Expect(
        world::PressurePlateOverlapsBox(MakePlate(plateCenter), onPlate, {1.0f, 1.0f, 1.0f}),
        "AABB helper overlaps a box sitting on the plate");
    Expect(
        !world::PressurePlateOverlapsBox(MakePlate(plateCenter), offPlate, {1.0f, 1.0f, 1.0f}),
        "AABB helper does not overlap a distant box");
    Expect(
        !world::PressurePlateOverlapsBox(
            MakePlate(plateCenter), parsed.level.initialSpawnVisualCenter, world::kPlayerVisualSize),
        "AABB helper is not a player filter; production never passes the player");

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize plate-only world");
        const int staticBefore = world.StaticBodyCount();
        const int dynamicBefore = world.DynamicBodyCount();
        Expect(world.GetPressurePlates().size() == 1, "one runtime plate");
        Expect(!world.GetPressurePlates()[0].active, "inactive with zero boxes");
        Expect(world.StaticBodyCount() == staticBefore, "plates do not add static bodies");
        Expect(world.DynamicBodyCount() == dynamicBefore, "plates do not add dynamic bodies");
        world.Shutdown();

        world::LevelDefinition withBoxes = level;
        withBoxes.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld withBodies;
        Expect(StartWorld(withBodies, withBoxes), "initialize plate plus box");
        const int staticWithBox = withBodies.StaticBodyCount();
        const int dynamicWithBox = withBodies.DynamicBodyCount();
        withBodies.Shutdown();

        world::LevelDefinition twoPlates = withBoxes;
        twoPlates.pressurePlates.push_back(MakePlate(plateBCenter));
        physics::PhysicsWorld extraPlates;
        Expect(StartWorld(extraPlates, twoPlates), "initialize two plates plus box");
        Expect(extraPlates.StaticBodyCount() == staticWithBox, "extra plates keep static count");
        Expect(extraPlates.DynamicBodyCount() == dynamicWithBox, "extra plates keep dynamic count");
        Expect(
            physics::AuthoredPhysicsBodiesWithinBudget(
                static_cast<int>(twoPlates.elevatedPlatforms.size()),
                static_cast<int>(twoPlates.dynamicBoxes.size()),
                static_cast<int>(twoPlates.doors.size())),
            "plates are outside the authored body leftover");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        level.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize distant box");
        Expect(!world.GetPressurePlates()[0].active, "distant authored box does not activate");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, onPlate, {}, {});
        Expect(world.GetPressurePlates()[0].active, "one overlapping box activates");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, offPlate, {}, {});
        Expect(!world.GetPressurePlates()[0].active, "leaving deactivates");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        level.dynamicBoxes.push_back(MakeBox({onPlate.x + 0.4f, onPlate.y, onPlate.z}));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize two overlapping boxes");
        Expect(world.GetPressurePlates()[0].active, "two overlapping boxes keep Active");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, offPlate, {}, {});
        Expect(world.GetPressurePlates()[0].active, "one remaining overlapping box keeps Active");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 1, offPlate, {}, {});
        Expect(!world.GetPressurePlates()[0].active, "final box leaving deactivates");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        level.pressurePlates.push_back(MakePlate(plateBCenter));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize two independent plates");
        const std::vector<physics::PressurePlateRuntimeState> one = world.GetPressurePlates();
        Expect(one.size() == 2, "two runtime plates");
        Expect(one[0].active && !one[1].active, "only the overlapped plate is Active");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world,
            0,
            {plateBCenter.x, onPlate.y, plateBCenter.z},
            {},
            {});
        const std::vector<physics::PressurePlateRuntimeState> swapped = world.GetPressurePlates();
        Expect(!swapped[0].active && swapped[1].active, "box moving to plate B swaps activation");
        Expect(ActiveCount(swapped) == 1, "exactly one plate active");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(parsed.level.initialSpawnVisualCenter, {2.0f, 2.0f, 2.0f}));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize spawn-overlapping plate");
        Expect(!world.GetPressurePlates()[0].active, "player does not activate");
        world.MovePlayer({1.0f, 0.0f}, kStepSeconds);
        world.Update(kStepSeconds);
        Expect(!world.GetPressurePlates()[0].active, "walking player still does not activate");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        world::StaticPropSpec prop{};
        prop.modelIdentity = "models/test_static.glb";
        prop.position = plateCenter;
        prop.rotationDegrees = world::kDefaultStaticPropRotationDegrees;
        prop.scale = world::kDefaultStaticPropScale;
        level.staticProps.push_back(prop);
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize overlapping Static Prop");
        Expect(!world.GetPressurePlates()[0].active, "Static Prop does not activate");
        Expect(world.DynamicBodyCount() == 0, "Static Prop still creates no Dynamic Box body");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        level.hazards.push_back({{plateCenter.x, plateCenter.y, plateCenter.z}, {2.0f, 1.0f, 2.0f}});
        level.collectibles.push_back({{plateCenter.x, plateCenter.y + 0.6f, plateCenter.z}, {1.0f, 1.2f, 1.0f}});
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize unrelated authored overlap");
        Expect(!world.GetPressurePlates()[0].active, "hazard/collectible overlap does not activate");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        level.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize carry overlap world");
        Expect(!world.GetPressurePlates()[0].active, "carry state alone does not activate");
        const core::Vec3 grabAim{offPlate.x - 1.2f, offPlate.y, offPlate.z};
        StepGrab(world, grabAim, 1.0f, {}, true);
        Expect(world.GetGrabState().carrying, "grabbed distant box");
        Expect(!world.GetPressurePlates()[0].active, "carried distant box still does not activate");

        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, onPlate, {}, {});
        Expect(world.GetPressurePlates()[0].active, "carried box activates through actual overlap");
        Expect(world.GetGrabState().carrying, "still carrying after overlap teleport");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, offPlate, {}, {});
        Expect(!world.GetPressurePlates()[0].active, "carried box leaving overlap deactivates");
        StepGrab(world, world.GetPlayerPhysicsState().visualCenter, 1.0f, {}, true);
        Expect(!world.GetGrabState().carrying, "drop clears carry");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, onPlate, {}, {});
        Expect(world.GetPressurePlates()[0].active, "dropped overlapping box activates");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, offPlate, {}, {});
        Expect(!world.GetPressurePlates()[0].active, "re-grab path: move away deactivates");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize kill-plane recovery world");
        Expect(world.GetPressurePlates()[0].active, "authored-on-plate starts Active");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, {onPlate.x, parsed.level.killPlaneY - 2.0f, onPlate.z}, {0.0f, -8.0f, 0.0f}, {});
        world.Update(kStepSeconds);
        const physics::DynamicBoxRuntimeState recovered = world.GetDynamicBoxes()[0];
        Expect(
            std::fabs(recovered.center.x - onPlate.x) < 0.05f
                && std::fabs(recovered.center.y - onPlate.y) < 0.05f,
            "recovery restores authored overlapping pose");
        Expect(world.GetPressurePlates()[0].active, "recovery onto plate stays Active");

        world::LevelDefinition offAuthored = parsed.level;
        offAuthored.pressurePlates.push_back(MakePlate(plateCenter));
        offAuthored.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld recoveryOff;
        Expect(StartWorld(recoveryOff, offAuthored), "initialize recovery-off-plate world");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            recoveryOff, 0, onPlate, {}, {});
        Expect(recoveryOff.GetPressurePlates()[0].active, "teleport onto plate activates");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            recoveryOff,
            0,
            {offPlate.x, offAuthored.killPlaneY - 2.0f, offPlate.z},
            {0.0f, -8.0f, 0.0f},
            {});
        recoveryOff.Update(kStepSeconds);
        Expect(
            !recoveryOff.GetPressurePlates()[0].active,
            "kill-plane recovery cannot leave stale Active");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        level.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize restart world");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, onPlate, {}, {});
        Expect(world.GetPressurePlates()[0].active, "runtime overlap before restart");
        world.ResetDynamicBoxes();
        Expect(!world.GetPressurePlates()[0].active, "Restart recomputes from authored poses");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize checkpoint world");
        Expect(world.GetPressurePlates()[0].active, "box remains on plate");
        world.ResetCharacter(level.initialSpawnVisualCenter, {});
        Expect(world.GetDynamicBoxes()[0].center.x == onPlate.x
                || std::fabs(world.GetDynamicBoxes()[0].center.x - onPlate.x) < 0.05f,
            "checkpoint respawn does not globally reset boxes");
        Expect(world.GetPressurePlates()[0].active, "checkpoint respawn keeps overlap Active");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize rebuild world");
        Expect(world.GetPressurePlates()[0].active, "pre-rebuild Active");
        world::LevelDefinition rebuilt = parsed.level;
        rebuilt.pressurePlates.push_back(MakePlate(plateBCenter));
        rebuilt.dynamicBoxes.push_back(MakeBox(onPlate));
        Expect(
            world.TryRebuild(
                rebuilt, rebuilt.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild with moved plate");
        Expect(world.GetPressurePlates().size() == 1, "rebuild has one plate");
        Expect(
            !world.GetPressurePlates()[0].active,
            "rebuild drops stale overlap against the old plate AABB");
        Expect(
            std::fabs(world.GetPressurePlates()[0].center.x - plateBCenter.x) < 0.001f,
            "rebuild uses active authored plate center");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.pressurePlates.push_back(MakePlate(plateCenter));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize workingCopy isolation world");
        Expect(world.GetPressurePlates()[0].center.x == plateCenter.x, "runtime uses applied center");
        level.pressurePlates[0].center.x = plateBCenter.x;
        Expect(
            world.GetPressurePlates()[0].center.x == plateCenter.x,
            "mutating the caller's LevelDefinition does not change runtime before rebuild");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Pressure Plate test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Pressure Plate tests passed.\n");
    return 0;
}
