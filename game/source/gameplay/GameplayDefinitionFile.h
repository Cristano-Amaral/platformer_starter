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

inline constexpr std::uintmax_t kMaxGameplayDefinitionsFileBytes = 65536;
inline constexpr std::size_t kMaxGameplayDefinitionsLineLength = 256;
inline constexpr std::size_t kMaxGameplayDefinitionsLines = 512;
inline constexpr std::size_t kMaxGameplayDefinitions = 128;

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

// Emits insertion order. Stats within a definition are emitted in enum order.
// Uses std::to_chars so a later parse recovers the same float.
WriteGameplayDefinitionsResult WriteGameplayDefinitionsText(
    const GameplayDefinitionRegistry& registry);
}
