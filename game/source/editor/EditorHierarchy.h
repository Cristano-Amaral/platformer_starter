#pragma once

// Authoring hierarchy view. Built from the working copy so pending Add /
// Duplicate / Delete appear before Apply. Not a scene graph.

#include "editor/EditorSelection.h"
#include "world/LevelDefinition.h"

#include <vector>

namespace editor
{
struct HierarchyEntry
{
    EditorSelection selection{};
    const char* group = "";
};

inline std::vector<HierarchyEntry> BuildHierarchyEntries(const world::LevelDefinition& level)
{
    std::vector<HierarchyEntry> entries;
    entries.push_back({{EditorObjectKind::Spawn, 0}, ""});
    entries.push_back({{EditorObjectKind::Camera, 0}, ""});
    entries.push_back({{EditorObjectKind::Ground, 0}, ""});
    for (std::size_t index = 0; index < level.elevatedPlatforms.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::ElevatedPlatform, index}, "Platforms"});
    }
    for (std::size_t index = 0; index < level.slopes.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::Slope, index}, "Slopes"});
    }
    entries.push_back({{EditorObjectKind::MovingPlatform, 0}, ""});
    for (std::size_t index = 0; index < level.checkpoints.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::Checkpoint, index}, "Checkpoints"});
    }
    for (std::size_t index = 0; index < level.hazards.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::Hazard, index}, "Hazards"});
    }
    for (std::size_t index = 0; index < level.collectibles.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::Collectible, index}, "Collectibles"});
    }
    entries.push_back({{EditorObjectKind::Goal, 0}, ""});
    for (std::size_t index = 0; index < level.dynamicBoxes.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::DynamicBox, index}, "Dynamic Boxes"});
    }
    for (std::size_t index = 0; index < level.pressurePlates.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::PressurePlate, index}, "Pressure Plates"});
    }
    for (std::size_t index = 0; index < level.staticProps.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::StaticProp, index}, "Static Props"});
    }
    return entries;
}
}
