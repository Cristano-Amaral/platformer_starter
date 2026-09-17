#include "gameplay/LevelTransition.h"

#include "world/LevelWriter.h"

namespace gameplay
{
LevelTransitionPrepareResult PrepareStagedLevelDestination(
    const std::filesystem::path& stagedAbsolutePath,
    std::string_view expectedLevelId)
{
    LevelTransitionPrepareResult result{};
    if (!world::IsValidLevelIdToken(expectedLevelId))
    {
        result.status = LevelTransitionPrepareStatus::Unsafe;
        result.message = "Destination identity is unsafe. Active level unchanged.";
        return result;
    }
    if (stagedAbsolutePath.empty() || !stagedAbsolutePath.is_absolute())
    {
        result.status = LevelTransitionPrepareStatus::Unsafe;
        result.message = "Destination path is unsafe. Active level unchanged.";
        return result;
    }
    for (const std::filesystem::path& part : stagedAbsolutePath)
    {
        if (part == "..")
        {
            result.status = LevelTransitionPrepareStatus::Unsafe;
            result.message = "Destination path is unsafe. Active level unchanged.";
            return result;
        }
    }
    if (!world::RuntimeLevelPathStemMatchesId(stagedAbsolutePath, expectedLevelId))
    {
        result.status = LevelTransitionPrepareStatus::Unsafe;
        result.message = "Destination path does not match identity. Active level unchanged.";
        return result;
    }

    const world::ParseLevelFileResult loaded = world::LoadLevelFile(stagedAbsolutePath);
    result.loadStatus = loaded.status;
    if (loaded.status == world::LoadLevelFileStatus::Missing)
    {
        result.status = LevelTransitionPrepareStatus::Missing;
        result.message = "Destination staged level is missing. Active level unchanged.";
        return result;
    }
    if (loaded.status == world::LoadLevelFileStatus::UnsupportedVersion)
    {
        result.status = LevelTransitionPrepareStatus::Invalid;
        result.message = "Destination staged level version is unsupported. Active level unchanged.";
        return result;
    }
    if (loaded.status != world::LoadLevelFileStatus::Loaded)
    {
        result.status = loaded.status == world::LoadLevelFileStatus::Invalid
            ? LevelTransitionPrepareStatus::Invalid
            : LevelTransitionPrepareStatus::Error;
        result.message = loaded.error.empty()
            ? std::string("Destination staged level is invalid. Active level unchanged.")
            : loaded.error + ". Active level unchanged.";
        return result;
    }
    if (loaded.level.id != expectedLevelId || !world::IsWritableLevelDefinition(loaded.level))
    {
        result.status = LevelTransitionPrepareStatus::Invalid;
        result.message = "Destination staged level failed validation. Active level unchanged.";
        return result;
    }

    result.status = LevelTransitionPrepareStatus::Ready;
    result.candidate = loaded.level;
    result.message = "Destination staged level candidate is ready.";
    return result;
}
}
