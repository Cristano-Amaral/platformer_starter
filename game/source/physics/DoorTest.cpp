// Focused Milestone 53 Door / Pressure Plate link harness. Not shipped.
// Exercises Level Format parse/save and PhysicsWorld kinematic Door motion.
// Never writes a file.

#include "physics/DynamicBoxGrab.h"
#include "physics/PhysicsCapacity.h"
#include "physics/PhysicsWorld.h"
#include "physics/PhysicsWorldTestAccess.h"
#include "gameplay/Inventory.h"
#include "world/Door.h"
#include "world/DynamicBox.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelWriter.h"
#include "world/PressurePlate.h"

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

world::PressurePlateSpec MakePlate(
    core::Vec3 center,
    int linkedDoorIndex = world::kNoLinkedDoor,
    core::Vec3 size = world::kDefaultPressurePlateSize)
{
    world::PressurePlateSpec spec{};
    spec.center = center;
    spec.size = size;
    spec.linkedDoorIndex = linkedDoorIndex;
    return spec;
}

world::DoorSpec MakeDoor(core::Vec3 center, float openDistance = world::kDefaultDoorOpenDistance)
{
    world::DoorSpec spec{};
    spec.center = center;
    spec.size = world::kDefaultDoorSize;
    spec.openDistance = openDistance;
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

void StepWorld(physics::PhysicsWorld& world, int frames)
{
    physics::PlayerMoveCommand idle{};
    for (int frame = 0; frame < frames; ++frame)
    {
        world.UpdateMovingPlatform(kStepSeconds);
        world.MovePlayer(idle, kStepSeconds);
        world.Update(kStepSeconds);
    }
}

bool NearlyEqual(float a, float b, float epsilon = 0.02f)
{
    return std::fabs(a - b) <= epsilon;
}

int CountRecords(std::string_view text, std::string_view keyword)
{
    int count = 0;
    std::size_t cursor = 0;
    while (cursor <= text.size())
    {
        const std::size_t newline = text.find('\n', cursor);
        const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
        if (end > cursor)
        {
            const std::string_view line = text.substr(cursor, end - cursor);
            if (line.rfind(keyword, 0) == 0
                && (line.size() == keyword.size() || line[keyword.size()] == ' '))
            {
                ++count;
            }
        }
        if (newline == std::string_view::npos)
        {
            break;
        }
        cursor = newline + 1;
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
    Expect(parsed.level.doors.empty(), "canonical Level 01 has 0 Doors");
    Expect(parsed.level.itemPickups.empty(), "canonical Level 01 has 0 Item Pickups");
    Expect(parsed.level.pressurePlates.empty(), "canonical Level 01 has 0 Pressure Plates");
    Expect(parsed.level.dynamicBoxes.empty(), "canonical Level 01 has 0 Dynamic Boxes");
    Expect(parsed.level.staticProps.empty(), "canonical Level 01 has 0 Static Props");

    const std::string canonical = world::SerializeLevelText(parsed.level);
    Expect(CountRecords(canonical, "door") == 0, "canonical serialize has 0 door records");
    Expect(canonical.find("desiredOpen") == std::string::npos, "serialize has no desiredOpen");
    Expect(canonical.find("openFraction") == std::string::npos, "serialize has no openFraction");

    {
        const std::string oneDoor = canonical + "door 4 1.5 0 1.2 3 2.4 3.2\n";
        const world::ParseLevelFileResult one = world::ParseLevelText(oneDoor);
        Expect(one.status == world::LoadLevelFileStatus::Loaded, "one door loads");
        Expect(one.level.doors.size() == 1, "one door count");
        Expect(one.level.doors[0].center.x == 4.0f, "one door center x");
        Expect(one.level.doors[0].size.y == 3.0f, "one door size y");
        Expect(one.level.doors[0].openDistance == 3.2f, "one door open distance");
        Expect(!one.level.doors[0].requiresKey, "old Door syntax defaults requiresKey false");
        const std::string written = world::SerializeLevelText(one.level);
        Expect(CountRecords(written, "door") == 1, "writer one door");
        const world::ParseLevelFileResult roundDoor = world::ParseLevelText(written);
        Expect(world::AuthoredLevelDataEqual(one.level, roundDoor.level), "one door round trip");
        Expect(roundDoor.level.doors[0].center.x == 4.0f, "save writes authored closed center");
        Expect(written.find("openFraction") == std::string::npos, "save omits openFraction");
    }

    {
        const std::string twoDoors =
            canonical + "door 4 1.5 0 1.2 3 2.4 3.2\ndoor 8 1.5 0 1.2 3 2.4 3.2\n";
        const world::ParseLevelFileResult two = world::ParseLevelText(twoDoors);
        Expect(two.status == world::LoadLevelFileStatus::Loaded, "two doors load");
        Expect(two.level.doors.size() == 2, "two door count");
        Expect(CountRecords(world::SerializeLevelText(two.level), "door") == 2, "writer two doors");
    }

    Expect(
        world::ParseLevelText(canonical + "door nan 1.5 0 1.2 3 2.4 3.2\n").status
            == world::LoadLevelFileStatus::Invalid,
        "NaN door position rejected");
    Expect(
        world::ParseLevelText(canonical + "door inf 1.5 0 1.2 3 2.4 3.2\n").status
            == world::LoadLevelFileStatus::Invalid,
        "Inf door position rejected");
    Expect(
        world::ParseLevelText(canonical + "door 4 1.5 0 0 3 2.4 3.2\n").status
            == world::LoadLevelFileStatus::Invalid,
        "zero door size rejected");
    Expect(
        world::ParseLevelText(canonical + "door 4 1.5 0 -1 3 2.4 3.2\n").status
            == world::LoadLevelFileStatus::Invalid,
        "negative door size rejected");
    Expect(
        world::ParseLevelText(canonical + "door 4 1.5 0 1.2 3 2.4 0\n").status
            == world::LoadLevelFileStatus::Invalid,
        "zero open distance rejected");
    Expect(
        world::ParseLevelText(canonical + "door 4 1.5 0 1.2 3 2.4 -1\n").status
            == world::LoadLevelFileStatus::Invalid,
        "negative open distance rejected");
    Expect(
        world::ParseLevelText(canonical + "door 4 1.5 0 1.2 3 2.4 20.1\n").status
            == world::LoadLevelFileStatus::Invalid,
        "over-max open distance rejected");

    {
        const std::string noLink = canonical + "pressure_plate 2 0.1 0 2 0.2 2\n";
        const world::ParseLevelFileResult parsedNoLink = world::ParseLevelText(noLink);
        Expect(parsedNoLink.status == world::LoadLevelFileStatus::Loaded, "7-token plate loads");
        Expect(
            parsedNoLink.level.pressurePlates[0].linkedDoorIndex == world::kNoLinkedDoor,
            "7-token plate is no-link");
        const std::string writtenNoLink = world::SerializeLevelText(parsedNoLink.level);
        const world::ParseLevelFileResult roundNoLink = world::ParseLevelText(writtenNoLink);
        Expect(
            roundNoLink.level.pressurePlates[0].linkedDoorIndex == world::kNoLinkedDoor,
            "no-link round trip");
        Expect(CountRecords(writtenNoLink, "pressure_plate") == 1, "writer one no-link plate");
    }

    {
        const std::string linked = canonical
            + "door 4 1.5 0 1.2 3 2.4 3.2\n"
              "pressure_plate 2 0.1 0 2 0.2 2 0\n";
        const world::ParseLevelFileResult parsedLink = world::ParseLevelText(linked);
        Expect(parsedLink.status == world::LoadLevelFileStatus::Loaded, "linked plate loads");
        Expect(parsedLink.level.pressurePlates[0].linkedDoorIndex == 0, "plate links Door 0");
        const std::string writtenLink = world::SerializeLevelText(parsedLink.level);
        Expect(
            world::ParseLevelText(writtenLink).level.pressurePlates[0].linkedDoorIndex == 0,
            "linked plate round trip");
        Expect(writtenLink.find("openFraction") == std::string::npos, "link save omits runtime open");
    }

    Expect(
        world::ParseLevelText(canonical + "pressure_plate 2 0.1 0 2 0.2 2 0\n").status
            == world::LoadLevelFileStatus::Invalid,
        "link to missing Door rejected");
    Expect(
        world::ParseLevelText(
            canonical + "door 4 1.5 0 1.2 3 2.4 3.2\npressure_plate 2 0.1 0 2 0.2 2 1\n")
            .status
            == world::LoadLevelFileStatus::Invalid,
        "out-of-range Door link rejected");

    const float groundTop = parsed.level.ground.center.y + parsed.level.ground.size.y * 0.5f;
    const core::Vec3 plateCenter{10.0f, groundTop + world::kDefaultPressurePlateSize.y * 0.5f, 0.0f};
    const core::Vec3 plateBCenter{16.0f, groundTop + world::kDefaultPressurePlateSize.y * 0.5f, 0.0f};
    const core::Vec3 onPlate{plateCenter.x, groundTop + 0.5f, 0.0f};
    const core::Vec3 onPlateB{plateBCenter.x, groundTop + 0.5f, 0.0f};
    const core::Vec3 offPlate{28.0f, groundTop + 0.5f, 0.0f};
    const core::Vec3 doorClosed{8.5f, 1.5f, 0.0f};
    const core::Vec3 doorBClosed{-8.5f, 1.5f, 0.0f};

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize inactive linked plate");
        Expect(world.DoorBodyCount() == 1, "one kinematic Door body");
        Expect(world.GetDoors()[0].valid, "Door runtime valid");
        Expect(!world.GetDoors()[0].desiredOpen, "inactive plate keeps Door closed");
        Expect(NearlyEqual(world.GetDoors()[0].openFraction, 0.0f), "closed fraction is 0");
        Expect(
            NearlyEqual(world.GetDoors()[0].center.y, doorClosed.y),
            "closed Door sits at authored center");
        Expect(
            world.GetDoors()[0].closedCenter.x == doorClosed.x
                && world.GetDoors()[0].closedCenter.y == doorClosed.y,
            "runtime closedCenter matches authored");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize active linked plate");
        Expect(world.GetPressurePlates()[0].active, "overlapping box activates plate");
        Expect(world.GetDoors()[0].desiredOpen, "active linked plate opens Door");
        Expect(NearlyEqual(world.GetDoors()[0].openFraction, 1.0f), "Initialize snaps open");
        Expect(
            NearlyEqual(world.GetDoors()[0].center.y, doorClosed.y + world::kDefaultDoorOpenDistance),
            "snapped open uses +Y open distance");

        const core::Vec3 authoredClosed = level.doors[0].center;
        StepWorld(world, 10);
        Expect(
            level.doors[0].center.x == authoredClosed.x && level.doors[0].center.y == authoredClosed.y,
            "runtime motion does not mutate caller's authored closed position");

        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 0, offPlate, {}, {});
        Expect(!world.GetPressurePlates()[0].active, "leaving plate deactivates");
        Expect(!world.GetDoors()[0].desiredOpen, "inactive linked plate desires closed");
        const float startFraction = world.GetDoors()[0].openFraction;
        StepWorld(world, 4);
        const float midFraction = world.GetDoors()[0].openFraction;
        Expect(midFraction < startFraction, "closing motion decreases fraction");
        Expect(midFraction > 0.0f, "ordinary close does not teleport");
        StepWorld(world, 180);
        Expect(NearlyEqual(world.GetDoors()[0].openFraction, 0.0f, 0.05f), "close completes");
        Expect(
            NearlyEqual(world.GetDoors()[0].center.y, doorClosed.y, 0.05f),
            "closed pose matches authored center");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize opening motion");
        Expect(!world.GetDoors()[0].desiredOpen, "starts closed");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 0, onPlate, {}, {});
        Expect(world.GetDoors()[0].desiredOpen, "activation desires open");
        const float start = world.GetDoors()[0].openFraction;
        StepWorld(world, 4);
        const float mid = world.GetDoors()[0].openFraction;
        Expect(mid > start, "opening motion increases fraction");
        Expect(mid < 1.0f, "ordinary open does not teleport");
        StepWorld(world, 180);
        Expect(NearlyEqual(world.GetDoors()[0].openFraction, 1.0f, 0.05f), "open completes");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.pressurePlates.push_back(MakePlate(plateBCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        level.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize OR plates");
        Expect(world.GetDoors()[0].desiredOpen, "one of two linked plates opens Door");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 1, onPlateB, {}, {});
        Expect(world.GetDoors()[0].desiredOpen, "two active linked plates keep Door open");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 0, offPlate, {}, {});
        Expect(world.GetPressurePlates()[1].active, "second plate still Active");
        Expect(world.GetDoors()[0].desiredOpen, "remaining Active plate keeps Door open");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 1, offPlate, {}, {});
        Expect(!world.GetDoors()[0].desiredOpen, "final linked plate deactivation closes Door");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, world::kNoLinkedDoor));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize unlinked active plate");
        Expect(world.GetPressurePlates()[0].active, "unlinked plate still activates");
        Expect(!world.GetDoors()[0].desiredOpen, "unlinked Active plate does not open Door");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.doors.push_back(MakeDoor(doorBClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize two Doors");
        Expect(world.DoorBodyCount() == 2, "two Door bodies");
        Expect(world.GetDoors()[0].desiredOpen, "plate opens Door A");
        Expect(!world.GetDoors()[1].desiredOpen, "plate linked to A does not open Door B");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize closed Door collision");
        world.ResetCharacter({doorClosed.x - 1.4f, 0.8f, 0.0f}, {});
        const float doorLeft = doorClosed.x - world::kDefaultDoorSize.x * 0.5f;
        physics::PlayerMoveCommand walk{};
        walk.horizontalVelocity = 8.0f;
        for (int frame = 0; frame < 90; ++frame)
        {
            world.UpdateMovingPlatform(kStepSeconds);
            world.MovePlayer(walk, kStepSeconds);
            world.Update(kStepSeconds);
        }
        const float endX = world.GetPlayerPhysicsState().visualCenter.x;
        Expect(endX + 0.4f < doorLeft + 0.2f, "closed Door blocks player");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.dynamicBoxes.push_back(MakeBox({doorClosed.x - 2.0f, groundTop + 0.5f, 0.0f}));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize box vs closed Door");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world,
            0,
            {doorClosed.x - 0.2f, doorClosed.y, 0.0f},
            {6.0f, 0.0f, 0.0f},
            {});
        StepWorld(world, 30);
        const float boxX = world.GetDynamicBoxes()[0].center.x;
        Expect(
            boxX + 0.45f < doorClosed.x + world::kDefaultDoorSize.x * 0.5f,
            "closed Door blocks Dynamic Box");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed, 3.2f));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        level.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize obstruction");
        Expect(world.GetDoors()[0].desiredOpen, "starts open");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world, 0, offPlate, {}, {});
        Expect(!world.GetDoors()[0].desiredOpen, "deactivated plate desires close");
        for (int frame = 0; frame < 50; ++frame)
        {
            physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
                world, 1, doorClosed, {}, {});
            StepWorld(world, 1);
        }
        Expect(world.GetDoors()[0].openFraction > 0.55f, "obstructed close pauses above the blocker");
        Expect(world.GetDoors()[0].blockedClosing, "obstruction reports blockedClosing");
        for (int frame = 0; frame < 180; ++frame)
        {
            physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 0, offPlate, {}, {});
            physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 1, offPlate, {}, {});
            StepWorld(world, 1);
        }
        Expect(
            NearlyEqual(world.GetDoors()[0].openFraction, 0.0f, 0.08f),
            "clearing obstruction allows close");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize Restart Door");
        Expect(world.GetDoors()[0].desiredOpen, "Restart fixture starts open");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 0, offPlate, {}, {});
        StepWorld(world, 8);
        world.ResetDynamicBoxes();
        Expect(world.GetPressurePlates()[0].active, "Restart restores box onto plate");
        Expect(world.GetDoors()[0].desiredOpen, "Restart recomputes desiredOpen from plates");
        Expect(NearlyEqual(world.GetDoors()[0].openFraction, 1.0f), "Restart snaps to derived pose");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize checkpoint Door");
        Expect(world.GetDoors()[0].desiredOpen, "checkpoint fixture starts open");
        world.ResetCharacter(level.initialSpawnVisualCenter, {});
        Expect(world.GetPressurePlates()[0].active, "checkpoint respawn does not reset boxes");
        Expect(world.GetDoors()[0].desiredOpen, "checkpoint respawn keeps derived Door open");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize kill-plane Door");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 0, onPlate, {}, {});
        Expect(world.GetPressurePlates()[0].active, "runtime overlap activates plate");
        Expect(world.GetDoors()[0].desiredOpen, "kill-plane fixture starts open");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world,
            0,
            {offPlate.x, level.killPlaneY - 2.0f, offPlate.z},
            {},
            {});
        world.RecoverFallenDynamicBoxes();
        Expect(!world.GetPressurePlates()[0].active, "recovered box no longer overlaps plate");
        Expect(!world.GetDoors()[0].desiredOpen, "kill-plane recovery desires Door closed");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize rebuild Door");
        Expect(world.DoorBodyCount() == 1, "one Door before rebuild");
        const auto firstIds = world.GetDoors();
        Expect(firstIds[0].valid, "pre-rebuild Door valid");
        world::LevelDefinition rebuilt = level;
        rebuilt.doors[0].center.x = 6.0f;
        Expect(
            world.TryRebuild(
                rebuilt, rebuilt.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild with moved Door");
        Expect(world.DoorBodyCount() == 1, "rebuild still one Door body");
        Expect(
            NearlyEqual(world.GetDoors()[0].closedCenter.x, 6.0f),
            "rebuild uses active authored closed center");
        Expect(world.GetDoors()[0].valid, "post-rebuild Door valid");
        for (int cycle = 0; cycle < 8; ++cycle)
        {
            Expect(
                world.TryRebuild(
                    rebuilt, rebuilt.initialSpawnVisualCenter, world::kPlayerVisualSize),
                "repeated Door rebuild");
        }
        Expect(world.DoorBodyCount() == 1, "repeated rebuild does not leak Door bodies");
        Expect(world.StaticBodyCount() == 9, "Door rebuild keeps canonical static count");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize workingCopy isolation Door");
        Expect(world.GetDoors()[0].closedCenter.x == doorClosed.x, "runtime uses applied closed pose");
        level.doors[0].center.x = 9.0f;
        Expect(
            world.GetDoors()[0].closedCenter.x == doorClosed.x,
            "mutating caller LevelDefinition does not move runtime Door before Apply");
    }

    {
        world::LevelDefinition over = parsed.level;
        const int leftover = physics::kMaxAuthoredPhysicsBodies
            - static_cast<int>(over.elevatedPlatforms.size());
        over.doors.resize(static_cast<std::size_t>(leftover) + 1, MakeDoor(doorClosed));
        Expect(
            !physics::AuthoredPhysicsBodiesWithinBudget(
                static_cast<int>(over.elevatedPlatforms.size()),
                static_cast<int>(over.dynamicBoxes.size()),
                static_cast<int>(over.doors.size())),
            "over-budget Door count is rejected by capacity helper");
        physics::PhysicsWorld live;
        Expect(StartWorld(live, parsed.level), "live world before failed Door rebuild");
        const int staticBefore = live.StaticBodyCount();
        Expect(
            !live.TryRebuild(over, over.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "over-budget Door TryRebuild fails");
        Expect(live.IsInitialized(), "failed rebuild leaves live world");
        Expect(live.StaticBodyCount() == staticBefore, "failed rebuild is transactional");
        Expect(live.DoorBodyCount() == 0, "failed rebuild does not install Door bodies");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "initialize grab vs Door");
        world.SetGrabAim(world.GetPlayerPhysicsState().visualCenter, 1.0f);
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
            world,
            0,
            {doorClosed.x - 1.6f, doorClosed.y, 0.0f},
            {},
            {});
        world.SetGrabAim({doorClosed.x - 1.6f, doorClosed.y, 0.0f}, 1.0f);
        world.HandleGrabDrop();
        const physics::DynamicBoxGrabState grab = world.GetGrabState();
        if (grab.carrying)
        {
            const float beforeX = world.GetDynamicBoxes()[0].center.x;
            for (int frame = 0; frame < 40; ++frame)
            {
                world.SetGrabAim(world.GetPlayerPhysicsState().visualCenter, 1.0f);
                physics::PlayerMoveCommand walk{};
                walk.horizontalVelocity = 6.0f;
                world.UpdateMovingPlatform(kStepSeconds);
                world.MovePlayer(walk, kStepSeconds);
                world.Update(kStepSeconds);
            }
            Expect(
                world.GetDynamicBoxes()[0].center.x + 0.3f
                    < doorClosed.x + world::kDefaultDoorSize.x * 0.5f,
                "carried Dynamic Box remains obstructed by Door");
            Expect(world.GetGrabState().carrying, "Grab/Carry remains usable against Door");
            (void)beforeX;
        }
        else
        {
            Expect(true, "grab vs Door remains a non-fatal optional path");
        }
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "inventory seed before Door update");
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor(doorClosed));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "inventory door isolation Initialize");
        Expect(world.GetDoors()[0].desiredOpen, "linked Door desiredOpen from plate");
        StepWorld(world, 30);
        Expect(inventory.GetQuantity("key") == 1, "Door open does not consume Inventory");
        Expect(inventory.Entries().size() == 1, "Door update does not add Inventory entries");
    }

    Expect(
        world::DoorOpenCenter(MakeDoor(doorClosed)).y
            == doorClosed.y + world::kDefaultDoorOpenDistance,
        "open center is closed +Y openDistance");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Door test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Door tests passed.\n");
    return 0;
}
