#include "world/LevelFile.h"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace world
{
namespace
{
constexpr std::uintmax_t kMaxLevelFileBytes = 65536;
constexpr std::size_t kMaxLevelLineLength = 512;
constexpr std::size_t kMaxLevelLines = 256;

ParseLevelFileResult MakeStatus(
    LoadLevelFileStatus status,
    int line,
    const char* message,
    int formatVersion = 0)
{
    ParseLevelFileResult result{};
    result.status = status;
    result.errorLine = line;
    result.error = message;
    result.formatVersion = formatVersion;
    return result;
}

std::string_view Trim(std::string_view text)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
    {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t'))
    {
        text.remove_suffix(1);
    }
    return text;
}

bool ConsumeLine(std::string_view& remaining, std::string_view& line)
{
    if (remaining.empty())
    {
        return false;
    }

    const std::size_t newline = remaining.find('\n');
    if (newline == std::string_view::npos)
    {
        line = remaining;
        remaining = {};
        return true;
    }

    line = remaining.substr(0, newline);
    if (!line.empty() && line.back() == '\r')
    {
        line.remove_suffix(1);
    }
    remaining.remove_prefix(newline + 1);
    return true;
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

bool IsUnsignedIntegerToken(std::string_view token)
{
    if (token.empty())
    {
        return false;
    }
    for (const char character : token)
    {
        if (character < '0' || character > '9')
        {
            return false;
        }
    }
    return true;
}

bool ParseFloatToken(std::string_view token, float& value)
{
    if (token.empty())
    {
        return false;
    }

    float parsed = 0.0f;
    const std::from_chars_result result =
        std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size())
    {
        return false;
    }
    if (!std::isfinite(parsed))
    {
        return false;
    }
    value = parsed;
    return true;
}

bool ParseIntToken(std::string_view token, int& value)
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

bool ParseVec3(
    const std::vector<std::string_view>& tokens,
    std::size_t offset,
    core::Vec3& value)
{
    return ParseFloatToken(tokens[offset], value.x)
        && ParseFloatToken(tokens[offset + 1], value.y)
        && ParseFloatToken(tokens[offset + 2], value.z);
}

struct ParseState
{
    bool seenId = false;
    bool seenSpawn = false;
    bool seenKillPlane = false;
    bool seenGround = false;
    bool seenSupportCp1 = false;
    bool seenSupportCp2 = false;
    bool seenSupportGoal = false;
    bool seenMovingPlatform = false;
    bool seenGoal = false;
    bool seenDynamicBox = false;
    bool seenCamera = false;
    std::vector<Box> platforms;
    std::vector<SlopeSpec> slopes;
    std::vector<CheckpointSpec> checkpoints;
    std::vector<HazardSpec> hazards;
    std::vector<CollectibleSpec> collectibles;
};

bool RequireTokenCount(
    const std::vector<std::string_view>& tokens,
    std::size_t expected,
    ParseLevelFileResult& failure,
    int line)
{
    if (tokens.size() == expected)
    {
        return true;
    }
    failure = MakeStatus(LoadLevelFileStatus::Invalid, line, "wrong field count");
    return false;
}

bool RequireSingleton(
    bool& seen,
    ParseLevelFileResult& failure,
    int line,
    const char* duplicateMessage)
{
    if (seen)
    {
        failure = MakeStatus(LoadLevelFileStatus::Invalid, line, duplicateMessage);
        return false;
    }
    seen = true;
    return true;
}
}

bool IsValidLevelIdToken(std::string_view token)
{
    if (token.empty())
    {
        return false;
    }
    const char first = token.front();
    if (!((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') || first == '_'))
    {
        return false;
    }
    for (const char character : token)
    {
        const bool ok = (character >= 'A' && character <= 'Z')
            || (character >= 'a' && character <= 'z')
            || (character >= '0' && character <= '9')
            || character == '_';
        if (!ok)
        {
            return false;
        }
    }
    return true;
}

ParseLevelFileResult ParseLevelText(std::string_view text)
{
    if (text.empty())
    {
        return MakeStatus(LoadLevelFileStatus::Invalid, 0, "empty file");
    }
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF
        && static_cast<unsigned char>(text[1]) == 0xBB
        && static_cast<unsigned char>(text[2]) == 0xBF)
    {
        return MakeStatus(LoadLevelFileStatus::Invalid, 1, "UTF-8 BOM is not allowed");
    }

    ParseState state{};
    ParseLevelFileResult loaded{};
    loaded.status = LoadLevelFileStatus::Loaded;
    bool seenHeader = false;
    int lineNumber = 0;
    std::string_view remaining = text;
    std::string_view rawLine;
    std::vector<std::string_view> tokens;

    while (ConsumeLine(remaining, rawLine))
    {
        ++lineNumber;
        if (static_cast<std::size_t>(lineNumber) > kMaxLevelLines)
        {
            return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "too many lines");
        }
        if (rawLine.size() > kMaxLevelLineLength)
        {
            return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "line too long");
        }

        const std::string_view line = Trim(rawLine);
        if (line.empty())
        {
            continue;
        }
        if (!SplitTokens(line, tokens))
        {
            return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "empty line");
        }

        if (!seenHeader)
        {
            if (tokens[0] != kLevelFileMagic)
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "wrong magic");
            }
            if (tokens.size() != 2)
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "malformed header");
            }
            if (tokens[1] == "1")
            {
                loaded.formatVersion = kLevelFileVersion;
                seenHeader = true;
                continue;
            }
            if (IsUnsignedIntegerToken(tokens[1]))
            {
                int version = 0;
                ParseIntToken(tokens[1], version);
                return MakeStatus(
                    LoadLevelFileStatus::UnsupportedVersion,
                    lineNumber,
                    "unsupported version",
                    version);
            }
            return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "malformed version");
        }

        const std::string_view keyword = tokens[0];
        ParseLevelFileResult failure{};

        if (keyword == "id")
        {
            if (!RequireSingleton(state.seenId, failure, lineNumber, "duplicate id")
                || !RequireTokenCount(tokens, 2, failure, lineNumber))
            {
                return failure;
            }
            if (!IsValidLevelIdToken(tokens[1]))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid id");
            }
            loaded.level.id = std::string(tokens[1]);
            continue;
        }
        if (keyword == "spawn")
        {
            if (!RequireSingleton(state.seenSpawn, failure, lineNumber, "duplicate spawn")
                || !RequireTokenCount(tokens, 4, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseVec3(tokens, 1, loaded.level.initialSpawnVisualCenter))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid spawn");
            }
            continue;
        }
        if (keyword == "kill_plane")
        {
            if (!RequireSingleton(state.seenKillPlane, failure, lineNumber, "duplicate kill_plane")
                || !RequireTokenCount(tokens, 2, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseFloatToken(tokens[1], loaded.level.killPlaneY))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid kill_plane");
            }
            continue;
        }
        if (keyword == "ground")
        {
            if (!RequireSingleton(state.seenGround, failure, lineNumber, "duplicate ground")
                || !RequireTokenCount(tokens, 7, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseVec3(tokens, 1, loaded.level.ground.center)
                || !ParseVec3(tokens, 4, loaded.level.ground.size))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid ground");
            }
            continue;
        }
        if (keyword == "platform")
        {
            if (!RequireTokenCount(tokens, 7, failure, lineNumber))
            {
                return failure;
            }
            Box platform{};
            if (!ParseVec3(tokens, 1, platform.center) || !ParseVec3(tokens, 4, platform.size))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid platform");
            }
            state.platforms.push_back(platform);
            continue;
        }
        if (keyword == "support_index_cp1")
        {
            if (!RequireSingleton(state.seenSupportCp1, failure, lineNumber, "duplicate support_index_cp1")
                || !RequireTokenCount(tokens, 2, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseIntToken(tokens[1], loaded.level.checkpoint1PlatformIndex))
            {
                return MakeStatus(
                    LoadLevelFileStatus::Invalid, lineNumber, "invalid support_index_cp1");
            }
            continue;
        }
        if (keyword == "support_index_cp2")
        {
            if (!RequireSingleton(state.seenSupportCp2, failure, lineNumber, "duplicate support_index_cp2")
                || !RequireTokenCount(tokens, 2, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseIntToken(tokens[1], loaded.level.checkpoint2PlatformIndex))
            {
                return MakeStatus(
                    LoadLevelFileStatus::Invalid, lineNumber, "invalid support_index_cp2");
            }
            continue;
        }
        if (keyword == "support_index_goal")
        {
            if (!RequireSingleton(
                    state.seenSupportGoal, failure, lineNumber, "duplicate support_index_goal")
                || !RequireTokenCount(tokens, 2, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseIntToken(tokens[1], loaded.level.goalPlatformIndex))
            {
                return MakeStatus(
                    LoadLevelFileStatus::Invalid, lineNumber, "invalid support_index_goal");
            }
            continue;
        }
        if (keyword == "slope")
        {
            if (!RequireTokenCount(tokens, 8, failure, lineNumber))
            {
                return failure;
            }
            SlopeSpec slope{};
            if (!ParseVec3(tokens, 1, slope.center) || !ParseVec3(tokens, 4, slope.size)
                || !ParseFloatToken(tokens[7], slope.rotationZDegrees))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid slope");
            }
            state.slopes.push_back(slope);
            continue;
        }
        if (keyword == "moving_platform")
        {
            if (!RequireSingleton(
                    state.seenMovingPlatform, failure, lineNumber, "duplicate moving_platform")
                || !RequireTokenCount(tokens, 10, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseVec3(tokens, 1, loaded.level.movingPlatform.size)
                || !ParseFloatToken(tokens[4], loaded.level.movingPlatform.centerY)
                || !ParseFloatToken(tokens[5], loaded.level.movingPlatform.centerZ)
                || !ParseFloatToken(tokens[6], loaded.level.movingPlatform.pathMinX)
                || !ParseFloatToken(tokens[7], loaded.level.movingPlatform.pathMaxX)
                || !ParseFloatToken(tokens[8], loaded.level.movingPlatform.speed)
                || !ParseFloatToken(tokens[9], loaded.level.movingPlatform.startX))
            {
                return MakeStatus(
                    LoadLevelFileStatus::Invalid, lineNumber, "invalid moving_platform");
            }
            continue;
        }
        if (keyword == "checkpoint")
        {
            if (!RequireTokenCount(tokens, 10, failure, lineNumber))
            {
                return failure;
            }
            CheckpointSpec checkpoint{};
            if (!ParseVec3(tokens, 1, checkpoint.center) || !ParseVec3(tokens, 4, checkpoint.size)
                || !ParseVec3(tokens, 7, checkpoint.respawnPosition))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid checkpoint");
            }
            state.checkpoints.push_back(checkpoint);
            continue;
        }
        if (keyword == "hazard")
        {
            if (!RequireTokenCount(tokens, 7, failure, lineNumber))
            {
                return failure;
            }
            HazardSpec hazard{};
            if (!ParseVec3(tokens, 1, hazard.center) || !ParseVec3(tokens, 4, hazard.size))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid hazard");
            }
            state.hazards.push_back(hazard);
            continue;
        }
        if (keyword == "collectible")
        {
            if (!RequireTokenCount(tokens, 7, failure, lineNumber))
            {
                return failure;
            }
            CollectibleSpec collectible{};
            if (!ParseVec3(tokens, 1, collectible.center)
                || !ParseVec3(tokens, 4, collectible.size))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid collectible");
            }
            state.collectibles.push_back(collectible);
            continue;
        }
        if (keyword == "goal")
        {
            if (!RequireSingleton(state.seenGoal, failure, lineNumber, "duplicate goal")
                || !RequireTokenCount(tokens, 7, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseVec3(tokens, 1, loaded.level.goal.center)
                || !ParseVec3(tokens, 4, loaded.level.goal.size))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid goal");
            }
            continue;
        }
        if (keyword == "dynamic_box")
        {
            if (!RequireSingleton(
                    state.seenDynamicBox, failure, lineNumber, "duplicate dynamic_box")
                || !RequireTokenCount(tokens, 8, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseVec3(tokens, 1, loaded.level.dynamicBox.center)
                || !ParseVec3(tokens, 4, loaded.level.dynamicBox.size)
                || !ParseFloatToken(tokens[7], loaded.level.dynamicBox.mass))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid dynamic_box");
            }
            continue;
        }
        if (keyword == "camera")
        {
            if (!RequireSingleton(state.seenCamera, failure, lineNumber, "duplicate camera")
                || !RequireTokenCount(tokens, 5, failure, lineNumber))
            {
                return failure;
            }
            if (!ParseVec3(tokens, 1, loaded.level.camera.offset)
                || !ParseFloatToken(tokens[4], loaded.level.camera.fieldOfViewY))
            {
                return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "invalid camera");
            }
            continue;
        }

        return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "unrecognized record");
    }

    if (!seenHeader)
    {
        return MakeStatus(LoadLevelFileStatus::Invalid, 0, "missing header");
    }
    if (!state.seenId || !state.seenSpawn || !state.seenKillPlane || !state.seenGround
        || !state.seenSupportCp1 || !state.seenSupportCp2 || !state.seenSupportGoal
        || !state.seenMovingPlatform || !state.seenGoal || !state.seenDynamicBox
        || !state.seenCamera)
    {
        return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "missing required record");
    }
    if (state.platforms.size() != static_cast<std::size_t>(kLevel01ElevatedPlatformCount)
        || state.slopes.size() != static_cast<std::size_t>(kLevel01SlopeCount)
        || state.checkpoints.size() != static_cast<std::size_t>(kCheckpointCount)
        || state.hazards.size() != static_cast<std::size_t>(kHazardCount)
        || state.collectibles.size() != static_cast<std::size_t>(kCollectibleCount))
    {
        return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "wrong record count");
    }

    for (std::size_t index = 0; index < state.platforms.size(); ++index)
    {
        loaded.level.elevatedPlatforms[index] = state.platforms[index];
    }
    for (std::size_t index = 0; index < state.slopes.size(); ++index)
    {
        loaded.level.slopes[index] = state.slopes[index];
    }
    for (std::size_t index = 0; index < state.checkpoints.size(); ++index)
    {
        loaded.level.checkpoints[index] = state.checkpoints[index];
    }
    for (std::size_t index = 0; index < state.hazards.size(); ++index)
    {
        loaded.level.hazards[index] = state.hazards[index];
    }
    for (std::size_t index = 0; index < state.collectibles.size(); ++index)
    {
        loaded.level.collectibles[index] = state.collectibles[index];
    }

    if (!LevelDefinitionHasRequiredAuthoredContent(loaded.level))
    {
        return MakeStatus(LoadLevelFileStatus::Invalid, lineNumber, "semantic validation failed");
    }

    loaded.status = LoadLevelFileStatus::Loaded;
    loaded.error.clear();
    loaded.errorLine = 0;
    return loaded;
}

ParseLevelFileResult LoadLevelFile(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return MakeStatus(LoadLevelFileStatus::Error, 0, "empty path");
    }

    std::error_code existsError;
    const bool exists = std::filesystem::exists(path, existsError);
    if (existsError)
    {
        return MakeStatus(LoadLevelFileStatus::Error, 0, "path query failed");
    }
    if (!exists)
    {
        return MakeStatus(LoadLevelFileStatus::Missing, 0, "missing file");
    }

    std::error_code typeError;
    if (!std::filesystem::is_regular_file(path, typeError) || typeError)
    {
        return MakeStatus(LoadLevelFileStatus::Error, 0, "not a regular file");
    }

    std::error_code sizeError;
    const std::uintmax_t size = std::filesystem::file_size(path, sizeError);
    if (sizeError)
    {
        return MakeStatus(LoadLevelFileStatus::Error, 0, "size query failed");
    }
    if (size > kMaxLevelFileBytes)
    {
        return MakeStatus(LoadLevelFileStatus::Error, 0, "file too large");
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return MakeStatus(LoadLevelFileStatus::Error, 0, "open failed");
    }

    std::string text(static_cast<std::size_t>(size), '\0');
    if (size > 0)
    {
        stream.read(text.data(), static_cast<std::streamsize>(size));
        if (stream.bad() || stream.gcount() != static_cast<std::streamsize>(size))
        {
            return MakeStatus(LoadLevelFileStatus::Error, 0, "read failed");
        }
    }

    return ParseLevelText(text);
}
}
