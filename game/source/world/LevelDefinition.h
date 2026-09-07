#pragma once

// Immutable authored level data populated by the Level File v1 parser.
// Runtime gameplay state stays outside this struct.

#include "core/Vec3.h"
#include "world/CollectibleWorld.h"
#include "world/GreyboxWorld.h"
#include "world/HazardWorld.h"
#include "world/LevelGoal.h"
#include "world/MovingPlatform.h"
#include "world/RespawnWorld.h"
#include "world/Slope.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace world
{
inline constexpr std::string_view kLevel01Id = "level_01";
// Canonical Level 01 instance counts. Variable-count v1 files may differ.
inline constexpr int kLevel01ElevatedPlatformCount = 6;
inline constexpr int kLevel01SlopeCount = 2;
inline constexpr int kLevel01WalkableSlopeIndex = 0;
inline constexpr int kLevel01SteepSlopeIndex = 1;

// Minimum platforms so support_index_* can stay in range.
inline constexpr int kMinElevatedPlatformCount = 1;
// Physics-derived platform budget (Jolt body allocator minus non-platform
// bodies). Must match physics::kMaxPhysicsElevatedPlatformCount.
inline constexpr int kMaxElevatedPlatformCount = 58;

static_assert(kMinElevatedPlatformCount >= 1);
static_assert(kLevel01ElevatedPlatformCount <= kMaxElevatedPlatformCount);

struct DynamicBoxSpec
{
    core::Vec3 center{};
    core::Vec3 size{};
    float mass = 0.0f;
};

// Level framing only. Follow dead-zone and sharpness stay on PlatformerCamera.
struct LevelCameraSpec
{
    core::Vec3 offset{};
    float fieldOfViewY = 0.0f;
};

struct LevelDefinition
{
    std::string id;

    core::Vec3 initialSpawnVisualCenter{};
    float killPlaneY = 0.0f;

    Box ground{};
    std::vector<Box> elevatedPlatforms{};
    // Authored v1 support_index_* metadata. 0-based indices into
    // elevatedPlatforms. Not gameplay runtime state. Platform delete remaps
    // R > D and rejects R == D; Add/Duplicate append so existing indices stay.
    int checkpoint1PlatformIndex = 0;
    int checkpoint2PlatformIndex = 0;
    int goalPlatformIndex = 0;

    std::array<SlopeSpec, kLevel01SlopeCount> slopes{};
    MovingPlatformSpec movingPlatform{};
    std::vector<CheckpointSpec> checkpoints{};
    std::vector<HazardSpec> hazards{};
    std::vector<CollectibleSpec> collectibles{};
    LevelGoalSpec goal{};
    DynamicBoxSpec dynamicBox{};
    LevelCameraSpec camera{};
};

bool LevelDefinitionHasRequiredAuthoredContent(const LevelDefinition& level);

// Exact field-by-field comparison of authored data. Every field is listed
// explicitly so a new v1 record cannot silently escape writer round-trip proof
// or editor change detection. Not a reflection/equality framework.
bool AuthoredLevelDataEqual(const LevelDefinition& a, const LevelDefinition& b);
}
