#include "gameplay/Equipment.h"
#include "gameplay/Inventory.h"
#include "gameplay/InventoryTestSupport.h"
#include "gameplay/InventoryUi.h"
#include "gameplay/InventoryView.h"
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
gameplay::Equipment gEquipment;

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

gameplay::InventoryUiInputAction HandleUi(
    gameplay::InventoryUiState& ui,
    gameplay::Inventory& inventory,
    const gameplay::GameplayDefinitionRegistry& registry,
    const input::InputState& input)
{
    return gameplay::HandleInventoryUiInput(ui, inventory, gEquipment, registry, input);
}
}

int main()
{
    const gameplay::GameplayDefinitionRegistry registry = gameplay::MakeStandardTestItemRegistry();

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
        Expect(
            HandleUi(ui, inventory, registry, PressToggle())
                == gameplay::InventoryUiInputAction::Open,
            "2. semantic toggle opens Inventory");
        Expect(ui.selectedItemId.empty(), "6. empty Inventory has no selection");
        Expect(gameplay::InventoryUiPausesSimulation(ui), "19. open Inventory pauses simulation");
        Expect(gameplay::InventoryUiBlocksGameplay(false, true), "open frame blocks gameplay");
    }

    {
        gameplay::Inventory inventory;
        Expect(
            HandleUi(ui, inventory, registry, PressToggle())
                == gameplay::InventoryUiInputAction::Close,
            "3. semantic toggle closes Inventory");
    }

    {
        gameplay::Inventory empty;
        Expect(
            HandleUi(ui, empty, registry, PressToggle())
                == gameplay::InventoryUiInputAction::Open,
            "5. empty Inventory opens safely");
        Expect(
            HandleUi(ui, empty, registry, PressCancel())
                == gameplay::InventoryUiInputAction::Close,
            "4. Esc closes Inventory");
        Expect(
            gameplay::InventoryUiBlocksGameplay(true, ui.open),
            "Esc close frame still blocks gameplay so Pause cannot share that edge");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/bau", 5), "seed bau");
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "seed key");
        Expect(inventory.Entries().size() == 3, "bau splits plus key");
        Expect(inventory.Entries()[0].itemId == "items/bau", "8. grouped order: bau before master_key");
        Expect(inventory.Entries().back().itemId == "items/master_key", "grouped order: master_key last");

        gameplay::InventoryUiState panel{};
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(panel.open, "non-empty open succeeds");
        Expect(panel.selectedItemId == "items/bau", "7. non-empty open selects first deterministic entry");

        gameplay::NavigateInventorySelection(panel, inventory, 1);
        Expect(panel.selectedItemId == "items/bau", "10. next stays on next bau stack");
        Expect(inventory.GetQuantity(panel.selectedItemId) == 5,
            "16. selected details reflect production quantity");

        gameplay::NavigateInventorySelection(panel, inventory, -1);
        Expect(panel.selectedItemId == "items/bau", "9. previous navigation");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/bau", 1), "preserve seed bau");
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "preserve seed key");
        gameplay::InventoryUiState panel{};
        panel.selectedItemId = "items/master_key";
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(panel.selectedItemId == "items/master_key", "12. previously selected itemId is preserved");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/bau", 1), "repair seed bau");
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "repair seed key");
        gameplay::InventoryUiState panel{};
        panel.open = true;
        panel.selectedItemId = "items/master_key";
        Expect(inventory.TryRemove("items/master_key", 1), "remove selected item");
        gameplay::RepairInventorySelection(panel, inventory);
        Expect(panel.selectedItemId == "items/bau", "13. removed selected item repairs to first entry");
        Expect(gameplay::FindInventoryUiSelectionIndex(inventory, panel.selectedItemId) == 0,
            "15. repair stores itemId, not a dangling index");
        inventory.Clear();
        gameplay::RepairInventorySelection(panel, inventory);
        Expect(panel.selectedItemId.empty(), "14. cleared Inventory clears selection");
        gameplay::NavigateInventorySelection(panel, inventory, 1);
        Expect(!panel.selectedItemId.empty() || panel.equipmentFocus,
            "empty inventory navigation focuses equipment");
    }

    {
        gameplay::Inventory inventory;
        world::ItemPickupSpec pickup{};
        pickup.position = {2.0f, 0.5f, 0.0f};
        pickup.itemId = "items/master_key";
        pickup.quantity = 1;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(gameplay::TryCollectItemPickup(inventory, run, {&pickup, 1}, 0, registry),
            "M100 collect key x1");
        gameplay::InventoryUiState panel{};
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(panel.selectedItemId == "items/master_key", "17. collected item appears with no UI copy");
        Expect(inventory.GetQuantity("items/master_key") == 1, "UI reads production quantity 1");

        pickup.quantity = 1;
        run = gameplay::MakeClearedItemPickupRunState(1);
        Expect(gameplay::TryCollectItemPickup(inventory, run, {&pickup, 1}, 0, registry),
            "M100 collect second non-stackable key");
        gameplay::RepairInventorySelection(panel, inventory);
        Expect(inventory.Entries().size() == 2, "18. non-stackable second unit is another stack");
        Expect(inventory.GetQuantity("items/master_key") == 2, "two keys");
        Expect(panel.selectedItemId == "items/master_key", "selection preserved across add");

        world::ItemPickupSpec coin{};
        coin.position = {4.0f, 0.5f, 0.0f};
        coin.itemId = "items/coin";
        coin.quantity = 5;
        gameplay::ItemPickupRunState coinRun = gameplay::MakeClearedItemPickupRunState(1);
        Expect(gameplay::TryCollectItemPickup(inventory, coinRun, {&coin, 1}, 0, registry),
            "M100 collect coin x5");
        Expect(inventory.GetQuantity("items/coin") == 5, "coin quantity");
        Expect(inventory.GetQuantity("items/master_key") == 2, "key quantity unchanged");
    }

    {
        gameplay::InventoryUiState panel{};
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "block seed");
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(gameplay::InventoryUiBlocksGameplay(true, true), "21. open suppresses Grab/Drop");
        Expect(gameplay::InventoryUiPausesSimulation(panel), "20. open suppresses jump/movement");

        input::InputState closeAndInteract = PressToggle();
        closeAndInteract.grabDropPressed = true;
        closeAndInteract.jumpPressed = true;
        closeAndInteract.moveX = 1.0f;
        const bool wasOpen = panel.open;
        Expect(
            HandleUi(panel, inventory, registry, closeAndInteract)
                == gameplay::InventoryUiInputAction::Close,
            "closing Tab is Inventory Close, not a raw-key cue");
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
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "checkpoint seed");
        gameplay::InventoryUiState panel{};
        panel.open = true;
        panel.selectedItemId = "items/master_key";
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::CheckpointRespawn);
        gameplay::ApplyInventoryUiLifecycle(
            panel, gameplay::InventoryLifecycleEvent::CheckpointRespawn, inventory);
        Expect(inventory.GetQuantity("items/master_key") == 1, "26. checkpoint preserves Inventory");
        Expect(panel.open, "checkpoint does not force-close UI");
        Expect(panel.selectedItemId == "items/master_key", "checkpoint preserves valid selection");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "restart seed");
        gameplay::InventoryUiState panel{};
        gameplay::OpenInventoryUi(panel, inventory);
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::RestartRun);
        gameplay::ApplyInventoryUiLifecycle(
            panel, gameplay::InventoryLifecycleEvent::RestartRun, inventory);
        Expect(inventory.Entries().empty(), "27. Restart clears Inventory");
        Expect(!panel.open, "Restart closes UI");
        Expect(panel.selectedItemId.empty(), "Restart clears selection");
        HandleUi(panel, inventory, registry, PressToggle());
        Expect(panel.open, "reopen after Restart is empty-safe");
        Expect(panel.selectedItemId.empty(), "reopened empty has no selection");
        gameplay::CloseInventoryUi(panel);
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/bau", 4), "apply seed");
        gameplay::InventoryUiState panel{};
        panel.open = true;
        panel.selectedItemId = "items/bau";
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
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "rebuild seed");
        gameplay::InventoryUiState panel{};
        panel.open = true;
        panel.selectedItemId = "items/master_key";
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild);
        gameplay::ApplyInventoryUiLifecycle(
            panel, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild, inventory);
        Expect(inventory.GetQuantity("items/master_key") == 1, "29. PhysicsWorld rebuild preserves Inventory");
        Expect(panel.open, "rebuild preserves open UI");
        Expect(panel.selectedItemId == "items/master_key", "rebuild preserves valid selection");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "transition seed");
        gameplay::InventoryUiState panel{};
        panel.open = true;
        panel.selectedItemId = "items/master_key";
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::LevelTransition);
        gameplay::ApplyInventoryUiLifecycle(
            panel, gameplay::InventoryLifecycleEvent::LevelTransition, inventory);
        Expect(inventory.GetQuantity("items/master_key") == 1, "level transition preserves Inventory");
        Expect(!panel.open, "level transition closes UI");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/battery", 1), "harness coexistence seed");
        gameplay::InventoryUiState panel{};
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(gameplay::AddTestItem(inventory, registry, "items/battery", 2),
            "31. harness-style TryAdd is the same Inventory");
        Expect(inventory.GetQuantity(panel.selectedItemId) == 3,
            "player UI and Development harness share production Inventory");
        Expect(inventory.TryRemove("items/battery", 1), "harness-style TryRemove");
        Expect(inventory.GetQuantity("items/battery") == 2, "shared Inventory after Remove");
    }

    Expect(!gameplay::kInventoryDevelopmentHarnessEnabled,
        "34. player UI module does not enable ImGui Development harness");

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/helmet", 1), "equip seed");
        gameplay::Equipment equipment;
        gameplay::InventoryUiState panel{};
        gameplay::OpenInventoryUi(panel, inventory);
        Expect(panel.selectedItemId == "items/helmet", "equipment UI selects helmet");
        const gameplay::InventoryEquipmentView view =
            gameplay::BuildInventoryEquipmentView(inventory, equipment, panel, registry);
        Expect(view.stacks.size() == 1 && view.stacks[0].canEquip, "resolved selected equipment is eligible");
        Expect(equipment.Equip(inventory, "items/helmet", registry)
                == gameplay::EquipmentTransactionStatus::Ok,
            "selected eligible item equips");
        Expect(inventory.GetQuantity("items/helmet") == 0, "equip consumes inventory quantity");
        Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Head) == "items/helmet",
            "Head slot owns the helmet");
        Expect(equipment.Unequip(inventory, gameplay::EquipmentSlot::Head, registry)
                == gameplay::EquipmentTransactionStatus::Ok,
            "unequip returns the item");
        Expect(inventory.GetQuantity("items/helmet") == 1, "unequip restores inventory quantity");
        Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Head).empty(), "Head slot is empty after unequip");
        const gameplay::InventoryEquipmentView missingView =
            gameplay::BuildInventoryEquipmentView(
                inventory, equipment, panel, gameplay::GameplayDefinitionRegistry{});
        Expect(missingView.stacks[0].missingDefinition, "UI view-model marks missing definition");
    }

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
