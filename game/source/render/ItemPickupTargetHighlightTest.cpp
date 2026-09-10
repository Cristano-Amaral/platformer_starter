// Milestone 58.3: gameplay Item Pickup target highlight presentation.
// Consumes the M55 target result. No window, no second target search.

#include "gameplay/Inventory.h"
#include "gameplay/ItemPickupRuntime.h"
#include "render/ItemPickupTargetHighlight.h"
#include "world/ItemPickup.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* what)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", what);
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

world::ItemPickupSpec MakePickup(
    core::Vec3 position,
    const char* itemId = "key",
    const char* model = "")
{
    world::ItemPickupSpec spec{};
    spec.position = position;
    spec.itemId = itemId;
    spec.quantity = 1;
    spec.modelIdentity = model;
    spec.visualOffset = {0.25f, 0.5f, -0.1f};
    spec.visualRotationDegrees = {10.0f, 45.0f, 5.0f};
    spec.visualScale = {0.5f, 1.25f, 0.75f};
    return spec;
}
}

int main()
{
    Expect(
        world::ItemPickupSpec{}.showInteractionBounds
            == world::kDefaultItemPickupShowInteractionBounds
            && world::kDefaultItemPickupShowInteractionBounds,
        "1. showInteractionBounds defaults true");

    const world::ItemPickupSpec modeled = MakePickup({2.0f, 1.0f, 0.0f}, "key", "models/test_static.glb");
    const world::ItemPickupSpec fallback = []() {
        world::ItemPickupSpec spec = MakePickup({2.0f, 1.0f, 0.0f});
        spec.modelIdentity.clear();
        return spec;
    }();

    const render::ItemPickupTargetPresentation idle =
        render::MakeItemPickupTargetPresentation(
            modeled, false, false, false, {}, {});
    Expect(!idle.drawModelHighlight, "14. non-targeted model has no gameplay highlight");
    Expect(!idle.drawHud, "non-targeted pickup has no HUD");
    Expect(!idle.drawInteractionBounds, "non-targeted pickup has no target bounds");
    Expect(idle.modelBacked, "modeled pickup is model-backed");
    Expect(Vec3Near(idle.visual.position, world::ItemPickupVisualPosition(modeled)),
        "16. presentation origin is position + visualOffset");
    Expect(Vec3Near(idle.visual.rotationDegrees, modeled.visualRotationDegrees),
        "17. presentation uses visualRotationDegrees");
    Expect(Vec3Near(idle.visual.scale, modeled.visualScale),
        "18. presentation uses visualScale");
    Expect(idle.visual.modelIdentity == modeled.modelIdentity,
        "19. presentation reuses the authored cached identity");

    const render::ItemPickupTargetPresentation targeted =
        render::MakeItemPickupTargetPresentation(
            modeled, true, false, false, {}, {});
    Expect(targeted.drawModelHighlight, "15. targeted model receives model highlight");
    Expect(targeted.drawHud, "targeted model keeps HUD");
    Expect(targeted.drawInteractionBounds, "21. showInteractionBounds true draws target bounds");
    Expect(!targeted.drawFallbackHighlight, "model-backed target does not use cube fill highlight");

    world::ItemPickupSpec boundsOff = modeled;
    boundsOff.showInteractionBounds = false;
    const render::ItemPickupTargetPresentation hiddenBounds =
        render::MakeItemPickupTargetPresentation(
            boundsOff, true, false, false, {}, {});
    Expect(hiddenBounds.drawModelHighlight, "23. false does not suppress model highlight");
    Expect(hiddenBounds.drawHud, "24. false does not suppress HUD");
    Expect(!hiddenBounds.drawInteractionBounds, "22. showInteractionBounds false suppresses bounds");

    const render::ItemPickupTargetPresentation collected =
        render::MakeItemPickupTargetPresentation(
            modeled, true, true, false, {}, {});
    Expect(!collected.drawModelHighlight, "20. collected pickup receives no target highlight");
    Expect(!collected.drawHud, "collected pickup has no HUD");
    Expect(!collected.drawInteractionBounds, "collected pickup has no bounds");

    const core::Vec3 loadedMin{-2.0f, -1.0f, -0.5f};
    const core::Vec3 loadedMax{2.0f, 3.0f, 0.5f};
    const render::ItemPickupTargetPresentation loaded =
        render::MakeItemPickupTargetPresentation(
            modeled, true, false, true, loadedMin, loadedMax);
    Expect(Vec3Near(loaded.localMin, loadedMin), "18. loaded model-local min is used");
    Expect(Vec3Near(loaded.localMax, loadedMax), "loaded model-local max is used");
    Expect(
        Vec3Near(
            loaded.boundsCorners[0],
            render::item_pickup_highlight_detail::WorldFromLocal(
                loaded.visual, loadedMin)),
        "25. transformed bounds follow pickup visual transform");
    Expect(
        !Vec3Near(loaded.boundsCorners[0], modeled.position),
        "bounds are not a world-axis cube on logical position");

    const render::ItemPickupTargetPresentation proxyBounds =
        render::MakeItemPickupTargetPresentation(
            modeled, true, false, false, {}, {});
    Expect(
        Vec3Near(proxyBounds.localMin, render::kItemPickupModelFallbackLocalMin),
        "missing loaded bounds uses local fallback/proxy");
    Expect(
        Vec3Near(proxyBounds.localMax, render::kItemPickupModelFallbackLocalMax),
        "missing loaded bounds uses local fallback max");

    const render::ItemPickupTargetPresentation fallbackTarget =
        render::MakeItemPickupTargetPresentation(
            fallback, true, false, false, {}, {});
    Expect(fallbackTarget.drawFallbackHighlight, "27. no-model fallback remains highlighted");
    Expect(fallbackTarget.drawHud, "fallback target still shows HUD");
    Expect(!fallbackTarget.drawModelHighlight, "fallback does not request a model highlight");
    Expect(fallbackTarget.drawInteractionBounds, "fallback bounds follow the checkbox");
    Expect(Vec3Near(fallbackTarget.visual.position, fallback.position),
        "fallback target cube stays on logical position");

    world::ItemPickupSpec missing = modeled;
    missing.modelIdentity = "models/missing_staged.glb";
    const render::ItemPickupTargetPresentation missingTarget =
        render::MakeItemPickupTargetPresentation(
            missing, true, false, false, {}, {});
    Expect(missingTarget.drawModelHighlight, "28. missing staged model still highlights safely");
    Expect(missingTarget.visual.modelIdentity == "models/missing_staged.glb",
        "29. missing identity is unchanged; no source fallback");
    Expect(missingTarget.visual.modelIdentity.find("assets/source") == std::string::npos,
        "presentation never rewrites identity to assets/source");

    const core::Vec3 spawn{0.0f, 0.8f, 0.0f};
    world::ItemPickupSpec search = modeled;
    search.position = {spawn.x + 1.5f, spawn.y, spawn.z};
    search.visualOffset = {80.0f, 4.0f, 3.0f};
    search.showInteractionBounds = false;
    const std::vector<world::ItemPickupSpec> pickups{search};
    gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
    const std::vector<std::uint8_t> los{0};
    Expect(
        gameplay::FindItemPickupTargetIndex(spawn, 1.0f, pickups, run.collected, los) == 0,
        "26/30. target search remains based on logical position");
    Expect(gameplay::kItemPickupMaxDistance == 2.5f, "31. targeting range unchanged");
    Expect(gameplay::kItemPickupMinFacingDot == 0.35f, "32. facing unchanged");
    Expect(gameplay::kItemPickupLosBlockFraction == 0.98f, "33. LOS fraction unchanged");
    world::ItemPickupSpec tiedA = search;
    world::ItemPickupSpec tiedB = search;
    tiedB.itemId = "coin";
    const std::vector<world::ItemPickupSpec> tied{tiedA, tiedB};
    gameplay::ItemPickupRunState tiedRun = gameplay::MakeClearedItemPickupRunState(2);
    const std::vector<std::uint8_t> tiedLos{0, 0};
    Expect(
        gameplay::FindItemPickupTargetIndex(spawn, 1.0f, tied, tiedRun.collected, tiedLos) == 0,
        "34. deterministic nearest/tie keeps lower session index");
    gameplay::Inventory inventory;
    Expect(gameplay::TryCollectItemPickup(inventory, run, pickups, 0), "35. collection unchanged");
    Expect(inventory.GetQuantity("key") == 1, "36. Inventory unchanged");
    Expect(pickups[0].showInteractionBounds == false, "collection does not mutate bounds flag");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d item-pickup target highlight test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Item-pickup target highlight tests passed.\n");
    return 0;
}
