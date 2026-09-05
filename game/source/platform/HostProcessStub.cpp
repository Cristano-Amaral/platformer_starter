#include "platform/HostProcess.h"

namespace platform
{
struct HostProcess::Impl
{
};

HostProcess::HostProcess() : impl(std::make_unique<Impl>()) {}

HostProcess::~HostProcess() = default;

bool HostProcess::Start(const HostProcessSpec&)
{
    return false;
}

void HostProcess::Poll() {}

void HostProcess::Shutdown() {}

HostProcessStatus HostProcess::Status() const
{
    return HostProcessStatus::Idle;
}

bool HostProcess::IsRunning() const
{
    return false;
}

int HostProcess::ExitCode() const
{
    return 0;
}

bool HostProcess::HasExitCode() const
{
    return false;
}

std::string HostProcess::DrainOutput()
{
    return {};
}

std::filesystem::path FindHostExecutable(std::string_view)
{
    return {};
}
}
