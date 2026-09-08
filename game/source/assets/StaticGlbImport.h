#pragma once

// Copies a compatible external static GLB into canonical source/models/.
// Does not cook, stage, or mutate LevelDefinition / editor authored state.

#include "assets/StaticGlb.h"
#include "assets/StaticModelCatalog.h"

#include <filesystem>
#include <string>

namespace assets
{
enum class StaticGlbImportStatus
{
    Imported,
    Cancelled,
    Missing,
    NotAFile,
    UnsupportedExtension,
    UnsafeName,
    UnsafeDestination,
    Collision,
    InvalidContainer,
    Incompatible,
    Error,
};

const char* StaticGlbImportStatusName(StaticGlbImportStatus status);

struct StaticGlbImportResult
{
    StaticGlbImportStatus status = StaticGlbImportStatus::Error;
    std::string canonicalIdentity;
    std::filesystem::path destinationPath;
    std::string message;
};

bool StaticGlbImportSucceeded(StaticGlbImportStatus status);

// Resolves sourceRoot/models/<fileName> without copying. Rejects traversal and
// out-of-root destinations. Used by import and by destination-safety tests.
bool TryResolveStaticGlbImportDestination(
    const std::filesystem::path& sourceRoot,
    std::string_view fileName,
    std::string& canonicalIdentity,
    std::filesystem::path& destination,
    std::string& error);

// sourceRoot is the canonical authored root (game/assets/source). Destination
// is always sourceRoot/models/<filename>.glb. originalExternalPath is input
// only and is never stored as identity.
StaticGlbImportResult ImportStaticGlb(
    const std::filesystem::path& originalExternalPath,
    const std::filesystem::path& sourceRoot,
    StaticModelCatalog* catalog = nullptr);
}
