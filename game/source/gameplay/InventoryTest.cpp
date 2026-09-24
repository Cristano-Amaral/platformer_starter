#include "gameplay/Inventory.h"
#include "gameplay/InventoryTestSupport.h"
#include "gameplay/ItemIdentity.h"
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

int CountFirstToken(std::string_view text, std::string_view keyword)
{
    int count = 0;
    std::size_t cursor = 0;
    while (cursor <= text.size())
    {
        const std::size_t newline = text.find('\n', cursor);
        const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
        if (end > cursor)
        {
            const std::string_view line = text.substr(cursor, end - cursor);
            const std::size_t space = line.find(' ');
            const std::string_view token =
                space == std::string_view::npos ? line : line.substr(0, space);
            if (token == keyword)
            {
                ++count;
            }
        }
        if (newline == std::string_view::npos)
        {
            break;
        }
        cursor = newline + 1;
    }
    return count;
}
}

int main()
{
    Expect(!gameplay::kInventoryDevelopmentHarnessEnabled,
        "InventoryTest is not the Development game binary; harness flag stays off here");

    Expect(gameplay::IsValidInventoryItemIdentity("items/master_key"), "items/master_key is valid");
    Expect(!gameplay::IsValidInventoryItemIdentity("key"), "legacy key is not a durable identity");
    Expect(!gameplay::IsValidInventoryItemIdentity("characters/player"), "characters rejected");

    const gameplay::AuthoredItemReferenceResolution mapped =
        gameplay::ResolveAuthoredItemReference("key");
    Expect(mapped.status == gameplay::AuthoredItemReferenceStatus::LegacyMapped,
        "legacy key maps");
    Expect(mapped.identity == gameplay::kCanonicalKeyItemIdentity, "key maps to master_key");
    const gameplay::AuthoredItemReferenceResolution numeric =
        gameplay::ResolveAuthoredItemReference("1");
    Expect(numeric.status == gameplay::AuthoredItemReferenceStatus::LegacyMapped
            && numeric.identity == gameplay::kCanonicalKeyItemIdentity,
        "numeric 1 maps to master_key");
    Expect(gameplay::ResolveAuthoredItemReference("coin").status
            == gameplay::AuthoredItemReferenceStatus::UnmappedLegacy,
        "unmapped M54 coin is diagnosed");
    Expect(gameplay::ResolveAuthoredItemReference("characters/player").status
            == gameplay::AuthoredItemReferenceStatus::WrongCategory,
        "characters/... is wrong category");
    Expect(gameplay::ResolveAuthoredItemReference("items/master_key").status
            == gameplay::AuthoredItemReferenceStatus::Resolved,
        "textual identity resolves");

    const gameplay::GameplayDefinitionRegistry registry = gameplay::MakeStandardTestItemRegistry();

    {
        gameplay::Inventory inventory;
        Expect(inventory.Entries().empty(), "1. new Inventory is empty");
        Expect(inventory.GetQuantity("items/master_key") == 0, "missing item returns zero");
        Expect(!inventory.Has("items/master_key", 1), "missing Has is false");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "add one");
        Expect(inventory.GetQuantity("items/master_key") == 1, "quantity after add");
        Expect(inventory.Entries().size() == 1, "one stack");
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1),
            "non-stackable second unit is a new stack");
        Expect(inventory.GetQuantity("items/master_key") == 2, "two non-stackable units");
        Expect(inventory.Entries().size() == 2, "non-stackable is one unit per stack");
        Expect(inventory.Entries()[0].quantity == 1 && inventory.Entries()[1].quantity == 1,
            "each non-stackable stack holds one");
        Expect(inventory.TryRemove("items/master_key", 2), "remove both");
        Expect(inventory.Entries().empty(), "zero-quantity stacks removed");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/bau", 4), "fill bau stack");
        Expect(inventory.Entries().size() == 1, "one max stack");
        Expect(gameplay::AddTestItem(inventory, registry, "items/bau", 5), "split remainder");
        Expect(inventory.GetQuantity("items/bau") == 9, "total quantity after split");
        Expect(inventory.Entries().size() == 3, "4 + 4 + 1 stacks");
        Expect(inventory.Entries()[0].quantity == 4, "first stack full");
        Expect(inventory.Entries()[1].quantity == 4, "second stack full");
        Expect(inventory.Entries()[2].quantity == 1, "remainder stack");
        Expect(inventory.TryRemove("items/bau", 5), "deterministic remove from first stacks");
        Expect(inventory.GetQuantity("items/bau") == 4, "quantity after remove");
        Expect(inventory.Entries().size() == 2, "emptied first stack removed");
        Expect(inventory.Entries()[0].quantity == 3, "second stack drained by one");
        Expect(inventory.Entries()[1].quantity == 1, "remainder stack unchanged");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/coin", 5), "seed coin");
        Expect(inventory.TryAdd("items/missing", 1, registry)
                == gameplay::InventoryMutationStatus::MissingDefinition,
            "missing definition is rejected");
        Expect(inventory.GetQuantity("items/coin") == 5, "missing add does not mutate");
        Expect(inventory.TryAdd("characters/player", 1, registry)
                == gameplay::InventoryMutationStatus::WrongCategory,
            "wrong category rejected");
        Expect(inventory.TryAdd("key", 1, registry)
                == gameplay::InventoryMutationStatus::MalformedIdentity,
            "legacy short id is not accepted as durable identity");
        Expect(inventory.GetQuantity("items/coin") == 5, "invalid add does not mutate");
        Expect(!inventory.TryRemove("items/coin", 6), "over-remove fails");
        Expect(inventory.GetQuantity("items/coin") == 5, "over-remove unchanged");
        Expect(inventory.TryAdd("items/coin", 0, registry)
                == gameplay::InventoryMutationStatus::InvalidQuantity,
            "zero quantity rejected");
        Expect(inventory.TryAdd("items/coin", gameplay::kMaxItemQuantity, registry)
                == gameplay::InventoryMutationStatus::QuantityOverflow,
            "per-identity quantity cap is preserved");
        Expect(inventory.GetQuantity("items/coin") == 5, "quantity overflow does not mutate");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/coin", 5), "enum coin");
        Expect(gameplay::AddTestItem(inventory, registry, "items/battery", 1), "enum battery");
        Expect(inventory.Entries()[0].itemId == "items/battery", "lexicographic battery first");
        Expect(inventory.Entries()[1].itemId == "items/coin", "coin second");
        gameplay::Inventory again;
        Expect(gameplay::AddTestItem(again, registry, "items/coin", 5)
                && gameplay::AddTestItem(again, registry, "items/battery", 1),
            "second inventory opposite insert order");
        Expect(again.Entries()[0].itemId == inventory.Entries()[0].itemId
                && again.Entries()[1].itemId == inventory.Entries()[1].itemId,
            "enumeration order is deterministic");
    }

    {
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "lifecycle seed");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::CheckpointRespawn);
        Expect(inventory.GetQuantity("items/master_key") == 1, "checkpoint preserves");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild);
        Expect(inventory.GetQuantity("items/master_key") == 1, "rebuild preserves");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::LevelTransition);
        Expect(inventory.GetQuantity("items/master_key") == 1, "transition preserves");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::RestartRun);
        Expect(inventory.Entries().empty(), "restart clears");
        Expect(gameplay::AddTestItem(inventory, registry, "items/coin", 1), "reseed");
        gameplay::ApplyInventoryLifecycle(inventory, gameplay::InventoryLifecycleEvent::NewRun);
        Expect(inventory.Entries().empty(), "NewRun clears");
        Expect(gameplay::AddTestItem(inventory, registry, "items/battery", 4), "reseed apply");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::ApplyCommittedLevel);
        Expect(inventory.Entries().empty(), "Apply clears");
    }

    {
        const world::ParseLevelFileResult parsed =
            world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
        Expect(parsed.status == world::LoadLevelFileStatus::Loaded, "load canonical Level 01");
        Expect(parsed.level.itemPickups.empty(), "canonical Level 01 has 0 Item Pickups");
        world::LevelDefinition workingCopy = parsed.level;
        world::LevelDefinition active = parsed.level;
        world::LevelDefinition savedSourceBaseline = parsed.level;
        gameplay::Inventory inventory;
        Expect(gameplay::AddTestItem(inventory, registry, "items/master_key", 1), "mutate runtime");
        Expect(world::AuthoredLevelDataEqual(workingCopy, active),
            "inventory mutation does not copy into workingCopy/active");
        Expect(world::AuthoredLevelDataEqual(active, savedSourceBaseline),
            "inventory mutation does not copy into savedSourceBaseline");
        const std::string written = world::SerializeLevelText(active);
        Expect(CountFirstToken(written, "inventory") == 0, "Inventory absent from Level Format");
        Expect(written.find("items/master_key") == std::string::npos,
            "serialized level does not contain runtime identity");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Inventory test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Inventory tests passed.\n");
    return 0;
}
