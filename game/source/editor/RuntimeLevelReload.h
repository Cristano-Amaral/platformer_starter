#pragma once

// Milestone 39: in-process staged Level Format v1 reload. Not a tool job.
// Parse/validate a candidate from an absolute staged runtime path. Callers
// commit Application/physics state only after Ready. Does not cook, stage,
// write files, or invoke HostProcess.

#include "editor/EditorSelection.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"

#include <filesystem>
#include <string>

namespace editor
{
enum class RuntimeLevelReloadStatus
{
    Ready,
    RejectedModified,
    Missing,
    Invalid,
    UnsupportedVersion,
    Error,
};

struct RuntimeLevelReloadPrepareResult
{
    RuntimeLevelReloadStatus status = RuntimeLevelReloadStatus::Error;
    std::string message;
    world::LevelDefinition candidate{};
    world::LoadLevelFileStatus loadStatus = world::LoadLevelFileStatus::Error;
};

struct RuntimeLevelReloadReconcileResult
{
    world::LevelDefinition workingCopy{};
    bool modified = false;
    bool dirty = false;
    EditorSelection selection{};
};

const char* RuntimeLevelReloadStatusName(RuntimeLevelReloadStatus status);

// stagedRuntimeLevelPath must be absolute. Never uses process CWD, source, or
// cooked trees. Does not write any file. If modified, does not read the file.
RuntimeLevelReloadPrepareResult PrepareRuntimeLevelReload(
    const std::filesystem::path& stagedRuntimeLevelPath,
    bool modified);

// After a successful commit of reloadedActive: workingCopy matches active,
// Modified is false, Dirty follows savedSourceBaseline, selection is cleared.
RuntimeLevelReloadReconcileResult ReconcileAfterRuntimeLevelReload(
    const world::LevelDefinition& reloadedActive,
    const world::LevelDefinition& savedSourceBaseline);
}
