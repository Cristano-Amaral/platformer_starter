#pragma once

#include <cstddef>
#include <cstdio>

namespace core
{
// Display-only breakdown of a run time. Internal elapsedSeconds stays double.
struct RunTimeParts
{
    int minutes = 0;
    int seconds = 0;
    int milliseconds = 0;
};

constexpr bool RunTimePartsEqual(RunTimeParts parts, int minutes, int seconds, int milliseconds)
{
    return parts.minutes == minutes && parts.seconds == seconds && parts.milliseconds == milliseconds;
}

// Truncate toward zero for non-negative millisecond totals. Minutes are not
// wrapped at 99. Negative totals display as 00:00.000.
constexpr RunTimeParts RunTimePartsFromTotalMilliseconds(long long totalMilliseconds)
{
    long long total = totalMilliseconds;
    if (total < 0)
    {
        total = 0;
    }

    const long long minutes = total / 60000;
    const long long remainder = total % 60000;
    return RunTimeParts{
        static_cast<int>(minutes),
        static_cast<int>(remainder / 1000),
        static_cast<int>(remainder % 1000),
    };
}

// Display conversion only. Does not quantize gameplay elapsedSeconds.
// Policy: floor/truncate to whole elapsed milliseconds so the HUD never
// shows a millisecond that has not elapsed (1.9999s -> 00:01.999).
// Non-finite and non-positive values display as 00:00.000.
constexpr RunTimeParts RunTimePartsFromSeconds(double elapsedSeconds)
{
    if (!(elapsedSeconds > 0.0))
    {
        return {};
    }

    const double totalMilliseconds = elapsedSeconds * 1000.0;
    return RunTimePartsFromTotalMilliseconds(static_cast<long long>(totalMilliseconds));
}

// Writes MM:SS.mmm (no TIME prefix). Minutes use at least two digits and
// grow past 99 without wrapping (123:45.678 is valid).
inline void FormatRunTimeParts(char* buffer, std::size_t bufferSize, RunTimeParts parts)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    std::snprintf(
        buffer,
        bufferSize,
        "%02d:%02d.%03d",
        parts.minutes,
        parts.seconds,
        parts.milliseconds);
}

inline void FormatRunTime(char* buffer, std::size_t bufferSize, double elapsedSeconds)
{
    FormatRunTimeParts(buffer, bufferSize, RunTimePartsFromSeconds(elapsedSeconds));
}

// Presentation-only. Does not parse back into session state.
// No-best HUD/debug text is exactly --:--.---, never 00:00.000.
inline constexpr const char* kNoSessionBestPlaceholder = "--:--.---";

inline void FormatSessionBestTime(
    char* buffer,
    std::size_t bufferSize,
    bool hasBestTime,
    double bestSeconds)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    if (!hasBestTime)
    {
        std::snprintf(buffer, bufferSize, "%s", kNoSessionBestPlaceholder);
        return;
    }

    FormatRunTime(buffer, bufferSize, bestSeconds);
}

// Integer-millisecond cases avoid brittle binary literals such as 0.001 or 5.2.
static_assert(RunTimePartsEqual(RunTimePartsFromTotalMilliseconds(0), 0, 0, 0));
static_assert(RunTimePartsEqual(RunTimePartsFromTotalMilliseconds(1), 0, 0, 1));
static_assert(RunTimePartsEqual(RunTimePartsFromTotalMilliseconds(5200), 0, 5, 200));
static_assert(RunTimePartsEqual(RunTimePartsFromTotalMilliseconds(59999), 0, 59, 999));
static_assert(RunTimePartsEqual(RunTimePartsFromTotalMilliseconds(60000), 1, 0, 0));
static_assert(RunTimePartsEqual(RunTimePartsFromTotalMilliseconds(65432), 1, 5, 432));
static_assert(RunTimePartsEqual(RunTimePartsFromTotalMilliseconds(754567), 12, 34, 567));
static_assert(RunTimePartsEqual(RunTimePartsFromSeconds(0.0), 0, 0, 0));
static_assert(RunTimePartsEqual(RunTimePartsFromSeconds(60.0), 1, 0, 0));
}
