#pragma once

// Narrow Pressure Plate Inspector helpers for M85.4 local-light targets.
// Extracted from the Development ImGui path so add/remove/duplicate policy
// can be regression-tested without ImGui. Not a generic receiver picker.

#include "world/PressurePlate.h"

#include <cstddef>

namespace editor
{
enum class PressurePlateLocalLightTargetEditResult
{
    Unchanged,
    Added,
    Removed,
};

inline PressurePlateLocalLightTargetEditResult TryAddPressurePlateLocalLightTarget(
    world::PressurePlateSpec& plate,
    world::LocalLightTarget target,
    std::size_t pointCount,
    std::size_t spotCount)
{
    if (!world::LocalLightTargetIsValid(target, pointCount, spotCount)
        || world::PressurePlateHasLocalLightTarget(plate, target))
    {
        return PressurePlateLocalLightTargetEditResult::Unchanged;
    }
    plate.controlledLocalLights.push_back(target);
    return PressurePlateLocalLightTargetEditResult::Added;
}

inline PressurePlateLocalLightTargetEditResult TryRemovePressurePlateLocalLightTargetAt(
    world::PressurePlateSpec& plate,
    std::size_t targetIndex)
{
    if (targetIndex >= plate.controlledLocalLights.size())
    {
        return PressurePlateLocalLightTargetEditResult::Unchanged;
    }
    plate.controlledLocalLights.erase(
        plate.controlledLocalLights.begin()
        + static_cast<std::ptrdiff_t>(targetIndex));
    return PressurePlateLocalLightTargetEditResult::Removed;
}
}
