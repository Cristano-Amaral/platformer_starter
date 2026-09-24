// Focused Milestone 62 Item Pickup collection HUD notification. Not shipped.
// Trigger only after TryCollectItemPickup succeeds. No second targeting path.

#include "gameplay/Inventory.h"
#include "gameplay/Equipment.h"
#include "gameplay/InventoryTestSupport.h"
#include "gameplay/ItemPickupCollectionFeedback.h"
#include "gameplay/ItemPickupCollectionHud.h"
#include "gameplay/ItemPickupRuntime.h"
#include "render/ItemPickupTargetHighlight.h"
#include "world/ItemPickup.h"

#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace
{
int gFailures = 0;
const gameplay::GameplayDefinitionRegistry registry = gameplay::MakeStandardTestItemRegistry();

void Expect(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name.c_str());
        ++gFailures;
    }
}

world::ItemPickupSpec MakePickup(
    core::Vec3 position,
    std::string_view itemId = "items/master_key",
    int quantity = 1)
{
    world::ItemPickupSpec spec{};
    spec.position = position;
    spec.itemId = std::string(itemId);
    spec.quantity = quantity;
    return spec;
}

bool CollectThenMaybeHud(
    gameplay::Inventory& inventory,
    gameplay::ItemPickupRunState& runState,
    gameplay::ItemPickupCollectionHudState& hud,
    gameplay::ItemPickupCollectionFeedbackState* feedback,
    std::span<const world::ItemPickupSpec> pickups,
    int index,
    double elapsedSeconds)
{
    if (!gameplay::TryCollectItemPickup(inventory, runState, pickups, index, registry))
    {
        return false;
    }
    if (feedback != nullptr)
    {
        (void)gameplay::SpawnItemPickupCollectionFeedback(
            *feedback, pickups[static_cast<std::size_t>(index)], elapsedSeconds);
    }
    return gameplay::SpawnItemPickupCollectionHud(
        hud, pickups[static_cast<std::size_t>(index)]);
}

bool TextEquals(const char* buffer, const char* expected)
{
    return buffer != nullptr && expected != nullptr && std::strcmp(buffer, expected) == 0;
}
}

int main()
{
    gameplay::Equipment equipment;

    const core::Vec3 nearby{1.55f, 1.0f, 0.0f};
    const core::Vec3 playerCenter{0.0f, 1.0f, 0.0f};

    Expect(
        gameplay::kItemPickupCollectionHudLifetimeSeconds >= 1.5f
            && gameplay::kItemPickupCollectionHudLifetimeSeconds <= 2.5f,
        "lifetime is in the 1.5-2.5 s band");
    Expect(
        gameplay::kItemPickupCollectionHudHoldSeconds > 0.0f
            && gameplay::kItemPickupCollectionHudHoldSeconds
                < gameplay::kItemPickupCollectionHudLifetimeSeconds,
        "hold is shorter than lifetime");
    Expect(
        gameplay::kItemPickupCollectionHudFadeSeconds > 0.0f,
        "fade window is positive");
    Expect(
        gameplay::kItemPickupCollectionHudCapacity >= 3
            && gameplay::kItemPickupCollectionHudCapacity <= 5,
        "capacity is a small bounded Item-Pickup list");

    {
        world::ItemPickupSpec pickup = MakePickup(nearby, "items/master_key", 2);
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionHudState hud{};
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(
            CollectThenMaybeHud(inventory, run, hud, &feedback, pickups, 0, 0.0),
            "1. successful collection emits one notification");
        Expect(hud.emittedCount == 1, "1. exactly one HUD emit");
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 1,
            "1. one active HUD entry");
        Expect(std::strcmp(hud.entries[0].itemId, "items/master_key") == 0, "3. itemId is key");
        Expect(hud.entries[0].quantity == 2, "3. quantity is 2");
        char text[gameplay::kItemPickupCollectionHudTextCapacity]{};
        gameplay::FormatItemPickupCollectionHudText(text, sizeof(text), "items/master_key", 2);
        Expect(TextEquals(text, "Picked Up items/master_key x2"), "3. exact HUD text format");
        Expect(feedback.emittedCount == 1, "10. M61 burst still emits once");
        Expect(
            gameplay::ActiveItemPickupCollectionEffectCount(feedback) == 1,
            "10. M61 one active burst");
        Expect(inventory.GetQuantity("items/master_key") == 2, "11. inventory increments once");
        Expect(run.collected[0] == 1, "11. pickup marked collected");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby, "items/coin");
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/coin", gameplay::kMaxItemQuantity), "seed inventory at cap");
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionHudState hud{};
        gameplay::ItemPickupCollectionFeedbackState feedback{};
        Expect(
            !CollectThenMaybeHud(inventory, run, hud, &feedback, pickups, 0, 0.0),
            "2. failed TryAdd emits none");
        Expect(hud.emittedCount == 0, "2. no HUD emit on failed add");
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 0,
            "2. no active HUD on failed add");
        Expect(feedback.emittedCount == 0, "10. failed add emits no M61 burst");
        Expect(run.collected[0] == 0, "11. failed add leaves pickup");
        Expect(
            inventory.GetQuantity("items/coin") == gameplay::kMaxItemQuantity,
            "11. failed add does not mutate inventory");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionHudState hud{};
        Expect(
            !CollectThenMaybeHud(
                inventory, run, hud, nullptr, pickups, gameplay::kNoItemPickupIndex, 0.0),
            "2. untargeted collect emits none");
        Expect(hud.emittedCount == 0, "2. untargeted has no HUD");
        Expect(CollectThenMaybeHud(inventory, run, hud, nullptr, pickups, 0, 0.0), "collect once");
        Expect(hud.emittedCount == 1, "first success emits once");
        Expect(
            !CollectThenMaybeHud(inventory, run, hud, nullptr, pickups, 0, 0.0),
            "2. already-collected emits none");
        Expect(hud.emittedCount == 1, "2. second collect does not emit HUD");
        Expect(inventory.GetQuantity("items/master_key") == 1, "11. collected quantity stays 1");
    }

    {
        world::ItemPickupSpec first = MakePickup(nearby, "items/coin", 1);
        world::ItemPickupSpec second = MakePickup({2.55f, 1.0f, 0.0f}, "items/master_key", 3);
        world::ItemPickupSpec third = MakePickup({3.55f, 1.0f, 0.0f}, "items/battery", 1);
        std::vector<world::ItemPickupSpec> pickups{first, second, third};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(3);
        gameplay::ItemPickupCollectionHudState hud{};
        Expect(CollectThenMaybeHud(inventory, run, hud, nullptr, pickups, 0, 0.0), "rapid coin");
        Expect(CollectThenMaybeHud(inventory, run, hud, nullptr, pickups, 1, 0.0), "rapid key");
        Expect(CollectThenMaybeHud(inventory, run, hud, nullptr, pickups, 2, 0.0), "rapid battery");
        Expect(hud.emittedCount == 3, "4. three rapid successes emit three");
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 3,
            "4. three independent HUD entries");
        Expect(std::strcmp(hud.entries[0].itemId, "items/coin") == 0, "4. oldest is first append");
        Expect(std::strcmp(hud.entries[1].itemId, "items/master_key") == 0, "4. middle keeps order");
        Expect(std::strcmp(hud.entries[2].itemId, "items/battery") == 0, "4. newest is last append");
        Expect(hud.entries[1].quantity == 3, "4. middle quantity preserved");
    }

    {
        gameplay::ItemPickupCollectionHudState hud{};
        const int extra = 3;
        const int total = gameplay::kItemPickupCollectionHudCapacity + extra;
        for (int n = 0; n < total; ++n)
        {
            char itemId[2]{static_cast<char>('a' + n), '\0'};
            Expect(
                gameplay::SpawnItemPickupCollectionHud(hud, itemId, n + 1),
                "5. overflow spawn");
        }
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud)
                == gameplay::kItemPickupCollectionHudCapacity,
            "5. active entries stay at capacity");
        Expect(hud.emittedCount == total, "5. overflow still counts each emit");
        char oldestId[2]{static_cast<char>('a' + extra), '\0'};
        char newestId[2]{static_cast<char>('a' + total - 1), '\0'};
        Expect(std::strcmp(hud.entries[0].itemId, oldestId) == 0, "5. oldest dropped deterministically");
        Expect(
            std::strcmp(
                hud.entries[gameplay::kItemPickupCollectionHudCapacity - 1].itemId, newestId)
                == 0,
            "5. newest is last remaining");
        Expect(hud.entries[0].quantity == extra + 1, "5. surviving oldest keeps its quantity");
    }

    {
        gameplay::ItemPickupCollectionHudState hud{};
        Expect(gameplay::SpawnItemPickupCollectionHud(hud, "key", 1), "spawn for expire");
        Expect(
            gameplay::ItemPickupCollectionHudAlpha(hud.entries[0]) == 255,
            "6. hold keeps full alpha");
        gameplay::UpdateItemPickupCollectionHud(hud, 0.0f);
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 1
                && hud.entries[0].ageSeconds == 0.0f,
            "timing authority: non-positive delta freezes aging");
        gameplay::UpdateItemPickupCollectionHud(
            hud, gameplay::kItemPickupCollectionHudHoldSeconds);
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 1,
            "6. still live at end of hold");
        Expect(
            gameplay::ItemPickupCollectionHudAlpha(hud.entries[0]) == 255,
            "6. alpha still full at hold");
        gameplay::UpdateItemPickupCollectionHud(hud, gameplay::kItemPickupCollectionHudFadeSeconds * 0.5f);
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 1,
            "6. fade window keeps the entry");
        Expect(
            gameplay::ItemPickupCollectionHudAlpha(hud.entries[0]) < 255
                && gameplay::ItemPickupCollectionHudAlpha(hud.entries[0]) > 0,
            "6. mid-fade alpha is partial");
        gameplay::UpdateItemPickupCollectionHud(
            hud, gameplay::kItemPickupCollectionHudFadeSeconds);
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 0,
            "6. expired entry is removed");
        Expect(hud.emittedCount == 1, "expire does not forget emit count until clear");
    }

    {
        gameplay::ItemPickupCollectionHudState hud{};
        Expect(gameplay::SpawnItemPickupCollectionHud(hud, "coin", 1), "older");
        gameplay::UpdateItemPickupCollectionHud(hud, 0.4f);
        Expect(gameplay::SpawnItemPickupCollectionHud(hud, "key", 1), "newer");
        gameplay::UpdateItemPickupCollectionHud(
            hud, gameplay::kItemPickupCollectionHudLifetimeSeconds - 0.4f);
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 1,
            "6. entries expire independently");
        Expect(std::strcmp(hud.entries[0].itemId, "key") == 0, "6. surviving entry is the newer one");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionHudState hud{};
        Expect(CollectThenMaybeHud(inventory, run, hud, nullptr, pickups, 0, 0.0), "before restart");
        Expect(gameplay::ActiveItemPickupCollectionHudCount(hud) == 1, "HUD live");
        gameplay::ClearItemPickupCollectionHud(hud);
        run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 0 && hud.emittedCount == 0,
            "7. Restart/Apply/reload clear HUD entries");
        Expect(gameplay::ItemPickupIsAvailable(run, 0), "restart restores pickup");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::ItemPickupCollectionHudState hud{};
        Expect(
            CollectThenMaybeHud(inventory, run, hud, nullptr, pickups, 0, 0.0),
            "collect before checkpoint");
        const int emitted = hud.emittedCount;
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::CheckpointRespawn);
        Expect(run.collected[0] == 1, "8. checkpoint preserves collected");
        Expect(hud.emittedCount == emitted, "8. checkpoint does not replay HUD");
        Expect(
            gameplay::ActiveItemPickupCollectionHudCount(hud) == 1,
            "8. checkpoint does not fabricate extra HUD");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild);
        Expect(hud.emittedCount == emitted, "8. rebuild does not fabricate HUD");
        Expect(run.collected[0] == 1, "rebuild preserves collected");
    }

    {
        world::ItemPickupSpec pickup = MakePickup(nearby);
        pickup.visualOffset = {0.5f, 0.0f, 0.0f};
        const std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::ItemPickupRunState available = gameplay::MakeClearedItemPickupRunState(1);
        const std::vector<std::uint8_t> los{0};
        const int target = gameplay::FindItemPickupTargetIndex(
            playerCenter, 1.0f, pickups, available.collected, los);
        Expect(target == 0, "9. range/facing targeting unchanged");
        const render::ItemPickupTargetPresentation presentation =
            render::MakeItemPickupTargetPresentation(pickup, true, false, false, {}, {}, 0.0);
        Expect(presentation.drawHud, "9. target HUD still draws for the current target");
        Expect(
            gameplay::kItemPickupMaxDistance == 2.5f
                && gameplay::kItemPickupMinFacingDot == 0.35f,
            "11. M55 range/facing constants unchanged");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d ItemPickupCollectionHudTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("ItemPickupCollectionHudTest passed\n");
    return 0;
}
