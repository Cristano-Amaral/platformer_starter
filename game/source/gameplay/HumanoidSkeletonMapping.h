#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace gameplay
{
enum class HumanoidJointRole
{
    Hips, Spine, Chest, Neck, Head,
    LeftShoulder, LeftUpperArm, LeftLowerArm, LeftHand,
    RightShoulder, RightUpperArm, RightLowerArm, RightHand,
    LeftUpperLeg, LeftLowerLeg, LeftFoot,
    RightUpperLeg, RightLowerLeg, RightFoot,
    Count
};
inline constexpr std::size_t kHumanoidJointRoleCount = static_cast<std::size_t>(HumanoidJointRole::Count);
inline constexpr std::array<std::string_view, kHumanoidJointRoleCount> kHumanoidJointRoleNames = {
    "Hips", "Spine", "Chest", "Neck", "Head",
    "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
    "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand",
    "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
    "RightUpperLeg", "RightLowerLeg", "RightFoot"};
inline std::string_view HumanoidJointRoleName(HumanoidJointRole role)
{
    const auto index = static_cast<std::size_t>(role);
    return index < kHumanoidJointRoleCount ? kHumanoidJointRoleNames[index] : std::string_view{};
}
inline std::optional<HumanoidJointRole> HumanoidJointRoleFromName(std::string_view name)
{
    for (std::size_t i = 0; i < kHumanoidJointRoleCount; ++i)
        if (name == kHumanoidJointRoleNames[i]) return static_cast<HumanoidJointRole>(i);
    return std::nullopt;
}
inline bool HumanoidJointRoleRequired(HumanoidJointRole role)
{
    return role != HumanoidJointRole::Chest && role != HumanoidJointRole::Neck
        && role != HumanoidJointRole::LeftShoulder && role != HumanoidJointRole::RightShoulder;
}
struct HumanoidSkeletonMapping
{
    // Empty is None. Joint names are authored identities; indices are resolved from the loaded model.
    std::array<std::string, kHumanoidJointRoleCount> joints{};
};
}
