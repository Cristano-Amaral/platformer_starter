#include "gameplay/GameplayDefinitionFile.h"

#include <array>
#include <charconv>
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

RegisterGameplayDefinitionResult FinishDefinition(
    GameplayDefinitionRegistry& registry,
    GameplayDefinition& current,
    bool& haveCurrent)
{
    RegisterGameplayDefinitionResult result;
    result.status = RegisterGameplayDefinitionStatus::Registered;
    if (!haveCurrent)
    {
        return result;
    }
    result = registry.Register(current);
    haveCurrent = false;
    current = {};
    return result;
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
                FinishDefinition(registry, current, haveCurrent);
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

    const RegisterGameplayDefinitionResult finished = FinishDefinition(registry, current, haveCurrent);
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

        for (std::size_t index = 0; index < kGameplayStatCount; ++index)
        {
            if (!definition.hasStat[index])
            {
                continue;
            }
            if (!IsValidGameplayStatValue(definition.statValue[index]))
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
            if (!AppendFloat(result.text, definition.statValue[index]))
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
}
