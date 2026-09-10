#pragma once

// Application-owned per-run Door lock flags (Milestone 57).
// Not authored, not serialized, not a generic lock/requirement framework.
// The concrete unlock item is the production M54 Inventory itemId "key".

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

inline constexpr std::string_view kDoorUnlockItemId = "key";
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
        state.unlocked[index] = doors[index].requiresKey ? 0 : 1;
    }
    return state;
}

inline bool DoorIsRuntimeUnlocked(const DoorLockRunState& state, std::size_t index)
{
    return index < state.unlocked.size() && state.unlocked[index] != 0;
}

inline const char* LockedDoorPromptText(bool hasKey)
{
    return hasKey ? "E Unlock Door (key)" : "Requires key";
}

// Nearest in-range in-front runtime-locked requiresKey Door. Lower session
// index wins when distances are within 1e-5. losBlocked[i] != 0 skips that
// index. Already-unlocked and non-key Doors never qualify.
int FindLockedDoorTargetIndex(
    core::Vec3 playerCenter,
    float facingX,
    std::span<const world::DoorSpec> doors,
    std::span<const std::uint8_t> unlocked,
    std::span<const std::uint8_t> losBlocked);

// TryRemove("key", 1) first. Only on success mark that runtime Door unlocked.
bool TryUnlockLockedDoor(
    Inventory& inventory,
    DoorLockRunState& lockState,
    std::span<const world::DoorSpec> doors,
    int index);
}
