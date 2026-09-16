#include "render/ItemPickupCollectionFeedbackDraw.h"

#include "render/StaticModelScene.h"

#include "raylib.h"
#include "rlgl.h"

namespace render
{
namespace
{
Vector3 ToRaylib(core::Vec3 value)
{
    return Vector3{value.x, value.y, value.z};
}
}

void DrawItemPickupCollectionFeedback(const gameplay::ItemPickupCollectionFeedbackState& state)
{
    if (gameplay::ActiveItemPickupCollectionEffectCount(state) == 0)
    {
        return;
    }

    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ALPHA);
    for (int effectIndex = 0; effectIndex < gameplay::kItemPickupCollectionEffectCapacity;
         ++effectIndex)
    {
        const gameplay::ItemPickupCollectionEffect& effect = state.effects[effectIndex];
        if (!effect.active)
        {
            continue;
        }
        const unsigned char alpha = gameplay::ItemPickupCollectionEffectAlpha(effect);
        if (alpha == 0)
        {
            continue;
        }
        const Color gold{
            gameplay::kItemPickupCollectionGoldRed,
            gameplay::kItemPickupCollectionGoldGreen,
            gameplay::kItemPickupCollectionGoldBlue,
            alpha};
        const float coreRadius = gameplay::ItemPickupCollectionCoreDrawRadius(effect);
        if (coreRadius > 0.0f)
        {
            DrawSphere(ToRaylib(effect.origin), coreRadius, gold);
        }
        const float sparkRadius = gameplay::ItemPickupCollectionSparkDrawRadius(effect);
        for (int sparkIndex = 0; sparkIndex < gameplay::kItemPickupCollectionSparkCount;
             ++sparkIndex)
        {
            const core::Vec3 sparkPosition =
                gameplay::ItemPickupCollectionSparkWorldPosition(effect, sparkIndex);
            DrawSphere(ToRaylib(sparkPosition), sparkRadius, gold);
        }
    }
    EndBlendMode();
    rlDrawRenderBatchActive();
    RestoreGreyboxImmediateState();
}
}
