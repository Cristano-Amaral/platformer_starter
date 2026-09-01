#include "platform/RuntimePaths.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>

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
}
