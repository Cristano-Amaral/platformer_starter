#pragma once

// Milestone 37 Phase A: canonical cooker/CMake command descriptions, repository
// root validation, Build All sequencing, and execution policy. No ImGui.
// Does not launch processes; EditorToolRunner owns that.

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace editor
{
enum class EditorToolKind
{
    CookAssets,
    BuildDebug,
    BuildDevelopment,
    BuildRelease,
    BuildAll,
};

enum class EditorSelfBuildPolicy
{
    AllowAndReport,
};

enum class BuildAllAdvanceResult
{
    Continue,
    Succeeded,
    Failed,
};

struct EditorToolCommand
{
    EditorToolKind kind = EditorToolKind::CookAssets;
    std::string displayLabel;
    std::string executableName;
    std::vector<std::string> arguments;
    std::filesystem::path workingDirectory;
};

inline constexpr EditorSelfBuildPolicy kEditorSelfBuildPolicy =
    EditorSelfBuildPolicy::AllowAndReport;

inline constexpr std::string_view kPythonExecutableName = "python";
inline constexpr std::string_view kCMakeExecutableName = "cmake";
inline constexpr std::string_view kCookAssetsScriptRelative = "tools/cook_assets.py";
inline constexpr std::string_view kCMakeConfigurePreset = "windows-vs2022";
inline constexpr std::string_view kCMakeDebugBuildPreset = "windows-debug";
inline constexpr std::string_view kCMakeDevelopmentBuildPreset = "windows-development";
inline constexpr std::string_view kCMakeReleaseBuildPreset = "windows-release";
inline constexpr std::string_view kCMakeBinaryDirRelative = "build/windows-vs2022";
inline constexpr int kBuildAllStepCount = 3;
inline constexpr std::size_t kEditorToolLogMaxBytes = 256 * 1024;

bool IsEditorToolExecutionAvailable();
bool CanStartEditorToolJob(bool executionAvailable, bool jobRunning);
bool IsDevelopmentSelfBuildBlocked();

std::filesystem::path RepositoryRoot();
bool IsValidRepositoryRoot(const std::filesystem::path& root);
bool IsCMakeBuildTreeConfigured(const std::filesystem::path& root);

EditorToolCommand MakeCookAssetsCommand(const std::filesystem::path& repositoryRoot);
EditorToolCommand MakeBuildDebugCommand(const std::filesystem::path& repositoryRoot);
EditorToolCommand MakeBuildDevelopmentCommand(const std::filesystem::path& repositoryRoot);
EditorToolCommand MakeBuildReleaseCommand(const std::filesystem::path& repositoryRoot);
std::vector<EditorToolCommand> MakeBuildAllPlan(const std::filesystem::path& repositoryRoot);

const char* EditorToolKindName(EditorToolKind kind);
const char* BuildAllStepLabel(int zeroBasedStep);

BuildAllAdvanceResult AdvanceBuildAll(int finishedStepIndex, int exitCode, int stepCount);

void AppendBoundedLog(std::string& log, std::string_view chunk, std::size_t maxBytes);
}
