#pragma once

// Milestone 61: Item Pickup collection burst draw. Immediate-mode spheres
// only. Not a particle/FX framework.

#include "gameplay/ItemPickupCollectionFeedback.h"

namespace render
{
void DrawItemPickupCollectionFeedback(
    const gameplay::ItemPickupCollectionFeedbackState& state);
}
