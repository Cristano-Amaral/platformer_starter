#include "editor/EditorToolCommands.h"
#include "editor/EditorToolRunner.h"
#include "platform/HostProcess.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name.c_str());
        ++gFailures;
    }
}

std::filesystem::path TestRepoRoot()
{
#if defined(PLATFORMER_REPOSITORY_ROOT)
    return std::filesystem::path{PLATFORMER_REPOSITORY_ROOT}.lexically_normal();
#else
    return {};
#endif
}

bool WaitForProcess(platform::HostProcess& process, int timeoutMilliseconds)
{
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMilliseconds);
    while (std::chrono::steady_clock::now() < deadline)
    {
        process.Poll();
        if (process.Status() != platform::HostProcessStatus::Running)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    process.Poll();
    return process.Status() != platform::HostProcessStatus::Running;
}

bool WaitForRunner(editor::EditorToolRunner& runner, int timeoutMilliseconds)
{
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMilliseconds);
    while (std::chrono::steady_clock::now() < deadline)
    {
        runner.Poll();
        if (!runner.IsRunning())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    runner.Poll();
    return !runner.IsRunning();
}

editor::EditorToolCommand MakeCmdCommand(const std::string& script, const char* label)
{
    editor::EditorToolCommand command{};
    command.kind = editor::EditorToolKind::CookAssets;
    command.displayLabel = label;
    command.executableName = "cmd";
    command.arguments = {"/C", script};
    command.workingDirectory = std::filesystem::temp_directory_path();
    return command;
}
}

int main()
{
    using editor::EditorToolKind;
    using editor::EditorToolJobState;

    {
        Expect(editor::EditorToolJobState::Idle == EditorToolJobState::Idle, "Idle exists");
        editor::EditorToolRunner runner;
        Expect(!runner.IsRunning(), "initial runner is idle");
        Expect(runner.Snapshot().state == EditorToolJobState::Idle, "snapshot starts Idle");
        Expect(std::string(editor::EditorToolJobStateName(EditorToolJobState::Idle)) == "Idle", "Idle name");
        Expect(std::string(editor::EditorToolJobStateName(EditorToolJobState::Running)) == "Running", "Running name");
        Expect(std::string(editor::EditorToolJobStateName(EditorToolJobState::Succeeded)) == "Succeeded", "Succeeded name");
        Expect(std::string(editor::EditorToolJobStateName(EditorToolJobState::Failed)) == "Failed", "Failed name");
        Expect(runner.Snapshot().log.empty(), "initial log empty");
    }

    {
        Expect(!editor::CanStartEditorToolJob(false, false), "Debug/Release: execution off");
        Expect(!editor::CanStartEditorToolJob(true, true), "one-active-job guard");
        Expect(editor::CanStartEditorToolJob(true, false), "Development idle can start");
        Expect(
            !editor::IsEditorToolExecutionAvailable(),
            "test target does not compile live editor-tool execution");
        Expect(!editor::IsDevelopmentSelfBuildBlocked(), "Policy A never blocks self-build");
        Expect(
            editor::kEditorSelfBuildPolicy == editor::EditorSelfBuildPolicy::AllowAndReport,
            "self-build policy is AllowAndReport");
    }

    {
        Expect(!editor::IsValidRepositoryRoot({}), "empty root invalid");
        Expect(!editor::IsValidRepositoryRoot(std::filesystem::path{"relative"}), "relative root invalid");
        const std::filesystem::path root = TestRepoRoot();
        Expect(!root.empty() && root.is_absolute(), "test injects absolute repository root");
        Expect(editor::IsValidRepositoryRoot(root), "real repository root validates");
        Expect(
            editor::RepositoryRoot().empty() || editor::RepositoryRoot() == root,
            "RepositoryRoot empty unless compiled into the game");
        Expect(editor::IsCMakeBuildTreeConfigured(root), "configured CMakeCache.txt present");
    }

    {
        const std::filesystem::path root = TestRepoRoot();
        const editor::EditorToolCommand cook = editor::MakeCookAssetsCommand(root);
        Expect(cook.executableName == editor::kPythonExecutableName, "cook uses python");
        Expect(
            cook.arguments.size() == 1
                && cook.arguments[0] == std::string(editor::kCookAssetsScriptRelative),
            "cook argument is tools/cook_assets.py");
        Expect(cook.workingDirectory == root, "cook cwd is repository root");
        Expect(cook.displayLabel == "Cook Assets", "cook display name");

        const editor::EditorToolCommand debugBuild = editor::MakeBuildDebugCommand(root);
        Expect(debugBuild.executableName == editor::kCMakeExecutableName, "build uses cmake");
        Expect(
            debugBuild.arguments.size() == 3 && debugBuild.arguments[0] == "--build"
                && debugBuild.arguments[1] == "--preset"
                && debugBuild.arguments[2] == std::string(editor::kCMakeDebugBuildPreset),
            "Build Debug preset");
        Expect(debugBuild.workingDirectory == root, "Build Debug cwd is repository root");

        const editor::EditorToolCommand developmentBuild =
            editor::MakeBuildDevelopmentCommand(root);
        Expect(
            developmentBuild.arguments[2] == std::string(editor::kCMakeDevelopmentBuildPreset),
            "Build Development preset");
        const editor::EditorToolCommand releaseBuild = editor::MakeBuildReleaseCommand(root);
        Expect(
            releaseBuild.arguments[2] == std::string(editor::kCMakeReleaseBuildPreset),
            "Build Release preset");

        const std::vector<editor::EditorToolCommand> plan = editor::MakeBuildAllPlan(root);
        Expect(plan.size() == 3, "Build All has three steps");
        Expect(plan[0].arguments[2] == std::string(editor::kCMakeDebugBuildPreset), "All step 1 Debug");
        Expect(
            plan[1].arguments[2] == std::string(editor::kCMakeDevelopmentBuildPreset),
            "All step 2 Development");
        Expect(
            plan[2].arguments[2] == std::string(editor::kCMakeReleaseBuildPreset),
            "All step 3 Release");
        Expect(
            std::string(editor::BuildAllStepLabel(0)) == "Debug"
                && std::string(editor::BuildAllStepLabel(1)) == "Development"
                && std::string(editor::BuildAllStepLabel(2)) == "Release",
            "Build All step labels");

        const editor::EditorToolCommand stage = editor::MakeStageRuntimeAssetsCommand(root);
        Expect(stage.executableName == editor::kCMakeExecutableName, "stage uses cmake");
        Expect(stage.workingDirectory == root, "stage cwd is repository root");
        Expect(stage.kind == EditorToolKind::StageRuntimeAssets, "stage kind");
        Expect(stage.displayLabel == "Stage Runtime Assets", "stage display name");
        bool stageHasBuild = false;
        bool stageHasScript = false;
        bool stageHasCookedDefine = false;
        bool stageHasDestDefine = false;
        bool stageHasDashP = false;
        for (const std::string& argument : stage.arguments)
        {
            if (argument == "--build")
            {
                stageHasBuild = true;
            }
            if (argument == "-P")
            {
                stageHasDashP = true;
            }
            if (argument.find("StageRuntimeAssets.cmake") != std::string::npos)
            {
                stageHasScript = true;
            }
            if (argument.find("-DPLATFORMER_COOKED_DIR=") == 0)
            {
                stageHasCookedDefine = true;
            }
            if (argument.find("-DPLATFORMER_STAGE_DEST=") == 0)
            {
                stageHasDestDefine = true;
            }
        }
        Expect(!stageHasBuild, "Stage Runtime Assets does not pass --build");
        Expect(stageHasDashP && stageHasScript, "stage invokes cmake -P script");
        Expect(stageHasCookedDefine && stageHasDestDefine, "stage passes cooked and dest defines");
        Expect(
            editor::DevelopmentRuntimeAssetsDirectory(root)
                == (root / "build/windows-vs2022/bin/Development/assets").lexically_normal(),
            "Development dest is bin/Development/assets");
        Expect(editor::CanStageRuntimeAssets(root), "real repo can stage");

        const std::vector<editor::EditorToolCommand> cookAndStage =
            editor::MakeCookAndStagePlan(root);
        Expect(cookAndStage.size() == 2, "Cook & Stage has two steps");
        Expect(cookAndStage[0].kind == EditorToolKind::CookAssets, "Cook & Stage step 1 cook");
        Expect(
            cookAndStage[1].kind == EditorToolKind::StageRuntimeAssets,
            "Cook & Stage step 2 stage");
        bool cookAndStageBuild = false;
        for (const std::string& argument : cookAndStage[1].arguments)
        {
            if (argument == "--build")
            {
                cookAndStageBuild = true;
            }
        }
        Expect(!cookAndStageBuild, "Cook & Stage stage step does not build");
        Expect(
            std::string(editor::CookAndStageStepLabel(0)) == "Cook Assets"
                && std::string(editor::CookAndStageStepLabel(1)) == "Stage Runtime Assets",
            "Cook & Stage step labels");
        Expect(editor::ToolSequenceStepCount(EditorToolKind::CookAndStage) == 2, "Cook & Stage count");
        Expect(editor::IsMultiStepEditorToolKind(EditorToolKind::CookAndStage), "Cook & Stage sequenced");
        Expect(
            !editor::IsMultiStepEditorToolKind(EditorToolKind::StageRuntimeAssets),
            "Stage alone is one step");
    }

    {
        Expect(
            editor::AdvanceBuildAll(0, 0, 3) == editor::BuildAllAdvanceResult::Continue,
            "Build All continues after Debug success");
        Expect(
            editor::AdvanceBuildAll(1, 0, 3) == editor::BuildAllAdvanceResult::Continue,
            "Build All continues after Development success");
        Expect(
            editor::AdvanceBuildAll(2, 0, 3) == editor::BuildAllAdvanceResult::Succeeded,
            "Build All succeeds after Release");
        Expect(
            editor::AdvanceBuildAll(0, 1, 3) == editor::BuildAllAdvanceResult::Failed,
            "Build All stops when Debug fails");
        Expect(
            editor::AdvanceBuildAll(1, 2, 3) == editor::BuildAllAdvanceResult::Failed,
            "Build All stops when Development fails");
        Expect(
            editor::AdvanceBuildAll(0, 0, 2) == editor::BuildAllAdvanceResult::Continue,
            "Cook & Stage continues after cook success");
        Expect(
            editor::AdvanceBuildAll(1, 0, 2) == editor::BuildAllAdvanceResult::Succeeded,
            "Cook & Stage succeeds after stage success");
        Expect(
            editor::AdvanceBuildAll(0, 1, 2) == editor::BuildAllAdvanceResult::Failed,
            "Cook & Stage stops when cook fails");
        Expect(
            editor::AdvanceBuildAll(1, 4, 2) == editor::BuildAllAdvanceResult::Failed,
            "Cook & Stage fails when stage fails");
    }

    {
        std::string log;
        editor::AppendBoundedLog(log, "hello", 256);
        Expect(log == "hello", "log append");
        log.clear();
        const std::string chunk(200, 'a');
        editor::AppendBoundedLog(log, chunk, 64);
        Expect(log.size() == 64, "bounded log respects max bytes");
        Expect(log.find("...[truncated]...") == 0, "truncation marker at front");
        Expect(log.find('a') != std::string::npos, "newest bytes kept");
    }

#if defined(_WIN32)
    {
        const std::filesystem::path cmd = platform::FindHostExecutable("cmd");
        Expect(!cmd.empty(), "cmd.exe resolved on PATH");
        Expect(
            platform::QuoteHostProcessArgument(L"hello") == L"hello",
            "unquoted argument without spaces");
        const std::wstring quoted = platform::QuoteHostProcessArgument(L"hello world");
        Expect(quoted.front() == L'"' && quoted.back() == L'"', "spaces are quoted");
    }

    {
        platform::HostProcess process;
        platform::HostProcessSpec spec{};
        spec.executable = platform::FindHostExecutable("cmd");
        spec.arguments = {"/C", "echo STDOUT_MARKER"};
        spec.workingDirectory = std::filesystem::temp_directory_path();
        Expect(process.Start(spec), "stdout process starts");
        Expect(WaitForProcess(process, 8000), "stdout process exits");
        const std::string output = process.DrainOutput();
        Expect(process.HasExitCode() && process.ExitCode() == 0, "stdout exit 0");
        Expect(output.find("STDOUT_MARKER") != std::string::npos, "stdout captured");
    }

    {
        platform::HostProcess process;
        platform::HostProcessSpec spec{};
        spec.executable = platform::FindHostExecutable("cmd");
        spec.arguments = {"/C", "echo STDERR_MARKER 1>&2"};
        spec.workingDirectory = std::filesystem::temp_directory_path();
        Expect(process.Start(spec), "stderr process starts");
        Expect(WaitForProcess(process, 8000), "stderr process exits");
        const std::string output = process.DrainOutput();
        Expect(process.ExitCode() == 0, "stderr redirect exit 0");
        Expect(output.find("STDERR_MARKER") != std::string::npos, "stderr merged into log");
    }

    {
        platform::HostProcess process;
        platform::HostProcessSpec spec{};
        spec.executable = platform::FindHostExecutable("cmd");
        spec.arguments = {"/C", "exit /B 7"};
        spec.workingDirectory = std::filesystem::temp_directory_path();
        Expect(process.Start(spec), "failure process starts");
        Expect(WaitForProcess(process, 8000), "failure process exits");
        Expect(process.HasExitCode() && process.ExitCode() == 7, "non-zero exit preserved");
    }

    {
        platform::HostProcess process;
        platform::HostProcessSpec spec{};
        spec.executable = platform::FindHostExecutable("cmd");
        spec.arguments = {"/C", "echo STEP1& ping 127.0.0.1 -n 2 >nul & echo STEP2"};
        spec.workingDirectory = std::filesystem::temp_directory_path();
        Expect(process.Start(spec), "incremental process starts");
        std::string output;
        bool sawFirst = false;
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(8000);
        while (std::chrono::steady_clock::now() < deadline)
        {
            process.Poll();
            output += process.DrainOutput();
            if (output.find("STEP1") != std::string::npos)
            {
                sawFirst = true;
            }
            if (process.Status() != platform::HostProcessStatus::Running)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        process.Poll();
        output += process.DrainOutput();
        Expect(sawFirst, "incremental capture saw STEP1 before or at exit");
        Expect(output.find("STEP2") != std::string::npos, "incremental capture saw STEP2");
        Expect(process.ExitCode() == 0, "incremental process exit 0");
    }

    {
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStart(EditorToolKind::CookAssets, {}, true),
            "invalid root still consumes the start request");
        Expect(runner.Snapshot().state == EditorToolJobState::Failed, "invalid root fails safely");
        Expect(
            runner.Snapshot().log.find("repository root is invalid") != std::string::npos,
            "invalid root explains the failure");
        Expect(!runner.IsRunning(), "invalid root does not leave Running");
    }

    {
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStart(EditorToolKind::CookAssets, TestRepoRoot(), false),
            "disabled execution records a failed job");
        Expect(runner.Snapshot().state == EditorToolJobState::Failed, "Debug execution path fails");
        Expect(
            runner.Snapshot().log.find("unavailable") != std::string::npos,
            "disabled execution explains the configuration");
    }

    {
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStartCommand(MakeCmdCommand("echo STDOUT_MARKER", "stdout-job"), true),
            "runner launches cmd stdout");
        Expect(runner.IsRunning() || runner.Snapshot().state != EditorToolJobState::Idle,
            "runner leaves Idle after start");
        Expect(WaitForRunner(runner, 8000), "runner stdout job completes");
        Expect(runner.Snapshot().state == EditorToolJobState::Succeeded, "success exit -> Succeeded");
        Expect(runner.Snapshot().hasExitCode && runner.Snapshot().exitCode == 0, "success exit code 0");
        Expect(
            runner.Snapshot().log.find("STDOUT_MARKER") != std::string::npos,
            "runner stdout in Tool Output log");
        const int exitBeforeClear = runner.Snapshot().exitCode;
        runner.ClearLog();
        Expect(runner.Snapshot().log.empty(), "ClearLog empties retained output");
        Expect(runner.Snapshot().state == EditorToolJobState::Succeeded, "ClearLog leaves Succeeded");
        Expect(runner.Snapshot().exitCode == exitBeforeClear, "ClearLog leaves exit code");
    }

    {
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStartCommand(MakeCmdCommand("echo STDERR_MARKER 1>&2", "stderr-job"), true),
            "runner launches cmd stderr");
        Expect(WaitForRunner(runner, 8000), "runner stderr job completes");
        Expect(
            runner.Snapshot().log.find("STDERR_MARKER") != std::string::npos,
            "runner stderr in Tool Output log");
        Expect(runner.Snapshot().state == EditorToolJobState::Succeeded, "stderr job succeeded");
    }

    {
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStartCommand(MakeCmdCommand("exit /B 9", "fail-job"), true),
            "runner launches failing cmd");
        Expect(WaitForRunner(runner, 8000), "runner fail job completes");
        Expect(runner.Snapshot().state == EditorToolJobState::Failed, "non-zero -> Failed");
        Expect(runner.Snapshot().hasExitCode && runner.Snapshot().exitCode == 9, "failure exit code 9");
    }

    {
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStartCommand(
                MakeCmdCommand("ping 127.0.0.1 -n 4 >nul", "long-job"), true),
            "long job starts");
        runner.Poll();
        Expect(runner.IsRunning(), "long job is Running");
        const std::string logWhileRunning = runner.Snapshot().log;
        runner.ClearLog();
        Expect(runner.Snapshot().log == logWhileRunning, "ClearLog ignored while Running");
        Expect(
            !runner.TryStartCommand(MakeCmdCommand("echo second", "second-job"), true),
            "second job rejected while Running");
        Expect(runner.Snapshot().displayLabel == "long-job", "rejected start leaves first job");
        runner.Shutdown();
        Expect(!runner.IsRunning(), "shutdown ends Running");
        Expect(runner.Snapshot().state == EditorToolJobState::Failed, "shutdown marks Failed");
        Expect(
            runner.Snapshot().log.find("shutting down") != std::string::npos,
            "shutdown writes a log line");
    }

    {
        const std::filesystem::path spaced =
            std::filesystem::temp_directory_path() / "m37 tool root";
        std::filesystem::create_directories(spaced);
        platform::HostProcess process;
        platform::HostProcessSpec spec{};
        spec.executable = platform::FindHostExecutable("cmd");
        spec.arguments = {"/C", "echo SPACE_CWD_OK"};
        spec.workingDirectory = spaced;
        Expect(process.Start(spec), "process starts with spaced working directory");
        Expect(WaitForProcess(process, 8000), "spaced cwd process exits");
        Expect(process.DrainOutput().find("SPACE_CWD_OK") != std::string::npos, "spaced cwd works");
        std::error_code error;
        std::filesystem::remove_all(spaced, error);
    }

    {
        editor::EditorToolCommand cookStep = MakeCmdCommand("echo COOK_OK", "Cook Assets");
        editor::EditorToolCommand stageStep = MakeCmdCommand("echo STAGE_OK", "Stage Runtime Assets");
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStartSequence(
                EditorToolKind::CookAndStage, {cookStep, stageStep}, true),
            "Cook & Stage fixture sequence starts");
        Expect(WaitForRunner(runner, 12000), "Cook & Stage fixture sequence completes");
        Expect(runner.Snapshot().state == EditorToolJobState::Succeeded, "Cook & Stage success");
        Expect(runner.Snapshot().hasExitCode && runner.Snapshot().exitCode == 0, "Cook & Stage exit 0");
        const std::string log = runner.Snapshot().log;
        Expect(log.find("=== Cook & Stage: Step 1/2 - Cook Assets ===") != std::string::npos,
            "Cook & Stage step 1 marker");
        Expect(log.find("=== Cook & Stage: Step 2/2 - Stage Runtime Assets ===") != std::string::npos,
            "Cook & Stage step 2 marker");
        Expect(log.find("COOK_OK") != std::string::npos, "Cook & Stage ran cook");
        Expect(log.find("STAGE_OK") != std::string::npos, "Cook & Stage ran stage");
        Expect(log.find("Cook & Stage succeeded.") != std::string::npos, "Cook & Stage success line");
        Expect(runner.Snapshot().displayLabel == "Cook & Stage", "Cook & Stage job label");
        Expect(runner.Snapshot().sequenceStepCount == 2, "Cook & Stage snapshot step count");
        Expect(runner.Snapshot().sequenceStepIndex == 2, "Cook & Stage ends on step 2");
        Expect(
            runner.Snapshot().sequenceStepLabel == "Stage Runtime Assets",
            "Cook & Stage final step label");
        Expect(
            runner.Snapshot().displayLabel.find("Build All") == std::string::npos,
            "Cook & Stage is not labeled Build All");
    }

    {
        editor::EditorToolCommand cookStep = MakeCmdCommand("exit /B 3", "Cook Assets");
        editor::EditorToolCommand stageStep = MakeCmdCommand("echo SHOULD_NOT_STAGE", "Stage Runtime Assets");
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStartSequence(
                EditorToolKind::CookAndStage, {cookStep, stageStep}, true),
            "Cook-fail sequence starts");
        Expect(WaitForRunner(runner, 8000), "Cook-fail sequence completes");
        Expect(runner.Snapshot().state == EditorToolJobState::Failed, "cook failure -> Failed");
        Expect(runner.Snapshot().exitCode == 3, "cook failure keeps cook exit code");
        const std::string log = runner.Snapshot().log;
        Expect(log.find("=== Cook & Stage: Step 1/2 - Cook Assets ===") != std::string::npos,
            "failed cook still logs step 1");
        Expect(log.find("=== Cook & Stage: Step 2/2") == std::string::npos, "stage step marker absent");
        Expect(log.find("SHOULD_NOT_STAGE") == std::string::npos, "stage command not launched");
        Expect(
            log.find("Cook & Stage failed at Cook Assets") != std::string::npos,
            "cook-failure stop reason");
        Expect(runner.Snapshot().displayLabel == "Cook & Stage", "cook-fail keeps Cook & Stage label");
        Expect(runner.Snapshot().sequenceStepCount == 2, "cook-fail still reports two-step job");
        Expect(runner.Snapshot().sequenceStepIndex == 1, "cook-fail stays on step 1");
        Expect(runner.Snapshot().sequenceStepLabel == "Cook Assets", "cook-fail step label");
    }

    {
        editor::EditorToolCommand cookStep = MakeCmdCommand("echo COOK_OK", "Cook Assets");
        editor::EditorToolCommand stageStep = MakeCmdCommand("exit /B 4", "Stage Runtime Assets");
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStartSequence(
                EditorToolKind::CookAndStage, {cookStep, stageStep}, true),
            "Stage-fail sequence starts");
        Expect(WaitForRunner(runner, 8000), "Stage-fail sequence completes");
        Expect(runner.Snapshot().state == EditorToolJobState::Failed, "stage failure -> Failed");
        Expect(runner.Snapshot().exitCode == 4, "stage failure keeps stage exit code");
        const std::string log = runner.Snapshot().log;
        Expect(log.find("COOK_OK") != std::string::npos, "cook ran before stage failure");
        Expect(log.find("=== Cook & Stage: Step 2/2 - Stage Runtime Assets ===") != std::string::npos,
            "stage step marker present");
        Expect(
            log.find("Cook & Stage failed at Stage Runtime Assets") != std::string::npos,
            "stage-failure stop reason");
        Expect(runner.Snapshot().displayLabel == "Cook & Stage", "stage-fail keeps Cook & Stage label");
        Expect(runner.Snapshot().sequenceStepIndex == 2, "stage-fail stays on step 2");
        Expect(
            runner.Snapshot().sequenceStepLabel == "Stage Runtime Assets",
            "stage-fail step label");
    }

    {
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStart(EditorToolKind::StageRuntimeAssets, TestRepoRoot(), true),
            "live Stage Runtime Assets start request");
        Expect(WaitForRunner(runner, 20000), "live Stage Runtime Assets completes");
        Expect(
            runner.Snapshot().state == EditorToolJobState::Succeeded,
            "repo Stage Runtime Assets succeeded");
        Expect(runner.Snapshot().exitCode == 0, "repo stage exit 0");
        const std::string log = runner.Snapshot().log;
        Expect(log.find("--build") == std::string::npos, "live stage log has no --build");
        Expect(log.find("destination:") != std::string::npos, "stage log names destination");
        Expect(log.find("Staging complete") != std::string::npos, "stage log reports completion");
        Expect(log.find("cook_assets.py") == std::string::npos, "Stage does not invoke cooker");
        Expect(runner.Snapshot().displayLabel == "Stage Runtime Assets", "Stage job label");
        Expect(runner.Snapshot().sequenceStepCount == 0, "Stage is not a multi-step snapshot");
    }

    {
        editor::EditorToolCommand debugStep = MakeCmdCommand("echo DBG_OK", "Debug");
        editor::EditorToolCommand developmentStep = MakeCmdCommand("echo DEV_OK", "Development");
        editor::EditorToolCommand releaseStep = MakeCmdCommand("echo REL_OK", "Release");
        editor::EditorToolRunner runner;
        Expect(
            runner.TryStartSequence(
                EditorToolKind::BuildAll, {debugStep, developmentStep, releaseStep}, true),
            "Build All fixture sequence starts");
        Expect(WaitForRunner(runner, 12000), "Build All fixture sequence completes");
        Expect(runner.Snapshot().state == EditorToolJobState::Succeeded, "Build All success");
        Expect(runner.Snapshot().displayLabel == "Build All", "Build All job label");
        Expect(runner.Snapshot().sequenceStepCount == 3, "Build All snapshot step count");
        Expect(runner.Snapshot().sequenceStepIndex == 3, "Build All ends on step 3");
        Expect(runner.Snapshot().sequenceStepLabel == "Release", "Build All final step label");
        const std::string log = runner.Snapshot().log;
        Expect(log.find("=== Build All: Step 1/3 - Debug ===") != std::string::npos,
            "Build All step 1 marker");
        Expect(log.find("=== Build All: Step 2/3 - Development ===") != std::string::npos,
            "Build All step 2 marker");
        Expect(log.find("=== Build All: Step 3/3 - Release ===") != std::string::npos,
            "Build All step 3 marker");
        Expect(log.find("Build All succeeded.") != std::string::npos, "Build All success line");
    }
#endif

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor tool runner test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor tool runner tests passed.\n");
    return 0;
}
