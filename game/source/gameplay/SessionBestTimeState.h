#pragma once

namespace gameplay
{
// Application-owned session-best completion time. Not owned by Player,
// PhysicsWorld, or Renderer. PerformRespawn and RestartRun must not clear this.
//
// Lifetime is the process/Application session, not one run. Enter starts a
// fresh run; only executable relaunch returns this to no-best.
//
// hasBestTime is the no-record flag. bestSeconds is meaningful only when
// hasBestTime is true; 0.0 with hasBestTime false is not a record. There is
// no persistence, previous-run list, or manager.
struct SessionBestTimeState
{
    bool hasBestTime = false;
    double bestSeconds = 0.0;
};

// Raw-double comparison. Ties and slower runs do not replace the record.
constexpr bool IsBetterSessionCompletion(
    const SessionBestTimeState& session,
    double elapsedSeconds)
{
    return !session.hasBestTime || elapsedSeconds < session.bestSeconds;
}

static_assert(!SessionBestTimeState{}.hasBestTime);
static_assert(SessionBestTimeState{}.bestSeconds == 0.0);
static_assert(IsBetterSessionCompletion(SessionBestTimeState{}, 1.0));
static_assert(IsBetterSessionCompletion(SessionBestTimeState{true, 10.0}, 9.0));
static_assert(!IsBetterSessionCompletion(SessionBestTimeState{true, 10.0}, 10.0));
static_assert(!IsBetterSessionCompletion(SessionBestTimeState{true, 10.0}, 11.0));
}
