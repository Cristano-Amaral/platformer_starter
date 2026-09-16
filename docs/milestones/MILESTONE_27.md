# Milestone 27 — Run Timer + Completion Time

### Status
**Complete (manually approved).** Do not reopen M27. Milestone 28 is the active milestone.

### Branch
`milestone/27-run-timer`

## Recommended Cursor model
Grok 4.6 High — Fast OFF

### Goal
Add a minimal run timer that measures the current run from fresh start until the first level completion, displays it in gameplay UI, preserves the completed time after `LEVEL COMPLETE`, and resets only on a fresh run via Enter.

This milestone introduces time measurement only. It does **not** introduce best-time persistence, leaderboards, score, ranking, medals, save/load, ghost data, or online services.

### Player-facing contract

Fresh run:

```text
TIME 00:00.000
```

During gameplay:

```text
TIME 00:12.347
```

First goal completion freezes the run time:

```text
TIME 00:38.912
LEVEL COMPLETE
PRESS ENTER TO RESTART
```

After completion:
- the displayed completion time remains frozen;
- movement continues as in M21;
- collectibles can still be collected as in M26;
- R / Fall / Hazard do not resume or reset the timer;
- Enter starts a fresh run and resets time to zero.

### Scope

#### Add
- minimal Application-owned run-timer state;
- monotonic frame-delta accumulation using the existing game loop timing;
- timer starts at fresh-run start;
- timer freezes on the first transition `completed: false -> true`;
- Release-visible `TIME MM:SS.mmm` HUD;
- Debug/Development run-timer metrics;
- fresh-run reset integration;
- manual regression validation.

#### Preserve
- M20 respawn semantics;
- M21 completion semantics;
- M22 fresh-run restart;
- M23 dynamic body safety;
- M24 ordered checkpoints;
- M25 hazards;
- M26 collectibles and `COLLECTED N / 3`;
- Player movement constants;
- camera;
- moving platform;
- cyan physics box;
- slopes;
- asset pipeline.

### Explicitly out of scope
- best time;
- persistent records;
- save/load;
- file I/O for timer;
- leaderboards;
- rankings;
- medals;
- score;
- currency;
- combo systems;
- split times;
- pause menu;
- countdown;
- time limit;
- time bonuses/penalties;
- collectible time bonuses;
- hazard time penalties;
- online services;
- ghost/replay;
- achievements;
- audio/particles;
- generic RunManager / TimerManager;
- ECS/event bus;
- new assets/dependencies;
- Milestone 28.

### Ownership

#### Application
Owns run timing state and decides when time advances/freezes/resets.

Suggested minimal state:

```cpp
struct RunTimerState
{
    double elapsedSeconds = 0.0;
    bool frozen = false;
};
```

A separate stored completion-time field is unnecessary if `elapsedSeconds` simply stops changing once completion occurs.

#### Renderer
Receives a small read-only elapsed time value and renders `TIME MM:SS.mmm`.

Renderer must not own or advance time.

#### PhysicsWorld
No timer knowledge.

#### Player
No timer knowledge.

#### world
No timer knowledge.

### Timing source
Use the existing per-frame delta time already used by the game loop / Application.

Do not add:
- wall-clock calendar time;
- system clock timestamps;
- platform-specific timing APIs;
- a second independent timing subsystem.

The timer should accumulate gameplay frame delta:

```text
elapsedSeconds += dt
```

while the run is active and not completed.

### Start semantics
A fresh run begins with:

```text
elapsedSeconds = 0
frozen = false
```

The timer advances from the first normal gameplay update of that run.

No pre-start countdown.

### Completion semantics
The timer freezes on the first frame that changes:

```text
completed == false
```

to:

```text
completed == true
```

After that:
- timer value remains unchanged;
- later goal overlap does nothing;
- R / Fall / Hazard do not alter time;
- collecting remaining collectibles does not alter time;
- only Enter fresh restart clears it.

### Same-frame goal rule
The frame that first completes the goal must produce one deterministic final time.

Preferred implementation:
- advance active timer once for the frame using the same frame-delta policy used throughout the run;
- process gameplay;
- if goal transitions to complete this frame, freeze after that frame's accumulated delta.

Do not create sub-frame interpolation.

### Respawn semantics
Manual R, Fall, and Hazard preserve accumulated time. If the level is not complete, timing continues after respawn. No time penalty is added.

### Completion + respawn
After `LEVEL COMPLETE`:
- timer is frozen;
- R/Fall/Hazard preserve frozen time;
- completion remains true;
- collectible progress remains preserved;
- completion UI remains visible.

### Enter restart
Existing fresh-run Enter restart must additionally:

```text
elapsedSeconds = 0
frozen = false
```

It must preserve all existing restart behavior:
- Player spawn;
- checkpoint reset;
- deathCount reset;
- respawn reason reset;
- completion reset;
- collectible reset to 0/3;
- moving platform reset;
- cyan box reset;
- CharacterVirtual reset;
- camera snap.

### HUD
Add Release-visible text:

```text
TIME MM:SS.mmm
```

Recommended placement:
- upper-left;
- `COLLECTED N / 3` remains upper-right;
- completion UI remains centered.

### Formatting
Use fixed-width time formatting:

```text
TIME 00:00.000
TIME 01:05.432
TIME 12:34.567
```

Minimum:
- 2-digit minutes;
- 2-digit seconds;
- 3-digit milliseconds.

If elapsed time exceeds 99 minutes, do not wrap. Allow additional minute digits.

No hours field is required.

### Numerical behavior
Store time in `double`.

Do not quantize internal state to milliseconds. Format milliseconds only for display.

### Debug metrics
Debug/Development should expose at least:
- Run time seconds;
- Timer state: Running / Frozen;
- Formatted run time;
- completion state;
- existing collectible/checkpoint/hazard/death metrics remain.

Metrics are read-only.

### Release
Release must contain:
- `TIME MM:SS.mmm`;
- collectible HUD;
- goal completion UI;
- all gameplay.

Release must not contain Dear ImGui/debug metrics.

### Architecture rules
- No `TimerManager`.
- No `RunManager`.
- No singleton.
- No event bus.
- No platform-specific clock API.
- No timer ownership in Renderer.
- No timer ownership in PhysicsWorld.
- No save file.
- No persistent best time.

### Phase A
Design/scaffolding only:
- inspect current dt/update/completion/restart flow;
- define exact timer state and ownership;
- define precise update/freeze/reset order;
- define formatting helper boundary;
- define HUD coexistence;
- build all configs;
- no live timer required yet.

Scaffolding added (compile-only; M26 gameplay unchanged):
- `gameplay/RunTimerState.h` (`elapsedSeconds`, `frozen`); Application member initialized to zero;
- `core/RunTimeFormat.h` (floor-to-millisecond display helper + integer `static_assert` cases);
- no `elapsedSeconds += dt` yet;
- no TIME HUD yet;
- `RestartRun` does not reset the timer yet (still always zero);
- DebugMetrics does not show timer fields yet.

### Phase B
Implementation complete / awaiting Phase C:
- live accumulation after `restartAvailableAtFrameStart` using `deltaSeconds`;
- first-completion freeze inside the existing goal transition;
- ordinary respawn preserves time (`PerformRespawn` unchanged);
- `RestartRun` resets `runTimerState`;
- Release-visible `TIME MM:SS.mmm` upper-left;
- Debug/Development Run timer metrics;
- docs/build/smoke.

Do not mark complete.

### Phase C manual validation
At minimum:
1. Fresh run starts near `TIME 00:00.000`.
2. Time increases while playing.
3. Manual R does not reset time.
4. Fall does not reset time.
5. Hazard does not reset time.
6. Checkpoints do not reset time.
7. Collectibles do not reset or otherwise modify time.
8. Goal works with less than 3/3 collectibles.
9. First goal completion freezes timer.
10. Walking after completion leaves time frozen.
11. Collecting an item after completion leaves time frozen.
12. R after completion leaves time frozen.
13. Hazard/Fall after completion leave time frozen.
14. Enter starts fresh run at zero.
15. Collectibles restore to 0/3 on new run.
16. deathCount/checkpoints/completion reset as before.
17. moving platform resets only on Enter.
18. cyan box resets only on Enter.
19. second run timer works again.
20. Release shows TIME + COLLECTED HUD and no ImGui.

### Completion criteria
M27 is complete only when:
- all three configs build;
- manual Phase C passes;
- timer behavior matches contracts above;
- no timer manager/persistence scope creep exists;
- docs are updated;
- user approves;
- feature branch is committed/pushed/merged;
- `main` is clean and synchronized.

### Git closure
After approval:

```powershell
git status
git add .
git commit -m "Milestone 27 - Run timer and completion time"
git push -u origin milestone/27-run-timer

git checkout main
git pull origin main
git merge milestone/27-run-timer
git push origin main
git status
```

Do not start Milestone 28 before `main` is clean and synchronized.
