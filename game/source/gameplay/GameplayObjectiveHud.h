#pragma once

// Milestone 67: compact Gameplay Level/objective HUD presentation.
// Derived only from currentRuntimeLevelId and already-loaded LevelGoalSpec
// data. Not an Objective, Quest, HUD, notification, or localization system.
// Never serialized and never mutates authored data.

#include "gameplay/GameFlowState.h"
#include "world/LevelGoal.h"
#include "world/LevelIdentity.h"

#include <cstddef>
#include <cstdio>
#include <string_view>
#include <vector>

namespace gameplay
{
inline constexpr const char* kGameplayObjectiveHudDestinationText =
    "OBJECTIVE: REACH THE LEVEL GOAL";
inline constexpr const char* kGameplayObjectiveHudTerminalText =
    "OBJECTIVE: REACH THE FINAL LEVEL GOAL";
inline constexpr const char* kGameplayObjectiveHudMixedText =
    "OBJECTIVE: REACH A LEVEL GOAL";
inline constexpr const char* kGameplayObjectiveHudUnknownLevelLabel = "LEVEL";

inline constexpr std::size_t kGameplayObjectiveHudLevelLabelCapacity = 32;
inline constexpr std::size_t kGameplayObjectiveHudObjectiveCapacity = 64;

// TIME is y=20 font 22; BEST is y=46 font 22 (through y=68). Keep this HUD
// in the same top-left band without overlapping those lines, the top-right
// COLLECTED counter, centered Inventory/completion overlays, or bottom
// interaction / M62 prompts. Not a layout engine.
inline constexpr int kGameplayObjectiveHudMarginX = 20;
inline constexpr int kGameplayObjectiveHudLevelFontSize = 20;
inline constexpr int kGameplayObjectiveHudObjectiveFontSize = 18;
inline constexpr int kGameplayObjectiveHudLevelY = 80;
inline constexpr int kGameplayObjectiveHudObjectiveY = 104;

enum class GameplayObjectiveKind
{
    None,
    Destination,
    Terminal,
    Mixed,
};

inline bool ObjectiveHudIsVisible(
    TopLevelFlow flow,
    bool editorActive,
    bool inventoryOpen,
    bool runCompleteActive,
    bool levelCompleted)
{
    return flow == TopLevelFlow::Gameplay && !editorActive && !inventoryOpen
        && !runCompleteActive && !levelCompleted;
}

inline GameplayObjectiveKind ClassifyGameplayObjective(
    const std::vector<world::LevelGoalSpec>& goals)
{
    if (goals.empty())
    {
        return GameplayObjectiveKind::None;
    }

    bool anyDestination = false;
    bool anyTerminal = false;
    for (const world::LevelGoalSpec& goal : goals)
    {
        if (goal.nextLevelId.empty())
        {
            anyTerminal = true;
        }
        else
        {
            anyDestination = true;
        }
        if (anyDestination && anyTerminal)
        {
            return GameplayObjectiveKind::Mixed;
        }
    }
    if (anyDestination)
    {
        return GameplayObjectiveKind::Destination;
    }
    return GameplayObjectiveKind::Terminal;
}

inline bool TryFriendlyLevelDigits(std::string_view runtimeLevelId, std::string_view& outDigits)
{
    constexpr std::string_view kPrefix = "level_";
    if (runtimeLevelId.size() <= kPrefix.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < kPrefix.size(); ++index)
    {
        char character = runtimeLevelId[index];
        if (character >= 'A' && character <= 'Z')
        {
            character = static_cast<char>(character - 'A' + 'a');
        }
        if (character != kPrefix[index])
        {
            return false;
        }
    }
    const std::string_view digits = runtimeLevelId.substr(kPrefix.size());
    if (digits.empty())
    {
        return false;
    }
    for (const char character : digits)
    {
        if (character < '0' || character > '9')
        {
            return false;
        }
    }
    outDigits = digits;
    return true;
}

inline void FormatGameplayLevelLabel(
    char* buffer,
    std::size_t bufferSize,
    std::string_view runtimeLevelId)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }
    buffer[0] = '\0';

    std::string_view digits;
    if (TryFriendlyLevelDigits(runtimeLevelId, digits))
    {
        std::snprintf(
            buffer,
            bufferSize,
            "LEVEL %.*s",
            static_cast<int>(digits.size()),
            digits.data());
        return;
    }

    if (!world::IsValidLevelIdToken(runtimeLevelId))
    {
        std::snprintf(buffer, bufferSize, "%s", kGameplayObjectiveHudUnknownLevelLabel);
        return;
    }

    std::size_t written = 0;
    const std::size_t limit = bufferSize - 1;
    for (const char character : runtimeLevelId)
    {
        if (written >= limit)
        {
            break;
        }
        if (character == '_')
        {
            if (written > 0 && buffer[written - 1] != ' ')
            {
                buffer[written++] = ' ';
            }
        }
        else if (character >= 'a' && character <= 'z')
        {
            buffer[written++] = static_cast<char>(character - 'a' + 'A');
        }
        else
        {
            buffer[written++] = character;
        }
    }
    while (written > 0 && buffer[written - 1] == ' ')
    {
        --written;
    }
    buffer[written] = '\0';
    if (written == 0)
    {
        std::snprintf(buffer, bufferSize, "%s", kGameplayObjectiveHudUnknownLevelLabel);
    }
}

inline void FormatGameplayObjectiveLine(
    char* buffer,
    std::size_t bufferSize,
    const std::vector<world::LevelGoalSpec>& goals)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }
    buffer[0] = '\0';

    const char* text = nullptr;
    switch (ClassifyGameplayObjective(goals))
    {
    case GameplayObjectiveKind::None:
        return;
    case GameplayObjectiveKind::Destination:
        text = kGameplayObjectiveHudDestinationText;
        break;
    case GameplayObjectiveKind::Terminal:
        text = kGameplayObjectiveHudTerminalText;
        break;
    case GameplayObjectiveKind::Mixed:
        text = kGameplayObjectiveHudMixedText;
        break;
    }
    if (text != nullptr)
    {
        std::snprintf(buffer, bufferSize, "%s", text);
    }
}
}
