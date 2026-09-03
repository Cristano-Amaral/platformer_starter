#include "platform/FileReplace.h"
#include "platform/RuntimePaths.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <knownfolders.h>
#include <objbase.h>
#include <shlobj.h>

#include <memory>
#include <string>
#include <system_error>

namespace platform::detail
{
std::filesystem::path QueryExecutableDirectory()
{
    DWORD capacity = MAX_PATH;
    std::wstring buffer(static_cast<size_t>(capacity), L'\0');

    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), capacity);
        if (length == 0)
        {
            return {};
        }

        if (length < capacity)
        {
            buffer.resize(static_cast<size_t>(length));
            const std::filesystem::path executable{buffer};
            if (!executable.is_absolute())
            {
                return {};
            }

            return executable.parent_path();
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            return {};
        }

        capacity *= 2;
        buffer.assign(static_cast<size_t>(capacity), L'\0');
    }

    return {};
}

std::filesystem::path QueryUserDataDirectory()
{
    PWSTR rawPath = nullptr;
    const HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &rawPath);
    std::unique_ptr<wchar_t, void (*)(wchar_t*)> owned(
        rawPath,
        [](wchar_t* value)
        {
            CoTaskMemFree(value);
        });
    if (FAILED(hr) || owned == nullptr)
    {
        return {};
    }

    const std::filesystem::path directory{owned.get()};
    if (directory.empty() || !directory.is_absolute())
    {
        return {};
    }

    return directory;
}
}

namespace platform
{
bool ReplaceFileWithTemporary(
    const std::filesystem::path& temporaryPath,
    const std::filesystem::path& finalPath)
{
    if (temporaryPath.empty() || finalPath.empty() || !temporaryPath.is_absolute()
        || !finalPath.is_absolute())
    {
        return false;
    }

    std::error_code existsError;
    const bool finalExists = std::filesystem::exists(finalPath, existsError);
    if (existsError)
    {
        return false;
    }

    if (finalExists)
    {
        std::error_code typeError;
        if (!std::filesystem::is_regular_file(finalPath, typeError) || typeError)
        {
            return false;
        }

        // lpReplacedFileName is the existing canonical save.
        // lpReplacementFileName is the completed temp sibling.
        // No backup file. REPLACEFILE_WRITE_THROUGH is documented as
        // unsupported, so flags stay 0. Durability of the temp contents is
        // the caller's flush/close of the temp stream, not this flag.
        return ReplaceFileW(
                   finalPath.c_str(),
                   temporaryPath.c_str(),
                   nullptr,
                   0,
                   nullptr,
                   nullptr)
            != FALSE;
    }

    // First-ever save: destination is missing, so ReplaceFileW cannot run.
    return MoveFileExW(
               temporaryPath.c_str(),
               finalPath.c_str(),
               MOVEFILE_WRITE_THROUGH)
        != FALSE;
}
}
