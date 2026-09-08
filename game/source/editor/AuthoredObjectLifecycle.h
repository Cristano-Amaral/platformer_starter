#pragma once

// Pure working-copy lifecycle for M41 repeatable authored objects.
// No ImGui. No Apply/Save/physics. Tests and Phase B menu share this.

#include "editor/EditorSelection.h"
#include "world/LevelDefinition.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace editor
{
// World +X offset applied to a duplicated object's center (and checkpoint
// respawn). One policy for all four categories; 1 unit is smaller than a
// Level 01 platform (~4) and visible next to a collectible (~1).
inline constexpr float kLifecycleDuplicateOffsetX = 1.0f;

// Add uses camera-local X/Y from placementAnchor and gameplay-lane Z from
// workingCopy.initialSpawnVisualCenter.z (Level Format v1 has no lane field).
// Lifecycle never reads EditorCamera or raylib. Spawn X/Y do not affect Add.
// Duplicate does not snap to the lane.
inline constexpr core::Vec3 kDefaultAddedPlatformOffset{0.0f, 0.0f, 0.0f};
inline constexpr core::Vec3 kDefaultAddedPlatformSize{4.0f, 0.5f, 3.0f};
inline constexpr core::Vec3 kDefaultAddedCheckpointOffset{0.0f, 0.0f, 0.0f};
inline constexpr core::Vec3 kDefaultAddedCheckpointSize{2.4f, 1.6f, 2.0f};
// Canonical Level 01: respawnPosition == trigger center (player visual center).
inline constexpr core::Vec3 kDefaultAddedCheckpointRespawnOffset{0.0f, 0.0f, 0.0f};
inline constexpr core::Vec3 kDefaultAddedHazardOffset{0.0f, 0.0f, 0.0f};
inline constexpr core::Vec3 kDefaultAddedHazardSize{1.4f, 1.0f, 2.0f};
inline constexpr core::Vec3 kDefaultAddedCollectibleOffset{0.0f, 0.0f, 0.0f};
inline constexpr core::Vec3 kDefaultAddedCollectibleSize{1.0f, 1.2f, 1.0f};
inline constexpr core::Vec3 kDefaultAddedDynamicBoxOffset{0.0f, 0.0f, 0.0f};

struct CategoryStructuralPending
{
    bool elevatedPlatforms = false;
    bool checkpoints = false;
    bool hazards = false;
    bool collectibles = false;
    bool dynamicBoxes = false;
};

inline void ClearCategoryStructuralPending(CategoryStructuralPending& pending)
{
    pending = {};
}

// Session-local active index -> workingCopy index. Not a GUID, not serialized.
// kNoStructuralIndex means the active object is pending-deleted.
inline constexpr int kNoStructuralIndex = -1;

struct CategoryIndexMap
{
    std::vector<int> activeToWorking;
};

struct StructuralIndexMap
{
    CategoryIndexMap elevatedPlatforms;
    CategoryIndexMap checkpoints;
    CategoryIndexMap hazards;
    CategoryIndexMap collectibles;
    CategoryIndexMap dynamicBoxes;
};

struct PendingDeleteVisuals
{
    std::vector<int> platformIndices;
    std::vector<world::Box> platforms;
    std::vector<int> checkpointIndices;
    std::vector<world::CheckpointSpec> checkpoints;
    std::vector<int> hazardIndices;
    std::vector<world::HazardSpec> hazards;
    std::vector<int> collectibleIndices;
    std::vector<core::Vec3> collectibleCenters;
    std::vector<int> dynamicBoxIndices;
    std::vector<world::DynamicBoxSpec> dynamicBoxes;
};

// Tiny editor visual mode. Not a render-state / material architecture.
// Cyan pending Add/Duplicate/Translate ghosts are a separate overlay policy.
enum class AuthoredEditorVisualMode
{
    Normal,
    PendingDelete,
};

enum class CollectibleEditorVisualMode
{
    Normal,
    CollectedAuthored,
    PendingDelete,
};

// Editor visual precedence for a single active object:
// 1. pending delete
// 2. pending workingCopy transform/add ghost (cyan overlay; not this enum)
// 3. collected authored-only visual
// 4. normal active runtime visual
inline AuthoredEditorVisualMode ResolveAuthoredEditorVisualMode(bool pendingDelete)
{
    return pendingDelete ? AuthoredEditorVisualMode::PendingDelete
                         : AuthoredEditorVisualMode::Normal;
}

inline CollectibleEditorVisualMode ResolveCollectibleEditorVisualMode(
    bool pendingDelete,
    std::uint8_t collectedFlag)
{
    if (pendingDelete)
    {
        return CollectibleEditorVisualMode::PendingDelete;
    }
    if (collectedFlag != 0)
    {
        return CollectibleEditorVisualMode::CollectedAuthored;
    }
    return CollectibleEditorVisualMode::Normal;
}

enum class LifecycleEditStatus
{
    Success,
    InvalidSelection,
    UnsupportedType,
    AtLimit,
    ReferencedPlatform,
    MinimumCount,
};

struct LifecycleEditResult
{
    bool succeeded = false;
    LifecycleEditStatus status = LifecycleEditStatus::InvalidSelection;
    EditorSelection selection{};
};

// Authored v1 support_index_* fields. Not gameplay runtime state.
bool IsAuthoredPlatformReferenced(const world::LevelDefinition& level, std::size_t platformIndex);

bool SupportsLifecycle(EditorObjectKind kind);
bool CategoryAtCountLimit(const world::LevelDefinition& level, EditorObjectKind kind);
std::size_t CategoryCount(const world::LevelDefinition& level, EditorObjectKind kind);
int CategoryMaxCount(EditorObjectKind kind);

void MarkCategoryStructuralPending(CategoryStructuralPending& pending, EditorObjectKind kind);
bool CategoryHasStructuralPending(
    EditorObjectKind kind,
    const CategoryStructuralPending& pending);

void ResetStructuralIndexMap(
    StructuralIndexMap& map,
    const world::LevelDefinition& active);
void EnsureStructuralIndexMap(
    StructuralIndexMap& map,
    const world::LevelDefinition& active);
void ApplyLifecycleToStructuralMap(
    StructuralIndexMap& map,
    EditorObjectKind kind,
    bool deletedWorkingObject,
    std::size_t deletedWorkingIndex);

int MappedWorkingIndex(
    const StructuralIndexMap& map,
    EditorObjectKind kind,
    std::size_t activeIndex);
int MappedActiveIndex(
    const StructuralIndexMap& map,
    EditorObjectKind kind,
    std::size_t workingIndex);
bool IsPendingDeleteActiveIndex(
    const StructuralIndexMap& map,
    EditorObjectKind kind,
    std::size_t activeIndex);

PendingDeleteVisuals MakePendingDeleteVisuals(
    const world::LevelDefinition& active,
    const StructuralIndexMap& map);

EditorSelection HighlightSelectionFromWorking(
    EditorSelection workingSelection,
    const StructuralIndexMap& map);

// Maps an active-world pick onto a workingCopy selection. Pending-deleted
// active objects have no working counterpart: the pick is ignored.
bool TryMapActiveWorldPick(
    EditorSelection activePick,
    const StructuralIndexMap& map,
    EditorSelection& outWorkingSelection);
bool ShouldAcceptActiveWorldPick(
    EditorSelection activePick,
    const StructuralIndexMap& map);

EditorSelection ReconcileSelection(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection);

// Camera-local X/Y, gameplay-lane Z. Camera-anchor Z is ignored.
core::Vec3 MakeAuthoredAddPlacement(core::Vec3 cameraAnchor, float gameplayLaneZ);

// Edit > Add: camera-region X/Y, gameplay-lane Z from spawn.z.
LifecycleEditResult AddPlatform(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor);
LifecycleEditResult AddCheckpoint(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor);
LifecycleEditResult AddHazard(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor);
LifecycleEditResult AddCollectible(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor);
LifecycleEditResult AddDynamicBox(
    world::LevelDefinition& workingCopy,
    core::Vec3 placementAnchor);

// Object Palette confirm: authored center is worldCenter. Does not snap to spawn.z.
LifecycleEditResult AddPlatformAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter);
LifecycleEditResult AddCheckpointAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter);
LifecycleEditResult AddHazardAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter);
LifecycleEditResult AddCollectibleAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter);
LifecycleEditResult AddDynamicBoxAt(
    world::LevelDefinition& workingCopy,
    core::Vec3 worldCenter);

inline const char* CategoryCapacityReason(EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        return "Physics body capacity reached.";
    case EditorObjectKind::Checkpoint:
    case EditorObjectKind::Hazard:
    case EditorObjectKind::Collectible:
        return "Level file record limit reached.";
    case EditorObjectKind::DynamicBox:
        return "Physics body capacity reached.";
    default:
        return "Technical capacity reached.";
    }
}

LifecycleEditResult DuplicateSelected(
    world::LevelDefinition& workingCopy,
    EditorSelection selection);

// On success, selection is cleared (no dangling type+index).
LifecycleEditResult DeleteSelected(
    world::LevelDefinition& workingCopy,
    EditorSelection selection);

// Phase B enable rules. Dirty is not a parameter and does not block.
bool CanAddLifecycleObject(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorObjectKind kind,
    bool gizmoDragging);
bool CanDuplicateSelected(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging);
bool CanDeleteSelected(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging);

// nullptr when Delete is enabled. Short reason for disabled menu/key.
const char* DeleteSelectedDisableReason(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging);

// Delete key uses the same enable rules as Edit > Delete Selected.
bool ShouldEmitDeleteSelectedRequest(
    bool deletePressed,
    bool imguiWantsKeyboard,
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging);
}
