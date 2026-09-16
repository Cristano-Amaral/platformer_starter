// Focused Milestone 61 Item Pickup collection feedback. Not shipped.
// Trigger only after TryCollectItemPickup succeeds. No second targeting path.

#include "gameplay/Inventory.h"
#include "gameplay/ItemPickupCollectionFeedback.h"
#include "gameplay/ItemPickupRuntime.h"
#include "render/ItemPickupTargetHighlight.h"
#include "world/ItemPickup.h"

#include <cmath>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name.c_str());
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b, float tolerance = 1.0e-4f)
{
    return std::fabs(a - b) <= tolerance;
}

bool Vec3Near(core::Vec3 a, core::Vec3 b, float tolerance = 1.0e-4f)
{
    return NearlyEqual(a.x, b.x, tolerance) && NearlyEqual(a.y, b.y, tolerance)
        && NearlyEqual(a.z, b.z, tolerance);
}

world::ItemPickupSpec MakePickup(core::Vec3 position, std::string_view itemId = "key")
{
    world::ItemPickupSpec spec{};
    spec.position = position;
    spec.itemId = std::string(itemId);
    spec.quantity = 1;
    return spec;
}

bool CollectThenMaybeFeedback(
    gameplay::Inventory& inventory,
    gameplay::ItemPickupRunState& runState,
    gameplay::ItemPickupCollectionFeedbackState& feedback,
    std::span<const world::ItemPickupSpec> pickups,
    int index,
    double elapsedSeconds)
{
    if (!gameplay::TryCollectItemPickup(inventory, runState, pickups, index))
    {
        return false;
    }
    return gameplay::SpawnItemPickupCollectionFeedback(
        feedback, pickups[static_cast<std::size_t>(index)], elapsedSeconds);
}
}

int main()
{
    const core::Vec3 nearby{1.55f, 1.0f, 0.0f};
    const core::Vec3 playerCenter{0.0f, 1.0f, 0.0f};

    Expect(
        gameplay::kItemPickupCollectionEffectDurationSeconds >= 0.25f
            && gameplay::kItemPickupCollectionEffectDurationSeconds <= 0.60f,
        "duration is in the 0.25-0.60 s band");
    Expect(gameplay::kItemPickupCollectionEffectCapacity >= 1, "capacity is bounded and non-zero");
    Expect(gameplay::kItemPickupCollectionSparkCount >= 1, "spark count is bounded and non-zero");

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(
            CollectThenMaybeFeedback(inventory, run, feedback, pickups, 0, 0.0),
            "1. successful collection emits feedback");
        Expect(feedback.emittedCount == 1, "1. exactly one feedback event");
        Expect(
            gameplay::ActiveItemPickupCollectionEffectCount(feedback) == 1,
            "1. one active effect");
        Expect(inventory.GetQuantity("key") == 1, "inventory increments once");
        Expect(run.collected[0] == 1, "pickup marked collected");
        Expect(Vec3Near(feedback.effects[0].origin, nearby), "4. origin uses presented position");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", gameplay::kMaxItemQuantity), "seed inventory at cap");
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(
            !CollectThenMaybeFeedback(inventory, run, feedback, pickups, 0, 0.0),
            "2. failed TryAdd emits none");
        Expect(feedback.emittedCount == 0, "2. no feedback event on failed add");
        Expect(
            gameplay::ActiveItemPickupCollectionEffectCount(feedback) == 0,
            "2. no active effect on failed add");
        Expect(run.collected[0] == 0, "failed add leaves pickup available");
        Expect(inventory.GetQuantity("key") == gameplay::kMaxItemQuantity, "failed add does not mutate");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(
            !CollectThenMaybeFeedback(
                inventory, run, feedback, pickups, gameplay::kNoItemPickupIndex, 0.0),
            "3. untargeted collect emits none");
        Expect(feedback.emittedCount == 0, "3. untargeted has no event");
        Expect(
            CollectThenMaybeFeedback(inventory, run, feedback, pickups, 0, 0.0),
            "collect once");
        Expect(feedback.emittedCount == 1, "first success emits once");
        Expect(
            !CollectThenMaybeFeedback(inventory, run, feedback, pickups, 0, 0.0),
            "3. already-collected emits none");
        Expect(feedback.emittedCount == 1, "3. second collect does not emit");
        Expect(inventory.GetQuantity("key") == 1, "collected quantity stays 1");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        pickup.visualOffset = {0.25f, 0.5f, -0.1f};
        pickup.idleAnimationEnabled = true;
        pickup.idleBobAmplitude = 0.2f;
        pickup.idleBobSpeed = 1.0f;
        const double elapsed = 0.25;
        const core::Vec3 presented = world::ItemPickupPresentedVisualPosition(pickup, elapsed);
        Expect(!Vec3Near(presented, pickup.position), "5. presented differs from logical position");
        Expect(
            NearlyEqual(presented.x, pickup.position.x + pickup.visualOffset.x)
                && NearlyEqual(presented.z, pickup.position.z + pickup.visualOffset.z),
            "5. visualOffset is in the origin");
        Expect(
            !NearlyEqual(presented.y, pickup.position.y + pickup.visualOffset.y),
            "5. enabled idle bob displaces Y");

        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(
            CollectThenMaybeFeedback(inventory, run, feedback, pickups, 0, elapsed),
            "spawn at presented origin");
        Expect(Vec3Near(feedback.effects[0].origin, presented), "4/5. captured presented origin");
        Expect(
            !Vec3Near(feedback.effects[0].origin, pickup.position),
            "origin is not logical position");

        const std::vector<std::uint8_t> los{0};
        const int target = gameplay::FindItemPickupTargetIndex(
            playerCenter, 1.0f, pickups, run.collected, los);
        Expect(target == gameplay::kNoItemPickupIndex, "collected pickup is not targeted");

        gameplay::ItemPickupRunState available = gameplay::MakeClearedItemPickupRunState(1);
        const int availableTarget = gameplay::FindItemPickupTargetIndex(
            playerCenter, 1.0f, pickups, available.collected, los);
        Expect(availableTarget == 0, "6. targeting still uses logical position");
        const render::ItemPickupTargetPresentation presentation =
            render::MakeItemPickupTargetPresentation(
                pickup, true, false, false, {}, {}, elapsed);
        Expect(presentation.drawHud, "14. HUD still draws for the current target");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        pickup.visualOffset = {0.0f, 0.4f, 0.0f};
        pickup.idleAnimationEnabled = false;
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(
            CollectThenMaybeFeedback(inventory, run, feedback, pickups, 0, 1.5),
            "idle-off spawn");
        Expect(
            Vec3Near(feedback.effects[0].origin, world::ItemPickupVisualPosition(pickup)),
            "idle disabled origin is position + visualOffset");
    }

    {
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        world::ItemPickupSpec pickup = MakePickup(nearby);
        Expect(gameplay::SpawnItemPickupCollectionFeedback(feedback, pickup, 0.0), "first spawn");
        gameplay::UpdateItemPickupCollectionFeedback(
            feedback, gameplay::kItemPickupCollectionEffectDurationSeconds);
        Expect(
            gameplay::ActiveItemPickupCollectionEffectCount(feedback) == 0,
            "7. expired effect is removed");
        Expect(feedback.emittedCount == 1, "expire does not forget the emit count until clear");

        for (int n = 0; n < gameplay::kItemPickupCollectionEffectCapacity + 3; ++n)
        {
            pickup.position.x = nearby.x + static_cast<float>(n);
            Expect(
                gameplay::SpawnItemPickupCollectionFeedback(feedback, pickup, 0.0),
                "capacity spawn reuses slots");
        }
        Expect(
            gameplay::ActiveItemPickupCollectionEffectCount(feedback)
                <= gameplay::kItemPickupCollectionEffectCapacity,
            "7. active effects stay within capacity");
        Expect(
            feedback.emittedCount == 1 + gameplay::kItemPickupCollectionEffectCapacity + 3,
            "reuse still counts each successful emit");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(CollectThenMaybeFeedback(inventory, run, feedback, pickups, 0, 0.0), "before restart");
        Expect(gameplay::ActiveItemPickupCollectionEffectCount(feedback) == 1, "effect live");
        gameplay::ClearItemPickupCollectionFeedback(feedback);
        run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(
            gameplay::ActiveItemPickupCollectionEffectCount(feedback) == 0
                && feedback.emittedCount == 0,
            "8. Restart/Apply/reload clear active effects");
        Expect(gameplay::ItemPickupIsAvailable(run, 0), "restart restores pickup");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(
            CollectThenMaybeFeedback(inventory, run, feedback, pickups, 0, 0.0),
            "collect before checkpoint");
        const int emitted = feedback.emittedCount;
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::CheckpointRespawn);
        Expect(run.collected[0] == 1, "9. checkpoint preserves collected");
        Expect(feedback.emittedCount == emitted, "9. checkpoint does not replay feedback");
        Expect(
            gameplay::ActiveItemPickupCollectionEffectCount(feedback) == 1,
            "checkpoint does not fabricate extra effects");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild);
        Expect(feedback.emittedCount == emitted, "10. rebuild does not fabricate effects");
        Expect(run.collected[0] == 1, "rebuild preserves collected");
    }

    {
        world::ItemPickupSpec modeled = MakePickup(nearby);
        modeled.modelIdentity = "models/test_static.glb";
        world::ItemPickupSpec fallback = MakePickup({2.55f, 1.0f, 0.0f});
        world::ItemPickupSpec missing = MakePickup({3.55f, 1.0f, 0.0f});
        missing.modelIdentity = "models/missing_pickup.glb";
        Expect(world::ItemPickupSpecIsValid(modeled), "13. model-backed spec valid");
        Expect(world::ItemPickupSpecIsValid(fallback), "13. fallback spec valid");
        Expect(world::ItemPickupSpecIsValid(missing), "13. missing-model identity can be valid");

        std::vector<world::ItemPickupSpec> pickups{modeled, fallback, missing};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(3);
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(
            CollectThenMaybeFeedback(inventory, run, feedback, pickups, 0, 0.0),
            "13. model-backed collect feedback");
        Expect(
            CollectThenMaybeFeedback(inventory, run, feedback, pickups, 1, 0.0),
            "13. fallback collect feedback");
        Expect(
            CollectThenMaybeFeedback(inventory, run, feedback, pickups, 2, 0.0),
            "13. missing-model collect feedback");
        Expect(feedback.emittedCount == 3, "13. one event per successful collect");
        Expect(
            gameplay::ActiveItemPickupCollectionEffectCount(feedback) == 3,
            "13. three independent effects");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        pickup.visualOffset = {0.5f, 0.0f, 0.0f};
        const std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::ItemPickupRunState available = gameplay::MakeClearedItemPickupRunState(1);
        const std::vector<std::uint8_t> los{0};
        const int target = gameplay::FindItemPickupTargetIndex(
            playerCenter, 1.0f, pickups, available.collected, los);
        Expect(target == 0, "14. range/facing targeting unchanged");
        Expect(
            gameplay::kItemPickupMaxDistance == 2.5f
                && gameplay::kItemPickupMinFacingDot == 0.35f,
            "14. M55 range/facing constants unchanged");
    }

    {
        Expect(
            gameplay::kItemPickupCollectionGoldRed == 255
                && gameplay::kItemPickupCollectionGoldGreen == 220
                && gameplay::kItemPickupCollectionGoldBlue == 72,
            "gold matches existing pickup gold");
        Expect(gameplay::kItemPickupCollectionEffectRadius > 0.0f, "modest world-space radius");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d ItemPickupCollectionFeedbackTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("ItemPickupCollectionFeedbackTest passed\n");
    return 0;
}
