#include "gameplay/ItemPickupRuntime.h"

#include <cmath>

namespace gameplay
{
namespace
{
int FacingSign(float facingX)
{
    return facingX < 0.0f ? -1 : 1;
}

float Vec3Length(core::Vec3 value)
{
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}
}

int FindItemPickupTargetIndex(
    core::Vec3 playerCenter,
    float facingX,
    std::span<const world::ItemPickupSpec> pickups,
    std::span<const std::uint8_t> collected,
    std::span<const std::uint8_t> losBlocked)
{
    const std::size_t count = pickups.size() < collected.size() ? pickups.size() : collected.size();
    const float facing = static_cast<float>(FacingSign(facingX));
    int bestIndex = kNoItemPickupIndex;
    float bestDistance = kItemPickupMaxDistance + 1.0f;
    for (std::size_t index = 0; index < count; ++index)
    {
        if (collected[index] != 0)
        {
            continue;
        }
        if (index < losBlocked.size() && losBlocked[index] != 0)
        {
            continue;
        }
        if (!world::ItemPickupSpecIsValid(pickups[index]))
        {
            continue;
        }

        const core::Vec3 delta{
            pickups[index].position.x - playerCenter.x,
            pickups[index].position.y - playerCenter.y,
            pickups[index].position.z - playerCenter.z};
        const float distance = Vec3Length(delta);
        if (!(distance <= kItemPickupMaxDistance) || distance < kItemPickupMinDistance)
        {
            continue;
        }

        const float facingDelta = delta.x * facing;
        if (facingDelta <= 0.0f)
        {
            continue;
        }
        if (facingDelta < distance * kItemPickupMinFacingDot)
        {
            continue;
        }

        if (distance + 1.0e-5f < bestDistance)
        {
            bestDistance = distance;
            bestIndex = static_cast<int>(index);
        }
    }
    return bestIndex;
}

bool TryCollectItemPickup(
    Inventory& inventory,
    ItemPickupRunState& runState,
    std::span<const world::ItemPickupSpec> pickups,
    int index)
{
    if (index < 0)
    {
        return false;
    }
    const std::size_t pickupIndex = static_cast<std::size_t>(index);
    if (pickupIndex >= pickups.size() || pickupIndex >= runState.collected.size())
    {
        return false;
    }
    if (runState.collected[pickupIndex] != 0)
    {
        return false;
    }
    const world::ItemPickupSpec& spec = pickups[pickupIndex];
    if (!world::ItemPickupSpecIsValid(spec))
    {
        return false;
    }
    if (!inventory.TryAdd(spec.itemId, spec.quantity))
    {
        return false;
    }
    runState.collected[pickupIndex] = 1;
    return true;
}
}
