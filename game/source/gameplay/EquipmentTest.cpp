#include "gameplay/Equipment.h"
#include "gameplay/EquipmentSlot.h"
#include "gameplay/Inventory.h"
#include "gameplay/InventoryTestSupport.h"
#include "gameplay/InventoryView.h"

#include <cstdio>
#include <string>

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
}

int main()
{
    Expect(gameplay::EquipmentSlotName(gameplay::EquipmentSlot::Head) == "Head", "Head name");
    Expect(gameplay::EquipmentSlotFromName("MainHand") == gameplay::EquipmentSlot::MainHand,
        "MainHand from name");
    Expect(!gameplay::EquipmentSlotFromName("Helmet").has_value(), "unknown slot rejected");
    Expect(!gameplay::EquipmentSlotFromName("head").has_value(), "slot names are case-sensitive");

    gameplay::GameplayDefinitionRegistry registry = gameplay::MakeStandardTestItemRegistry();
    gameplay::Inventory inventory;
    gameplay::Equipment equipment;

    Expect(!equipment.CanEquip("items/master_key", registry), "key is not equipment");
    Expect(equipment.CanEquip("items/helmet", registry), "helmet can equip");

    Expect(equipment.Equip(inventory, "items/helmet", registry)
            == gameplay::EquipmentTransactionStatus::MissingInventoryItem,
        "equip without inventory fails");
    Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Head).empty(), "failed equip empty slot");

    Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "seed key");
    Expect(equipment.Equip(inventory, "items/master_key", registry)
            == gameplay::EquipmentTransactionStatus::NotEquipment,
        "reject non-equipment");
    Expect(inventory.GetQuantity("items/master_key") == 1, "rejected equip leaves inventory");

    Expect(gameplay::AddTestItem(inventory, registry, "items/helmet", 1), "seed helmet");
    Expect(equipment.Equip(inventory, "items/helmet", registry)
            == gameplay::EquipmentTransactionStatus::Ok,
        "equip eligible helmet");
    Expect(inventory.GetQuantity("items/helmet") == 0, "equip consumes one");
    Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Head) == "items/helmet", "head occupied");

    Expect(equipment.Unequip(inventory, gameplay::EquipmentSlot::Head, registry)
            == gameplay::EquipmentTransactionStatus::Ok,
        "unequip returns item");
    Expect(inventory.GetQuantity("items/helmet") == 1, "unequip restores inventory");
    Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Head).empty(), "slot cleared");

    Expect(gameplay::AddTestItem(inventory, registry, "items/armor", 1), "seed armor");
    Expect(equipment.Equip(inventory, "items/helmet", registry) == gameplay::EquipmentTransactionStatus::Ok,
        "equip helmet again");
    Expect(equipment.Equip(inventory, "items/armor", registry) == gameplay::EquipmentTransactionStatus::Ok,
        "equip armor to body");
    Expect(gameplay::AddTestItem(inventory, registry, "items/sword", 1), "seed sword");
    Expect(equipment.Equip(inventory, "items/sword", registry) == gameplay::EquipmentTransactionStatus::Ok,
        "equip sword");

    Expect(gameplay::AddTestItem(inventory, registry, "items/helmet", 1), "second helmet");
    Expect(equipment.Equip(inventory, "items/helmet", registry) == gameplay::EquipmentTransactionStatus::Ok,
        "occupied-slot replacement");
    Expect(inventory.GetQuantity("items/helmet") == 1, "replaced helmet returned");
    Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Head) == "items/helmet", "head still helmet");
    Expect(inventory.GetQuantity("items/armor") == 0, "armor still equipped, not duplicated");
    Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Body) == "items/armor", "body unchanged");

    gameplay::GameplayDefinitionRegistry emptyRegistry;
    Expect(equipment.Unequip(inventory, gameplay::EquipmentSlot::Body, emptyRegistry)
            == gameplay::EquipmentTransactionStatus::InventoryRejected,
        "failed unequip when definition missing");
    Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Body) == "items/armor",
        "failed unequip leaves slot");
    Expect(inventory.GetQuantity("items/armor") == 0, "failed unequip does not duplicate");

    Expect(equipment.Equip(inventory, "items/armor", emptyRegistry)
            != gameplay::EquipmentTransactionStatus::Ok,
        "failed equip with empty registry");
    Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Body) == "items/armor",
        "failed equip leaves equipped item");

    gameplay::ApplyEquipmentLifecycle(equipment, gameplay::InventoryLifecycleEvent::RestartRun);
    Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Head).empty(), "restart clears equipment");
    Expect(equipment.GetEquipped(gameplay::EquipmentSlot::Body).empty(), "restart clears body");

    {
        gameplay::Inventory viewInventory;
        gameplay::Equipment viewEquipment;
        Expect(gameplay::AddTestItem(viewInventory, registry, "items/bau", 2), "view seed");
        Expect(gameplay::AddTestItem(viewInventory, registry, "items/helmet", 1), "view helmet");
        Expect(viewEquipment.Equip(viewInventory, "items/helmet", registry)
                == gameplay::EquipmentTransactionStatus::Ok,
            "view equip");
        gameplay::InventoryUiState ui{};
        ui.selectedItemId = "items/bau";
        ui.selectedStackIndex = 0;
        const gameplay::InventoryEquipmentView view = gameplay::BuildInventoryEquipmentView(
            viewInventory, viewEquipment, ui, registry);
        Expect(view.stacks.size() == 1, "view one remaining stack");
        Expect(view.stacks[0].displayName == "coin" || view.stacks[0].identity == "items/bau",
            "view shows identity-backed stack");
        Expect(view.stacks[0].quantity == 2, "view quantity");
        Expect(!view.stacks[0].missingDefinition, "bau resolves");
        Expect(view.equipment[0].slot == gameplay::EquipmentSlot::Head, "head slot view");
        Expect(view.equipment[0].occupied, "head occupied in view");
        Expect(!view.equipment[0].missingDefinition, "helmet resolves in equipment view");
    }

    {
        gameplay::Inventory missingInventory;
        gameplay::InventoryEntry orphan;
        // Direct mutation is not available; add then drop definition.
        gameplay::GameplayDefinitionRegistry temp = gameplay::MakeStandardTestItemRegistry();
        Expect(gameplay::AddTestItem(missingInventory, temp, "items/bau", 1), "orphan seed");
        gameplay::GameplayDefinitionRegistry gone;
        gameplay::InventoryUiState ui{};
        const gameplay::InventoryEquipmentView view =
            gameplay::BuildInventoryEquipmentView(missingInventory, equipment, ui, gone);
        Expect(view.stacks.size() == 1, "orphan stack remains");
        Expect(view.stacks[0].missingDefinition, "missing definition is explicit");
        Expect(view.stacks[0].identity == "items/bau", "identity preserved");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Equipment test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Equipment tests passed.\n");
    return 0;
}
