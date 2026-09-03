#include "platform/FileReplace.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <system_error>

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
