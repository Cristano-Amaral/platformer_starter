#pragma once

// Milestone 81: persistent Authoring Group semantics. Testable without ImGui.
// Groups reconstruct M78 PRIMARY + additionalSelections. No GUIDs, scene graph,
// Prefabs, or group-level transform.

#include "editor/EditorSelection.h"
#include "editor/EditorSelectionSet.h"
#include "world/AuthoringGroup.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"

#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace editor
{
inline constexpr std::size_t kNoAuthoringGroupIndex =
    std::numeric_limits<std::size_t>::max();
inline constexpr std::size_t kAuthoringGroupRenameBufferSize = 64;

enum class AuthoringGroupEditStatus
{
    Success,
    InvalidSelection,
    TooFewMembers,
    MemberAlreadyGrouped,
    AtLimit,
    InvalidName,
    DuplicateName,
    InvalidGroup,
};

struct AuthoringGroupEditResult
{
    bool succeeded = false;
    AuthoringGroupEditStatus status = AuthoringGroupEditStatus::InvalidSelection;
    std::size_t groupIndex = kNoAuthoringGroupIndex;
    EditorSelection selection{};
    std::vector<EditorSelection> additionalSelections{};
};

inline bool TryAuthoringGroupMemberKindFromSelection(
    EditorObjectKind kind,
    world::AuthoringGroupMemberKind& outKind)
{
    switch (kind)
    {
    case EditorObjectKind::Spawn:
        outKind = world::AuthoringGroupMemberKind::Spawn;
        return true;
    case EditorObjectKind::Camera:
        outKind = world::AuthoringGroupMemberKind::Camera;
        return true;
    case EditorObjectKind::Ground:
        outKind = world::AuthoringGroupMemberKind::Ground;
        return true;
    case EditorObjectKind::ElevatedPlatform:
        outKind = world::AuthoringGroupMemberKind::ElevatedPlatform;
        return true;
    case EditorObjectKind::Slope:
        outKind = world::AuthoringGroupMemberKind::Slope;
        return true;
    case EditorObjectKind::MovingPlatform:
        outKind = world::AuthoringGroupMemberKind::MovingPlatform;
        return true;
    case EditorObjectKind::Checkpoint:
        outKind = world::AuthoringGroupMemberKind::Checkpoint;
        return true;
    case EditorObjectKind::Hazard:
        outKind = world::AuthoringGroupMemberKind::Hazard;
        return true;
    case EditorObjectKind::Collectible:
        outKind = world::AuthoringGroupMemberKind::Collectible;
        return true;
    case EditorObjectKind::Goal:
        outKind = world::AuthoringGroupMemberKind::Goal;
        return true;
    case EditorObjectKind::DynamicBox:
        outKind = world::AuthoringGroupMemberKind::DynamicBox;
        return true;
    case EditorObjectKind::PressurePlate:
        outKind = world::AuthoringGroupMemberKind::PressurePlate;
        return true;
    case EditorObjectKind::Door:
        outKind = world::AuthoringGroupMemberKind::Door;
        return true;
    case EditorObjectKind::ItemPickup:
        outKind = world::AuthoringGroupMemberKind::ItemPickup;
        return true;
    case EditorObjectKind::StaticProp:
        outKind = world::AuthoringGroupMemberKind::StaticProp;
        return true;
    case EditorObjectKind::PointLight:
        outKind = world::AuthoringGroupMemberKind::PointLight;
        return true;
    case EditorObjectKind::SpotLight:
        outKind = world::AuthoringGroupMemberKind::SpotLight;
        return true;
    case EditorObjectKind::Environment:
    case EditorObjectKind::DirectionalLight:
    case EditorObjectKind::Terrain:
    case EditorObjectKind::None:
        break;
    }
    return false;
}

inline EditorObjectKind EditorObjectKindFromAuthoringGroupMember(
    world::AuthoringGroupMemberKind kind)
{
    switch (kind)
    {
    case world::AuthoringGroupMemberKind::Spawn:
        return EditorObjectKind::Spawn;
    case world::AuthoringGroupMemberKind::Camera:
        return EditorObjectKind::Camera;
    case world::AuthoringGroupMemberKind::Ground:
        return EditorObjectKind::Ground;
    case world::AuthoringGroupMemberKind::ElevatedPlatform:
        return EditorObjectKind::ElevatedPlatform;
    case world::AuthoringGroupMemberKind::Slope:
        return EditorObjectKind::Slope;
    case world::AuthoringGroupMemberKind::MovingPlatform:
        return EditorObjectKind::MovingPlatform;
    case world::AuthoringGroupMemberKind::Checkpoint:
        return EditorObjectKind::Checkpoint;
    case world::AuthoringGroupMemberKind::Hazard:
        return EditorObjectKind::Hazard;
    case world::AuthoringGroupMemberKind::Collectible:
        return EditorObjectKind::Collectible;
    case world::AuthoringGroupMemberKind::Goal:
        return EditorObjectKind::Goal;
    case world::AuthoringGroupMemberKind::DynamicBox:
        return EditorObjectKind::DynamicBox;
    case world::AuthoringGroupMemberKind::PressurePlate:
        return EditorObjectKind::PressurePlate;
    case world::AuthoringGroupMemberKind::Door:
        return EditorObjectKind::Door;
    case world::AuthoringGroupMemberKind::ItemPickup:
        return EditorObjectKind::ItemPickup;
    case world::AuthoringGroupMemberKind::StaticProp:
        return EditorObjectKind::StaticProp;
    case world::AuthoringGroupMemberKind::PointLight:
        return EditorObjectKind::PointLight;
    case world::AuthoringGroupMemberKind::SpotLight:
        return EditorObjectKind::SpotLight;
    }
    return EditorObjectKind::None;
}

inline bool TryAuthoringGroupMemberFromSelection(
    EditorSelection selection,
    world::AuthoringGroupMember& outMember)
{
    world::AuthoringGroupMemberKind kind{};
    if (EditorSelectionIsNone(selection)
        || !TryAuthoringGroupMemberKindFromSelection(selection.kind, kind))
    {
        return false;
    }
    outMember.kind = kind;
    outMember.index = selection.index;
    return true;
}

inline EditorSelection EditorSelectionFromAuthoringGroupMember(
    world::AuthoringGroupMember member)
{
    return {EditorObjectKindFromAuthoringGroupMember(member.kind), member.index};
}

inline std::vector<EditorSelection> EditorSelectionsFromAuthoringGroup(
    const world::AuthoringGroup& group)
{
    std::vector<EditorSelection> members;
    members.reserve(group.members.size());
    for (const world::AuthoringGroupMember& member : group.members)
    {
        const EditorSelection selection = EditorSelectionFromAuthoringGroupMember(member);
        if (EditorSelectionIsNone(selection) || ContainsEditorSelection(members, selection))
        {
            continue;
        }
        members.push_back(selection);
    }
    return members;
}

inline bool AuthoringGroupNameExists(
    const world::LevelDefinition& level,
    std::string_view name,
    std::size_t ignoreIndex = kNoAuthoringGroupIndex)
{
    for (std::size_t index = 0; index < level.authoringGroups.size(); ++index)
    {
        if (index == ignoreIndex)
        {
            continue;
        }
        if (level.authoringGroups[index].name == name)
        {
            return true;
        }
    }
    return false;
}

std::string MakeDefaultAuthoringGroupName(const world::LevelDefinition& level);
std::string MakeCopiedAuthoringGroupName(
    const world::LevelDefinition& level,
    std::string_view originalName);

bool SelectionCanJoinAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection selection);

std::size_t FindAuthoringGroupContaining(
    const world::LevelDefinition& level,
    EditorSelection selection);

bool SelectionIntersectsAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional);

std::size_t FindExactAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional);

bool CanCreateAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional);

const char* CreateAuthoringGroupDisableReason(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional);

AuthoringGroupEditResult CreateAuthoringGroupFromSelection(
    world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional);

bool TrySelectAuthoringGroup(
    const world::LevelDefinition& workingCopy,
    std::size_t groupIndex,
    EditorSelection& outPrimary,
    std::vector<EditorSelection>& outAdditional);

AuthoringGroupEditResult RenameAuthoringGroup(
    world::LevelDefinition& workingCopy,
    std::size_t groupIndex,
    std::string_view newName);

AuthoringGroupEditResult UngroupAuthoringGroup(
    world::LevelDefinition& workingCopy,
    std::size_t groupIndex);

bool CanUngroupAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional);

const char* UngroupAuthoringGroupDisableReason(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional);

// Partial-group duplicate is refused: only an exact complete group, or a
// selection that contains no grouped members, may Duplicate Selected.
bool SelectionAllowsAuthoringGroupDuplicate(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional);

const char* AuthoringGroupDuplicateDisableReason(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional);

bool TryCreateCopiedAuthoringGroup(
    world::LevelDefinition& workingCopy,
    std::string_view originalName,
    EditorSelection copiedPrimary,
    const std::vector<EditorSelection>& copiedAdditional);

struct AuthoringGroupRenameFieldState
{
    char buffer[kAuthoringGroupRenameBufferSize]{};
    std::size_t boundGroupIndex = kNoAuthoringGroupIndex;
    bool editing = false;
};

void SyncAuthoringGroupRenameField(
    AuthoringGroupRenameFieldState& field,
    std::size_t groupIndex,
    std::string_view sourceName,
    bool widgetActive);
}
