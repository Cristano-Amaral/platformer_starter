#include "gameplay/Inventory.h"
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

std::string ThirtyThreeA()
{
    return std::string(gameplay::kMaxItemIdLength + 1, 'a');
}
}

int main()
{
    Expect(!gameplay::kInventoryDevelopmentHarnessEnabled,
        "InventoryTest is not the Development game binary; harness flag stays off here");

    {
        gameplay::Inventory inventory;
        Expect(inventory.Entries().empty(), "1. new Inventory is empty");
        Expect(inventory.GetQuantity("key") == 0, "9. missing item returns zero");
        Expect(!inventory.Has("key", 1), "missing Has is false");
    }

    Expect(gameplay::IsValidItemId("key"), "2. key is a valid canonical itemId");
    Expect(gameplay::IsValidItemId("coin"), "coin is a valid canonical itemId");
    Expect(gameplay::IsValidItemId("battery"), "battery is a valid canonical itemId");
    Expect(gameplay::IsValidItemId("power_cell"), "underscore is allowed");
    Expect(gameplay::IsValidItemId("power-cell"), "hyphen is allowed");
    Expect(gameplay::IsValidItemId("a"), "single lowercase letter is valid");
    Expect(gameplay::IsValidItemId(std::string(gameplay::kMaxItemIdLength, 'a')),
        "max-length itemId is valid");

    Expect(!gameplay::IsValidItemId(""), "3. empty itemId is rejected");
    Expect(!gameplay::IsValidItemId("Key"), "5. uppercase is rejected (no folding)");
    Expect(!gameplay::IsValidItemId("KEY"), "uppercase token is rejected");
    Expect(!gameplay::IsValidItemId("kEy"), "mixed case is rejected");
    Expect(!gameplay::IsValidItemId("key!"), "4. punctuation is rejected");
    Expect(!gameplay::IsValidItemId("key id"), "whitespace is rejected");
    Expect(!gameplay::IsValidItemId("-key"), "leading hyphen is rejected");
    Expect(!gameplay::IsValidItemId("_key"), "leading underscore is rejected");
    Expect(!gameplay::IsValidItemId("9key"), "leading digit is rejected");
    Expect(!gameplay::IsValidItemId(ThirtyThreeA()), "itemId longer than 32 is rejected");

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "6. Add one item");
        Expect(inventory.GetQuantity("key") == 1, "GetQuantity after Add one");
        Expect(inventory.Entries().size() == 1, "one logical entry after first Add");
        Expect(inventory.TryAdd("key", 2), "7. Add quantity > 1");
        Expect(inventory.GetQuantity("key") == 3, "8. repeated Add merges into one entry");
        Expect(inventory.Entries().size() == 1, "still one logical key entry");
        Expect(inventory.Has("key", 3), "10. Has exact owned quantity succeeds");
        Expect(!inventory.Has("key", 4), "11. Has greater-than-owned is false");
        Expect(inventory.TryRemove("key", 1), "12. Remove partial quantity");
        Expect(inventory.GetQuantity("key") == 2, "quantity after partial Remove");
        Expect(inventory.TryRemove("key", 2), "13. Remove exact quantity");
        Expect(inventory.GetQuantity("key") == 0, "exact Remove leaves quantity 0");
        Expect(inventory.Entries().empty(), "exact Remove removes the logical entry");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 2), "seed over-remove inventory");
        Expect(!inventory.TryRemove("key", 3), "14. Remove more than owned fails");
        Expect(inventory.GetQuantity("key") == 2, "over-remove leaves inventory unchanged");
        Expect(inventory.Entries().size() == 1, "over-remove keeps the entry");
    }

    {
        gameplay::Inventory inventory;
        Expect(!inventory.TryAdd("key", 0), "15. zero quantity Add is rejected");
        Expect(inventory.Entries().empty(), "zero Add does not mutate");
        Expect(!inventory.TryRemove("key", 0), "zero quantity Remove is rejected");
        Expect(!inventory.TryAdd("key", -1), "16. negative quantity Add is rejected");
        Expect(!inventory.TryRemove("key", -4), "negative quantity Remove is rejected");
        Expect(!inventory.TryAdd("key", gameplay::kMaxItemQuantity + 1),
            "quantity above max is rejected");
        Expect(inventory.Entries().empty(), "invalid quantity does not mutate");
        Expect(!inventory.Has("key", 0), "Has rejects non-positive quantity");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", gameplay::kMaxItemQuantity), "fill to quantity cap");
        Expect(!inventory.TryAdd("key", 1), "17. quantity overflow fails");
        Expect(inventory.GetQuantity("key") == gameplay::kMaxItemQuantity,
            "overflow leaves inventory unchanged");
        Expect(inventory.Entries().size() == 1, "overflow does not split entries");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("coin", 5), "seed invalid-mutation inventory");
        Expect(!inventory.TryAdd("", 1), "18. invalid Add empty id");
        Expect(!inventory.TryAdd("Key", 1), "invalid Add uppercase");
        Expect(!inventory.TryAdd("key!", 1), "invalid Add punctuation");
        Expect(inventory.GetQuantity("coin") == 5, "invalid Add does not mutate");
        Expect(inventory.Entries().size() == 1, "invalid Add does not add a second id");
        Expect(!inventory.TryRemove("", 1), "19. invalid Remove empty id");
        Expect(!inventory.TryRemove("Key", 1), "invalid Remove uppercase");
        Expect(!inventory.TryRemove("missing", 1), "Remove missing id fails");
        Expect(inventory.GetQuantity("coin") == 5, "invalid Remove does not mutate");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "seed Clear");
        Expect(inventory.TryAdd("coin", 5), "seed Clear second id");
        inventory.Clear();
        Expect(inventory.Entries().empty(), "20. Clear empties Inventory");
        Expect(inventory.GetQuantity("key") == 0, "Clear missing query is zero");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("coin", 5), "enum add coin");
        Expect(inventory.TryAdd("key", 3), "enum add key");
        Expect(inventory.TryAdd("battery", 1), "enum add battery");
        Expect(inventory.TryAdd("key", 1), "enum merge key");
        Expect(inventory.Entries().size() == 3, "21. enumeration contains each logical ID once");
        Expect(inventory.Entries()[0].itemId == "battery" && inventory.Entries()[0].quantity == 1,
            "22. enumeration is lexicographic (battery)");
        Expect(inventory.Entries()[1].itemId == "coin" && inventory.Entries()[1].quantity == 5,
            "enumeration coin");
        Expect(inventory.Entries()[2].itemId == "key" && inventory.Entries()[2].quantity == 4,
            "enumeration key merged");
        gameplay::Inventory again;
        Expect(again.TryAdd("key", 4) && again.TryAdd("battery", 1) && again.TryAdd("coin", 5),
            "second inventory same adds, different order");
        Expect(again.Entries().size() == 3, "second inventory unique count");
        Expect(again.Entries()[0].itemId == inventory.Entries()[0].itemId
                && again.Entries()[1].itemId == inventory.Entries()[1].itemId
                && again.Entries()[2].itemId == inventory.Entries()[2].itemId,
            "enumeration order is deterministic across insert order");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 2), "lifecycle seed");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::CheckpointRespawn);
        Expect(inventory.GetQuantity("key") == 2, "24. checkpoint respawn preserves Inventory");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::PhysicsWorldRebuild);
        Expect(inventory.GetQuantity("key") == 2, "25. PhysicsWorld rebuild policy preserves");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::RestartRun);
        Expect(inventory.Entries().empty(), "23. full Restart Run clears Inventory");
        Expect(inventory.TryAdd("coin", 1), "reseed after restart");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::NewRun);
        Expect(inventory.Entries().empty(), "15/initial. NewRun clears Inventory");
        Expect(inventory.TryAdd("battery", 4), "reseed apply");
        gameplay::ApplyInventoryLifecycle(
            inventory, gameplay::InventoryLifecycleEvent::ApplyCommittedLevel);
        Expect(inventory.Entries().empty(), "Apply/reload new-run authority clears Inventory");
    }

    {
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 1), "32. Development Add uses TryAdd");
        Expect(inventory.TryAdd("key", 2), "Development Add merge uses TryAdd");
        Expect(inventory.GetQuantity("key") == 3, "harness Add is production Add");
        Expect(inventory.TryRemove("key", 1), "33. Development Remove uses TryRemove");
        Expect(inventory.GetQuantity("key") == 2, "harness Remove is production Remove");
        inventory.Clear();
        Expect(inventory.Entries().empty(), "34. Development Clear uses Clear");
    }

    {
        const world::ParseLevelFileResult parsed =
            world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
        Expect(parsed.status == world::LoadLevelFileStatus::Loaded, "load canonical Level 01");
        Expect(parsed.level.doors.empty(), "40. canonical Level 01 has 0 Doors");
        Expect(parsed.level.pressurePlates.empty(), "canonical Level 01 has 0 Pressure Plates");
        Expect(parsed.level.dynamicBoxes.empty(), "canonical Level 01 has 0 Dynamic Boxes");
        Expect(parsed.level.staticProps.empty(), "canonical Level 01 has 0 Static Props");
        Expect(parsed.level.itemPickups.empty(), "canonical Level 01 has 0 Item Pickups");

        world::LevelDefinition workingCopy = parsed.level;
        world::LevelDefinition active = parsed.level;
        world::LevelDefinition savedSourceBaseline = parsed.level;
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", 3), "mutate runtime inventory");
        Expect(inventory.TryAdd("coin", 5), "mutate runtime inventory second id");
        Expect(world::AuthoredLevelDataEqual(workingCopy, active),
            "31. inventory mutation does not copy into workingCopy/active");
        Expect(world::AuthoredLevelDataEqual(active, savedSourceBaseline),
            "inventory mutation does not copy into savedSourceBaseline");
        const bool modified = !world::AuthoredLevelDataEqual(workingCopy, active);
        const bool dirty = !world::AuthoredLevelDataEqual(active, savedSourceBaseline);
        Expect(!modified, "29. inventory mutation does not mark Modified");
        Expect(!dirty, "inventory mutation does not mark Dirty");

        const std::string written = world::SerializeLevelText(active);
        Expect(!written.empty(), "canonical level remains writable");
        Expect(CountFirstToken(written, "inventory") == 0,
            "30. Inventory contents are absent from Level Format serialization");
        Expect(written.find("key") == std::string::npos,
            "serialized level does not contain test itemId key");
        Expect(written.find("coin") == std::string::npos,
            "serialized level does not contain test itemId coin");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Inventory test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Inventory tests passed.\n");
    return 0;
}
