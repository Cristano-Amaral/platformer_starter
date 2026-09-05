#pragma once

// Milestone 37/38: canonical cooker, staging, and CMake command descriptions.
// No ImGui. Does not launch processes; EditorToolRunner owns that.

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
    StageRuntimeAssets,
    CookAndStage,
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
inline constexpr std::string_view kCookedAssetsRelative = "game/assets/cooked";
inline constexpr std::string_view kStageRuntimeAssetsScriptRelative =
    "cmake/StageRuntimeAssets.cmake";
inline constexpr std::string_view kRuntimeOutputBinRelative = "bin";
inline constexpr std::string_view kDevelopmentConfigDirectoryName = "Development";
inline constexpr std::string_view kRuntimeAssetsDirectoryName = "assets";
inline constexpr std::string_view kCMakeConfigurePreset = "windows-vs2022";
inline constexpr std::string_view kCMakeDebugBuildPreset = "windows-debug";
inline constexpr std::string_view kCMakeDevelopmentBuildPreset = "windows-development";
inline constexpr std::string_view kCMakeReleaseBuildPreset = "windows-release";
inline constexpr std::string_view kCMakeBinaryDirRelative = "build/windows-vs2022";
inline constexpr int kBuildAllStepCount = 3;
inline constexpr int kCookAndStageStepCount = 2;
inline constexpr std::size_t kEditorToolLogMaxBytes = 256 * 1024;

bool IsEditorToolExecutionAvailable();
bool CanStartEditorToolJob(bool executionAvailable, bool jobRunning);
bool IsDevelopmentSelfBuildBlocked();

std::filesystem::path RepositoryRoot();
bool IsValidRepositoryRoot(const std::filesystem::path& root);
bool IsCMakeBuildTreeConfigured(const std::filesystem::path& root);
bool IsCookedAssetsRoot(const std::filesystem::path& cookedRoot);
bool CanStageRuntimeAssets(const std::filesystem::path& repositoryRoot);

std::filesystem::path CookedAssetsRoot(const std::filesystem::path& repositoryRoot);
std::filesystem::path StageRuntimeAssetsScriptPath(const std::filesystem::path& repositoryRoot);
std::filesystem::path DevelopmentRuntimeAssetsDirectory(
    const std::filesystem::path& repositoryRoot);

EditorToolCommand MakeCookAssetsCommand(const std::filesystem::path& repositoryRoot);
EditorToolCommand MakeStageRuntimeAssetsCommand(const std::filesystem::path& repositoryRoot);
EditorToolCommand MakeBuildDebugCommand(const std::filesystem::path& repositoryRoot);
EditorToolCommand MakeBuildDevelopmentCommand(const std::filesystem::path& repositoryRoot);
EditorToolCommand MakeBuildReleaseCommand(const std::filesystem::path& repositoryRoot);
std::vector<EditorToolCommand> MakeBuildAllPlan(const std::filesystem::path& repositoryRoot);
std::vector<EditorToolCommand> MakeCookAndStagePlan(const std::filesystem::path& repositoryRoot);

const char* EditorToolKindName(EditorToolKind kind);
const char* BuildAllStepLabel(int zeroBasedStep);
const char* CookAndStageStepLabel(int zeroBasedStep);
const char* ToolSequenceStepLabel(EditorToolKind kind, int zeroBasedStep);
int ToolSequenceStepCount(EditorToolKind kind);
bool IsMultiStepEditorToolKind(EditorToolKind kind);

BuildAllAdvanceResult AdvanceBuildAll(int finishedStepIndex, int exitCode, int stepCount);

void AppendBoundedLog(std::string& log, std::string_view chunk, std::size_t maxBytes);
}
