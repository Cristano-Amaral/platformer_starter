#include "world/Collision.h"

#include "world/GreyboxWorld.h"

// Legacy AABB helpers. Player movement no longer calls these after Milestone 11.

namespace world
{
namespace
{
// Post-snap reconstructed extents can sit one ulp past a face. Keep this small so
// geometry is unchanged while stable contact and wall/ceiling stops still hold.
constexpr float kSurfaceEpsilon = 0.001f;

bool OverlapsClosed(float aMin, float aMax, float bMin, float bMax)
{
    return aMin <= bMax && bMin <= aMax;
}

// Touching a top/bottom face is not a side hit, so standing on a box can still walk.
bool OverlapsInterior(float aMin, float aMax, float bMin, float bMax)
{
    return aMin < bMax - kSurfaceEpsilon && bMin < aMax - kSurfaceEpsilon;
}

float BoxMinX(const Box& box)
{
    return box.center.x - box.size.x * 0.5f;
}

float BoxMaxX(const Box& box)
{
    return box.center.x + box.size.x * 0.5f;
}

float BoxMinZ(const Box& box)
{
    return box.center.z - box.size.z * 0.5f;
}

float BoxMaxZ(const Box& box)
{
    return box.center.z + box.size.z * 0.5f;
}

template <typename Function>
void ForEachSolidBox(Function&& function)
{
    function(kGround);
    for (const Box& platform : kElevatedPlatforms)
    {
        function(platform);
    }
}

void ConsiderSupportSurface(
    const Box& box,
    float previousBottom,
    float currentBottom,
    float playerMinX,
    float playerMaxX,
    float playerMinZ,
    float playerMaxZ,
    bool& found,
    float& bestTop)
{
    const float top = TopY(box);

    const bool startedAtOrAbove = previousBottom >= top - kSurfaceEpsilon;
    const bool nowAtOrBelow = currentBottom <= top + kSurfaceEpsilon;
    if (!startedAtOrAbove || !nowAtOrBelow)
    {
        return;
    }

    if (!OverlapsClosed(playerMinX, playerMaxX, BoxMinX(box), BoxMaxX(box)))
    {
        return;
    }
    if (!OverlapsClosed(playerMinZ, playerMaxZ, BoxMinZ(box), BoxMaxZ(box)))
    {
        return;
    }

    if (!found || top > bestTop)
    {
        found = true;
        bestTop = top;
    }
}
}

float ResolveHorizontalPosition(
    core::Vec3 previousPosition,
    core::Vec3 proposedPosition,
    core::Vec3 size)
{
    const float displacementX = proposedPosition.x - previousPosition.x;
    if (displacementX == 0.0f)
    {
        return proposedPosition.x;
    }

    const float halfX = size.x * 0.5f;
    const float halfY = size.y * 0.5f;
    const float halfZ = size.z * 0.5f;
    const float previousMinX = previousPosition.x - halfX;
    const float previousMaxX = previousPosition.x + halfX;
    const float proposedMinX = proposedPosition.x - halfX;
    const float proposedMaxX = proposedPosition.x + halfX;
    const float playerMinY = proposedPosition.y - halfY;
    const float playerMaxY = proposedPosition.y + halfY;
    const float playerMinZ = proposedPosition.z - halfZ;
    const float playerMaxZ = proposedPosition.z + halfZ;

    bool hit = false;
    float nearestFaceX = 0.0f;

    ForEachSolidBox([&](const Box& box) {
        if (!OverlapsInterior(playerMinY, playerMaxY, BottomY(box), TopY(box)))
        {
            return;
        }
        if (!OverlapsClosed(playerMinZ, playerMaxZ, BoxMinZ(box), BoxMaxZ(box)))
        {
            return;
        }

        if (displacementX > 0.0f)
        {
            const float boxLeft = BoxMinX(box);
            const bool crossedLeftFace =
                previousMaxX <= boxLeft + kSurfaceEpsilon && proposedMaxX >= boxLeft - kSurfaceEpsilon;
            if (!crossedLeftFace)
            {
                return;
            }
            if (!hit || boxLeft < nearestFaceX)
            {
                hit = true;
                nearestFaceX = boxLeft;
            }
        }
        else
        {
            const float boxRight = BoxMaxX(box);
            const bool crossedRightFace =
                previousMinX >= boxRight - kSurfaceEpsilon && proposedMinX <= boxRight + kSurfaceEpsilon;
            if (!crossedRightFace)
            {
                return;
            }
            if (!hit || boxRight > nearestFaceX)
            {
                hit = true;
                nearestFaceX = boxRight;
            }
        }
    });

    if (!hit)
    {
        return proposedPosition.x;
    }

    if (displacementX > 0.0f)
    {
        return nearestFaceX - halfX;
    }
    return nearestFaceX + halfX;
}

SupportContact ResolveGroundContact(
    core::Vec3 previousPosition,
    core::Vec3 proposedPosition,
    core::Vec3 size,
    float verticalVelocity)
{
    SupportContact result;
    result.positionY = proposedPosition.y;
    result.verticalVelocity = verticalVelocity;
    result.grounded = false;

    if (verticalVelocity > 0.0f)
    {
        return result;
    }

    const float halfX = size.x * 0.5f;
    const float halfY = size.y * 0.5f;
    const float halfZ = size.z * 0.5f;
    const float previousBottom = previousPosition.y - halfY;
    const float currentBottom = proposedPosition.y - halfY;
    const float playerMinX = proposedPosition.x - halfX;
    const float playerMaxX = proposedPosition.x + halfX;
    const float playerMinZ = proposedPosition.z - halfZ;
    const float playerMaxZ = proposedPosition.z + halfZ;

    bool found = false;
    float bestTop = 0.0f;

    ForEachSolidBox([&](const Box& box) {
        ConsiderSupportSurface(
            box,
            previousBottom,
            currentBottom,
            playerMinX,
            playerMaxX,
            playerMinZ,
            playerMaxZ,
            found,
            bestTop);
    });

    if (found)
    {
        result.grounded = true;
        result.verticalVelocity = 0.0f;
        result.positionY = bestTop + halfY;
    }

    return result;
}

CeilingContact ResolveCeilingContact(
    core::Vec3 previousPosition,
    core::Vec3 proposedPosition,
    core::Vec3 size,
    float verticalVelocity)
{
    CeilingContact result;
    result.positionY = proposedPosition.y;
    result.verticalVelocity = verticalVelocity;

    if (verticalVelocity <= 0.0f)
    {
        return result;
    }

    const float halfX = size.x * 0.5f;
    const float halfY = size.y * 0.5f;
    const float halfZ = size.z * 0.5f;
    const float previousTop = previousPosition.y + halfY;
    const float proposedTop = proposedPosition.y + halfY;
    const float playerMinX = proposedPosition.x - halfX;
    const float playerMaxX = proposedPosition.x + halfX;
    const float playerMinZ = proposedPosition.z - halfZ;
    const float playerMaxZ = proposedPosition.z + halfZ;

    bool hit = false;
    float nearestCeiling = 0.0f;

    ForEachSolidBox([&](const Box& box) {
        if (!OverlapsClosed(playerMinX, playerMaxX, BoxMinX(box), BoxMaxX(box)))
        {
            return;
        }
        if (!OverlapsClosed(playerMinZ, playerMaxZ, BoxMinZ(box), BoxMaxZ(box)))
        {
            return;
        }

        const float boxBottom = BottomY(box);
        const bool crossedUnderside =
            previousTop <= boxBottom + kSurfaceEpsilon && proposedTop >= boxBottom - kSurfaceEpsilon;
        if (!crossedUnderside)
        {
            return;
        }
        if (!hit || boxBottom < nearestCeiling)
        {
            hit = true;
            nearestCeiling = boxBottom;
        }
    });

    if (hit)
    {
        result.positionY = nearestCeiling - halfY;
        result.verticalVelocity = 0.0f;
    }

    return result;
}
}
