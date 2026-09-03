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

bool Vec3Equal(core::Vec3 a, core::Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool BoxEqual(const Box& a, const Box& b)
{
    return Vec3Equal(a.center, b.center) && Vec3Equal(a.size, b.size);
}
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

bool AuthoredLevelDataEqual(const LevelDefinition& a, const LevelDefinition& b)
{
    if (a.id != b.id || !Vec3Equal(a.initialSpawnVisualCenter, b.initialSpawnVisualCenter)
        || a.killPlaneY != b.killPlaneY || !BoxEqual(a.ground, b.ground)
        || a.checkpoint1PlatformIndex != b.checkpoint1PlatformIndex
        || a.checkpoint2PlatformIndex != b.checkpoint2PlatformIndex
        || a.goalPlatformIndex != b.goalPlatformIndex)
    {
        return false;
    }

    for (std::size_t index = 0; index < a.elevatedPlatforms.size(); ++index)
    {
        if (!BoxEqual(a.elevatedPlatforms[index], b.elevatedPlatforms[index]))
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < a.slopes.size(); ++index)
    {
        if (!Vec3Equal(a.slopes[index].center, b.slopes[index].center)
            || !Vec3Equal(a.slopes[index].size, b.slopes[index].size)
            || a.slopes[index].rotationZDegrees != b.slopes[index].rotationZDegrees)
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < a.checkpoints.size(); ++index)
    {
        if (!Vec3Equal(a.checkpoints[index].center, b.checkpoints[index].center)
            || !Vec3Equal(a.checkpoints[index].size, b.checkpoints[index].size)
            || !Vec3Equal(
                a.checkpoints[index].respawnPosition,
                b.checkpoints[index].respawnPosition))
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < a.hazards.size(); ++index)
    {
        if (!Vec3Equal(a.hazards[index].center, b.hazards[index].center)
            || !Vec3Equal(a.hazards[index].size, b.hazards[index].size))
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < a.collectibles.size(); ++index)
    {
        if (!Vec3Equal(a.collectibles[index].center, b.collectibles[index].center)
            || !Vec3Equal(a.collectibles[index].size, b.collectibles[index].size))
        {
            return false;
        }
    }

    return Vec3Equal(a.movingPlatform.size, b.movingPlatform.size)
        && a.movingPlatform.centerY == b.movingPlatform.centerY
        && a.movingPlatform.centerZ == b.movingPlatform.centerZ
        && a.movingPlatform.pathMinX == b.movingPlatform.pathMinX
        && a.movingPlatform.pathMaxX == b.movingPlatform.pathMaxX
        && a.movingPlatform.speed == b.movingPlatform.speed
        && a.movingPlatform.startX == b.movingPlatform.startX
        && Vec3Equal(a.goal.center, b.goal.center) && Vec3Equal(a.goal.size, b.goal.size)
        && Vec3Equal(a.dynamicBox.center, b.dynamicBox.center)
        && Vec3Equal(a.dynamicBox.size, b.dynamicBox.size)
        && a.dynamicBox.mass == b.dynamicBox.mass
        && Vec3Equal(a.camera.offset, b.camera.offset)
        && a.camera.fieldOfViewY == b.camera.fieldOfViewY;
}
}
