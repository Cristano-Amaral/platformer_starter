#pragma once

// Authored gameplay-definition catalog (Milestone 98).
// One textual file. Not Level Format, not JSON, and not a cooked runtime asset.
// Source location: game/assets/source/gameplay/definitions.gameplay
// Development inspection resolves that path through the authoring root.
// Gameplay simulation does not load this file.

#include "gameplay/GameplayDefinition.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace gameplay
{
inline constexpr std::string_view kGameplayDefinitionsMagic = "PLATFORMER_GAMEPLAY_DEFINITIONS";
inline constexpr std::string_view kGameplayDefinitionsLogicalPath = "gameplay/definitions.gameplay";

inline constexpr std::uintmax_t kMaxGameplayDefinitionsFileBytes = 262144;
inline constexpr std::size_t kMaxGameplayDefinitionsLineLength = 256;
inline constexpr std::size_t kMaxGameplayDefinitionsLines = 4096;
inline constexpr std::size_t kMaxGameplayDefinitions = 128;
inline constexpr std::string_view kGameplayDefinitionsTemporarySuffix = ".tmp";

enum class LoadGameplayDefinitionsStatus
{
    Loaded,
    Missing,
    Invalid,
    Error,
};

struct ParseGameplayDefinitionsResult
{
    LoadGameplayDefinitionsStatus status = LoadGameplayDefinitionsStatus::Invalid;
    int errorLine = 0;
    std::string error;
    GameplayDefinitionRegistry registry;
};

struct WriteGameplayDefinitionsResult
{
    bool ok = false;
    std::string text;
    std::string error;
};

struct SaveGameplayDefinitionsResult
{
    bool ok = false;
    std::string error;
};

inline const char* LoadGameplayDefinitionsStatusName(LoadGameplayDefinitionsStatus status)
{
    switch (status)
    {
    case LoadGameplayDefinitionsStatus::Loaded:
        return "Loaded";
    case LoadGameplayDefinitionsStatus::Missing:
        return "Missing";
    case LoadGameplayDefinitionsStatus::Invalid:
        return "Invalid";
    case LoadGameplayDefinitionsStatus::Error:
        return "Error";
    }
    return "Error";
}

// Strict in-memory parse. Empty input is Invalid, not Missing.
// On failure `registry` is empty.
ParseGameplayDefinitionsResult ParseGameplayDefinitionsText(std::string_view text);

// Caller supplies the path. This does not consult the process working directory.
ParseGameplayDefinitionsResult LoadGameplayDefinitionsFile(const std::filesystem::path& path);

// Emits insertion order. Item fields then modifiers in authored order, then
// stats in enum order. Uses std::to_chars so a later parse recovers the same float.
// world_model and icon identities are always quoted so catalog paths that contain
// spaces round-trip. The reader also accepts a legacy unquoted remainder.
WriteGameplayDefinitionsResult WriteGameplayDefinitionsText(
    const GameplayDefinitionRegistry& registry);

// Validate, serialize, write a sibling temp, flush/close, then promote onto
// path. The path must be absolute. Invalid catalogs leave an existing file
// untouched.
SaveGameplayDefinitionsResult SaveGameplayDefinitionsFile(
    const std::filesystem::path& path,
    const GameplayDefinitionRegistry& registry);

std::filesystem::path GameplayDefinitionsTemporaryPath(const std::filesystem::path& path);
}
