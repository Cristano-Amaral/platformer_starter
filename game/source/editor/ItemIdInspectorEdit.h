#pragma once

// Narrow Inspector Item identity buffer sync/commit. Extracted from the
// Development Item Pickup inspector so focus-loss commit can be regression-
// tested without ImGui. Accepts items/<name> only. Not a catalog.

#include "editor/EditorSelection.h"
#include "gameplay/ItemIdentity.h"

#include <cstdio>
#include <string>
#include <string_view>

namespace editor
{
inline constexpr std::size_t kItemIdInspectorBufferSize =
    gameplay::kMaxGameplayIdentityLength + 1;

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
// the buffer so typed text is not clobbered by the last committed identity.
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

inline ItemIdInspectorCommitResult TryAcceptItemIdInspectorField(
    std::string& destItemId,
    const ItemIdInspectorFieldState& field)
{
    if (!gameplay::IsValidInventoryItemIdentity(std::string_view{field.buffer}))
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
