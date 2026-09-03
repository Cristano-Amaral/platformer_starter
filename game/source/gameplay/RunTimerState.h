#pragma once

namespace gameplay
{
// Application-owned current-run elapsed time. Not owned by Player,
// PhysicsWorld, or Renderer. PerformRespawn must not clear this.
//
// elapsedSeconds is the single source of truth for the current run.
// When frozen, that same value is the completion time. There is no
// separate completionTime, bestTime, or previous-run field.
//
// frozen is the timer's own pause flag: Application sets it on the
// first completed false -> true transition and clears it in RestartRun.
// It is kept explicit so DebugMetrics can report Running/Frozen without
// inferring from LevelCompletionState, and so accumulation does not
// silently couple to completion except at those two sites.
struct RunTimerState
{
    double elapsedSeconds = 0.0;
    bool frozen = false;
};
}
