// Focused Milestone 57 / 57.1 Door Inventory unlock harness. Not shipped.
// Authored requiredItemId, runtime lock, E arbitration, and M53 gating.

#include "gameplay/DoorLockRuntime.h"
#include "gameplay/Inventory.h"
#include "gameplay/InventoryUi.h"
#include "gameplay/ItemPickupRuntime.h"
#include "physics/DynamicBoxGrab.h"
#include "physics/PhysicsCapacity.h"
#include "physics/PhysicsWorld.h"
#include "physics/PhysicsWorldTestAccess.h"
#include "world/Door.h"
#include "world/DynamicBox.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelWriter.h"
#include "world/PressurePlate.h"
#include "world/StaticProp.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace
{
int gFailures = 0;

std::string LockedDoorPrompt(std::string_view itemId, bool hasItem)
{
    char buffer[64]{};
    gameplay::FormatLockedDoorPrompt(buffer, sizeof(buffer), itemId, hasItem);
    return buffer;
}

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

world::DoorSpec MakeDoor(
    core::Vec3 center,
    std::string_view requiredItemId = {},
    float openDistance = world::kDefaultDoorOpenDistance)
{
    world::DoorSpec spec{};
    spec.center = center;
    spec.size = world::kDefaultDoorSize;
    spec.openDistance = openDistance;
    spec.requiredItemId = std::string(requiredItemId);
    return spec;
}

world::ItemPickupSpec MakePickup(
    core::Vec3 position,
    std::string_view itemId = world::kDefaultItemPickupId,
    int quantity = world::kDefaultItemPickupQuantity)
{
    world::ItemPickupSpec spec{};
    spec.position = position;
    spec.itemId = std::string(itemId);
    spec.quantity = quantity;
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

void BindLocks(physics::PhysicsWorld& world, const gameplay::DoorLockRunState& locks)
{
    world.SetDoorRuntimeUnlocked(locks.unlocked);
}

void StepWorld(
    physics::PhysicsWorld& world,
    const gameplay::DoorLockRunState& locks,
    int frames)
{
    physics::PlayerMoveCommand idle{};
    for (int frame = 0; frame < frames; ++frame)
    {
        BindLocks(world, locks);
        world.UpdateMovingPlatform(kStepSeconds);
        world.MovePlayer(idle, kStepSeconds);
        world.Update(kStepSeconds);
    }
}

void PressInteract(
    physics::PhysicsWorld& world,
    gameplay::Inventory& inventory,
    gameplay::ItemPickupRunState& pickups,
    std::span<const world::ItemPickupSpec> pickupSpecs,
    int pickupTarget,
    gameplay::DoorLockRunState& locks,
    std::span<const world::DoorSpec> doors,
    int doorTarget)
{
    const bool wasCarrying = world.GetGrabState().carrying;
    world.HandleGrabDrop();
    if (!wasCarrying && !world.GetGrabState().carrying)
    {
        if (pickupTarget != gameplay::kNoItemPickupIndex)
        {
            (void)gameplay::TryCollectItemPickup(inventory, pickups, pickupSpecs, pickupTarget);
        }
        else
        {
            (void)gameplay::TryUnlockLockedDoor(inventory, locks, doors, doorTarget);
        }
    }
    BindLocks(world, locks);
}

std::vector<std::uint8_t> EmptyLos(std::size_t count)
{
    return std::vector<std::uint8_t>(count, 0);
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
    Expect(parsed.level.itemPickups.empty(), "canonical Level 01 has 0 Item Pickups");
    Expect(parsed.level.doors.empty(), "canonical Level 01 has 0 Doors");
    Expect(parsed.level.pressurePlates.empty(), "canonical Level 01 has 0 Pressure Plates");
    Expect(parsed.level.dynamicBoxes.empty(), "canonical Level 01 has 0 Dynamic Boxes");
    Expect(parsed.level.staticProps.empty(), "canonical Level 01 has 0 Static Props");
    const std::string canonical = world::SerializeLevelText(parsed.level);
    Expect(CountRecords(canonical, "door") == 0, "canonical serialize has 0 door records");
    Expect(canonical.find("dynamic_box 0 5 0 1 1 1 30") == std::string::npos,
        "legacy probe dynamic_box remains absent");
    Expect(canonical.find("requiredItemId") == std::string::npos, "no requiredItemId field");
    Expect(canonical.find("lockId") == std::string::npos, "no lockId field");

    const core::Vec3 spawn = parsed.level.initialSpawnVisualCenter;
    const float groundTop = parsed.level.ground.center.y + parsed.level.ground.size.y * 0.5f;
    const core::Vec3 plateCenter{
        10.0f, groundTop + world::kDefaultPressurePlateSize.y * 0.5f, 0.0f};
    const core::Vec3 onPlate{plateCenter.x, groundTop + 0.5f, 0.0f};
    const core::Vec3 offPlate{28.0f, groundTop + 0.5f, 0.0f};
    const core::Vec3 nearbyDoor{spawn.x + 1.55f, spawn.y, spawn.z};
    const core::Vec3 farDoor{spawn.x + 4.5f, spawn.y, spawn.z};
    const core::Vec3 behindDoor{spawn.x - 1.55f, spawn.y, spawn.z};

    {
        const std::string oldDoor = canonical + "door 4 1.5 0 1.2 3 2.4 3.2\n";
        const world::ParseLevelFileResult one = world::ParseLevelText(oldDoor);
        Expect(one.status == world::LoadLevelFileStatus::Loaded, "1. old Door syntax remains valid");
        Expect(
            one.level.doors[0].requiredItemId.empty(),
            "2. old Door syntax defaults no required item");
        const std::string written = world::SerializeLevelText(one.level);
        Expect(written.find("door 4 1.5 0") != std::string::npos, "writer keeps Door pose");
        Expect(
            world::ParseLevelText(written).level.doors[0].requiredItemId.empty(),
            "4. canonical writer round-trip defaults empty");
        Expect(written.find(" 3.2 0\n") != std::string::npos
                || written.find(" 3.2 0\r") != std::string::npos
                || written.find("3.2 0\n") != std::string::npos,
            "4. writer emits trailing 0 for no requirement");
    }

    {
        const std::string keyed = canonical + "door 4 1.5 0 1.2 3 2.4 3.2 1\n";
        const world::ParseLevelFileResult one = world::ParseLevelText(keyed);
        Expect(one.status == world::LoadLevelFileStatus::Loaded, "3. M57 trailing 1 parses");
        Expect(one.level.doors[0].requiredItemId == "key", "3. trailing 1 maps to key");
        const std::string written = world::SerializeLevelText(one.level);
        Expect(
            world::AuthoredLevelDataEqual(one.level, world::ParseLevelText(written).level),
            "4. keyed Door round-trip");
        Expect(written.find(" 3.2 key\n") != std::string::npos
                || written.find("3.2 key\n") != std::string::npos,
            "canonical writer emits key, not 1");
    }

    {
        const std::string zero = canonical + "door 4 1.5 0 1.2 3 2.4 3.2 0\n";
        Expect(
            world::ParseLevelText(zero).level.doors[0].requiredItemId.empty(),
            "M57 trailing 0 is no requirement");
        const std::string card = canonical + "door 4 1.5 0 1.2 3 2.4 3.2 card\n";
        Expect(world::ParseLevelText(card).level.doors[0].requiredItemId == "card", "card parses");
        const std::string red = canonical + "door 4 1.5 0 1.2 3 2.4 3.2 red_key\n";
        Expect(world::ParseLevelText(red).level.doors[0].requiredItemId == "red_key", "red_key parses");
        Expect(
            world::ParseLevelText(canonical + "door 4 1.5 0 1.2 3 2.4 3.2 redKey\n").status
                == world::LoadLevelFileStatus::Invalid,
            "redKey rejected by IsValidItemId");
        Expect(!gameplay::IsValidItemId("redKey"), "IsValidItemId rejects redKey");
        Expect(gameplay::IsValidItemId("red_key"), "IsValidItemId accepts red_key");
        const std::string writtenCard = world::SerializeLevelText(world::ParseLevelText(card).level);
        Expect(
            writtenCard.find(" 3.2 card\n") != std::string::npos
                || writtenCard.find("3.2 card\n") != std::string::npos,
            "canonical writer emits card");
    }

    Expect(
        world::ParseLevelText(canonical + "door 4 1.5 0 1.2 3 2.4 3.2 2\n").status
            == world::LoadLevelFileStatus::Invalid,
        "5. invalid required item token 2 rejected");
    Expect(
        world::ParseLevelText(canonical + "door 4 1.5 0 1.2 3 2.4 3.2 True\n").status
            == world::LoadLevelFileStatus::Invalid,
        "5. invalid required item token True rejected");
    Expect(
        world::ParseLevelText(canonical + "door 4 1.5 0 1.2 3 2.4 3.2 -1\n").status
            == world::LoadLevelFileStatus::Invalid,
        "5. invalid required item token -1 rejected");
    Expect(
        world::ParseLevelText(canonical + "door 4 1.5 0 1.2 3 2.4 3.2 1 extra\n").status
            == world::LoadLevelFileStatus::Invalid,
        "extra Door token rejected");

    {
        world::LevelDefinition a = parsed.level;
        world::LevelDefinition b = parsed.level;
        a.doors.push_back(MakeDoor({4.0f, 1.5f, 0.0f}));
        b.doors.push_back(MakeDoor({4.0f, 1.5f, 0.0f}, "key"));
        Expect(!world::AuthoredLevelDataEqual(a, b), "6. authored equality includes requiredItemId");
        b.doors[0].requiredItemId.clear();
        Expect(world::AuthoredLevelDataEqual(a, b), "equality matches when requiredItemId matches");
    }

    {
        world::DoorSpec added{};
        added.center = {4.0f, 1.5f, 0.0f};
        added.size = world::kDefaultDoorSize;
        added.openDistance = world::kDefaultDoorOpenDistance;
        Expect(added.requiredItemId.empty(), "7. Add Door defaults no required item");
        world::LevelDefinition working = parsed.level;
        working.doors.push_back(added);
        working.doors[0].requiredItemId = "card";
        world::DoorSpec duplicated = working.doors[0];
        duplicated.center.x += 1.0f;
        working.doors.push_back(duplicated);
        Expect(working.doors[1].requiredItemId == "card", "8. Duplicate Door preserves required item");
        working.doors[0].requiredItemId = "key";
        Expect(
            working.doors[0].requiredItemId != working.doors[1].requiredItemId,
            "9. Inspector-style Required Item edit mutates only workingCopy");
        const world::LevelDefinition active = parsed.level;
        Expect(!world::AuthoredLevelDataEqual(working, active), "10. Modified includes requiredItemId");
        working.doors[0].requiredItemId = "card";
        const std::string saved = world::SerializeLevelText(working);
        Expect(
            world::ParseLevelText(saved).level.doors[0].requiredItemId == "card",
            "10. Save preserves authored value");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor({8.5f, 1.5f, 0.0f}));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "non-key Door Initialize");
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(level.doors);
        BindLocks(world, locks);
        Expect(gameplay::DoorIsRuntimeUnlocked(locks, 0), "11. non-key Door starts unlocked");
        Expect(world.GetDoors()[0].desiredOpen, "11. non-key Door retains M53 plate open");
        Expect(world.DoorBodyCount() == 1, "41. one Door body, no extra lock body");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor({8.5f, 1.5f, 0.0f}, "key"));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "key Door Initialize");
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(level.doors);
        BindLocks(world, locks);
        Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 0), "12. required-item Door begins locked");
        Expect(world.GetPressurePlates()[0].active, "plate is Active while Door is locked");
        Expect(!world.GetDoors()[0].desiredOpen, "13. locked Door ignores active linked plate");
        Expect(NearlyEqual(world.GetDoors()[0].openFraction, 0.0f), "locked Door stays closed");
        Expect(level.doors[0].requiredItemId == "key", "32. authored requiredItemId unchanged after init");
        Expect(world.DoorBodyCount() == 1, "41. lock adds no Jolt body");
        Expect(
            physics::AuthoredPhysicsBodiesWithinBudget(
                static_cast<int>(level.elevatedPlatforms.size()),
                static_cast<int>(level.dynamicBoxes.size()),
                static_cast<int>(level.doors.size())),
            "42. M53 body budget unchanged");
    }

    {
        const std::vector<world::DoorSpec> doors{MakeDoor(nearbyDoor, "key")};
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
        const std::vector<std::uint8_t> los = EmptyLos(1);
        Expect(
            gameplay::FindLockedDoorTargetIndex(spawn, 1.0f, doors, locks.unlocked, los) == 0,
            "14. valid locked Door becomes target");
        locks.unlocked[0] = 1;
        Expect(
            gameplay::FindLockedDoorTargetIndex(spawn, 1.0f, doors, locks.unlocked, los)
                == gameplay::kNoLockedDoorIndex,
            "15. unlocked Door is not an unlock target");
        const std::vector<world::DoorSpec> nonKey{MakeDoor(nearbyDoor)};
        gameplay::DoorLockRunState unlocked = gameplay::MakeDoorLockRunState(nonKey);
        Expect(
            gameplay::FindLockedDoorTargetIndex(spawn, 1.0f, nonKey, unlocked.unlocked, los)
                == gameplay::kNoLockedDoorIndex,
            "16. non-key Door is not an unlock target");
        const std::vector<world::DoorSpec> far{MakeDoor(farDoor, "key")};
        gameplay::DoorLockRunState farLocks = gameplay::MakeDoorLockRunState(far);
        Expect(
            gameplay::FindLockedDoorTargetIndex(spawn, 1.0f, far, farLocks.unlocked, los)
                == gameplay::kNoLockedDoorIndex,
            "17. out-of-range Door is not target");
        const std::vector<world::DoorSpec> behind{MakeDoor(behindDoor, "key")};
        gameplay::DoorLockRunState behindLocks = gameplay::MakeDoorLockRunState(behind);
        Expect(
            gameplay::FindLockedDoorTargetIndex(spawn, 1.0f, behind, behindLocks.unlocked, los)
                == gameplay::kNoLockedDoorIndex,
            "17. facing rule rejects behind Door");
        std::vector<std::uint8_t> blocked = EmptyLos(1);
        blocked[0] = 1;
        Expect(
            gameplay::FindLockedDoorTargetIndex(spawn, 1.0f, doors, gameplay::MakeDoorLockRunState(doors).unlocked, blocked)
                == gameplay::kNoLockedDoorIndex,
            "17. LOS-blocked Door is not target");
    }

    {
        const std::vector<world::DoorSpec> doors{
            MakeDoor({spawn.x + 1.8f, spawn.y, spawn.z}, "key"),
            MakeDoor({spawn.x + 1.2f, spawn.y, spawn.z}, "key")};
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
        const std::vector<std::uint8_t> los = EmptyLos(2);
        Expect(
            gameplay::FindLockedDoorTargetIndex(spawn, 1.0f, doors, locks.unlocked, los) == 1,
            "18. nearest Door target wins");
    }

    {
        const std::vector<world::DoorSpec> doors{
            MakeDoor({spawn.x + 1.5f, spawn.y, spawn.z}, "key"),
            MakeDoor({spawn.x + 1.5f, spawn.y, spawn.z}, "key")};
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
        const std::vector<std::uint8_t> los = EmptyLos(2);
        Expect(
            gameplay::FindLockedDoorTargetIndex(spawn, 1.0f, doors, locks.unlocked, los) == 0,
            "19. equal distance keeps lower session index");
    }

    {
        world::LevelDefinition level = parsed.level;
        const core::Vec3 boxCenter{spawn.x + 1.55f, spawn.y, spawn.z};
        level.dynamicBoxes.push_back(MakeBox(boxCenter));
        level.doors.push_back(MakeDoor({spawn.x + 2.35f, spawn.y, spawn.z}, "key"));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "priority Initialize");
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(level.doors);
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "seed key for priority");
        gameplay::ItemPickupRunState pickups = gameplay::MakeClearedItemPickupRunState(0);
        world.SetGrabAim(spawn, 1.0f);
        world.Update(kStepSeconds);
        PressInteract(world, inventory, pickups, {}, gameplay::kNoItemPickupIndex, locks, level.doors, 0);
        Expect(world.GetGrabState().carrying, "21. Dynamic Box target + locked Door -> Grab only");
        Expect(inventory.GetQuantity("key") == 1, "Grab does not consume key");
        Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 0), "Grab does not unlock Door");
        PressInteract(world, inventory, pickups, {}, gameplay::kNoItemPickupIndex, locks, level.doors, 0);
        Expect(!world.GetGrabState().carrying, "20. carrying Dynamic Box + E -> Drop only");
        Expect(inventory.GetQuantity("key") == 1, "Drop does not consume key");
        Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 0), "Drop does not unlock Door");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.itemPickups.push_back(MakePickup({spawn.x + 1.55f, spawn.y, spawn.z}, "coin"));
        level.doors.push_back(MakeDoor({spawn.x + 2.35f, spawn.y, spawn.z}, "key"));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "pickup-priority Initialize");
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(level.doors);
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "seed key under pickup");
        gameplay::ItemPickupRunState pickups = gameplay::MakeClearedItemPickupRunState(1);
        world.SetGrabAim(spawn, 1.0f);
        world.Update(kStepSeconds);
        PressInteract(world, inventory, pickups, level.itemPickups, 0, locks, level.doors, 0);
        Expect(inventory.GetQuantity("coin") == 1, "22. Item Pickup target + locked Door -> Collect only");
        Expect(inventory.GetQuantity("key") == 1, "collect does not consume key");
        Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 0), "collect does not unlock Door");
        Expect(!gameplay::ItemPickupIsAvailable(pickups, 0), "pickup collected");
    }

    {
        const std::vector<world::DoorSpec> doors{MakeDoor(nearbyDoor, "key")};
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
        gameplay::Inventory inventory;
        Expect(
            !gameplay::TryUnlockLockedDoor(inventory, locks, doors, 0),
            "24. no key -> unlock fails unchanged");
        Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 0), "Door remains locked without key");
        Expect(inventory.Entries().empty(), "Inventory unchanged without key");
        Expect(LockedDoorPrompt("key", false) == "Requires key",
            "25. no-key prompt");
        Expect(inventory.TryAdd("key", 1), "seed one key");
        Expect(LockedDoorPrompt("key", true) == "E Unlock Door (key)",
            "25. has-key prompt");
        Expect(
            gameplay::TryUnlockLockedDoor(inventory, locks, doors, 0),
            "25. key x1 -> unlock succeeds");
        Expect(inventory.GetQuantity("key") == 0, "25. consumes exactly one");
        Expect(inventory.Entries().empty(), "27. consuming final key removes Inventory entry");
        Expect(gameplay::DoorIsRuntimeUnlocked(locks, 0), "Door unlocked");
        Expect(doors[0].requiredItemId == "key", "29. unlock does not mutate authored requiredItemId");
    }

    {
        const std::vector<world::DoorSpec> doors{
            MakeDoor(nearbyDoor, "key"), MakeDoor({spawn.x + 1.8f, spawn.y, spawn.z}, "key")};
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 2), "seed key x2");
        Expect(gameplay::TryUnlockLockedDoor(inventory, locks, doors, 0), "unlock Door A");
        Expect(inventory.GetQuantity("key") == 1, "26. key x2 unlock leaves key x1");
        Expect(gameplay::DoorIsRuntimeUnlocked(locks, 0), "28. only targeted Door A unlocks");
        Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 1), "28. Door B stays locked");
        Expect(gameplay::TryUnlockLockedDoor(inventory, locks, doors, 1), "unlock Door B");
        Expect(inventory.GetQuantity("key") == 0, "second unlock consumes remaining key");
        Expect(gameplay::DoorIsRuntimeUnlocked(locks, 1), "Door B unlocks independently");
        Expect(inventory.Entries().empty(), "final key entry removed");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor({8.5f, 1.5f, 0.0f}, "key"));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "unlock-open Initialize");
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(level.doors);
        BindLocks(world, locks);
        Expect(world.GetPressurePlates()[0].active, "plate Active before unlock");
        Expect(!world.GetDoors()[0].desiredOpen, "locked ignores plate");
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "seed unlock-open key");
        const world::LevelDefinition authoredBefore = level;
        Expect(gameplay::TryUnlockLockedDoor(inventory, locks, level.doors, 0), "unlock with plate");
        BindLocks(world, locks);
        Expect(world.GetDoors()[0].desiredOpen, "31. active plate opens newly unlocked Door");
        Expect(world::AuthoredLevelDataEqual(authoredBefore, level), "29. authored Door list unchanged");
        StepWorld(world, locks, 8);
        Expect(world.GetDoors()[0].openFraction > 0.0f, "31. M53 opening begins after unlock");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 0, offPlate, {}, {});
        BindLocks(world, locks);
        Expect(!world.GetDoors()[0].desiredOpen, "33. unlocked Door follows plate off");
        StepWorld(world, locks, 180);
        Expect(NearlyEqual(world.GetDoors()[0].openFraction, 0.0f, 0.08f), "33. unlocked Door closes");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 0, onPlate, {}, {});
        BindLocks(world, locks);
        Expect(world.GetDoors()[0].desiredOpen, "33. subsequent plate OR opens without another key");
        Expect(inventory.GetQuantity("key") == 0, "no additional key consumed");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor({8.5f, 1.5f, 0.0f}, "key"));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "unlock-closed Initialize");
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(level.doors);
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "seed unlock-closed key");
        Expect(gameplay::TryUnlockLockedDoor(inventory, locks, level.doors, 0), "unlock without plate");
        BindLocks(world, locks);
        Expect(!world.GetPressurePlates()[0].active, "plate Inactive");
        Expect(!world.GetDoors()[0].desiredOpen, "32. unlock alone does not open Door");
        Expect(NearlyEqual(world.GetDoors()[0].openFraction, 0.0f), "stays closed without plate");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor({8.5f, 1.5f, 0.0f}, "key"));
        level.pressurePlates.push_back(MakePlate(plateCenter, 0));
        level.dynamicBoxes.push_back(MakeBox(onPlate));
        level.dynamicBoxes.push_back(MakeBox(offPlate));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "obstruction after unlock");
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(level.doors);
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "seed obstruction key");
        Expect(gameplay::TryUnlockLockedDoor(inventory, locks, level.doors, 0), "unlock for obstruction");
        BindLocks(world, locks);
        Expect(world.GetDoors()[0].desiredOpen, "unlocked starts open-desired");
        StepWorld(world, locks, 180);
        Expect(NearlyEqual(world.GetDoors()[0].openFraction, 1.0f, 0.05f), "opened fully");
        physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(world, 0, offPlate, {}, {});
        BindLocks(world, locks);
        for (int frame = 0; frame < 50; ++frame)
        {
            physics::PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
                world, 1, {8.5f, 1.5f, 0.0f}, {}, {});
            StepWorld(world, locks, 1);
        }
        Expect(world.GetDoors()[0].openFraction > 0.55f, "34. obstructed close pauses");
        Expect(world.GetDoors()[0].blockedClosing, "34. M53 obstruction-safe closing remains");
    }

    {
        const std::vector<world::DoorSpec> doors{MakeDoor(nearbyDoor, "key")};
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 2), "checkpoint seed");
        Expect(gameplay::TryUnlockLockedDoor(inventory, locks, doors, 0), "unlock before checkpoint");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::CheckpointRespawn);
        Expect(inventory.GetQuantity("key") == 1, "35. checkpoint preserves Inventory");
        Expect(gameplay::DoorIsRuntimeUnlocked(locks, 0), "35. checkpoint preserves unlocked Door");
        gameplay::ApplyInventoryLifecycle(inventory, gameplay::InventoryLifecycleEvent::RestartRun);
        locks = gameplay::MakeDoorLockRunState(doors);
        Expect(inventory.Entries().empty(), "36. Restart clears Inventory");
        Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 0), "36. Restart relocks authored key Door");
        gameplay::Inventory applyInv;
        Expect(applyInv.TryAdd("key", 1), "apply seed");
        gameplay::DoorLockRunState applyLocks = gameplay::MakeDoorLockRunState(doors);
        Expect(gameplay::TryUnlockLockedDoor(applyInv, applyLocks, doors, 0), "unlock before apply");
        gameplay::ApplyInventoryLifecycle(
            applyInv, gameplay::InventoryLifecycleEvent::ApplyCommittedLevel);
        applyLocks = gameplay::MakeDoorLockRunState(doors);
        Expect(applyInv.Entries().empty(), "37. Apply/reload clears Inventory");
        Expect(!gameplay::DoorIsRuntimeUnlocked(applyLocks, 0), "37. Apply/reload rebuilds lock state");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor({8.5f, 1.5f, 0.0f}, "key"));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "rebuild lock Initialize");
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(level.doors);
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "rebuild seed");
        Expect(gameplay::TryUnlockLockedDoor(inventory, locks, level.doors, 0), "unlock before rebuild");
        Expect(
            world.TryRebuild(level, level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild alone");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild);
        BindLocks(world, locks);
        Expect(inventory.GetQuantity("key") == 0, "38. rebuild preserves Inventory");
        Expect(gameplay::DoorIsRuntimeUnlocked(locks, 0), "38. rebuild preserves unlocked state");
        Expect(world.GetDoors()[0].unlocked, "PhysicsWorld receives preserved unlock flags");
        Expect(world.DoorBodyCount() == 1, "rebuild still one Door body");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "M56 seed");
        gameplay::InventoryUiState ui{};
        gameplay::OpenInventoryUi(ui, inventory);
        Expect(ui.open, "M56 opens");
        Expect(inventory.GetQuantity("key") == 1, "39. UI reads production key");
        const std::vector<world::DoorSpec> doors{MakeDoor(nearbyDoor, "key")};
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
        Expect(
            gameplay::InventoryUiBlocksGameplay(true, true),
            "40. M56-open suppresses world Door unlock");
        Expect(gameplay::TryUnlockLockedDoor(inventory, locks, doors, 0), "unlock while UI would be closed");
        gameplay::RepairInventorySelection(ui, inventory);
        Expect(inventory.Entries().empty(), "39. M56 reflects consumed key by reading Inventory");
        Expect(ui.selectedItemId.empty(), "last key removal clears UI selection");
    }

    {
        world::LevelDefinition level = parsed.level;
        const core::Vec3 wall{spawn.x + 0.8f, 1.5f, spawn.z};
        const core::Vec3 hiddenDoor{spawn.x + 2.2f, 1.5f, spawn.z};
        world::Box extra = parsed.level.elevatedPlatforms.empty()
            ? parsed.level.ground
            : parsed.level.elevatedPlatforms[0];
        extra.center = wall;
        extra.size = {0.6f, 3.0f, 2.4f};
        level.elevatedPlatforms.push_back(extra);
        level.doors.push_back(MakeDoor(hiddenDoor, "key"));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "LOS world Initialize");
        Expect(
            world.WorldSolidBlocksSegmentIgnoringDoor(spawn, hiddenDoor, 0),
            "17. platform between player and Door blocks LOS");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "M54 seed");
        Expect(inventory.GetQuantity("key") == 1, "43. M54 GetQuantity");
        Expect(inventory.TryRemove("key", 1), "43. M54 TryRemove");
        Expect(inventory.Entries().empty(), "43. M54 empty after remove");
        Expect(!inventory.TryRemove("key", 1), "43. M54 failed remove unchanged");
    }

    Expect(
        std::string(world::kM57LegacyRequiredKeyItemId) == "key",
        "50. M57 trailing 1 still maps to the Inventory itemId key");

    {
        world::LevelDefinition working = parsed.level;
        working.itemPickups.push_back(MakePickup({2.0f, 0.5f, 0.0f}, "card"));
        working.itemPickups.push_back(MakePickup({3.0f, 0.5f, 0.0f}, "card"));
        working.itemPickups.push_back(MakePickup({4.0f, 0.5f, 0.0f}, "key"));
        working.itemPickups.push_back(MakePickup({5.0f, 0.5f, 0.0f}, "red_key"));
        const std::vector<std::string> ids =
            world::UniqueAuthoredPickupItemIds(working.itemPickups);
        Expect(ids.size() == 3, "selector lists unique itemIds");
        Expect(ids[0] == "card" && ids[1] == "key" && ids[2] == "red_key",
            "selector order is deterministic sorted itemId");
        working.doors.push_back(MakeDoor({4.0f, 1.5f, 0.0f}));
        working.doors[0].requiredItemId = ids[0];
        Expect(working.doors[0].requiredItemId == "card", "selector writes itemId, not pickup index");
        working.itemPickups.erase(working.itemPickups.begin());
        Expect(working.doors[0].requiredItemId == "card",
            "deleting first matching pickup does not rewrite Door requirement");
        working.itemPickups.erase(working.itemPickups.begin());
        Expect(working.doors[0].requiredItemId == "card",
            "required item survives after all matching pickups are deleted");
        Expect(world::DoorSpecIsValid(working.doors[0]), "orphan valid itemId remains valid");
    }

    {
        const std::vector<world::DoorSpec> doors{
            MakeDoor(nearbyDoor, "card"),
            MakeDoor({spawn.x + 1.8f, spawn.y, spawn.z}, "key"),
            MakeDoor({spawn.x + 2.1f, spawn.y, spawn.z}, "red_key")};
        {
            gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
            gameplay::Inventory inventory;
            Expect(inventory.TryAdd("key", 1), "seed key only");
            Expect(!gameplay::TryUnlockLockedDoor(inventory, locks, doors, 0), "key cannot unlock card Door");
            Expect(inventory.GetQuantity("key") == 1, "failed card unlock leaves key");
            Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 0), "card Door stays locked");
            Expect(!gameplay::TryUnlockLockedDoor(inventory, locks, doors, 2), "key cannot unlock red_key Door");
        }
        {
            gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
            gameplay::Inventory inventory;
            Expect(inventory.TryAdd("card", 1), "seed card only");
            Expect(!gameplay::TryUnlockLockedDoor(inventory, locks, doors, 1), "card cannot unlock key Door");
            Expect(inventory.GetQuantity("card") == 1, "failed key unlock leaves card");
            Expect(!gameplay::TryUnlockLockedDoor(inventory, locks, doors, 2), "card cannot unlock red_key Door");
            Expect(gameplay::TryUnlockLockedDoor(inventory, locks, doors, 0), "card unlocks card Door");
            Expect(inventory.GetQuantity("card") == 0, "correct item consumes exactly one");
            Expect(inventory.Entries().empty(), "final card removal follows M54");
            Expect(gameplay::DoorIsRuntimeUnlocked(locks, 0), "only targeted card Door unlocks");
            Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 1), "key Door still locked");
            Expect(!gameplay::DoorIsRuntimeUnlocked(locks, 2), "red_key Door still locked");
            Expect(LockedDoorPrompt("card", false) == "Requires card", "card missing prompt");
            Expect(LockedDoorPrompt("red_key", true) == "E Unlock Door (red_key)", "red_key owned prompt");
            gameplay::InventoryUiState ui{};
            gameplay::OpenInventoryUi(ui, inventory);
            Expect(inventory.GetQuantity("card") == 0, "M56 reflects consumed card automatically");
        }
    }

    {
        world::LevelDefinition level = parsed.level;
        level.doors.push_back(MakeDoor({8.5f, 1.5f, 0.0f}, "card"));
        world::PressurePlateSpec plate = MakePlate(parsed.level.initialSpawnVisualCenter, 0, {2.0f, 2.0f, 2.0f});
        plate.activateByDynamicBox = false;
        plate.activateByPlayer = true;
        plate.visibleInGameplay = false;
        level.pressurePlates.push_back(plate);
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "invisible player plate + card Door");
        gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(level.doors);
        BindLocks(world, locks);
        Expect(world.GetPressurePlates()[0].active, "player-only invisible plate is Active");
        Expect(!world.GetPressurePlates()[0].visibleInGameplay, "invisible plate runtime flag");
        Expect(!world.GetDoors()[0].desiredOpen, "locked Door ignores active player plate");
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("card", 1), "seed card after standing on plate");
        Expect(inventory.GetQuantity("card") == 1, "plate overlap does not auto-consume card");
        Expect(gameplay::TryUnlockLockedDoor(inventory, locks, level.doors, 0), "explicit E unlock");
        BindLocks(world, locks);
        Expect(inventory.GetQuantity("card") == 0, "E consumes card");
        Expect(world.GetDoors()[0].desiredOpen, "after unlock, already-active plate opens Door");
        Expect(world.DoorBodyCount() == 1, "no extra Jolt body for plate modes");
        Expect(
            physics::AuthoredPhysicsBodiesWithinBudget(
                static_cast<int>(level.elevatedPlatforms.size()),
                static_cast<int>(level.dynamicBoxes.size()),
                static_cast<int>(level.doors.size())),
            "M53 body budget unchanged");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d locked door test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Locked door tests passed.\n");
    return 0;
}
