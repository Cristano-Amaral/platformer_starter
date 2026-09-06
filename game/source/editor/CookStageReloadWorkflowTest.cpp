#include "editor/CookStageReloadWorkflow.h"
#include "editor/EditorWorkspace.h"

#include <cstdio>
#include <string>

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
}

int main()
{
    using editor::CookStageReloadState;
    using editor::CookStageReloadStatus;
    using editor::CookStageReloadWorkflow;
    using editor::EditorToolJobState;
    using editor::EditorToolKind;

    {
        Expect(
            !editor::CanStartCookStageReload(false, false, false, false),
            "start rejected when authoring unavailable");
        Expect(
            !editor::CanStartCookStageReload(true, true, false, false),
            "start rejected while Modified");
        Expect(
            !editor::CanStartCookStageReload(true, false, true, false),
            "start rejected while runner is Running");
        Expect(
            !editor::CanStartCookStageReload(true, false, false, true),
            "start rejected while workflow pending");
        Expect(
            editor::CanStartCookStageReload(true, false, false, false),
            "start accepted when authoring, unmodified, idle, not pending");
        Expect(
            !editor::CanReloadRuntimeLevel(true, false, false, true),
            "manual Reload blocked while convenience workflow is pending");
        Expect(
            editor::CanReloadRuntimeLevel(true, false, false, false),
            "manual Reload still enabled when workflow is idle");
    }

    {
        CookStageReloadWorkflow workflow;
        Expect(workflow.State() == CookStageReloadState::Idle, "initial Idle");
        Expect(!workflow.IsPending(), "initial not pending");
        Expect(workflow.BeginWaiting(), "start accepted BeginWaiting");
        Expect(
            workflow.State() == CookStageReloadState::WaitingForCookAndStage,
            "waiting after begin");
        Expect(workflow.IsPending(), "pending while waiting");
        Expect(!workflow.BeginWaiting(), "second begin rejected while pending");
    }

    {
        CookStageReloadWorkflow workflow;
        Expect(workflow.BeginWaiting(), "exactly-once: begin");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Running);
        Expect(!workflow.TakeReloadRequest(), "running produces zero reload");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Succeeded);
        Expect(
            workflow.State() == CookStageReloadState::ReloadPending,
            "external success arms ReloadPending");
        Expect(workflow.TakeReloadRequest(), "exactly one reload request");
        Expect(!workflow.TakeReloadRequest(), "second take is false");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Succeeded);
        Expect(!workflow.TakeReloadRequest(), "repeated-frame Succeeded does not re-arm");
        workflow.NotifyReloadFinished(true);
        Expect(
            workflow.LastStatus() == CookStageReloadStatus::Completed,
            "reload success status");
        Expect(workflow.State() == CookStageReloadState::Idle, "idle after consume");
    }

    {
        CookStageReloadWorkflow workflow;
        Expect(workflow.BeginWaiting(), "failure-stop: begin");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Failed);
        Expect(!workflow.TakeReloadRequest(), "CookAndStage Failed: zero reload");
        Expect(!workflow.IsPending(), "failed workflow is not pending");
        Expect(
            workflow.LastStatus() == CookStageReloadStatus::ExternalFailed,
            "external failure status");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Failed);
        Expect(!workflow.TakeReloadRequest(), "later Failed frames still zero reload");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Succeeded);
        Expect(
            !workflow.TakeReloadRequest(),
            "stale Succeeded after failure does not reload");
    }

    {
        CookStageReloadWorkflow workflow;
        Expect(workflow.State() == CookStageReloadState::Idle, "start failure never begins");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Succeeded);
        Expect(!workflow.TakeReloadRequest(), "start failure: no future reload");
        Expect(!workflow.IsPending(), "start failure leaves Idle");
    }

    {
        CookStageReloadWorkflow workflow;
        Expect(workflow.BeginWaiting(), "reload failure: begin");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Succeeded);
        Expect(workflow.TakeReloadRequest(), "reload failure still consumes once");
        workflow.NotifyReloadFinished(false);
        Expect(
            workflow.LastStatus() == CookStageReloadStatus::ReloadFailed,
            "reload failure status");
        Expect(!workflow.TakeReloadRequest(), "no retry after reload failure");
        Expect(workflow.BeginWaiting(), "ReloadFailed still allows a later new attempt");
        workflow.Cancel();
    }

    {
        CookStageReloadWorkflow workflow;
        Expect(workflow.BeginWaiting(), "Completed recovery: begin");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Succeeded);
        Expect(workflow.TakeReloadRequest(), "Completed recovery: consume");
        workflow.NotifyReloadFinished(true);
        Expect(workflow.BeginWaiting(), "Completed still allows a later new attempt");
        workflow.Cancel();
    }

    {
        CookStageReloadWorkflow workflow;
        Expect(workflow.BeginWaiting(), "ExternalFailed recovery: begin");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Failed);
        Expect(workflow.BeginWaiting(), "ExternalFailed still allows a later new attempt");
        workflow.Cancel();
    }

    {
        CookStageReloadWorkflow workflow;
        Expect(workflow.BeginWaiting(), "shutdown cancel: begin");
        workflow.Cancel();
        Expect(!workflow.IsPending(), "cancel clears pending");
        Expect(
            workflow.LastStatus() == CookStageReloadStatus::Cancelled, "cancelled status");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Succeeded);
        Expect(!workflow.TakeReloadRequest(), "later terminal state after cancel never reloads");
    }

    {
        CookStageReloadWorkflow workflow;
        Expect(workflow.BeginWaiting(), "cancel during ReloadPending");
        workflow.Observe(EditorToolKind::CookAndStage, EditorToolJobState::Succeeded);
        workflow.Cancel();
        Expect(!workflow.TakeReloadRequest(), "cancel before take blocks reload");
    }

    {
        CookStageReloadWorkflow workflow;
        workflow.MarkRejected();
        Expect(
            workflow.LastStatus() == CookStageReloadStatus::Rejected, "rejected status");
        Expect(!workflow.IsPending(), "reject does not arm waiting");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d cook-stage-reload workflow test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("CookStageReload workflow tests passed.\n");
    return 0;
}
