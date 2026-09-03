#include "persistence/BestTimeSave.h"

#include "platform/FileReplace.h"
#include "platform/RuntimePaths.h"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

namespace persistence
{
namespace
{
bool ConsumeLine(std::string_view& remaining, std::string_view& line)
{
    if (remaining.empty())
    {
        return false;
    }

    const std::size_t newline = remaining.find('\n');
    if (newline == std::string_view::npos)
    {
        line = remaining;
        remaining = {};
        return true;
    }

    line = remaining.substr(0, newline);
    if (!line.empty() && line.back() == '\r')
    {
        line.remove_suffix(1);
    }

    remaining.remove_prefix(newline + 1);
    return true;
}

bool IsUnsignedIntegerToken(std::string_view token)
{
    if (token.empty())
    {
        return false;
    }

    for (const char character : token)
    {
        if (character < '0' || character > '9')
        {
            return false;
        }
    }

    return true;
}

LoadBestTimeResult InvalidResult()
{
    return LoadBestTimeResult{LoadBestTimeStatus::Invalid, 0.0};
}

LoadBestTimeResult ParseHeaderLine(std::string_view line)
{
    constexpr std::string_view prefix = "PLATFORMER_SAVE ";
    if (!line.starts_with(prefix))
    {
        return InvalidResult();
    }

    const std::string_view versionToken = line.substr(prefix.size());
    if (versionToken == "1")
    {
        LoadBestTimeResult ok{};
        ok.status = LoadBestTimeStatus::Loaded;
        return ok;
    }

    if (IsUnsignedIntegerToken(versionToken))
    {
        return LoadBestTimeResult{LoadBestTimeStatus::UnsupportedVersion, 0.0};
    }

    return InvalidResult();
}

LoadBestTimeResult ParseBestSecondsLine(std::string_view line)
{
    constexpr std::string_view prefix = "best_seconds ";
    if (!line.starts_with(prefix))
    {
        return InvalidResult();
    }

    const std::string_view numberToken = line.substr(prefix.size());
    if (numberToken.empty())
    {
        return InvalidResult();
    }

    double value = 0.0;
    const std::from_chars_result parsed =
        std::from_chars(numberToken.data(), numberToken.data() + numberToken.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != numberToken.data() + numberToken.size())
    {
        return InvalidResult();
    }

    if (!std::isfinite(value) || !(value > 0.0))
    {
        return InvalidResult();
    }

    return LoadBestTimeResult{LoadBestTimeStatus::Loaded, value};
}

std::filesystem::path JoinUnderUserData(std::string_view relative)
{
    const std::filesystem::path userData = platform::UserDataDirectory();
    if (userData.empty())
    {
        return {};
    }

    return (userData / std::string(kBestTimeProjectDirectoryName) / std::string(relative))
        .lexically_normal();
}

LoadBestTimeResult ErrorResult()
{
    return LoadBestTimeResult{LoadBestTimeStatus::Error, 0.0};
}

void BestEffortRemove(const std::filesystem::path& path)
{
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

bool PathUsable(const std::filesystem::path& path)
{
    return !path.empty() && path.is_absolute();
}
}

std::string SerializeBestTimeV1(double bestSeconds)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << kBestTimeSaveMagic << ' ' << kBestTimeSaveVersion << '\n'
           << kBestTimeSecondsKey << ' '
           << std::setprecision(std::numeric_limits<double>::max_digits10) << bestSeconds
           << '\n';
    return stream.str();
}

LoadBestTimeResult ParseBestTimeV1(std::string_view text)
{
    if (text.empty())
    {
        return InvalidResult();
    }

    std::string_view remaining = text;
    std::string_view headerLine;
    if (!ConsumeLine(remaining, headerLine))
    {
        return InvalidResult();
    }

    const LoadBestTimeResult header = ParseHeaderLine(headerLine);
    if (header.status == LoadBestTimeStatus::UnsupportedVersion)
    {
        return header;
    }
    if (header.status != LoadBestTimeStatus::Loaded)
    {
        return InvalidResult();
    }

    std::string_view valueLine;
    if (!ConsumeLine(remaining, valueLine))
    {
        return InvalidResult();
    }

    if (!remaining.empty())
    {
        return InvalidResult();
    }

    return ParseBestSecondsLine(valueLine);
}

std::filesystem::path BestTimeSaveDirectory()
{
    const std::filesystem::path userData = platform::UserDataDirectory();
    if (userData.empty())
    {
        return {};
    }

    return (userData / std::string(kBestTimeProjectDirectoryName)).lexically_normal();
}

std::filesystem::path BestTimeSavePath()
{
    return JoinUnderUserData(kBestTimeSaveFileName);
}

std::filesystem::path BestTimeTempPath()
{
    return JoinUnderUserData(kBestTimeTempFileName);
}

LoadBestTimeResult LoadBestTime()
{
    const std::filesystem::path path = BestTimeSavePath();
    if (!PathUsable(path))
    {
        return ErrorResult();
    }

    std::error_code existsError;
    const bool exists = std::filesystem::exists(path, existsError);
    if (existsError)
    {
        return ErrorResult();
    }
    if (!exists)
    {
        return LoadBestTimeResult{LoadBestTimeStatus::Missing, 0.0};
    }

    std::error_code typeError;
    if (!std::filesystem::is_regular_file(path, typeError) || typeError)
    {
        return ErrorResult();
    }

    std::error_code sizeError;
    const std::uintmax_t size = std::filesystem::file_size(path, sizeError);
    if (sizeError)
    {
        return ErrorResult();
    }

    constexpr std::uintmax_t kMaxSaveBytes = 4096;
    if (size > kMaxSaveBytes)
    {
        return ErrorResult();
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return ErrorResult();
    }

    std::string text(static_cast<std::size_t>(size), '\0');
    if (size > 0)
    {
        stream.read(text.data(), static_cast<std::streamsize>(size));
        if (stream.bad() || stream.gcount() != static_cast<std::streamsize>(size))
        {
            return ErrorResult();
        }
    }

    return ParseBestTimeV1(text);
}

SaveBestTimeStatus SaveBestTime(double bestSeconds)
{
    if (!std::isfinite(bestSeconds) || !(bestSeconds > 0.0))
    {
        return SaveBestTimeStatus::Error;
    }

    const std::filesystem::path directory = BestTimeSaveDirectory();
    const std::filesystem::path finalPath = BestTimeSavePath();
    const std::filesystem::path tempPath = BestTimeTempPath();
    if (!PathUsable(directory) || !PathUsable(finalPath) || !PathUsable(tempPath))
    {
        return SaveBestTimeStatus::Error;
    }

    std::error_code createError;
    std::filesystem::create_directories(directory, createError);
    if (createError)
    {
        return SaveBestTimeStatus::Error;
    }

    const std::string text = SerializeBestTimeV1(bestSeconds);
    {
        std::ofstream stream(tempPath, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!stream)
        {
            return SaveBestTimeStatus::Error;
        }

        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        stream.flush();
        const bool writeOk = static_cast<bool>(stream);
        stream.close();
        if (!writeOk || stream.fail())
        {
            BestEffortRemove(tempPath);
            return SaveBestTimeStatus::Error;
        }
    }

    if (!platform::ReplaceFileWithTemporary(tempPath, finalPath))
    {
        BestEffortRemove(tempPath);
        return SaveBestTimeStatus::Error;
    }

    return SaveBestTimeStatus::Saved;
}
}
