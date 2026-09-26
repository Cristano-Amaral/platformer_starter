#include "gameplay/GameplayDefinitionFile.h"

#include "platform/FileReplace.h"

#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <system_error>
#include <vector>

namespace gameplay
{
namespace
{
ParseGameplayDefinitionsResult MakeStatus(
    LoadGameplayDefinitionsStatus status,
    int line,
    std::string error)
{
    ParseGameplayDefinitionsResult result;
    result.status = status;
    result.errorLine = line;
    result.error = std::move(error);
    return result;
}

void BestEffortRemove(const std::filesystem::path& path)
{
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

std::string_view TrimAsciiSpace(std::string_view line)
{
    if (!line.empty() && line.back() == '\r')
    {
        line.remove_suffix(1);
    }
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
    {
        line.remove_prefix(1);
    }
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t'))
    {
        line.remove_suffix(1);
    }
    return line;
}

bool SplitTokens(std::string_view line, std::vector<std::string_view>& tokens)
{
    tokens.clear();
    std::size_t index = 0;
    while (index < line.size())
    {
        while (index < line.size() && (line[index] == ' ' || line[index] == '\t'))
        {
            ++index;
        }
        if (index >= line.size())
        {
            break;
        }
        const std::size_t start = index;
        while (index < line.size() && line[index] != ' ' && line[index] != '\t')
        {
            ++index;
        }
        tokens.push_back(line.substr(start, index - start));
    }
    return !tokens.empty();
}

bool ParseFiniteFloat(std::string_view token, float& value)
{
    if (token.empty())
    {
        return false;
    }
    float parsed = 0.0f;
    const std::from_chars_result result =
        std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size() || !std::isfinite(parsed))
    {
        return false;
    }
    value = parsed;
    return true;
}

bool ParseBoundedInt(std::string_view token, int& value)
{
    if (token.empty())
    {
        return false;
    }
    int parsed = 0;
    const std::from_chars_result result =
        std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size())
    {
        return false;
    }
    value = parsed;
    return true;
}

bool AppendFloat(std::string& out, float value)
{
    std::array<char, 64> buffer{};
    const std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec != std::errc{})
    {
        return false;
    }
    out.append(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
    return true;
}

bool AppendInt(std::string& out, int value)
{
    std::array<char, 32> buffer{};
    const std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec != std::errc{})
    {
        return false;
    }
    out.append(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
    return true;
}

bool ParseQuotedString(std::string_view remainder, std::string& value)
{
    remainder = TrimAsciiSpace(remainder);
    if (remainder.size() < 2 || remainder.front() != '"' || remainder.back() != '"')
    {
        return false;
    }
    const std::string_view inner = remainder.substr(1, remainder.size() - 2);
    for (const char character : inner)
    {
        if (!ItemAuthoredTextCharIsAllowed(character))
        {
            return false;
        }
    }
    value.assign(inner);
    return true;
}

bool ConsumeKeywordRemainder(
    std::string_view line,
    std::string_view keyword,
    std::string_view& remainder)
{
    if (!line.starts_with(keyword))
    {
        return false;
    }
    remainder = line.substr(keyword.size());
    if (!remainder.empty() && remainder.front() != ' ' && remainder.front() != '\t')
    {
        return false;
    }
    remainder = TrimAsciiSpace(remainder);
    return true;
}

// Quoted remainder is the writer form. Unquoted remainder is the rest of the
// line as one identity, so catalog paths that contain spaces still parse.
bool ParseIdentityRemainder(std::string_view remainder, std::string& value)
{
    if (remainder.empty())
    {
        return false;
    }
    if (remainder.front() == '"')
    {
        return ParseQuotedString(remainder, value);
    }
    value.assign(remainder);
    return true;
}

void AppendQuotedString(std::string& out, std::string_view value)
{
    out.push_back('"');
    out.append(value);
    out.push_back('"');
}

struct ItemFieldFlags
{
    bool displayName = false;
    bool description = false;
    bool type = false;
    bool stackable = false;
    bool maxStack = false;
    bool equipmentSlot = false;
    bool attachmentJoint = false;
    bool attachmentTranslation = false;
    bool attachmentRotation = false;
    bool attachmentScale = false;
    bool worldModel = false;
    bool icon = false;
    bool characterType = false;
    bool animationIdle = false;
    bool animationMove = false;
    bool animationJump = false;
    bool animationIdleAsset = false;
    bool animationMoveAsset = false;
    bool animationJumpAsset = false;
    bool sourceAsset = false;
    bool sourceClip = false;
    bool playback = false;
};

RegisterGameplayDefinitionResult FinishDefinition(
    GameplayDefinitionRegistry& registry,
    GameplayDefinition& current,
    ItemFieldFlags& flags,
    bool& haveCurrent)
{
    RegisterGameplayDefinitionResult result;
    result.status = RegisterGameplayDefinitionStatus::Registered;
    if (!haveCurrent)
    {
        return result;
    }
    if (current.category == GameplayDefinitionCategory::Item)
    {
        if (!flags.displayName)
        {
            current.item.displayName = DefaultItemDisplayName(current.identity);
        }
        result = registry.Register(current);
    }
    else if (current.category == GameplayDefinitionCategory::Character)
    {
        current.item = {};
        if (!flags.displayName)
        {
            current.character.displayName = DefaultCharacterDisplayName(current.identity);
        }
        result = registry.Register(current);
    }
    else
    {
        current.item = {};
        current.character = {};
        result = registry.Register(current);
    }
    haveCurrent = false;
    current = {};
    flags = {};
    return result;
}

bool RequireItem(const GameplayDefinition& current, bool haveCurrent)
{
    return haveCurrent && current.category == GameplayDefinitionCategory::Item;
}

bool RequireCharacter(const GameplayDefinition& current, bool haveCurrent)
{
    return haveCurrent && current.category == GameplayDefinitionCategory::Character;
}

bool RequireAnimation(const GameplayDefinition& current, bool haveCurrent)
{
    return haveCurrent && current.category == GameplayDefinitionCategory::Animation;
}
}

ParseGameplayDefinitionsResult ParseGameplayDefinitionsText(std::string_view text)
{
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF
        && static_cast<unsigned char>(text[1]) == 0xBB
        && static_cast<unsigned char>(text[2]) == 0xBF)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, 1, "UTF-8 BOM is not allowed");
    }
    if (text.size() > kMaxGameplayDefinitionsFileBytes)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, 0, "file too large");
    }

    if (text.empty())
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, 0, "empty file");
    }

    GameplayDefinitionRegistry registry;
    GameplayDefinition current;
    ItemFieldFlags flags;
    bool haveCurrent = false;
    bool sawHeader = false;
    int lineNumber = 0;
    int nonBlankLines = 0;
    std::size_t cursor = 0;
    std::vector<std::string_view> tokens;

    while (cursor < text.size())
    {
        const std::size_t newline = text.find('\n', cursor);
        const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
        std::string_view line = TrimAsciiSpace(text.substr(cursor, end - cursor));
        const bool last = newline == std::string_view::npos;
        cursor = last ? text.size() : newline + 1;

        ++lineNumber;
        if (lineNumber > static_cast<int>(kMaxGameplayDefinitionsLines))
        {
            return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "too many lines");
        }
        if (line.size() > kMaxGameplayDefinitionsLineLength)
        {
            return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "line too long");
        }
        if (line.empty())
        {
            continue;
        }

        ++nonBlankLines;
        if (!SplitTokens(line, tokens))
        {
            return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "empty line");
        }

        if (!sawHeader)
        {
            if (tokens.size() != 1 || tokens[0] != kGameplayDefinitionsMagic)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong magic");
            }
            sawHeader = true;
            continue;
        }

        if (tokens[0] == "definition")
        {
            if (tokens.size() != 2)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong field count");
            }
            const RegisterGameplayDefinitionResult finished =
                FinishDefinition(registry, current, flags, haveCurrent);
            if (finished.status != RegisterGameplayDefinitionStatus::Registered)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, finished.error);
            }
            if (registry.Count() >= kMaxGameplayDefinitions)
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid, lineNumber, "too many definitions");
            }
            ParsedGameplayIdentity parsed;
            if (!TryParseGameplayIdentity(tokens[1], parsed))
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "malformed identity");
            }
            current = {};
            current.identity = parsed.text;
            current.category = parsed.category;
            if (parsed.category == GameplayDefinitionCategory::Item)
            {
                current.item = MakeDefaultItemDefinition(parsed.text);
                current.item.displayName.clear();
            }
            else if (parsed.category == GameplayDefinitionCategory::Character)
            {
                current.character = MakeDefaultCharacterDefinition(parsed.text);
                current.character.displayName.clear();
            }
            flags = {};
            haveCurrent = true;
        }
        else if (tokens[0] == "stat")
        {
            if (tokens.size() != 3)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong field count");
            }
            if (!haveCurrent)
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid, lineNumber, "stat without definition");
            }
            const std::optional<GameplayStatId> stat = GameplayStatIdFromName(tokens[1]);
            if (!stat.has_value())
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "unknown stat");
            }
            float value = 0.0f;
            if (!ParseFiniteFloat(tokens[2], value) || !IsValidGameplayStatValue(value))
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid stat value");
            }
            const SetGameplayStatStatus set = TrySetGameplayStat(current, *stat, value);
            if (set == SetGameplayStatStatus::Duplicate)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate stat");
            }
            if (set != SetGameplayStatStatus::Set)
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid stat value");
            }
        }
        else if (tokens[0] == "display_name")
        {
            if (!haveCurrent)
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid,
                    lineNumber,
                    "display_name without definition");
            }
            if (flags.displayName)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate display_name");
            }
            const std::size_t keywordEnd = line.find("display_name");
            std::string value;
            if (keywordEnd == std::string_view::npos
                || !ParseQuotedString(line.substr(keywordEnd + std::string_view("display_name").size()), value)
                || (current.category == GameplayDefinitionCategory::Item
                    ? !IsValidItemDisplayName(value) : !IsValidCharacterDisplayName(value)))
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid display name");
            }
            if (current.category == GameplayDefinitionCategory::Item) current.item.displayName = std::move(value);
            else current.character.displayName = std::move(value);
            flags.displayName = true;
        }
        else if (tokens[0] == "description")
        {
            if (!haveCurrent)
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid,
                    lineNumber,
                    "description without definition");
            }
            if (flags.description)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate description");
            }
            const std::size_t keywordEnd = line.find("description");
            std::string value;
            if (keywordEnd == std::string_view::npos
                || !ParseQuotedString(line.substr(keywordEnd + std::string_view("description").size()), value)
                || (current.category == GameplayDefinitionCategory::Item
                    ? !IsValidItemDescription(value) : !IsValidCharacterDescription(value)))
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid description");
            }
            if (current.category == GameplayDefinitionCategory::Item) current.item.description = std::move(value);
            else current.character.description = std::move(value);
            flags.description = true;
        }
        else if (tokens[0] == "item_type")
        {
            if (tokens.size() != 2)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong field count");
            }
            if (!RequireItem(current, haveCurrent))
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid,
                    lineNumber,
                    haveCurrent ? "item field on Character" : "item field without definition");
            }
            if (flags.type)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate item_type");
            }
            const std::optional<ItemType> type = ItemTypeFromName(tokens[1]);
            if (!type.has_value())
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "unknown item type");
            }
            current.item.type = *type;
            flags.type = true;
        }
        else if (tokens[0] == "character_type")
        {
            if (tokens.size() != 2)
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong field count");
            if (!RequireCharacter(current, haveCurrent))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber,
                    haveCurrent ? "character field on Item" : "character field without definition");
            if (flags.characterType)
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate character_type");
            const std::optional<CharacterType> type = CharacterTypeFromName(tokens[1]);
            if (!type.has_value())
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "unknown character type");
            current.character.type = *type;
            flags.characterType = true;
        }
        else if (tokens[0] == "stackable")
        {
            if (tokens.size() != 2)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong field count");
            }
            if (!RequireItem(current, haveCurrent))
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid,
                    lineNumber,
                    haveCurrent ? "item field on Character" : "item field without definition");
            }
            if (flags.stackable)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate stackable");
            }
            if (tokens[1] == "true")
            {
                current.item.stackable = true;
            }
            else if (tokens[1] == "false")
            {
                current.item.stackable = false;
            }
            else
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid stackable");
            }
            flags.stackable = true;
        }
        else if (tokens[0] == "max_stack")
        {
            if (tokens.size() != 2)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong field count");
            }
            if (!RequireItem(current, haveCurrent))
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid,
                    lineNumber,
                    haveCurrent ? "item field on Character" : "item field without definition");
            }
            if (flags.maxStack)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate max_stack");
            }
            int value = 0;
            if (!ParseBoundedInt(tokens[1], value))
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid max stack");
            }
            current.item.maxStack = value;
            flags.maxStack = true;
        }
        else if (tokens[0] == "equipment_slot")
        {
            if (tokens.size() != 2)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong field count");
            }
            if (!RequireItem(current, haveCurrent))
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid,
                    lineNumber,
                    haveCurrent ? "item field on Character" : "item field without definition");
            }
            if (flags.equipmentSlot)
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate equipment_slot");
            }
            const std::optional<EquipmentSlot> slot = EquipmentSlotFromName(tokens[1]);
            if (!slot.has_value())
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid, lineNumber, "unknown equipment slot");
            }
            current.item.equipmentSlot = slot;
            flags.equipmentSlot = true;
        }
        else if (tokens[0] == "attachment_joint")
        {
            std::string_view remainder; std::string joint;
            if (!RequireItem(current, haveCurrent))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber,
                    haveCurrent ? "attachment field on Character" : "attachment field without definition");
            if (flags.attachmentJoint || !ConsumeKeywordRemainder(line, "attachment_joint", remainder)
                || !ParseQuotedString(remainder, joint))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid or duplicate attachment_joint");
            if (!current.item.equipmentAttachment.has_value()) current.item.equipmentAttachment.emplace();
            current.item.equipmentAttachment->jointName = std::move(joint);
            flags.attachmentJoint = true;
        }
        else if (tokens[0] == "attachment_translation" || tokens[0] == "attachment_rotation"
            || tokens[0] == "attachment_scale")
        {
            if (!RequireItem(current, haveCurrent))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber,
                    haveCurrent ? "attachment field on Character" : "attachment field without definition");
            if (tokens.size() != 4)
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong attachment field count");
            bool* flag = tokens[0] == "attachment_translation" ? &flags.attachmentTranslation
                : tokens[0] == "attachment_rotation" ? &flags.attachmentRotation : &flags.attachmentScale;
            core::Vec3 value{};
            if (*flag || !ParseFiniteFloat(tokens[1], value.x) || !ParseFiniteFloat(tokens[2], value.y)
                || !ParseFiniteFloat(tokens[3], value.z))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid or duplicate attachment transform");
            if (!current.item.equipmentAttachment.has_value()) current.item.equipmentAttachment.emplace();
            if (tokens[0] == "attachment_translation") current.item.equipmentAttachment->translation = value;
            else if (tokens[0] == "attachment_rotation") current.item.equipmentAttachment->rotationDegrees = value;
            else current.item.equipmentAttachment->scale = value;
            *flag = true;
        }
        else if (tokens[0] == "world_model")
        {
            std::string_view remainder;
            std::string identity;
            if (!ConsumeKeywordRemainder(line, "world_model", remainder)
                || !ParseIdentityRemainder(remainder, identity))
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid world model");
            }
            if (!haveCurrent)
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid,
                    lineNumber,
                    "world model without definition");
            }
            if (flags.worldModel)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate world_model");
            }
            if (!IsValidItemWorldModelIdentity(identity))
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid world model");
            }
            if (current.category == GameplayDefinitionCategory::Item)
                current.item.worldModelIdentity = std::move(identity);
            else
                current.character.worldModelIdentity = std::move(identity);
            flags.worldModel = true;
        }
        else if (tokens[0] == "animation_idle" || tokens[0] == "animation_move"
            || tokens[0] == "animation_jump")
        {
            if (!RequireCharacter(current, haveCurrent))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber,
                    haveCurrent ? "animation field on Item" : "animation field without definition");
            std::string_view remainder;
            std::string clip;
            if (!ConsumeKeywordRemainder(line, tokens[0], remainder)
                || !ParseIdentityRemainder(remainder, clip)
                || !IsValidCharacterAnimationClipName(clip))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid animation binding");
            bool* flag = tokens[0] == "animation_idle" ? &flags.animationIdle
                : tokens[0] == "animation_move" ? &flags.animationMove : &flags.animationJump;
            if (*flag) return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate animation binding");
            *flag = true;
            std::string* target = tokens[0] == "animation_idle" ? &current.character.animations.idle
                : tokens[0] == "animation_move" ? &current.character.animations.move
                : &current.character.animations.jump;
            *target = std::move(clip);
        }
        else if (tokens[0] == "animation_idle_asset" || tokens[0] == "animation_move_asset"
            || tokens[0] == "animation_jump_asset")
        {
            if (!RequireCharacter(current, haveCurrent))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber,
                    haveCurrent ? "animation asset binding on non-Character" : "animation asset binding without definition");
            std::string_view remainder;
            std::string identity;
            if (!ConsumeKeywordRemainder(line, tokens[0], remainder)
                || !ParseIdentityRemainder(remainder, identity) || !IsValidAnimationIdentity(identity))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid animation asset binding");
            bool* flag = tokens[0] == "animation_idle_asset" ? &flags.animationIdleAsset
                : tokens[0] == "animation_move_asset" ? &flags.animationMoveAsset : &flags.animationJumpAsset;
            if (*flag) return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate animation asset binding");
            *flag = true;
            std::string* target = tokens[0] == "animation_idle_asset" ? &current.character.animations.idleAsset
                : tokens[0] == "animation_move_asset" ? &current.character.animations.moveAsset
                : &current.character.animations.jumpAsset;
            *target = std::move(identity);
        }
        else if (tokens[0] == "source_asset")
        {
            std::string_view remainder; std::string identity;
            if (!RequireAnimation(current, haveCurrent) || flags.sourceAsset
                || !ConsumeKeywordRemainder(line, "source_asset", remainder)
                || !ParseIdentityRemainder(remainder, identity) || !IsValidItemWorldModelIdentity(identity))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid or duplicate animation source asset");
            current.animation.sourceAssetIdentity = std::move(identity); flags.sourceAsset = true;
        }
        else if (tokens[0] == "source_clip")
        {
            std::string_view remainder; std::string clip;
            if (!RequireAnimation(current, haveCurrent) || flags.sourceClip
                || !ConsumeKeywordRemainder(line, "source_clip", remainder)
                || !ParseIdentityRemainder(remainder, clip) || !IsValidAnimationClipName(clip))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid or duplicate animation source clip");
            current.animation.sourceClipName = std::move(clip); flags.sourceClip = true;
        }
        else if (tokens[0] == "playback")
        {
            if (!RequireAnimation(current, haveCurrent) || flags.playback || tokens.size() != 2
                || (tokens[1] != "Loop" && tokens[1] != "Clamp"))
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid or duplicate animation playback");
            current.animation.playbackMode = tokens[1] == "Loop"
                ? animation::PlaybackMode::Loop : animation::PlaybackMode::Clamp;
            flags.playback = true;
        }
        else if (tokens[0] == "icon")
        {
            std::string_view remainder;
            std::string identity;
            if (!ConsumeKeywordRemainder(line, "icon", remainder)
                || !ParseIdentityRemainder(remainder, identity))
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid icon");
            }
            if (!RequireItem(current, haveCurrent))
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid,
                    lineNumber,
                    haveCurrent ? "item field on Character" : "item field without definition");
            }
            if (flags.icon)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "duplicate icon");
            }
            if (!IsValidItemIconTextureIdentity(identity))
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid icon");
            }
            current.item.iconTextureIdentity = std::move(identity);
            flags.icon = true;
        }
        else if (tokens[0] == "modifier")
        {
            if (tokens.size() != 3)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "wrong field count");
            }
            if (!RequireItem(current, haveCurrent))
            {
                return MakeStatus(
                    LoadGameplayDefinitionsStatus::Invalid,
                    lineNumber,
                    haveCurrent ? "item field on Character" : "item field without definition");
            }
            if (current.item.modifiers.size() >= kMaxItemModifiers)
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "too many modifiers");
            }
            const std::optional<GameplayStatId> stat = GameplayStatIdFromName(tokens[1]);
            if (!stat.has_value())
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "unknown stat");
            }
            float addend = 0.0f;
            if (!ParseFiniteFloat(tokens[2], addend) || !IsValidGameplayStatAddend(addend))
            {
                return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "invalid modifier");
            }
            current.item.modifiers.push_back(GameplayStatModifier{*stat, addend});
        }
        else
        {
            return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, "unknown keyword");
        }
    }

    if (nonBlankLines == 0)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, 0, "empty file");
    }
    if (!sawHeader)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, 1, "wrong magic");
    }

    const RegisterGameplayDefinitionResult finished =
        FinishDefinition(registry, current, flags, haveCurrent);
    if (finished.status != RegisterGameplayDefinitionStatus::Registered)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Invalid, lineNumber, finished.error);
    }

    ParseGameplayDefinitionsResult loaded;
    loaded.status = LoadGameplayDefinitionsStatus::Loaded;
    loaded.registry = std::move(registry);
    return loaded;
}

ParseGameplayDefinitionsResult LoadGameplayDefinitionsFile(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Error, 0, "empty path");
    }

    std::error_code existsError;
    const bool exists = std::filesystem::exists(path, existsError);
    if (existsError)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Error, 0, "path query failed");
    }
    if (!exists)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Missing, 0, "missing file");
    }

    std::error_code typeError;
    if (!std::filesystem::is_regular_file(path, typeError) || typeError)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Error, 0, "not a regular file");
    }

    std::error_code sizeError;
    const std::uintmax_t size = std::filesystem::file_size(path, sizeError);
    if (sizeError)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Error, 0, "size query failed");
    }
    if (size > kMaxGameplayDefinitionsFileBytes)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Error, 0, "file too large");
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return MakeStatus(LoadGameplayDefinitionsStatus::Error, 0, "open failed");
    }

    std::string text(static_cast<std::size_t>(size), '\0');
    if (size > 0)
    {
        stream.read(text.data(), static_cast<std::streamsize>(size));
        if (stream.bad() || stream.gcount() != static_cast<std::streamsize>(size))
        {
            return MakeStatus(LoadGameplayDefinitionsStatus::Error, 0, "read failed");
        }
    }
    return ParseGameplayDefinitionsText(text);
}

WriteGameplayDefinitionsResult WriteGameplayDefinitionsText(
    const GameplayDefinitionRegistry& registry)
{
    WriteGameplayDefinitionsResult result;
    result.text.assign(kGameplayDefinitionsMagic);
    result.text += '\n';

    for (const GameplayDefinition& definition : registry.Definitions())
    {
        ParsedGameplayIdentity parsed;
        if (!TryParseGameplayIdentity(definition.identity, parsed)
            || parsed.category != definition.category || parsed.text != definition.identity)
        {
            result.ok = false;
            result.text.clear();
            result.error = "malformed identity";
            return result;
        }
        result.text += "definition ";
        result.text += definition.identity;
        result.text += '\n';

        if (definition.category == GameplayDefinitionCategory::Item)
        {
            if (ValidateItemDefinition(definition.item) != ValidateItemStatus::Valid)
            {
                result.ok = false;
                result.text.clear();
                result.error = "invalid item";
                return result;
            }
            result.text += "display_name ";
            AppendQuotedString(result.text, definition.item.displayName);
            result.text += '\n';
            result.text += "description ";
            AppendQuotedString(result.text, definition.item.description);
            result.text += '\n';
            result.text += "item_type ";
            result.text += ItemTypeName(definition.item.type);
            result.text += '\n';
            result.text += "stackable ";
            result.text += definition.item.stackable ? "true" : "false";
            result.text += '\n';
            result.text += "max_stack ";
            if (!AppendInt(result.text, definition.item.maxStack))
            {
                result.ok = false;
                result.text.clear();
                result.error = "invalid item";
                return result;
            }
            result.text += '\n';
            if (definition.item.type == ItemType::Equipment
                && definition.item.equipmentSlot.has_value())
            {
                result.text += "equipment_slot ";
                result.text += EquipmentSlotName(*definition.item.equipmentSlot);
                result.text += '\n';
            }
            if (definition.item.equipmentAttachment.has_value())
            {
                const EquipmentAttachmentDefinition& attachment = *definition.item.equipmentAttachment;
                result.text += "attachment_joint ";
                AppendQuotedString(result.text, attachment.jointName);
                result.text += '\n';
                const auto appendVec3 = [&result](std::string_view field, core::Vec3 value) {
                    result.text += field; result.text += ' ';
                    return AppendFloat(result.text, value.x) && (result.text += ' ', true)
                        && AppendFloat(result.text, value.y) && (result.text += ' ', true)
                        && AppendFloat(result.text, value.z) && (result.text += '\n', true);
                };
                if (!appendVec3("attachment_translation", attachment.translation)
                    || !appendVec3("attachment_rotation", attachment.rotationDegrees)
                    || !appendVec3("attachment_scale", attachment.scale))
                {
                    result.ok = false; result.text.clear(); result.error = "invalid attachment"; return result;
                }
            }
            if (!definition.item.worldModelIdentity.empty())
            {
                result.text += "world_model ";
                AppendQuotedString(result.text, definition.item.worldModelIdentity);
                result.text += '\n';
            }
            if (!definition.item.iconTextureIdentity.empty())
            {
                result.text += "icon ";
                AppendQuotedString(result.text, definition.item.iconTextureIdentity);
                result.text += '\n';
            }
            for (const GameplayStatModifier& modifier : definition.item.modifiers)
            {
                result.text += "modifier ";
                result.text += GameplayStatName(modifier.stat);
                result.text += ' ';
                if (!AppendFloat(result.text, modifier.addend))
                {
                    result.ok = false;
                    result.text.clear();
                    result.error = "invalid modifier";
                    return result;
                }
                result.text += '\n';
            }
        }
        else if (definition.category == GameplayDefinitionCategory::Character)
        {
            if (ValidateCharacterDefinition(definition.character) != ValidateCharacterStatus::Valid)
            {
                result.ok = false;
                result.text.clear();
                result.error = "invalid character";
                return result;
            }
            result.text += "display_name ";
            AppendQuotedString(result.text, definition.character.displayName);
            result.text += '\n';
            result.text += "description ";
            AppendQuotedString(result.text, definition.character.description);
            result.text += '\n';
            result.text += "character_type ";
            result.text += CharacterTypeName(definition.character.type);
            result.text += '\n';
            if (!definition.character.worldModelIdentity.empty())
            {
                result.text += "world_model ";
                AppendQuotedString(result.text, definition.character.worldModelIdentity);
                result.text += '\n';
            }
            const auto appendAnimation = [&result](std::string_view field, const std::string& clip) {
                if (clip.empty()) return;
                result.text += field;
                result.text += ' ';
                AppendQuotedString(result.text, clip);
                result.text += '\n';
            };
            appendAnimation("animation_idle", definition.character.animations.idle);
            appendAnimation("animation_move", definition.character.animations.move);
            appendAnimation("animation_jump", definition.character.animations.jump);
            appendAnimation("animation_idle_asset", definition.character.animations.idleAsset);
            appendAnimation("animation_move_asset", definition.character.animations.moveAsset);
            appendAnimation("animation_jump_asset", definition.character.animations.jumpAsset);
        }
        else
        {
            if (!ValidateAnimationDefinition(definition.animation))
            {
                result.ok = false; result.text.clear(); result.error = "invalid animation"; return result;
            }
            result.text += "source_asset ";
            AppendQuotedString(result.text, definition.animation.sourceAssetIdentity);
            result.text += '\n';
            result.text += "source_clip ";
            AppendQuotedString(result.text, definition.animation.sourceClipName);
            result.text += '\n';
            result.text += "playback ";
            result.text += definition.animation.playbackMode == animation::PlaybackMode::Loop ? "Loop\n" : "Clamp\n";
        }

        for (std::size_t index = 0; index < kGameplayStatCount; ++index)
        {
            if (!GameplayDefinitionHasStat(definition, static_cast<GameplayStatId>(index)))
            {
                continue;
            }
            const float statValue = *GameplayDefinitionStat(
                definition, static_cast<GameplayStatId>(index));
            if (!IsValidGameplayStatValue(statValue))
            {
                result.ok = false;
                result.text.clear();
                result.error = "invalid stat value";
                return result;
            }
            const std::string_view name =
                GameplayStatName(static_cast<GameplayStatId>(index));
            result.text += "stat ";
            result.text += name;
            result.text += ' ';
            if (!AppendFloat(result.text, statValue))
            {
                result.ok = false;
                result.text.clear();
                result.error = "invalid stat value";
                return result;
            }
            result.text += '\n';
        }
    }

    result.ok = true;
    return result;
}

std::filesystem::path GameplayDefinitionsTemporaryPath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return {};
    }
    std::filesystem::path temporary = path;
    temporary += std::string(kGameplayDefinitionsTemporarySuffix);
    return temporary;
}

SaveGameplayDefinitionsResult SaveGameplayDefinitionsFile(
    const std::filesystem::path& path,
    const GameplayDefinitionRegistry& registry)
{
    SaveGameplayDefinitionsResult result;
    const WriteGameplayDefinitionsResult written = WriteGameplayDefinitionsText(registry);
    if (!written.ok)
    {
        result.error = written.error.empty() ? "invalid catalog" : written.error;
        return result;
    }
    if (path.empty() || !path.is_absolute())
    {
        result.error = "path must be absolute";
        return result;
    }

    const std::filesystem::path temporaryPath = GameplayDefinitionsTemporaryPath(path);
    if (temporaryPath.empty())
    {
        result.error = "temporary path unavailable";
        return result;
    }

    {
        std::ofstream stream(temporaryPath, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!stream)
        {
            result.error = "temporary open failed";
            return result;
        }
        stream.write(written.text.data(), static_cast<std::streamsize>(written.text.size()));
        stream.flush();
        const bool writeOk = static_cast<bool>(stream);
        stream.close();
        if (!writeOk || stream.fail())
        {
            BestEffortRemove(temporaryPath);
            result.error = "temporary write failed";
            return result;
        }
    }

    if (!platform::ReplaceFileWithTemporary(temporaryPath, path))
    {
        BestEffortRemove(temporaryPath);
        result.error = "replace failed";
        return result;
    }

    result.ok = true;
    return result;
}
}
