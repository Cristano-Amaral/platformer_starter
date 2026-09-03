#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelWriter.h"

#include <array>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>

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

bool Vec3Equal(core::Vec3 a, core::Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool BoxEqual(const world::Box& a, const world::Box& b)
{
    return Vec3Equal(a.center, b.center) && Vec3Equal(a.size, b.size);
}

bool CanonicalLevel01Values(const world::LevelDefinition& level)
{
    return level.id == world::kLevel01Id
        && Vec3Equal(level.initialSpawnVisualCenter, {0.0f, 0.8f, 0.0f})
        && level.killPlaneY == -8.0f
        && BoxEqual(level.ground, {{0.0f, -0.25f, 0.0f}, {56.0f, 0.5f, 8.0f}})
        && level.checkpoint1PlatformIndex == 2 && level.checkpoint2PlatformIndex == 4
        && level.goalPlatformIndex == 5
        && Vec3Equal(level.elevatedPlatforms[0].center, {5.0f, 0.75f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[1].center, {-4.5f, 2.25f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[2].center, {16.5f, 0.75f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[3].center, {-10.0f, 2.0f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[4].center, {-15.5f, 1.75f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[5].center, {-21.0f, 2.75f, 0.0f})
        && Vec3Equal(level.slopes[0].center, {21.70f, 1.6732f, 0.0f})
        && level.slopes[0].rotationZDegrees == 30.0f
        && Vec3Equal(level.slopes[1].center, {25.60f, 0.9660f, 0.0f})
        && level.movingPlatform.speed == 2.5f && level.movingPlatform.startX == 0.0f
        && Vec3Equal(level.checkpoints[0].center, {16.5f, 1.8f, 0.0f})
        && Vec3Equal(level.checkpoints[1].center, {-15.5f, 2.8f, 0.0f})
        && Vec3Equal(level.hazards[0].center, {11.5f, 0.5f, 0.0f})
        && Vec3Equal(level.hazards[1].center, {-18.5f, 0.5f, 0.0f})
        && Vec3Equal(level.collectibles[0].center, {5.0f, 2.5f, 0.0f})
        && Vec3Equal(level.collectibles[1].center, {-4.5f, 4.0f, 0.0f})
        && Vec3Equal(level.collectibles[2].center, {-10.0f, 3.75f, 0.0f})
        && Vec3Equal(level.goal.center, {-21.0f, 3.8f, 0.0f})
        && level.dynamicBox.mass == 30.0f
        && Vec3Equal(level.dynamicBox.center, {0.0f, 5.0f, 0.0f})
        && Vec3Equal(level.camera.offset, {2.0f, 3.5f, 12.0f})
        && level.camera.fieldOfViewY == 40.0f;
}

std::string_view FirstToken(std::string_view line)
{
    const std::size_t space = line.find(' ');
    return space == std::string_view::npos ? line : line.substr(0, space);
}

int CountRecords(std::string_view text, std::string_view keyword)
{
    int count = 0;
    std::size_t cursor = 0;
    while (cursor <= text.size())
    {
        const std::size_t newline = text.find('\n', cursor);
        const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
        if (end > cursor && FirstToken(text.substr(cursor, end - cursor)) == keyword)
        {
            ++count;
        }
        if (newline == std::string_view::npos)
        {
            break;
        }
        cursor = newline + 1;
    }
    return count;
}

// Whitelist proof: no line may carry a keyword outside the v1 grammar, so no
// runtime state (active checkpoint, deaths, collected flags, completion, TIME,
// BEST, platform/box poses, Jolt ids, smoothed camera target) can appear.
bool OnlyAuthoredKeywords(std::string_view text)
{
    static constexpr std::array<std::string_view, 17> allowed{
        "PLATFORMER_LEVEL",
        "id",
        "spawn",
        "kill_plane",
        "ground",
        "platform",
        "support_index_cp1",
        "support_index_cp2",
        "support_index_goal",
        "slope",
        "moving_platform",
        "checkpoint",
        "hazard",
        "collectible",
        "goal",
        "dynamic_box",
        "camera"};

    std::size_t cursor = 0;
    while (cursor <= text.size())
    {
        const std::size_t newline = text.find('\n', cursor);
        const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
        if (end > cursor)
        {
            const std::string_view keyword = FirstToken(text.substr(cursor, end - cursor));
            bool found = false;
            for (const std::string_view candidate : allowed)
            {
                found = found || keyword == candidate;
            }
            if (!found)
            {
                return false;
            }
        }
        if (newline == std::string_view::npos)
        {
            break;
        }
        cursor = newline + 1;
    }
    return true;
}

std::string ReadAll(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

std::string ReplaceFirstLineStartingWith(const std::string& text, std::string_view prefix, const char* replacement)
{
    std::string result;
    std::size_t cursor = 0;
    bool replaced = false;
    while (cursor < text.size())
    {
        const std::size_t newline = text.find('\n', cursor);
        const std::size_t end = newline == std::string::npos ? text.size() : newline + 1;
        const std::string_view line(text.data() + cursor, end - cursor);
        if (!replaced && line.size() >= prefix.size()
            && std::string_view(line.data(), prefix.size()) == prefix)
        {
            result += replacement;
            if (result.empty() || result.back() != '\n')
            {
                result += '\n';
            }
            replaced = true;
        }
        else
        {
            result.append(line);
        }
        cursor = end;
    }
    return result;
}

std::string InsertAfterHeader(const std::string& text, const char* extraLine)
{
    const std::size_t newline = text.find('\n');
    if (newline == std::string::npos)
    {
        return text;
    }
    std::string result = text.substr(0, newline + 1);
    result += extraLine;
    result += '\n';
    result += text.substr(newline + 1);
    return result;
}

void ExpectStatus(
    const char* name,
    std::string_view text,
    world::LoadLevelFileStatus status)
{
    const world::ParseLevelFileResult result = world::ParseLevelText(text);
    if (result.status != status)
    {
        std::fprintf(
            stderr,
            "FAIL %s: status %s (line %d: %s)\n",
            name,
            world::LoadLevelFileStatusName(result.status),
            result.errorLine,
            result.error.c_str());
        ++gFailures;
    }
}

void ExpectInvalid(const char* name, std::string_view text)
{
    ExpectStatus(name, text, world::LoadLevelFileStatus::Invalid);
}
}

int main()
{
    const std::string canonical = ReadAll(PLATFORMER_LEVEL01_SOURCE_PATH);
    Expect(!canonical.empty(), "read canonical source level");

    const world::ParseLevelFileResult parsed = world::ParseLevelText(canonical);
    if (parsed.status != world::LoadLevelFileStatus::Loaded)
    {
        std::fprintf(
            stderr,
            "canonical parse: %s line %d %s\n",
            world::LoadLevelFileStatusName(parsed.status),
            parsed.errorLine,
            parsed.error.c_str());
    }
    Expect(parsed.status == world::LoadLevelFileStatus::Loaded, "parse canonical Loaded");
    Expect(parsed.formatVersion == world::kLevelFileVersion, "canonical format version 1");
    Expect(parsed.level.id == "level_01", "canonical id");
    Expect(CanonicalLevel01Values(parsed.level), "canonical Level 01 authored values");

    {
        std::string mutating = canonical;
        const world::ParseLevelFileResult owned = world::ParseLevelText(mutating);
        mutating.assign(mutating.size(), 'Q');
        Expect(owned.status == world::LoadLevelFileStatus::Loaded, "owned-id parse Loaded");
        Expect(owned.level.id == "level_01", "LevelDefinition owns id after source overwrite");
        Expect(owned.level.id.size() == 8, "owned id length");
    }

    const world::ParseLevelFileResult otherId = world::ParseLevelText(
        ReplaceFirstLineStartingWith(canonical, "id ", "id other_level"));
    Expect(otherId.status == world::LoadLevelFileStatus::Loaded, "parser accepts non-level_01 id");
    Expect(otherId.level.id == "other_level", "parsed other id owned");

    const world::ParseLevelFileResult loadedFile = world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
    Expect(loadedFile.status == world::LoadLevelFileStatus::Loaded, "LoadLevelFile canonical");
    Expect(CanonicalLevel01Values(loadedFile.level), "LoadLevelFile canonical values");

    const world::ParseLevelFileResult missing = world::LoadLevelFile(
        std::filesystem::path(PLATFORMER_LEVEL01_SOURCE_PATH).parent_path()
        / "missing_level_01.level");
    Expect(missing.status == world::LoadLevelFileStatus::Missing, "missing file");

    ExpectInvalid("empty", "");
    ExpectInvalid("wrong magic", "PLATFORMER_SAVE 1\nid level_01\n");
    ExpectStatus(
        "unsupported version",
        "PLATFORMER_LEVEL 2\nid level_01\n",
        world::LoadLevelFileStatus::UnsupportedVersion);

    ExpectInvalid("missing required record", ReplaceFirstLineStartingWith(canonical, "goal ", ""));
    ExpectInvalid("duplicate spawn", InsertAfterHeader(canonical, "spawn 0 0.8 0"));
    ExpectInvalid(
        "wrong platform count",
        canonical + "platform 0 1 0 1 1 1\n");
    ExpectInvalid("malformed number", ReplaceFirstLineStartingWith(canonical, "kill_plane ", "kill_plane abc"));
    ExpectInvalid("overflow", ReplaceFirstLineStartingWith(canonical, "kill_plane ", "kill_plane 1e1000"));
    ExpectInvalid("nan", ReplaceFirstLineStartingWith(canonical, "kill_plane ", "kill_plane nan"));
    ExpectInvalid("inf", ReplaceFirstLineStartingWith(canonical, "kill_plane ", "kill_plane inf"));
    ExpectInvalid(
        "zero size",
        ReplaceFirstLineStartingWith(
            canonical, "ground ", "ground 0 -0.25 0 56 0 8"));
    ExpectInvalid(
        "negative size",
        ReplaceFirstLineStartingWith(
            canonical, "ground ", "ground 0 -0.25 0 56 -0.5 8"));
    ExpectInvalid(
        "invalid moving-platform path",
        ReplaceFirstLineStartingWith(
            canonical,
            "moving_platform ",
            "moving_platform 4 0.4 3 1.3 0 6 -6 2.5 0"));
    ExpectInvalid(
        "invalid mass",
        ReplaceFirstLineStartingWith(
            canonical, "dynamic_box ", "dynamic_box 0 5 0 1 1 1 0"));
    ExpectInvalid(
        "invalid FOV",
        ReplaceFirstLineStartingWith(canonical, "camera ", "camera 2 3.5 12 0"));
    ExpectInvalid("trailing malformed", canonical + "not_a_record 1\n");

    {
        const std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        const std::filesystem::path invalidPath = tempDir / "platformer3d_m31_invalid.level";
        const std::filesystem::path unsupportedPath = tempDir / "platformer3d_m31_unsupported.level";
        {
            std::ofstream invalid(invalidPath, std::ios::binary | std::ios::trunc);
            invalid << "NOT_A_LEVEL\n";
        }
        {
            std::ofstream unsupported(unsupportedPath, std::ios::binary | std::ios::trunc);
            unsupported << "PLATFORMER_LEVEL 2\n";
        }
        Expect(
            world::LoadLevelFile(invalidPath).status == world::LoadLevelFileStatus::Invalid,
            "LoadLevelFile invalid temp");
        Expect(
            world::LoadLevelFile(unsupportedPath).status
                == world::LoadLevelFileStatus::UnsupportedVersion,
            "LoadLevelFile unsupported temp");
        std::error_code ignored;
        std::filesystem::remove(invalidPath, ignored);
        std::filesystem::remove(unsupportedPath, ignored);
    }

    // ---- Level Format v1 writer (M32 Phase A) ----------------------------
    // Everything below works on strings and disposable temporary files. The
    // canonical source is only ever read.

    const std::string written = world::SerializeLevelText(parsed.level);
    Expect(!written.empty(), "serialize canonical level");
    Expect(written.starts_with("PLATFORMER_LEVEL 1\n"), "writer emits exact v1 header");
    Expect(!written.empty() && written.back() == '\n', "writer terminates last record");
    Expect(written.find('\r') == std::string::npos, "writer emits LF only");
    Expect(written == world::SerializeLevelText(parsed.level), "writer output is deterministic");

    Expect(CountRecords(written, "PLATFORMER_LEVEL") == 1, "writer header count");
    Expect(CountRecords(written, "id") == 1, "writer id count");
    Expect(CountRecords(written, "spawn") == 1, "writer spawn count");
    Expect(CountRecords(written, "kill_plane") == 1, "writer kill_plane count");
    Expect(CountRecords(written, "ground") == 1, "writer ground count");
    Expect(
        CountRecords(written, "platform") == world::kLevel01ElevatedPlatformCount,
        "writer platform count");
    Expect(CountRecords(written, "support_index_cp1") == 1, "writer support_index_cp1 count");
    Expect(CountRecords(written, "support_index_cp2") == 1, "writer support_index_cp2 count");
    Expect(CountRecords(written, "support_index_goal") == 1, "writer support_index_goal count");
    Expect(CountRecords(written, "slope") == world::kLevel01SlopeCount, "writer slope count");
    Expect(CountRecords(written, "moving_platform") == 1, "writer moving_platform count");
    Expect(CountRecords(written, "checkpoint") == world::kCheckpointCount, "writer checkpoint count");
    Expect(CountRecords(written, "hazard") == world::kHazardCount, "writer hazard count");
    Expect(
        CountRecords(written, "collectible") == world::kCollectibleCount,
        "writer collectible count");
    Expect(CountRecords(written, "goal") == 1, "writer goal count");
    Expect(CountRecords(written, "dynamic_box") == 1, "writer dynamic_box count");
    Expect(CountRecords(written, "camera") == 1, "writer camera count");
    Expect(OnlyAuthoredKeywords(written), "writer emits no runtime state records");

    const world::ParseLevelFileResult reparsed = world::ParseLevelText(written);
    if (reparsed.status != world::LoadLevelFileStatus::Loaded)
    {
        std::fprintf(
            stderr,
            "writer reparse: %s line %d %s\n",
            world::LoadLevelFileStatusName(reparsed.status),
            reparsed.errorLine,
            reparsed.error.c_str());
    }
    Expect(reparsed.status == world::LoadLevelFileStatus::Loaded, "writer output reparses Loaded");
    Expect(reparsed.formatVersion == world::kLevelFileVersion, "writer output is version 1");
    // world::AuthoredLevelDataEqual is the same comparison the editor uses for
    // Modified/Dirty, so round-trip coverage also covers editor change
    // detection.
    Expect(
        world::AuthoredLevelDataEqual(parsed.level, reparsed.level),
        "parse -> serialize -> parse preserves every authored field");
    Expect(CanonicalLevel01Values(reparsed.level), "round-tripped canonical Level 01 values");
    Expect(
        world::SerializeLevelText(reparsed.level) == written,
        "second generation serializes identically");

    {
        // The owned std::string id is written verbatim; no interning.
        world::LevelDefinition renamed = parsed.level;
        renamed.id = "other_level";
        const std::string renamedText = world::SerializeLevelText(renamed);
        Expect(renamedText.find("\nid other_level\n") != std::string::npos, "writer emits owned id");
        Expect(
            world::ParseLevelText(renamedText).level.id == "other_level",
            "written id reparses");

        world::LevelDefinition badId = parsed.level;
        badId.id = "1_bad id";
        Expect(!world::IsWritableLevelDefinition(badId), "writer rejects non-grammar id");
        Expect(world::SerializeLevelText(badId).empty(), "invalid id serializes to nothing");
    }

    {
        world::LevelDefinition zeroFov = parsed.level;
        zeroFov.camera.fieldOfViewY = 0.0f;
        Expect(world::SerializeLevelText(zeroFov).empty(), "writer rejects FOV 0");

        world::LevelDefinition negativeSize = parsed.level;
        negativeSize.elevatedPlatforms[3].size.y = -1.0f;
        Expect(world::SerializeLevelText(negativeSize).empty(), "writer rejects negative size");

        world::LevelDefinition nanCoordinate = parsed.level;
        nanCoordinate.ground.center.x = std::numeric_limits<float>::quiet_NaN();
        Expect(world::SerializeLevelText(nanCoordinate).empty(), "writer rejects NaN coordinate");

        world::LevelDefinition infiniteSpawn = parsed.level;
        infiniteSpawn.initialSpawnVisualCenter.y = std::numeric_limits<float>::infinity();
        Expect(world::SerializeLevelText(infiniteSpawn).empty(), "writer rejects infinite spawn");

        world::LevelDefinition emptyId = parsed.level;
        emptyId.id.clear();
        Expect(world::SerializeLevelText(emptyId).empty(), "writer rejects empty id");
    }

    {
        const std::filesystem::path saveDir =
            std::filesystem::temp_directory_path() / "platformer3d_m32_writer";
        std::error_code cleanupError;
        std::filesystem::remove_all(saveDir, cleanupError);
        std::filesystem::create_directories(saveDir, cleanupError);
        const std::filesystem::path savePath = saveDir / "level_01.level";
        const std::filesystem::path tempPath = world::LevelFileTemporaryPath(savePath);
        Expect(
            tempPath == savePath.string() + std::string(world::kLevelFileTemporarySuffix),
            "temporary is a sibling of the target");

        const world::WriteLevelFileResult firstSave =
            world::SaveLevelFile(savePath, parsed.level);
        if (firstSave.status != world::WriteLevelFileStatus::Saved)
        {
            std::fprintf(stderr, "first save: %s\n", firstSave.error.c_str());
        }
        Expect(firstSave.status == world::WriteLevelFileStatus::Saved, "save to new file Saved");
        Expect(!std::filesystem::exists(tempPath), "no leftover temp after first save");
        Expect(ReadAll(savePath) == written, "saved bytes match SerializeLevelText");
        Expect(
            world::LoadLevelFile(savePath).status == world::LoadLevelFileStatus::Loaded,
            "saved file reparses through LoadLevelFile");

        const world::WriteLevelFileResult replaceSave =
            world::SaveLevelFile(savePath, parsed.level);
        Expect(
            replaceSave.status == world::WriteLevelFileStatus::Saved,
            "save over existing file Saved");
        Expect(!std::filesystem::exists(tempPath), "no leftover temp after replacement save");

        world::LevelDefinition invalidLevel = parsed.level;
        invalidLevel.camera.fieldOfViewY = 0.0f;
        const world::WriteLevelFileResult invalidSave =
            world::SaveLevelFile(savePath, invalidLevel);
        Expect(invalidSave.status == world::WriteLevelFileStatus::Invalid, "invalid save Invalid");
        Expect(!std::filesystem::exists(tempPath), "invalid save writes no temp");
        Expect(ReadAll(savePath) == written, "invalid save leaves existing file untouched");

        // Absolute-only contract: a relative target can never resolve against
        // whatever directory launched the process.
        const world::WriteLevelFileResult relativeSave =
            world::SaveLevelFile(std::filesystem::path("level_01.level"), parsed.level);
        Expect(
            relativeSave.status == world::WriteLevelFileStatus::Error,
            "relative path save Error");

        // Replacement failure: the target name exists but is a directory, so
        // platform::ReplaceFileWithTemporary refuses to promote onto it.
        const std::filesystem::path directoryTarget = saveDir / "as_directory.level";
        std::filesystem::create_directories(directoryTarget, cleanupError);
        const world::WriteLevelFileResult replaceError =
            world::SaveLevelFile(directoryTarget, parsed.level);
        Expect(
            replaceError.status == world::WriteLevelFileStatus::Error,
            "replacement failure returns Error");
        Expect(
            !std::filesystem::exists(world::LevelFileTemporaryPath(directoryTarget)),
            "failed replacement removes its temp");

        std::filesystem::remove_all(saveDir, cleanupError);
    }

    // ---- Authored-data equality (M32 editor Modified/Dirty detection) ----
    {
        Expect(
            world::AuthoredLevelDataEqual(parsed.level, parsed.level),
            "authored equality is reflexive");

        // One case per editable M32 field, so the editor cannot report
        // "unmodified" after a supported edit.
        world::LevelDefinition spawnEdit = parsed.level;
        spawnEdit.initialSpawnVisualCenter.x += 0.5f;
        Expect(
            !world::AuthoredLevelDataEqual(parsed.level, spawnEdit),
            "authored equality detects spawn edit");

        world::LevelDefinition offsetEdit = parsed.level;
        offsetEdit.camera.offset.z += 1.0f;
        Expect(
            !world::AuthoredLevelDataEqual(parsed.level, offsetEdit),
            "authored equality detects camera offset edit");

        world::LevelDefinition fovEdit = parsed.level;
        fovEdit.camera.fieldOfViewY = 45.0f;
        Expect(
            !world::AuthoredLevelDataEqual(parsed.level, fovEdit),
            "authored equality detects camera FOV edit");

        world::LevelDefinition groundCenterEdit = parsed.level;
        groundCenterEdit.ground.center.y += 0.1f;
        Expect(
            !world::AuthoredLevelDataEqual(parsed.level, groundCenterEdit),
            "authored equality detects ground center edit");

        world::LevelDefinition groundSizeEdit = parsed.level;
        groundSizeEdit.ground.size.x += 1.0f;
        Expect(
            !world::AuthoredLevelDataEqual(parsed.level, groundSizeEdit),
            "authored equality detects ground size edit");

        for (std::size_t index = 0; index < parsed.level.elevatedPlatforms.size(); ++index)
        {
            world::LevelDefinition centerEdit = parsed.level;
            centerEdit.elevatedPlatforms[index].center.x += 0.5f;
            Expect(
                !world::AuthoredLevelDataEqual(parsed.level, centerEdit),
                "authored equality detects platform center edit");

            world::LevelDefinition sizeEdit = parsed.level;
            sizeEdit.elevatedPlatforms[index].size.y += 0.25f;
            Expect(
                !world::AuthoredLevelDataEqual(parsed.level, sizeEdit),
                "authored equality detects platform size edit");
        }

        // Non-editable fields still participate, so an externally changed
        // level is never mistaken for the saved baseline.
        world::LevelDefinition slopeEdit = parsed.level;
        slopeEdit.slopes[1].rotationZDegrees += 1.0f;
        Expect(
            !world::AuthoredLevelDataEqual(parsed.level, slopeEdit),
            "authored equality detects slope rotation change");

        world::LevelDefinition idEdit = parsed.level;
        idEdit.id = "other_level";
        Expect(
            !world::AuthoredLevelDataEqual(parsed.level, idEdit),
            "authored equality detects id change");
    }

    Expect(
        ReadAll(PLATFORMER_LEVEL01_SOURCE_PATH) == canonical,
        "canonical source level unchanged by writer tests");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d LevelFile test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("LevelFile tests passed.\n");
    return 0;
}
