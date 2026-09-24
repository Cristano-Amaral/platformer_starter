#include "gameplay/GameplayDefinition.h"
#include "gameplay/GameplayDefinitionFile.h"
#include "gameplay/GameplayIdentity.h"
#include "gameplay/GameplayStat.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include <string>
#include <utility>
#include <vector>

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

bool SameDefinition(const gameplay::GameplayDefinition& a, const gameplay::GameplayDefinition& b)
{
    if (a.identity != b.identity || a.category != b.category)
    {
        return false;
    }
    for (std::size_t index = 0; index < gameplay::kGameplayStatCount; ++index)
    {
        if (a.hasStat[index] != b.hasStat[index])
        {
            return false;
        }
        if (a.hasStat[index] && a.statValue[index] != b.statValue[index])
        {
            return false;
        }
    }
    return true;
}

bool SameRegistry(
    const gameplay::GameplayDefinitionRegistry& a,
    const gameplay::GameplayDefinitionRegistry& b)
{
    if (a.Count() != b.Count())
    {
        return false;
    }
    for (std::size_t index = 0; index < a.Count(); ++index)
    {
        if (!SameDefinition(a.Definitions()[index], b.Definitions()[index]))
        {
            return false;
        }
    }
    return true;
}

gameplay::GameplayDefinition MakeDefinition(
    std::string_view identity,
    std::initializer_list<std::pair<gameplay::GameplayStatId, float>> stats)
{
    gameplay::ParsedGameplayIdentity parsed;
    const bool ok = gameplay::TryParseGameplayIdentity(identity, parsed);
    Expect(ok, "test helper identity");
    gameplay::GameplayDefinition definition;
    if (!ok)
    {
        return definition;
    }
    definition.identity = parsed.text;
    definition.category = parsed.category;
    if (parsed.category == gameplay::GameplayDefinitionCategory::Item)
    {
        definition.item = gameplay::MakeDefaultItemDefinition(parsed.text);
    }
    for (const auto& [stat, value] : stats)
    {
        Expect(
            gameplay::TrySetGameplayStat(definition, stat, value) == gameplay::SetGameplayStatStatus::Set,
            "test helper stat");
    }
    return definition;
}
}

int main()
{
    using gameplay::GameplayDefinitionCategory;
    using gameplay::GameplayReferenceStatus;
    using gameplay::GameplayStatId;

    {
        gameplay::ParsedGameplayIdentity parsed;
        Expect(gameplay::TryParseGameplayIdentity("items/master_key", parsed), "valid items/master_key");
        Expect(parsed.text == "items/master_key", "identity text is unchanged");
        Expect(parsed.name == "master_key", "name is the leaf");
        Expect(parsed.category == GameplayDefinitionCategory::Item, "items prefix is Item");

        Expect(gameplay::TryParseGameplayIdentity("items/health_potion", parsed), "valid items/health_potion");
        Expect(gameplay::TryParseGameplayIdentity("characters/player", parsed), "valid characters/player");
        Expect(parsed.category == GameplayDefinitionCategory::Character, "characters prefix is Character");
        Expect(gameplay::TryParseGameplayIdentity("characters/guard", parsed), "valid characters/guard");
        Expect(gameplay::TryParseGameplayIdentity("items/a", parsed), "single-letter name is valid");
        Expect(
            gameplay::TryParseGameplayIdentity(
                "items/" + std::string(gameplay::kMaxGameplayDefinitionNameLength, 'a'), parsed),
            "max-length name is valid");
        Expect(parsed.text.size() == gameplay::kMaxGameplayIdentityLength
                || parsed.text.size() == std::string("items/").size()
                    + gameplay::kMaxGameplayDefinitionNameLength,
            "max item identity length stays inside the cap");
    }

    {
        gameplay::ParsedGameplayIdentity parsed;
        parsed.text = "sentinel";
        const char* malformed[] = {
            "",
            "key",
            "master_key",
            "items",
            "items/",
            "/master_key",
            "items//master_key",
            "items/master_key/extra",
            "items/Master_Key",
            "Items/master_key",
            "ITEMS/master_key",
            "items/master-key",
            "items/master.key",
            "items/1key",
            "items/_key",
            "characters/Player",
            "item/master_key",
            "characters/guard ",
            "items/master key",
            "items/../secret",
            "items/.",
        };
        for (const char* token : malformed)
        {
            const bool accepted = gameplay::TryParseGameplayIdentity(token, parsed);
            if (accepted)
            {
                std::fprintf(stderr, "FAIL malformed identity was accepted: %s\n", token);
                ++gFailures;
            }
            Expect(parsed.text == "sentinel", "malformed identity does not rewrite the output");
        }
        const std::string tooLong =
            "characters/" + std::string(gameplay::kMaxGameplayDefinitionNameLength + 1, 'a');
        Expect(!gameplay::TryParseGameplayIdentity(tooLong, parsed), "overlong name is rejected");
    }

    {
        gameplay::GameplayDefinitionRegistry registry;
        const gameplay::GameplayDefinition key =
            MakeDefinition("items/master_key", {{GameplayStatId::InteractionRange, 2.5f}});
        const auto first = registry.Register(key);
        Expect(first.status == gameplay::RegisterGameplayDefinitionStatus::Registered, "first register");
        const auto duplicate = registry.Register(key);
        Expect(
            duplicate.status == gameplay::RegisterGameplayDefinitionStatus::DuplicateIdentity,
            "duplicate identity is rejected");
        Expect(registry.Count() == 1, "duplicate register does not add a second row");
        Expect(
            gameplay::GameplayDefinitionStat(*registry.Find("items/master_key"), GameplayStatId::InteractionRange)
                == 2.5f,
            "first definition remains after duplicate rejection");

        gameplay::GameplayDefinition mismatched = key;
        mismatched.category = GameplayDefinitionCategory::Character;
        mismatched.identity = "items/health_potion";
        const auto mismatch = registry.Register(mismatched);
        Expect(
            mismatch.status == gameplay::RegisterGameplayDefinitionStatus::CategoryMismatch,
            "category mismatch is rejected at register");
        Expect(registry.Find("items/health_potion") == nullptr, "mismatched definition is not stored");
    }

    {
        gameplay::GameplayDefinitionRegistry registry;
        Expect(
            registry.Register(MakeDefinition(
                            "characters/guard",
                            {{GameplayStatId::MaxHealth, 40.0f}, {GameplayStatId::AttackPower, 8.0f}}))
                    .status
                == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "register guard");
        Expect(
            registry.Register(MakeDefinition("items/master_key", {{GameplayStatId::InteractionRange, 2.5f}}))
                    .status
                == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "register key");

        const gameplay::GameplayDefinition* key = registry.Find("items/master_key");
        Expect(key != nullptr, "lookup by identity");
        Expect(key->category == GameplayDefinitionCategory::Item, "lookup preserves Item");
        Expect(key->identity == "items/master_key", "lookup identity is the authored key");
        const gameplay::GameplayDefinition* guard = registry.Find("characters/guard");
        Expect(guard != nullptr && guard->category == GameplayDefinitionCategory::Character,
            "lookup preserves Character");
        Expect(registry.Find("items/missing_relic") == nullptr, "missing lookup is null");
        Expect(registry.Find("items/Master_Key") == nullptr, "lookup does not fold case");
        Expect(registry.Find("") == nullptr, "empty lookup is null");
    }

    {
        gameplay::GameplayDefinitionRegistry registry;
        Expect(
            registry.Register(MakeDefinition("items/master_key", {{GameplayStatId::MaxHealth, 1.0f}})).status
                == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "reference registry key");
        Expect(
            registry.Register(MakeDefinition("characters/guard", {{GameplayStatId::MaxHealth, 40.0f}})).status
                == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "reference registry guard");

        gameplay::GameplayDefinitionReference missing;
        missing.identity = "items/missing_relic";
        const auto missingResult = registry.Resolve(missing, GameplayDefinitionCategory::Item);
        Expect(missingResult.status == GameplayReferenceStatus::Missing, "missing reference");
        Expect(missingResult.definition == nullptr, "missing reference does not bind a definition");
        Expect(!missingResult.index.has_value(), "missing reference has no index");
        Expect(registry.Find("items/master_key") != nullptr, "missing reference leaves other definitions");

        gameplay::GameplayDefinitionReference malformed;
        malformed.identity = "items/Master_Key";
        const auto malformedResult = registry.Resolve(malformed, GameplayDefinitionCategory::Item);
        Expect(malformedResult.status == GameplayReferenceStatus::Malformed, "malformed reference");
        Expect(malformedResult.definition == nullptr, "malformed reference does not bind");

        gameplay::GameplayDefinitionReference none;
        Expect(none.IsNone(), "empty reference is None");
        const auto noneResult = registry.Resolve(none, GameplayDefinitionCategory::Item);
        Expect(noneResult.status == GameplayReferenceStatus::None, "None reference status");
        Expect(noneResult.definition == nullptr, "None reference does not bind");

        gameplay::GameplayDefinitionReference guard;
        guard.identity = "characters/guard";
        const std::string guardText = guard.identity;
        const auto mismatch = registry.Resolve(guard, GameplayDefinitionCategory::Item);
        Expect(mismatch.status == GameplayReferenceStatus::CategoryMismatch, "category mismatch");
        Expect(mismatch.definition == nullptr, "category mismatch does not bind the other category");
        Expect(guard.identity == guardText, "resolve does not mutate the authored identity");
        const auto matched = registry.Resolve(guard, GameplayDefinitionCategory::Character);
        Expect(matched.status == GameplayReferenceStatus::Resolved, "matching category resolves");
        Expect(matched.definition != nullptr && matched.definition->identity == "characters/guard",
            "resolved definition keeps its identity");
        Expect(guard.identity == guardText, "successful resolve does not mutate the authored identity");
    }

    {
        const gameplay::GameplayStatId ids[] = {
            GameplayStatId::MaxHealth,
            GameplayStatId::MoveSpeed,
            GameplayStatId::JumpStrength,
            GameplayStatId::GravityScale,
            GameplayStatId::AttackPower,
            GameplayStatId::Defense,
            GameplayStatId::InteractionRange,
        };
        for (const GameplayStatId id : ids)
        {
            const std::string_view name = gameplay::GameplayStatName(id);
            const auto parsed = gameplay::GameplayStatIdFromName(name);
            Expect(parsed.has_value() && *parsed == id, "stat name round-trip");
        }
        Expect(!gameplay::GameplayStatIdFromName("maxHealth").has_value(), "stat names are case-sensitive");
        Expect(!gameplay::GameplayStatIdFromName("Health").has_value(), "unknown stat name");
        Expect(!gameplay::GameplayStatIdFromName("").has_value(), "empty stat name");
        Expect(gameplay::IsValidGameplayStatValue(0.0f), "zero stat value is valid");
        Expect(gameplay::IsValidGameplayStatValue(1.6732f), "finite positive stat value is valid");
        Expect(!gameplay::IsValidGameplayStatValue(-1.0f), "negative stat value is invalid");
        Expect(
            !gameplay::IsValidGameplayStatValue(std::numeric_limits<float>::infinity()),
            "infinite stat value is invalid");
        Expect(
            !gameplay::IsValidGameplayStatValue(std::numeric_limits<float>::quiet_NaN()),
            "NaN stat value is invalid");
        gameplay::GameplayDefinition definition =
            MakeDefinition("items/health_potion", {{GameplayStatId::MaxHealth, 25.0f}});
        Expect(
            gameplay::TrySetGameplayStat(definition, GameplayStatId::MaxHealth, 30.0f)
                == gameplay::SetGameplayStatStatus::Duplicate,
            "duplicate stat does not overwrite");
        Expect(gameplay::GameplayDefinitionStat(definition, GameplayStatId::MaxHealth) == 25.0f,
            "original stat remains");
        Expect(
            gameplay::TrySetGameplayStat(definition, GameplayStatId::MoveSpeed, -1.0f)
                == gameplay::SetGameplayStatStatus::InvalidValue,
            "negative stat assignment is rejected");
    }

    {
        const gameplay::GameplayStatModifier equipment[] = {
            {GameplayStatId::MaxHealth, 2.0f},
            {GameplayStatId::Defense, 9.0f},
            {GameplayStatId::MaxHealth, 1.0f},
        };
        const gameplay::GameplayStatModifier temporary[] = {
            {GameplayStatId::MaxHealth, 3.0f},
            {GameplayStatId::MoveSpeed, 100.0f},
        };
        const gameplay::GameplayStatEvaluation health = gameplay::EvaluateGameplayStat(
            10.0f, GameplayStatId::MaxHealth, equipment, temporary);
        Expect(health.valid && health.effective == 16.0f, "modifiers sum base + equipment + temporary");
        const gameplay::GameplayStatEvaluation defense = gameplay::EvaluateGameplayStat(
            1.0f, GameplayStatId::Defense, equipment, temporary);
        Expect(defense.valid && defense.effective == 10.0f, "unrelated modifiers are ignored");
        const gameplay::GameplayStatModifier penalty[] = {{GameplayStatId::AttackPower, -4.0f}};
        const gameplay::GameplayStatEvaluation attack = gameplay::EvaluateGameplayStat(
            10.0f, GameplayStatId::AttackPower, penalty, {});
        Expect(attack.valid && attack.effective == 6.0f, "negative addend is applied");
        const gameplay::GameplayStatEvaluation bare =
            gameplay::EvaluateGameplayStat(10.0f, GameplayStatId::JumpStrength, {}, {});
        Expect(bare.valid && bare.effective == 10.0f, "no modifiers leaves the base");
        const gameplay::GameplayStatModifier nonFinite[] = {
            {GameplayStatId::MaxHealth, std::numeric_limits<float>::infinity()},
        };
        const gameplay::GameplayStatEvaluation bad = gameplay::EvaluateGameplayStat(
            10.0f, GameplayStatId::MaxHealth, nonFinite, {});
        Expect(!bad.valid, "non-finite modifier is invalid");
        const gameplay::GameplayStatEvaluation badBase = gameplay::EvaluateGameplayStat(
            std::numeric_limits<float>::quiet_NaN(), GameplayStatId::MaxHealth, {}, {});
        Expect(!badBase.valid, "non-finite base is invalid");
    }

    {
        const gameplay::GameplayDefinition player = MakeDefinition(
            "characters/player",
            {{GameplayStatId::MaxHealth, 100.0f}, {GameplayStatId::MoveSpeed, 6.0f}});
        const gameplay::GameplayDefinition guard =
            MakeDefinition("characters/guard", {{GameplayStatId::AttackPower, 8.0f}});

        gameplay::GameplayDefinitionRegistry forward;
        Expect(forward.Register(player).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "forward player");
        Expect(forward.Register(guard).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "forward guard");
        gameplay::GameplayDefinitionRegistry reverse;
        Expect(reverse.Register(guard).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "reverse guard");
        Expect(reverse.Register(player).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "reverse player");

        gameplay::GameplayDefinitionReference reference;
        reference.identity = "characters/player";
        const std::string authored = reference.identity;
        const auto forwardHit = forward.Resolve(reference, GameplayDefinitionCategory::Character);
        const auto reverseHit = reverse.Resolve(reference, GameplayDefinitionCategory::Character);
        Expect(forwardHit.status == GameplayReferenceStatus::Resolved, "forward resolve");
        Expect(reverseHit.status == GameplayReferenceStatus::Resolved, "reverse resolve");
        Expect(forwardHit.definition != nullptr && reverseHit.definition != nullptr, "both hits bind");
        Expect(SameDefinition(*forwardHit.definition, *reverseHit.definition),
            "registry order does not change the resolved definition");
        Expect(forwardHit.index.has_value() && reverseHit.index.has_value() && *forwardHit.index != *reverseHit.index,
            "runtime index may follow insertion order");
        Expect(reference.identity == authored, "reorder resolve does not mutate the authored identity");
        Expect(forward.Find("characters/guard")->identity == "characters/guard", "other identity stays put");
    }

    {
        const char* text =
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition items/health_potion\n"
            "stat MaxHealth 1.6732\n"
            "stat Defense 0\n"
            "definition characters/guard\n"
            "stat AttackPower 8\n";
        const auto parsed = gameplay::ParseGameplayDefinitionsText(text);
        Expect(parsed.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "parse sample");
        Expect(parsed.registry.Count() == 2, "sample definition count");
        const auto written = gameplay::WriteGameplayDefinitionsText(parsed.registry);
        Expect(written.ok, "write sample");
        const auto reparsed = gameplay::ParseGameplayDefinitionsText(written.text);
        Expect(reparsed.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "reparse sample");
        Expect(SameRegistry(parsed.registry, reparsed.registry), "serialization round-trip");
        const auto rewritten = gameplay::WriteGameplayDefinitionsText(reparsed.registry);
        Expect(rewritten.ok && rewritten.text == written.text, "writer output is stable");
        Expect(
            gameplay::GameplayDefinitionStat(
                *reparsed.registry.Find("items/health_potion"), GameplayStatId::MaxHealth)
                == 1.6732f,
            "float round-trip keeps 1.6732");

        const char* crlf =
            "PLATFORMER_GAMEPLAY_DEFINITIONS\r\n"
            "definition items/master_key\r\n"
            "stat InteractionRange 2.5\r\n";
        const auto crlfParsed = gameplay::ParseGameplayDefinitionsText(crlf);
        Expect(crlfParsed.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "CRLF parse");
        Expect(
            gameplay::GameplayDefinitionStat(
                *crlfParsed.registry.Find("items/master_key"), GameplayStatId::InteractionRange)
                == 2.5f,
            "CRLF stat value");
    }

    {
        const auto unknown = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition items/master_key\nstat Speed 1\n");
        Expect(unknown.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "unknown stat");
        Expect(unknown.error == "unknown stat", "unknown stat error");
        Expect(unknown.registry.Count() == 0, "unknown stat does not keep a partial catalog");

        const auto negative = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition items/master_key\nstat MaxHealth -1\n");
        Expect(negative.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "negative stat");
        Expect(negative.error == "invalid stat value", "negative stat error");

        const auto nan = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition items/master_key\nstat MaxHealth nan\n");
        Expect(nan.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "nan stat");

        const auto inf = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition items/master_key\nstat MaxHealth inf\n");
        Expect(inf.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "inf stat");

        const auto duplicate = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition items/master_key\n"
            "definition items/master_key\n");
        Expect(duplicate.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "duplicate file identity");
        Expect(duplicate.error == "duplicate identity", "duplicate file error");
        Expect(duplicate.registry.Count() == 0, "duplicate file does not publish a partial catalog");

        const auto badIdentity = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition key\n");
        Expect(badIdentity.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "bare itemId is not an identity");
        Expect(badIdentity.error == "malformed identity", "malformed identity error");

        const auto versioned = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS 1\ndefinition items/master_key\n");
        Expect(versioned.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "version token is rejected");

        const auto orphan = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\nstat MaxHealth 1\n");
        Expect(orphan.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "stat without definition");
        Expect(orphan.error == "stat without definition", "orphan stat error");
    }

    {
        const auto loaded = gameplay::LoadGameplayDefinitionsFile(PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
        Expect(loaded.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "fixture file loads");
        Expect(loaded.registry.Count() == 4, "fixture definition count");
        const gameplay::GameplayDefinition* key = loaded.registry.Find("items/master_key");
        const gameplay::GameplayDefinition* potion = loaded.registry.Find("items/health_potion");
        const gameplay::GameplayDefinition* player = loaded.registry.Find("characters/player");
        const gameplay::GameplayDefinition* guard = loaded.registry.Find("characters/guard");
        Expect(key != nullptr && key->category == GameplayDefinitionCategory::Item, "fixture key");
        Expect(potion != nullptr && gameplay::GameplayDefinitionStat(*potion, GameplayStatId::MaxHealth) == 25.0f,
            "fixture potion");
        Expect(player != nullptr && player->category == GameplayDefinitionCategory::Character, "fixture player");
        Expect(guard != nullptr && gameplay::GameplayDefinitionStat(*guard, GameplayStatId::Defense) == 2.0f,
            "fixture guard");

        gameplay::GameplayDefinitionReference valid;
        valid.identity = "items/master_key";
        const auto resolved = loaded.registry.Resolve(valid, GameplayDefinitionCategory::Item);
        Expect(resolved.status == GameplayReferenceStatus::Resolved, "fixture valid resolve");
        Expect(resolved.definition != nullptr && resolved.definition->identity == valid.identity,
            "fixture resolve keeps the authored identity");

        gameplay::GameplayDefinitionReference missing;
        missing.identity = "items/missing_relic";
        const auto missingResult = loaded.registry.Resolve(missing, GameplayDefinitionCategory::Item);
        Expect(missingResult.status == GameplayReferenceStatus::Missing, "fixture missing identity");
        Expect(missingResult.definition == nullptr, "fixture missing does not bind");

        const auto written = gameplay::WriteGameplayDefinitionsText(loaded.registry);
        const auto reparsed = gameplay::ParseGameplayDefinitionsText(written.text);
        Expect(written.ok && reparsed.status == gameplay::LoadGameplayDefinitionsStatus::Loaded
                && SameRegistry(loaded.registry, reparsed.registry),
            "fixture round-trip");

        const auto missingFile = gameplay::LoadGameplayDefinitionsFile(
            std::string(PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH) + ".missing");
        Expect(missingFile.status == gameplay::LoadGameplayDefinitionsStatus::Missing, "missing file status");
        Expect(missingFile.registry.Count() == 0, "missing file registry is empty");
    }

    {
        using gameplay::ItemType;
        Expect(gameplay::ItemTypeFromName("Generic") == ItemType::Generic, "Generic type");
        Expect(gameplay::ItemTypeFromName("Consumable") == ItemType::Consumable, "Consumable type");
        Expect(gameplay::ItemTypeFromName("Equipment") == ItemType::Equipment, "Equipment type");
        Expect(gameplay::ItemTypeFromName("Key") == ItemType::Key, "Key type");
        Expect(gameplay::ItemTypeFromName("Quest") == ItemType::Quest, "Quest type");
        Expect(gameplay::ItemTypeName(ItemType::Key) == "Key", "Key name");
        Expect(!gameplay::ItemTypeFromName("key").has_value(), "item type is case-sensitive");
        Expect(!gameplay::ItemTypeFromName("Weapon").has_value(), "unknown item type");
        Expect(gameplay::IsValidItemStack(false, 1), "non-stackable max 1");
        Expect(!gameplay::IsValidItemStack(false, 5), "non-stackable max 5 is invalid");
        Expect(gameplay::IsValidItemStack(true, 1), "stackable max 1 is allowed");
        Expect(gameplay::IsValidItemStack(true, 99), "stackable max 99");
        Expect(!gameplay::IsValidItemStack(true, 0), "max stack 0 is invalid");
        Expect(!gameplay::IsValidItemStack(true, 100), "max stack 100 is invalid");
        gameplay::ItemDefinition item = gameplay::MakeDefaultItemDefinition("items/master_key");
        Expect(item.displayName == "master_key", "default display name is the leaf");
        Expect(item.type == ItemType::Generic, "default item type");
        Expect(!item.stackable && item.maxStack == 1, "default stack");
        Expect(item.worldModelIdentity.empty() && item.iconTextureIdentity.empty(), "default assets none");
        Expect(item.modifiers.empty(), "default modifiers empty");
        Expect(gameplay::ValidateItemDefinition(item) == gameplay::ValidateItemStatus::Valid, "default item valid");
        item.displayName = " Master";
        Expect(gameplay::ValidateItemDefinition(item) == gameplay::ValidateItemStatus::InvalidDisplayName,
            "leading space display name");
        item = gameplay::MakeDefaultItemDefinition("items/master_key");
        item.stackable = false;
        item.maxStack = 4;
        Expect(gameplay::ValidateItemDefinition(item) == gameplay::ValidateItemStatus::InvalidStack,
            "contradictory stack");
        item = gameplay::MakeDefaultItemDefinition("items/master_key");
        item.worldModelIdentity = "not-a-model";
        Expect(gameplay::ValidateItemDefinition(item) == gameplay::ValidateItemStatus::InvalidWorldModel,
            "invalid world model identity");
        item = gameplay::MakeDefaultItemDefinition("items/master_key");
        item.iconTextureIdentity = "models/crate.glb";
        Expect(gameplay::ValidateItemDefinition(item) == gameplay::ValidateItemStatus::InvalidIcon,
            "model path is not an icon");
        item = gameplay::MakeDefaultItemDefinition("items/master_key");
        item.modifiers.push_back({GameplayStatId::MaxHealth, std::numeric_limits<float>::quiet_NaN()});
        Expect(gameplay::ValidateItemDefinition(item) == gameplay::ValidateItemStatus::InvalidModifier,
            "NaN modifier");
    }

    {
        const char* text =
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition items/health_potion\n"
            "display_name \"Health Potion\"\n"
            "description \"Restores health.\"\n"
            "item_type Consumable\n"
            "stackable true\n"
            "max_stack 10\n"
            "world_model models/missing_item.glb\n"
            "icon textures/missing_icon.png\n"
            "modifier MaxHealth 25\n"
            "modifier Defense 1\n"
            "modifier MaxHealth 5\n"
            "stat MaxHealth 25\n"
            "definition characters/guard\n"
            "stat AttackPower 8\n";
        const auto parsed = gameplay::ParseGameplayDefinitionsText(text);
        Expect(parsed.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "parse item payload");
        const gameplay::GameplayDefinition* potion = parsed.registry.Find("items/health_potion");
        Expect(potion != nullptr && potion->item.displayName == "Health Potion", "item display name");
        Expect(potion->item.type == gameplay::ItemType::Consumable, "item type persisted");
        Expect(potion->item.stackable && potion->item.maxStack == 10, "stack persisted");
        Expect(potion->item.worldModelIdentity == "models/missing_item.glb", "missing model identity kept");
        Expect(potion->item.iconTextureIdentity == "textures/missing_icon.png", "missing icon identity kept");
        Expect(potion->item.modifiers.size() == 3, "three authored modifiers");
        Expect(potion->item.modifiers[0].stat == GameplayStatId::MaxHealth && potion->item.modifiers[0].addend == 25.0f,
            "first modifier");
        Expect(potion->item.modifiers[2].stat == GameplayStatId::MaxHealth && potion->item.modifiers[2].addend == 5.0f,
            "duplicate-stat modifiers keep authored order");
        Expect(gameplay::GameplayDefinitionStat(*potion, GameplayStatId::MaxHealth) == 25.0f, "M98 base stat remains");
        const gameplay::GameplayDefinition* guard = parsed.registry.Find("characters/guard");
        Expect(guard != nullptr && guard->category == GameplayDefinitionCategory::Character, "character remains");
        Expect(guard->item.displayName.empty(), "character has no item payload");

        const auto written = gameplay::WriteGameplayDefinitionsText(parsed.registry);
        Expect(written.ok, "write item payload");
        const auto reparsed = gameplay::ParseGameplayDefinitionsText(written.text);
        Expect(reparsed.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "reparse item payload");
        const gameplay::GameplayDefinition* again = reparsed.registry.Find("items/health_potion");
        Expect(again != nullptr && gameplay::ItemDefinitionsEqual(potion->item, again->item),
            "item serialization round-trip");
        Expect(gameplay::GameplayDefinitionStat(*again, GameplayStatId::MaxHealth) == 25.0f,
            "item base stat round-trip");
        Expect(written.text.find("world_model \"models/missing_item.glb\"") != std::string::npos,
            "writer quotes world_model");
        Expect(written.text.find("icon \"textures/missing_icon.png\"") != std::string::npos,
            "writer quotes icon");
        const auto rewritten = gameplay::WriteGameplayDefinitionsText(reparsed.registry);
        Expect(rewritten.ok && rewritten.text == written.text, "item writer output is stable");
    }

    {
        constexpr char kSpacedModel[] = "models/Chest by Quaternius - O72u4Drp8k.glb";
        const char* observedSave =
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition items/health_potion\n"
            "display_name \"health_potion\"\n"
            "description \"\"\n"
            "item_type Generic\n"
            "stackable false\n"
            "max_stack 1\n"
            "stat MaxHealth 25\n"
            "definition items/bau\n"
            "display_name \"bau\"\n"
            "description \"\"\n"
            "item_type Generic\n"
            "stackable true\n"
            "max_stack 3\n"
            "world_model models/Chest by Quaternius - O72u4Drp8k.glb\n";
        const auto observed = gameplay::ParseGameplayDefinitionsText(observedSave);
        Expect(observed.status == gameplay::LoadGameplayDefinitionsStatus::Loaded,
            "observed unquoted spaced world_model loads");
        Expect(observed.error.empty(), "observed spaced world_model is not wrong field count");
        const gameplay::GameplayDefinition* bau = observed.registry.Find("items/bau");
        Expect(bau != nullptr && bau->item.worldModelIdentity == kSpacedModel,
            "unquoted spaced world_model identity is one field");

        const char* quotedSpaced =
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition items/chest_key\n"
            "world_model \"models/Chest by Quaternius - O72u4Drp8k.glb\"\n"
            "icon \"textures/missing_icon.png\"\n";
        const auto quoted = gameplay::ParseGameplayDefinitionsText(quotedSpaced);
        Expect(quoted.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "quoted spaced world_model");
        const gameplay::GameplayDefinition* chestKey = quoted.registry.Find("items/chest_key");
        Expect(chestKey != nullptr && chestKey->item.worldModelIdentity == kSpacedModel,
            "quoted spaced world_model identity");
        Expect(chestKey->item.iconTextureIdentity == "textures/missing_icon.png", "quoted icon identity");

        const auto writtenSpaced = gameplay::WriteGameplayDefinitionsText(quoted.registry);
        Expect(writtenSpaced.ok, "write spaced world_model");
        Expect(
            writtenSpaced.text.find("world_model \"models/Chest by Quaternius - O72u4Drp8k.glb\"")
                != std::string::npos,
            "writer quotes spaced world_model");
        const auto reparsedSpaced = gameplay::ParseGameplayDefinitionsText(writtenSpaced.text);
        Expect(reparsedSpaced.status == gameplay::LoadGameplayDefinitionsStatus::Loaded,
            "quoted spaced world_model round-trip parse");
        const gameplay::GameplayDefinition* reparsedChest =
            reparsedSpaced.registry.Find("items/chest_key");
        Expect(
            reparsedChest != nullptr && reparsedChest->item.worldModelIdentity == kSpacedModel,
            "quoted spaced world_model round-trip identity");

        const auto quotedExtra = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition items/master_key\n"
            "world_model \"models/crate.glb\" leftover\n");
        Expect(quotedExtra.status == gameplay::LoadGameplayDefinitionsStatus::Invalid,
            "quoted world_model rejects trailing tokens");
        Expect(quotedExtra.error == "invalid world model", "quoted extra is not silently accepted");
        Expect(quotedExtra.registry.Count() == 0, "quoted extra does not keep a partial catalog");

        const auto unquotedExtra = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition items/master_key\n"
            "world_model models/crate.glb leftover\n");
        Expect(unquotedExtra.status == gameplay::LoadGameplayDefinitionsStatus::Invalid,
            "unquoted remainder with extra tokens is invalid");
        Expect(unquotedExtra.error == "invalid world model", "unquoted extra fails identity validation");
    }

    {
        const auto unknownType = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition items/master_key\nitem_type Weapon\n");
        Expect(unknownType.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "unknown item type");
        Expect(unknownType.error == "unknown item type", "unknown item type error");
        Expect(unknownType.registry.Count() == 0, "unknown item type does not keep a partial catalog");

        const auto badStack = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition items/master_key\nstackable false\nmax_stack 5\n");
        Expect(badStack.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "contradictory stack parse");
        Expect(badStack.error == "InvalidStack", "contradictory stack error");

        const auto onCharacter = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition characters/player\nitem_type Key\n");
        Expect(onCharacter.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "item field on character");
        Expect(onCharacter.error == "item field on Character", "item field on character error");

        const auto unknownMod = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition items/master_key\nmodifier Speed 1\n");
        Expect(unknownMod.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "unknown modifier stat");
        Expect(unknownMod.error == "unknown stat", "unknown modifier stat error");

        const auto infMod = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition items/master_key\nmodifier MaxHealth inf\n");
        Expect(infMod.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "inf modifier");
        Expect(infMod.error == "invalid modifier", "inf modifier error");
    }

    {
        const auto loaded = gameplay::LoadGameplayDefinitionsFile(PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
        Expect(loaded.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "fixture still loads");
        const gameplay::GameplayDefinition* key = loaded.registry.Find("items/master_key");
        const gameplay::GameplayDefinition* player = loaded.registry.Find("characters/player");
        Expect(key != nullptr && key->item.displayName == "master_key", "M98 item gets default display name");
        Expect(key->item.type == gameplay::ItemType::Generic, "M98 item type default Generic");
        Expect(player != nullptr && player->category == GameplayDefinitionCategory::Character,
            "M98 character compatibility");
        Expect(player->item.displayName.empty(), "M98 character has empty item payload");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d GameplayDefinition test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("GameplayDefinition tests passed.\n");
    return 0;
}
