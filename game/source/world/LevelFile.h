#pragma once

// PLATFORMER_LEVEL v1 parser/loader. Syntax and semantic validation only.
// Does not activate gameplay, create physics bodies, render, or touch BEST.

#include "world/LevelDefinition.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace world
{
inline constexpr std::string_view kLevelFileMagic = "PLATFORMER_LEVEL";
inline constexpr int kLevelFileVersion = 1;
inline constexpr std::string_view kLevel01RuntimeLogicalId = "levels/level_01.level";

enum class LoadLevelFileStatus
{
    Loaded,
    Missing,
    Invalid,
    UnsupportedVersion,
    Error,
};

struct ParseLevelFileResult
{
    LoadLevelFileStatus status = LoadLevelFileStatus::Invalid;
    int formatVersion = 0;
    int errorLine = 0;
    std::string error;
    LevelDefinition level;
};

inline const char* LoadLevelFileStatusName(LoadLevelFileStatus status)
{
    switch (status)
    {
    case LoadLevelFileStatus::Loaded:
        return "Loaded";
    case LoadLevelFileStatus::Missing:
        return "Missing";
    case LoadLevelFileStatus::Invalid:
        return "Invalid";
    case LoadLevelFileStatus::UnsupportedVersion:
        return "UnsupportedVersion";
    case LoadLevelFileStatus::Error:
        return "Error";
    }
    return "Error";
}

// Strict in-memory parse. Empty input is Invalid, not Missing.
ParseLevelFileResult ParseLevelText(std::string_view text);

// Filesystem load then ParseLevelText. Missing file -> Missing. I/O errors -> Error.
// Does not use the process CWD; the caller supplies an absolute runtime path.
ParseLevelFileResult LoadLevelFile(const std::filesystem::path& path);
}
