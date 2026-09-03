#include "world/LevelDefinition.h"
#include "world/LevelFile.h"

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
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

std::string ReadAll(const char* path)
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

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d LevelFile test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("LevelFile tests passed.\n");
    return 0;
}
