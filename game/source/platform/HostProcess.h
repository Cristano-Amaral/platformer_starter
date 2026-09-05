#pragma once

// Portable child-process boundary for Milestone 37 editor tools.
// Generic editor/tool-runner code must not include OS headers.
// Windows implementation: platform/WindowsHostProcess.cpp
// Other hosts: platform/HostProcessStub.cpp

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace platform
{
struct HostProcessSpec
{
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path workingDirectory;
};

enum class HostProcessStatus
{
    Idle,
    Running,
    Exited,
    FailedToStart,
};

class HostProcess
{
public:
    HostProcess();
    HostProcess(const HostProcess&) = delete;
    HostProcess& operator=(const HostProcess&) = delete;
    HostProcess(HostProcess&&) = delete;
    HostProcess& operator=(HostProcess&&) = delete;
    ~HostProcess();

    // Non-blocking. Captures stdout and stderr into one stream.
    bool Start(const HostProcessSpec& spec);
    void Poll();
    void Shutdown();

    HostProcessStatus Status() const;
    bool IsRunning() const;
    int ExitCode() const;
    bool HasExitCode() const;
    std::string DrainOutput();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

std::filesystem::path FindHostExecutable(std::string_view name);

#if defined(_WIN32)
std::wstring QuoteHostProcessArgument(std::wstring_view argument);
std::wstring BuildHostProcessCommandLine(
    const std::filesystem::path& executable,
    const std::vector<std::string>& arguments);
#endif
}
