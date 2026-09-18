#pragma once

// Narrow Inspector Item ID buffer sync/commit. Extracted from the Development
// ImGui InputText path so focus-loss commit can be regression-tested without
// ImGui. Not a property-editing framework, ItemDefinition system, or catalog.

#include "editor/EditorSelection.h"
#include "gameplay/Inventory.h"

#include <cstdio>
#include <string>
#include <string_view>

namespace editor
{
inline constexpr std::size_t kItemIdInspectorBufferSize = gameplay::kMaxItemIdLength + 1;

struct ItemIdInspectorFieldState
{
    char buffer[kItemIdInspectorBufferSize]{};
    EditorSelection boundSelection{};
    bool editing = false;
};

enum class ItemIdInspectorCommitResult
{
    Unchanged,
    Accepted,
    Rejected,
};

inline void CopyItemIdInspectorBuffer(char* buffer, std::string_view itemId)
{
    const int copied = std::snprintf(
        buffer,
        kItemIdInspectorBufferSize,
        "%.*s",
        static_cast<int>(itemId.size()),
        itemId.data());
    if (copied < 0)
    {
        buffer[0] = '\0';
    }
}

// Reload from workingCopy when PRIMARY identity changes or the widget is not
// the active editor. While the same PRIMARY Item Pickup is being edited, keep
// the buffer so typed text is not clobbered by the last committed id.
inline void SyncItemIdInspectorField(
    ItemIdInspectorFieldState& field,
    EditorSelection primary,
    std::string_view sourceItemId,
    bool widgetActive)
{
    const bool samePrimary =
        primary.kind == EditorObjectKind::ItemPickup && field.boundSelection == primary;
    if (!samePrimary || !field.editing)
    {
        CopyItemIdInspectorBuffer(field.buffer, sourceItemId);
        field.boundSelection = primary;
    }
    field.editing = widgetActive && primary.kind == EditorObjectKind::ItemPickup;
}

// Live InputText path: write workingCopy only when the buffer is a valid M54
// itemId. Invalid intermediates leave both dest and buffer unchanged so the
// user can keep typing.
inline ItemIdInspectorCommitResult TryAcceptItemIdInspectorField(
    std::string& destItemId,
    const ItemIdInspectorFieldState& field)
{
    if (!gameplay::IsValidItemId(std::string_view{field.buffer}))
    {
        return ItemIdInspectorCommitResult::Rejected;
    }
    if (destItemId == field.buffer)
    {
        return ItemIdInspectorCommitResult::Unchanged;
    }
    destItemId = field.buffer;
    return ItemIdInspectorCommitResult::Accepted;
}

// Focus-loss path: accept a valid id, otherwise restore the buffer from the
// last committed workingCopy value. Duplicate itemIds remain legal.
inline ItemIdInspectorCommitResult CommitItemIdInspectorFieldOnFocusLoss(
    std::string& destItemId,
    ItemIdInspectorFieldState& field)
{
    const ItemIdInspectorCommitResult result =
        TryAcceptItemIdInspectorField(destItemId, field);
    if (result == ItemIdInspectorCommitResult::Rejected)
    {
        CopyItemIdInspectorBuffer(field.buffer, destItemId);
    }
    return result;
}
}
