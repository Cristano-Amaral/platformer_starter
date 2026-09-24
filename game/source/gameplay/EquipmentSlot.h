#pragma once

// Typed equipment slots (Milestone 100). Intentionally small. Not sockets,
// attachment, armor rendering, or a character paper-doll system.

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace gameplay
{
enum class EquipmentSlot
{
    Head = 0,
    Body,
    MainHand,
    OffHand,
    Accessory,
};

inline constexpr std::size_t kEquipmentSlotCount = 5;

inline constexpr std::array<std::string_view, kEquipmentSlotCount> kEquipmentSlotNames = {
    "Head",
    "Body",
    "MainHand",
    "OffHand",
    "Accessory",
};

inline constexpr std::array<EquipmentSlot, kEquipmentSlotCount> kEquipmentSlots = {
    EquipmentSlot::Head,
    EquipmentSlot::Body,
    EquipmentSlot::MainHand,
    EquipmentSlot::OffHand,
    EquipmentSlot::Accessory,
};

inline std::string_view EquipmentSlotName(EquipmentSlot slot)
{
    const auto index = static_cast<std::size_t>(slot);
    if (index >= kEquipmentSlotCount)
    {
        return {};
    }
    return kEquipmentSlotNames[index];
}

inline std::optional<EquipmentSlot> EquipmentSlotFromName(std::string_view name)
{
    for (std::size_t index = 0; index < kEquipmentSlotCount; ++index)
    {
        if (name == kEquipmentSlotNames[index])
        {
            return static_cast<EquipmentSlot>(index);
        }
    }
    return std::nullopt;
}

inline bool IsValidEquipmentSlot(EquipmentSlot slot)
{
    return static_cast<std::size_t>(slot) < kEquipmentSlotCount;
}
}
