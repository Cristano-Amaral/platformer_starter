#include "gameplay/Inventory.h"
#include "gameplay/InventoryUi.h"
#include "gameplay/ItemPickupRuntime.h"
#include "input/InputState.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelWriter.h"

#include <cstdio>
#include <string>
#include <string_view>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++gFailures;
    }
}

input::InputState PressToggle()
{
    input::InputState input;
    input.toggleInventoryPressed = true;
    return input;
}

input::InputState PressCancel()
{
    input::InputState input;
    input.cancelPressed = true;
    return input;
}

input::InputState PressPrevious()
{
    input::InputState input;
    input.inventoryPreviousPressed = true;
    return input;
}

input::InputState PressNext()
{
    input::InputState input;
    input.inventoryNextPressed = true;
    return input;
}
}

int main()
{
    Expect(gameplay::kPlayerInventoryUiEnabled, "32. player Inventory UI is always enabled");
    Expect(!gameplay::kInventoryDevelopmentHarnessEnabled,
        "33. this test binary excludes the Development Inventory harness");

    gameplay::InventoryUiState ui{};
    Expect(!ui.open, "1. Inventory UI starts closed");
    Expect(ui.selectedItemId.empty(), "closed UI has empty selection by default");
    Expect(!gameplay::InventoryUiPausesSimulation(ui), "closed UI does not pause simulation");
    Expect(!gameplay::InventoryUiBlocksGameplay(false, false), "closed frame does not block gameplay");

    {
        gameplay::Inventory inventory;
        gameplay::HandleInventoryUiInput(ui, inventory, PressToggle());
        Expect(ui.open, "2. semantic toggle opens Inventory");
        Expect(ui.selectedItemId.empty(), "6. empty Inventory has no selection");
        Expect(gameplay::InventoryUiPausesSimulation(ui), "19. open Inventory pauses simulation");
        Expect(gameplay::InventoryUiBlocksGameplay(false, true), "open frame blocks gameplay");
    }

    {
        gameplay::Inventory inventory;
        gameplay::HandleInventoryUiInput(ui, inventory, PressToggle());
        Expect(!ui.open, "3. semantic toggle closes Inventory");
    }

    {
        gameplay::HandleInventoryUiInput(ui, gameplay::Inventory{}, PressToggle());
        Expect(ui.open, "5. empty Inventory opens safely");
        gameplay::HandleInventoryUiInput(ui, gameplay::Inventory{}, PressCancel());
        Expect(!ui.open, "4. Esc closes Inventory");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("coin", 5), "seed coin");
        Expect(inventory.TryAdd("key", 3), "seed key");
        Expect(inventory.Entries().size() == 2, "two logical entries");
        Expect(inventory.Entries()[0].itemId == "coin", "8. M54 order: coin before key");
        Expect(inventory.Entries()[1].itemId == "key", "M54 order: key second");

        gameplay::InventoryUiState panel{};
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(panel.open, "non-empty open succeeds");
        Expect(panel.selectedItemId == "coin", "7. non-empty open selects first deterministic entry");

        gameplay::NavigateInventorySelection(panel, inventory, 1);
        Expect(panel.selectedItemId == "key", "10. next navigation");
        Expect(inventory.GetQuantity(panel.selectedItemId) == 3,
            "16. selected details reflect production quantity");

        gameplay::NavigateInventorySelection(panel, inventory, -1);
        Expect(panel.selectedItemId == "coin", "9. previous navigation");

        gameplay::NavigateInventorySelection(panel, inventory, -1);
        Expect(panel.selectedItemId == "key", "11. previous wraps first -> last");
        gameplay::NavigateInventorySelection(panel, inventory, 1);
        Expect(panel.selectedItemId == "coin", "11b. next wraps last -> first");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("coin", 1), "preserve seed coin");
        Expect(inventory.TryAdd("key", 1), "preserve seed key");
        gameplay::InventoryUiState panel{};
        panel.selectedItemId = "key";
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(panel.selectedItemId == "key", "12. previously selected itemId is preserved");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("coin", 1), "repair seed coin");
        Expect(inventory.TryAdd("key", 1), "repair seed key");
        gameplay::InventoryUiState panel{};
        panel.open = true;
        panel.selectedItemId = "key";
        Expect(inventory.TryRemove("key", 1), "remove selected item");
        gameplay::RepairInventorySelection(panel, inventory);
        Expect(panel.selectedItemId == "coin", "13. removed selected item repairs to first entry");
        Expect(gameplay::FindInventoryUiSelectionIndex(inventory, panel.selectedItemId) == 0,
            "15. repair stores itemId, not a dangling index");
        inventory.Clear();
        gameplay::RepairInventorySelection(panel, inventory);
        Expect(panel.selectedItemId.empty(), "14. cleared Inventory clears selection");
        gameplay::NavigateInventorySelection(panel, inventory, 1);
        Expect(panel.selectedItemId.empty(), "empty navigation is a no-op");
    }

    {
        gameplay::Inventory inventory;
        world::ItemPickupSpec pickup{};
        pickup.position = {2.0f, 0.5f, 0.0f};
        pickup.itemId = "key";
        pickup.quantity = 1;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(gameplay::TryCollectItemPickup(inventory, run, {&pickup, 1}, 0),
            "M55 collect key x1");
        gameplay::InventoryUiState panel{};
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(panel.selectedItemId == "key", "17. collected item appears with no UI copy");
        Expect(inventory.GetQuantity("key") == 1, "UI reads production quantity 1");

        pickup.quantity = 2;
        run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(gameplay::TryCollectItemPickup(inventory, run, {&pickup, 1}, 0),
            "M55 collect key x2");
        gameplay::RepairInventorySelection(panel, inventory);
        Expect(inventory.Entries().size() == 1, "18. repeated additions stay one entry");
        Expect(inventory.GetQuantity("key") == 3, "merged M54 quantity is 3");
        Expect(panel.selectedItemId == "key", "selection preserved across merge");

        world::ItemPickupSpec coin{};
        coin.position = {4.0f, 0.5f, 0.0f};
        coin.itemId = "coin";
        coin.quantity = 5;
        gameplay::ItemPickupRunState coinRun = gameplay::MakeClearedItemPickupRunState(1);
        Expect(gameplay::TryCollectItemPickup(inventory, coinRun, {&coin, 1}, 0),
            "M55 collect coin x5");
        Expect(inventory.Entries().size() == 2, "coin is its own entry");
        Expect(inventory.Entries()[0].itemId == "coin" && inventory.Entries()[0].quantity == 5,
            "M54 order after coin: coin then key");
        Expect(inventory.GetQuantity("key") == 3, "key quantity unchanged");
    }

    {
        gameplay::InventoryUiState panel{};
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "block seed");
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(gameplay::InventoryUiBlocksGameplay(true, true), "21. open suppresses Grab/Drop");
        Expect(gameplay::InventoryUiPausesSimulation(panel), "20. open suppresses jump/movement");

        input::InputState closeAndInteract = PressToggle();
        closeAndInteract.grabDropPressed = true;
        closeAndInteract.jumpPressed = true;
        closeAndInteract.moveX = 1.0f;
        const bool wasOpen = panel.open;
        gameplay::HandleInventoryUiInput(panel, inventory, closeAndInteract);
        Expect(!panel.open, "24. closing resumes (UI closed)");
        Expect(gameplay::InventoryUiBlocksGameplay(wasOpen, panel.open),
            "25. closing frame still blocks gameplay so E is not synthesized");
        Expect(closeAndInteract.grabDropPressed,
            "HandleInventoryUiInput does not consume or forge grabDropPressed");
        Expect(!gameplay::InventoryUiBlocksGameplay(false, false),
            "next closed frame allows gameplay");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 2), "checkpoint seed");
        gameplay::InventoryUiState panel{};
        panel.open = true;
        panel.selectedItemId = "key";
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::CheckpointRespawn);
        gameplay::ApplyInventoryUiLifecycle(
            panel, gameplay::InventoryLifecycleEvent::CheckpointRespawn, inventory);
        Expect(inventory.GetQuantity("key") == 2, "26. checkpoint preserves Inventory");
        Expect(panel.open, "checkpoint does not force-close UI");
        Expect(panel.selectedItemId == "key", "checkpoint preserves valid selection");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "restart seed");
        gameplay::InventoryUiState panel{};
        gameplay::OpenInventoryUi(panel, inventory);
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::RestartRun);
        gameplay::ApplyInventoryUiLifecycle(
            panel, gameplay::InventoryLifecycleEvent::RestartRun, inventory);
        Expect(inventory.Entries().empty(), "27. Restart clears Inventory");
        Expect(!panel.open, "Restart closes UI");
        Expect(panel.selectedItemId.empty(), "Restart clears selection");
        gameplay::HandleInventoryUiInput(panel, inventory, PressToggle());
        Expect(panel.open, "reopen after Restart is empty-safe");
        Expect(panel.selectedItemId.empty(), "reopened empty has no selection");
        gameplay::CloseInventoryUi(panel);
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("coin", 4), "apply seed");
        gameplay::InventoryUiState panel{};
        panel.open = true;
        panel.selectedItemId = "coin";
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::ApplyCommittedLevel);
        gameplay::ApplyInventoryUiLifecycle(
            panel, gameplay::InventoryLifecycleEvent::ApplyCommittedLevel, inventory);
        Expect(inventory.Entries().empty(), "28. Apply/reload clears Inventory");
        Expect(!panel.open, "Apply/reload closes UI");
        Expect(panel.selectedItemId.empty(), "Apply/reload clears stale selection");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 3), "rebuild seed");
        gameplay::InventoryUiState panel{};
        panel.open = true;
        panel.selectedItemId = "key";
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild);
        gameplay::ApplyInventoryUiLifecycle(
            panel, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild, inventory);
        Expect(inventory.GetQuantity("key") == 3, "29. PhysicsWorld rebuild preserves Inventory");
        Expect(panel.open, "rebuild preserves open UI");
        Expect(panel.selectedItemId == "key", "rebuild preserves valid selection");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("battery", 1), "harness coexistence seed");
        gameplay::InventoryUiState panel{};
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(inventory.TryAdd("battery", 2), "31. harness-style TryAdd is the same Inventory");
        Expect(inventory.GetQuantity(panel.selectedItemId) == 3,
            "player UI and Development harness share production Inventory");
        Expect(inventory.TryRemove("battery", 1), "harness-style TryRemove");
        Expect(inventory.GetQuantity("battery") == 2, "shared Inventory after Remove");
    }

    Expect(!gameplay::kInventoryDevelopmentHarnessEnabled,
        "34. player UI module does not enable ImGui Development harness");

    {
        const world::ParseLevelFileResult parsed =
            world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
        Expect(parsed.status == world::LoadLevelFileStatus::Loaded, "load canonical Level 01");
        Expect(parsed.level.itemPickups.empty(), "41. canonical Level 01 has 0 Item Pickups");
        Expect(parsed.level.doors.empty(), "canonical Level 01 has 0 Doors");
        Expect(parsed.level.pressurePlates.empty(), "canonical Level 01 has 0 Pressure Plates");
        Expect(parsed.level.dynamicBoxes.empty(), "canonical Level 01 has 0 Dynamic Boxes");
        Expect(parsed.level.staticProps.empty(), "canonical Level 01 has 0 Static Props");
        const std::string written = world::SerializeLevelText(parsed.level);
        Expect(written.find("inventory_ui") == std::string::npos, "42. no Inventory UI Level Format");
        Expect(written.find("item_use") == std::string::npos, "43. no item-use Level Format");
        Expect(written.find("hotbar") == std::string::npos, "no hotbar Level Format");
        Expect(written.find("dynamic_box 0 5 0 1 1 1 30") == std::string::npos,
            "legacy dynamic_box line remains absent");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Inventory UI test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Inventory UI tests passed.\n");
    return 0;
}
