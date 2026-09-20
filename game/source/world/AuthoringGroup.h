#pragma once

// Persistent Authoring Group metadata. Organizational Level Design composition
// of existing authored objects. Not a gameplay entity, transform parent,
// Prefab, or GUID.

#include "world/LevelIdentity.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace world
{
enum class AuthoringGroupMemberKind
{
    Spawn,
    Camera,
    Ground,
    ElevatedPlatform,
    Slope,
    MovingPlatform,
    Checkpoint,
    Hazard,
    Collectible,
    Goal,
    DynamicBox,
    PressurePlate,
    Door,
    ItemPickup,
    StaticProp,
    PointLight,
    SpotLight,
};

struct AuthoringGroupMember
{
    AuthoringGroupMemberKind kind = AuthoringGroupMemberKind::Spawn;
    std::size_t index = 0;
};

inline bool operator==(AuthoringGroupMember a, AuthoringGroupMember b)
{
    return a.kind == b.kind && a.index == b.index;
}

inline bool operator!=(AuthoringGroupMember a, AuthoringGroupMember b)
{
    return !(a == b);
}

struct AuthoringGroup
{
    // Repository-safe identifier: same grammar as Level `id`.
    std::string name;
    // members[0] is the preferred/deterministic PRIMARY when the group is
    // reconstructed as an M78 multi-selection.
    std::vector<AuthoringGroupMember> members;
};

inline constexpr std::string_view kAuthoringGroupRecordKeyword = "authoring_group";

inline const char* AuthoringGroupMemberKindKeyword(AuthoringGroupMemberKind kind)
{
    switch (kind)
    {
    case AuthoringGroupMemberKind::Spawn:
        return "spawn";
    case AuthoringGroupMemberKind::Camera:
        return "camera";
    case AuthoringGroupMemberKind::Ground:
        return "ground";
    case AuthoringGroupMemberKind::ElevatedPlatform:
        return "platform";
    case AuthoringGroupMemberKind::Slope:
        return "slope";
    case AuthoringGroupMemberKind::MovingPlatform:
        return "moving_platform";
    case AuthoringGroupMemberKind::Checkpoint:
        return "checkpoint";
    case AuthoringGroupMemberKind::Hazard:
        return "hazard";
    case AuthoringGroupMemberKind::Collectible:
        return "collectible";
    case AuthoringGroupMemberKind::Goal:
        return "level_goal";
    case AuthoringGroupMemberKind::DynamicBox:
        return "dynamic_box";
    case AuthoringGroupMemberKind::PressurePlate:
        return "pressure_plate";
    case AuthoringGroupMemberKind::Door:
        return "door";
    case AuthoringGroupMemberKind::ItemPickup:
        return "item_pickup";
    case AuthoringGroupMemberKind::StaticProp:
        return "static_prop";
    case AuthoringGroupMemberKind::PointLight:
        return "point_light";
    case AuthoringGroupMemberKind::SpotLight:
        return "spot_light";
    }
    return "";
}

inline bool TryParseAuthoringGroupMemberKind(
    std::string_view token,
    AuthoringGroupMemberKind& kind)
{
    if (token == "spawn")
    {
        kind = AuthoringGroupMemberKind::Spawn;
        return true;
    }
    if (token == "camera")
    {
        kind = AuthoringGroupMemberKind::Camera;
        return true;
    }
    if (token == "ground")
    {
        kind = AuthoringGroupMemberKind::Ground;
        return true;
    }
    if (token == "platform")
    {
        kind = AuthoringGroupMemberKind::ElevatedPlatform;
        return true;
    }
    if (token == "slope")
    {
        kind = AuthoringGroupMemberKind::Slope;
        return true;
    }
    if (token == "moving_platform")
    {
        kind = AuthoringGroupMemberKind::MovingPlatform;
        return true;
    }
    if (token == "checkpoint")
    {
        kind = AuthoringGroupMemberKind::Checkpoint;
        return true;
    }
    if (token == "hazard")
    {
        kind = AuthoringGroupMemberKind::Hazard;
        return true;
    }
    if (token == "collectible")
    {
        kind = AuthoringGroupMemberKind::Collectible;
        return true;
    }
    if (token == "level_goal")
    {
        kind = AuthoringGroupMemberKind::Goal;
        return true;
    }
    if (token == "dynamic_box")
    {
        kind = AuthoringGroupMemberKind::DynamicBox;
        return true;
    }
    if (token == "pressure_plate")
    {
        kind = AuthoringGroupMemberKind::PressurePlate;
        return true;
    }
    if (token == "door")
    {
        kind = AuthoringGroupMemberKind::Door;
        return true;
    }
    if (token == "item_pickup")
    {
        kind = AuthoringGroupMemberKind::ItemPickup;
        return true;
    }
    if (token == "static_prop")
    {
        kind = AuthoringGroupMemberKind::StaticProp;
        return true;
    }
    if (token == "point_light")
    {
        kind = AuthoringGroupMemberKind::PointLight;
        return true;
    }
    if (token == "spot_light")
    {
        kind = AuthoringGroupMemberKind::SpotLight;
        return true;
    }
    return false;
}

inline bool IsValidAuthoringGroupName(std::string_view name)
{
    return IsValidLevelIdToken(name);
}
}
