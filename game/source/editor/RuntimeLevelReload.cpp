#include "editor/RuntimeLevelReload.h"

#include "world/LevelDefinition.h"
#include "world/LevelWriter.h"

namespace editor
{
const char* RuntimeLevelReloadStatusName(RuntimeLevelReloadStatus status)
{
    switch (status)
    {
    case RuntimeLevelReloadStatus::Ready:
        return "Ready";
    case RuntimeLevelReloadStatus::RejectedModified:
        return "RejectedModified";
    case RuntimeLevelReloadStatus::Missing:
        return "Missing";
    case RuntimeLevelReloadStatus::Invalid:
        return "Invalid";
    case RuntimeLevelReloadStatus::UnsupportedVersion:
        return "UnsupportedVersion";
    case RuntimeLevelReloadStatus::Error:
        return "Error";
    }
    return "Error";
}

RuntimeLevelReloadPrepareResult PrepareRuntimeLevelReload(
    const std::filesystem::path& stagedRuntimeLevelPath,
    bool modified)
{
    RuntimeLevelReloadPrepareResult result{};
    if (modified)
    {
        result.status = RuntimeLevelReloadStatus::RejectedModified;
        result.message =
            "Reload rejected: apply or revert unapplied working-copy edits first. "
            "Active level unchanged.";
        return result;
    }

    if (stagedRuntimeLevelPath.empty() || !stagedRuntimeLevelPath.is_absolute())
    {
        result.status = RuntimeLevelReloadStatus::Error;
        result.message =
            "Reload requires an absolute staged runtime level path. "
            "Active level unchanged.";
        return result;
    }

    const world::ParseLevelFileResult loaded = world::LoadLevelFile(stagedRuntimeLevelPath);
    result.loadStatus = loaded.status;
    if (loaded.status == world::LoadLevelFileStatus::Missing)
    {
        result.status = RuntimeLevelReloadStatus::Missing;
        result.message = "Staged runtime level is missing. Active level unchanged.";
        return result;
    }
    if (loaded.status == world::LoadLevelFileStatus::UnsupportedVersion)
    {
        result.status = RuntimeLevelReloadStatus::UnsupportedVersion;
        result.message = "Staged runtime level version is unsupported. Active level unchanged.";
        return result;
    }
    if (loaded.status != world::LoadLevelFileStatus::Loaded)
    {
        result.status = loaded.status == world::LoadLevelFileStatus::Invalid
            ? RuntimeLevelReloadStatus::Invalid
            : RuntimeLevelReloadStatus::Error;
        result.message = loaded.error.empty()
            ? std::string("Staged runtime level is invalid. Active level unchanged.")
            : loaded.error + ". Active level unchanged.";
        return result;
    }
    if (loaded.level.id != world::kLevel01Id
        || !world::IsWritableLevelDefinition(loaded.level))
    {
        result.status = RuntimeLevelReloadStatus::Invalid;
        result.message =
            "Staged runtime level failed validation. Active level unchanged.";
        return result;
    }

    result.status = RuntimeLevelReloadStatus::Ready;
    result.candidate = loaded.level;
    result.message = "Staged runtime level candidate is ready.";
    return result;
}

RuntimeLevelReloadReconcileResult ReconcileAfterRuntimeLevelReload(
    const world::LevelDefinition& reloadedActive,
    const world::LevelDefinition& savedSourceBaseline)
{
    RuntimeLevelReloadReconcileResult result{};
    result.workingCopy = reloadedActive;
    result.modified = !world::AuthoredLevelDataEqual(result.workingCopy, reloadedActive);
    result.dirty = !world::AuthoredLevelDataEqual(reloadedActive, savedSourceBaseline);
    result.selection = ClearSelection();
    return result;
}
}
