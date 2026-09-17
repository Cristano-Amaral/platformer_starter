#include "editor/AuthoredLevelCatalog.h"
#include "world/LevelFile.h"
#include "world/LevelWriter.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
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

void WriteAll(const std::filesystem::path& path, std::string_view text)
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
}

std::string ReadAll(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string(
        (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

bool FileExists(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(path, error) && !error;
}
}

int main()
{
    Expect(editor::AuthoredLevelSwitchNeedsDiscard(true, false), "modified needs discard");
    Expect(editor::AuthoredLevelSwitchNeedsDiscard(false, true), "dirty needs discard");
    Expect(editor::AuthoredLevelSwitchNeedsDiscard(true, true), "modified and dirty need discard");
    Expect(!editor::AuthoredLevelSwitchNeedsDiscard(false, false), "clean switch needs no discard");

    const world::ParseLevelFileResult level01 = world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
    const world::ParseLevelFileResult level02 = world::LoadLevelFile(PLATFORMER_LEVEL02_SOURCE_PATH);
    Expect(level01.status == world::LoadLevelFileStatus::Loaded, "canonical level_01 loads");
    Expect(level02.status == world::LoadLevelFileStatus::Loaded, "canonical level_02 loads");

    std::error_code cleanupError;
    const std::filesystem::path scratch =
        std::filesystem::temp_directory_path() / "platformer3d_m64_1_levels";
    std::filesystem::remove_all(scratch, cleanupError);
    const std::filesystem::path sourceRoot = scratch / "source";
    const std::filesystem::path levelsDir = sourceRoot / "levels";
    WriteAll(levelsDir / "level_02.level", world::SerializeLevelText(level02.level));
    WriteAll(levelsDir / "level_01.level", world::SerializeLevelText(level01.level));
    WriteAll(levelsDir / "notes.txt", "not a level\n");
    WriteAll(levelsDir / "bad-name.level", "PLATFORMER_LEVEL 1\nid bad-name\n");
    WriteAll(levelsDir / "unsafe.level", "PLATFORMER_LEVEL 1\nid ../secret\n");
    WriteAll(levelsDir / "malformed.level", "NOT_A_LEVEL\n");
    WriteAll(levelsDir / "id_mismatch.level", world::SerializeLevelText(level02.level));

    editor::AuthoredLevelCatalog catalog;
    catalog.Refresh(sourceRoot);
    Expect(catalog.Count() == 2, "discovery finds two valid levels");
    Expect(catalog.Contains("level_01"), "discovery includes level_01");
    Expect(catalog.Contains("level_02"), "discovery includes level_02");
    Expect(!catalog.Contains("malformed"), "malformed files are not offered");
    Expect(!catalog.Contains("bad-name"), "unsafe file names are not offered");
    Expect(!catalog.Contains("notes"), "non-level files are not offered");
    Expect(catalog.Entries().size() == 2, "no duplicate entries");
    Expect(catalog.Entries()[0].id == "level_01", "deterministic order first");
    Expect(catalog.Entries()[1].id == "level_02", "deterministic order second");

    catalog.Refresh(std::filesystem::path("relative"));
    Expect(catalog.Count() == 0, "relative source root yields empty catalog");
    catalog.Refresh(sourceRoot);
    Expect(catalog.Count() == 2, "refresh restores discovered levels");

    world::LevelGoalSpec goal{};
    const std::vector<editor::NextLevelSelectorEntry> terminalChoices =
        editor::MakeNextLevelSelectorEntries(catalog, "");
    Expect(terminalChoices.size() == 3, "None plus two discovered destinations");
    Expect(terminalChoices[0].id.empty() && !terminalChoices[0].missing, "None is first");
    Expect(terminalChoices[1].id == "level_01", "selector reuses discovery order");
    Expect(editor::ApplyNextLevelSelectorId(goal, ""), "None writes terminal empty id");
    Expect(goal.nextLevelId.empty(), "None is terminal");
    Expect(editor::ApplyNextLevelSelectorId(goal, "level_02"), "selection writes logical id");
    Expect(goal.nextLevelId == "level_02", "nextLevelId stores discovered identity");
    Expect(!editor::ApplyNextLevelSelectorId(goal, "../secret"), "unsafe selector id is rejected");
    Expect(goal.nextLevelId == "level_02", "rejected selector leaves authored value");

    goal.nextLevelId = "level_99";
    const std::vector<editor::NextLevelSelectorEntry> missingChoices =
        editor::MakeNextLevelSelectorEntries(catalog, goal.nextLevelId);
    bool missingPreserved = false;
    for (const editor::NextLevelSelectorEntry& entry : missingChoices)
    {
        if (entry.id == "level_99" && entry.missing)
        {
            missingPreserved = true;
        }
    }
    Expect(missingPreserved, "missing destination is preserved and flagged");
    Expect(goal.nextLevelId == "level_99", "missing destination is not erased");

    const world::LevelDefinition createdTemplate = editor::MakeMinimalPlayableLevel("level_03");
    Expect(createdTemplate.id == "level_03", "new level uses requested id");
    Expect(world::IsWritableLevelDefinition(createdTemplate), "new level is playable/writable");
    Expect(createdTemplate.levelGoals.empty(), "new level has no Level Goal fixtures");
    Expect(editor::MakeMinimalPlayableLevel("../x").id.empty(), "unsafe template id is empty");

    const editor::AuthoredLevelCreateResult duplicate =
        editor::TryCreateAuthoredLevel(sourceRoot, "level_01", catalog);
    Expect(duplicate.status == editor::AuthoredLevelCreateStatus::Duplicate, "duplicate id rejected");
    const std::string level01Before = ReadAll(levelsDir / "level_01.level");
    Expect(ReadAll(levelsDir / "level_01.level") == level01Before, "duplicate does not overwrite");

    const editor::AuthoredLevelCreateResult unsafe =
        editor::TryCreateAuthoredLevel(sourceRoot, "level_03.level", catalog);
    Expect(unsafe.status == editor::AuthoredLevelCreateStatus::InvalidId, "extension id rejected");
    Expect(!FileExists(levelsDir / "level_03.level.level"), "unsafe create writes nothing");

    const editor::AuthoredLevelCreateResult traversal =
        editor::TryCreateAuthoredLevel(sourceRoot, "../secret", catalog);
    Expect(traversal.status == editor::AuthoredLevelCreateStatus::InvalidId, "traversal id rejected");

    const editor::AuthoredLevelCreateResult created =
        editor::TryCreateAuthoredLevel(sourceRoot, "level_03", catalog);
    Expect(created.status == editor::AuthoredLevelCreateStatus::Created, "safe new level succeeds");
    Expect(FileExists(created.path), "new level file exists");
    const std::string createdText = ReadAll(created.path);
    Expect(createdText.find("id level_03") != std::string::npos, "new file stores requested id");
    Expect(createdText.find("goal -21") == std::string::npos, "new file has no legacy goal");
    Expect(
        createdText.find("dynamic_box 0 5 0 1 1 1 30") == std::string::npos,
        "new file has no legacy dynamic_box");

    const std::string originalCreated = createdText;
    catalog.Refresh(sourceRoot);
    Expect(catalog.Contains("level_03"), "refresh exposes the new level");
    const editor::AuthoredLevelCreateResult exists =
        editor::TryCreateAuthoredLevel(sourceRoot, "level_03", catalog);
    Expect(
        exists.status == editor::AuthoredLevelCreateStatus::Duplicate
            || exists.status == editor::AuthoredLevelCreateStatus::Exists,
        "second create does not overwrite");
    Expect(ReadAll(created.path) == originalCreated, "existing new level is not overwritten");

    const editor::AuthoredLevelOpenPrepareResult opened =
        editor::PrepareAuthoredLevelOpen(created.path, "level_03");
    Expect(opened.status == editor::AuthoredLevelOpenStatus::Ready, "open prepare is Ready");
    Expect(opened.candidate.id == "level_03", "open candidate identity matches");
    Expect(
        world::AuthoredLevelDataEqual(opened.candidate, createdTemplate),
        "clean open candidate matches authored source");

    world::LevelDefinition working = opened.candidate;
    world::LevelDefinition active = opened.candidate;
    world::LevelDefinition baseline = opened.candidate;
    Expect(!editor::AuthoredLevelSwitchNeedsDiscard(
               !world::AuthoredLevelDataEqual(working, active),
               !world::AuthoredLevelDataEqual(active, baseline)),
        "clean open has no false Dirty/Modified");

    working.killPlaneY = -3.0f;
    Expect(
        editor::AuthoredLevelSwitchNeedsDiscard(
            !world::AuthoredLevelDataEqual(working, active),
            !world::AuthoredLevelDataEqual(active, baseline)),
        "pending workingCopy edit requires discard");
    const world::LevelDefinition preservedWorking = working;
    Expect(preservedWorking.killPlaneY == -3.0f, "cancel leaves workingCopy edits intact");

    const editor::AuthoredLevelOpenPrepareResult missing = editor::PrepareAuthoredLevelOpen(
        levelsDir / "missing_level.level", "missing_level");
    Expect(missing.status == editor::AuthoredLevelOpenStatus::Missing, "missing open is Missing");
    Expect(missing.candidate.id.empty(), "failed open has no candidate");

    const editor::AuthoredLevelOpenPrepareResult relative = editor::PrepareAuthoredLevelOpen(
        std::filesystem::path("levels") / "level_03.level", "level_03");
    Expect(relative.status == editor::AuthoredLevelOpenStatus::Unsafe, "relative open is unsafe");

    std::filesystem::remove_all(scratch, cleanupError);

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d AuthoredLevelCatalogTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("AuthoredLevelCatalogTest passed\n");
    return 0;
}
