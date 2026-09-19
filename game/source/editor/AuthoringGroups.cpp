#include "editor/AuthoringGroups.h"

#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

namespace editor
{
namespace
{
AuthoringGroupEditResult Fail(AuthoringGroupEditStatus status)
{
    AuthoringGroupEditResult result{};
    result.status = status;
    return result;
}

AuthoringGroupEditResult Ok(
    std::size_t groupIndex,
    EditorSelection primary,
    std::vector<EditorSelection> additional)
{
    AuthoringGroupEditResult result{};
    result.succeeded = true;
    result.status = AuthoringGroupEditStatus::Success;
    result.groupIndex = groupIndex;
    result.selection = primary;
    result.additionalSelections = std::move(additional);
    SanitizeEditorSelectionSet(result.selection, result.additionalSelections);
    return result;
}

bool AuthoringGroupHasMember(
    const world::AuthoringGroup& group,
    world::AuthoringGroupMember member)
{
    for (const world::AuthoringGroupMember& existing : group.members)
    {
        if (existing == member)
        {
            return true;
        }
    }
    return false;
}

bool SelectionSetMatchesGroupMembers(
    const std::vector<EditorSelection>& members,
    const world::AuthoringGroup& group)
{
    const std::vector<EditorSelection> groupMembers = EditorSelectionsFromAuthoringGroup(group);
    if (members.size() != groupMembers.size() || members.size() < 2)
    {
        return false;
    }
    for (const EditorSelection& member : members)
    {
        if (!ContainsEditorSelection(groupMembers, member))
        {
            return false;
        }
    }
    for (const EditorSelection& member : groupMembers)
    {
        if (!ContainsEditorSelection(members, member))
        {
            return false;
        }
    }
    return true;
}
}

std::string MakeDefaultAuthoringGroupName(const world::LevelDefinition& level)
{
    for (int number = 1; number < 10000; ++number)
    {
        char buffer[32]{};
        if (number < 100)
        {
            std::snprintf(buffer, sizeof(buffer), "Group_%02d", number);
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "Group_%d", number);
        }
        if (!AuthoringGroupNameExists(level, buffer))
        {
            return buffer;
        }
    }
    return {};
}

std::string MakeCopiedAuthoringGroupName(
    const world::LevelDefinition& level,
    std::string_view originalName)
{
    std::string base = std::string(originalName) + "_Copy";
    if (world::IsValidAuthoringGroupName(base) && !AuthoringGroupNameExists(level, base))
    {
        return base;
    }
    for (int number = 2; number < 10000; ++number)
    {
        const std::string candidate = base + "_" + std::to_string(number);
        if (world::IsValidAuthoringGroupName(candidate)
            && !AuthoringGroupNameExists(level, candidate))
        {
            return candidate;
        }
    }
    return {};
}

bool SelectionCanJoinAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection selection)
{
    world::AuthoringGroupMember member{};
    return TryAuthoringGroupMemberFromSelection(selection, member)
        && IsValidSelection(level, selection)
        && world::AuthoringGroupMemberIsValid(level, member);
}

std::size_t FindAuthoringGroupContaining(
    const world::LevelDefinition& level,
    EditorSelection selection)
{
    world::AuthoringGroupMember member{};
    if (!TryAuthoringGroupMemberFromSelection(selection, member))
    {
        return kNoAuthoringGroupIndex;
    }
    for (std::size_t index = 0; index < level.authoringGroups.size(); ++index)
    {
        if (AuthoringGroupHasMember(level.authoringGroups[index], member))
        {
            return index;
        }
    }
    return kNoAuthoringGroupIndex;
}

bool SelectionIntersectsAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    const std::vector<EditorSelection> members = EditorSelectionSetMembers(primary, additional);
    for (const EditorSelection& member : members)
    {
        if (FindAuthoringGroupContaining(level, member) != kNoAuthoringGroupIndex)
        {
            return true;
        }
    }
    return false;
}

std::size_t FindExactAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    const std::vector<EditorSelection> members = EditorSelectionSetMembers(primary, additional);
    for (std::size_t index = 0; index < level.authoringGroups.size(); ++index)
    {
        if (SelectionSetMatchesGroupMembers(members, level.authoringGroups[index]))
        {
            return index;
        }
    }
    return kNoAuthoringGroupIndex;
}

bool CanCreateAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    return CreateAuthoringGroupDisableReason(level, primary, additional) == nullptr;
}

const char* CreateAuthoringGroupDisableReason(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    const std::vector<EditorSelection> members = EditorSelectionSetMembers(primary, additional);
    if (members.size() < 2)
    {
        return "Group Selected requires two or more ungrouped authored objects.";
    }
    if (world::CountLevelV1RecordLines(level) >= static_cast<int>(world::kMaxLevelLines))
    {
        return "Level file record limit reached.";
    }
    for (const EditorSelection& member : members)
    {
        if (!SelectionCanJoinAuthoringGroup(level, member))
        {
            return "Group Selected blocked: selection contains an unsupported object.";
        }
        if (FindAuthoringGroupContaining(level, member) != kNoAuthoringGroupIndex)
        {
            return "Group Selected blocked: a selected object already belongs to a group.";
        }
    }
    return nullptr;
}

AuthoringGroupEditResult CreateAuthoringGroupFromSelection(
    world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    const char* reason = CreateAuthoringGroupDisableReason(workingCopy, primary, additional);
    if (reason != nullptr)
    {
        if (std::string_view(reason)
            == "Group Selected blocked: a selected object already belongs to a group.")
        {
            return Fail(AuthoringGroupEditStatus::MemberAlreadyGrouped);
        }
        if (std::string_view(reason) == "Level file record limit reached.")
        {
            return Fail(AuthoringGroupEditStatus::AtLimit);
        }
        const std::vector<EditorSelection> members =
            EditorSelectionSetMembers(primary, additional);
        if (members.size() < 2)
        {
            return Fail(AuthoringGroupEditStatus::TooFewMembers);
        }
        return Fail(AuthoringGroupEditStatus::InvalidSelection);
    }

    const std::vector<EditorSelection> members = EditorSelectionSetMembers(primary, additional);
    world::AuthoringGroup group{};
    group.name = MakeDefaultAuthoringGroupName(workingCopy);
    if (group.name.empty())
    {
        return Fail(AuthoringGroupEditStatus::InvalidName);
    }
    group.members.reserve(members.size());
    for (const EditorSelection& member : members)
    {
        world::AuthoringGroupMember authored{};
        if (!TryAuthoringGroupMemberFromSelection(member, authored))
        {
            return Fail(AuthoringGroupEditStatus::InvalidSelection);
        }
        group.members.push_back(authored);
    }

    workingCopy.authoringGroups.push_back(std::move(group));
    std::vector<EditorSelection> remaining = members;
    const EditorSelection keptPrimary = remaining.front();
    remaining.erase(remaining.begin());
    return Ok(workingCopy.authoringGroups.size() - 1, keptPrimary, std::move(remaining));
}

bool TrySelectAuthoringGroup(
    const world::LevelDefinition& workingCopy,
    std::size_t groupIndex,
    EditorSelection& outPrimary,
    std::vector<EditorSelection>& outAdditional)
{
    if (groupIndex >= workingCopy.authoringGroups.size())
    {
        return false;
    }
    const std::vector<EditorSelection> members =
        EditorSelectionsFromAuthoringGroup(workingCopy.authoringGroups[groupIndex]);
    if (members.size() < 2)
    {
        return false;
    }
    for (const EditorSelection& member : members)
    {
        if (!IsValidSelection(workingCopy, member))
        {
            return false;
        }
    }
    outPrimary = members.front();
    outAdditional.assign(members.begin() + 1, members.end());
    SanitizeEditorSelectionSet(outPrimary, outAdditional);
    return !EditorSelectionIsNone(outPrimary) && !outAdditional.empty();
}

AuthoringGroupEditResult RenameAuthoringGroup(
    world::LevelDefinition& workingCopy,
    std::size_t groupIndex,
    std::string_view newName)
{
    if (groupIndex >= workingCopy.authoringGroups.size())
    {
        return Fail(AuthoringGroupEditStatus::InvalidGroup);
    }
    if (!world::IsValidAuthoringGroupName(newName))
    {
        return Fail(AuthoringGroupEditStatus::InvalidName);
    }
    if (AuthoringGroupNameExists(workingCopy, newName, groupIndex))
    {
        return Fail(AuthoringGroupEditStatus::DuplicateName);
    }

    std::vector<EditorSelection> additional;
    EditorSelection primary{};
    if (!TrySelectAuthoringGroup(workingCopy, groupIndex, primary, additional))
    {
        return Fail(AuthoringGroupEditStatus::InvalidGroup);
    }
    if (workingCopy.authoringGroups[groupIndex].name == newName)
    {
        return Ok(groupIndex, primary, std::move(additional));
    }
    workingCopy.authoringGroups[groupIndex].name = std::string(newName);
    return Ok(groupIndex, primary, std::move(additional));
}

AuthoringGroupEditResult UngroupAuthoringGroup(
    world::LevelDefinition& workingCopy,
    std::size_t groupIndex)
{
    EditorSelection primary{};
    std::vector<EditorSelection> additional;
    if (!TrySelectAuthoringGroup(workingCopy, groupIndex, primary, additional))
    {
        return Fail(AuthoringGroupEditStatus::InvalidGroup);
    }
    workingCopy.authoringGroups.erase(
        workingCopy.authoringGroups.begin() + static_cast<std::ptrdiff_t>(groupIndex));
    return Ok(kNoAuthoringGroupIndex, primary, std::move(additional));
}

bool CanUngroupAuthoringGroup(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    return FindExactAuthoringGroup(level, primary, additional) != kNoAuthoringGroupIndex;
}

const char* UngroupAuthoringGroupDisableReason(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    if (CanUngroupAuthoringGroup(level, primary, additional))
    {
        return nullptr;
    }
    return "Ungroup requires a selected Authoring Group.";
}

bool SelectionAllowsAuthoringGroupDuplicate(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    if (!SelectionIntersectsAuthoringGroup(level, primary, additional))
    {
        return true;
    }
    return FindExactAuthoringGroup(level, primary, additional) != kNoAuthoringGroupIndex;
}

const char* AuthoringGroupDuplicateDisableReason(
    const world::LevelDefinition& level,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    if (SelectionAllowsAuthoringGroupDuplicate(level, primary, additional))
    {
        return nullptr;
    }
    return "Duplicate blocked: select the complete Authoring Group.";
}

bool TryCreateCopiedAuthoringGroup(
    world::LevelDefinition& workingCopy,
    std::string_view originalName,
    EditorSelection copiedPrimary,
    const std::vector<EditorSelection>& copiedAdditional)
{
    const std::vector<EditorSelection> members =
        EditorSelectionSetMembers(copiedPrimary, copiedAdditional);
    if (members.size() < 2)
    {
        return false;
    }

    world::AuthoringGroup group{};
    group.name = MakeCopiedAuthoringGroupName(workingCopy, originalName);
    if (group.name.empty())
    {
        return false;
    }
    group.members.reserve(members.size());
    for (const EditorSelection& member : members)
    {
        world::AuthoringGroupMember authored{};
        if (!TryAuthoringGroupMemberFromSelection(member, authored)
            || !world::AuthoringGroupMemberIsValid(workingCopy, authored)
            || FindAuthoringGroupContaining(workingCopy, member) != kNoAuthoringGroupIndex)
        {
            return false;
        }
        group.members.push_back(authored);
    }
    workingCopy.authoringGroups.push_back(std::move(group));
    return true;
}

void SyncAuthoringGroupRenameField(
    AuthoringGroupRenameFieldState& field,
    std::size_t groupIndex,
    std::string_view sourceName,
    bool widgetActive)
{
    const bool sameGroup = field.boundGroupIndex == groupIndex && groupIndex != kNoAuthoringGroupIndex;
    if (!sameGroup || !field.editing)
    {
        const int copied = std::snprintf(
            field.buffer,
            kAuthoringGroupRenameBufferSize,
            "%.*s",
            static_cast<int>(sourceName.size()),
            sourceName.data());
        if (copied < 0)
        {
            field.buffer[0] = '\0';
        }
        field.boundGroupIndex = groupIndex;
    }
    field.editing = widgetActive && groupIndex != kNoAuthoringGroupIndex;
}
}
