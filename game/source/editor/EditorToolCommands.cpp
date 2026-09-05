#include "editor/EditorToolCommands.h"

#include <string>
#include <system_error>

namespace editor
{
namespace
{
EditorToolCommand MakeBuildPresetCommand(
    EditorToolKind kind,
    const char* displayLabel,
    std::string_view preset,
    const std::filesystem::path& repositoryRoot)
{
    EditorToolCommand command{};
    command.kind = kind;
    command.displayLabel = displayLabel;
    command.executableName = std::string(kCMakeExecutableName);
    command.arguments = {"--build", "--preset", std::string(preset)};
    command.workingDirectory = repositoryRoot;
    return command;
}

bool PathIsRegularFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(path, error) && !error;
}

std::string MakeCMakeDefineArgument(const char* name, const std::filesystem::path& value)
{
    return std::string("-D") + name + "=" + value.lexically_normal().generic_string();
}
}

bool IsEditorToolExecutionAvailable()
{
#if defined(PLATFORMER_ENABLE_EDITOR_TOOLS)
    return true;
#else
    return false;
#endif
}

bool CanStartEditorToolJob(bool executionAvailable, bool jobRunning)
{
    return executionAvailable && !jobRunning;
}

bool IsDevelopmentSelfBuildBlocked()
{
    // Policy A: Build Development stays enabled. Windows may fail with LNK1168
    // if the running Development .exe is locked; Tool Output reports the exit.
    return false;
}

std::filesystem::path RepositoryRoot()
{
#if defined(PLATFORMER_REPOSITORY_ROOT)
    std::filesystem::path root =
        std::filesystem::path{PLATFORMER_REPOSITORY_ROOT}.lexically_normal();
    if (root.empty() || !root.is_absolute())
    {
        return {};
    }
    return root;
#else
    return {};
#endif
}

bool IsValidRepositoryRoot(const std::filesystem::path& root)
{
    if (root.empty() || !root.is_absolute())
    {
        return false;
    }
    return PathIsRegularFile(root / "CMakeLists.txt")
        && PathIsRegularFile(root / "CMakePresets.json")
        && PathIsRegularFile(root / std::string(kCookAssetsScriptRelative));
}

bool IsCMakeBuildTreeConfigured(const std::filesystem::path& root)
{
    if (!IsValidRepositoryRoot(root))
    {
        return false;
    }
    return PathIsRegularFile(root / std::string(kCMakeBinaryDirRelative) / "CMakeCache.txt");
}

std::filesystem::path CookedAssetsRoot(const std::filesystem::path& repositoryRoot)
{
    return (repositoryRoot / std::string(kCookedAssetsRelative)).lexically_normal();
}

std::filesystem::path StageRuntimeAssetsScriptPath(const std::filesystem::path& repositoryRoot)
{
    return (repositoryRoot / std::string(kStageRuntimeAssetsScriptRelative)).lexically_normal();
}

std::filesystem::path DevelopmentRuntimeAssetsDirectory(const std::filesystem::path& repositoryRoot)
{
    return (repositoryRoot / std::string(kCMakeBinaryDirRelative)
                / std::string(kRuntimeOutputBinRelative)
                / std::string(kDevelopmentConfigDirectoryName)
                / std::string(kRuntimeAssetsDirectoryName))
        .lexically_normal();
}

bool IsCookedAssetsRoot(const std::filesystem::path& cookedRoot)
{
    if (cookedRoot.empty() || !cookedRoot.is_absolute())
    {
        return false;
    }
    std::error_code error;
    return std::filesystem::is_directory(cookedRoot, error) && !error;
}

bool CanStageRuntimeAssets(const std::filesystem::path& repositoryRoot)
{
    return IsCMakeBuildTreeConfigured(repositoryRoot)
        && IsCookedAssetsRoot(CookedAssetsRoot(repositoryRoot))
        && PathIsRegularFile(StageRuntimeAssetsScriptPath(repositoryRoot));
}

EditorToolCommand MakeCookAssetsCommand(const std::filesystem::path& repositoryRoot)
{
    EditorToolCommand command{};
    command.kind = EditorToolKind::CookAssets;
    command.displayLabel = "Cook Assets";
    command.executableName = std::string(kPythonExecutableName);
    command.arguments = {std::string(kCookAssetsScriptRelative)};
    command.workingDirectory = repositoryRoot;
    return command;
}

EditorToolCommand MakeStageRuntimeAssetsCommand(const std::filesystem::path& repositoryRoot)
{
    EditorToolCommand command{};
    command.kind = EditorToolKind::StageRuntimeAssets;
    command.displayLabel = "Stage Runtime Assets";
    command.executableName = std::string(kCMakeExecutableName);
    command.arguments = {
        MakeCMakeDefineArgument("PLATFORMER_COOKED_DIR", CookedAssetsRoot(repositoryRoot)),
        MakeCMakeDefineArgument(
            "PLATFORMER_STAGE_DEST", DevelopmentRuntimeAssetsDirectory(repositoryRoot)),
        "-P",
        StageRuntimeAssetsScriptPath(repositoryRoot).generic_string(),
    };
    command.workingDirectory = repositoryRoot;
    return command;
}

EditorToolCommand MakeBuildDebugCommand(const std::filesystem::path& repositoryRoot)
{
    return MakeBuildPresetCommand(
        EditorToolKind::BuildDebug, "Build Debug", kCMakeDebugBuildPreset, repositoryRoot);
}

EditorToolCommand MakeBuildDevelopmentCommand(const std::filesystem::path& repositoryRoot)
{
    return MakeBuildPresetCommand(
        EditorToolKind::BuildDevelopment,
        "Build Development",
        kCMakeDevelopmentBuildPreset,
        repositoryRoot);
}

EditorToolCommand MakeBuildReleaseCommand(const std::filesystem::path& repositoryRoot)
{
    return MakeBuildPresetCommand(
        EditorToolKind::BuildRelease, "Build Release", kCMakeReleaseBuildPreset, repositoryRoot);
}

std::vector<EditorToolCommand> MakeBuildAllPlan(const std::filesystem::path& repositoryRoot)
{
    return {
        MakeBuildDebugCommand(repositoryRoot),
        MakeBuildDevelopmentCommand(repositoryRoot),
        MakeBuildReleaseCommand(repositoryRoot),
    };
}

std::vector<EditorToolCommand> MakeCookAndStagePlan(const std::filesystem::path& repositoryRoot)
{
    return {
        MakeCookAssetsCommand(repositoryRoot),
        MakeStageRuntimeAssetsCommand(repositoryRoot),
    };
}

const char* EditorToolKindName(EditorToolKind kind)
{
    switch (kind)
    {
    case EditorToolKind::CookAssets:
        return "Cook Assets";
    case EditorToolKind::StageRuntimeAssets:
        return "Stage Runtime Assets";
    case EditorToolKind::CookAndStage:
        return "Cook & Stage";
    case EditorToolKind::BuildDebug:
        return "Build Debug";
    case EditorToolKind::BuildDevelopment:
        return "Build Development";
    case EditorToolKind::BuildRelease:
        return "Build Release";
    case EditorToolKind::BuildAll:
        return "Build All";
    }
    return "Cook Assets";
}

const char* BuildAllStepLabel(int zeroBasedStep)
{
    switch (zeroBasedStep)
    {
    case 0:
        return "Debug";
    case 1:
        return "Development";
    case 2:
        return "Release";
    default:
        return "";
    }
}

const char* CookAndStageStepLabel(int zeroBasedStep)
{
    switch (zeroBasedStep)
    {
    case 0:
        return "Cook Assets";
    case 1:
        return "Stage Runtime Assets";
    default:
        return "";
    }
}

const char* ToolSequenceStepLabel(EditorToolKind kind, int zeroBasedStep)
{
    if (kind == EditorToolKind::CookAndStage)
    {
        return CookAndStageStepLabel(zeroBasedStep);
    }
    return BuildAllStepLabel(zeroBasedStep);
}

int ToolSequenceStepCount(EditorToolKind kind)
{
    if (kind == EditorToolKind::CookAndStage)
    {
        return kCookAndStageStepCount;
    }
    if (kind == EditorToolKind::BuildAll)
    {
        return kBuildAllStepCount;
    }
    return 1;
}

bool IsMultiStepEditorToolKind(EditorToolKind kind)
{
    return kind == EditorToolKind::BuildAll || kind == EditorToolKind::CookAndStage;
}

BuildAllAdvanceResult AdvanceBuildAll(int finishedStepIndex, int exitCode, int stepCount)
{
    if (exitCode != 0)
    {
        return BuildAllAdvanceResult::Failed;
    }
    if (finishedStepIndex < 0 || finishedStepIndex + 1 >= stepCount)
    {
        return BuildAllAdvanceResult::Succeeded;
    }
    return BuildAllAdvanceResult::Continue;
}

void AppendBoundedLog(std::string& log, std::string_view chunk, std::size_t maxBytes)
{
    if (chunk.empty() || maxBytes == 0)
    {
        return;
    }

    log.append(chunk.data(), chunk.size());
    if (log.size() <= maxBytes)
    {
        return;
    }

    constexpr std::string_view kMarker = "...[truncated]...\n";
    const std::size_t keep = maxBytes > kMarker.size() ? maxBytes - kMarker.size() : 0;
    if (keep == 0)
    {
        log.assign(kMarker.substr(0, maxBytes));
        return;
    }
    log = std::string(kMarker) + log.substr(log.size() - keep);
    if (log.size() > maxBytes)
    {
        log.resize(maxBytes);
    }
}
}
