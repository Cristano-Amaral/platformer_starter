#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace persistence
{
inline constexpr std::string_view kBestTimeSaveMagic = "PLATFORMER_SAVE";
inline constexpr int kBestTimeSaveVersion = 1;
inline constexpr std::string_view kBestTimeProjectDirectoryName = "Platformer3D";
inline constexpr std::string_view kBestTimeSaveFileName = "best_time_v1.txt";
inline constexpr std::string_view kBestTimeTempFileName = "best_time_v1.tmp";
inline constexpr std::string_view kBestTimeSecondsKey = "best_seconds";

// Load outcomes. Missing is first-run, not an error. UnsupportedVersion is
// distinct from Invalid so a future v2 file is not mistaken for garbage.
enum class LoadBestTimeStatus
{
    Missing,
    Loaded,
    Invalid,
    UnsupportedVersion,
    Error,
};

// Save outcomes. NotAttempted is Application debug state, not a disk result.
enum class SaveBestTimeStatus
{
    NotAttempted,
    Saved,
    Error,
};

struct LoadBestTimeResult
{
    LoadBestTimeStatus status = LoadBestTimeStatus::Invalid;
    // Meaningful only when status == Loaded.
    double bestSeconds = 0.0;
};

inline const char* LoadBestTimeStatusName(LoadBestTimeStatus status)
{
    switch (status)
    {
    case LoadBestTimeStatus::Missing:
        return "Missing";
    case LoadBestTimeStatus::Loaded:
        return "Loaded";
    case LoadBestTimeStatus::Invalid:
        return "Invalid";
    case LoadBestTimeStatus::UnsupportedVersion:
        return "UnsupportedVersion";
    case LoadBestTimeStatus::Error:
        return "Error";
    }
    return "Error";
}

inline const char* SaveBestTimeStatusName(SaveBestTimeStatus status)
{
    switch (status)
    {
    case SaveBestTimeStatus::NotAttempted:
        return "NotAttempted";
    case SaveBestTimeStatus::Saved:
        return "Saved";
    case SaveBestTimeStatus::Error:
        return "Error";
    }
    return "Error";
}

// Canonical v1 text. Newline is '\n'. Precision is max_digits10.
std::string SerializeBestTimeV1(double bestSeconds);

// Strict in-memory parse. Does not touch the filesystem. Empty input is Invalid,
// not Missing (Missing is a missing canonical file at load time).
LoadBestTimeResult ParseBestTimeV1(std::string_view text);

// Path helpers join platform::UserDataDirectory() / Platformer3D / filename.
// LoadBestTime reads only best_time_v1.txt (never .tmp), does not create the
// user-data directory, and maps missing file to Missing. SaveBestTime creates
// the directory, writes a complete temp sibling, flush/closes, then calls
// platform::ReplaceFileWithTemporary. No standalone remove(final).
std::filesystem::path BestTimeSaveDirectory();
std::filesystem::path BestTimeSavePath();
std::filesystem::path BestTimeTempPath();

LoadBestTimeResult LoadBestTime();
SaveBestTimeStatus SaveBestTime(double bestSeconds);
}
