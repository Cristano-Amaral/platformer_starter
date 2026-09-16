# Milestone 29 --- Persistent Best Time (Save File v1) [ACTIVE]

### Status

**Phase B — implementation complete / awaiting Phase C manual validation.** Do not mark complete until Phase C manual validation and Git closure are finished. Do not start Milestone 30.

### Branch

`milestone/29-persistent-best-time`

### Recommended Cursor model

Grok 4.6 High --- Fast OFF

### Goal

Extend M28 so the session BEST can survive a full executable restart
through one tiny versioned save file.

M29 is the project's **first persistent player-data milestone**. The
goal is not a generic save system. Persist exactly one datum: the best
completion time.

The design must preserve portability boundaries and CWD independence.

### Player-facing contract

First-ever launch / no valid save:

``` text
BEST --:--.---
```

Complete a run:

``` text
TIME 00:40.500
BEST 00:40.500
```

Close the game completely and launch again:

``` text
BEST 00:40.500
```

Complete a slower run:

``` text
BEST 00:40.500
```

Complete a faster run:

``` text
BEST 00:37.250
```

Close/relaunch:

``` text
BEST 00:37.250
```

If the save is missing, malformed, unsupported, non-finite, negative, or
otherwise invalid, the game must recover safely to:

``` text
BEST --:--.---
```

### Core architecture rule

Gameplay/Application owns the meaning of BEST.

Persistence only loads/stores a small data value. It must not decide
whether a run is a record.

Do not create a generic SaveManager.

### Persistence scope

Persist exactly: - whether a valid best exists; - the raw best time in
seconds.

Do not persist: - current TIME; - completion; - checkpoints; - death
count; - collectibles; - moving platform; - cyan box; - player
transform; - camera; - run history; - previous run; - settings.

### Save format

Use a tiny project-owned, human-readable, versioned text format.

Recommended v1:

``` text
PLATFORMER_SAVE 1
best_seconds 40.500123456
```

Requirements: - explicit magic/header; - explicit version `1`; - one
`best_seconds` value; - enough precision to round-trip the stored
`double`; - strict validation; - no JSON dependency.

The exact whitespace may follow a simple documented parser contract.

### Save location

M29 must not depend on the process current working directory.

Phase A must inspect the current platform/path layer and choose the
smallest writable user-data path boundary appropriate for Windows-first
development without leaking Windows APIs into gameplay.

Preferred architectural direction:

``` text
platform::UserDataDirectory()
```

Application/persistence code may use the returned project-owned path,
but OS-specific path discovery belongs behind `platform`.

Do not blindly write beside the executable if that would create an
install-permission problem.

Phase A must report the exact chosen Windows path policy before Phase B
writes files.

### Suggested save filename

``` text
best_time_v1.txt
```

Use a project-specific subdirectory if required by the chosen user-data
path policy.

### State ownership

Preserve M28:

``` cpp
struct SessionBestTimeState
{
    bool hasBestTime = false;
    double bestSeconds = 0.0;
};
```

Application still owns the live BEST state.

Persistence should expose a tiny data-oriented boundary, for example:

``` cpp
struct PersistentBestTimeData
{
    bool hasBestTime = false;
    double bestSeconds = 0.0;
};
```

A still-smaller equivalent is acceptable after Phase A inspection.

Do not make the persistence layer own gameplay state.

### Load semantics

Load once during Application initialization, before normal gameplay
rendering.

Valid save: - populate session BEST from disk.

Missing save: - normal first-run condition; - no warning/error
required; - BEST remains unavailable.

Invalid/corrupt save: - do not crash; - do not accept partial garbage; -
BEST remains unavailable; - Development/Debug may report a concise
diagnostic.

Unsupported future version: - do not guess; - do not parse as v1; - BEST
remains unavailable.

### Validation

A loaded `best_seconds` must be: - finite; - strictly greater than
zero; - representable as the project's `double`; - syntactically valid
under the documented v1 format.

Reject NaN, infinity, zero, negative values, missing fields, wrong
magic, wrong version, trailing malformed content, and parse failure.

### Save semantics

Write only when a new BEST is established: - first completed run; -
later faster completed run.

Do not write: - every frame; - on Enter; - on R; - on Fall; - on
Hazard; - on checkpoint; - on collectible; - on slower/tied
completion; - merely on shutdown.

### Failure semantics

A save-write failure must not invalidate the in-memory run result.

If the player sets a new BEST but persistence fails: - live BEST remains
the new record for the current process; - game continues; -
Debug/Development reports failure; - no crash.

Release must remain playable.

### File-write safety

Do not intentionally expose the save to obvious partial-write
corruption.

Corrected Phase A strategy (Windows-validated): 1. write complete v1
contents to a sibling `best_time_v1.tmp`; 2. flush/close successfully;
3. promote temp -> final through `platform::ReplaceFileWithTemporary`.
Do **not** `remove(final)` as a separate step before promotion.

Windows: `ReplaceFileW` when `best_time_v1.txt` already exists (old
final stays valid until that OS call succeeds); `MoveFileExW` with
`MOVEFILE_WRITE_THROUGH` when the final file is missing (first save).
POSIX: `std::filesystem::rename` for compile compatibility only.

Do not claim crash-proof/durable transactional storage. M29 requires
avoiding obvious partial final writes and delete-old-before-promote,
plus flush/close of the temp. It does not require fsync, directory
fsync, journaling, power-loss-proof transactions, or backup
generations.

Keep it narrow. Do not build journaling, backups, transactions, or a
generic atomic-file framework.

Temp leftover policy: load never reads `best_time_v1.tmp`. If temp
write or promotion fails, Phase B should best-effort `remove(temp)`.
If that cleanup fails, ignore it for gameplay; Save still reports
Error. No temp recovery, journal, or backup rotation.

### Precision

BEST comparison remains M28 raw `double`.

Persist enough decimal precision for a `double` round trip (for example
`std::numeric_limits<double>::max_digits10`).

HUD remains `MM:SS.mmm` via the existing formatter.

Do not store only formatted milliseconds.

### Restart semantics

Enter: - resets current run state; - preserves loaded/current BEST; -
performs no save unless a new record was just established at completion.

Full process relaunch: - loads persisted BEST.

### HUD

No new gameplay HUD element is required.

Preserve:

``` text
TIME MM:SS.mmm
BEST MM:SS.mmm
COLLECTED N / 3
```

Before any valid saved or newly completed BEST:

``` text
BEST --:--.---
```

### Debug/Development diagnostics

Add minimal persistence diagnostics, such as: - Save path; - Load
status: Missing / Loaded / Invalid / UnsupportedVersion / Error; - Save
status: NotAttempted / Saved / Error; - persisted BEST formatted value
when valid.

Do not expose a save editor.

### Release

Release: - loads/saves the BEST; - shows normal gameplay HUD; - no Dear
ImGui; - no verbose console spam required for normal missing-save
behavior.

### Portability

-   gameplay must not call Win32 APIs;
-   OS-specific writable-directory discovery stays in platform code;
-   persistence parsing/serialization should be standard C++ where
    practical;
-   use `std::filesystem`;
-   preserve case-sensitive path discipline;
-   do not introduce a Windows-only save design into Application.

### Explicitly out of scope

No generic save system, save slots, autosave framework, manual save/load
UI, current-run resume, settings persistence, checkpoint persistence,
collectible persistence, death persistence, run history, previous times,
leaderboard, ranking, medals, score, ghost/replay, cloud sync, Steam
integration, achievements, encryption, compression, checksum system,
backup rotation, migration framework, JSON library, database,
SaveManager, ProfileManager, ECS/event bus, or M30.

### Phase A --- Persistence design/scaffolding

Inspect and report: - current Application initialization; - M28 BEST
update point; - current platform/path abstractions; -
executable-relative asset path behavior; - available `std::filesystem`
utilities; - exact Windows writable user-data path policy; - exact save
filename/path; - v1 grammar; - parser validation; - double serialization
precision; - load result model; - save result model; - safe-write
strategy; - ownership boundaries; - load insertion point; - save
insertion point; - CWD independence; - failure behavior; - test
strategy.

Phase A may add compile-only structs/helpers/interfaces and
parser/serializer unit-like static/runtime checks, but must not make
persistence live.

Scaffolding added and approved in Phase A (safe-replacement correction included):
- `platform::UserDataDirectory()` (Windows `FOLDERID_LocalAppData`; POSIX stub empty);
- `platform::ReplaceFileWithTemporary` (`platform/FileReplace.h`; Windows `ReplaceFileW` / first-save `MoveFileExW`; POSIX `rename`);
- `persistence/BestTimeSave.h` + `.cpp`: v1 serialize/parse, path helpers, load/save status enums.

Safe-replacement correction: the earlier `remove(final)` then `rename(temp, final)` plan is rejected. Promotion goes through the platform boundary only.

### Phase B --- Implementation

Implemented: - user-data path boundary; - `LoadBestTime` / `SaveBestTime`; - load once in Initialize after `sessionBestTimeState = {}`; - save only on new record after in-memory update; - temp sibling + `ReplaceFileWithTemporary`; - nonfatal save failures; - Debug/Development persistence metrics; - docs; - all Windows configs; - Development/Release smoke from unrelated CWD.

Do not mark M29 complete until Phase C.

### Phase C --- Manual validation

Canonical save (manipulate **only** this file; do not delete unrelated LocalAppData content):

``` text
%LOCALAPPDATA%\Platformer3D\best_time_v1.txt
```

Temp sibling (never load; do not treat as recovery):

``` text
%LOCALAPPDATA%\Platformer3D\best_time_v1.tmp
```

At minimum: 1. Start with no save: `BEST --:--.---`, Load Missing, TIME fresh. 2. Complete Run 1: BEST = TIME, Save Saved, canonical file created (`PLATFORMER_SAVE 1` / `best_seconds <positive finite>`). 3. Close/relaunch from unrelated CWD: BEST restored, Load Loaded, TIME/completion/COLLECTED fresh. 4. Enter fresh run: BEST preserved; no save merely due to Enter. 5. Complete slower run: BEST unchanged; save mtime unchanged if timestamps are reliable; Save stays NotAttempted unless this process already saved a record. 6. Complete faster run: BEST updates, Save Saved, file updates. 7. Close/relaunch: faster BEST restored. 8. Delete only the canonical save: next launch returns to placeholder, Load Missing, no crash. 9. Corrupt save contents: Load Invalid, placeholder, no crash. 10. Wrong magic: Invalid. 11. Unsupported version (`PLATFORMER_SAVE 2`): UnsupportedVersion. 12. `best_seconds 0`, negative, NaN, infinity: Invalid. 13. Restore/create valid save: loads again. 14. R/Fall/Hazard/checkpoints/collectibles/Enter without a new record do not rewrite the save. 15. Release from unrelated CWD loads/saves correctly and has no ImGui. 16. Confirm only BEST persists (TIME, completion, checkpoints, deathCount, collectibles, platform, cyan box, camera are fresh).

### Completion criteria

M29 is complete only when: - all configs build; - valid BEST survives
process relaunch; - missing/invalid/unsupported saves fail safely; -
save writes occur only for a new record; - raw double precision is
preserved; - CWD independence is demonstrated; - gameplay remains
independent of OS-specific APIs; - no generic save framework/scope creep
exists; - Phase C passes; - user approves; - branch is
committed/pushed/merged; - main is clean and synchronized.

### Git closure

After approval:

``` powershell
git status
git add .
git commit -m "Milestone 29 - Persistent best time"
git push -u origin milestone/29-persistent-best-time

git checkout main
git pull origin main
git merge milestone/29-persistent-best-time
git push origin main
git status
```

Do not start Milestone 30 before `main` is clean and synchronized.
