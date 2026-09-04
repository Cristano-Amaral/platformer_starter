#pragma once

// Fixed Level 01 hierarchy enumeration. Not a scene graph, not a tree model.

#include "editor/EditorSelection.h"

#include <cstddef>

namespace editor
{
struct HierarchyEntry
{
    EditorSelection selection{};
    const char* group = "";
    const char* label = "";
};

inline constexpr HierarchyEntry kHierarchyEntries[] = {
    {{EditorObjectKind::Spawn, 0}, "", "Player Spawn"},
    {{EditorObjectKind::Camera, 0}, "", "Camera"},
    {{EditorObjectKind::Ground, 0}, "", "Ground"},
    {{EditorObjectKind::ElevatedPlatform, 0}, "Platforms", "Platform 0"},
    {{EditorObjectKind::ElevatedPlatform, 1}, "Platforms", "Platform 1"},
    {{EditorObjectKind::ElevatedPlatform, 2}, "Platforms", "Platform 2"},
    {{EditorObjectKind::ElevatedPlatform, 3}, "Platforms", "Platform 3"},
    {{EditorObjectKind::ElevatedPlatform, 4}, "Platforms", "Platform 4"},
    {{EditorObjectKind::ElevatedPlatform, 5}, "Platforms", "Platform 5"},
    {{EditorObjectKind::Slope, 0}, "Slopes", "Slope 0"},
    {{EditorObjectKind::Slope, 1}, "Slopes", "Slope 1"},
    {{EditorObjectKind::MovingPlatform, 0}, "", "Moving Platform"},
    {{EditorObjectKind::Checkpoint, 0}, "Checkpoints", "Checkpoint 0"},
    {{EditorObjectKind::Checkpoint, 1}, "Checkpoints", "Checkpoint 1"},
    {{EditorObjectKind::Hazard, 0}, "Hazards", "Hazard 0"},
    {{EditorObjectKind::Hazard, 1}, "Hazards", "Hazard 1"},
    {{EditorObjectKind::Collectible, 0}, "Collectibles", "Collectible 0"},
    {{EditorObjectKind::Collectible, 1}, "Collectibles", "Collectible 1"},
    {{EditorObjectKind::Collectible, 2}, "Collectibles", "Collectible 2"},
    {{EditorObjectKind::Goal, 0}, "", "Goal"},
    {{EditorObjectKind::DynamicBox, 0}, "", "Dynamic Cyan Box"},
};

inline constexpr std::size_t kHierarchyEntryCount =
    sizeof(kHierarchyEntries) / sizeof(kHierarchyEntries[0]);
}
