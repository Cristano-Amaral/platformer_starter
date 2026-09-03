#include "platform/FileReplace.h"

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

    // POSIX rename typically replaces an existing destination on the same
    // filesystem. This is compile-compatible scaffolding only. Windows is
    // the M29-validated platform.
    std::error_code error;
    std::filesystem::rename(temporaryPath, finalPath, error);
    return !error;
}
}
