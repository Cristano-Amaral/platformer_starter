#include "world/LevelDefinition.h"

#include <cmath>
#include <cstddef>

namespace world
{
namespace
{
bool Vec3Finite(core::Vec3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool PositiveSize(core::Vec3 size)
{
    return std::isfinite(size.x) && std::isfinite(size.y) && std::isfinite(size.z) && size.x > 0.0f
        && size.y > 0.0f && size.z > 0.0f;
}
}

LevelDefinition CreateLevel01Definition()
{
    LevelDefinition level{};
    level.id = kLevel01Id;
    level.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
    level.killPlaneY = -8.0f;
    level.ground = {{0.0f, -0.25f, 0.0f}, {56.0f, 0.5f, 8.0f}};
    level.elevatedPlatforms = {{
        {{5.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}},
        {{-4.5f, 2.25f, 0.0f}, {3.0f, 0.5f, 2.5f}},
        {{16.5f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}},
        {{-10.0f, 2.0f, 0.0f}, {3.0f, 0.5f, 2.5f}},
        {{-15.5f, 1.75f, 0.0f}, {4.0f, 0.5f, 3.0f}},
        {{-21.0f, 2.75f, 0.0f}, {3.0f, 0.5f, 2.5f}},
    }};
    level.checkpoint1PlatformIndex = 2;
    level.checkpoint2PlatformIndex = 4;
    level.goalPlatformIndex = 5;
    level.slopes[static_cast<std::size_t>(kLevel01WalkableSlopeIndex)] = {
        {21.70f, 1.6732f, 0.0f},
        {6.0f, 0.4f, 4.0f},
        30.0f};
    level.slopes[static_cast<std::size_t>(kLevel01SteepSlopeIndex)] = {
        {25.60f, 0.9660f, 0.0f},
        {2.0f, 0.4f, 3.0f},
        60.0f};
    level.movingPlatform = {{4.0f, 0.4f, 3.0f}, 1.3f, 0.0f, -6.0f, 6.0f, 2.5f, 0.0f};
    level.checkpoints = {{
        {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}},
        {{-15.5f, 2.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {-15.5f, 2.8f, 0.0f}},
    }};
    level.hazards = {{
        {{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}},
        {{-18.5f, 0.5f, 0.0f}, {1.2f, 1.0f, 2.0f}},
    }};
    level.collectibles = {{
        {{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}},
        {{-4.5f, 4.0f, 0.0f}, {1.0f, 1.2f, 1.0f}},
        {{-10.0f, 3.75f, 0.0f}, {1.0f, 1.2f, 1.0f}},
    }};
    level.goal = {{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}};
    level.dynamicBox = {{0.0f, 5.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 30.0f};
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    return level;
}

bool LevelDefinitionHasRequiredAuthoredContent(const LevelDefinition& level)
{
    if (level.id.empty())
    {
        return false;
    }
    if (!Vec3Finite(level.initialSpawnVisualCenter) || !std::isfinite(level.killPlaneY)
        || !Vec3Finite(level.ground.center) || !PositiveSize(level.ground.size)
        || !PositiveSize(level.goal.size) || !Vec3Finite(level.goal.center)
        || !PositiveSize(level.dynamicBox.size) || !Vec3Finite(level.dynamicBox.center)
        || !PositiveSize(level.movingPlatform.size) || !Vec3Finite(level.camera.offset))
    {
        return false;
    }
    if (!std::isfinite(level.dynamicBox.mass) || !(level.dynamicBox.mass > 0.0f)
        || !std::isfinite(level.camera.fieldOfViewY) || !(level.camera.fieldOfViewY > 0.0f)
        || !(level.camera.fieldOfViewY < 180.0f))
    {
        return false;
    }
    if (!std::isfinite(level.movingPlatform.pathMinX) || !std::isfinite(level.movingPlatform.pathMaxX)
        || !std::isfinite(level.movingPlatform.speed) || !std::isfinite(level.movingPlatform.centerY)
        || !std::isfinite(level.movingPlatform.centerZ) || !std::isfinite(level.movingPlatform.startX)
        || !(level.movingPlatform.pathMinX < level.movingPlatform.pathMaxX)
        || !(level.movingPlatform.speed > 0.0f)
        || level.movingPlatform.startX < level.movingPlatform.pathMinX
        || level.movingPlatform.startX > level.movingPlatform.pathMaxX)
    {
        return false;
    }
    if (level.checkpoint1PlatformIndex < 0
        || level.checkpoint1PlatformIndex >= kLevel01ElevatedPlatformCount
        || level.checkpoint2PlatformIndex < 0
        || level.checkpoint2PlatformIndex >= kLevel01ElevatedPlatformCount
        || level.goalPlatformIndex < 0
        || level.goalPlatformIndex >= kLevel01ElevatedPlatformCount)
    {
        return false;
    }

    for (const Box& platform : level.elevatedPlatforms)
    {
        if (!Vec3Finite(platform.center) || !PositiveSize(platform.size))
        {
            return false;
        }
    }
    for (const SlopeSpec& slope : level.slopes)
    {
        if (!Vec3Finite(slope.center) || !PositiveSize(slope.size)
            || !std::isfinite(slope.rotationZDegrees))
        {
            return false;
        }
    }
    for (const CheckpointSpec& checkpoint : level.checkpoints)
    {
        if (!Vec3Finite(checkpoint.center) || !PositiveSize(checkpoint.size)
            || !Vec3Finite(checkpoint.respawnPosition))
        {
            return false;
        }
    }
    for (const HazardSpec& hazard : level.hazards)
    {
        if (!Vec3Finite(hazard.center) || !PositiveSize(hazard.size))
        {
            return false;
        }
    }
    for (const CollectibleSpec& collectible : level.collectibles)
    {
        if (!Vec3Finite(collectible.center) || !PositiveSize(collectible.size))
        {
            return false;
        }
    }

    return true;
}
}
