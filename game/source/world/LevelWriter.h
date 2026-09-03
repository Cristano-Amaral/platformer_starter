#pragma once

// PLATFORMER_LEVEL v1 serializer. Inverse companion to LevelFile.h.
// Emits only authored LevelDefinition data; runtime state has no records.
// No Win32/raylib/Jolt types. Safe file promotion goes through
// platform::ReplaceFileWithTemporary, not a private Win32 call.

#include "world/LevelDefinition.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace world
{
enum class WriteLevelFileStatus
{
    Saved,
    Invalid,
    Error,
};

struct WriteLevelFileResult
{
    WriteLevelFileStatus status = WriteLevelFileStatus::Invalid;
    std::string error;
};

inline const char* WriteLevelFileStatusName(WriteLevelFileStatus status)
{
    switch (status)
    {
    case WriteLevelFileStatus::Saved:
        return "Saved";
    case WriteLevelFileStatus::Invalid:
        return "Invalid";
    case WriteLevelFileStatus::Error:
        return "Error";
    }
    return "Error";
}

// Semantic gate shared by SerializeLevelText and SaveLevelFile:
// LevelDefinitionHasRequiredAuthoredContent plus the `id` grammar rule.
bool IsWritableLevelDefinition(const LevelDefinition& level);

// Deterministic canonical v1 text. Identical input bytes for identical input
// data. Returns an empty string when IsWritableLevelDefinition is false, so an
// invalid definition can never reach a file.
std::string SerializeLevelText(const LevelDefinition& level);

// Validate, serialize, write a sibling temp, flush/close, then promote onto
// path. The path must be absolute: the writer never resolves against the
// process CWD. An invalid definition leaves an existing file untouched.
WriteLevelFileResult SaveLevelFile(
    const std::filesystem::path& path,
    const LevelDefinition& level);

// Sibling temp used by SaveLevelFile. Exposed so tests can assert no leftover.
std::filesystem::path LevelFileTemporaryPath(const std::filesystem::path& path);

inline constexpr std::string_view kLevelFileTemporarySuffix = ".tmp";
}
