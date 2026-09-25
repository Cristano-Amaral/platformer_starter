#pragma once

// Bounded M104 presentation seam. Equipment remains authoritative; these
// helpers only resolve its current identities against definitions and a pose.

#include "animation/SkeletalAnimation.h"
#include "gameplay/Equipment.h"
#include "gameplay/ItemDefinition.h"

#include <array>
#include <cmath>
#include <optional>
#include <string_view>

namespace gameplay
{
enum class EquipmentAttachmentStatus
{
    Empty, MissingDefinition, MissingModel, MissingAttachment, MissingJoint, Ready
};

struct VisibleEquipmentAttachment
{
    EquipmentSlot slot = EquipmentSlot::Head;
    std::string_view itemIdentity{};
    std::string_view modelIdentity{};
    std::string_view jointName{};
    int jointIndex = -1;
    EquipmentAttachmentStatus status = EquipmentAttachmentStatus::Empty;
};

inline int ResolveSkeletonJoint(const animation::Skeleton& skeleton, std::string_view name)
{
    for (std::size_t index = 0; index < skeleton.joints.size(); ++index)
        if (skeleton.joints[index].name == name) return static_cast<int>(index);
    return -1;
}

inline VisibleEquipmentAttachment ResolveVisibleEquipmentAttachment(
    EquipmentSlot slot, const Equipment& equipment,
    const GameplayDefinitionRegistry& registry, const animation::Skeleton& skeleton)
{
    VisibleEquipmentAttachment result{}; result.slot = slot;
    result.itemIdentity = equipment.GetEquipped(slot);
    if (result.itemIdentity.empty()) return result;
    const GameplayDefinition* definition = registry.Find(result.itemIdentity);
    if (definition == nullptr || definition->category != GameplayDefinitionCategory::Item
        || definition->item.type != ItemType::Equipment)
    { result.status = EquipmentAttachmentStatus::MissingDefinition; return result; }
    result.modelIdentity = definition->item.worldModelIdentity;
    if (result.modelIdentity.empty())
    { result.status = EquipmentAttachmentStatus::MissingModel; return result; }
    if (!definition->item.equipmentAttachment.has_value())
    { result.status = EquipmentAttachmentStatus::MissingAttachment; return result; }
    result.jointName = definition->item.equipmentAttachment->jointName;
    result.jointIndex = ResolveSkeletonJoint(skeleton, result.jointName);
    if (result.jointIndex < 0)
    { result.status = EquipmentAttachmentStatus::MissingJoint; return result; }
    result.status = EquipmentAttachmentStatus::Ready;
    return result;
}

inline animation::Quaternion MultiplyQuaternion(
    animation::Quaternion a, animation::Quaternion b)
{
    return animation::Normalize({
        a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
        a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
        a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z});
}

inline animation::Quaternion EulerXyzDegrees(core::Vec3 degrees)
{
    constexpr float kRadians = 3.14159265358979323846f / 180.0f;
    const auto axis = [](float angle, core::Vec3 v) {
        const float half = angle * 0.5f;
        const float sine = std::sin(half);
        return animation::Quaternion{v.x*sine, v.y*sine, v.z*sine, std::cos(half)};
    };
    const animation::Quaternion x = axis(degrees.x*kRadians, {1,0,0});
    const animation::Quaternion y = axis(degrees.y*kRadians, {0,1,0});
    const animation::Quaternion z = axis(degrees.z*kRadians, {0,0,1});
    return MultiplyQuaternion(MultiplyQuaternion(z, y), x);
}

// Column-vector composition: world * animated joint-global * authored local.
inline animation::Matrix4 ComposeEquipmentAttachmentTransform(
    const animation::Matrix4& characterWorld,
    const animation::Matrix4& animatedJointGlobal,
    const EquipmentAttachmentDefinition& attachment)
{
    animation::JointTransform local{};
    local.translation = attachment.translation;
    local.rotation = EulerXyzDegrees(attachment.rotationDegrees);
    local.scale = attachment.scale;
    return animation::Multiply(animation::Multiply(characterWorld, animatedJointGlobal),
        animation::TransformMatrix(local));
}
}
