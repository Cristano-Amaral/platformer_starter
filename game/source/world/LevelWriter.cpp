#include "world/LevelWriter.h"

#include "platform/FileReplace.h"
#include "world/LevelFile.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <fstream>
#include <system_error>

namespace world
{
namespace
{
// std::to_chars shortest round-trip: locale-independent, and from_chars
// recovers the exact same float. Non-finite values never reach here because
// IsWritableLevelDefinition rejects them first.
void AppendFloat(std::string& out, float value)
{
    std::array<char, 64> buffer{};
    const std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec != std::errc{})
    {
        out += '0';
        return;
    }
    out.append(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
}

void AppendInt(std::string& out, int value)
{
    std::array<char, 32> buffer{};
    const std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec != std::errc{})
    {
        out += '0';
        return;
    }
    out.append(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
}

void AppendVec3(std::string& out, core::Vec3 value)
{
    AppendFloat(out, value.x);
    out += ' ';
    AppendFloat(out, value.y);
    out += ' ';
    AppendFloat(out, value.z);
}

void AppendBoxRecord(std::string& out, std::string_view keyword, const Box& box)
{
    out.append(keyword);
    out += ' ';
    AppendVec3(out, box.center);
    out += ' ';
    AppendVec3(out, box.size);
    out += '\n';
}

void AppendCenterSizeRecord(
    std::string& out,
    std::string_view keyword,
    core::Vec3 center,
    core::Vec3 size)
{
    out.append(keyword);
    out += ' ';
    AppendVec3(out, center);
    out += ' ';
    AppendVec3(out, size);
    out += '\n';
}

void AppendIntRecord(std::string& out, std::string_view keyword, int value)
{
    out.append(keyword);
    out += ' ';
    AppendInt(out, value);
    out += '\n';
}

WriteLevelFileResult MakeWriteResult(WriteLevelFileStatus status, const char* error)
{
    WriteLevelFileResult result{};
    result.status = status;
    result.error = error;
    return result;
}

void BestEffortRemove(const std::filesystem::path& path)
{
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}
}

bool IsWritableLevelDefinition(const LevelDefinition& level)
{
    return IsValidLevelIdToken(level.id) && LevelDefinitionHasRequiredAuthoredContent(level);
}

std::string SerializeLevelText(const LevelDefinition& level)
{
    if (!IsWritableLevelDefinition(level))
    {
        return {};
    }

    // Canonical v1 record order. Documented in docs/LEVEL_FORMAT_V1.md.
    // The parser accepts any order; the writer emits exactly one.
    std::string out;
    out.reserve(1024);

    out.append(kLevelFileMagic);
    out += ' ';
    AppendInt(out, kLevelFileVersion);
    out += '\n';

    out += "id ";
    out.append(level.id);
    out += '\n';

    out += "spawn ";
    AppendVec3(out, level.initialSpawnVisualCenter);
    out += '\n';

    out += "kill_plane ";
    AppendFloat(out, level.killPlaneY);
    out += '\n';

    AppendBoxRecord(out, "ground", level.ground);

    for (const Box& platform : level.elevatedPlatforms)
    {
        AppendBoxRecord(out, "platform", platform);
    }

    AppendIntRecord(out, "support_index_cp1", level.checkpoint1PlatformIndex);
    AppendIntRecord(out, "support_index_cp2", level.checkpoint2PlatformIndex);
    AppendIntRecord(out, "support_index_goal", level.goalPlatformIndex);

    for (const SlopeSpec& slope : level.slopes)
    {
        out += "slope ";
        AppendVec3(out, slope.center);
        out += ' ';
        AppendVec3(out, slope.size);
        out += ' ';
        AppendFloat(out, slope.rotationZDegrees);
        out += '\n';
    }

    out += "moving_platform ";
    AppendVec3(out, level.movingPlatform.size);
    out += ' ';
    AppendFloat(out, level.movingPlatform.centerY);
    out += ' ';
    AppendFloat(out, level.movingPlatform.centerZ);
    out += ' ';
    AppendFloat(out, level.movingPlatform.pathMinX);
    out += ' ';
    AppendFloat(out, level.movingPlatform.pathMaxX);
    out += ' ';
    AppendFloat(out, level.movingPlatform.speed);
    out += ' ';
    AppendFloat(out, level.movingPlatform.startX);
    out += '\n';

    for (const CheckpointSpec& checkpoint : level.checkpoints)
    {
        out += "checkpoint ";
        AppendVec3(out, checkpoint.center);
        out += ' ';
        AppendVec3(out, checkpoint.size);
        out += ' ';
        AppendVec3(out, checkpoint.respawnPosition);
        out += '\n';
    }

    for (const HazardSpec& hazard : level.hazards)
    {
        AppendCenterSizeRecord(out, "hazard", hazard.center, hazard.size);
    }

    for (const CollectibleSpec& collectible : level.collectibles)
    {
        AppendCenterSizeRecord(out, "collectible", collectible.center, collectible.size);
    }

    AppendCenterSizeRecord(out, "goal", level.goal.center, level.goal.size);

    out += "dynamic_box ";
    AppendVec3(out, level.dynamicBox.center);
    out += ' ';
    AppendVec3(out, level.dynamicBox.size);
    out += ' ';
    AppendFloat(out, level.dynamicBox.mass);
    out += '\n';

    out += "camera ";
    AppendVec3(out, level.camera.offset);
    out += ' ';
    AppendFloat(out, level.camera.fieldOfViewY);
    out += '\n';

    return out;
}

std::filesystem::path LevelFileTemporaryPath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return {};
    }

    std::filesystem::path temporary = path;
    temporary += std::string(kLevelFileTemporarySuffix);
    return temporary;
}

WriteLevelFileResult SaveLevelFile(
    const std::filesystem::path& path,
    const LevelDefinition& level)
{
    if (!IsWritableLevelDefinition(level))
    {
        return MakeWriteResult(WriteLevelFileStatus::Invalid, "semantic validation failed");
    }
    // Absolute-only: the authoring target is always supplied explicitly, so a
    // relative path can never resolve against the process CWD.
    if (path.empty() || !path.is_absolute())
    {
        return MakeWriteResult(WriteLevelFileStatus::Error, "path must be absolute");
    }

    const std::filesystem::path temporaryPath = LevelFileTemporaryPath(path);
    if (temporaryPath.empty())
    {
        return MakeWriteResult(WriteLevelFileStatus::Error, "temporary path unavailable");
    }

    const std::string text = SerializeLevelText(level);
    if (text.empty())
    {
        return MakeWriteResult(WriteLevelFileStatus::Invalid, "serialization produced no text");
    }

    {
        std::ofstream stream(temporaryPath, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!stream)
        {
            return MakeWriteResult(WriteLevelFileStatus::Error, "temporary open failed");
        }

        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        stream.flush();
        const bool writeOk = static_cast<bool>(stream);
        stream.close();
        if (!writeOk || stream.fail())
        {
            BestEffortRemove(temporaryPath);
            return MakeWriteResult(WriteLevelFileStatus::Error, "temporary write failed");
        }
    }

    if (!platform::ReplaceFileWithTemporary(temporaryPath, path))
    {
        BestEffortRemove(temporaryPath);
        return MakeWriteResult(WriteLevelFileStatus::Error, "replace failed");
    }

    return MakeWriteResult(WriteLevelFileStatus::Saved, "");
}
}
