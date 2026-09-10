#include "world/LevelDefinition.h"
#include "world/LevelFile.h"

#include "physics/PhysicsCapacity.h"

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
        || !PositiveSize(level.movingPlatform.size) || !Vec3Finite(level.camera.offset))
    {
        return false;
    }
    if (!std::isfinite(level.camera.fieldOfViewY) || !(level.camera.fieldOfViewY > 0.0f)
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
    const int platformCount = static_cast<int>(level.elevatedPlatforms.size());
    if (!physics::AuthoredPhysicsBodiesWithinBudget(
            platformCount,
            static_cast<int>(level.dynamicBoxes.size()),
            static_cast<int>(level.doors.size())))
    {
        return false;
    }
    if (CountLevelV1RecordLines(level) > static_cast<int>(kMaxLevelLines))
    {
        return false;
    }
    if (platformCount < kMinElevatedPlatformCount
        || level.checkpoint1PlatformIndex < 0 || level.checkpoint1PlatformIndex >= platformCount
        || level.checkpoint2PlatformIndex < 0 || level.checkpoint2PlatformIndex >= platformCount
        || level.goalPlatformIndex < 0 || level.goalPlatformIndex >= platformCount)
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
    for (const DynamicBoxSpec& box : level.dynamicBoxes)
    {
        if (!DynamicBoxSpecIsValid(box))
        {
            return false;
        }
    }
    for (const PressurePlateSpec& plate : level.pressurePlates)
    {
        if (!PressurePlateSpecIsValid(plate)
            || !PressurePlateDoorLinkIsValid(plate.linkedDoorIndex, level.doors.size()))
        {
            return false;
        }
    }
    for (const DoorSpec& door : level.doors)
    {
        if (!DoorSpecIsValid(door))
        {
            return false;
        }
    }
    for (const ItemPickupSpec& pickup : level.itemPickups)
    {
        if (!ItemPickupSpecIsValid(pickup))
        {
            return false;
        }
    }
    for (const StaticPropSpec& prop : level.staticProps)
    {
        if (!StaticPropSpecIsValid(prop))
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

    if (a.elevatedPlatforms.size() != b.elevatedPlatforms.size()
        || a.checkpoints.size() != b.checkpoints.size()
        || a.hazards.size() != b.hazards.size()
        || a.collectibles.size() != b.collectibles.size()
        || a.dynamicBoxes.size() != b.dynamicBoxes.size()
        || a.pressurePlates.size() != b.pressurePlates.size()
        || a.doors.size() != b.doors.size()
        || a.itemPickups.size() != b.itemPickups.size()
        || a.staticProps.size() != b.staticProps.size())
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
    for (std::size_t index = 0; index < a.dynamicBoxes.size(); ++index)
    {
        if (!Vec3Equal(a.dynamicBoxes[index].center, b.dynamicBoxes[index].center)
            || !Vec3Equal(a.dynamicBoxes[index].size, b.dynamicBoxes[index].size)
            || a.dynamicBoxes[index].massKg != b.dynamicBoxes[index].massKg)
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < a.pressurePlates.size(); ++index)
    {
        if (!Vec3Equal(a.pressurePlates[index].center, b.pressurePlates[index].center)
            || !Vec3Equal(a.pressurePlates[index].size, b.pressurePlates[index].size)
            || a.pressurePlates[index].linkedDoorIndex != b.pressurePlates[index].linkedDoorIndex
            || a.pressurePlates[index].activateByDynamicBox
                != b.pressurePlates[index].activateByDynamicBox
            || a.pressurePlates[index].activateByPlayer != b.pressurePlates[index].activateByPlayer
            || a.pressurePlates[index].visibleInGameplay != b.pressurePlates[index].visibleInGameplay)
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < a.doors.size(); ++index)
    {
        if (!Vec3Equal(a.doors[index].center, b.doors[index].center)
            || !Vec3Equal(a.doors[index].size, b.doors[index].size)
            || a.doors[index].openDistance != b.doors[index].openDistance
            || a.doors[index].requiredItemId != b.doors[index].requiredItemId)
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < a.itemPickups.size(); ++index)
    {
        if (!Vec3Equal(a.itemPickups[index].position, b.itemPickups[index].position)
            || a.itemPickups[index].itemId != b.itemPickups[index].itemId
            || a.itemPickups[index].quantity != b.itemPickups[index].quantity
            || a.itemPickups[index].modelIdentity != b.itemPickups[index].modelIdentity
            || !Vec3Equal(a.itemPickups[index].visualOffset, b.itemPickups[index].visualOffset)
            || !Vec3Equal(
                a.itemPickups[index].visualRotationDegrees,
                b.itemPickups[index].visualRotationDegrees)
            || !Vec3Equal(a.itemPickups[index].visualScale, b.itemPickups[index].visualScale)
            || a.itemPickups[index].showInteractionBounds
                != b.itemPickups[index].showInteractionBounds
            || a.itemPickups[index].targetHighlightIntensity
                != b.itemPickups[index].targetHighlightIntensity)
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < a.staticProps.size(); ++index)
    {
        if (a.staticProps[index].modelIdentity != b.staticProps[index].modelIdentity
            || !Vec3Equal(a.staticProps[index].position, b.staticProps[index].position)
            || !Vec3Equal(a.staticProps[index].rotationDegrees, b.staticProps[index].rotationDegrees)
            || !Vec3Equal(a.staticProps[index].scale, b.staticProps[index].scale))
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
        && Vec3Equal(a.camera.offset, b.camera.offset)
        && a.camera.fieldOfViewY == b.camera.fieldOfViewY;
}
}
