#pragma once

// Application-owned per-run Item Pickup availability. Not authored, not
// physics-owned, not serialized. Collection uses gameplay::Inventory::TryAdd.

#include "core/Vec3.h"
#include "gameplay/GameplayDefinition.h"
#include "gameplay/Inventory.h"
#include "world/ItemPickup.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace gameplay
{
inline constexpr int kNoItemPickupIndex = -1;

// Same world-scale targeting as M51 Dynamic Box grab.
inline constexpr float kItemPickupMaxDistance = 2.5f;
inline constexpr float kItemPickupMinFacingDot = 0.35f;
inline constexpr float kItemPickupMinDistance = 0.05f;
inline constexpr float kItemPickupLosBlockFraction = 0.98f;

struct ItemPickupRunState
{
    std::vector<std::uint8_t> collected{};
};

inline ItemPickupRunState MakeClearedItemPickupRunState(std::size_t count)
{
    ItemPickupRunState state{};
    state.collected.assign(count, 0);
    return state;
}

inline bool ItemPickupIsAvailable(const ItemPickupRunState& state, std::size_t index)
{
    return index < state.collected.size() && state.collected[index] == 0;
}

// Nearest in-range in-front available pickup. Lower session index wins when
// distances are within 1e-5. losBlocked[i] != 0 skips that index.
int FindItemPickupTargetIndex(
    core::Vec3 playerCenter,
    float facingX,
    std::span<const world::ItemPickupSpec> pickups,
    std::span<const std::uint8_t> collected,
    std::span<const std::uint8_t> losBlocked);

// TryAdd first. Only on success mark collected. No partial quantity.
// Missing/wrong-category/malformed ItemDefinition leaves the pickup available.
bool TryCollectItemPickup(
    Inventory& inventory,
    ItemPickupRunState& runState,
    std::span<const world::ItemPickupSpec> pickups,
    int index,
    const GameplayDefinitionRegistry& registry);

// ItemDefinition World Model is presentation authority. Authored pickup
// modelIdentity is a narrow legacy visual fallback only.
inline std::string_view ResolveItemPickupWorldModel(
    const world::ItemPickupSpec& spec,
    const GameplayDefinitionRegistry* registry)
{
    if (registry != nullptr)
    {
        const GameplayReferenceResolution resolution = registry->Resolve(
            GameplayDefinitionReference{spec.itemId},
            GameplayDefinitionCategory::Item);
        if (resolution.status == GameplayReferenceStatus::Resolved && resolution.definition != nullptr
            && !resolution.definition->item.worldModelIdentity.empty())
        {
            return resolution.definition->item.worldModelIdentity;
        }
    }
    return spec.modelIdentity;
}

inline bool ItemPickupHasResolvedVisualModel(
    const world::ItemPickupSpec& spec,
    const GameplayDefinitionRegistry* registry)
{
    return !ResolveItemPickupWorldModel(spec, registry).empty();
}

// One visual composition for workingCopy preview, Translate/Rotate/Scale
// ghosts, editor picking/bounds, and post-Apply DrawProp (idle excluded):
// identity = ItemDefinition World Model, else leftover authored modelIdentity;
// origin = position + visualOffset; rotation = visualRotationDegrees;
// scale = visualScale. Empty identity keeps the primitive fallback.
inline world::StaticPropSpec ItemPickupResolvedVisualProp(
    const world::ItemPickupSpec& spec,
    const GameplayDefinitionRegistry* registry)
{
    world::ItemPickupSpec presented = spec;
    presented.modelIdentity.assign(ResolveItemPickupWorldModel(spec, registry));
    return world::ItemPickupVisualProp(presented);
}
}
