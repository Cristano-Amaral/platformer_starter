#include "platform/HostProcess.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace platform
{
namespace
{
constexpr std::size_t kHostProcessOutputMaxBytes = 1024 * 1024;

std::wstring Utf8ToWide(std::string_view utf8)
{
    if (utf8.empty())
    {
        return {};
    }
    const int needed = MultiByteToWideChar(
        CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (needed <= 0)
    {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), needed);
    return wide;
}

void CloseIfValid(HANDLE& handle)
{
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle);
    }
    handle = nullptr;
}

void AppendBounded(std::string& buffer, std::string_view chunk)
{
    buffer.append(chunk.data(), chunk.size());
    if (buffer.size() > kHostProcessOutputMaxBytes)
    {
        buffer.erase(0, buffer.size() - kHostProcessOutputMaxBytes);
    }
}
}

std::wstring QuoteHostProcessArgument(std::wstring_view argument)
{
    const bool needsQuotes = argument.empty()
        || argument.find_first_of(L" \t\n\v\"") != std::wstring_view::npos;
    if (!needsQuotes)
    {
        return std::wstring(argument);
    }

    std::wstring quoted;
    quoted.push_back(L'"');
    std::size_t backslashes = 0;
    for (wchar_t ch : argument)
    {
        if (ch == L'\\')
        {
            ++backslashes;
            continue;
        }
        if (ch == L'"')
        {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(L'"');
            backslashes = 0;
            continue;
        }
        quoted.append(backslashes, L'\\');
        quoted.push_back(ch);
        backslashes = 0;
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

std::wstring BuildHostProcessCommandLine(
    const std::filesystem::path& executable,
    const std::vector<std::string>& arguments)
{
    std::wstring commandLine = QuoteHostProcessArgument(executable.wstring());
    for (const std::string& argument : arguments)
    {
        commandLine.push_back(L' ');
        commandLine.append(QuoteHostProcessArgument(Utf8ToWide(argument)));
    }
    return commandLine;
}

std::filesystem::path FindHostExecutable(std::string_view name)
{
    if (name.empty())
    {
        return {};
    }

    std::filesystem::path requested{std::string(name)};
    std::error_code error;
    if (requested.is_absolute() && std::filesystem::is_regular_file(requested, error) && !error)
    {
        return requested.lexically_normal();
    }

    std::wstring search = requested.wstring();
    if (requested.extension().empty())
    {
        search += L".exe";
    }

    wchar_t buffer[MAX_PATH]{};
    const DWORD copied =
        SearchPathW(nullptr, search.c_str(), nullptr, MAX_PATH, buffer, nullptr);
    if (copied == 0 || copied >= MAX_PATH)
    {
        return {};
    }
    return std::filesystem::path(buffer).lexically_normal();
}

struct HostProcess::Impl
{
    HANDLE process = nullptr;
    HANDLE stdoutRead = nullptr;
    std::thread reader;
    std::mutex outputMutex;
    std::string output;
    std::atomic<HostProcessStatus> status{HostProcessStatus::Idle};
    std::atomic<int> exitCode{0};
    std::atomic<bool> hasExitCode{false};
    std::atomic<bool> readerDone{true};

    void JoinReader()
    {
        if (reader.joinable())
        {
            reader.join();
        }
    }

    void ResetHandles()
    {
        CloseIfValid(stdoutRead);
        CloseIfValid(process);
    }
};

HostProcess::HostProcess() : impl(std::make_unique<Impl>()) {}

HostProcess::~HostProcess()
{
    Shutdown();
}

bool HostProcess::Start(const HostProcessSpec& spec)
{
    Shutdown();

    if (spec.executable.empty() || spec.workingDirectory.empty()
        || !spec.workingDirectory.is_absolute())
    {
        impl->status = HostProcessStatus::FailedToStart;
        std::lock_guard<std::mutex> lock(impl->outputMutex);
        impl->output = "error: executable or working directory is invalid.\n";
        return false;
    }

    std::error_code error;
    if (!std::filesystem::is_directory(spec.workingDirectory, error) || error)
    {
        impl->status = HostProcessStatus::FailedToStart;
        std::lock_guard<std::mutex> lock(impl->outputMutex);
        impl->output = "error: working directory does not exist.\n";
        return false;
    }

    const std::filesystem::path resolved = spec.executable.is_absolute()
        ? spec.executable
        : FindHostExecutable(spec.executable.string());
    if (resolved.empty() || !std::filesystem::is_regular_file(resolved, error) || error)
    {
        impl->status = HostProcessStatus::FailedToStart;
        std::lock_guard<std::mutex> lock(impl->outputMutex);
        impl->output = "error: executable not found.\n";
        return false;
    }

    SECURITY_ATTRIBUTES security{};
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;

    HANDLE stdoutWrite = nullptr;
    if (CreatePipe(&impl->stdoutRead, &stdoutWrite, &security, 0) == 0)
    {
        impl->status = HostProcessStatus::FailedToStart;
        std::lock_guard<std::mutex> lock(impl->outputMutex);
        impl->output = "error: failed to create output pipe.\n";
        return false;
    }
    SetHandleInformation(impl->stdoutRead, HANDLE_FLAG_INHERIT, 0);

    std::wstring commandLine = BuildHostProcessCommandLine(resolved, spec.arguments);
    std::vector<wchar_t> commandBuffer(commandLine.begin(), commandLine.end());
    commandBuffer.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = stdoutWrite;
    startup.hStdError = stdoutWrite;

    PROCESS_INFORMATION info{};
    const BOOL created = CreateProcessW(
        resolved.c_str(),
        commandBuffer.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
        nullptr,
        spec.workingDirectory.c_str(),
        &startup,
        &info);
    CloseIfValid(stdoutWrite);
    if (created == 0)
    {
        impl->ResetHandles();
        impl->status = HostProcessStatus::FailedToStart;
        std::lock_guard<std::mutex> lock(impl->outputMutex);
        impl->output = "error: CreateProcessW failed.\n";
        return false;
    }
    CloseHandle(info.hThread);

    impl->process = info.hProcess;
    impl->hasExitCode = false;
    impl->exitCode = 0;
    impl->readerDone = false;
    impl->status = HostProcessStatus::Running;
    impl->output.clear();

    HANDLE readHandle = impl->stdoutRead;
    impl->reader = std::thread([this, readHandle]() {
        char buffer[4096];
        DWORD bytesRead = 0;
        while (ReadFile(readHandle, buffer, sizeof(buffer), &bytesRead, nullptr) != 0
            && bytesRead > 0)
        {
            std::lock_guard<std::mutex> lock(impl->outputMutex);
            AppendBounded(impl->output, std::string_view(buffer, bytesRead));
        }
        impl->readerDone = true;
    });
    return true;
}

void HostProcess::Poll()
{
    if (impl->status != HostProcessStatus::Running || impl->process == nullptr)
    {
        return;
    }

    if (WaitForSingleObject(impl->process, 0) != WAIT_OBJECT_0)
    {
        return;
    }

    DWORD code = 0;
    GetExitCodeProcess(impl->process, &code);
    if (!impl->readerDone.load())
    {
        return;
    }

    impl->JoinReader();
    impl->exitCode = static_cast<int>(code);
    impl->hasExitCode = true;
    impl->status = HostProcessStatus::Exited;
}

void HostProcess::Shutdown()
{
    if (impl->process != nullptr && impl->status == HostProcessStatus::Running)
    {
        TerminateProcess(impl->process, 1);
        WaitForSingleObject(impl->process, 2000);
    }
    CloseIfValid(impl->stdoutRead);
    impl->JoinReader();
    impl->ResetHandles();
    if (impl->status == HostProcessStatus::Running)
    {
        impl->status = HostProcessStatus::Idle;
        impl->hasExitCode = false;
    }
    impl->readerDone = true;
}

HostProcessStatus HostProcess::Status() const
{
    return impl->status;
}

bool HostProcess::IsRunning() const
{
    return impl->status == HostProcessStatus::Running;
}

int HostProcess::ExitCode() const
{
    return impl->exitCode;
}

bool HostProcess::HasExitCode() const
{
    return impl->hasExitCode;
}

std::string HostProcess::DrainOutput()
{
    std::lock_guard<std::mutex> lock(impl->outputMutex);
    std::string chunk;
    chunk.swap(impl->output);
    return chunk;
}
}
