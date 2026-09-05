#include "editor/EditorToolRunner.h"

#include "platform/HostProcess.h"

#include <string>

namespace editor
{
namespace
{
void RefreshElapsed(double& elapsedSeconds, std::chrono::steady_clock::time_point startTime)
{
    elapsedSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime)
                         .count();
}
}

const char* EditorToolJobStateName(EditorToolJobState state)
{
    switch (state)
    {
    case EditorToolJobState::Idle:
        return "Idle";
    case EditorToolJobState::Running:
        return "Running";
    case EditorToolJobState::Succeeded:
        return "Succeeded";
    case EditorToolJobState::Failed:
        return "Failed";
    }
    return "Idle";
}

bool EditorToolRunner::IsRunning() const
{
    return state == EditorToolJobState::Running;
}

EditorToolJobSnapshot EditorToolRunner::Snapshot() const
{
    EditorToolJobSnapshot snapshot{};
    snapshot.kind = kind;
    snapshot.displayLabel = displayLabel;
    snapshot.state = state;
    snapshot.exitCode = exitCode;
    snapshot.hasExitCode = hasExitCode;
    snapshot.elapsedSeconds = elapsedSeconds;
    snapshot.log = log;
    if (kind == EditorToolKind::BuildAll && !sequence.empty())
    {
        snapshot.buildAllStep = static_cast<int>(sequenceIndex) + 1;
        snapshot.buildAllStepCount = static_cast<int>(sequence.size());
        snapshot.currentStepLabel = BuildAllStepLabel(static_cast<int>(sequenceIndex));
    }
    return snapshot;
}

bool EditorToolRunner::BeginFailed(EditorToolKind requestedKind, std::string_view message)
{
    kind = requestedKind;
    displayLabel = EditorToolKindName(requestedKind);
    state = EditorToolJobState::Failed;
    hasExitCode = false;
    exitCode = 0;
    elapsedSeconds = 0.0;
    startTime = std::chrono::steady_clock::now();
    log.clear();
    sequence.clear();
    sequenceIndex = 0;
    AppendOutput(message);
    return true;
}

bool EditorToolRunner::LaunchCurrentCommand()
{
    if (sequenceIndex >= sequence.size())
    {
        return false;
    }

    const EditorToolCommand& command = sequence[sequenceIndex];
    const std::filesystem::path resolved = platform::FindHostExecutable(command.executableName);
    if (resolved.empty())
    {
        AppendOutput("error: executable not found: ");
        AppendOutput(command.executableName);
        AppendOutput("\n");
        state = EditorToolJobState::Failed;
        hasExitCode = false;
        process.Shutdown();
        return false;
    }

    AppendOutput("starting ");
    AppendOutput(command.displayLabel);
    AppendOutput(" (");
    AppendOutput(command.executableName);
    for (const std::string& argument : command.arguments)
    {
        AppendOutput(" ");
        AppendOutput(argument);
    }
    AppendOutput(")\n");

    platform::HostProcessSpec spec{};
    spec.executable = resolved;
    spec.arguments = command.arguments;
    spec.workingDirectory = command.workingDirectory;
    process.Shutdown();
    if (!process.Start(spec))
    {
        AppendOutput(process.DrainOutput());
        if (log.empty())
        {
            AppendOutput("error: failed to start process.\n");
        }
        state = EditorToolJobState::Failed;
        hasExitCode = false;
        return false;
    }
    return true;
}

bool EditorToolRunner::TryStart(
    EditorToolKind requestedKind,
    const std::filesystem::path& repositoryRoot,
    bool executionAvailable)
{
    if (IsRunning())
    {
        return false;
    }

    log.clear();
    hasExitCode = false;
    exitCode = 0;
    elapsedSeconds = 0.0;
    sequence.clear();
    sequenceIndex = 0;
    process.Shutdown();

    if (!executionAvailable)
    {
        return BeginFailed(
            requestedKind, "error: external tool execution is unavailable in this configuration.\n");
    }
    if (!IsValidRepositoryRoot(repositoryRoot))
    {
        return BeginFailed(
            requestedKind,
            "error: repository root is invalid. Expected CMakeLists.txt, CMakePresets.json, and "
            "tools/cook_assets.py. No process was launched.\n");
    }

    kind = requestedKind;
    displayLabel = EditorToolKindName(requestedKind);
    startTime = std::chrono::steady_clock::now();

    if (requestedKind == EditorToolKind::BuildAll)
    {
        if (!IsCMakeBuildTreeConfigured(repositoryRoot))
        {
            return BeginFailed(
                requestedKind,
                "error: CMake build tree is not configured. Run: cmake --preset windows-vs2022\n");
        }
        sequence = MakeBuildAllPlan(repositoryRoot);
    }
    else if (requestedKind == EditorToolKind::CookAssets)
    {
        sequence = {MakeCookAssetsCommand(repositoryRoot)};
    }
    else if (requestedKind == EditorToolKind::BuildDebug)
    {
        if (!IsCMakeBuildTreeConfigured(repositoryRoot))
        {
            return BeginFailed(
                requestedKind,
                "error: CMake build tree is not configured. Run: cmake --preset windows-vs2022\n");
        }
        sequence = {MakeBuildDebugCommand(repositoryRoot)};
    }
    else if (requestedKind == EditorToolKind::BuildDevelopment)
    {
        if (!IsCMakeBuildTreeConfigured(repositoryRoot))
        {
            return BeginFailed(
                requestedKind,
                "error: CMake build tree is not configured. Run: cmake --preset windows-vs2022\n");
        }
        sequence = {MakeBuildDevelopmentCommand(repositoryRoot)};
    }
    else
    {
        if (!IsCMakeBuildTreeConfigured(repositoryRoot))
        {
            return BeginFailed(
                requestedKind,
                "error: CMake build tree is not configured. Run: cmake --preset windows-vs2022\n");
        }
        sequence = {MakeBuildReleaseCommand(repositoryRoot)};
    }

    state = EditorToolJobState::Running;
    if (kind == EditorToolKind::BuildAll)
    {
        AppendBuildAllStepMarker(0);
    }
    if (!LaunchCurrentCommand())
    {
        RefreshElapsed(elapsedSeconds, startTime);
        return true;
    }
    return true;
}

bool EditorToolRunner::TryStartCommand(const EditorToolCommand& command, bool executionAvailable)
{
    if (IsRunning())
    {
        return false;
    }

    log.clear();
    hasExitCode = false;
    exitCode = 0;
    elapsedSeconds = 0.0;
    sequence = {command};
    sequenceIndex = 0;
    process.Shutdown();
    if (!executionAvailable)
    {
        return BeginFailed(
            command.kind, "error: external tool execution is unavailable in this configuration.\n");
    }
    kind = command.kind;
    displayLabel = command.displayLabel.empty() ? EditorToolKindName(command.kind)
                                                : command.displayLabel;
    startTime = std::chrono::steady_clock::now();
    state = EditorToolJobState::Running;
    if (!LaunchCurrentCommand())
    {
        RefreshElapsed(elapsedSeconds, startTime);
        return true;
    }
    return true;
}

void EditorToolRunner::AppendOutput(std::string_view chunk)
{
    AppendBoundedLog(log, chunk, kEditorToolLogMaxBytes);
}

void EditorToolRunner::AppendBuildAllStepMarker(int zeroBasedStep)
{
    AppendOutput("=== Build All: Step ");
    AppendOutput(std::to_string(zeroBasedStep + 1));
    AppendOutput("/3 - ");
    AppendOutput(BuildAllStepLabel(zeroBasedStep));
    AppendOutput(" ===\n");
}

void EditorToolRunner::AppendExitCodeLine(int processExitCode)
{
    AppendOutput("exit code: ");
    AppendOutput(std::to_string(processExitCode));
    AppendOutput("\n");
}

void EditorToolRunner::FinishProcess(int processExitCode)
{
    hasExitCode = true;
    exitCode = processExitCode;
    AppendExitCodeLine(processExitCode);
    if (kind != EditorToolKind::BuildAll)
    {
        state = processExitCode == 0 ? EditorToolJobState::Succeeded : EditorToolJobState::Failed;
        RefreshElapsed(elapsedSeconds, startTime);
        return;
    }

    const BuildAllAdvanceResult advance = AdvanceBuildAll(
        static_cast<int>(sequenceIndex), processExitCode, static_cast<int>(sequence.size()));
    if (advance == BuildAllAdvanceResult::Failed)
    {
        AppendOutput("Build All failed at ");
        AppendOutput(BuildAllStepLabel(static_cast<int>(sequenceIndex)));
        AppendOutput("\n");
        state = EditorToolJobState::Failed;
        RefreshElapsed(elapsedSeconds, startTime);
        return;
    }
    if (advance == BuildAllAdvanceResult::Succeeded)
    {
        AppendOutput("Build All succeeded.\n");
        state = EditorToolJobState::Succeeded;
        RefreshElapsed(elapsedSeconds, startTime);
        return;
    }

    ++sequenceIndex;
    AppendBuildAllStepMarker(static_cast<int>(sequenceIndex));
    hasExitCode = false;
    if (!LaunchCurrentCommand())
    {
        RefreshElapsed(elapsedSeconds, startTime);
    }
}

void EditorToolRunner::Poll()
{
    if (state != EditorToolJobState::Running)
    {
        return;
    }

    process.Poll();
    AppendOutput(process.DrainOutput());
    RefreshElapsed(elapsedSeconds, startTime);

    if (process.Status() == platform::HostProcessStatus::FailedToStart)
    {
        state = EditorToolJobState::Failed;
        hasExitCode = false;
        return;
    }
    if (process.HasExitCode() && process.Status() == platform::HostProcessStatus::Exited)
    {
        AppendOutput(process.DrainOutput());
        FinishProcess(process.ExitCode());
    }
}

void EditorToolRunner::ClearLog()
{
    if (state == EditorToolJobState::Running)
    {
        return;
    }
    log.clear();
}

void EditorToolRunner::Shutdown()
{
    process.Shutdown();
    if (state == EditorToolJobState::Running)
    {
        AppendOutput("error: tool process terminated because the editor is shutting down.\n");
        state = EditorToolJobState::Failed;
        hasExitCode = false;
        RefreshElapsed(elapsedSeconds, startTime);
    }
}
}
