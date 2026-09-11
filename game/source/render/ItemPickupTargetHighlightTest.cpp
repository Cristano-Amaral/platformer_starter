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
    Expect(
        world::ItemPickupSpec{}.targetHighlightIntensity
            == world::kDefaultItemPickupTargetHighlightIntensity
            && world::kDefaultItemPickupTargetHighlightIntensity == 0.70f,
        "1. targetHighlightIntensity defaults 0.70");
    Expect(
        render::ItemPickupTargetHighlightAlpha(0.70f) >= 178
            && render::ItemPickupTargetHighlightAlpha(0.70f) <= 180,
        "23. default 0.70 maps near M58.3 alpha 180");
    Expect(render::ItemPickupTargetHighlightAlpha(0.0f) == 0, "22. intensity 0 maps to alpha 0");
    Expect(
        world::ItemPickupSpec{}.targetHighlightGoldAmount
            == world::kDefaultItemPickupTargetHighlightGoldAmount
            && world::kDefaultItemPickupTargetHighlightGoldAmount == 0.70f,
        "5. targetHighlightGoldAmount defaults 0.70");

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
    Expect(hiddenBounds.drawModelHighlight, "30. bounds false + intensity default still highlights");

    world::ItemPickupSpec zeroIntensity = modeled;
    zeroIntensity.targetHighlightIntensity = 0.0f;
    const render::ItemPickupTargetPresentation zeroHighlight =
        render::MakeItemPickupTargetPresentation(
            zeroIntensity, true, false, false, {}, {});
    Expect(!zeroHighlight.drawModelHighlight, "22. intensity 0 suppresses model highlight");
    Expect(zeroHighlight.drawHud, "27. intensity 0 still shows HUD");
    Expect(zeroHighlight.drawInteractionBounds, "31. bounds true + intensity 0 still draws bounds");
    Expect(zeroHighlight.highlightAlpha == 0, "intensity 0 has no highlight contribution");
    Expect(Vec3Near(zeroHighlight.visual.position, world::ItemPickupVisualPosition(zeroIntensity)),
        "28. intensity does not change visual transform");

    world::ItemPickupSpec hudOnly = modeled;
    hudOnly.showInteractionBounds = false;
    hudOnly.targetHighlightIntensity = 0.0f;
    const render::ItemPickupTargetPresentation hudOnlyPresentation =
        render::MakeItemPickupTargetPresentation(
            hudOnly, true, false, false, {}, {});
    Expect(!hudOnlyPresentation.drawModelHighlight, "D. HUD-only has no model highlight");
    Expect(!hudOnlyPresentation.drawInteractionBounds, "D. HUD-only has no bounds");
    Expect(hudOnlyPresentation.drawHud, "32. bounds false + intensity 0 preserves HUD");

    world::ItemPickupSpec strongNoBounds = modeled;
    strongNoBounds.showInteractionBounds = false;
    strongNoBounds.targetHighlightIntensity = 0.90f;
    const render::ItemPickupTargetPresentation strong =
        render::MakeItemPickupTargetPresentation(
            strongNoBounds, true, false, false, {}, {});
    Expect(strong.drawModelHighlight, "30. bounds false + intensity 0.90 highlights");
    Expect(!strong.drawInteractionBounds, "bounds stay independent of intensity");
    Expect(strong.drawHud, "HUD remains at high intensity");
    Expect(strong.highlightAlpha == render::ItemPickupTargetHighlightAlpha(0.90f),
        "0.90 maps through the same alpha helper");

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
    world::ItemPickupSpec zeroCollect = search;
    zeroCollect.targetHighlightIntensity = 0.0f;
    const std::vector<world::ItemPickupSpec> zeroCollectPickups{zeroCollect};
    gameplay::ItemPickupRunState zeroRun = gameplay::MakeClearedItemPickupRunState(1);
    gameplay::Inventory zeroInventory;
    Expect(
        gameplay::FindItemPickupTargetIndex(
            spawn, 1.0f, zeroCollectPickups, zeroRun.collected, los)
            == 0,
        "33. intensity 0 preserves targeting");
    Expect(
        gameplay::TryCollectItemPickup(zeroInventory, zeroRun, zeroCollectPickups, 0),
        "34. intensity 0 preserves collection");
    Expect(zeroInventory.GetQuantity("key") == 1, "collection at intensity 0 still grants item");
    Expect(
        zeroCollectPickups[0].targetHighlightIntensity == 0.0f,
        "collection does not mutate targetHighlightIntensity");

    world::ItemPickupSpec fallbackZero = fallback;
    fallbackZero.targetHighlightIntensity = 0.0f;
    const render::ItemPickupTargetPresentation fallbackZeroPresentation =
        render::MakeItemPickupTargetPresentation(
            fallbackZero, true, false, false, {}, {});
    Expect(!fallbackZeroPresentation.drawFallbackHighlight,
        "43. fallback intensity 0 has no extra fill highlight");
    Expect(fallbackZeroPresentation.drawHud, "fallback intensity 0 still shows HUD");

    world::ItemPickupSpec goldZero = modeled;
    goldZero.targetHighlightIntensity = 0.80f;
    goldZero.targetHighlightGoldAmount = 0.0f;
    const render::ItemPickupTargetPresentation goldZeroPresentation =
        render::MakeItemPickupTargetPresentation(
            goldZero, true, false, false, {}, {});
    world::ItemPickupSpec goldOne = goldZero;
    goldOne.targetHighlightGoldAmount = 1.0f;
    const render::ItemPickupTargetPresentation goldOnePresentation =
        render::MakeItemPickupTargetPresentation(
            goldOne, true, false, false, {}, {});
    Expect(goldZeroPresentation.drawModelHighlight, "gold 0 at intensity 0.80 still draws the pass");
    Expect(goldOnePresentation.drawModelHighlight, "gold 1 at intensity 0.80 still draws the pass");
    Expect(
        goldZeroPresentation.highlightAlpha == goldOnePresentation.highlightAlpha
            && goldZeroPresentation.highlightAlpha
                == render::ItemPickupTargetHighlightAlpha(0.80f),
        "38. Intensity owns alpha; Gold Amount does not change alpha");
    Expect(
        goldZeroPresentation.highlightRed == 255
            && goldZeroPresentation.highlightGreen == 255
            && goldZeroPresentation.highlightBlue == 255,
        "25. Gold Amount 0 keeps white tint (original coloration)");
    Expect(
        goldOnePresentation.highlightRed == render::kItemPickupTargetGoldRed
            && goldOnePresentation.highlightGreen == render::kItemPickupTargetGoldGreen
            && goldOnePresentation.highlightBlue == render::kItemPickupTargetGoldBlue,
        "26. Gold Amount 1 is the existing target gold");
    Expect(
        goldZeroPresentation.highlightGreen != goldOnePresentation.highlightGreen
            || goldZeroPresentation.highlightBlue != goldOnePresentation.highlightBlue,
        "37. Gold Amount 0 materially differs from 1 at fixed Intensity");

    world::ItemPickupSpec intensityLow = modeled;
    intensityLow.targetHighlightIntensity = 0.20f;
    intensityLow.targetHighlightGoldAmount = 1.0f;
    world::ItemPickupSpec intensityHigh = intensityLow;
    intensityHigh.targetHighlightIntensity = 0.90f;
    const render::ItemPickupTargetPresentation intensityLowPresentation =
        render::MakeItemPickupTargetPresentation(
            intensityLow, true, false, false, {}, {});
    const render::ItemPickupTargetPresentation intensityHighPresentation =
        render::MakeItemPickupTargetPresentation(
            intensityHigh, true, false, false, {}, {});
    Expect(
        intensityLowPresentation.highlightAlpha
            != intensityHighPresentation.highlightAlpha,
        "38. Intensity changes overall contribution at fixed Gold Amount");
    Expect(
        intensityLowPresentation.highlightRed == intensityHighPresentation.highlightRed
            && intensityLowPresentation.highlightGreen == intensityHighPresentation.highlightGreen
            && intensityLowPresentation.highlightBlue == intensityHighPresentation.highlightBlue,
        "39. Gold Amount owns RGB; Intensity does not change gold RGB");

    world::ItemPickupSpec intensityZeroGoldOne = modeled;
    intensityZeroGoldOne.targetHighlightIntensity = 0.0f;
    intensityZeroGoldOne.targetHighlightGoldAmount = 1.0f;
    const render::ItemPickupTargetPresentation intensityZeroGold =
        render::MakeItemPickupTargetPresentation(
            intensityZeroGoldOne, true, false, false, {}, {});
    Expect(!intensityZeroGold.drawModelHighlight, "36. Intensity 0 suppresses extra model highlight");
    Expect(intensityZeroGold.drawHud, "41. HUD remains independent of Gold Amount");
    Expect(intensityZeroGold.highlightAlpha == 0, "24. Intensity 0 has no extra contribution");

    world::ItemPickupSpec boundsGold = modeled;
    boundsGold.showInteractionBounds = false;
    boundsGold.targetHighlightIntensity = 0.80f;
    boundsGold.targetHighlightGoldAmount = 1.0f;
    const render::ItemPickupTargetPresentation boundsGoldPresentation =
        render::MakeItemPickupTargetPresentation(
            boundsGold, true, false, false, {}, {});
    Expect(boundsGoldPresentation.drawModelHighlight, "40. bounds false + gold 1 still highlights");
    Expect(!boundsGoldPresentation.drawInteractionBounds, "40. showInteractionBounds stays independent");

    world::ItemPickupSpec idlePickup = modeled;
    idlePickup.idleAnimationEnabled = true;
    idlePickup.idleBobAmplitude = 0.15f;
    idlePickup.idleBobSpeed = 1.0f;
    idlePickup.idleSpinSpeedDegrees = 90.0f;
    const double idleTime = 0.25;
    const render::ItemPickupTargetPresentation idleVisual =
        render::MakeItemPickupTargetPresentation(
            idlePickup, true, false, false, {}, {}, idleTime);
    Expect(
        Vec3Near(
            idleVisual.visual.position,
            world::ItemPickupPresentedVisualPosition(idlePickup, idleTime)),
        "33. target highlight follows animated visual position");
    Expect(
        Vec3Near(
            idleVisual.visual.rotationDegrees,
            world::ItemPickupPresentedVisualRotationDegrees(idlePickup, idleTime)),
        "33. target highlight follows animated visual rotation");
    Expect(
        !Vec3Near(idleVisual.visual.position, world::ItemPickupVisualPosition(idlePickup)),
        "33. animated highlight is not the static authored origin");
    Expect(
        Vec3Near(
            idleVisual.boundsCorners[0],
            render::item_pickup_highlight_detail::WorldFromLocal(
                idleVisual.visual, idleVisual.localMin)),
        "34. target bounds follow animated visual");
    Expect(
        Vec3Near(idlePickup.position, modeled.position)
            && Vec3Near(idlePickup.visualOffset, modeled.visualOffset)
            && Vec3Near(idlePickup.visualRotationDegrees, modeled.visualRotationDegrees)
            && Vec3Near(idlePickup.visualScale, modeled.visualScale),
        "14. idle presentation does not mutate authored state");
    const world::StaticPropSpec authoredGhost = world::ItemPickupVisualProp(idlePickup);
    Expect(
        Vec3Near(authoredGhost.position, world::ItemPickupVisualPosition(idlePickup))
            && Vec3Near(authoredGhost.rotationDegrees, idlePickup.visualRotationDegrees),
        "35. editor ghost/gizmo origin stays on authored transform");
    Expect(
        !Vec3Near(authoredGhost.position, idleVisual.visual.position)
            || !Vec3Near(authoredGhost.rotationDegrees, idleVisual.visual.rotationDegrees),
        "35. authored ghost does not chase runtime animation");

    const render::ItemPickupTargetPresentation collectedIdle =
        render::MakeItemPickupTargetPresentation(
            idlePickup, true, true, false, {}, {}, idleTime);
    Expect(!collectedIdle.drawModelHighlight, "32. collected pickup does not render/animate");
    Expect(collectedIdle.visual.modelIdentity.empty(), "32. collected presentation is empty");

    world::ItemPickupSpec fallbackIdle = fallback;
    fallbackIdle.idleAnimationEnabled = true;
    fallbackIdle.idleBobAmplitude = 0.15f;
    fallbackIdle.idleBobSpeed = 1.0f;
    fallbackIdle.idleSpinSpeedDegrees = 45.0f;
    const render::ItemPickupTargetPresentation fallbackIdlePresentation =
        render::MakeItemPickupTargetPresentation(
            fallbackIdle, true, false, false, {}, {}, 0.25);
    Expect(fallbackIdlePresentation.drawFallbackHighlight, "43. no-model fallback highlight remains");
    Expect(
        NearlyEqual(
            fallbackIdlePresentation.visual.position.y,
            fallbackIdle.position.y + world::ItemPickupIdleBobOffsetY(fallbackIdle, 0.25)),
        "43. fallback idle bob is visual-only");
    Expect(
        NearlyEqual(
            fallbackIdlePresentation.visual.rotationDegrees.y,
            world::ItemPickupIdleSpinYDegrees(fallbackIdle, 0.25)),
        "43. fallback idle spin is visual-only");

    unsigned char mixRed = 0;
    unsigned char mixGreen = 0;
    unsigned char mixBlue = 0;
    render::ItemPickupTargetHighlightTint(0.0f, mixRed, mixGreen, mixBlue);
    unsigned char goldRed = 0;
    unsigned char goldGreen = 0;
    unsigned char goldBlue = 0;
    render::ItemPickupTargetHighlightTint(1.0f, goldRed, goldGreen, goldBlue);
    Expect(mixRed == 255 && mixGreen == 255 && mixBlue == 255, "25. tint at gold 0 is white");
    Expect(
        goldRed == 255 && goldGreen == 220 && goldBlue == 72,
        "26. tint at gold 1 is RGB(255,220,72)");
    Expect(
        render::ItemPickupTargetHighlightAlpha(0.80f)
            == goldZeroPresentation.highlightAlpha,
        "22. Gold Amount is not another alpha multiplier");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d item-pickup target highlight test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Item-pickup target highlight tests passed.\n");
    return 0;
}
