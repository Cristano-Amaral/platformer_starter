#pragma once

// Milestone 78: transient multi-selection around the existing primary
// EditorSelection. Type + index remains identity. No GUIDs, no persistence.

#include "editor/EditorSelection.h"

#include <cstddef>
#include <vector>

namespace editor
{
inline bool EditorSelectionIsNone(EditorSelection selection)
{
    return selection.kind == EditorObjectKind::None;
}

inline bool IsExclusiveAuthoringSelection(EditorSelection selection)
{
    return IsLightingAuthoringSelection(selection)
        || selection.kind == EditorObjectKind::Terrain;
}

inline bool EditorSelectionSetContainsExclusiveAuthoring(
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    if (IsExclusiveAuthoringSelection(primary))
    {
        return true;
    }
    for (const EditorSelection& entry : additional)
    {
        if (IsExclusiveAuthoringSelection(entry))
        {
            return true;
        }
    }
    return false;
}

inline bool EditorSelectionSetContainsLightingAuthoring(
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    if (IsLightingAuthoringSelection(primary))
    {
        return true;
    }
    for (const EditorSelection& entry : additional)
    {
        if (IsLightingAuthoringSelection(entry))
        {
            return true;
        }
    }
    return false;
}

inline bool EditorSelectionSetIsMulti(
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    return !EditorSelectionIsNone(primary) && !additional.empty();
}

inline bool ContainsEditorSelection(
    const std::vector<EditorSelection>& items,
    EditorSelection item)
{
    if (EditorSelectionIsNone(item))
    {
        return false;
    }
    for (const EditorSelection& entry : items)
    {
        if (entry == item)
        {
            return true;
        }
    }
    return false;
}

inline bool EditorSelectionSetContains(
    EditorSelection primary,
    const std::vector<EditorSelection>& additional,
    EditorSelection item)
{
    if (EditorSelectionIsNone(item))
    {
        return false;
    }
    return primary == item || ContainsEditorSelection(additional, item);
}

inline void RemoveEditorSelection(
    std::vector<EditorSelection>& items,
    EditorSelection item)
{
    std::size_t write = 0;
    for (std::size_t read = 0; read < items.size(); ++read)
    {
        if (items[read] != item)
        {
            items[write] = items[read];
            ++write;
        }
    }
    items.resize(write);
}

inline void ClearEditorSelectionSet(
    EditorSelection& primary,
    std::vector<EditorSelection>& additional)
{
    primary = ClearSelection();
    additional.clear();
}

inline std::vector<EditorSelection> EditorSelectionSetMembers(
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    std::vector<EditorSelection> members;
    if (!EditorSelectionIsNone(primary))
    {
        members.push_back(primary);
    }
    for (const EditorSelection& entry : additional)
    {
        if (EditorSelectionIsNone(entry) || entry == primary
            || ContainsEditorSelection(members, entry))
        {
            continue;
        }
        members.push_back(entry);
    }
    return members;
}

inline void SanitizeEditorSelectionSet(
    EditorSelection& primary,
    std::vector<EditorSelection>& additional)
{
    std::vector<EditorSelection> cleaned;
    for (const EditorSelection& entry : additional)
    {
        if (EditorSelectionIsNone(entry) || entry == primary
            || ContainsEditorSelection(cleaned, entry))
        {
            continue;
        }
        cleaned.push_back(entry);
    }
    additional = std::move(cleaned);
}

inline void ReplaceEditorSelection(
    EditorSelection& primary,
    std::vector<EditorSelection>& additional,
    EditorSelection item)
{
    additional.clear();
    primary = item;
}

// Most recently directly selected/added object becomes primary. The previous
// primary is appended to additional so remaining-primary fallback is the last
// additional member (most recently added among remaining).
inline void PromoteEditorSelectionAsPrimary(
    EditorSelection& primary,
    std::vector<EditorSelection>& additional,
    EditorSelection item)
{
    if (EditorSelectionIsNone(item))
    {
        return;
    }
    RemoveEditorSelection(additional, item);
    if (!EditorSelectionIsNone(primary) && primary != item)
    {
        if (!ContainsEditorSelection(additional, primary))
        {
            additional.push_back(primary);
        }
    }
    primary = item;
}

inline void RemoveFromEditorSelectionSet(
    EditorSelection& primary,
    std::vector<EditorSelection>& additional,
    EditorSelection item)
{
    if (EditorSelectionIsNone(item))
    {
        return;
    }
    if (primary == item)
    {
        if (additional.empty())
        {
            primary = ClearSelection();
            return;
        }
        primary = additional.back();
        additional.pop_back();
        RemoveEditorSelection(additional, primary);
        return;
    }
    RemoveEditorSelection(additional, item);
}

inline void ToggleOrAddEditorSelection(
    EditorSelection& primary,
    std::vector<EditorSelection>& additional,
    EditorSelection item)
{
    if (EditorSelectionIsNone(item))
    {
        return;
    }
    if (EditorSelectionSetContains(primary, additional, item))
    {
        RemoveFromEditorSelectionSet(primary, additional, item);
        return;
    }
    PromoteEditorSelectionAsPrimary(primary, additional, item);
}

// additive false: ordinary click replaces (None clears).
// additive true: Ctrl+click toggles/adds; None is a no-op (does not clear).
inline void ApplyEditorSelectionClick(
    EditorSelection& primary,
    std::vector<EditorSelection>& additional,
    EditorSelection clicked,
    bool additive)
{
    if (IsExclusiveAuthoringSelection(clicked)
        || EditorSelectionSetContainsExclusiveAuthoring(primary, additional))
    {
        if (additive && primary == clicked && additional.empty()
            && IsExclusiveAuthoringSelection(clicked))
        {
            ClearEditorSelectionSet(primary, additional);
            return;
        }
        ReplaceEditorSelection(primary, additional, clicked);
        return;
    }
    if (!additive)
    {
        ReplaceEditorSelection(primary, additional, clicked);
        return;
    }
    ToggleOrAddEditorSelection(primary, additional, clicked);
}

inline void ReconcileEditorSelectionSet(
    const world::LevelDefinition& workingCopy,
    EditorSelection& primary,
    std::vector<EditorSelection>& additional)
{
    std::vector<EditorSelection> validAdditional;
    for (const EditorSelection& entry : additional)
    {
        if (EditorSelectionIsNone(entry) || !IsValidSelection(workingCopy, entry)
            || entry == primary || ContainsEditorSelection(validAdditional, entry))
        {
            continue;
        }
        validAdditional.push_back(entry);
    }
    additional = std::move(validAdditional);

    if (!EditorSelectionIsNone(primary) && IsValidSelection(workingCopy, primary))
    {
        RemoveEditorSelection(additional, primary);
        return;
    }

    if (!additional.empty())
    {
        primary = additional.back();
        additional.pop_back();
        RemoveEditorSelection(additional, primary);
        return;
    }
    primary = ClearSelection();
}
}
