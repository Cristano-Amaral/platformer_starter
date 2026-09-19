#pragma once

// Authoring hierarchy view. Built from the working copy so pending Add /
// Duplicate / Delete appear before Apply. Not a scene graph.
//
// BuildHierarchyEntries remains the complete authored-object inventory
// (picking / category order). BuildHierarchyRows is the M82 grouped
// presentation: persistent Authoring Groups first, then ungrouped objects.
// Grouped members appear only under their group.

#include "editor/AuthoringGroups.h"
#include "editor/EditorSelection.h"
#include "editor/EditorSelectionSet.h"
#include "world/LevelDefinition.h"

#include <string>
#include <string_view>
#include <vector>

namespace editor
{
inline constexpr const char* kAuthoringGroupsSectionLabel = "Authoring Groups";

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
    entries.push_back({{EditorObjectKind::Environment, 0}, ""});
    entries.push_back({{EditorObjectKind::DirectionalLight, 0}, ""});
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
    for (std::size_t index = 0; index < level.levelGoals.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::Goal, index}, "Level Goals"});
    }
    for (std::size_t index = 0; index < level.dynamicBoxes.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::DynamicBox, index}, "Dynamic Boxes"});
    }
    for (std::size_t index = 0; index < level.pressurePlates.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::PressurePlate, index}, "Pressure Plates"});
    }
    for (std::size_t index = 0; index < level.doors.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::Door, index}, "Doors"});
    }
    for (std::size_t index = 0; index < level.itemPickups.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::ItemPickup, index}, "Item Pickups"});
    }
    for (std::size_t index = 0; index < level.staticProps.size(); ++index)
    {
        entries.push_back({{EditorObjectKind::StaticProp, index}, "Static Props"});
    }
    return entries;
}

enum class HierarchyRowKind
{
    Group,
    GroupMember,
    UngroupedObject,
};

struct HierarchyRow
{
    HierarchyRowKind kind = HierarchyRowKind::UngroupedObject;
    std::size_t groupIndex = kNoAuthoringGroupIndex;
    std::size_t memberIndex = 0;
    EditorSelection selection{};
    const char* category = "";
};

inline bool SelectionBelongsToAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection selection)
{
    return FindAuthoringGroupContaining(level, selection) != kNoAuthoringGroupIndex;
}

inline std::vector<HierarchyRow> BuildHierarchyRows(const world::LevelDefinition& level)
{
    std::vector<HierarchyRow> rows;
    for (std::size_t groupIndex = 0; groupIndex < level.authoringGroups.size(); ++groupIndex)
    {
        const world::AuthoringGroup& group = level.authoringGroups[groupIndex];
        HierarchyRow groupRow{};
        groupRow.kind = HierarchyRowKind::Group;
        groupRow.groupIndex = groupIndex;
        rows.push_back(groupRow);
        for (std::size_t memberIndex = 0; memberIndex < group.members.size(); ++memberIndex)
        {
            HierarchyRow memberRow{};
            memberRow.kind = HierarchyRowKind::GroupMember;
            memberRow.groupIndex = groupIndex;
            memberRow.memberIndex = memberIndex;
            memberRow.selection = EditorSelectionFromAuthoringGroupMember(group.members[memberIndex]);
            rows.push_back(memberRow);
        }
    }

    for (const HierarchyEntry& entry : BuildHierarchyEntries(level))
    {
        if (SelectionBelongsToAuthoringGroup(level, entry.selection))
        {
            continue;
        }
        HierarchyRow objectRow{};
        objectRow.kind = HierarchyRowKind::UngroupedObject;
        objectRow.selection = entry.selection;
        objectRow.category = entry.group;
        rows.push_back(objectRow);
    }
    return rows;
}

inline bool HierarchyRowIsTopLevelEditableObject(const HierarchyRow& row)
{
    return row.kind == HierarchyRowKind::UngroupedObject;
}

inline bool HierarchyHasDuplicateTopLevelObject(
    const std::vector<HierarchyRow>& rows,
    EditorSelection selection)
{
    if (EditorSelectionIsNone(selection))
    {
        return false;
    }
    bool asMember = false;
    bool asTopLevel = false;
    for (const HierarchyRow& row : rows)
    {
        if (row.kind == HierarchyRowKind::GroupMember && row.selection == selection)
        {
            asMember = true;
        }
        if (row.kind == HierarchyRowKind::UngroupedObject && row.selection == selection)
        {
            asTopLevel = true;
        }
    }
    return asMember && asTopLevel;
}

inline const HierarchyRow* FindHierarchyGroupRow(
    const std::vector<HierarchyRow>& rows,
    std::size_t groupIndex)
{
    for (const HierarchyRow& row : rows)
    {
        if (row.kind == HierarchyRowKind::Group && row.groupIndex == groupIndex)
        {
            return &row;
        }
    }
    return nullptr;
}

inline const HierarchyRow* FindHierarchyMemberRow(
    const std::vector<HierarchyRow>& rows,
    EditorSelection selection)
{
    if (EditorSelectionIsNone(selection))
    {
        return nullptr;
    }
    for (const HierarchyRow& row : rows)
    {
        if (row.kind == HierarchyRowKind::GroupMember && row.selection == selection)
        {
            return &row;
        }
    }
    return nullptr;
}

inline const HierarchyRow* FindHierarchyUngroupedRow(
    const std::vector<HierarchyRow>& rows,
    EditorSelection selection)
{
    if (EditorSelectionIsNone(selection))
    {
        return nullptr;
    }
    for (const HierarchyRow& row : rows)
    {
        if (row.kind == HierarchyRowKind::UngroupedObject && row.selection == selection)
        {
            return &row;
        }
    }
    return nullptr;
}

inline bool HierarchyGroupRowIsSelected(
    const world::LevelDefinition& level,
    std::size_t groupIndex,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    return FindExactAuthoringGroup(level, primary, additional) == groupIndex;
}

inline bool HierarchyObjectRowIsSelected(
    EditorSelection rowSelection,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    return EditorSelectionSetContains(primary, additional, rowSelection);
}

// Ctrl on a group row is ignored: always reconstruct the complete group.
// Member and ungrouped rows reuse M78 ApplyEditorSelectionClick.
inline bool ApplyHierarchyRowClick(
    const world::LevelDefinition& workingCopy,
    const HierarchyRow& row,
    EditorSelection& primary,
    std::vector<EditorSelection>& additional,
    bool additive)
{
    if (row.kind == HierarchyRowKind::Group)
    {
        return TrySelectAuthoringGroup(workingCopy, row.groupIndex, primary, additional);
    }
    if (EditorSelectionIsNone(row.selection) || !IsValidSelection(workingCopy, row.selection))
    {
        return false;
    }
    ApplyEditorSelectionClick(primary, additional, row.selection, additive);
    return true;
}

// Transient Hierarchy presentation. Not LevelDefinition. Never Dirty.
struct HierarchyExpansionState
{
    std::vector<std::string> collapsedGroupNames{};
    std::size_t revealGroupIndex = kNoAuthoringGroupIndex;
    EditorSelection lastRevealedSelection{};
    bool renameFromHierarchy = false;
    bool renameFocusPending = false;
};

inline bool HierarchyGroupIsExpanded(
    const HierarchyExpansionState& expansion,
    std::string_view groupName)
{
    for (const std::string& collapsed : expansion.collapsedGroupNames)
    {
        if (collapsed == groupName)
        {
            return false;
        }
    }
    return true;
}

inline void SetHierarchyGroupExpanded(
    HierarchyExpansionState& expansion,
    std::string_view groupName,
    bool expanded)
{
    if (groupName.empty())
    {
        return;
    }
    std::vector<std::string> surviving;
    surviving.reserve(expansion.collapsedGroupNames.size());
    for (const std::string& collapsed : expansion.collapsedGroupNames)
    {
        if (collapsed != groupName)
        {
            surviving.push_back(collapsed);
        }
    }
    if (!expanded)
    {
        surviving.push_back(std::string(groupName));
    }
    expansion.collapsedGroupNames = std::move(surviving);
}

inline void RevealHierarchyGroup(
    HierarchyExpansionState& expansion,
    std::size_t groupIndex,
    const world::LevelDefinition& level)
{
    if (groupIndex >= level.authoringGroups.size())
    {
        return;
    }
    expansion.revealGroupIndex = groupIndex;
    SetHierarchyGroupExpanded(expansion, level.authoringGroups[groupIndex].name, true);
}

inline void ReconcileHierarchyExpansion(
    HierarchyExpansionState& expansion,
    const world::LevelDefinition& level)
{
    std::vector<std::string> surviving;
    for (const std::string& collapsed : expansion.collapsedGroupNames)
    {
        for (const world::AuthoringGroup& group : level.authoringGroups)
        {
            if (group.name == collapsed)
            {
                surviving.push_back(collapsed);
                break;
            }
        }
    }
    expansion.collapsedGroupNames = std::move(surviving);
    if (expansion.revealGroupIndex != kNoAuthoringGroupIndex
        && expansion.revealGroupIndex >= level.authoringGroups.size())
    {
        expansion.revealGroupIndex = kNoAuthoringGroupIndex;
    }
    if (!EditorSelectionIsNone(expansion.lastRevealedSelection)
        && !IsValidSelection(level, expansion.lastRevealedSelection))
    {
        expansion.lastRevealedSelection = ClearSelection();
    }
}

// Expand the group that owns PRIMARY when selection identity changes, so a
// viewport-picked grouped member is visible in Hierarchy. Does not re-expand
// after the user later collapses the same still-selected group.
inline void SyncHierarchyRevealForSelection(
    HierarchyExpansionState& expansion,
    const world::LevelDefinition& level,
    EditorSelection primary)
{
    if (expansion.lastRevealedSelection == primary)
    {
        return;
    }
    expansion.lastRevealedSelection = primary;
    const std::size_t groupIndex = FindAuthoringGroupContaining(level, primary);
    if (groupIndex == kNoAuthoringGroupIndex)
    {
        return;
    }
    SetHierarchyGroupExpanded(expansion, level.authoringGroups[groupIndex].name, true);
}
}
