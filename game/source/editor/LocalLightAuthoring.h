#pragma once

// Milestone 85.3 editor-only Point/Spot Light visualization and picking.
// Origin and range/cone are authored lighting state. Not gameplay geometry.

#include "core/Vec3.h"
#include "editor/DirectionalLightAuthoring.h"
#include "world/LocalLight.h"

namespace editor
{
inline constexpr core::Vec3 kLocalLightAuthoringPickSize{0.7f, 0.7f, 0.7f};

inline core::Vec3 LocalLightPickExtents()
{
    return kLocalLightAuthoringPickSize;
}

inline core::Vec3 RotateAuthoredSpotDirection(core::Vec3 direction, core::Vec3 axis, float degrees)
{
    return RotateAuthoredDirectionalRay(
        world::CanonicalSpotLightDirection(direction), axis, degrees);
}

inline bool TryCommitAuthoredSpotDirection(core::Vec3 proposed, core::Vec3& current)
{
    if (!world::SpotLightDirectionIsValid(proposed))
    {
        return false;
    }
    current = world::CanonicalSpotLightDirection(proposed);
    return true;
}
}
