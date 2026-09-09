#include "editor/AuthoredObjectLifecycle.h"

#include "physics/PhysicsCapacity.h"
#include "world/LevelFile.h"

#include <cmath>
#include <cstddef>
#include <string>

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
    case EditorObjectKind::DynamicBox:
    case EditorObjectKind::StaticProp:
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
        return physics::kMaxAuthoredPhysicsBodies;
    case EditorObjectKind::Checkpoint:
    case EditorObjectKind::Hazard:
    case EditorObjectKind::Collectible:
    case EditorObjectKind::StaticProp:
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
    case EditorObjectKind::DynamicBox:
        return level.dynamicBoxes.size();
    case EditorObjectKind::StaticProp:
        return level.staticProps.size();
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
    if (kind == EditorObjectKind::ElevatedPlatform || kind == EditorObjectKind::DynamicBox)
    {
        const int nextPlatforms = static_cast<int>(level.elevatedPlatforms.size())
            + (kind == EditorObjectKind::ElevatedPlatform ? 1 : 0);
        const int nextBoxes = static_cast<int>(level.dynamicBoxes.size())
            + (kind == EditorObjectKind::DynamicBox ? 1 : 0);
        if (!physics::AuthoredPhysicsBodiesWithinBudget(nextPlatforms, nextBoxes))
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
    case EditorObjectKind::DynamicBox:
        pending.dynamicBoxes = true;
        break;
    case EditorObjectKind::StaticProp:
        pending.staticProps = true;
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
    case EditorObjectKind::DynamicBox:
        return pending.dynamicBoxes;
    case EditorObjectKind::StaticProp:
        return pending.staticProps;
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
    case EditorObjectKind::DynamicBox:
        return &map.dynamicBoxes;
    case EditorObjectKind::StaticProp:
        return &map.staticProps;
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
    case EditorObjectKind::DynamicBox:
        return &map.dynamicBoxes;
    case EditorObjectKind::StaticProp:
        return &map.staticProps;
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
    FillIdentity(map.dynamicBoxes, active.dynamicBoxes.size());
    FillIdentity(map.staticProps, active.staticProps.size());
}

void EnsureStructuralIndexMap(
    StructuralIndexMap& map,
    const world::LevelDefinition& active)
{
    if (CategoryMapMatchesActive(map.elevatedPlatforms, active.elevatedPlatforms.size())
        && CategoryMapMatchesActive(map.checkpoints, active.checkpoints.size())
        && CategoryMapMatchesActive(map.hazards, active.hazards.size())
        && CategoryMapMatchesActive(map.collectibles, active.collectibles.size())
        && CategoryMapMatchesActive(map.dynamicBoxes, active.dynamicBoxes.size())
        && CategoryMapMatchesActive(map.staticProps, active.staticProps.size()))
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
    case EditorObjectKind::DynamicBox:
    {
        world::DynamicBoxSpec copy = workingCopy.dynamicBoxes[selection.index];
        OffsetX(copy.center, kLifecycleDuplicateOffsetX);
        workingCopy.dynamicBoxes.push_back(copy);
        return Ok({EditorObjectKind::DynamicBox, workingCopy.dynamicBoxes.size() - 1});
    }
    case EditorObjectKind::StaticProp:
    {
        world::StaticPropSpec copy = workingCopy.staticProps[selection.index];
        OffsetX(copy.position, kLifecycleDuplicateOffsetX);
        workingCopy.staticProps.push_back(copy);
        return Ok({EditorObjectKind::StaticProp, workingCopy.staticProps.size() - 1});
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
    case EditorObjectKind::DynamicBox:
        workingCopy.dynamicBoxes.erase(
            workingCopy.dynamicBoxes.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    case EditorObjectKind::StaticProp:
        workingCopy.staticProps.erase(
            workingCopy.staticProps.begin() + static_cast<std::ptrdiff_t>(selection.index));
        break;
    default:
        return Fail(LifecycleEditStatus::UnsupportedType, selection);
    }

    return Ok(ClearSelection());
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

const char* DeleteSelectedDisableReason(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging)
{
    if (CanDeleteSelected(authoringAvailable, workingCopy, selection, gizmoDragging))
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
    if (selection.kind == EditorObjectKind::ElevatedPlatform
        && IsValidSelection(workingCopy, selection))
    {
        if (workingCopy.elevatedPlatforms.size()
            <= static_cast<std::size_t>(world::kMinElevatedPlatformCount))
        {
            return "At least one Platform is required.";
        }
        if (IsAuthoredPlatformReferenced(workingCopy, selection.index))
        {
            return "Platform is referenced by checkpoint/goal support metadata.";
        }
    }
    if (!SupportsLifecycle(selection.kind) || !IsValidSelection(workingCopy, selection))
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
    bool gizmoDragging)
{
    if (!deletePressed || imguiWantsKeyboard)
    {
        return false;
    }
    return CanDeleteSelected(authoringAvailable, workingCopy, selection, gizmoDragging);
}
}
