#pragma once

#include "animation/CharacterAssetValidator.h"
#include "gameplay/HumanoidSkeletonMapping.h"

#include <array>
#include <cctype>
#include <string>
#include <vector>

namespace animation
{
enum class HumanoidMappingState { NoMapping, Incomplete, Invalid, Usable };
inline const char* HumanoidMappingStateName(HumanoidMappingState state)
{
    switch (state) {
    case HumanoidMappingState::NoMapping: return "No Mapping";
    case HumanoidMappingState::Incomplete: return "Incomplete";
    case HumanoidMappingState::Invalid: return "Invalid";
    case HumanoidMappingState::Usable: return "Usable";
    }
    return "Invalid";
}
struct HumanoidMappingDiagnostic
{
    gameplay::HumanoidJointRole role{};
    std::string detail;
    int jointIndex = -1;
};
struct HumanoidMappingValidation
{
    HumanoidMappingState state = HumanoidMappingState::NoMapping;
    std::vector<HumanoidMappingDiagnostic> details;
};

inline HumanoidMappingValidation ValidateHumanoidMapping(
    const gameplay::HumanoidSkeletonMapping& mapping, const CharacterModelValidationResult& model)
{
    using gameplay::HumanoidJointRole;
    HumanoidMappingValidation result;
    std::array<int, gameplay::kHumanoidJointRoleCount> resolved{};
    resolved.fill(-1);
    bool any = false, invalid = false, incomplete = false;
    for (std::size_t i = 0; i < resolved.size(); ++i)
    {
        const auto role = static_cast<HumanoidJointRole>(i);
        const auto& name = mapping.joints[i];
        if (name.empty())
        {
            if (gameplay::HumanoidJointRoleRequired(role))
            { incomplete = true; result.details.push_back({role, "Required role unassigned", -1}); }
            continue;
        }
        any = true;
        int matches = 0;
        for (std::size_t j = 0; j < model.allJoints.size(); ++j)
            if (model.allJoints[j].name == name) { resolved[i] = static_cast<int>(j); ++matches; }
        if (matches > 1)
        { invalid = true; result.details.push_back({role, "Ambiguous joint name: " + name, -1}); }
        if (resolved[i] < 0)
        { invalid = true; result.details.push_back({role, "Stale/missing joint: " + name, -1}); }
        else result.details.push_back({role, name, resolved[i]});
    }
    if (!any) return result;
    if (model.status != CharacterModelValidationStatus::Resolved || !model.hasSkeleton
        || !model.skinned || model.allJoints.size() != static_cast<std::size_t>(model.jointCount))
    { invalid = true; result.details.push_back({HumanoidJointRole::Hips, "World Model has no usable complete skeleton", -1}); }
    for (std::size_t i = 0; i < resolved.size(); ++i)
        if (resolved[i] >= 0)
            for (std::size_t j = i + 1; j < resolved.size(); ++j)
                if (resolved[i] == resolved[j])
                { invalid = true; result.details.push_back({static_cast<HumanoidJointRole>(j),
                    "Duplicate joint assignment (including left/right conflict): " + mapping.joints[j], resolved[j]}); }
    const auto ancestor = [&](HumanoidJointRole top, HumanoidJointRole lower) {
        int parent = resolved[static_cast<std::size_t>(top)];
        int child = resolved[static_cast<std::size_t>(lower)];
        if (parent < 0 || child < 0 || static_cast<std::size_t>(child) >= model.allJoints.size()) return;
        int node = model.allJoints[child].parentIndex;
        for (std::size_t steps = 0; node >= 0 && steps < model.allJoints.size(); ++steps)
        {
            if (node == parent) return;
            if (static_cast<std::size_t>(node) >= model.allJoints.size()) break;
            node = model.allJoints[node].parentIndex;
        }
        invalid = true;
        result.details.push_back({lower, std::string(gameplay::HumanoidJointRoleName(top))
            + " is not an ancestor of " + std::string(gameplay::HumanoidJointRoleName(lower)), child});
    };
    const auto chain = [&](std::initializer_list<HumanoidJointRole> roles) {
        int previous = -1;
        for (auto role : roles)
        {
            if (resolved[static_cast<std::size_t>(role)] < 0) continue;
            if (previous >= 0) ancestor(static_cast<HumanoidJointRole>(previous), role);
            previous = static_cast<int>(role);
        }
    };
    chain({HumanoidJointRole::Hips, HumanoidJointRole::Spine, HumanoidJointRole::Chest,
        HumanoidJointRole::Neck, HumanoidJointRole::Head});
    chain({HumanoidJointRole::LeftUpperArm, HumanoidJointRole::LeftLowerArm, HumanoidJointRole::LeftHand});
    chain({HumanoidJointRole::RightUpperArm, HumanoidJointRole::RightLowerArm, HumanoidJointRole::RightHand});
    chain({HumanoidJointRole::LeftUpperLeg, HumanoidJointRole::LeftLowerLeg, HumanoidJointRole::LeftFoot});
    chain({HumanoidJointRole::RightUpperLeg, HumanoidJointRole::RightLowerLeg, HumanoidJointRole::RightFoot});
    if (resolved[static_cast<std::size_t>(HumanoidJointRole::LeftShoulder)] >= 0)
    { ancestor(resolved[static_cast<std::size_t>(HumanoidJointRole::Chest)] >= 0
        ? HumanoidJointRole::Chest : HumanoidJointRole::Spine, HumanoidJointRole::LeftShoulder);
      ancestor(HumanoidJointRole::LeftShoulder, HumanoidJointRole::LeftUpperArm); }
    if (resolved[static_cast<std::size_t>(HumanoidJointRole::RightShoulder)] >= 0)
    { ancestor(resolved[static_cast<std::size_t>(HumanoidJointRole::Chest)] >= 0
        ? HumanoidJointRole::Chest : HumanoidJointRole::Spine, HumanoidJointRole::RightShoulder);
      ancestor(HumanoidJointRole::RightShoulder, HumanoidJointRole::RightUpperArm); }
    result.state = invalid ? HumanoidMappingState::Invalid
        : incomplete ? HumanoidMappingState::Incomplete : HumanoidMappingState::Usable;
    return result;
}

inline std::string NormalizeHumanoidJointName(std::string_view name)
{
    std::string normalized;
    for (unsigned char ch : name)
        if (std::isalnum(ch)) normalized += static_cast<char>(std::tolower(ch));
    return normalized;
}
inline void SuggestHumanoidMapping(gameplay::HumanoidSkeletonMapping& mapping,
    const CharacterModelValidationResult& model)
{
    if (!model.hasSkeleton || model.allJoints.size() != static_cast<std::size_t>(model.jointCount)) return;
    constexpr std::array<std::string_view, gameplay::kHumanoidJointRoleCount> aliases = {
        "hips pelvis", "spine", "chest", "neck", "head",
        "leftshoulder shoulderleft lshoulder", "leftupperarm upperarmleft lupperarm",
        "leftlowerarm leftforearm lowerarmleft forearmleft llowerarm lforearm",
        "lefthand handleft lhand",
        "rightshoulder shoulderright rshoulder", "rightupperarm upperarmright rupperarm",
        "rightlowerarm rightforearm lowerarmright forearmright rlowerarm rforearm",
        "righthand handright rhand",
        "leftupperleg leftthigh upperlegleft thighleft lupperleg lthigh",
        "leftlowerleg leftcalf leftshin lowerlegleft calfleft shinleft llowerleg lcalf lshin",
        "leftfoot footleft lfoot",
        "rightupperleg rightthigh upperlegright thighright rupperleg rthigh",
        "rightlowerleg rightcalf rightshin lowerlegright calfright shinright rlowerleg rcalf rshin",
        "rightfoot footright rfoot"};
    for (std::size_t i = 0; i < mapping.joints.size(); ++i)
    {
        if (!mapping.joints[i].empty()) continue;
        const auto match = [&](std::string_view candidate) {
            const auto normalized = NormalizeHumanoidJointName(candidate);
            const auto patterns = aliases[i];
            std::size_t start = 0;
            while (start < patterns.size())
            {
                const auto end = patterns.find(' ', start);
                const auto pattern = patterns.substr(start, end == std::string_view::npos ? end : end - start);
                if (normalized == pattern) return true;
                if (end == std::string_view::npos) break;
                start = end + 1;
            }
            return false;
        };
        const CharacterJointDiagnostic* unique = nullptr;
        for (const auto& joint : model.allJoints)
            if (match(joint.name)) { if (unique != nullptr) { unique = nullptr; break; } unique = &joint; }
        if (unique != nullptr)
        {
            bool alreadyAssigned = false;
            for (std::size_t other = 0; other < mapping.joints.size(); ++other)
                if (other != i && mapping.joints[other] == unique->name) alreadyAssigned = true;
            if (!alreadyAssigned) mapping.joints[i] = unique->name;
        }
    }
}
}
