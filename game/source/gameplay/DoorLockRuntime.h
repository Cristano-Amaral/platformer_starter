#pragma once

// Application-owned per-run Door lock flags (Milestone 57 / 57.1).
// Not authored, not serialized, not a generic lock/requirement framework.
// The unlock item is each Door's authored requiredItemId.

#include "core/Vec3.h"
#include "gameplay/Inventory.h"
#include "world/Door.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace gameplay
{
inline constexpr int kNoLockedDoorIndex = -1;

// Same world-scale targeting as M51 grab / M55 pickup.
inline constexpr float kLockedDoorMaxDistance = 2.5f;
inline constexpr float kLockedDoorMinFacingDot = 0.35f;
inline constexpr float kLockedDoorMinDistance = 0.05f;
inline constexpr float kLockedDoorLosBlockFraction = 0.98f;

inline constexpr int kDoorUnlockQuantity = 1;

struct DoorLockRunState
{
    // 1 = unlocked (M53 plate OR may open). 0 = locked (desiredOpen stays false).
    std::vector<std::uint8_t> unlocked{};
};

inline DoorLockRunState MakeDoorLockRunState(std::span<const world::DoorSpec> doors)
{
    DoorLockRunState state{};
    state.unlocked.resize(doors.size());
    for (std::size_t index = 0; index < doors.size(); ++index)
    {
        state.unlocked[index] = world::DoorRequiresInventoryItem(doors[index]) ? 0 : 1;
    }
    return state;
}

inline bool DoorIsRuntimeUnlocked(const DoorLockRunState& state, std::size_t index)
{
    return index < state.unlocked.size() && state.unlocked[index] != 0;
}

void FormatLockedDoorPrompt(
    char* buffer,
    std::size_t bufferSize,
    std::string_view requiredItemId,
    bool hasRequiredItem);

// Nearest in-range in-front runtime-locked required-item Door. Lower session
// index wins when distances are within 1e-5. losBlocked[i] != 0 skips that
// index. Already-unlocked and no-requirement Doors never qualify.
int FindLockedDoorTargetIndex(
    core::Vec3 playerCenter,
    float facingX,
    std::span<const world::DoorSpec> doors,
    std::span<const std::uint8_t> unlocked,
    std::span<const std::uint8_t> losBlocked);

// TryRemove(requiredItemId, 1) first. Only on success mark that runtime Door
// unlocked.
bool TryUnlockLockedDoor(
    Inventory& inventory,
    DoorLockRunState& lockState,
    std::span<const world::DoorSpec> doors,
    int index);
}
