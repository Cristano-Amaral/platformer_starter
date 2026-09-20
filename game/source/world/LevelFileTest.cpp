#include "editor/AuthoredObjectLifecycle.h"
#include "editor/RuntimeLevelReload.h"
#include "physics/PhysicsCapacity.h"
#include "world/HazardWorld.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelWriter.h"
#include "world/RespawnWorld.h"

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

core::Vec3 StandingPlayerVisualCenterOnGround(const world::Box& ground, core::Vec3 lane)
{
    const float groundTopY = ground.center.y + ground.size.y * 0.5f;
    return {lane.x, groundTopY + world::kPlayerVisualSize.y * 0.5f, lane.z};
}

bool CanonicalLevel01Values(const world::LevelDefinition& level)
{
    return level.id == world::kLevel01Id
        && Vec3Equal(level.initialSpawnVisualCenter, {0.0f, 0.8f, 0.0f})
        && level.killPlaneY == -8.0f
        && BoxEqual(level.ground, {{0.0f, -0.25f, 0.0f}, {56.0f, 0.5f, 8.0f}})
        && level.checkpoint1PlatformIndex == 2 && level.checkpoint2PlatformIndex == 4
        && level.goalPlatformIndex == 5
        && level.elevatedPlatforms.size()
            == static_cast<std::size_t>(world::kLevel01ElevatedPlatformCount)
        && Vec3Equal(level.elevatedPlatforms[0].center, {5.0f, 0.75f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[1].center, {-4.5f, 0.75f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[2].center, {16.5f, 0.75f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[3].center, {-10.0f, 1.5f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[4].center, {-15.5f, 1.75f, 0.0f})
        && Vec3Equal(level.elevatedPlatforms[5].center, {-21.0f, 2.25f, 0.0f})
        && Vec3Equal(level.slopes[0].center, {21.70f, 1.6732f, 0.0f})
        && level.slopes[0].rotationZDegrees == 30.0f
        && Vec3Equal(level.slopes[1].center, {25.60f, 0.9660f, 0.0f})
        && level.movingPlatform.speed == 2.5f && level.movingPlatform.startX == 0.0f
        && level.checkpoints.size() == static_cast<std::size_t>(world::kLevel01CheckpointCount)
        && Vec3Equal(level.checkpoints[0].center, {16.5f, 1.8f, 0.0f})
        && Vec3Equal(level.checkpoints[1].center, {-15.5f, 2.8f, 0.0f})
        && level.hazards.size() == static_cast<std::size_t>(world::kLevel01HazardCount)
        && Vec3Equal(level.hazards[0].center, {11.5f, 0.5f, 0.0f})
        && Vec3Equal(level.hazards[1].center, {-18.5f, 0.5f, 0.0f})
        && level.collectibles.size() == static_cast<std::size_t>(world::kLevel01CollectibleCount)
        && Vec3Equal(level.collectibles[0].center, {5.0f, 2.5f, 0.0f})
        && Vec3Equal(level.collectibles[1].center, {-4.5f, 2.5f, 0.0f})
        && Vec3Equal(level.collectibles[2].center, {-10.0f, 3.3f, 0.0f})
        && level.levelGoals.size() == static_cast<std::size_t>(world::kLevel01LevelGoalCount)
        && Vec3Equal(level.levelGoals[0].center, {-21.0f, 3.3f, 0.0f})
        && Vec3Equal(level.levelGoals[0].size, {2.0f, 1.6f, 1.8f})
        && level.levelGoals[0].nextLevelId == world::kLevel02Id
        && level.dynamicBoxes.empty()
        && level.pressurePlates.empty()
        && level.doors.empty()
        && level.itemPickups.empty()
        && level.staticProps.empty()
        && Vec3Equal(level.camera.offset, {2.0f, 3.5f, 12.0f})
        && level.camera.fieldOfViewY == 40.0f
        && world::LevelEnvironmentEqual(
            level.environment, world::MakeDefaultLevelEnvironment());
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
// BEST, platform/box poses, Jolt ids, smoothed camera target, inventory) can appear.
bool OnlyAuthoredKeywords(std::string_view text)
{
    static constexpr std::array<std::string_view, 26> allowed{
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
        "level_goal",
        "dynamic_box",
        "pressure_plate",
        "door",
        "item_pickup",
        "static_prop",
        "authoring_group",
        "camera",
        "environment",
        "directional_light",
        "point_light",
        "spot_light"};

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

void WriteAll(const std::filesystem::path& path, std::string_view text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
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
    Expect(canonical.find("player_model") == std::string::npos, "canonical text has no player_model");
    Expect(
        canonical.find("models/player.glb") == std::string::npos,
        "canonical Level 01 has no player model identity");

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

    const world::ParseLevelFileResult loadedLevel02 =
        world::LoadLevelFile(PLATFORMER_LEVEL02_SOURCE_PATH);
    Expect(loadedLevel02.status == world::LoadLevelFileStatus::Loaded, "LoadLevelFile level_02");
    Expect(loadedLevel02.level.id == world::kLevel02Id, "level_02 id");
    Expect(loadedLevel02.level.levelGoals.size() == 1, "canonical level_02 has one Level Goal");
    Expect(loadedLevel02.level.levelGoals[0].nextLevelId.empty(),
        "canonical level_02 Goal is terminal");
    Expect(loadedLevel02.level.checkpoints.size() == 1, "level_02 has one Checkpoint");
    Expect(loadedLevel02.level.hazards.size() == 1, "level_02 has one Hazard");
    Expect(
        Vec3Equal(loadedLevel02.level.hazards[0].center, {7.5f, 1.0f, 0.0f}),
        "level_02 Hazard sits on the floor at the M74 horizontal lane");
    Expect(
        Vec3Equal(loadedLevel02.level.hazards[0].size, {1.2f, 1.0f, 2.0f}),
        "level_02 Hazard size is unchanged");
    {
        const core::Vec3 standingOnFloor = StandingPlayerVisualCenterOnGround(
            loadedLevel02.level.ground,
            loadedLevel02.level.hazards[0].center);
        Expect(
            world::FindHazardIndexContaining(standingOnFloor, loadedLevel02.level.hazards) == 0,
            "canonical level_02 Hazard contains a standing player on the traversable floor");

        const std::array<world::HazardSpec, 1> belowFloorHazard{{
            {{7.5f, 0.7f, 0.0f}, {1.2f, 1.0f, 2.0f}},
        }};
        Expect(
            world::FindHazardIndexContaining(standingOnFloor, belowFloorHazard)
                == world::kNoHazardIndex,
            "the original below-floor Hazard misses a standing player on the floor");
    }
    Expect(loadedLevel02.level.collectibles.size() == 1, "level_02 has one Collectible");
    Expect(loadedLevel02.level.itemPickups.size() == 1, "level_02 has one Key Item Pickup");
    Expect(loadedLevel02.level.itemPickups[0].itemId == "key", "level_02 pickup is key");
    Expect(loadedLevel02.level.dynamicBoxes.size() == 1, "level_02 has one Dynamic Box");
    Expect(loadedLevel02.level.pressurePlates.size() == 1, "level_02 has one Pressure Plate");
    Expect(loadedLevel02.level.pressurePlates[0].visibleInGameplay, "level_02 plate is visible");
    Expect(loadedLevel02.level.doors.size() == 2, "level_02 has key Door and plate Door");
    Expect(loadedLevel02.level.doors[0].requiredItemId == "key", "level_02 first Door requires key");
    Expect(loadedLevel02.level.doors[1].requiredItemId.empty(), "level_02 second Door is plate-driven");
    Expect(world::IsWritableLevelDefinition(loadedLevel02.level), "level_02 is writable");

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

    ExpectInvalid("missing required record", ReplaceFirstLineStartingWith(canonical, "camera ", ""));
    ExpectInvalid("duplicate spawn", InsertAfterHeader(canonical, "spawn 0 0.8 0"));
    {
        const world::ParseLevelFileResult extraPlatform =
            world::ParseLevelText(canonical + "platform 0 1 0 1 1 1\n");
        Expect(extraPlatform.status == world::LoadLevelFileStatus::Loaded, "extra platform still v1");
        Expect(
            extraPlatform.level.elevatedPlatforms.size()
                == static_cast<std::size_t>(world::kLevel01ElevatedPlatformCount + 1),
            "extra platform increases count");
    }
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
        canonical + "dynamic_box 0 5 0 1 1 1 0\n");
    ExpectInvalid(
        "invalid FOV",
        ReplaceFirstLineStartingWith(canonical, "camera ", "camera 2 3.5 12 0"));
    ExpectInvalid("trailing malformed", canonical + "not_a_record 1\n");
    ExpectInvalid("player_model is not Level Format syntax", canonical + "player_model models/player.glb\n");
    ExpectInvalid(
        "support index out of range",
        ReplaceFirstLineStartingWith(canonical, "support_index_goal ", "support_index_goal 6"));
    ExpectInvalid(
        "negative support index",
        ReplaceFirstLineStartingWith(canonical, "support_index_cp1 ", "support_index_cp1 -1"));
    {
        std::string zeroPlatforms = canonical;
        for (;;)
        {
            const std::string next = ReplaceFirstLineStartingWith(zeroPlatforms, "platform ", "");
            if (next == zeroPlatforms)
            {
                break;
            }
            zeroPlatforms = next;
        }
        ExpectInvalid("zero platforms", zeroPlatforms);
    }

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
    Expect(written.find("player_model") == std::string::npos, "writer does not emit player_model");
    Expect(
        written.find("models/player.glb") == std::string::npos,
        "writer does not emit player model identity");
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
    Expect(CountRecords(written, "checkpoint") == world::kLevel01CheckpointCount, "writer checkpoint count");
    Expect(CountRecords(written, "hazard") == world::kLevel01HazardCount, "writer hazard count");
    Expect(CountRecords(written, "collectible") == world::kLevel01CollectibleCount,
        "writer collectible count");
    Expect(CountRecords(written, "level_goal") == world::kLevel01LevelGoalCount, "writer level_goal count");
    Expect(CountRecords(written, "goal") == 0, "writer emits no legacy goal singleton");
    Expect(CountRecords(written, "dynamic_box") == 0, "writer dynamic_box count");
    Expect(CountRecords(written, "pressure_plate") == 0, "writer pressure_plate count");
    Expect(CountRecords(written, "door") == 0, "writer door count");
    Expect(CountRecords(written, "item_pickup") == 0, "writer item_pickup count");
    Expect(CountRecords(written, "static_prop") == 0, "writer static_prop count");
    Expect(CountRecords(written, "authoring_group") == 0, "writer authoring_group count");
    Expect(CountRecords(written, "camera") == 1, "writer camera count");
    Expect(CountRecords(written, "environment") == 1, "writer environment count");
    Expect(CountRecords(written, "directional_light") == 1, "writer directional_light count");
    Expect(CountRecords(written, "inventory") == 0, "writer emits no inventory records");
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

        world::LevelDefinition countEdit = parsed.level;
        countEdit.elevatedPlatforms.push_back(countEdit.elevatedPlatforms.back());
        Expect(
            !world::AuthoredLevelDataEqual(parsed.level, countEdit),
            "authored equality detects platform count change");

        world::LevelDefinition plateEdit = parsed.level;
        world::PressurePlateSpec plate{};
        plate.center = {2.0f, 0.1f, 0.0f};
        plate.size = world::kDefaultPressurePlateSize;
        plateEdit.pressurePlates.push_back(plate);
        Expect(
            !world::AuthoredLevelDataEqual(parsed.level, plateEdit),
            "authored equality detects Pressure Plate add");
        plateEdit.pressurePlates[0].center.x += 1.0f;
        world::LevelDefinition plateMoved = plateEdit;
        plateMoved.pressurePlates[0].center.x += 1.0f;
        Expect(
            !world::AuthoredLevelDataEqual(plateEdit, plateMoved),
            "authored equality detects Pressure Plate position edit");
        world::LevelDefinition plateSized = plateEdit;
        plateSized.pressurePlates[0].size.x += 0.5f;
        Expect(
            !world::AuthoredLevelDataEqual(plateEdit, plateSized),
            "authored equality detects Pressure Plate size edit");
    }

    // ---- Milestone 39 staged reload core (no ImGui, no tools) ----
    // Persistent BEST is Application/session state, not LevelDefinition.
    // Prepare/Reconcile never read or write best_time_v1.txt.
    {
        std::error_code cleanupError;
        const std::filesystem::path reloadDir =
            std::filesystem::temp_directory_path() / "platformer3d_m39_reload";
        std::filesystem::remove_all(reloadDir, cleanupError);
        std::filesystem::create_directories(reloadDir, cleanupError);

        const std::filesystem::path stagedPath = reloadDir / "assets" / "levels" / "level_01.level";
        const std::filesystem::path sourcePath = reloadDir / "source" / "levels" / "level_01.level";
        const std::filesystem::path cookedPath = reloadDir / "cooked" / "levels" / "level_01.level";

        world::LevelDefinition stagedFov55 = parsed.level;
        stagedFov55.camera.fieldOfViewY = 55.0f;
        const std::string staged55Text = world::SerializeLevelText(stagedFov55);
        const std::string staged40Text = world::SerializeLevelText(parsed.level);
        WriteAll(stagedPath, staged55Text);
        WriteAll(sourcePath, staged40Text);
        WriteAll(cookedPath, staged40Text);
        const std::string sourceBefore = ReadAll(sourcePath);
        const std::string cookedBefore = ReadAll(cookedPath);
        const std::string stagedBefore = ReadAll(stagedPath);

        const editor::RuntimeLevelReloadPrepareResult ready =
            editor::PrepareRuntimeLevelReload(stagedPath, false);
        Expect(ready.status == editor::RuntimeLevelReloadStatus::Ready, "reload success Ready");
        Expect(ready.candidate.camera.fieldOfViewY == 55.0f, "reload candidate FOV from staged 55");
        Expect(ready.candidate.camera.fieldOfViewY != 40.0f, "reload does not use FOV 40 source");

        Expect(ReadAll(sourcePath) == sourceBefore, "reload does not write source");
        Expect(ReadAll(cookedPath) == cookedBefore, "reload does not write cooked");
        Expect(ReadAll(stagedPath) == stagedBefore, "reload does not rewrite staged");

        WriteAll(stagedPath, staged40Text);
        const editor::RuntimeLevelReloadPrepareResult stale =
            editor::PrepareRuntimeLevelReload(stagedPath, false);
        Expect(stale.status == editor::RuntimeLevelReloadStatus::Ready, "stale staged still Ready");
        Expect(
            stale.candidate.camera.fieldOfViewY == 40.0f,
            "reload authority is staged FOV 40 when source/cooked differ");

        const editor::RuntimeLevelReloadPrepareResult modifiedGuard =
            editor::PrepareRuntimeLevelReload(stagedPath, true);
        Expect(
            modifiedGuard.status == editor::RuntimeLevelReloadStatus::RejectedModified,
            "Modified guard rejects before read");
        Expect(modifiedGuard.candidate.id.empty(), "Modified guard has no candidate");

        const editor::RuntimeLevelReloadPrepareResult missingStaged =
            editor::PrepareRuntimeLevelReload(reloadDir / "assets" / "levels" / "missing.level", false);
        Expect(missingStaged.status == editor::RuntimeLevelReloadStatus::Missing, "missing staged Missing");

        const editor::RuntimeLevelReloadPrepareResult relative =
            editor::PrepareRuntimeLevelReload(std::filesystem::path("levels") / "level_01.level", false);
        Expect(relative.status == editor::RuntimeLevelReloadStatus::Error, "relative path Error");

        const std::filesystem::path malformedPath = reloadDir / "assets" / "levels" / "malformed.level";
        WriteAll(malformedPath, "NOT_A_LEVEL\n");
        const editor::RuntimeLevelReloadPrepareResult malformed =
            editor::PrepareRuntimeLevelReload(malformedPath, false);
        Expect(malformed.status == editor::RuntimeLevelReloadStatus::Invalid, "malformed staged Invalid");

        world::LevelDefinition invalidLevel = parsed.level;
        invalidLevel.camera.fieldOfViewY = 0.0f;
        const std::filesystem::path invalidPath = reloadDir / "assets" / "levels" / "invalid.level";
        WriteAll(invalidPath, "PLATFORMER_LEVEL 1\nid level_01\nspawn 0 0.8 0\n");
        const editor::RuntimeLevelReloadPrepareResult invalid =
            editor::PrepareRuntimeLevelReload(invalidPath, false);
        Expect(invalid.status == editor::RuntimeLevelReloadStatus::Invalid, "invalid candidate Invalid");

        world::LevelDefinition otherLevel = parsed.level;
        otherLevel.id = "other_level";
        const std::filesystem::path otherIdPath = reloadDir / "assets" / "levels" / "other.level";
        WriteAll(otherIdPath, world::SerializeLevelText(otherLevel));
        const editor::RuntimeLevelReloadPrepareResult wrongId =
            editor::PrepareRuntimeLevelReload(otherIdPath, false);
        Expect(wrongId.status == editor::RuntimeLevelReloadStatus::Invalid, "id/stem mismatch Invalid");

        world::LevelDefinition matchingDestination = parsed.level;
        matchingDestination.id = "level_02";
        const std::filesystem::path matchingPath =
            reloadDir / "assets" / "levels" / "level_02.level";
        WriteAll(matchingPath, world::SerializeLevelText(matchingDestination));
        const editor::RuntimeLevelReloadPrepareResult matchingId =
            editor::PrepareRuntimeLevelReload(matchingPath, false);
        Expect(matchingId.status == editor::RuntimeLevelReloadStatus::Ready, "matching stem/id Ready");
        Expect(matchingId.candidate.id == "level_02", "matching candidate id");

        const editor::RuntimeLevelReloadReconcileResult sameBaseline =
            editor::ReconcileAfterRuntimeLevelReload(parsed.level, parsed.level);
        Expect(world::AuthoredLevelDataEqual(sameBaseline.workingCopy, parsed.level), "reconcile copies active");
        Expect(!sameBaseline.modified, "successful reload Modified false");
        Expect(!sameBaseline.dirty, "Dirty false when staged matches saved source baseline");
        Expect(sameBaseline.selection.kind == editor::EditorObjectKind::None, "selection cleared");

        world::LevelDefinition saved55 = parsed.level;
        saved55.camera.fieldOfViewY = 55.0f;
        const editor::RuntimeLevelReloadReconcileResult dirtyStaged =
            editor::ReconcileAfterRuntimeLevelReload(parsed.level, saved55);
        Expect(!dirtyStaged.modified, "Dirty-but-not-Modified: Modified false");
        Expect(dirtyStaged.dirty, "Dirty true when staged active differs from saved source");

        std::filesystem::remove_all(reloadDir, cleanupError);
    }

    // ---- Milestone 41 variable-count v1 (in-memory; canonical file untouched) ----
    {
        auto RoundTrip = [](world::LevelDefinition level, const char* name) {
            const std::string text = world::SerializeLevelText(level);
            Expect(!text.empty(), name);
            const world::ParseLevelFileResult again = world::ParseLevelText(text);
            Expect(again.status == world::LoadLevelFileStatus::Loaded, name);
            Expect(world::AuthoredLevelDataEqual(level, again.level), name);
            Expect(world::SerializeLevelText(again.level) == text, name);
            return again.level;
        };

        world::LevelDefinition extraPlatform = parsed.level;
        extraPlatform.elevatedPlatforms.push_back({{0.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 1.0f}});
        Expect(extraPlatform.elevatedPlatforms.size() == 7, "variable platform count +1");
        const world::LevelDefinition extraPlatformRound = RoundTrip(extraPlatform, "platform round trip");
        Expect(extraPlatformRound.elevatedPlatforms.size() == 7, "platform count preserved");
        Expect(
            extraPlatformRound.elevatedPlatforms[6].center.x == 0.0f, "appended platform order");

        world::LevelDefinition fewerPlatforms = parsed.level;
        fewerPlatforms.elevatedPlatforms.pop_back();
        fewerPlatforms.goalPlatformIndex = 4;
        Expect(fewerPlatforms.elevatedPlatforms.size() == 5, "variable platform count -1");
        RoundTrip(fewerPlatforms, "fewer platforms round trip");

        {
            world::LevelDefinition remapped = parsed.level;
            const world::Box formerCp1 = remapped.elevatedPlatforms[2];
            const world::Box formerCp2 = remapped.elevatedPlatforms[4];
            const world::Box formerGoal = remapped.elevatedPlatforms[5];
            const editor::LifecycleEditResult deleted = editor::DeleteSelected(
                remapped, {editor::EditorObjectKind::ElevatedPlatform, 1});
            Expect(deleted.succeeded, "delete-before-reference for writer");
            Expect(remapped.checkpoint1PlatformIndex == 1, "writer remap cp1");
            Expect(remapped.checkpoint2PlatformIndex == 3, "writer remap cp2");
            Expect(remapped.goalPlatformIndex == 4, "writer remap goal");
            Expect(
                remapped.elevatedPlatforms[1].center.x == formerCp1.center.x, "semantic cp1 target");
            Expect(
                remapped.elevatedPlatforms[3].center.x == formerCp2.center.x, "semantic cp2 target");
            Expect(
                remapped.elevatedPlatforms[4].center.x == formerGoal.center.x,
                "semantic goal target");
            const std::string remappedText = world::SerializeLevelText(remapped);
            Expect(!remappedText.empty(), "remapped writer produces v1");
            const world::ParseLevelFileResult remappedParsed = world::ParseLevelText(remappedText);
            Expect(
                remappedParsed.status == world::LoadLevelFileStatus::Loaded, "remapped parse Loaded");
            Expect(
                world::AuthoredLevelDataEqual(remapped, remappedParsed.level),
                "remapped round trip equal");
            Expect(
                world::SerializeLevelText(remappedParsed.level) == remappedText,
                "remapped writer deterministic");
            Expect(remappedParsed.level.checkpoint1PlatformIndex == 1, "parsed remapped cp1");
            Expect(
                remappedParsed.level.elevatedPlatforms[1].center.x == formerCp1.center.x,
                "parsed remapped semantic target");
        }

        world::LevelDefinition extraCheckpoint = parsed.level;
        extraCheckpoint.checkpoints.push_back(
            {{0.0f, 2.0f, 0.0f}, {2.4f, 1.6f, 2.0f}, {0.0f, 2.0f, 0.0f}});
        RoundTrip(extraCheckpoint, "checkpoint round trip");
        Expect(extraCheckpoint.checkpoints.size() == 3, "checkpoint count +1");

        world::LevelDefinition extraHazard = parsed.level;
        extraHazard.hazards.push_back({{0.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        RoundTrip(extraHazard, "hazard round trip");

        world::LevelDefinition extraCollectible = parsed.level;
        extraCollectible.collectibles.push_back({{1.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        RoundTrip(extraCollectible, "collectible round trip");
        Expect(extraCollectible.collectibles.size() == 4, "collectible count +1");

        world::LevelDefinition manyCollectibles = parsed.level;
        while (manyCollectibles.collectibles.size() < 17)
        {
            const float x = static_cast<float>(manyCollectibles.collectibles.size());
            manyCollectibles.collectibles.push_back({{x, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        }
        RoundTrip(manyCollectibles, "17 collectibles round trip");
        Expect(manyCollectibles.collectibles.size() == 17, "17 collectibles remain authored");

        world::LevelDefinition manyCheckpoints = parsed.level;
        while (manyCheckpoints.checkpoints.size() < 9)
        {
            const float x = static_cast<float>(manyCheckpoints.checkpoints.size());
            manyCheckpoints.checkpoints.push_back(
                {{x, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {x, 1.8f, 0.0f}});
        }
        RoundTrip(manyCheckpoints, "9 checkpoints round trip");
        Expect(world::ReconcileActiveCheckpointIndex(8, 9) == 8, "checkpoint progression at 9");

        world::LevelDefinition manyHazards = parsed.level;
        while (manyHazards.hazards.size() < 9)
        {
            const float x = static_cast<float>(manyHazards.hazards.size()) * 2.0f;
            manyHazards.hazards.push_back({{x, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        }
        RoundTrip(manyHazards, "9 hazards round trip");
        Expect(
            world::FindHazardIndexContaining(manyHazards.hazards.back().center, manyHazards.hazards)
                == static_cast<int>(manyHazards.hazards.size() - 1),
            "hazard overlap iteration at 9");

        world::LevelDefinition manyPlatforms = parsed.level;
        while (manyPlatforms.elevatedPlatforms.size() < 17)
        {
            const float x = 40.0f + static_cast<float>(manyPlatforms.elevatedPlatforms.size());
            manyPlatforms.elevatedPlatforms.push_back({{x, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        }
        RoundTrip(manyPlatforms, "17 platforms round trip");

        std::string tooManyPlatforms = canonical;
        const int extras = world::kMaxElevatedPlatformCount - world::kLevel01ElevatedPlatformCount + 1;
        for (int extra = 0; extra < extras; ++extra)
        {
            tooManyPlatforms += "platform 0 1 0 1 1 1\n";
        }
        ExpectInvalid("platform count exceeds physics body capacity", tooManyPlatforms);

        {
            const std::string oneBox = canonical + "dynamic_box 2 1 0 1 1 1 30\n";
            const world::ParseLevelFileResult one = world::ParseLevelText(oneBox);
            Expect(one.status == world::LoadLevelFileStatus::Loaded, "one dynamic_box loads");
            Expect(one.level.dynamicBoxes.size() == 1, "one dynamic_box count");
            Expect(one.level.dynamicBoxes[0].center.x == 2.0f, "one dynamic_box center x");
            Expect(one.level.dynamicBoxes[0].massKg == 30.0f, "one dynamic_box default mass");
            const std::string writtenOne = world::SerializeLevelText(one.level);
            Expect(CountRecords(writtenOne, "dynamic_box") == 1, "writer one dynamic_box");
            Expect(
                world::AuthoredLevelDataEqual(one.level, world::ParseLevelText(writtenOne).level),
                "one dynamic_box round trip");

            const std::string twoBoxes =
                canonical + "dynamic_box 2 1 0 1 1 1 30\ndynamic_box 4 1 0 1 1 1 5\n";
            const world::ParseLevelFileResult two = world::ParseLevelText(twoBoxes);
            Expect(two.status == world::LoadLevelFileStatus::Loaded, "two dynamic_box loads");
            Expect(two.level.dynamicBoxes.size() == 2, "two dynamic_box count");
            Expect(two.level.dynamicBoxes[0].massKg == 30.0f, "encounter order first");
            Expect(two.level.dynamicBoxes[1].massKg == 5.0f, "encounter order second");
            Expect(
                CountRecords(world::SerializeLevelText(two.level), "dynamic_box") == 2,
                "writer two dynamic_box");

            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 1 1 1 0\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "zero mass rejected");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 1 1 1 -1\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "negative mass rejected");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 1 1 1 nan\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "NaN mass rejected");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 1 1 1 inf\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "Inf mass rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "dynamic_box 2 1 0 1 1 1 10000.1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "above-max mass rejected");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 1 1 1 10000\n").status
                    == world::LoadLevelFileStatus::Loaded,
                "max mass accepted");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 0 1 1 30\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "zero size rejected");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 -1 1 1 30\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "negative size rejected");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 nan 1 1 30\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "NaN size rejected");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 inf 1 1 30\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "Inf size rejected");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 0.12 0.12 0.12 30\n").status
                    == world::LoadLevelFileStatus::Loaded,
                "minimum extent accepted");
            Expect(
                world::ParseLevelText(canonical + "dynamic_box 2 1 0 0.119 1 1 30\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "below minimum extent rejected");

            world::LevelDefinition saveAuthored = parsed.level;
            world::DynamicBoxSpec authoredBox{};
            authoredBox.center = {2.0f, 1.0f, 0.0f};
            authoredBox.size = world::kDefaultDynamicBoxSize;
            authoredBox.massKg = world::kDefaultDynamicBoxMassKg;
            saveAuthored.dynamicBoxes.push_back(authoredBox);
            const std::string saved = world::SerializeLevelText(saveAuthored);
            Expect(saved.find("dynamic_box 2 ") != std::string::npos, "Save writes authored X=2");
            Expect(saved.find("dynamic_box 8 ") == std::string::npos, "Save does not bake X=8");

            world::LevelDefinition combined = parsed.level;
            combined.elevatedPlatforms.resize(
                static_cast<std::size_t>(physics::kMaxAuthoredPhysicsBodies) - 2,
                {{40.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
            combined.dynamicBoxes.assign(
                2, world::DynamicBoxSpec{{2.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 30.0f});
            Expect(
                world::LevelDefinitionHasRequiredAuthoredContent(combined),
                "exact combined capacity is valid");
            combined.dynamicBoxes.push_back(
                {{6.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 30.0f});
            Expect(
                !world::LevelDefinitionHasRequiredAuthoredContent(combined),
                "one body over combined capacity is invalid");
        }

        {
            const std::string onePlate = canonical + "pressure_plate 2 0.1 0 2 0.2 2\n";
            const world::ParseLevelFileResult one = world::ParseLevelText(onePlate);
            Expect(one.status == world::LoadLevelFileStatus::Loaded, "one pressure_plate loads");
            Expect(one.level.pressurePlates.size() == 1, "one pressure_plate count");
            Expect(one.level.pressurePlates[0].center.x == 2.0f, "one pressure_plate center x");
            Expect(one.level.pressurePlates[0].size.y == 0.2f, "one pressure_plate size y");
            const std::string writtenOne = world::SerializeLevelText(one.level);
            Expect(CountRecords(writtenOne, "pressure_plate") == 1, "writer one pressure_plate");
            Expect(
                world::AuthoredLevelDataEqual(one.level, world::ParseLevelText(writtenOne).level),
                "one pressure_plate round trip");
            Expect(writtenOne.find("active") == std::string::npos, "activation is not serialized");

            const std::string twoPlates =
                canonical + "pressure_plate 2 0.1 0 2 0.2 2\npressure_plate 6 0.1 0 2 0.2 2\n";
            const world::ParseLevelFileResult two = world::ParseLevelText(twoPlates);
            Expect(two.status == world::LoadLevelFileStatus::Loaded, "two pressure_plate loads");
            Expect(two.level.pressurePlates.size() == 2, "two pressure_plate count");
            Expect(two.level.pressurePlates[0].center.x == 2.0f, "encounter order first plate");
            Expect(two.level.pressurePlates[1].center.x == 6.0f, "encounter order second plate");
            Expect(
                CountRecords(world::SerializeLevelText(two.level), "pressure_plate") == 2,
                "writer two pressure_plate");

            Expect(
                world::ParseLevelText(canonical + "pressure_plate nan 0.1 0 2 0.2 2\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "NaN pressure_plate position rejected");
            Expect(
                world::ParseLevelText(canonical + "pressure_plate inf 0.1 0 2 0.2 2\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "Inf pressure_plate position rejected");
            Expect(
                world::ParseLevelText(canonical + "pressure_plate 2 0.1 0 0 0.2 2\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "zero pressure_plate size rejected");
            Expect(
                world::ParseLevelText(canonical + "pressure_plate 2 0.1 0 -1 0.2 2\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "negative pressure_plate size rejected");
            Expect(
                world::ParseLevelText(canonical + "pressure_plate 2 0.1 0 0.119 0.2 2\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "below-minimum pressure_plate size rejected");
            Expect(
                world::ParseLevelText(canonical + "pressure_plate 2 0.1 0 0.12 0.12 0.12\n").status
                    == world::LoadLevelFileStatus::Loaded,
                "minimum pressure_plate extent accepted");

            world::LevelDefinition manyPlates = parsed.level;
            while (manyPlates.pressurePlates.size() < 8)
            {
                const float x = static_cast<float>(manyPlates.pressurePlates.size()) * 3.0f;
                manyPlates.pressurePlates.push_back({{x, 0.1f, 0.0f}, world::kDefaultPressurePlateSize});
            }
            RoundTrip(manyPlates, "8 pressure plates round trip");
            Expect(
                physics::AuthoredPhysicsBodiesWithinBudget(
                    static_cast<int>(manyPlates.elevatedPlatforms.size()),
                    static_cast<int>(manyPlates.dynamicBoxes.size()),
                    static_cast<int>(manyPlates.doors.size())),
                "Pressure Plates do not consume physics leftover");
        }

        {
            const std::string oneDoor = canonical + "door 4 1.5 0 1.2 3 2.4 3.2\n";
            const world::ParseLevelFileResult one = world::ParseLevelText(oneDoor);
            Expect(one.status == world::LoadLevelFileStatus::Loaded, "one door loads");
            Expect(one.level.doors.size() == 1, "one door count");
            Expect(one.level.doors[0].center.x == 4.0f, "one door center x");
            Expect(one.level.doors[0].openDistance == 3.2f, "one door openDistance");
            Expect(
                one.level.doors[0].requiredItemId.empty(),
                "old Door syntax defaults no required item");
            const std::string writtenOne = world::SerializeLevelText(one.level);
            Expect(CountRecords(writtenOne, "door") == 1, "writer one door");
            Expect(
                world::AuthoredLevelDataEqual(one.level, world::ParseLevelText(writtenOne).level),
                "one door round trip");
            Expect(writtenOne.find("openFraction") == std::string::npos, "writer omits openFraction");

            const std::string twoDoors =
                canonical + "door 4 1.5 0 1.2 3 2.4 3.2\ndoor 8 1.5 0 1.2 3 2.4 3.2\n";
            Expect(world::ParseLevelText(twoDoors).level.doors.size() == 2, "two doors load");
            Expect(
                world::ParseLevelText(canonical + "door nan 1.5 0 1.2 3 2.4 3.2\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "NaN door position rejected");
            Expect(
                world::ParseLevelText(canonical + "door 4 1.5 0 0 3 2.4 3.2\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "zero door size rejected");
            Expect(
                world::ParseLevelText(canonical + "door 4 1.5 0 1.2 3 2.4 0\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "zero openDistance rejected");

            const std::string noLink = canonical + "pressure_plate 2 0.1 0 2 0.2 2\n";
            Expect(
                world::ParseLevelText(noLink).level.pressurePlates[0].linkedDoorIndex
                    == world::kNoLinkedDoor,
                "7-token pressure_plate is no-link");
            const std::string linked =
                canonical + "door 4 1.5 0 1.2 3 2.4 3.2\npressure_plate 2 0.1 0 2 0.2 2 0\n";
            Expect(
                world::ParseLevelText(linked).level.pressurePlates[0].linkedDoorIndex == 0,
                "8-token pressure_plate Door 0 link");
            Expect(
                world::ParseLevelText(canonical + "pressure_plate 2 0.1 0 2 0.2 2 0\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "Door link without Doors rejected");
            const std::string oldSeven = canonical + "pressure_plate 2 0.1 0 2 0.2 2\n";
            const world::PressurePlateSpec oldPlate =
                world::ParseLevelText(oldSeven).level.pressurePlates[0];
            Expect(
                oldPlate.activateByDynamicBox && !oldPlate.activateByPlayer
                    && oldPlate.visibleInGameplay,
                "7-token pressure_plate defaults box-only visible");
            const std::string modes =
                canonical + "door 4 1.5 0 1.2 3 2.4 3.2\npressure_plate 2 0.1 0 2 0.2 2 0 0 1 0\n";
            const world::ParseLevelFileResult modeParsed = world::ParseLevelText(modes);
            Expect(modeParsed.status == world::LoadLevelFileStatus::Loaded, "11-token pressure_plate loads");
            Expect(
                !modeParsed.level.pressurePlates[0].activateByDynamicBox
                    && modeParsed.level.pressurePlates[0].activateByPlayer
                    && !modeParsed.level.pressurePlates[0].visibleInGameplay,
                "11-token pressure_plate flags parse");
            const std::string writtenModes = world::SerializeLevelText(modeParsed.level);
            Expect(
                writtenModes.find("pressure_plate 2 0.1 0 2 0.2 2 0 0 1 0 0") != std::string::npos,
                "canonical writer emits plate mode flags and light-control default");
            Expect(
                !modeParsed.level.pressurePlates[0].controlsDirectionalLight,
                "11-token pressure_plate does not control Directional Light");
            const std::string lightControl =
                canonical
                + "door 4 1.5 0 1.2 3 2.4 3.2\npressure_plate 2 0.1 0 2 0.2 2 0 0 1 0 1\n";
            const world::ParseLevelFileResult lightParsed = world::ParseLevelText(lightControl);
            Expect(
                lightParsed.status == world::LoadLevelFileStatus::Loaded,
                "12-token pressure_plate loads");
            Expect(
                lightParsed.level.pressurePlates[0].controlsDirectionalLight,
                "12-token pressure_plate light-control parses");
            Expect(
                lightParsed.level.pressurePlates[0].linkedDoorIndex == 0,
                "12-token pressure_plate keeps Door link");
            const std::string writtenLight = world::SerializeLevelText(lightParsed.level);
            Expect(
                writtenLight.find("pressure_plate 2 0.1 0 2 0.2 2 0 0 1 0 1") != std::string::npos,
                "writer emits deterministic light-control token");
            Expect(
                world::AuthoredLevelDataEqual(
                    lightParsed.level, world::ParseLevelText(writtenLight).level),
                "12-token pressure_plate roundtrip");
            Expect(
                !world::ParseLevelText(oldSeven).level.pressurePlates[0].controlsDirectionalLight,
                "7-token pressure_plate does not control Directional Light");
            world::LevelDefinition dirtyPlate = lightParsed.level;
            Expect(
                world::AuthoredLevelDataEqual(dirtyPlate, lightParsed.level),
                "Inspector no-op does not Dirty light-control");
            dirtyPlate.pressurePlates[0].controlsDirectionalLight = false;
            Expect(
                !world::AuthoredLevelDataEqual(dirtyPlate, lightParsed.level),
                "Inspector light-control edit sets Dirty");
            Expect(
                world::ParseLevelText(canonical + "pressure_plate 2 0.1 0 2 0.2 2 0 0 1 0 2\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "malformed pressure_plate light-control rejected");
            Expect(
                world::ParseLevelText(canonical + "pressure_plate 2 0.1 0 2 0.2 2 0 0 1 0 1 extra\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "13-token pressure_plate rejected");
            Expect(
                world::ParseLevelText(canonical + "pressure_plate 2 0.1 0 2 0.2 2 0 2 0 1\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "invalid pressure_plate bool flag rejected");
        }

        {
            const std::string oneGoal = canonical + "level_goal -21 3.8 0 2 1.6 1.8\n";
            const world::ParseLevelFileResult one = world::ParseLevelText(oneGoal);
            Expect(one.status == world::LoadLevelFileStatus::Loaded, "one extra level_goal loads");
            Expect(one.level.levelGoals.size() == 2, "appended level_goal count");
            Expect(Vec3Equal(one.level.levelGoals.back().center, {-21.0f, 3.8f, 0.0f}),
                "appended level_goal center");
            Expect(Vec3Equal(one.level.levelGoals.back().size, {2.0f, 1.6f, 1.8f}),
                "appended level_goal size");
            const std::string writtenOne = world::SerializeLevelText(one.level);
            Expect(CountRecords(writtenOne, "level_goal") == 2, "writer appended level_goal");
            Expect(
                world::AuthoredLevelDataEqual(one.level, world::ParseLevelText(writtenOne).level),
                "appended level_goal round trip");
            Expect(writtenOne.find("completed") == std::string::npos, "runtime completion not serialized");
            Expect(one.level.levelGoals.back().nextLevelId.empty(), "7-token goal is terminal");
            Expect(
                writtenOne.find("level_goal -21 3.8 0 2 1.6 1.8\n") != std::string::npos,
                "writer omits empty destination");

            const std::string destinationGoal =
                canonical + "level_goal -21 3.8 0 2 1.6 1.8 level_02\n";
            const world::ParseLevelFileResult destination = world::ParseLevelText(destinationGoal);
            Expect(destination.status == world::LoadLevelFileStatus::Loaded, "destination goal loads");
            Expect(
                destination.level.levelGoals.size() == 2
                    && destination.level.levelGoals.back().nextLevelId == "level_02",
                "destination goal captures nextLevelId");
            const std::string writtenDestination = world::SerializeLevelText(destination.level);
            Expect(
                writtenDestination.find("level_goal -21 3.8 0 2 1.6 1.8 level_02\n") != std::string::npos,
                "writer emits destination identity");
            Expect(
                world::AuthoredLevelDataEqual(
                    destination.level, world::ParseLevelText(writtenDestination).level),
                "destination goal round trip");
            world::LevelDefinition destinationEdit = destination.level;
            destinationEdit.levelGoals.back().nextLevelId.clear();
            Expect(
                !world::AuthoredLevelDataEqual(destination.level, destinationEdit),
                "authored equality detects Next Level edit");

            const std::string twoGoals = canonical
                + "level_goal -21 3.8 0 2 1.6 1.8\n"
                  "level_goal 8 1 0 2 1.6 1.8\n";
            const world::ParseLevelFileResult two = world::ParseLevelText(twoGoals);
            Expect(two.status == world::LoadLevelFileStatus::Loaded, "two extra level_goal load");
            Expect(two.level.levelGoals.size() == 3, "two extra level_goal count");
            Expect(Vec3Equal(two.level.levelGoals.back().center, {8.0f, 1.0f, 0.0f}),
                "second extra level_goal center");
            const std::string writtenTwo = world::SerializeLevelText(two.level);
            Expect(CountRecords(writtenTwo, "level_goal") == 3, "writer two extra level_goal");
            Expect(
                world::AuthoredLevelDataEqual(two.level, world::ParseLevelText(writtenTwo).level),
                "two level_goal round trip");

            Expect(
                world::ParseLevelText(canonical + "level_goal nan 3.8 0 2 1.6 1.8\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "non-finite level_goal center rejected");
            Expect(
                world::ParseLevelText(canonical + "level_goal -21 3.8 0 0 1.6 1.8\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "zero level_goal extent rejected");
            Expect(
                world::ParseLevelText(canonical + "level_goal -21 3.8 0 -2 1.6 1.8\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "negative level_goal extent rejected");
            Expect(
                world::ParseLevelText(canonical + "level_goal -21 3.8 0 0.05 1.6 1.8\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "below-minimum level_goal extent rejected");
            Expect(
                world::ParseLevelText(canonical + "level_goal -21 3.8 0 2 1.6\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "short level_goal token count rejected");
            Expect(
                world::ParseLevelText(canonical + "level_goal -21 3.8 0 2 1.6 1.8 extra extra\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "two extra level_goal tokens rejected");
            Expect(
                world::ParseLevelText(canonical + "level_goal -21 3.8 0 2 1.6 1.8 ../secret\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "traversal destination rejected");
            Expect(
                world::ParseLevelText(canonical + "level_goal -21 3.8 0 2 1.6 1.8 levels/level_02\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "path destination rejected");
            Expect(
                world::ParseLevelText(canonical + "level_goal -21 3.8 0 2 1.6 1.8 level_02.level\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "extension destination rejected");
            Expect(
                world::ParseLevelText(canonical + "goal -21 3.8 0 2 1.6 1.8\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "legacy singleton goal keyword rejected");
        }

        {
            const std::string onePickup = canonical + "item_pickup 2 0.5 0 1 key\n";
            const world::ParseLevelFileResult one = world::ParseLevelText(onePickup);
            Expect(one.status == world::LoadLevelFileStatus::Loaded, "one item_pickup loads");
            Expect(one.level.itemPickups.size() == 1, "one item_pickup count");
            Expect(one.level.itemPickups[0].itemId == "key", "one item_pickup itemId");
            Expect(one.level.itemPickups[0].quantity == 1, "one item_pickup quantity");
            Expect(one.level.itemPickups[0].position.x == 2.0f, "one item_pickup position x");
            Expect(one.level.itemPickups[0].modelIdentity.empty(), "one item_pickup has no model");
            Expect(one.level.itemPickups[0].visualOffset.x == 0.0f
                    && one.level.itemPickups[0].visualOffset.y == 0.0f
                    && one.level.itemPickups[0].visualOffset.z == 0.0f,
                "old item_pickup visualOffset defaults to 0");
            Expect(one.level.itemPickups[0].visualRotationDegrees.x == 0.0f
                    && one.level.itemPickups[0].visualRotationDegrees.y == 0.0f
                    && one.level.itemPickups[0].visualRotationDegrees.z == 0.0f,
                "old item_pickup visualRotation defaults to 0");
            Expect(one.level.itemPickups[0].visualScale.x == 1.0f
                    && one.level.itemPickups[0].visualScale.y == 1.0f
                    && one.level.itemPickups[0].visualScale.z == 1.0f,
                "old item_pickup visualScale defaults to 1");
            Expect(
                one.level.itemPickups[0].showInteractionBounds,
                "legacy item_pickup showInteractionBounds defaults true");
            Expect(
                one.level.itemPickups[0].targetHighlightIntensity
                    == world::kDefaultItemPickupTargetHighlightIntensity,
                "legacy item_pickup targetHighlightIntensity defaults 0.70");
            const std::string writtenOne = world::SerializeLevelText(one.level);
            Expect(CountRecords(writtenOne, "item_pickup") == 1, "writer one item_pickup");
            Expect(
                world::AuthoredLevelDataEqual(one.level, world::ParseLevelText(writtenOne).level),
                "one item_pickup round trip");
            Expect(writtenOne.find("collected") == std::string::npos,
                "writer omits collected runtime state");

            const std::string modeled =
                canonical + "item_pickup 3 1 0 2 coin models/test_static.glb\n";
            const world::ParseLevelFileResult modeledParsed = world::ParseLevelText(modeled);
            Expect(modeledParsed.status == world::LoadLevelFileStatus::Loaded,
                "item_pickup with model loads");
            Expect(
                modeledParsed.level.itemPickups[0].modelIdentity == "models/test_static.glb",
                "optional model identity");
            Expect(
                world::AuthoredLevelDataEqual(
                    modeledParsed.level,
                    world::ParseLevelText(world::SerializeLevelText(modeledParsed.level)).level),
                "modeled item_pickup round trip");

            const std::string twoPickups = canonical
                + "item_pickup 2 0.5 0 1 key\n"
                  "item_pickup 4 0.5 0 5 coin\n";
            const world::ParseLevelFileResult two = world::ParseLevelText(twoPickups);
            Expect(two.status == world::LoadLevelFileStatus::Loaded, "two item_pickup load");
            Expect(two.level.itemPickups.size() == 2, "two item_pickup count");
            Expect(two.level.itemPickups[1].itemId == "coin", "encounter order second itemId");

            Expect(
                world::ParseLevelText(canonical + "item_pickup nan 0.5 0 1 key\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "non-finite item_pickup position rejected");
            Expect(
                world::ParseLevelText(canonical + "item_pickup 2 0.5 0 1 Key\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "invalid M54 itemId rejected");
            Expect(
                world::ParseLevelText(canonical + "item_pickup 2 0.5 0 0 key\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "zero item_pickup quantity rejected");
            Expect(
                world::ParseLevelText(canonical + "item_pickup 2 0.5 0 -1 key\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "negative item_pickup quantity rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key models/missing.txt\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "invalid item_pickup model identity rejected");

            const std::string visualPickup =
                canonical
                + "item_pickup 5 1 0 2 coin visual 0 0.5 0 0 90 0 0.15 0.2 0.25 "
                  "models/test_static.glb\n";
            const world::ParseLevelFileResult visualParsed = world::ParseLevelText(visualPickup);
            Expect(visualParsed.status == world::LoadLevelFileStatus::Loaded,
                "M58 visual item_pickup loads");
            Expect(visualParsed.level.itemPickups[0].position.x == 5.0f, "visual record keeps gameplay x");
            Expect(visualParsed.level.itemPickups[0].visualOffset.y == 0.5f, "visualOffset y");
            Expect(visualParsed.level.itemPickups[0].visualRotationDegrees.y == 90.0f,
                "visualRotation y");
            Expect(visualParsed.level.itemPickups[0].visualScale.x == 0.15f, "visualScale x");
            Expect(
                visualParsed.level.itemPickups[0].modelIdentity == "models/test_static.glb",
                "visual record keeps model identity");
            const std::string writtenVisual = world::SerializeLevelText(visualParsed.level);
            Expect(writtenVisual.find(" visual ") != std::string::npos,
                "canonical writer emits visual marker");
            Expect(writtenVisual.find(" bounds 1") != std::string::npos,
                "canonical writer emits bounds marker default true");
            Expect(writtenVisual.find(" highlight ") != std::string::npos,
                "canonical writer emits highlight marker");
            Expect(writtenVisual.find("models/test_static.glb") != std::string::npos,
                "canonical writer keeps identity after visual fields");
            Expect(
                world::AuthoredLevelDataEqual(
                    visualParsed.level, world::ParseLevelText(writtenVisual).level),
                "M58 visual item_pickup round trip");

            const std::string spaced =
                canonical
                + "item_pickup 3 1 0 1 key models/Chest by Quaternius - O72u4Drp8k.glb\n";
            const world::ParseLevelFileResult spacedParsed = world::ParseLevelText(spaced);
            Expect(spacedParsed.status == world::LoadLevelFileStatus::Loaded,
                "old spaced modelIdentity still parses");
            Expect(
                spacedParsed.level.itemPickups[0].modelIdentity
                    == "models/Chest by Quaternius - O72u4Drp8k.glb",
                "old spaced identity reassembles");
            Expect(
                spacedParsed.level.itemPickups[0].visualScale.x == 1.0f,
                "old spaced identity keeps default visualScale");
            Expect(
                spacedParsed.level.itemPickups[0].showInteractionBounds,
                "old spaced identity defaults showInteractionBounds true");
            const std::string writtenSpaced = world::SerializeLevelText(spacedParsed.level);
            Expect(
                world::AuthoredLevelDataEqual(
                    spacedParsed.level, world::ParseLevelText(writtenSpaced).level),
                "spaced modelIdentity round-trips through visual writer");
            Expect(
                writtenSpaced.find("models/Chest by Quaternius - O72u4Drp8k.glb") != std::string::npos,
                "writer keeps spaced identity after visual fields");

            const std::string emptyVisual =
                canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1\n";
            const world::ParseLevelFileResult emptyVisualParsed = world::ParseLevelText(emptyVisual);
            Expect(emptyVisualParsed.status == world::LoadLevelFileStatus::Loaded,
                "visual record with empty identity is valid");
            Expect(emptyVisualParsed.level.itemPickups[0].modelIdentity.empty(),
                "visual-only record has empty identity");

            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key visual 0 inf 0 0 0 0 1 1 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "non-finite visualOffset rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0 nan 0 0 1 1 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "non-finite visualRotation rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 inf 1 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "non-finite visualScale rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 0 1 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "zero visualScale rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 -1 1 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "negative visualScale rejected");
            Expect(
                world::ParseLevelText(canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "incomplete visual segment rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 models/missing.txt\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "invalid identity after visual segment rejected");

            const std::string boundsOff =
                canonical
                + "item_pickup 5 1 0 2 coin visual 0 0.5 0 0 90 0 0.15 0.2 0.25 bounds 0 "
                  "models/test_static.glb\n";
            const world::ParseLevelFileResult boundsOffParsed = world::ParseLevelText(boundsOff);
            Expect(boundsOffParsed.status == world::LoadLevelFileStatus::Loaded,
                "item_pickup bounds 0 loads");
            Expect(
                !boundsOffParsed.level.itemPickups[0].showInteractionBounds,
                "exact 0 parses showInteractionBounds false");
            Expect(
                boundsOffParsed.level.itemPickups[0].modelIdentity == "models/test_static.glb",
                "bounds marker does not consume modelIdentity");
            const std::string writtenBoundsOff = world::SerializeLevelText(boundsOffParsed.level);
            Expect(writtenBoundsOff.find(" bounds 0") != std::string::npos,
                "canonical writer emits bounds 0");
            Expect(
                world::AuthoredLevelDataEqual(
                    boundsOffParsed.level, world::ParseLevelText(writtenBoundsOff).level),
                "bounds 0 round trip");

            const std::string boundsOn =
                canonical
                + "item_pickup 5 1 0 2 coin visual 0 0.5 0 0 90 0 0.15 0.2 0.25 bounds 1 "
                  "models/test_static.glb\n";
            const world::ParseLevelFileResult boundsOnParsed = world::ParseLevelText(boundsOn);
            Expect(boundsOnParsed.status == world::LoadLevelFileStatus::Loaded,
                "item_pickup bounds 1 loads");
            Expect(
                boundsOnParsed.level.itemPickups[0].showInteractionBounds,
                "exact 1 parses showInteractionBounds true");
            world::LevelDefinition boundsChanged = boundsOnParsed.level;
            boundsChanged.itemPickups[0].showInteractionBounds = false;
            Expect(
                !world::AuthoredLevelDataEqual(boundsOnParsed.level, boundsChanged),
                "authored equality detects showInteractionBounds change");

            const std::string spacedBounds =
                canonical
                + "item_pickup 3 1 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 0 "
                  "models/Chest by Quaternius - O72u4Drp8k.glb\n";
            const world::ParseLevelFileResult spacedBoundsParsed =
                world::ParseLevelText(spacedBounds);
            Expect(spacedBoundsParsed.status == world::LoadLevelFileStatus::Loaded,
                "bounds before spaced modelIdentity loads");
            Expect(
                spacedBoundsParsed.level.itemPickups[0].modelIdentity
                    == "models/Chest by Quaternius - O72u4Drp8k.glb",
                "modelIdentity with spaces round-trips after bounds");
            Expect(
                !spacedBoundsParsed.level.itemPickups[0].showInteractionBounds,
                "spaced identity keeps parsed bounds 0");
            const std::string writtenSpacedBounds =
                world::SerializeLevelText(spacedBoundsParsed.level);
            Expect(
                writtenSpacedBounds.find("models/Chest by Quaternius - O72u4Drp8k.glb")
                    != std::string::npos,
                "writer keeps spaced identity after bounds");
            Expect(
                world::AuthoredLevelDataEqual(
                    spacedBoundsParsed.level, world::ParseLevelText(writtenSpacedBounds).level),
                "spaced identity with bounds round trip");

            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 2\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "invalid bounds token rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds true\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "non 0/1 bounds token rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "incomplete bounds marker rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key bounds 0\n").status
                    == world::LoadLevelFileStatus::Loaded,
                "bounds without visual is valid");
            Expect(
                world::ParseLevelText(canonical + "item_pickup 2 0.5 0 1 key bounds 0\n")
                    .level.itemPickups[0]
                    .showInteractionBounds
                    == false,
                "bounds 0 without visual parses false");

            const std::string highlightRecord =
                canonical
                + "item_pickup 5 1 0 2 coin visual 0 0.5 0 0 90 0 0.15 0.2 0.25 bounds 1 "
                  "highlight 0.25 models/test_static.glb\n";
            const world::ParseLevelFileResult highlightParsed =
                world::ParseLevelText(highlightRecord);
            Expect(highlightParsed.status == world::LoadLevelFileStatus::Loaded,
                "item_pickup highlight loads");
            Expect(
                highlightParsed.level.itemPickups[0].targetHighlightIntensity == 0.25f,
                "highlight 0.25 round-trips");
            Expect(
                highlightParsed.level.itemPickups[0].modelIdentity == "models/test_static.glb",
                "highlight marker does not consume modelIdentity");
            const std::string writtenHighlight = world::SerializeLevelText(highlightParsed.level);
            Expect(writtenHighlight.find(" highlight ") != std::string::npos,
                "canonical writer emits highlight marker after bounds");
            Expect(
                world::AuthoredLevelDataEqual(
                    highlightParsed.level, world::ParseLevelText(writtenHighlight).level),
                "highlight value round trip");

            const std::string spacedHighlight =
                canonical
                + "item_pickup 3 1 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0 "
                  "models/Chest by Quaternius - O72u4Drp8k.glb\n";
            const world::ParseLevelFileResult spacedHighlightParsed =
                world::ParseLevelText(spacedHighlight);
            Expect(spacedHighlightParsed.status == world::LoadLevelFileStatus::Loaded,
                "highlight before spaced modelIdentity loads");
            Expect(
                spacedHighlightParsed.level.itemPickups[0].modelIdentity
                    == "models/Chest by Quaternius - O72u4Drp8k.glb",
                "modelIdentity with spaces round-trips after highlight");
            Expect(
                spacedHighlightParsed.level.itemPickups[0].targetHighlightIntensity == 0.0f,
                "highlight 0 parses as zero intensity");

            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "incomplete highlight marker rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight abc\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "malformed highlight value rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight -0.1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "out-of-range negative highlight rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 1.01\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "out-of-range highlight >1 rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight nan\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "NaN highlight rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight inf\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "Inf highlight rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "item_pickup 2 0.5 0 1 key bounds 1 highlight 1\n").status
                    == world::LoadLevelFileStatus::Loaded,
                "highlight 1.0 without visual is valid");
            const world::ParseLevelFileResult legacyM584 = world::ParseLevelText(
                canonical + "item_pickup 2 0.5 0 1 key bounds 1 highlight 1\n");
            Expect(
                legacyM584.level.itemPickups[0].targetHighlightGoldAmount
                    == world::kDefaultItemPickupTargetHighlightGoldAmount,
                "16. old M58.4 record receives M59 gold default");
            Expect(
                !legacyM584.level.itemPickups[0].idleAnimationEnabled
                    && legacyM584.level.itemPickups[0].idleBobAmplitude
                        == world::kDefaultItemPickupIdleBobAmplitude
                    && legacyM584.level.itemPickups[0].idleBobSpeed
                        == world::kDefaultItemPickupIdleBobSpeed
                    && legacyM584.level.itemPickups[0].idleSpinSpeedDegrees
                        == world::kDefaultItemPickupIdleSpinSpeedDegrees,
                "16. old M58.4 record receives M59 idle defaults");

            const std::string goldIdleRecord =
                canonical
                + "item_pickup 5 1 0 2 coin visual 0 0.5 0 0 90 0 0.15 0.2 0.25 bounds 1 "
                  "highlight 0.25 gold 0.5 idle 1 0.2 2 90 models/test_static.glb\n";
            const world::ParseLevelFileResult goldIdleParsed =
                world::ParseLevelText(goldIdleRecord);
            Expect(goldIdleParsed.status == world::LoadLevelFileStatus::Loaded,
                "18. full M59 record loads");
            Expect(
                goldIdleParsed.level.itemPickups[0].targetHighlightGoldAmount == 0.5f,
                "gold 0.5 round-trips");
            Expect(
                goldIdleParsed.level.itemPickups[0].idleAnimationEnabled
                    && goldIdleParsed.level.itemPickups[0].idleBobAmplitude == 0.2f
                    && goldIdleParsed.level.itemPickups[0].idleBobSpeed == 2.0f
                    && goldIdleParsed.level.itemPickups[0].idleSpinSpeedDegrees == 90.0f,
                "idle 1 0.2 2 90 round-trips");
            Expect(
                goldIdleParsed.level.itemPickups[0].modelIdentity == "models/test_static.glb",
                "gold/idle markers do not consume modelIdentity");
            const std::string writtenGoldIdle = world::SerializeLevelText(goldIdleParsed.level);
            Expect(
                writtenGoldIdle.find(" highlight ") != std::string::npos
                    && writtenGoldIdle.find(" gold ") != std::string::npos
                    && writtenGoldIdle.find(" idle ") != std::string::npos,
                "17. canonical writer emits deterministic M59 markers");
            const std::size_t highlightAt = writtenGoldIdle.find(" highlight ");
            const std::size_t goldAt = writtenGoldIdle.find(" gold ");
            const std::size_t idleAt = writtenGoldIdle.find(" idle ");
            Expect(
                highlightAt != std::string::npos && goldAt != std::string::npos
                    && idleAt != std::string::npos && highlightAt < goldAt && goldAt < idleAt,
                "17. writer order is highlight, gold, idle");
            Expect(
                world::AuthoredLevelDataEqual(
                    goldIdleParsed.level, world::ParseLevelText(writtenGoldIdle).level),
                "18. full M59 record round-trips");

            const std::string spacedGold =
                canonical
                + "item_pickup 3 1 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0 "
                  "gold 1 idle 0 0.15 1 45 models/Chest by Quaternius - O72u4Drp8k.glb\n";
            const world::ParseLevelFileResult spacedGoldParsed =
                world::ParseLevelText(spacedGold);
            Expect(spacedGoldParsed.status == world::LoadLevelFileStatus::Loaded,
                "19. gold/idle before spaced modelIdentity loads");
            Expect(
                spacedGoldParsed.level.itemPickups[0].modelIdentity
                    == "models/Chest by Quaternius - O72u4Drp8k.glb",
                "19. modelIdentity with spaces remains unchanged");
            Expect(
                spacedGoldParsed.level.itemPickups[0].targetHighlightGoldAmount == 1.0f
                    && !spacedGoldParsed.level.itemPickups[0].idleAnimationEnabled,
                "spaced identity keeps gold/idle values");

            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "21. incomplete gold marker rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold abc\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "21. malformed gold float rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold -0.1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "13. out-of-range gold rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold nan\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "14. NaN gold rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold inf\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "15. Inf gold rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold 0.7 idle 2 0.15 1 45\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "20. malformed idle bool rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold 0.7 idle 1 abc 1 45\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "21. malformed idle float rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold 0.7 idle 1 2.01 1 45\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "13. idle amplitude out of range rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold 0.7 idle 1 0.15 1 720.01\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "13. idle spin out of range rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold 0.7 idle 1 0.15 inf 45\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "15. Inf idle speed rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold 0 gold 0.5\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "22. duplicate gold is not Level Format v2 / ambiguous");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold 0.7 idle 0 0 0 -720\n")
                    .status
                    == world::LoadLevelFileStatus::Loaded,
                "12. idle range endpoints accepted");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "item_pickup 2 0.5 0 1 key visual 0 0 0 0 0 0 1 1 1 bounds 1 highlight 0.7 "
                      "gold 1 idle 1 2 10 720\n")
                    .status
                    == world::LoadLevelFileStatus::Loaded,
                "12. idle and gold max endpoints accepted");
        }

        {
            const std::string oneProp =
                canonical + "static_prop 3 1.5 0 0 45 0 1 2 1 models/test_static.glb\n";
            const world::ParseLevelFileResult one = world::ParseLevelText(oneProp);
            Expect(one.status == world::LoadLevelFileStatus::Loaded, "one static_prop loads");
            Expect(one.level.staticProps.size() == 1, "one static_prop count");
            Expect(one.level.staticProps[0].modelIdentity == "models/test_static.glb",
                "one static_prop identity");
            Expect(one.level.staticProps[0].position.x == 3.0f, "one static_prop position x");
            Expect(one.level.staticProps[0].rotationDegrees.y == 45.0f, "one static_prop rotation y");
            Expect(one.level.staticProps[0].scale.y == 2.0f, "one static_prop scale y");
            const std::string writtenOne = world::SerializeLevelText(one.level);
            Expect(CountRecords(writtenOne, "static_prop") == 1, "writer one static_prop");
            Expect(
                world::AuthoredLevelDataEqual(one.level, world::ParseLevelText(writtenOne).level),
                "one static_prop round trip");

            const std::string twoProps = canonical
                + "static_prop 3 1.5 0 0 45 0 1 2 1 models/test_static.glb\n"
                  "static_prop -2 0.5 1 10 0 90 0.5 0.5 0.5 models/test_authored.glb\n";
            const world::ParseLevelFileResult two = world::ParseLevelText(twoProps);
            Expect(two.status == world::LoadLevelFileStatus::Loaded, "two static_prop loads");
            Expect(two.level.staticProps.size() == 2, "two static_prop count");
            Expect(
                two.level.staticProps[0].modelIdentity == "models/test_static.glb",
                "encounter order first identity");
            Expect(
                two.level.staticProps[1].modelIdentity == "models/test_authored.glb",
                "encounter order second identity");
            Expect(
                CountRecords(world::SerializeLevelText(two.level), "static_prop") == 2,
                "writer two static_prop");
            Expect(
                world::AuthoredLevelDataEqual(two.level, world::ParseLevelText(
                    world::SerializeLevelText(two.level)).level),
                "two static_prop round trip");

            Expect(
                world::ParseLevelText(canonical).status == world::LoadLevelFileStatus::Loaded
                    && world::ParseLevelText(canonical).level.staticProps.empty(),
                "old levels with zero static_prop remain valid");

            const world::ParseLevelFileResult missingEnvironment = world::ParseLevelText(canonical);
            Expect(
                missingEnvironment.status == world::LoadLevelFileStatus::Loaded,
                "old Level without Environment syntax remains valid");
            Expect(
                world::LevelEnvironmentEqual(
                    missingEnvironment.level.environment, world::MakeDefaultLevelEnvironment()),
                "missing Environment resolves to M85 defaults");
            Expect(
                missingEnvironment.level.environment.directionalEnabled,
                "default directional enabled");
            Expect(
                missingEnvironment.level.environment.directionalShadowsEnabled,
                "default shadows enabled");

            world::LevelDefinition authoredLighting = missingEnvironment.level;
            authoredLighting.environment.ambientIntensity = 0.5f;
            authoredLighting.environment.directionalEnabled = false;
            authoredLighting.environment.directionalShadowsEnabled = false;
            authoredLighting.environment.directionalRayDirection =
                world::CanonicalLevelDirectionalRay({0.0f, -1.0f, 0.2f});
            authoredLighting.environment.directionalColor = {0.8f, 0.7f, 0.6f};
            authoredLighting.environment.directionalIntensity = 0.4f;
            authoredLighting.environment.ambientColor = {0.9f, 0.95f, 1.0f};
            const std::string lightingText = world::SerializeLevelText(authoredLighting);
            Expect(CountRecords(lightingText, "environment") == 1, "serialize environment");
            Expect(CountRecords(lightingText, "directional_light") == 1, "serialize directional");
            const world::ParseLevelFileResult lightingRoundTrip =
                world::ParseLevelText(lightingText);
            Expect(
                lightingRoundTrip.status == world::LoadLevelFileStatus::Loaded
                    && world::AuthoredLevelDataEqual(authoredLighting, lightingRoundTrip.level),
                "Environment/Directional Light roundtrip");
            Expect(
                lightingRoundTrip.level.environment.directionalEnabled == false,
                "enabled false roundtrips");
            Expect(
                lightingRoundTrip.level.environment.directionalShadowsEnabled == false,
                "shadows false roundtrips");

            Expect(
                world::ParseLevelText(canonical + "environment 1 1 1 0.34\n").status
                    == world::LoadLevelFileStatus::Loaded,
                "explicit default environment is valid");
            Expect(
                world::ParseLevelText(
                    canonical + "environment 1 1 1 0.34\nenvironment 1 1 1 0.2\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "duplicate environment is invalid");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "directional_light 1 0 -1 0 1 0.96 0.88 0.88 1\n"
                      "directional_light 1 0 -1 0 1 0.96 0.88 0.88 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "duplicate directional_light is invalid");
            Expect(
                world::ParseLevelText(canonical + "environment 2 0 0 0.34\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "ambient color above 1 is invalid");
            Expect(
                world::ParseLevelText(canonical + "environment 1 1 1 -0.1\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "negative ambient intensity is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "directional_light 1 0 -1 0 2 0 0 0.88 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "directional color above 1 is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "directional_light 1 0 -1 0 1 0.96 0.88 -1 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "negative directional intensity is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "directional_light 1 0 0 0 1 0.96 0.88 0.88 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "zero directional ray is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "directional_light true 0 -1 0 1 0.96 0.88 0.88 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "malformed bool is invalid");
            Expect(
                world::ParseLevelText(canonical + "linkedLightIndex 0\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "gameplay light-link syntax is absent");
            Expect(
                world::ParseLevelText(canonical + "ambient_light 1 1 1 0.3\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "ambient_light remains unrecognized");
            const world::ParseLevelFileResult directionalLight =
                world::ParseLevelText(canonical + "directional_light 0 -1 0 1 1 1 1\n");
            Expect(
                directionalLight.status == world::LoadLevelFileStatus::Invalid,
                "short directional_light record is invalid");
            const world::ParseLevelFileResult shadowSettings =
                world::ParseLevelText(canonical + "shadow_settings 2048 0.002 72\n");
            Expect(
                shadowSettings.status == world::LoadLevelFileStatus::Invalid,
                "shadow_settings is not Level syntax");

            world::LevelDefinition dirtyProbe = missingEnvironment.level;
            Expect(
                world::AuthoredLevelDataEqual(dirtyProbe, missingEnvironment.level),
                "no-op Environment compare is equal");
            dirtyProbe.environment.ambientIntensity = 0.41f;
            Expect(
                !world::AuthoredLevelDataEqual(dirtyProbe, missingEnvironment.level),
                "ambient intensity edit is a semantic authored change");

            Expect(
                world::ParseLevelText(
                    canonical + "point_light 2 3 4 1 0.5 0.25 1.5 8 1\n")
                    .status
                    == world::LoadLevelFileStatus::Loaded,
                "valid point_light");
            const world::ParseLevelFileResult onePoint = world::ParseLevelText(
                canonical + "point_light 2 3 4 1 0.5 0.25 1.5 8 1\n");
            Expect(onePoint.level.pointLights.size() == 1, "one point_light count");
            Expect(
                onePoint.level.pointLights[0].position.x == 2.0f
                    && onePoint.level.pointLights[0].enabled,
                "point_light fields");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "point_light 0 2 0 1 1 1 1 8 1\n"
                      "point_light 4 2 0 0.2 0.4 1 2 6 0\n")
                    .status
                    == world::LoadLevelFileStatus::Loaded
                    && world::ParseLevelText(
                           canonical
                           + "point_light 0 2 0 1 1 1 1 8 1\n"
                             "point_light 4 2 0 0.2 0.4 1 2 6 0\n")
                           .level.pointLights.size()
                        == 2,
                "multiple point_light records");
            Expect(
                world::ParseLevelText(
                    canonical + "spot_light 1 4 0 0 -1 0 1 1 1 1.5 8 20 35 1\n")
                    .status
                    == world::LoadLevelFileStatus::Loaded,
                "valid spot_light");
            const world::ParseLevelFileResult oneSpot = world::ParseLevelText(
                canonical + "spot_light 1 4 0 0 -2 0 1 1 1 1.5 8 20 35 1\n");
            Expect(oneSpot.status == world::LoadLevelFileStatus::Loaded, "spot direction loads");
            Expect(
                oneSpot.level.spotLights.size() == 1
                    && oneSpot.level.spotLights[0].direction.y == -1.0f,
                "spot direction is normalized");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "spot_light 0 4 0 0 -1 0 1 1 1 1 8 15 30 1\n"
                      "spot_light 6 4 0 1 0 0 1 0.8 0.6 2 10 10 25 0\n")
                    .level.spotLights.size()
                    == 2,
                "multiple spot_light records");
            const world::ParseLevelFileResult mixedLights = world::ParseLevelText(
                canonical
                + "point_light 0 2 0 1 1 1 1 8 1\n"
                  "spot_light 1 4 0 0 -1 0 1 1 1 1.5 8 20 35 1\n");
            Expect(
                mixedLights.status == world::LoadLevelFileStatus::Loaded
                    && mixedLights.level.pointLights.size() == 1
                    && mixedLights.level.spotLights.size() == 1,
                "mixed point/spot lights");
            const std::string mixedText = world::SerializeLevelText(mixedLights.level);
            Expect(CountRecords(mixedText, "point_light") == 1, "writer point_light count");
            Expect(CountRecords(mixedText, "spot_light") == 1, "writer spot_light count");
            Expect(
                world::AuthoredLevelDataEqual(
                    mixedLights.level, world::ParseLevelText(mixedText).level),
                "local light writer roundtrip");
            Expect(
                missingEnvironment.level.pointLights.empty()
                    && missingEnvironment.level.spotLights.empty(),
                "old Levels without local lights remain valid");
            Expect(
                world::ParseLevelText(canonical + "point_light 0 2 0 1 1 1 1 8\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "short point_light token count is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "spot_light 1 4 0 0 -1 0 1 1 1 1.5 8 20 35\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "short spot_light token count is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "point_light 0 2 0 1 1 1 1 8 true\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "invalid point_light bool");
            Expect(
                world::ParseLevelText(
                    canonical + "point_light 0 nan 0 1 1 1 1 8 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "non-finite point_light is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "point_light 0 2 0 1 1 1 -1 8 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "negative intensity is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "point_light 0 2 0 1 1 1 1 0 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "zero range is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "spot_light 1 4 0 0 0 0 1 1 1 1.5 8 20 35 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "zero spot direction is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "spot_light 1 4 0 0 -1 0 1 1 1 1.5 8 35 20 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "inner >= outer cone is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "spot_light 1 4 0 0 -1 0 1 1 1 1.5 8 -1 35 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "negative inner cone is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "spot_light 1 4 0 0 -1 0 1 1 1 1.5 8 20 120 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "outer cone above bound is invalid");
            world::LevelDefinition defaultsProbe = missingEnvironment.level;
            defaultsProbe.pointLights.push_back(world::MakeDefaultPointLight({0.0f, 2.0f, 0.0f}));
            defaultsProbe.spotLights.push_back(world::MakeDefaultSpotLight({1.0f, 4.0f, 0.0f}));
            Expect(world::PointLightIsValid(defaultsProbe.pointLights[0]), "default point is valid");
            Expect(world::SpotLightIsValid(defaultsProbe.spotLights[0]), "default spot is valid");
            Expect(
                world::ParseLevelText(world::SerializeLevelText(defaultsProbe)).status
                    == world::LoadLevelFileStatus::Loaded,
                "default local lights serialize/parse");
            Expect(
                world::CountLevelV1RecordLines(defaultsProbe)
                    == world::CountLevelV1RecordLines(missingEnvironment.level) + 2,
                "local lights count toward Level v1 line constraints");
            Expect(
                world::AuthoredLevelDataEqual(defaultsProbe, defaultsProbe),
                "Inspector no-op local-light compare is equal");
            world::LevelDefinition dirtyLights = defaultsProbe;
            dirtyLights.pointLights[0].intensity = 2.0f;
            Expect(
                !world::AuthoredLevelDataEqual(defaultsProbe, dirtyLights),
                "Inspector semantic intensity edit is Dirty");
            dirtyLights = defaultsProbe;
            dirtyLights.spotLights[0].innerConeDegrees = 18.0f;
            Expect(
                !world::AuthoredLevelDataEqual(defaultsProbe, dirtyLights),
                "Inspector semantic cone edit is Dirty");
            Expect(
                world::ParseLevelText(
                    canonical + "point_light 0 2 0 1 1 1 1 8 1 extra\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "extra point_light tokens are invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "spot_light 1 4 0 0 -1 0 1 1 1 1.5 8 20 35 1 extra\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "extra spot_light tokens are invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "point_light 0 2 0 1 1 1 5 8 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "point intensity above bound is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "point_light 0 2 0 1.1 1 1 1 8 1\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "point color above 1 is invalid");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop 0 1 0 0 0 0 1 1 1 models/missing_not_on_disk.glb\n")
                    .status
                    == world::LoadLevelFileStatus::Loaded,
                "missing on-disk asset is still a valid authored identity");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop 0 1 0 0 0 0 1 1 1 models/../secret.glb\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "traversal identity rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop 0 1 0 0 0 0 1 1 1 C:/temp/crate.glb\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "absolute identity rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop 0 1 0 0 0 0 1 1 1 models/crate.txt\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "non-glb identity rejected");
            Expect(
                world::ParseLevelText(canonical + "static_prop 0 1 0 0 0 0 1 1 1\n").status
                    == world::LoadLevelFileStatus::Invalid,
                "missing identity rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "static_prop 0 1 0 0 0 0 1 1 1 models/test_static.glb material 1 0 0\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "M84: no Level material override syntax");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop nan 1 0 0 0 0 1 1 1 models/test_static.glb\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "NaN position rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop 0 inf 0 0 0 0 1 1 1 models/test_static.glb\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "Inf position rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop 0 1 0 nan 0 0 1 1 1 models/test_static.glb\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "NaN rotation rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop 0 1 0 0 0 0 0 1 1 models/test_static.glb\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "zero scale rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop 0 1 0 0 0 0 -1 1 1 models/test_static.glb\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "negative scale rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "static_prop 0 1 0 0 0 0 inf 1 1 models/test_static.glb\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "Inf scale rejected");

            world::LevelDefinition saveAuthored = parsed.level;
            world::StaticPropSpec authoredProp{};
            authoredProp.modelIdentity = "models/test_static.glb";
            authoredProp.position = {4.0f, 2.0f, 1.0f};
            authoredProp.rotationDegrees = {15.0f, 30.0f, 45.0f};
            authoredProp.scale = {0.5f, 2.0f, 1.5f};
            saveAuthored.staticProps.push_back(authoredProp);
            const std::string saved = world::SerializeLevelText(saveAuthored);
            Expect(saved.find("static_prop 4 ") != std::string::npos, "Save writes authored position");
            Expect(saved.find("models/test_static.glb") != std::string::npos, "Save writes identity");
            Expect(
                saved.find("15") != std::string::npos && saved.find("0.5") != std::string::npos,
                "Save writes rotation and scale");
        }

        {
            world::LevelDefinition grouped = parsed.level;
            world::StaticPropSpec propA{};
            propA.modelIdentity = "models/test_static.glb";
            propA.position = {1.0f, 1.0f, 0.0f};
            propA.scale = {1.0f, 1.0f, 1.0f};
            world::StaticPropSpec propB = propA;
            propB.position.x = 3.0f;
            grouped.staticProps.push_back(propA);
            grouped.staticProps.push_back(propB);
            world::StaticPropSpec propC = propA;
            propC.position.x = 5.0f;
            world::StaticPropSpec propD = propA;
            propD.position.x = 7.0f;
            grouped.staticProps.push_back(propC);
            grouped.staticProps.push_back(propD);
            world::AuthoringGroup group{};
            group.name = "Group_01";
            group.members.push_back({world::AuthoringGroupMemberKind::StaticProp, 0});
            group.members.push_back({world::AuthoringGroupMemberKind::StaticProp, 1});
            grouped.authoringGroups.push_back(group);
            grouped.pointLights.push_back(world::MakeDefaultPointLight({0.0f, 3.0f, 0.0f}));
            grouped.spotLights.push_back(world::MakeDefaultSpotLight({2.0f, 5.0f, 0.0f}));
            world::AuthoringGroup lamp{};
            lamp.name = "Lamp_01";
            lamp.members.push_back({world::AuthoringGroupMemberKind::StaticProp, 2});
            lamp.members.push_back({world::AuthoringGroupMemberKind::SpotLight, 0});
            grouped.authoringGroups.push_back(lamp);
            world::AuthoringGroup wallLamp{};
            wallLamp.name = "Lamp_02";
            wallLamp.members.push_back({world::AuthoringGroupMemberKind::StaticProp, 3});
            wallLamp.members.push_back({world::AuthoringGroupMemberKind::PointLight, 0});
            grouped.authoringGroups.push_back(wallLamp);
            const std::string groupedText = world::SerializeLevelText(grouped);
            Expect(CountRecords(groupedText, "authoring_group") == 3, "writer three authoring_groups");
            Expect(groupedText.find("authoring_group Lamp_01 static_prop 2 spot_light 0")
                    != std::string::npos,
                "writer emits spot_light group member token");
            Expect(groupedText.find("authoring_group Lamp_02 static_prop 3 point_light 0")
                    != std::string::npos,
                "writer emits point_light group member token");
            Expect(OnlyAuthoredKeywords(groupedText), "authoring_group is an authored keyword");
            const world::ParseLevelFileResult groupedParsed = world::ParseLevelText(groupedText);
            Expect(groupedParsed.status == world::LoadLevelFileStatus::Loaded,
                "authoring_group round trip loads");
            Expect(world::AuthoredLevelDataEqual(grouped, groupedParsed.level),
                "authoring_group round trip equality including local lights");
            std::string invalidPointIndex = groupedText;
            const std::string goodLamp02 = "authoring_group Lamp_02 static_prop 3 point_light 0\n";
            const std::string badLamp02 = "authoring_group Lamp_02 static_prop 3 point_light 9\n";
            const std::size_t lamp02 = invalidPointIndex.find(goodLamp02);
            Expect(lamp02 != std::string::npos, "Lamp_02 group line exists");
            if (lamp02 != std::string::npos)
            {
                invalidPointIndex.replace(lamp02, goodLamp02.size(), badLamp02);
            }
            Expect(
                world::ParseLevelText(invalidPointIndex).status == world::LoadLevelFileStatus::Invalid,
                "out-of-range point_light group index rejected");
            Expect(
                world::ParseLevelText(
                    canonical + "authoring_group Group_01 static_prop 0\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "one-member authoring_group rejected");
            Expect(
                world::ParseLevelText(
                    canonical
                    + "static_prop 1 1 0 0 0 0 1 1 1 models/test_static.glb\n"
                      "static_prop 2 1 0 0 0 0 1 1 1 models/test_static.glb\n"
                      "authoring_group Group_01 static_prop 0 static_prop 0\n")
                    .status
                    == world::LoadLevelFileStatus::Invalid,
                "duplicate membership in one group rejected");
        }

        std::error_code cleanupError;
        const std::filesystem::path reloadDir =
            std::filesystem::temp_directory_path() / "platformer3d_m41_reload_counts";
        std::filesystem::remove_all(reloadDir, cleanupError);
        std::filesystem::create_directories(reloadDir / "assets" / "levels", cleanupError);
        const std::filesystem::path stagedPath = reloadDir / "assets" / "levels" / "level_01.level";
        WriteAll(stagedPath, world::SerializeLevelText(extraPlatform));
        const editor::RuntimeLevelReloadPrepareResult readyCounts =
            editor::PrepareRuntimeLevelReload(stagedPath, false);
        Expect(readyCounts.status == editor::RuntimeLevelReloadStatus::Ready, "M39 ready variable count");
        Expect(readyCounts.candidate.elevatedPlatforms.size() == 7, "M39 candidate platform count");
        const editor::RuntimeLevelReloadReconcileResult reconciledCounts =
            editor::ReconcileAfterRuntimeLevelReload(readyCounts.candidate, parsed.level);
        Expect(reconciledCounts.selection.kind == editor::EditorObjectKind::None, "M39 still clears selection");
        Expect(reconciledCounts.workingCopy.elevatedPlatforms.size() == 7, "M39 working matches active");
        std::filesystem::remove_all(reloadDir, cleanupError);
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
