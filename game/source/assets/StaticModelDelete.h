#pragma once

// Path-safe deletion of a registered static model. Not ImGui. Not a recycle
// bin. Uses canonical identity plus explicit source/cooked/staged roots.

#include "assets/StaticGlb.h"
#include "assets/StaticModelCatalog.h"

#include <filesystem>
#include <string>
#include <vector>

namespace assets
{
enum class StaticModelDeleteStatus
{
    Deleted,
    NoSelection,
    InvalidIdentity,
    UnsafePath,
    MissingSource,
    Error,
};

const char* StaticModelDeleteStatusName(StaticModelDeleteStatus status);

struct StaticModelDeleteRoots
{
    std::filesystem::path sourceRoot;
    std::filesystem::path cookedRoot;
    std::vector<std::filesystem::path> stagedRoots;
};

struct StaticModelDeleteResult
{
    StaticModelDeleteStatus status = StaticModelDeleteStatus::Error;
    std::string canonicalIdentity;
    std::string message;
    bool sourceRemoved = false;
    bool cookedRemoved = false;
    int stagedRemovedCount = 0;
    bool cookedWasMissing = false;
    int stagedMissingCount = 0;
};

bool StaticModelDeleteSucceeded(StaticModelDeleteStatus status);

// Resolves source/cooked/staged files for a canonical identity. Does not
// delete. Rejects traversal and out-of-root mapping.
bool TryResolveStaticModelDeletePaths(
    std::string_view canonicalIdentity,
    const StaticModelDeleteRoots& roots,
    std::filesystem::path& sourcePath,
    std::filesystem::path& cookedPath,
    std::vector<std::filesystem::path>& stagedPaths,
    std::string& error);

// Generated counterparts are removed first. Source is removed last so a
// failed generated cleanup leaves catalog authority intact and is retry-safe.
// Missing cooked/staged files are not errors. Cancellation is a UI concern:
// this function is not called.
StaticModelDeleteResult DeleteStaticModel(
    std::string_view canonicalIdentity,
    const StaticModelDeleteRoots& roots,
    StaticModelCatalog* catalog = nullptr);
}
