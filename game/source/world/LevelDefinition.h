#pragma once

// Immutable authored level data. Level 01 instances are created by
// CreateLevel01Definition() in Level01.cpp. Runtime state stays elsewhere.

#include "core/Vec3.h"
#include "world/CollectibleWorld.h"
#include "world/GreyboxWorld.h"
#include "world/HazardWorld.h"
#include "world/LevelGoal.h"
#include "world/MovingPlatform.h"
#include "world/RespawnWorld.h"
#include "world/Slope.h"

#include <array>
#include <string_view>

namespace world
{
inline constexpr std::string_view kLevel01Id = "level_01";
inline constexpr int kLevel01ElevatedPlatformCount = 6;
inline constexpr int kLevel01SlopeCount = 2;
inline constexpr int kLevel01WalkableSlopeIndex = 0;
inline constexpr int kLevel01SteepSlopeIndex = 1;

static_assert(kCheckpointCount == 2);
static_assert(kHazardCount == 2);
static_assert(kCollectibleCount == 3);

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
    std::string_view id;

    core::Vec3 initialSpawnVisualCenter{};
    float killPlaneY = 0.0f;

    Box ground{};
    std::array<Box, kLevel01ElevatedPlatformCount> elevatedPlatforms{};
    int checkpoint1PlatformIndex = 0;
    int checkpoint2PlatformIndex = 0;
    int goalPlatformIndex = 0;

    std::array<SlopeSpec, kLevel01SlopeCount> slopes{};
    MovingPlatformSpec movingPlatform{};
    std::array<CheckpointSpec, kCheckpointCount> checkpoints{};
    std::array<HazardSpec, kHazardCount> hazards{};
    std::array<CollectibleSpec, kCollectibleCount> collectibles{};
    LevelGoalSpec goal{};
    DynamicBoxSpec dynamicBox{};
    LevelCameraSpec camera{};
};

LevelDefinition CreateLevel01Definition();
bool LevelDefinitionHasRequiredAuthoredContent(const LevelDefinition& level);
}
