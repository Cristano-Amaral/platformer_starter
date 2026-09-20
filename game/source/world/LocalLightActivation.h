#pragma once

// Milestone 85.4: resolve authored Point/Spot enablement against transient
// Pressure Plate overlap. Not an event bus, receiver list, or generic
// Source/Target framework. Directional Light stays on the M85.2 path.

#include "world/LocalLight.h"
#include "world/PressurePlate.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace world
{
struct LocalLightActivation
{
    bool hasControllingPlates = false;
    bool anyControllingPlateActive = false;
};

inline LocalLightActivation ResolveLocalLightActivation(
    LocalLightKind kind,
    int index,
    const std::vector<PressurePlateSpec>& plates,
    const std::uint8_t* plateActive,
    std::size_t plateActiveCount)
{
    LocalLightActivation activation{};
    const LocalLightTarget wanted{kind, index};
    for (std::size_t plateIndex = 0; plateIndex < plates.size(); ++plateIndex)
    {
        if (!PressurePlateHasLocalLightTarget(plates[plateIndex], wanted))
        {
            continue;
        }
        activation.hasControllingPlates = true;
        if (plateIndex < plateActiveCount && plateActive != nullptr
            && plateActive[plateIndex] != 0)
        {
            activation.anyControllingPlateActive = true;
        }
    }
    return activation;
}

// effectiveEnabled = authoredEnabled && (noControllingPlates || anyControllingPlateActive)
inline bool AuthoredLocalLightIsEffectivelyEnabled(
    bool authoredEnabled,
    const LocalLightActivation& activation)
{
    return authoredEnabled
        && (!activation.hasControllingPlates || activation.anyControllingPlateActive);
}

inline void FillEffectiveLocalLightEnabled(
    const std::vector<PointLightSpec>& pointLights,
    const std::vector<SpotLightSpec>& spotLights,
    const std::vector<PressurePlateSpec>& plates,
    const std::uint8_t* plateActive,
    std::size_t plateActiveCount,
    std::vector<std::uint8_t>& outPointEnabled,
    std::vector<std::uint8_t>& outSpotEnabled)
{
    outPointEnabled.assign(pointLights.size(), 0);
    outSpotEnabled.assign(spotLights.size(), 0);
    for (std::size_t index = 0; index < pointLights.size(); ++index)
    {
        const LocalLightActivation activation = ResolveLocalLightActivation(
            LocalLightKind::Point,
            static_cast<int>(index),
            plates,
            plateActive,
            plateActiveCount);
        outPointEnabled[index] = AuthoredLocalLightIsEffectivelyEnabled(
                                     pointLights[index].enabled, activation)
            ? 1
            : 0;
    }
    for (std::size_t index = 0; index < spotLights.size(); ++index)
    {
        const LocalLightActivation activation = ResolveLocalLightActivation(
            LocalLightKind::Spot,
            static_cast<int>(index),
            plates,
            plateActive,
            plateActiveCount);
        outSpotEnabled[index] = AuthoredLocalLightIsEffectivelyEnabled(
                                    spotLights[index].enabled, activation)
            ? 1
            : 0;
    }
}
}
