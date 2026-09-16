# Milestone 28 — Best Time (Session Record)

### Status
**Complete (manually approved).** Do not reopen M28. Milestone 29 is the active milestone.

### Branch
`milestone/28-session-best-time`

### Recommended Cursor model
Grok 4.6 High — Fast OFF

### Goal
Build on M27 with a session-only best completion time. On each run's first level completion, compare the frozen M27 run time with the best completed run since process launch. Display the record in gameplay UI and preserve it across Enter fresh-run restarts.

M28 deliberately has no disk persistence.

### Player-facing contract
Before any completed run:
```text
TIME 00:12.345
BEST --:--.---
COLLECTED 0 / 3
```

First completion:
```text
TIME 00:40.500
BEST 00:40.500
LEVEL COMPLETE
PRESS ENTER TO RESTART
```

After Enter:
```text
TIME 00:00.000
BEST 00:40.500
```

A slower later run leaves BEST unchanged. A faster later run replaces BEST.

### Core rule
BEST changes only on the first `completed: false -> true` transition of a run.

Candidate value is exactly `runTimerState.elapsedSeconds`, after the M27 completion-frame `dt` has been accumulated. Compare underlying `double` values, not formatted strings.

### Minimal state
Application owns session state. Preferred:
```cpp
struct SessionBestTimeState
{
    bool hasBestTime = false;
    double bestSeconds = 0.0;
};
```
`std::optional<double>` is acceptable if it fits existing conventions better. Phase A must choose the smallest clear representation.

Renderer and DebugMetrics receive read-only data. Player, PhysicsWorld, and world remain unaware.

### Comparison
No existing best:
```text
best = current completion time
```

Later completion:
```text
if current < best:
    replace best
else:
    preserve best
```

Exact ties need not rewrite the record. Do not quantize stored time to milliseconds.

### Reset/lifetime semantics
Manual R / Fall / Hazard:
- preserve current TIME;
- preserve BEST;
- never update BEST.

Checkpoints/collectibles:
- no effect on BEST.

After completion:
- TIME remains frozen;
- BEST remains unchanged;
- post-completion movement/collecting/respawns remain allowed.

Enter fresh restart:
- resets current run state exactly as M27;
- resets TIME to zero;
- **preserves BEST**.

Full executable relaunch:
- BEST returns to no-best-yet;
- no persistence.

### HUD
Preserve:
- TIME upper-left;
- COLLECTED upper-right;
- completion UI centered.

Add below TIME:
```text
BEST --:--.---
```
or:
```text
BEST 00:40.500
```

Visible in Debug, Development, and Release. Reuse M27 `RunTimeFormat`; no second formatter.

### Debug
Debug/Development:
- Has session best: true/false
- Session best seconds when available
- Formatted session best / placeholder
- preserve M27 timer and all existing metrics.

### Explicitly out of scope
No persistent record, save/load, files, JSON/config/registry, leaderboards, rankings, medals, score, run history, previous-time list, ghost/replay, splits, achievements, reset-best input, profile, networking, cloud, BestTimeManager, StatisticsManager, RunManager, event bus, ECS, new assets/dependencies, or M29.

### Phase A
Design/scaffolding only:
- inspect exact M27 accumulation/freeze/completion order;
- inspect RestartRun;
- define session lifetime and ownership;
- choose no-best representation;
- define exact best-update insertion point;
- define Renderer boundary/HUD placement;
- define debug metrics;
- prove Enter preserves BEST while process relaunch clears it;
- build Debug/Development/Release.

Scaffolding added (compile-only; M27 gameplay unchanged):
- `gameplay/SessionBestTimeState.h` (`hasBestTime`, `bestSeconds`) plus raw-double `IsBetterSessionCompletion`;
- Application member default/Initialize no-best; `RestartRun` does not reset it;
- `FormatSessionBestTime` in `core/RunTimeFormat.h` (`--:--.---` when no record);
- no live comparison at goal;
- no BEST HUD yet;
- DebugMetrics does not show session-best fields yet.

BEST need not be live yet.

### Phase B
Implementation complete / awaiting Phase C:
- first-completion record via `IsBetterSessionCompletion` in the existing goal branch;
- faster-run replacement; slower/tie preservation;
- `RestartRun` still does not reset `sessionBestTimeState`;
- Release-visible `BEST` HUD below TIME (y = 46);
- Debug/Development Session best metrics;
- docs/build/runtime smoke.
Do not mark complete.

### Phase C manual validation
1. Fresh executable: `BEST --:--.---`.
2. TIME runs normally.
3. R/Fall/Hazard before completion do not affect BEST.
4. Complete Run 1: BEST equals frozen Run 1 TIME.
5. Post-completion wait/move/collect/R/Fall/Hazard: BEST unchanged.
6. Enter: TIME resets; BEST remains.
7. Complete deliberately slower Run 2: BEST remains Run 1.
8. Enter again: BEST remains.
9. Complete deliberately faster Run 3: BEST updates to Run 3 TIME.
10. Goal remains independent of collectible count.
11. BEST updates at most once per run.
12. Release shows TIME + BEST + COLLECTED; no ImGui.
13. Fully close/relaunch executable: BEST returns to `--:--.---`.
14. Confirm no save/config/record file is created.

### Completion criteria
All configs build; Phase C passes; comparison uses raw `double`; Enter preserves BEST; process relaunch clears it; no persistence/scope creep; docs updated; user approves; branch merged; main clean/synchronized.

### Git closure
```powershell
git status
git add .
git commit -m "Milestone 28 - Session best time"
git push -u origin milestone/28-session-best-time

git checkout main
git pull origin main
git merge milestone/28-session-best-time
git push origin main
git status
```

Do not start M29 before main is clean and synchronized.
