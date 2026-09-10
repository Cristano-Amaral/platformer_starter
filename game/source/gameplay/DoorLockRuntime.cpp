#include "gameplay/DoorLockRuntime.h"

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

int FindLockedDoorTargetIndex(
    core::Vec3 playerCenter,
    float facingX,
    std::span<const world::DoorSpec> doors,
    std::span<const std::uint8_t> unlocked,
    std::span<const std::uint8_t> losBlocked)
{
    const std::size_t count = doors.size() < unlocked.size() ? doors.size() : unlocked.size();
    const float facing = static_cast<float>(FacingSign(facingX));
    int bestIndex = kNoLockedDoorIndex;
    float bestDistance = kLockedDoorMaxDistance + 1.0f;
    for (std::size_t index = 0; index < count; ++index)
    {
        if (!doors[index].requiresKey)
        {
            continue;
        }
        if (unlocked[index] != 0)
        {
            continue;
        }
        if (index < losBlocked.size() && losBlocked[index] != 0)
        {
            continue;
        }
        if (!world::DoorSpecIsValid(doors[index]))
        {
            continue;
        }

        const core::Vec3 delta{
            doors[index].center.x - playerCenter.x,
            doors[index].center.y - playerCenter.y,
            doors[index].center.z - playerCenter.z};
        const float distance = Vec3Length(delta);
        if (!(distance <= kLockedDoorMaxDistance) || distance < kLockedDoorMinDistance)
        {
            continue;
        }

        const float facingDelta = delta.x * facing;
        if (facingDelta <= 0.0f)
        {
            continue;
        }
        if (facingDelta < distance * kLockedDoorMinFacingDot)
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

bool TryUnlockLockedDoor(
    Inventory& inventory,
    DoorLockRunState& lockState,
    std::span<const world::DoorSpec> doors,
    int index)
{
    if (index < 0)
    {
        return false;
    }
    const std::size_t doorIndex = static_cast<std::size_t>(index);
    if (doorIndex >= doors.size() || doorIndex >= lockState.unlocked.size())
    {
        return false;
    }
    if (!doors[doorIndex].requiresKey)
    {
        return false;
    }
    if (lockState.unlocked[doorIndex] != 0)
    {
        return false;
    }
    if (!world::DoorSpecIsValid(doors[doorIndex]))
    {
        return false;
    }
    if (!inventory.TryRemove(kDoorUnlockItemId, kDoorUnlockQuantity))
    {
        return false;
    }
    lockState.unlocked[doorIndex] = 1;
    return true;
}
}
