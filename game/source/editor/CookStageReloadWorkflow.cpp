#include "editor/CookStageReloadWorkflow.h"

namespace editor
{
const char* CookStageReloadStateName(CookStageReloadState state)
{
    switch (state)
    {
    case CookStageReloadState::Idle:
        return "Idle";
    case CookStageReloadState::WaitingForCookAndStage:
        return "WaitingForCookAndStage";
    case CookStageReloadState::ReloadPending:
        return "ReloadPending";
    }
    return "Idle";
}

const char* CookStageReloadStatusName(CookStageReloadStatus status)
{
    switch (status)
    {
    case CookStageReloadStatus::NotAttempted:
        return "NotAttempted";
    case CookStageReloadStatus::WaitingForCookAndStage:
        return "WaitingForCookAndStage";
    case CookStageReloadStatus::ReloadPending:
        return "ReloadPending";
    case CookStageReloadStatus::Completed:
        return "Completed";
    case CookStageReloadStatus::ExternalFailed:
        return "ExternalFailed";
    case CookStageReloadStatus::ReloadFailed:
        return "ReloadFailed";
    case CookStageReloadStatus::Rejected:
        return "Rejected";
    case CookStageReloadStatus::Cancelled:
        return "Cancelled";
    }
    return "NotAttempted";
}

CookStageReloadState CookStageReloadWorkflow::State() const
{
    return state;
}

CookStageReloadStatus CookStageReloadWorkflow::LastStatus() const
{
    return lastStatus;
}

bool CookStageReloadWorkflow::IsPending() const
{
    return state == CookStageReloadState::WaitingForCookAndStage
        || state == CookStageReloadState::ReloadPending;
}

bool CookStageReloadWorkflow::BeginWaiting()
{
    if (state != CookStageReloadState::Idle)
    {
        return false;
    }
    state = CookStageReloadState::WaitingForCookAndStage;
    lastStatus = CookStageReloadStatus::WaitingForCookAndStage;
    return true;
}

void CookStageReloadWorkflow::Observe(EditorToolKind kind, EditorToolJobState jobState)
{
    if (state != CookStageReloadState::WaitingForCookAndStage)
    {
        return;
    }

    if (jobState == EditorToolJobState::Running)
    {
        return;
    }

    if (jobState == EditorToolJobState::Succeeded && kind == EditorToolKind::CookAndStage)
    {
        state = CookStageReloadState::ReloadPending;
        lastStatus = CookStageReloadStatus::ReloadPending;
        return;
    }

    state = CookStageReloadState::Idle;
    lastStatus = jobState == EditorToolJobState::Idle
        ? CookStageReloadStatus::Cancelled
        : CookStageReloadStatus::ExternalFailed;
}

bool CookStageReloadWorkflow::TakeReloadRequest()
{
    if (state != CookStageReloadState::ReloadPending)
    {
        return false;
    }
    state = CookStageReloadState::Idle;
    return true;
}

void CookStageReloadWorkflow::NotifyReloadFinished(bool succeeded)
{
    lastStatus = succeeded
        ? CookStageReloadStatus::Completed
        : CookStageReloadStatus::ReloadFailed;
}

void CookStageReloadWorkflow::Cancel()
{
    if (state == CookStageReloadState::Idle
        && lastStatus != CookStageReloadStatus::WaitingForCookAndStage
        && lastStatus != CookStageReloadStatus::ReloadPending)
    {
        return;
    }
    state = CookStageReloadState::Idle;
    lastStatus = CookStageReloadStatus::Cancelled;
}

void CookStageReloadWorkflow::MarkRejected()
{
    lastStatus = CookStageReloadStatus::Rejected;
}
}
