// Focused Milestone 55 Item Pickup harness. Not shipped.
// Targeting, TryAdd collection, M51 E priority, LOS, and run isolation.

#include "gameplay/Inventory.h"
#include "gameplay/ItemPickupRuntime.h"
#include "physics/DynamicBoxGrab.h"
#include "physics/PhysicsCapacity.h"
#include "physics/PhysicsWorld.h"
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
#include <limits>
#include <span>
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

void PressInteract(
    physics::PhysicsWorld& world,
    gameplay::Inventory& inventory,
    gameplay::ItemPickupRunState& runState,
    std::span<const world::ItemPickupSpec> pickups,
    int pickupTarget)
{
    const bool wasCarrying = world.GetGrabState().carrying;
    world.HandleGrabDrop();
    if (!wasCarrying && !world.GetGrabState().carrying)
    {
        (void)gameplay::TryCollectItemPickup(inventory, runState, pickups, pickupTarget);
    }
}

std::vector<std::uint8_t> EmptyLos(std::size_t count)
{
    return std::vector<std::uint8_t>(count, 0);
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
    Expect(CountRecords(canonical, "item_pickup") == 0, "canonical serialize has 0 item_pickup");
    Expect(canonical.find("collected") == std::string::npos, "serialize has no collected field");

    const core::Vec3 spawn = parsed.level.initialSpawnVisualCenter;
    const core::Vec3 nearby{spawn.x + 1.55f, spawn.y, spawn.z};
    const core::Vec3 farAway{spawn.x + 4.5f, spawn.y, spawn.z};
    const core::Vec3 behind{spawn.x - 1.55f, spawn.y, spawn.z};

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        Expect(world::ItemPickupSpecIsValid(pickup), "default pickup is valid");
        Expect(pickup.itemId == "key", "default itemId is key");
        Expect(pickup.quantity == 1, "default quantity is 1");
        Expect(pickup.modelIdentity.empty(), "default has no model");
        Expect(pickup.visualOffset.x == 0.0f && pickup.visualOffset.y == 0.0f
                && pickup.visualOffset.z == 0.0f,
            "Add default visualOffset is 0");
        Expect(pickup.visualRotationDegrees.x == 0.0f && pickup.visualRotationDegrees.y == 0.0f
                && pickup.visualRotationDegrees.z == 0.0f,
            "Add default visualRotation is 0");
        Expect(pickup.visualScale.x == 1.0f && pickup.visualScale.y == 1.0f
                && pickup.visualScale.z == 1.0f,
            "Add default visualScale is 1");
        Expect(
            world::ItemPickupVisualPosition(pickup).x == pickup.position.x
                && world::ItemPickupVisualPosition(pickup).y == pickup.position.y
                && world::ItemPickupVisualPosition(pickup).z == pickup.position.z,
            "neutral visual position equals gameplay position");
    }

    {
        const std::vector<world::ItemPickupSpec> pickups{MakePickup(nearby)};
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(gameplay::ItemPickupIsAvailable(run, 0), "15. pickup starts available");
        const std::vector<std::uint8_t> los = EmptyLos(1);
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, pickups, run.collected, los) == 0,
            "16. nearby in-front pickup is target");
        const std::vector<world::ItemPickupSpec> farPickups{MakePickup(farAway)};
        const std::vector<world::ItemPickupSpec> behindPickups{MakePickup(behind)};
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, farPickups, run.collected, los)
                == gameplay::kNoItemPickupIndex,
            "17. out-of-range pickup is not target");
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, behindPickups, run.collected, los)
                == gameplay::kNoItemPickupIndex,
            "facing rule rejects behind");
    }

    {
        world::ItemPickupSpec shifted = MakePickup(nearby, "key");
        shifted.visualOffset = {4.0f, 8.0f, 3.0f};
        shifted.visualRotationDegrees = {90.0f, 45.0f, 15.0f};
        shifted.visualScale = {4.0f, 4.0f, 4.0f};
        const std::vector<world::ItemPickupSpec> pickups{shifted};
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        const std::vector<std::uint8_t> los = EmptyLos(1);
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, pickups, run.collected, los) == 0,
            "targeting uses gameplay position, not visualOffset");
        const world::ItemPickupSpec visualFar = []() {
            world::ItemPickupSpec spec = MakePickup({100.0f, 1.0f, 0.0f}, "key");
            spec.visualOffset = {-98.45f, 0.0f, 0.0f};
            return spec;
        }();
        const std::vector<world::ItemPickupSpec> visualNearMesh{visualFar};
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, visualNearMesh, run.collected, los)
                == gameplay::kNoItemPickupIndex,
            "mesh-near visualOffset does not create a gameplay target");
        gameplay::Inventory inventory;
        Expect(gameplay::TryCollectItemPickup(inventory, run, pickups, 0), "collect transformed pickup");
        Expect(inventory.GetQuantity("key") == 1, "collection still grants itemId");
        Expect(pickups[0].visualOffset.y == 8.0f, "collection does not mutate authored visualOffset");
        Expect(pickups[0].visualScale.x == 4.0f, "collection does not mutate authored visualScale");
        Expect(pickups[0].position.x == nearby.x, "collection does not mutate gameplay position");
    }

    {
        const std::vector<world::ItemPickupSpec> pickups{
            MakePickup({spawn.x + 1.8f, spawn.y, spawn.z}, "coin"),
            MakePickup({spawn.x + 1.2f, spawn.y, spawn.z}, "key")};
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(2);
        const std::vector<std::uint8_t> los = EmptyLos(2);
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, pickups, run.collected, los) == 1,
            "19. nearest pickup wins");
    }

    {
        const std::vector<world::ItemPickupSpec> pickups{
            MakePickup({spawn.x + 1.5f, spawn.y, spawn.z}, "key"),
            MakePickup({spawn.x + 1.5f, spawn.y, spawn.z}, "coin")};
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(2);
        const std::vector<std::uint8_t> los = EmptyLos(2);
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, pickups, run.collected, los) == 0,
            "20. equal distance keeps lower session index");
    }

    {
        const std::vector<world::ItemPickupSpec> pickups{MakePickup(nearby, "key", 2)};
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::Inventory inventory;
        Expect(
            gameplay::TryCollectItemPickup(inventory, run, pickups, 0),
            "21. collection uses production TryAdd");
        Expect(inventory.GetQuantity("key") == 2, "TryAdd stored quantity");
        Expect(run.collected[0] == 1, "22. only that pickup is collected");
        Expect(!gameplay::ItemPickupIsAvailable(run, 0), "23. collected is untargetable");
        const std::vector<std::uint8_t> los = EmptyLos(1);
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, pickups, run.collected, los)
                == gameplay::kNoItemPickupIndex,
            "collected pickup is not targeted");
        Expect(
            !gameplay::TryCollectItemPickup(inventory, run, pickups, 0),
            "27. second collect of same pickup fails");
        Expect(inventory.GetQuantity("key") == 2, "no double collection");
    }

    {
        const std::vector<world::ItemPickupSpec> pickups{
            MakePickup(nearby, "key", gameplay::kMaxItemQuantity)};
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "seed inventory near cap");
        Expect(
            !gameplay::TryCollectItemPickup(inventory, run, pickups, 0),
            "25. failed TryAdd leaves pickup");
        Expect(gameplay::ItemPickupIsAvailable(run, 0), "26. overflow does not consume pickup");
        Expect(inventory.GetQuantity("key") == 1, "overflow does not partial-add");
    }

    {
        const std::vector<world::ItemPickupSpec> pickups{
            MakePickup(nearby, "key"), MakePickup({spawn.x + 1.8f, spawn.y, spawn.z}, "coin")};
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(2);
        gameplay::Inventory inventory;
        Expect(gameplay::TryCollectItemPickup(inventory, run, pickups, 0), "collect first only");
        Expect(run.collected[0] == 1 && run.collected[1] == 0, "sibling pickup remains available");
        Expect(inventory.Has("coin", 1) == false, "untargeted pickup is not added");
    }

    {
        world::LevelDefinition authored = parsed.level;
        authored.itemPickups.push_back(MakePickup(nearby));
        const world::LevelDefinition before = authored;
        gameplay::ItemPickupRunState run =
            gameplay::MakeClearedItemPickupRunState(authored.itemPickups.size());
        gameplay::Inventory inventory;
        Expect(
            gameplay::TryCollectItemPickup(inventory, run, authored.itemPickups, 0),
            "collect against authored snapshot");
        Expect(
            world::AuthoredLevelDataEqual(before, authored),
            "31. collection does not mutate authored data");
        Expect(authored.itemPickups.size() == 1, "authored pickup remains");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.itemPickups.push_back(MakePickup(nearby));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "pickup-only Initialize");
        Expect(world.DynamicBodyCount() == 0, "38. Item Pickup adds no dynamic body");
        Expect(world.DoorBodyCount() == 0, "Item Pickup adds no Door body");
        Expect(
            world.StaticBodyCount() == 9,
            "canonical static count unchanged with Item Pickup");
        Expect(
            physics::AuthoredPhysicsBodiesWithinBudget(
                static_cast<int>(level.elevatedPlatforms.size()),
                static_cast<int>(level.dynamicBoxes.size()),
                static_cast<int>(level.doors.size())),
            "39. M53 leftover still valid");
    }

    {
        world::LevelDefinition level = parsed.level;
        world::PressurePlateSpec plate{};
        plate.center = nearby;
        plate.size = world::kDefaultPressurePlateSize;
        level.pressurePlates.push_back(plate);
        level.itemPickups.push_back(MakePickup(nearby));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "plate+pickup Initialize");
        Expect(world.GetPressurePlates().size() == 1, "one plate");
        Expect(!world.GetPressurePlates()[0].active, "37. Item Pickup does not activate plate");
    }

    {
        world::LevelDefinition level = parsed.level;
        const core::Vec3 pickupPos{spawn.x + 3.0f, spawn.y, spawn.z};
        world::DoorSpec door{};
        door.center = {spawn.x + 1.5f, 1.5f, spawn.z};
        door.size = world::kDefaultDoorSize;
        door.openDistance = world::kDefaultDoorOpenDistance;
        level.doors.push_back(door);
        level.itemPickups.push_back(MakePickup(pickupPos));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "LOS door Initialize");
        Expect(
            world.WorldSolidBlocksSegment(spawn, pickupPos),
            "18. closed Door blocks pickup LOS");
        std::vector<std::uint8_t> los(1, 1);
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, level.itemPickups, run.collected, los)
                == gameplay::kNoItemPickupIndex,
            "LOS-blocked pickup is not target");
    }

    {
        world::LevelDefinition level = parsed.level;
        const core::Vec3 boxCenter{spawn.x + 1.55f, spawn.y, spawn.z};
        const core::Vec3 pickupPos{spawn.x + 1.8f, spawn.y, spawn.z};
        level.dynamicBoxes.push_back(MakeBox(boxCenter));
        level.itemPickups.push_back(MakePickup(pickupPos));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "grab-priority Initialize");
        world.SetGrabAim(spawn, 1.0f);
        world.Update(kStepSeconds);
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        const int pickupTarget = 0;
        PressInteract(world, inventory, run, level.itemPickups, pickupTarget);
        Expect(world.GetGrabState().carrying, "29. Dynamic Box grab wins E");
        Expect(gameplay::ItemPickupIsAvailable(run, 0), "grab press does not collect");
        PressInteract(world, inventory, run, level.itemPickups, pickupTarget);
        Expect(!world.GetGrabState().carrying, "28. carrying E drops");
        Expect(gameplay::ItemPickupIsAvailable(run, 0), "drop press does not collect");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.itemPickups.push_back(MakePickup({spawn.x + 1.55f, spawn.y, spawn.z}));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "pickup-only interact Initialize");
        world.SetGrabAim(spawn, 1.0f);
        world.Update(kStepSeconds);
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        PressInteract(world, inventory, run, level.itemPickups, 0);
        Expect(!world.GetGrabState().carrying, "pickup-only E does not grab");
        Expect(inventory.GetQuantity("key") == 1, "30. pickup E collects after no box target");
        Expect(!gameplay::ItemPickupIsAvailable(run, 0), "collected after dedicated pickup press");
    }

    {
        world::LevelDefinition level = parsed.level;
        level.itemPickups.push_back(MakePickup(nearby));
        physics::PhysicsWorld world;
        Expect(StartWorld(world, level), "rebuild isolation Initialize");
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(gameplay::TryCollectItemPickup(inventory, run, level.itemPickups, 0), "collect before rebuild");
        Expect(
            world.TryRebuild(
                level, level.initialSpawnVisualCenter, world::kPlayerVisualSize),
            "TryRebuild with collected pickup");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild);
        Expect(inventory.GetQuantity("key") == 1, "36. rebuild preserves Inventory");
        Expect(run.collected[0] == 1, "rebuild does not respawn collected pickup");
        Expect(level.itemPickups.size() == 1, "rebuild does not delete authored pickup");
    }

    {
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        const std::vector<world::ItemPickupSpec> pickups{MakePickup(nearby)};
        Expect(gameplay::TryCollectItemPickup(inventory, run, pickups, 0), "collect before respawn policy");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::CheckpointRespawn);
        Expect(inventory.GetQuantity("key") == 1, "33. checkpoint respawn preserves Inventory");
        Expect(run.collected[0] == 1, "checkpoint respawn preserves collected");
        gameplay::ApplyInventoryLifecycle(inventory, gameplay::InventoryLifecycleEvent::RestartRun);
        run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(inventory.Entries().empty(), "34. Restart clears Inventory");
        Expect(gameplay::ItemPickupIsAvailable(run, 0), "Restart restores pickup available");
        Expect(gameplay::TryCollectItemPickup(inventory, run, pickups, 0), "collect before apply reset");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::ApplyCommittedLevel);
        run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(inventory.Entries().empty(), "35. Apply/reload clears Inventory");
        Expect(gameplay::ItemPickupIsAvailable(run, 0), "Apply/reload restores pickups");
    }

    {
        world::LevelDefinition level = parsed.level;
        world::ItemPickupSpec pickup = MakePickup(nearby, "key");
        pickup.modelIdentity = "models/test_static.glb";
        level.itemPickups.push_back(pickup);
        Expect(
            world::LevelReferencesStaticPropIdentity(level, "models/test_static.glb"),
            "40. authored pickup model blocks Delete Asset");
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::Inventory inventory;
        Expect(gameplay::TryCollectItemPickup(inventory, run, level.itemPickups, 0), "collect modeled pickup");
        Expect(
            world::LevelReferencesStaticPropIdentity(level, "models/test_static.glb"),
            "41. collected runtime does not weaken authored asset protection");
    }

    {
        world::ItemPickupSpec badPos = MakePickup(nearby);
        badPos.position.x = std::numeric_limits<float>::quiet_NaN();
        Expect(!world::ItemPickupSpecIsValid(badPos), "non-finite position rejected");
        world::ItemPickupSpec badId = MakePickup(nearby, "Key");
        Expect(!world::ItemPickupSpecIsValid(badId), "invalid M54 itemId rejected");
        world::ItemPickupSpec badQty = MakePickup(nearby, "key", 0);
        Expect(!world::ItemPickupSpecIsValid(badQty), "invalid quantity rejected");
        world::ItemPickupSpec badModel = MakePickup(nearby);
        badModel.modelIdentity = "not-a-model";
        Expect(!world::ItemPickupSpecIsValid(badModel), "invalid model identity rejected");
        world::ItemPickupSpec badOffset = MakePickup(nearby);
        badOffset.visualOffset.y = std::numeric_limits<float>::infinity();
        Expect(!world::ItemPickupSpecIsValid(badOffset), "non-finite visualOffset rejected");
        world::ItemPickupSpec badRotation = MakePickup(nearby);
        badRotation.visualRotationDegrees.x = std::numeric_limits<float>::quiet_NaN();
        Expect(!world::ItemPickupSpecIsValid(badRotation), "non-finite visualRotation rejected");
        world::ItemPickupSpec zeroScale = MakePickup(nearby);
        zeroScale.visualScale.x = 0.0f;
        Expect(!world::ItemPickupSpecIsValid(zeroScale), "zero visualScale rejected");
        world::ItemPickupSpec negativeScale = MakePickup(nearby);
        negativeScale.visualScale.y = -1.0f;
        Expect(!world::ItemPickupSpecIsValid(negativeScale), "negative visualScale rejected");
        world::ItemPickupSpec okVisual = MakePickup(nearby);
        okVisual.visualOffset = {0.0f, 0.5f, 0.0f};
        okVisual.visualRotationDegrees = {0.0f, 45.0f, 0.0f};
        okVisual.visualScale = {0.15f, 0.15f, 0.15f};
        Expect(world::ItemPickupSpecIsValid(okVisual), "finite positive visual transform is valid");
        Expect(
            world::ItemPickupVisualPosition(okVisual).y == nearby.y + 0.5f
                && world::ItemPickupVisualPosition(okVisual).x == nearby.x,
            "visual position is gameplay position plus offset");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d item pickup test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Item pickup tests passed.\n");
    return 0;
}
