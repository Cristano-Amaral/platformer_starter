#pragma once

// One-active-job editor tool runner. Captures HostProcess output. No ImGui.
// The Development Build menu calls TryStart; this type owns job state.

#include "editor/EditorToolCommands.h"
#include "platform/HostProcess.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace editor
{
enum class EditorToolJobState
{
    Idle,
    Running,
    Succeeded,
    Failed,
};

struct EditorToolJobSnapshot
{
    EditorToolKind kind = EditorToolKind::CookAssets;
    std::string displayLabel;
    EditorToolJobState state = EditorToolJobState::Idle;
    int exitCode = 0;
    bool hasExitCode = false;
    double elapsedSeconds = 0.0;
    std::string log;
    // Filled only for multi-step jobs (Build All, Cook & Stage). Index is 1-based.
    int sequenceStepIndex = 0;
    int sequenceStepCount = 0;
    std::string sequenceStepLabel;
};

const char* EditorToolJobStateName(EditorToolJobState state);

class EditorToolRunner
{
public:
    bool IsRunning() const;
    EditorToolJobSnapshot Snapshot() const;

    bool TryStart(
        EditorToolKind kind,
        const std::filesystem::path& repositoryRoot,
        bool executionAvailable);

    // Not a user command field. Tests use this to drive HostProcess without
    // invoking CMake or the cooker.
    bool TryStartCommand(const EditorToolCommand& command, bool executionAvailable);

    // Tests drive multi-step jobs (Cook & Stage) with fixture children.
    bool TryStartSequence(
        EditorToolKind kind,
        const std::vector<EditorToolCommand>& steps,
        bool executionAvailable);

    // Synchronous Development feedback (M47 import). Does not launch a process.
    // Fails and leaves state unchanged while a job is running.
    bool ReportLocalResult(EditorToolKind kind, bool succeeded, std::string_view message);

    void Poll();
    void Shutdown();
    void ClearLog();

private:
    bool BeginFailed(EditorToolKind kind, std::string_view message);
    bool LaunchCurrentCommand();
    void FinishProcess(int exitCode);
    void AppendOutput(std::string_view chunk);
    void AppendSequenceStepMarker(int zeroBasedStep);
    void AppendExitCodeLine(int processExitCode);

    platform::HostProcess process;
    std::vector<EditorToolCommand> sequence;
    std::size_t sequenceIndex = 0;
    EditorToolKind kind = EditorToolKind::CookAssets;
    EditorToolJobState state = EditorToolJobState::Idle;
    std::string displayLabel;
    std::string log;
    int exitCode = 0;
    bool hasExitCode = false;
    std::chrono::steady_clock::time_point startTime{};
    double elapsedSeconds = 0.0;
};
}
