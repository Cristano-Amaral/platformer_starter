#include "world/Collision.h"

#include "world/GreyboxWorld.h"

namespace world
{
namespace
{
// Post-snap feet are reconstructed as (top + halfHeight) - halfHeight, which can sit
// one ulp below the surface. That must still count as support, not a lost crossing.
constexpr float kSurfaceEpsilon = 0.001f;

bool OverlapsClosed(float aMin, float aMax, float bMin, float bMax)
{
    return aMin <= bMax && bMin <= aMax;
}

void ConsiderSurface(
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

    // Crossing from above: previous feet at/above the surface, current feet at/below.
    // Stable support: the same test, with epsilon so a snapped pose is not treated as
    // "started below" on the following frame.
    const bool startedAtOrAbove = previousBottom >= top - kSurfaceEpsilon;
    const bool nowAtOrBelow = currentBottom <= top + kSurfaceEpsilon;
    if (!startedAtOrAbove || !nowAtOrBelow)
    {
        return;
    }

    const float halfX = box.size.x * 0.5f;
    const float halfZ = box.size.z * 0.5f;
    if (!OverlapsClosed(playerMinX, playerMaxX, box.center.x - halfX, box.center.x + halfX))
    {
        return;
    }
    if (!OverlapsClosed(playerMinZ, playerMaxZ, box.center.z - halfZ, box.center.z + halfZ))
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

SupportContact ResolveGroundContact(
    core::Vec3 previousPosition,
    core::Vec3 currentPosition,
    core::Vec3 size,
    float verticalVelocity)
{
    SupportContact result;
    result.positionY = currentPosition.y;
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
    const float currentBottom = currentPosition.y - halfY;
    const float playerMinX = currentPosition.x - halfX;
    const float playerMaxX = currentPosition.x + halfX;
    const float playerMinZ = currentPosition.z - halfZ;
    const float playerMaxZ = currentPosition.z + halfZ;

    bool found = false;
    float bestTop = 0.0f;

    ConsiderSurface(
        kGround,
        previousBottom,
        currentBottom,
        playerMinX,
        playerMaxX,
        playerMinZ,
        playerMaxZ,
        found,
        bestTop);

    for (const Box& platform : kElevatedPlatforms)
    {
        ConsiderSurface(
            platform,
            previousBottom,
            currentBottom,
            playerMinX,
            playerMaxX,
            playerMinZ,
            playerMaxZ,
            found,
            bestTop);
    }

    if (found)
    {
        result.grounded = true;
        result.verticalVelocity = 0.0f;
        result.positionY = bestTop + halfY;
    }

    return result;
}
}
