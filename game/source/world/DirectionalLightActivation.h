#pragma once

// Milestone 85.2: resolve authored Directional Light enablement against
// transient Pressure Plate overlap. Not an event bus, receiver list, or
// generic Source/Target framework.

#include "world/PressurePlate.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace world
{
struct DirectionalLightActivation
{
    bool hasLinkedPressurePlates = false;
    bool anyLinkedPressurePlateActive = false;
};

inline DirectionalLightActivation ResolveDirectionalLightActivation(
    const std::vector<PressurePlateSpec>& plates,
    const std::uint8_t* plateActive,
    std::size_t plateActiveCount)
{
    DirectionalLightActivation activation{};
    for (std::size_t index = 0; index < plates.size(); ++index)
    {
        if (!plates[index].controlsDirectionalLight)
        {
            continue;
        }
        activation.hasLinkedPressurePlates = true;
        if (index < plateActiveCount && plateActive != nullptr && plateActive[index] != 0)
        {
            activation.anyLinkedPressurePlateActive = true;
        }
    }
    return activation;
}

// effectiveEnabled = authoredEnabled && (noLinkedPlates || anyLinkedPlateActive)
inline bool AuthoredDirectionalLightIsEffectivelyEnabled(
    bool authoredEnabled,
    const DirectionalLightActivation& activation)
{
    return authoredEnabled
        && (!activation.hasLinkedPressurePlates || activation.anyLinkedPressurePlateActive);
}
}
