#include "editor/EditorSelection.h"

namespace editor
{
const char* EditorObjectKindName(EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::None:
        return "None";
    case EditorObjectKind::Spawn:
        return "Spawn";
    case EditorObjectKind::Camera:
        return "Camera";
    case EditorObjectKind::Ground:
        return "Ground";
    case EditorObjectKind::ElevatedPlatform:
        return "ElevatedPlatform";
    case EditorObjectKind::Slope:
        return "Slope";
    case EditorObjectKind::MovingPlatform:
        return "MovingPlatform";
    case EditorObjectKind::Checkpoint:
        return "Checkpoint";
    case EditorObjectKind::Hazard:
        return "Hazard";
    case EditorObjectKind::Collectible:
        return "Collectible";
    case EditorObjectKind::Goal:
        return "Goal";
    case EditorObjectKind::DynamicBox:
        return "DynamicBox";
    }
    return "None";
}

const char* SelectionDisplayName(EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::None:
        return "(none)";
    case EditorObjectKind::Spawn:
        return "Player Spawn";
    case EditorObjectKind::Camera:
        return "Camera";
    case EditorObjectKind::Ground:
        return "Ground";
    case EditorObjectKind::ElevatedPlatform:
        switch (selection.index)
        {
        case 0:
            return "Platform 0";
        case 1:
            return "Platform 1";
        case 2:
            return "Platform 2";
        case 3:
            return "Platform 3";
        case 4:
            return "Platform 4";
        case 5:
            return "Platform 5";
        default:
            return "Platform";
        }
    case EditorObjectKind::Slope:
        return selection.index == 0 ? "Slope 0" : (selection.index == 1 ? "Slope 1" : "Slope");
    case EditorObjectKind::MovingPlatform:
        return "Moving Platform";
    case EditorObjectKind::Checkpoint:
        return selection.index == 0 ? "Checkpoint 0"
                                    : (selection.index == 1 ? "Checkpoint 1" : "Checkpoint");
    case EditorObjectKind::Hazard:
        return selection.index == 0 ? "Hazard 0" : (selection.index == 1 ? "Hazard 1" : "Hazard");
    case EditorObjectKind::Collectible:
        switch (selection.index)
        {
        case 0:
            return "Collectible 0";
        case 1:
            return "Collectible 1";
        case 2:
            return "Collectible 2";
        default:
            return "Collectible";
        }
    case EditorObjectKind::Goal:
        return "Goal";
    case EditorObjectKind::DynamicBox:
        return "Dynamic Cyan Box";
    }
    return "(none)";
}

bool IsValidSelection(const world::LevelDefinition&, EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::None:
    case EditorObjectKind::Spawn:
    case EditorObjectKind::Camera:
    case EditorObjectKind::Ground:
    case EditorObjectKind::MovingPlatform:
    case EditorObjectKind::Goal:
    case EditorObjectKind::DynamicBox:
        return selection.index == 0;
    case EditorObjectKind::ElevatedPlatform:
        return selection.index < world::kLevel01ElevatedPlatformCount;
    case EditorObjectKind::Slope:
        return selection.index < world::kLevel01SlopeCount;
    case EditorObjectKind::Checkpoint:
        return selection.index < static_cast<std::size_t>(world::kCheckpointCount);
    case EditorObjectKind::Hazard:
        return selection.index < static_cast<std::size_t>(world::kHazardCount);
    case EditorObjectKind::Collectible:
        return selection.index < static_cast<std::size_t>(world::kCollectibleCount);
    }
    return false;
}

bool IsEditableSelection(EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::Spawn:
    case EditorObjectKind::Camera:
    case EditorObjectKind::Ground:
    case EditorObjectKind::ElevatedPlatform:
        return true;
    default:
        return false;
    }
}
}
