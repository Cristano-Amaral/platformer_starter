#pragma once

// Path-safe deletion of a registered standalone runtime PNG. Not ImGui.
// Uses canonical identity plus explicit source/cooked/staged roots.

#include "assets/RuntimePng.h"
#include "assets/SourceTextureCatalog.h"

#include <filesystem>
#include <string>
#include <vector>

namespace assets
{
enum class RuntimePngDeleteStatus
{
    Deleted,
    NoSelection,
    InvalidIdentity,
    UnsafePath,
    MissingSource,
    Error,
};

const char* RuntimePngDeleteStatusName(RuntimePngDeleteStatus status);

struct RuntimePngDeleteRoots
{
    std::filesystem::path sourceRoot;
    std::filesystem::path cookedRoot;
    std::vector<std::filesystem::path> stagedRoots;
};

struct RuntimePngDeleteResult
{
    RuntimePngDeleteStatus status = RuntimePngDeleteStatus::Error;
    std::string canonicalIdentity;
    std::string message;
    bool sourceRemoved = false;
    bool cookedRemoved = false;
    int stagedRemovedCount = 0;
    bool cookedWasMissing = false;
    int stagedMissingCount = 0;
};

bool RuntimePngDeleteSucceeded(RuntimePngDeleteStatus status);

bool TryResolveRuntimePngDeletePaths(
    std::string_view canonicalIdentity,
    const RuntimePngDeleteRoots& roots,
    std::filesystem::path& sourcePath,
    std::filesystem::path& cookedPath,
    std::vector<std::filesystem::path>& stagedPaths,
    std::string& error);

RuntimePngDeleteResult DeleteRuntimePng(
    std::string_view canonicalIdentity,
    const RuntimePngDeleteRoots& roots,
    SourceTextureCatalog* catalog = nullptr);
}
