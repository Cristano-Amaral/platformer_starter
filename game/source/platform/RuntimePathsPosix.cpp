#include "platform/RuntimePaths.h"

#include <cstddef>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__) || defined(__FreeBSD__)
#include <unistd.h>
#endif

namespace platform::detail
{
std::filesystem::path QueryExecutableDirectory()
{
#if defined(__linux__)
    std::vector<char> buffer(4096, '\0');
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0)
    {
        return {};
    }

    const std::filesystem::path executable{std::string(buffer.data(), static_cast<std::size_t>(length))};
    if (!executable.is_absolute())
    {
        return {};
    }

    return executable.parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size == 0)
    {
        return {};
    }

    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0)
    {
        return {};
    }

    const std::filesystem::path executable{buffer.c_str()};
    if (!executable.is_absolute())
    {
        return {};
    }

    return executable.parent_path();
#else
    return {};
#endif
}

std::filesystem::path QueryUserDataDirectory()
{
    // M29 is Windows-first. Future POSIX/XDG/mobile roots belong here without
    // changing persistence or gameplay APIs.
    return {};
}
}
