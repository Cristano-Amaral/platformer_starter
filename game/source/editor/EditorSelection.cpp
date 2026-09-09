#include "editor/EditorSelection.h"

#include <cstdio>

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
    case EditorObjectKind::StaticProp:
        return "StaticProp";
    }
    return "None";
}

void FormatSelectionDisplayName(
    EditorSelection selection,
    char* buffer,
    std::size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    switch (selection.kind)
    {
    case EditorObjectKind::None:
        std::snprintf(buffer, bufferSize, "(none)");
        return;
    case EditorObjectKind::Spawn:
        std::snprintf(buffer, bufferSize, "Player Spawn");
        return;
    case EditorObjectKind::Camera:
        std::snprintf(buffer, bufferSize, "Camera");
        return;
    case EditorObjectKind::Ground:
        std::snprintf(buffer, bufferSize, "Ground");
        return;
    case EditorObjectKind::ElevatedPlatform:
        std::snprintf(buffer, bufferSize, "Platform %zu", selection.index);
        return;
    case EditorObjectKind::Slope:
        std::snprintf(buffer, bufferSize, "Slope %zu", selection.index);
        return;
    case EditorObjectKind::MovingPlatform:
        std::snprintf(buffer, bufferSize, "Moving Platform");
        return;
    case EditorObjectKind::Checkpoint:
        std::snprintf(buffer, bufferSize, "Checkpoint %zu", selection.index);
        return;
    case EditorObjectKind::Hazard:
        std::snprintf(buffer, bufferSize, "Hazard %zu", selection.index);
        return;
    case EditorObjectKind::Collectible:
        std::snprintf(buffer, bufferSize, "Collectible %zu", selection.index);
        return;
    case EditorObjectKind::Goal:
        std::snprintf(buffer, bufferSize, "Goal");
        return;
    case EditorObjectKind::DynamicBox:
        std::snprintf(buffer, bufferSize, "Dynamic Box %zu", selection.index);
        return;
    case EditorObjectKind::StaticProp:
        std::snprintf(buffer, bufferSize, "Static Prop %zu", selection.index);
        return;
    }
    std::snprintf(buffer, bufferSize, "(none)");
}

const char* SelectionDisplayName(EditorSelection selection)
{
    static char buffer[64];
    FormatSelectionDisplayName(selection, buffer, sizeof(buffer));
    return buffer;
}

bool IsValidSelection(const world::LevelDefinition& level, EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::None:
    case EditorObjectKind::Spawn:
    case EditorObjectKind::Camera:
    case EditorObjectKind::Ground:
    case EditorObjectKind::MovingPlatform:
    case EditorObjectKind::Goal:
        return selection.index == 0;
    case EditorObjectKind::DynamicBox:
        return selection.index < level.dynamicBoxes.size();
    case EditorObjectKind::StaticProp:
        return selection.index < level.staticProps.size();
    case EditorObjectKind::ElevatedPlatform:
        return selection.index < level.elevatedPlatforms.size();
    case EditorObjectKind::Slope:
        return selection.index < level.slopes.size();
    case EditorObjectKind::Checkpoint:
        return selection.index < level.checkpoints.size();
    case EditorObjectKind::Hazard:
        return selection.index < level.hazards.size();
    case EditorObjectKind::Collectible:
        return selection.index < level.collectibles.size();
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
}
