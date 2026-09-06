#pragma once

// Milestone 40: one known Cook → Stage → Reload convenience workflow.
// Not a generic engine. Cook/Stage stay on EditorToolRunner; Reload stays
// the M39 in-process path. This type only tracks pending/consume state.

#include "editor/EditorToolRunner.h"

namespace editor
{
enum class CookStageReloadState
{
    Idle,
    WaitingForCookAndStage,
    ReloadPending,
};

enum class CookStageReloadStatus
{
    NotAttempted,
    WaitingForCookAndStage,
    ReloadPending,
    Completed,
    ExternalFailed,
    ReloadFailed,
    Rejected,
    Cancelled,
};

const char* CookStageReloadStateName(CookStageReloadState state);
const char* CookStageReloadStatusName(CookStageReloadStatus status);

class CookStageReloadWorkflow
{
public:
    CookStageReloadState State() const;
    CookStageReloadStatus LastStatus() const;
    bool IsPending() const;

    // Idle -> WaitingForCookAndStage. Caller must already have started
    // canonical CookAndStage and observed IsRunning(). Does not launch tools.
    bool BeginWaiting();

    // Observe the single Application Poll snapshot. Never Polls, never waits.
    void Observe(EditorToolKind kind, EditorToolJobState jobState);

    // After Observe moves to ReloadPending: exactly one true, then Idle.
    bool TakeReloadRequest();

    void NotifyReloadFinished(bool succeeded);
    void Cancel();
    void MarkRejected();

private:
    CookStageReloadState state = CookStageReloadState::Idle;
    CookStageReloadStatus lastStatus = CookStageReloadStatus::NotAttempted;
};
}
