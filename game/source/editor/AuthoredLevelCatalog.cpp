#include "editor/AuthoredLevelCatalog.h"

#include "world/LevelWriter.h"

#include <algorithm>
#include <system_error>
#include <utility>

namespace editor
{
namespace
{
constexpr std::string_view kLevelsDirectoryName = "levels";

bool PathIsRegularFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(path, error) && !error;
}

bool PathIsDirectory(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_directory(path, error) && !error;
}

bool PathExists(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
}

bool SourceRootIsUsable(const std::filesystem::path& sourceRoot)
{
    return !sourceRoot.empty() && sourceRoot.is_absolute() && PathIsDirectory(sourceRoot);
}

bool FileNameIsExactLevelExtension(const std::filesystem::path& path)
{
    return path.extension() == world::kLevelRuntimeExtension;
}

AuthoredLevelCreateResult MakeCreateResult(
    AuthoredLevelCreateStatus status,
    std::string_view message,
    std::string_view levelId = {},
    const std::filesystem::path& path = {})
{
    AuthoredLevelCreateResult result{};
    result.status = status;
    result.message = std::string(message);
    result.levelId = std::string(levelId);
    result.path = path;
    return result;
}
}

void AuthoredLevelCatalog::Clear()
{
    entries.clear();
}

void AuthoredLevelCatalog::Refresh(const std::filesystem::path& sourceRoot)
{
    entries.clear();
    if (!SourceRootIsUsable(sourceRoot))
    {
        return;
    }

    const std::filesystem::path levelsRoot =
        (sourceRoot / std::string(kLevelsDirectoryName)).lexically_normal();
    if (!PathIsDirectory(levelsRoot))
    {
        return;
    }

    std::error_code iteratorError;
    const std::filesystem::directory_iterator end{};
    for (std::filesystem::directory_iterator it(levelsRoot, iteratorError);
         !iteratorError && it != end;
         it.increment(iteratorError))
    {
        if (iteratorError)
        {
            break;
        }
        const std::filesystem::path& path = it->path();
        if (!PathIsRegularFile(path) || !FileNameIsExactLevelExtension(path))
        {
            continue;
        }
        const std::string stem = path.stem().string();
        if (!world::IsValidLevelIdToken(stem) || !world::RuntimeLevelPathStemMatchesId(path, stem))
        {
            continue;
        }
        const world::ParseLevelFileResult loaded = world::LoadLevelFile(path);
        if (loaded.status != world::LoadLevelFileStatus::Loaded)
        {
            continue;
        }
        if (loaded.level.id != stem || !world::IsWritableLevelDefinition(loaded.level))
        {
            continue;
        }

        AuthoredLevelEntry entry{};
        entry.id = loaded.level.id;
        entries.push_back(std::move(entry));
    }

    std::sort(
        entries.begin(),
        entries.end(),
        [](const AuthoredLevelEntry& left, const AuthoredLevelEntry& right) {
            return left.id < right.id;
        });
    entries.erase(
        std::unique(
            entries.begin(),
            entries.end(),
            [](const AuthoredLevelEntry& left, const AuthoredLevelEntry& right) {
                return left.id == right.id;
            }),
        entries.end());
}

const std::vector<AuthoredLevelEntry>& AuthoredLevelCatalog::Entries() const
{
    return entries;
}

const AuthoredLevelEntry* AuthoredLevelCatalog::Find(std::string_view levelId) const
{
    for (const AuthoredLevelEntry& entry : entries)
    {
        if (entry.id == levelId)
        {
            return &entry;
        }
    }
    return nullptr;
}

bool AuthoredLevelCatalog::Contains(std::string_view levelId) const
{
    return Find(levelId) != nullptr;
}

std::size_t AuthoredLevelCatalog::Count() const
{
    return entries.size();
}

std::filesystem::path MakeAuthoredLevelSourcePath(
    const std::filesystem::path& sourceRoot,
    std::string_view levelId)
{
    if (!sourceRoot.is_absolute() || sourceRoot.empty())
    {
        return {};
    }
    const std::string logical = world::MakeRuntimeLevelLogicalId(levelId);
    if (logical.empty())
    {
        return {};
    }

    std::filesystem::path relative =
        std::filesystem::path{logical}.lexically_normal();
    if (relative.empty() || relative.is_absolute() || relative.has_root_name())
    {
        return {};
    }
    for (const std::filesystem::path& part : relative)
    {
        if (part == "..")
        {
            return {};
        }
    }

    std::filesystem::path resolved = (sourceRoot / relative).lexically_normal();
    if (!resolved.is_absolute() || !world::RuntimeLevelPathStemMatchesId(resolved, levelId))
    {
        return {};
    }
    return resolved;
}

world::LevelDefinition MakeMinimalPlayableLevel(std::string_view levelId)
{
    world::LevelDefinition level{};
    if (!world::IsValidLevelIdToken(levelId))
    {
        return level;
    }

    level.id = std::string(levelId);
    level.initialSpawnVisualCenter = {0.0f, 2.3f, 0.0f};
    level.killPlaneY = -8.0f;
    level.ground = {{0.0f, 0.15f, 0.0f}, {12.0f, 0.7f, 5.0f}};
    level.elevatedPlatforms.push_back({{0.0f, 1.5f, 0.0f}, {4.0f, 0.5f, 3.0f}});
    level.checkpoint1PlatformIndex = 0;
    level.checkpoint2PlatformIndex = 0;
    level.goalPlatformIndex = 0;
    level.slopes[0] = {{3.5f, 2.15f, 0.0f}, {4.0f, 0.4f, 3.0f}, 30.0f};
    level.slopes[1] = {{6.5f, 1.45f, 0.0f}, {2.0f, 0.4f, 2.0f}, 60.0f};
    level.movingPlatform.size = {2.5f, 0.4f, 2.0f};
    level.movingPlatform.centerY = 1.2f;
    level.movingPlatform.centerZ = 0.0f;
    level.movingPlatform.pathMinX = 4.0f;
    level.movingPlatform.pathMaxX = 8.0f;
    level.movingPlatform.speed = 1.5f;
    level.movingPlatform.startX = 4.0f;
    level.camera.offset = {0.0f, 4.0f, 12.0f};
    level.camera.fieldOfViewY = 55.0f;
    return level;
}

const char* AuthoredLevelCreateStatusName(AuthoredLevelCreateStatus status)
{
    switch (status)
    {
    case AuthoredLevelCreateStatus::Created:
        return "Created";
    case AuthoredLevelCreateStatus::InvalidId:
        return "InvalidId";
    case AuthoredLevelCreateStatus::Duplicate:
        return "Duplicate";
    case AuthoredLevelCreateStatus::Exists:
        return "Exists";
    case AuthoredLevelCreateStatus::Unsafe:
        return "Unsafe";
    case AuthoredLevelCreateStatus::Invalid:
        return "Invalid";
    case AuthoredLevelCreateStatus::Error:
        return "Error";
    }
    return "Error";
}

AuthoredLevelCreateResult TryCreateAuthoredLevel(
    const std::filesystem::path& sourceRoot,
    std::string_view levelId,
    const AuthoredLevelCatalog& catalog)
{
    if (!world::IsValidLevelIdToken(levelId))
    {
        return MakeCreateResult(
            AuthoredLevelCreateStatus::InvalidId,
            "Level ID is unsafe. Use a logical identity such as level_03.");
    }
    if (catalog.Contains(levelId))
    {
        return MakeCreateResult(
            AuthoredLevelCreateStatus::Duplicate,
            "A Level with that ID already exists. Existing files were not changed.",
            levelId);
    }

    const std::filesystem::path path = MakeAuthoredLevelSourcePath(sourceRoot, levelId);
    if (path.empty())
    {
        return MakeCreateResult(
            AuthoredLevelCreateStatus::Unsafe,
            "Authoring path is unsafe. Existing files were not changed.",
            levelId);
    }
    if (PathExists(path))
    {
        return MakeCreateResult(
            AuthoredLevelCreateStatus::Exists,
            "A Level file with that ID already exists. Existing files were not overwritten.",
            levelId,
            path);
    }

    std::error_code createError;
    std::filesystem::create_directories(path.parent_path(), createError);
    if (createError)
    {
        return MakeCreateResult(
            AuthoredLevelCreateStatus::Error,
            "Could not create the authored Levels directory. Existing files were not changed.",
            levelId,
            path);
    }

    const world::LevelDefinition level = MakeMinimalPlayableLevel(levelId);
    if (!world::IsWritableLevelDefinition(level))
    {
        return MakeCreateResult(
            AuthoredLevelCreateStatus::Invalid,
            "New Level template failed validation. Existing files were not changed.",
            levelId,
            path);
    }

    const world::WriteLevelFileResult written = world::SaveLevelFile(path, level);
    if (written.status != world::WriteLevelFileStatus::Saved)
    {
        return MakeCreateResult(
            AuthoredLevelCreateStatus::Error,
            written.error.empty()
                ? "Failed to create Level file. Existing files were not changed."
                : written.error,
            levelId,
            path);
    }

    return MakeCreateResult(
        AuthoredLevelCreateStatus::Created,
        "Created authored Level source.",
        levelId,
        path);
}

const char* AuthoredLevelOpenStatusName(AuthoredLevelOpenStatus status)
{
    switch (status)
    {
    case AuthoredLevelOpenStatus::Ready:
        return "Ready";
    case AuthoredLevelOpenStatus::Missing:
        return "Missing";
    case AuthoredLevelOpenStatus::Invalid:
        return "Invalid";
    case AuthoredLevelOpenStatus::Unsafe:
        return "Unsafe";
    case AuthoredLevelOpenStatus::Error:
        return "Error";
    }
    return "Error";
}

AuthoredLevelOpenPrepareResult PrepareAuthoredLevelOpen(
    const std::filesystem::path& sourceAbsolutePath,
    std::string_view expectedLevelId)
{
    AuthoredLevelOpenPrepareResult result{};
    if (!world::IsValidLevelIdToken(expectedLevelId))
    {
        result.status = AuthoredLevelOpenStatus::Unsafe;
        result.message = "Level identity is unsafe. Current Level unchanged.";
        return result;
    }
    if (sourceAbsolutePath.empty() || !sourceAbsolutePath.is_absolute())
    {
        result.status = AuthoredLevelOpenStatus::Unsafe;
        result.message = "Authored Level path is unsafe. Current Level unchanged.";
        return result;
    }
    for (const std::filesystem::path& part : sourceAbsolutePath)
    {
        if (part == "..")
        {
            result.status = AuthoredLevelOpenStatus::Unsafe;
            result.message = "Authored Level path is unsafe. Current Level unchanged.";
            return result;
        }
    }
    if (!world::RuntimeLevelPathStemMatchesId(sourceAbsolutePath, expectedLevelId))
    {
        result.status = AuthoredLevelOpenStatus::Unsafe;
        result.message = "Authored Level path does not match identity. Current Level unchanged.";
        return result;
    }

    const world::ParseLevelFileResult loaded = world::LoadLevelFile(sourceAbsolutePath);
    result.loadStatus = loaded.status;
    if (loaded.status == world::LoadLevelFileStatus::Missing)
    {
        result.status = AuthoredLevelOpenStatus::Missing;
        result.message = "Authored Level is missing. Current Level unchanged.";
        return result;
    }
    if (loaded.status == world::LoadLevelFileStatus::UnsupportedVersion)
    {
        result.status = AuthoredLevelOpenStatus::Invalid;
        result.message = "Authored Level version is unsupported. Current Level unchanged.";
        return result;
    }
    if (loaded.status != world::LoadLevelFileStatus::Loaded)
    {
        result.status = loaded.status == world::LoadLevelFileStatus::Invalid
            ? AuthoredLevelOpenStatus::Invalid
            : AuthoredLevelOpenStatus::Error;
        result.message = loaded.error.empty()
            ? std::string("Authored Level is invalid. Current Level unchanged.")
            : loaded.error + ". Current Level unchanged.";
        return result;
    }
    if (loaded.level.id != expectedLevelId || !world::IsWritableLevelDefinition(loaded.level))
    {
        result.status = AuthoredLevelOpenStatus::Invalid;
        result.message = "Authored Level failed validation. Current Level unchanged.";
        return result;
    }

    result.status = AuthoredLevelOpenStatus::Ready;
    result.candidate = loaded.level;
    result.message = "Authored Level candidate is ready.";
    return result;
}

std::vector<NextLevelSelectorEntry> MakeNextLevelSelectorEntries(
    const AuthoredLevelCatalog& catalog,
    std::string_view currentNextLevelId)
{
    std::vector<NextLevelSelectorEntry> entries;
    NextLevelSelectorEntry none{};
    entries.push_back(none);

    for (const AuthoredLevelEntry& discovered : catalog.Entries())
    {
        NextLevelSelectorEntry entry{};
        entry.id = discovered.id;
        entries.push_back(entry);
    }

    if (!currentNextLevelId.empty())
    {
        bool found = false;
        for (const NextLevelSelectorEntry& entry : entries)
        {
            if (entry.id == currentNextLevelId)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            NextLevelSelectorEntry missing{};
            missing.id = std::string(currentNextLevelId);
            missing.missing = true;
            entries.push_back(std::move(missing));
        }
    }
    return entries;
}

bool ApplyNextLevelSelectorId(world::LevelGoalSpec& goal, std::string_view chosenId)
{
    if (!world::IsValidNextLevelId(chosenId))
    {
        return false;
    }
    goal.nextLevelId = std::string(chosenId);
    return true;
}
}
