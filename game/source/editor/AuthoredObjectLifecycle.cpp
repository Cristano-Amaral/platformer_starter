#include "editor/AuthoredObjectLifecycle.h"

#include "editor/AuthoringGroups.h"
#include "editor/EditorSelectionSet.h"
#include "physics/PhysicsCapacity.h"
#include "world/LevelFile.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace editor
{
core::Vec3 MakeAuthoredAddPlacement(core::Vec3 cameraAnchor, float gameplayLaneZ);

namespace
{
void OffsetX(core::Vec3& value, float delta)
{
    value.x += delta;
}

void RemapSupportIndexAfterPlatformDelete(int& supportIndex, std::size_t deletedIndex)
{
    const int deleted = static_cast<int>(deletedIndex);
    if (supportIndex > deleted)
    {
        --supportIndex;
    }
}

void RemapPressurePlateDoorLinksAfterDoorDelete(
    world::LevelDefinition& workingCopy,
    std::size_t deletedIndex)
{
    const int deleted = static_cast<int>(deletedIndex);
    for (world::PressurePlateSpec& plate : workingCopy.pressurePlates)
    {
        if (plate.linkedDoorIndex == deleted)
        {
            plate.linkedDoorIndex = world::kNoLinkedDoor;
        }
        else if (plate.linkedDoorIndex > deleted)
        {
            --plate.linkedDoorIndex;
        }
    }
}

LifecycleEditResult Fail(LifecycleEditStatus status, EditorSelection selection = {})
{
    LifecycleEditResult result{};
    result.succeeded = false;
    result.status = status;
    result.selection = selection;
    return result;
}

LifecycleEditResult Ok(EditorSelection selection)
{
    LifecycleEditResult result{};
    result.succeeded = true;
    result.status = LifecycleEditStatus::Success;
    result.selection = selection;
    return result;
}

core::Vec3 ApplyPlacementAnchor(
    core::Vec3 cameraAnchor,
    core::Vec3 offset,
    float gameplayLaneZ)
{
    const core::Vec3 base = MakeAuthoredAddPlacement(cameraAnchor, gameplayLaneZ);
    return {base.x + offset.x, base.y + offset.y, base.z + offset.z};
}

core::Vec3 ApplyWorldCenter(core::Vec3 worldCenter, core::Vec3 offset)
{
    if (!std::isfinite(worldCenter.x))
    {
        worldCenter.x = 0.0f;
    }
    if (!std::isfinite(worldCenter.y))
    {
        worldCenter.y = 0.0f;
    }
    if (!std::isfinite(worldCenter.z))
    {
        worldCenter.z = 0.0f;
    }
    return {worldCenter.x + offset.x, worldCenter.y + offset.y, worldCenter.z + offset.z};
}
}

core::Vec3 MakeAuthoredAddPlacement(core::Vec3 cameraAnchor, float gameplayLaneZ)
{
    if (!std::isfinite(cameraAnchor.x))
    {
        cameraAnchor.x = 0.0f;
    }
    if (!std::isfinite(cameraAnchor.y))
    {
        cameraAnchor.y = 0.0f;
    }
    if (!std::isfinite(gameplayLaneZ))
    {
        gameplayLaneZ = 0.0f;
    }
    return {cameraAnchor.x, cameraAnchor.y, gameplayLaneZ};
}

bool SupportsLifecycle(EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
    case EditorObjectKind::Checkpoint:
    case EditorObjectKind::Hazard:
    case EditorObjectKind::Collectible:
    case EditorObjectKind::Goal:
    case EditorObjectKind::DynamicBox:
    case EditorObjectKind::PressurePlate:
    case EditorObjectKind::Door:
    case EditorObjectKind::ItemPickup:
    case EditorObjectKind::StaticProp:
    case EditorObjectKind::PointLight:
    case EditorObjectKind::SpotLight:
        return true;
    default:
        return false;
    }
}

int CategoryMaxCount(EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        return world::kMaxElevatedPlatformCount;
    case EditorObjectKind::DynamicBox:
    case EditorObjectKind::Door:
        return physics::kMaxAuthoredPhysicsBodies;
    case EditorObjectKind::Checkpoint:
    case EditorObjectKind::Hazard:
    case EditorObjectKind::Collectible:
    case EditorObjectKind::Goal:
    case EditorObjectKind::PressurePlate:
    case EditorObjectKind::StaticProp:
    case EditorObjectKind::ItemPickup:
    case EditorObjectKind::PointLight:
    case EditorObjectKind::SpotLight:
        return static_cast<int>(world::kMaxLevelLines) - world::kLevelV1FixedRecordLineCount;
    default:
        return 0;
    }
}

std::size_t CategoryCount(const world::LevelDefinition& level, EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        return level.elevatedPlatforms.size();
    case EditorObjectKind::Checkpoint:
        return level.checkpoints.size();
    case EditorObjectKind::Hazard:
        return level.hazards.size();
    case EditorObjectKind::Collectible:
        return level.collectibles.size();
    case EditorObjectKind::Goal:
        return level.levelGoals.size();
    case EditorObjectKind::DynamicBox:
        return level.dynamicBoxes.size();
    case EditorObjectKind::PressurePlate:
        return level.pressurePlates.size();
    case EditorObjectKind::Door:
        return level.doors.size();
    case EditorObjectKind::ItemPickup:
        return level.itemPickups.size();
    case EditorObjectKind::StaticProp:
        return level.staticProps.size();
    case EditorObjectKind::PointLight:
        return level.pointLights.size();
    case EditorObjectKind::SpotLight:
        return level.spotLights.size();
    default:
        return 0;
    }
}

bool CategoryAtCountLimit(const world::LevelDefinition& level, EditorObjectKind kind)
{
    if (!SupportsLifecycle(kind))
    {
        return true;
    }
    if (world::CountLevelV1RecordLines(level) >= static_cast<int>(world::kMaxLevelLines))
    {
        return true;
    }
    if (kind == EditorObjectKind::ElevatedPlatform || kind == EditorObjectKind::DynamicBox
        || kind == EditorObjectKind::Door)
    {
        const int nextPlatforms = static_cast<int>(level.elevatedPlatforms.size())
            + (kind == EditorObjectKind::ElevatedPlatform ? 1 : 0);
        const int nextBoxes = static_cast<int>(level.dynamicBoxes.size())
            + (kind == EditorObjectKind::DynamicBox ? 1 : 0);
        const int nextDoors = static_cast<int>(level.doors.size())
            + (kind == EditorObjectKind::Door ? 1 : 0);
        if (!physics::AuthoredPhysicsBodiesWithinBudget(nextPlatforms, nextBoxes, nextDoors))
        {
            return true;
        }
    }
    return false;
}

void MarkCategoryStructuralPending(CategoryStructuralPending& pending, EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        pending.elevatedPlatforms = true;
        break;
    case EditorObjectKind::Checkpoint:
        pending.checkpoints = true;
        break;
    case EditorObjectKind::Hazard:
        pending.hazards = true;
        break;
    case EditorObjectKind::Collectible:
        pending.collectibles = true;
        break;
    case EditorObjectKind::Goal:
        pending.levelGoals = true;
        break;
    case EditorObjectKind::DynamicBox:
        pending.dynamicBoxes = true;
        break;
    case EditorObjectKind::PressurePlate:
        pending.pressurePlates = true;
        break;
    case EditorObjectKind::Door:
        pending.doors = true;
        break;
    case EditorObjectKind::ItemPickup:
        pending.itemPickups = true;
        break;
    case EditorObjectKind::StaticProp:
        pending.staticProps = true;
        break;
    case EditorObjectKind::PointLight:
        pending.pointLights = true;
        break;
    case EditorObjectKind::SpotLight:
        pending.spotLights = true;
        break;
    default:
        break;
    }
}

bool CategoryHasStructuralPending(
    EditorObjectKind kind,
    const CategoryStructuralPending& pending)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        return pending.elevatedPlatforms;
    case EditorObjectKind::Checkpoint:
        return pending.checkpoints;
    case EditorObjectKind::Hazard:
        return pending.hazards;
    case EditorObjectKind::Collectible:
        return pending.collectibles;
    case EditorObjectKind::Goal:
        return pending.levelGoals;
    case EditorObjectKind::DynamicBox:
        return pending.dynamicBoxes;
    case EditorObjectKind::PressurePlate:
        return pending.pressurePlates;
    case EditorObjectKind::Door:
        return pending.doors;
    case EditorObjectKind::ItemPickup:
        return pending.itemPickups;
    case EditorObjectKind::StaticProp:
        return pending.staticProps;
    case EditorObjectKind::PointLight:
        return pending.pointLights;
    case EditorObjectKind::SpotLight:
        return pending.spotLights;
    default:
        return false;
    }
}

namespace
{
CategoryIndexMap* MutableCategoryMap(StructuralIndexMap& map, EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        return &map.elevatedPlatforms;
    case EditorObjectKind::Checkpoint:
        return &map.checkpoints;
    case EditorObjectKind::Hazard:
        return &map.hazards;
    case EditorObjectKind::Collectible:
        return &map.collectibles;
    case EditorObjectKind::Goal:
        return &map.levelGoals;
    case EditorObjectKind::DynamicBox:
        return &map.dynamicBoxes;
    case EditorObjectKind::PressurePlate:
        return &map.pressurePlates;
    case EditorObjectKind::Door:
        return &map.doors;
    case EditorObjectKind::ItemPickup:
        return &map.itemPickups;
    case EditorObjectKind::StaticProp:
        return &map.staticProps;
    case EditorObjectKind::PointLight:
        return &map.pointLights;
    case EditorObjectKind::SpotLight:
        return &map.spotLights;
    default:
        return nullptr;
    }
}

const CategoryIndexMap* CategoryMap(const StructuralIndexMap& map, EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        return &map.elevatedPlatforms;
    case EditorObjectKind::Checkpoint:
        return &map.checkpoints;
    case EditorObjectKind::Hazard:
        return &map.hazards;
    case EditorObjectKind::Collectible:
        return &map.collectibles;
    case EditorObjectKind::Goal:
        return &map.levelGoals;
    case EditorObjectKind::DynamicBox:
        return &map.dynamicBoxes;
    case EditorObjectKind::PressurePlate:
        return &map.pressurePlates;
    case EditorObjectKind::Door:
        return &map.doors;
    case EditorObjectKind::ItemPickup:
        return &map.itemPickups;
    case EditorObjectKind::StaticProp:
        return &map.staticProps;
    case EditorObjectKind::PointLight:
        return &map.pointLights;
    case EditorObjectKind::SpotLight:
        return &map.spotLights;
    default:
        return nullptr;
    }
}

void FillIdentity(CategoryIndexMap& category, std::size_t activeCount)
{
    category.activeToWorking.resize(activeCount);
    for (std::size_t index = 0; index < activeCount; ++index)
    {
        category.activeToWorking[index] = static_cast<int>(index);
    }
}

bool CategoryMapMatchesActive(const CategoryIndexMap& category, std::size_t activeCount)
{
    return category.activeToWorking.size() == activeCount;
}
}

void ResetStructuralIndexMap(
    StructuralIndexMap& map,
    const world::LevelDefinition& active)
{
    FillIdentity(map.elevatedPlatforms, active.elevatedPlatforms.size());
    FillIdentity(map.checkpoints, active.checkpoints.size());
    FillIdentity(map.hazards, active.hazards.size());
    FillIdentity(map.collectibles, active.collectibles.size());
    FillIdentity(map.levelGoals, active.levelGoals.size());
    FillIdentity(map.dynamicBoxes, active.dynamicBoxes.size());
    FillIdentity(map.pressurePlates, active.pressurePlates.size());
    FillIdentity(map.doors, active.doors.size());
    FillIdentity(map.itemPickups, active.itemPickups.size());
    FillIdentity(map.staticProps, active.staticProps.size());
    FillIdentity(map.pointLights, active.pointLights.size());
    FillIdentity(map.spotLights, active.spotLights.size());
}

void EnsureStructuralIndexMap(
    StructuralIndexMap& map,
    const world::LevelDefinition& active)
{
    if (CategoryMapMatchesActive(map.elevatedPlatforms, active.elevatedPlatforms.size())
        && CategoryMapMatchesActive(map.checkpoints, active.checkpoints.size())
        && CategoryMapMatchesActive(map.hazards, active.hazards.size())
        && CategoryMapMatchesActive(map.collectibles, active.collectibles.size())
        && CategoryMapMatchesActive(map.levelGoals, active.levelGoals.size())
        && CategoryMapMatchesActive(map.dynamicBoxes, active.dynamicBoxes.size())
        && CategoryMapMatchesActive(map.pressurePlates, active.pressurePlates.size())
        && CategoryMapMatchesActive(map.doors, active.doors.size())
        && CategoryMapMatchesActive(map.itemPickups, active.itemPickups.size())
        && CategoryMapMatchesActive(map.staticProps, active.staticProps.size())
        && CategoryMapMatchesActive(map.pointLights, active.pointLights.size())
        && CategoryMapMatchesActive(map.spotLights, active.spotLights.size()))
    {
        return;
    }
    ResetStructuralIndexMap(map, active);
}

void ApplyLifecycleToStructuralMap(
    StructuralIndexMap& map,
    EditorObjectKind kind,
    bool deletedWorkingObject,
    std::size_t deletedWorkingIndex)
{
    if (!deletedWorkingObject)
    {
        return;
    }
    CategoryIndexMap* category = MutableCategoryMap(map, kind);
    if (category == nullptr)
    {
        return;
    }
    const int deleted = static_cast<int>(deletedWorkingIndex);
    for (int& workingIndex : category->activeToWorking)
    {
        if (workingIndex == deleted)
        {
            workingIndex = kNoStructuralIndex;
        }
        else if (workingIndex > deleted)
        {
            --workingIndex;
        }
    }
}

int MappedWorkingIndex(
    const StructuralIndexMap& map,
    EditorObjectKind kind,
    std::size_t activeIndex)
{
    const CategoryIndexMap* category = CategoryMap(map, kind);
    if (category == nullptr)
    {
        return SupportsLifecycle(kind) ? kNoStructuralIndex : static_cast<int>(activeIndex);
    }
    if (category->activeToWorking.empty())
    {
        return static_cast<int>(activeIndex);
    }
    if (activeIndex >= category->activeToWorking.size())
    {
        return kNoStructuralIndex;
    }
    return category->activeToWorking[activeIndex];
}

int MappedActiveIndex(
    const StructuralIndexMap& map,
    EditorObjectKind kind,
    std::size_t workingIndex)
{
    const CategoryIndexMap* category = CategoryMap(map, kind);
    if (category == nullptr)
    {
        return SupportsLifecycle(kind) ? kNoStructuralIndex : static_cast<int>(workingIndex);
    }
    if (category->activeToWorking.empty())
    {
        return kNoStructuralIndex;
    }
    const int wanted = static_cast<int>(workingIndex);
    for (std::size_t index = 0; index < category->activeToWorking.size(); ++index)
    {
        if (category->activeToWorking[index] == wanted)
        {
            return static_cast<int>(index);
        }
    }
    return kNoStructuralIndex;
}

bool IsPendingDeleteActiveIndex(
    const StructuralIndexMap& map,
    EditorObjectKind kind,
    std::size_t activeIndex)
{
    return SupportsLifecycle(kind)
        && MappedWorkingIndex(map, kind, activeIndex) == kNoStructuralIndex;
}

PendingDeleteVisuals MakePendingDeleteVisuals(
    const world::LevelDefinition& active,
    const StructuralIndexMap& map)
{
    PendingDeleteVisuals visuals{};
    const std::size_t platformLimit = active.elevatedPlatforms.size() < map.elevatedPlatforms.activeToWorking.size()
        ? active.elevatedPlatforms.size()
        : map.elevatedPlatforms.activeToWorking.size();
    for (std::size_t index = 0; index < platformLimit; ++index)
    {
        if (map.elevatedPlatforms.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.platformIndices.push_back(static_cast<int>(index));
        visuals.platforms.push_back(active.elevatedPlatforms[index]);
    }
    const std::size_t checkpointLimit = active.checkpoints.size() < map.checkpoints.activeToWorking.size()
        ? active.checkpoints.size()
        : map.checkpoints.activeToWorking.size();
    for (std::size_t index = 0; index < checkpointLimit; ++index)
    {
        if (map.checkpoints.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.checkpointIndices.push_back(static_cast<int>(index));
        visuals.checkpoints.push_back(active.checkpoints[index]);
    }
    const std::size_t hazardLimit = active.hazards.size() < map.hazards.activeToWorking.size()
        ? active.hazards.size()
        : map.hazards.activeToWorking.size();
    for (std::size_t index = 0; index < hazardLimit; ++index)
    {
        if (map.hazards.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.hazardIndices.push_back(static_cast<int>(index));
        visuals.hazards.push_back(active.hazards[index]);
    }
    const std::size_t collectibleLimit =
        active.collectibles.size() < map.collectibles.activeToWorking.size()
        ? active.collectibles.size()
        : map.collectibles.activeToWorking.size();
    for (std::size_t index = 0; index < collectibleLimit; ++index)
    {
        if (map.collectibles.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.collectibleIndices.push_back(static_cast<int>(index));
        visuals.collectibleCenters.push_back(active.collectibles[index].center);
    }
    const std::size_t levelGoalLimit =
        active.levelGoals.size() < map.levelGoals.activeToWorking.size()
        ? active.levelGoals.size()
        : map.levelGoals.activeToWorking.size();
    for (std::size_t index = 0; index < levelGoalLimit; ++index)
    {
        if (map.levelGoals.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.levelGoalIndices.push_back(static_cast<int>(index));
        visuals.levelGoals.push_back(active.levelGoals[index]);
    }
    const std::size_t dynamicBoxLimit =
        active.dynamicBoxes.size() < map.dynamicBoxes.activeToWorking.size()
        ? active.dynamicBoxes.size()
        : map.dynamicBoxes.activeToWorking.size();
    for (std::size_t index = 0; index < dynamicBoxLimit; ++index)
    {
        if (map.dynamicBoxes.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.dynamicBoxIndices.push_back(static_cast<int>(index));
        visuals.dynamicBoxes.push_back(active.dynamicBoxes[index]);
    }
    const std::size_t pressurePlateLimit =
        active.pressurePlates.size() < map.pressurePlates.activeToWorking.size()
        ? active.pressurePlates.size()
        : map.pressurePlates.activeToWorking.size();
    for (std::size_t index = 0; index < pressurePlateLimit; ++index)
    {
        if (map.pressurePlates.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.pressurePlateIndices.push_back(static_cast<int>(index));
        visuals.pressurePlates.push_back(active.pressurePlates[index]);
    }
    const std::size_t doorLimit = active.doors.size() < map.doors.activeToWorking.size()
        ? active.doors.size()
        : map.doors.activeToWorking.size();
    for (std::size_t index = 0; index < doorLimit; ++index)
    {
        if (map.doors.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.doorIndices.push_back(static_cast<int>(index));
        visuals.doors.push_back(active.doors[index]);
    }
    const std::size_t itemPickupLimit =
        active.itemPickups.size() < map.itemPickups.activeToWorking.size()
        ? active.itemPickups.size()
        : map.itemPickups.activeToWorking.size();
    for (std::size_t index = 0; index < itemPickupLimit; ++index)
    {
        if (map.itemPickups.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.itemPickupIndices.push_back(static_cast<int>(index));
        visuals.itemPickups.push_back(active.itemPickups[index]);
    }
    const std::size_t staticPropLimit =
        active.staticProps.size() < map.staticProps.activeToWorking.size()
        ? active.staticProps.size()
        : map.staticProps.activeToWorking.size();
    for (std::size_t index = 0; index < staticPropLimit; ++index)
    {
        if (map.staticProps.activeToWorking[index] != kNoStructuralIndex)
        {
            continue;
        }
        visuals.staticPropIndices.push_back(static_cast<int>(index));
        visuals.staticProps.push_back(active.staticProps[index]);
    }
    return visuals;
}

EditorSelection HighlightSelectionFromWorking(
    EditorSelection workingSelection,
    const StructuralIndexMap& map)
{
    if (workingSelection.kind == EditorObjectKind::None)
    {
        return ClearSelection();
    }
    if (!SupportsLifecycle(workingSelection.kind))
    {
        return workingSelection;
    }
    const int activeIndex =
        MappedActiveIndex(map, workingSelection.kind, workingSelection.index);
    if (activeIndex < 0)
    {
        return ClearSelection();
    }
    workingSelection.index = static_cast<std::size_t>(activeIndex);
    return workingSelection;
}

bool TryMapActiveWorldPick(
    EditorSelection activePick,
    const StructuralIndexMap& map,
    EditorSelection& outWorkingSelection)
{
    if (activePick.kind == EditorObjectKind::None)
    {
        outWorkingSelection = ClearSelection();
        return true;
    }
    if (!SupportsLifecycle(activePick.kind))
    {
        outWorkingSelection = activePick;
        return true;
    }
    const int workingIndex = MappedWorkingIndex(map, activePick.kind, activePick.index);
    if (workingIndex < 0)
    {
        return false;
    }
    outWorkingSelection = {activePick.kind, static_cast<std::size_t>(workingIndex)};
    return true;
}

bool ShouldAcceptActiveWorldPick(
    EditorSelection activePick,
    const StructuralIndexMap& map)
{
    EditorSelection unused{};
    return TryMapActiveWorldPick(activePick, map, unused);
}

EditorSelection ReconcileSelection(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    if (!IsValidSelection(workingCopy, selection))
    {
        return ClearSelection();
    }
    return selection;
}

LifecycleEditResult AddPlatformAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::ElevatedPlatform))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::Box platform{};
    platform.center = ApplyWorldCenter(worldCenter, {});
    platform.size = kDefaultAddedPlatformSize;
    workingCopy.elevatedPlatforms.push_back(platform);
    return Ok(
        {EditorObjectKind::ElevatedPlatform, workingCopy.elevatedPlatforms.size() - 1});
}

LifecycleEditResult AddCheckpointAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::Checkpoint))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::CheckpointSpec checkpoint{};
    checkpoint.center = ApplyWorldCenter(worldCenter, {});
    checkpoint.size = kDefaultAddedCheckpointSize;
    checkpoint.respawnPosition = {
        checkpoint.center.x + kDefaultAddedCheckpointRespawnOffset.x,
        checkpoint.center.y + kDefaultAddedCheckpointRespawnOffset.y,
        checkpoint.center.z + kDefaultAddedCheckpointRespawnOffset.z};
    workingCopy.checkpoints.push_back(checkpoint);
    return Ok({EditorObjectKind::Checkpoint, workingCopy.checkpoints.size() - 1});
}

LifecycleEditResult AddHazardAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::Hazard))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::HazardSpec hazard{};
    hazard.center = ApplyWorldCenter(worldCenter, {});
    hazard.size = kDefaultAddedHazardSize;
    workingCopy.hazards.push_back(hazard);
    return Ok({EditorObjectKind::Hazard, workingCopy.hazards.size() - 1});
}

LifecycleEditResult AddCollectibleAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::Collectible))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::CollectibleSpec collectible{};
    collectible.center = ApplyWorldCenter(worldCenter, {});
    collectible.size = kDefaultAddedCollectibleSize;
    workingCopy.collectibles.push_back(collectible);
    return Ok({EditorObjectKind::Collectible, workingCopy.collectibles.size() - 1});
}

LifecycleEditResult AddGoalAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::Goal))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::LevelGoalSpec goal{};
    goal.center = ApplyWorldCenter(worldCenter, {});
    goal.size = world::kDefaultLevelGoalSize;
    workingCopy.levelGoals.push_back(goal);
    return Ok({EditorObjectKind::Goal, workingCopy.levelGoals.size() - 1});
}

LifecycleEditResult AddDynamicBoxAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::DynamicBox))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::DynamicBoxSpec box{};
    box.center = ApplyWorldCenter(worldCenter, {});
    box.size = world::kDefaultDynamicBoxSize;
    box.massKg = world::kDefaultDynamicBoxMassKg;
    workingCopy.dynamicBoxes.push_back(box);
    return Ok({EditorObjectKind::DynamicBox, workingCopy.dynamicBoxes.size() - 1});
}

LifecycleEditResult AddPressurePlateAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::PressurePlate))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::PressurePlateSpec plate{};
    plate.center = ApplyWorldCenter(worldCenter, {});
    plate.size = world::kDefaultPressurePlateSize;
    workingCopy.pressurePlates.push_back(plate);
    return Ok({EditorObjectKind::PressurePlate, workingCopy.pressurePlates.size() - 1});
}

LifecycleEditResult AddDoorAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::Door))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::DoorSpec door{};
    door.center = ApplyWorldCenter(worldCenter, {});
    door.size = world::kDefaultDoorSize;
    door.openDistance = world::kDefaultDoorOpenDistance;
    workingCopy.doors.push_back(door);
    return Ok({EditorObjectKind::Door, workingCopy.doors.size() - 1});
}

LifecycleEditResult AddItemPickupAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::ItemPickup))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::ItemPickupSpec pickup{};
    pickup.position = ApplyWorldCenter(worldCenter, {});
    pickup.itemId = std::string(world::kDefaultItemPickupId);
    pickup.quantity = world::kDefaultItemPickupQuantity;
    workingCopy.itemPickups.push_back(pickup);
    return Ok({EditorObjectKind::ItemPickup, workingCopy.itemPickups.size() - 1});
}

LifecycleEditResult AddPlatform(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddPlatformAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedPlatformOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddCheckpoint(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddCheckpointAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedCheckpointOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddHazard(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddHazardAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedHazardOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddCollectible(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddCollectibleAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedCollectibleOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddGoal(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddGoalAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedLevelGoalOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddDynamicBox(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddDynamicBoxAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedDynamicBoxOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddPressurePlate(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddPressurePlateAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedPressurePlateOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddDoor(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddDoorAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedDoorOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddItemPickup(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddItemPickupAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedItemPickupOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddStaticPropAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter,
    std::string_view modelIdentity)
{
    if (!world::StaticPropIdentityIsValid(modelIdentity))
    {
        return Fail(LifecycleEditStatus::InvalidAssetReference);
    }
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::StaticProp))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    world::StaticPropSpec prop{};
    prop.modelIdentity = std::string(modelIdentity);
    prop.position = ApplyWorldCenter(worldCenter, {});
    prop.rotationDegrees = world::kDefaultStaticPropRotationDegrees;
    prop.scale = world::kDefaultStaticPropScale;
    workingCopy.staticProps.push_back(prop);
    return Ok({EditorObjectKind::StaticProp, workingCopy.staticProps.size() - 1});
}

LifecycleEditResult AddPointLightAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::PointLight))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    workingCopy.pointLights.push_back(
        world::MakeDefaultPointLight(ApplyWorldCenter(worldCenter, {})));
    return Ok({EditorObjectKind::PointLight, workingCopy.pointLights.size() - 1});
}

LifecycleEditResult AddSpotLightAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter)
{
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::SpotLight))
    {
        return Fail(LifecycleEditStatus::AtLimit);
    }

    workingCopy.spotLights.push_back(
        world::MakeDefaultSpotLight(ApplyWorldCenter(worldCenter, {})));
    return Ok({EditorObjectKind::SpotLight, workingCopy.spotLights.size() - 1});
}

LifecycleEditResult AddPointLight(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddPointLightAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedPointLightOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddSpotLight(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor)
{
    return AddSpotLightAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedSpotLightOffset,
            workingCopy.initialSpawnVisualCenter.z));
}

LifecycleEditResult AddStaticProp(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor,
    std::string_view modelIdentity)
{
    return AddStaticPropAt(
        workingCopy,
        ApplyPlacementAnchor(
            placementAnchor,
            kDefaultAddedStaticPropOffset,
            workingCopy.initialSpawnVisualCenter.z),
        modelIdentity);
}

LifecycleEditResult DuplicateSelected(
    world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    if (!SupportsLifecycle(selection.kind))
    {
        return Fail(LifecycleEditStatus::UnsupportedType, selection);
    }
    if (!IsValidSelection(workingCopy, selection))
    {
        return Fail(LifecycleEditStatus::InvalidSelection, selection);
    }
    if (CategoryAtCountLimit(workingCopy, selection.kind))
    {
        return Fail(LifecycleEditStatus::AtLimit, selection);
    }

    switch (selection.kind)
    {
    case EditorObjectKind::ElevatedPlatform:
    {
        world::Box copy = workingCopy.elevatedPlatforms[selection.index];
        OffsetX(copy.center, kLifecycleDuplicateOffsetX);
        workingCopy.elevatedPlatforms.push_back(copy);
        return Ok(
            {EditorObjectKind::ElevatedPlatform, workingCopy.elevatedPlatforms.size() - 1});
    }
    case EditorObjectKind::Checkpoint:
    {
        world::CheckpointSpec copy = workingCopy.checkpoints[selection.index];
        OffsetX(copy.center, kLifecycleDuplicateOffsetX);
        OffsetX(copy.respawnPosition, kLifecycleDuplicateOffsetX);
        workingCopy.checkpoints.push_back(copy);
        return Ok({EditorObjectKind::Checkpoint, workingCopy.checkpoints.size() - 1});
    }
    case EditorObjectKind::Hazard:
    {
        world::HazardSpec copy = workingCopy.hazards[selection.index];
        OffsetX(copy.center, kLifecycleDuplicateOffsetX);
        workingCopy.hazards.push_back(copy);
        return Ok({EditorObjectKind::Hazard, workingCopy.hazards.size() - 1});
    }
    case EditorObjectKind::Collectible:
    {
        world::CollectibleSpec copy = workingCopy.collectibles[selection.index];
        OffsetX(copy.center, kLifecycleDuplicateOffsetX);
        workingCopy.collectibles.push_back(copy);
        return Ok({EditorObjectKind::Collectible, workingCopy.collectibles.size() - 1});
    }
    case EditorObjectKind::Goal:
    {
        world::LevelGoalSpec copy = workingCopy.levelGoals[selection.index];
        OffsetX(copy.center, kLifecycleDuplicateOffsetX);
        workingCopy.levelGoals.push_back(copy);
        return Ok({EditorObjectKind::Goal, workingCopy.levelGoals.size() - 1});
    }
    case EditorObjectKind::DynamicBox:
    {
        world::DynamicBoxSpec copy = workingCopy.dynamicBoxes[selection.index];
        OffsetX(copy.center, kLifecycleDuplicateOffsetX);
        workingCopy.dynamicBoxes.push_back(copy);
        return Ok({EditorObjectKind::DynamicBox, workingCopy.dynamicBoxes.size() - 1});
    }
    case EditorObjectKind::PressurePlate:
    {
        world::PressurePlateSpec copy = workingCopy.pressurePlates[selection.index];
        OffsetX(copy.center, kLifecycleDuplicateOffsetX);
        workingCopy.pressurePlates.push_back(copy);
        return Ok({EditorObjectKind::PressurePlate, workingCopy.pressurePlates.size() - 1});
    }
    case EditorObjectKind::Door:
    {
        world::DoorSpec copy = workingCopy.doors[selection.index];
        OffsetX(copy.center, kLifecycleDuplicateOffsetX);
        workingCopy.doors.push_back(copy);
        return Ok({EditorObjectKind::Door, workingCopy.doors.size() - 1});
    }
    case EditorObjectKind::ItemPickup:
    {
        world::ItemPickupSpec copy = workingCopy.itemPickups[selection.index];
        OffsetX(copy.position, kLifecycleDuplicateOffsetX);
        workingCopy.itemPickups.push_back(copy);
        return Ok({EditorObjectKind::ItemPickup, workingCopy.itemPickups.size() - 1});
    }
    case EditorObjectKind::StaticProp:
    {
        world::StaticPropSpec copy = workingCopy.staticProps[selection.index];
        OffsetX(copy.position, kLifecycleDuplicateOffsetX);
        workingCopy.staticProps.push_back(copy);
        return Ok({EditorObjectKind::StaticProp, workingCopy.staticProps.size() - 1});
    }
    case EditorObjectKind::PointLight:
    {
        world::PointLightSpec copy = workingCopy.pointLights[selection.index];
        OffsetX(copy.position, kLifecycleDuplicateOffsetX);
        workingCopy.pointLights.push_back(copy);
        return Ok({EditorObjectKind::PointLight, workingCopy.pointLights.size() - 1});
    }
    case EditorObjectKind::SpotLight:
    {
        world::SpotLightSpec copy = workingCopy.spotLights[selection.index];
        OffsetX(copy.position, kLifecycleDuplicateOffsetX);
        workingCopy.spotLights.push_back(copy);
        return Ok({EditorObjectKind::SpotLight, workingCopy.spotLights.size() - 1});
    }
    default:
        break;
    }
    return Fail(LifecycleEditStatus::UnsupportedType, selection);
}

bool IsAuthoredPlatformReferenced(const world::LevelDefinition& level, std::size_t platformIndex)
{
    const int index = static_cast<int>(platformIndex);
    return level.checkpoint1PlatformIndex == index || level.checkpoint2PlatformIndex == index
        || level.goalPlatformIndex == index;
}

LifecycleEditResult DeleteSelected(
    world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    if (!SupportsLifecycle(selection.kind))
    {
        return Fail(LifecycleEditStatus::UnsupportedType, selection);
    }
    if (!IsValidSelection(workingCopy, selection))
    {
        return Fail(LifecycleEditStatus::InvalidSelection, selection);
    }

    switch (selection.kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        if (workingCopy.elevatedPlatforms.size()
            <= static_cast<std::size_t>(world::kMinElevatedPlatformCount))
        {
            return Fail(LifecycleEditStatus::MinimumCount, selection);
        }
        if (IsAuthoredPlatformReferenced(workingCopy, selection.index))
        {
            return Fail(LifecycleEditStatus::ReferencedPlatform, selection);
        }
        RemapSupportIndexAfterPlatformDelete(
            workingCopy.checkpoint1PlatformIndex, selection.index);
        RemapSupportIndexAfterPlatformDelete(
            workingCopy.checkpoint2PlatformIndex, selection.index);
        RemapSupportIndexAfterPlatformDelete(workingCopy.goalPlatformIndex, selection.index);
        workingCopy.elevatedPlatforms.erase(
            workingCopy.elevatedPlatforms.begin()
            + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::Checkpoint:
        workingCopy.checkpoints.erase(
            workingCopy.checkpoints.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::Hazard:
        workingCopy.hazards.erase(
            workingCopy.hazards.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::Collectible:
        workingCopy.collectibles.erase(
            workingCopy.collectibles.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::Goal:
        workingCopy.levelGoals.erase(
            workingCopy.levelGoals.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::DynamicBox:
        workingCopy.dynamicBoxes.erase(
            workingCopy.dynamicBoxes.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::PressurePlate:
        workingCopy.pressurePlates.erase(
            workingCopy.pressurePlates.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::Door:
        RemapPressurePlateDoorLinksAfterDoorDelete(workingCopy, selection.index);
        workingCopy.doors.erase(
            workingCopy.doors.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::ItemPickup:
        workingCopy.itemPickups.erase(
            workingCopy.itemPickups.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::StaticProp:
        workingCopy.staticProps.erase(
            workingCopy.staticProps.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::PointLight:
        workingCopy.pointLights.erase(
            workingCopy.pointLights.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::SpotLight:
        workingCopy.spotLights.erase(
            workingCopy.spotLights.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    default:
        return Fail(LifecycleEditStatus::UnsupportedType, selection);
    }

    world::AuthoringGroupMemberKind groupKind{};
    if (TryAuthoringGroupMemberKindFromSelection(selection.kind, groupKind))
    {
        world::RemapAuthoringGroupsAfterDelete(workingCopy, groupKind, selection.index);
    }

    return Ok(ClearSelection());
}

std::vector<EditorSelection> MakeDescendingCategoryDeletePlan(
    const std::vector<EditorSelection>& members)
{
    std::vector<EditorSelection> plan;
    for (const EditorSelection& member : members)
    {
        if (EditorSelectionIsNone(member) || ContainsEditorSelection(plan, member))
        {
            continue;
        }
        plan.push_back(member);
    }
    std::sort(
        plan.begin(),
        plan.end(),
        [](EditorSelection a, EditorSelection b)
        {
            if (a.kind != b.kind)
            {
                return static_cast<int>(a.kind) < static_cast<int>(b.kind);
            }
            return a.index > b.index;
        });
    return plan;
}

LifecycleEditResult DuplicateSelectionSet(
    world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    const std::vector<EditorSelection> members =
        EditorSelectionSetMembers(primary, additional);
    if (members.empty())
    {
        return Fail(LifecycleEditStatus::InvalidSelection, primary);
    }

    for (const EditorSelection& member : members)
    {
        if (!SupportsLifecycle(member.kind))
        {
            return Fail(LifecycleEditStatus::UnsupportedType, primary);
        }
        if (!IsValidSelection(workingCopy, member))
        {
            return Fail(LifecycleEditStatus::InvalidSelection, primary);
        }
    }

    if (!SelectionAllowsAuthoringGroupDuplicate(workingCopy, primary, additional))
    {
        return Fail(LifecycleEditStatus::InvalidGroupOperation, primary);
    }

    const std::size_t completeGroupIndex =
        FindExactAuthoringGroup(workingCopy, primary, additional);
    std::string copiedGroupSourceName;
    if (completeGroupIndex != kNoAuthoringGroupIndex)
    {
        copiedGroupSourceName = workingCopy.authoringGroups[completeGroupIndex].name;
    }

    if (additional.empty())
    {
        LifecycleEditResult result = DuplicateSelected(workingCopy, primary);
        if (result.succeeded)
        {
            result.appliedPlan = members;
        }
        return result;
    }

    world::LevelDefinition trial = workingCopy;
    EditorSelection newPrimary{};
    std::vector<EditorSelection> newAdditional;
    for (const EditorSelection& member : members)
    {
        const LifecycleEditResult duplicated = DuplicateSelected(trial, member);
        if (!duplicated.succeeded)
        {
            return Fail(duplicated.status, primary);
        }
        if (member == primary)
        {
            newPrimary = duplicated.selection;
        }
        else
        {
            newAdditional.push_back(duplicated.selection);
        }
    }

    if (!copiedGroupSourceName.empty()
        && !TryCreateCopiedAuthoringGroup(
            trial, copiedGroupSourceName, newPrimary, newAdditional))
    {
        return Fail(LifecycleEditStatus::InvalidGroupOperation, primary);
    }

    workingCopy = std::move(trial);
    LifecycleEditResult result = Ok(newPrimary);
    result.additionalSelections = std::move(newAdditional);
    result.appliedPlan = members;
    SanitizeEditorSelectionSet(result.selection, result.additionalSelections);
    return result;
}

LifecycleEditResult DeleteSelectionSet(
    world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    const std::vector<EditorSelection> members =
        EditorSelectionSetMembers(primary, additional);
    if (members.empty())
    {
        return Fail(LifecycleEditStatus::InvalidSelection, primary);
    }

    const std::vector<EditorSelection> plan = MakeDescendingCategoryDeletePlan(members);
    for (const EditorSelection& member : plan)
    {
        if (!SupportsLifecycle(member.kind))
        {
            return Fail(LifecycleEditStatus::UnsupportedType, primary);
        }
        if (!IsValidSelection(workingCopy, member))
        {
            return Fail(LifecycleEditStatus::InvalidSelection, primary);
        }
    }

    if (additional.empty())
    {
        LifecycleEditResult result = DeleteSelected(workingCopy, primary);
        if (result.succeeded)
        {
            result.appliedPlan = plan;
        }
        return result;
    }

    world::LevelDefinition trial = workingCopy;
    for (const EditorSelection& member : plan)
    {
        const LifecycleEditResult deleted = DeleteSelected(trial, member);
        if (!deleted.succeeded)
        {
            return Fail(deleted.status, primary);
        }
    }

    workingCopy = std::move(trial);
    LifecycleEditResult result = Ok(ClearSelection());
    result.appliedPlan = plan;
    return result;
}

bool CanAddLifecycleObject(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorObjectKind kind,
    bool gizmoDragging)
{
    return authoringAvailable && !gizmoDragging && SupportsLifecycle(kind)
        && !CategoryAtCountLimit(workingCopy, kind);
}

bool CanDuplicateSelected(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging)
{
    return authoringAvailable && !gizmoDragging && SupportsLifecycle(selection.kind)
        && IsValidSelection(workingCopy, selection)
        && !CategoryAtCountLimit(workingCopy, selection.kind);
}

bool CanDuplicateSelected(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional,
    bool gizmoDragging)
{
    if (additional.empty())
    {
        return CanDuplicateSelected(authoringAvailable, workingCopy, primary, gizmoDragging);
    }
    if (!authoringAvailable || gizmoDragging)
    {
        return false;
    }
    world::LevelDefinition trial = workingCopy;
    return DuplicateSelectionSet(trial, primary, additional).succeeded;
}

bool CanDeleteSelected(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging)
{
    if (!authoringAvailable || gizmoDragging || !SupportsLifecycle(selection.kind)
        || !IsValidSelection(workingCopy, selection))
    {
        return false;
    }
    if (selection.kind == EditorObjectKind::ElevatedPlatform)
    {
        if (workingCopy.elevatedPlatforms.size()
            <= static_cast<std::size_t>(world::kMinElevatedPlatformCount))
        {
            return false;
        }
        if (IsAuthoredPlatformReferenced(workingCopy, selection.index))
        {
            return false;
        }
    }
    return true;
}

bool CanDeleteSelected(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional,
    bool gizmoDragging)
{
    if (additional.empty())
    {
        return CanDeleteSelected(authoringAvailable, workingCopy, primary, gizmoDragging);
    }
    if (!authoringAvailable || gizmoDragging)
    {
        return false;
    }
    world::LevelDefinition trial = workingCopy;
    return DeleteSelectionSet(trial, primary, additional).succeeded;
}

const char* DuplicateSelectedDisableReason(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging,
    const std::vector<EditorSelection>& additional)
{
    if (CanDuplicateSelected(authoringAvailable, workingCopy, selection, additional, gizmoDragging))
    {
        return nullptr;
    }
    if (!authoringAvailable)
    {
        return "Lifecycle editing is available in Development only.";
    }
    if (gizmoDragging)
    {
        return "Lifecycle blocked: finish the gizmo drag first.";
    }
    const std::vector<EditorSelection> members =
        EditorSelectionSetMembers(selection, additional);
    const bool multiSelected = EditorSelectionSetIsMulti(selection, additional);
    for (const EditorSelection& member : members)
    {
        if (!SupportsLifecycle(member.kind) || !IsValidSelection(workingCopy, member))
        {
            return multiSelected
                ? "Duplicate blocked: selection contains an unsupported object."
                : "Duplicate requires a supported selection.";
        }
    }
    if (members.empty())
    {
        return "Duplicate requires a supported selection.";
    }
    if (!SelectionAllowsAuthoringGroupDuplicate(workingCopy, selection, additional))
    {
        return AuthoringGroupDuplicateDisableReason(workingCopy, selection, additional);
    }
    return CategoryCapacityReason(selection.kind);
}

const char* DeleteSelectedDisableReason(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging,
    const std::vector<EditorSelection>& additional)
{
    if (CanDeleteSelected(authoringAvailable, workingCopy, selection, additional, gizmoDragging))
    {
        return nullptr;
    }
    if (!authoringAvailable)
    {
        return "Lifecycle editing is available in Development only.";
    }
    if (gizmoDragging)
    {
        return "Lifecycle blocked: finish the gizmo drag first.";
    }

    const std::vector<EditorSelection> members =
        EditorSelectionSetMembers(selection, additional);
    const bool multiSelected = EditorSelectionSetIsMulti(selection, additional);
    std::size_t selectedPlatforms = 0;
    bool referencedPlatform = false;
    for (const EditorSelection& member : members)
    {
        if (!SupportsLifecycle(member.kind) || !IsValidSelection(workingCopy, member))
        {
            return multiSelected
                ? "Delete blocked: selection contains an unsupported object."
                : "Delete requires a supported selection.";
        }
        if (member.kind == EditorObjectKind::ElevatedPlatform)
        {
            ++selectedPlatforms;
            if (IsAuthoredPlatformReferenced(workingCopy, member.index))
            {
                referencedPlatform = true;
            }
        }
    }
    if (selectedPlatforms > 0
        && workingCopy.elevatedPlatforms.size() < selectedPlatforms
            + static_cast<std::size_t>(world::kMinElevatedPlatformCount))
    {
        return "At least one Platform is required.";
    }
    if (referencedPlatform)
    {
        return "Platform is referenced by checkpoint/goal support metadata.";
    }
    if (members.empty() || !SupportsLifecycle(selection.kind)
        || !IsValidSelection(workingCopy, selection))
    {
        return "Delete requires a supported selection.";
    }
    return "Delete Selected is unavailable.";
}

bool ShouldEmitDeleteSelectedRequest(
    bool deletePressed,
    bool imguiWantsKeyboard,
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging,
    const std::vector<EditorSelection>& additional)
{
    if (!deletePressed || imguiWantsKeyboard)
    {
        return false;
    }
    return CanDeleteSelected(
        authoringAvailable, workingCopy, selection, additional, gizmoDragging);
}
}
