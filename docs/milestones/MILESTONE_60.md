## Milestone 60 — Split Milestone Documentation

### Status

**Implemented.** Awaiting manual acceptance. Do not mark CLOSED. Do not
start Milestone 61.

### Branch

`milestone/60-split-milestone-docs`

### Cursor Model

**Grok 4.6 High — Fast OFF**

### Goal

Reduce Cursor context/token cost caused by the monolithic
`docs/MILESTONES.md` history by migrating milestone documentation to one
canonical file per milestone under `docs/milestones/`.

This milestone is documentation and Cursor-context architecture only. It
does not change gameplay, editor, renderer, physics, Level Format, or
runtime asset-loading behavior.

After M60, a normal implementation prompt must be able to reference only
the specific active milestone file rather than requiring the full
milestone history.

### Canonical layout

Create exactly:

    docs/milestones/

Use lowercase `milestones`. Do not create `docs/Milestones/`,
`docs/MILESTONES/`, or `docs/milestone/`.

Canonical naming:

    docs/milestones/MILESTONE_<NUMBER>.md

Integer example:

    Milestone 29 -> docs/milestones/MILESTONE_29.md

Decimal example:

    Milestone 58.4 -> docs/milestones/MILESTONE_58_4.md

Use underscores in filenames for decimal separators. Do not create
dotted filename alternatives such as `MILESTONE_58.4.md`. Preserve the
human-readable number (for example `58.4`) inside the document.

Leading zeros in historical headings (`Milestone 00`, `Milestone 01`)
map to integer filenames `MILESTONE_0.md` and `MILESTONE_1.md`. The
original heading text stays inside those files.

One milestone per file. Do not group `M58` / `M58.1` / `M58.2` /
`M58.3` / `M58.4` into one file.

This file, `docs/milestones/MILESTONE_60.md`, is the canonical active
definition for M60. Understanding M60 must not require loading the
historical corpus.

### Historical preservation

Every milestone previously represented in `docs/MILESTONES.md` receives
one canonical individual file.

During extraction preserve, as applicable:

- milestone heading;
- milestone status;
- branch;
- Cursor model;
- goal;
- scope;
- architecture decisions;
- implementation requirements;
- validation;
- tests;
- manual acceptance;
- limitations;
- closure notes;
- commands;
- code blocks;
- meaningful historical wording.

Do **not** rewrite historical milestones according to today's
architecture. Do **not** silently "fix" old requirements because the
engine later evolved. Old milestone files are historical records.
Current code and tests remain authority for current behavior.

Mechanical Markdown cleanup is allowed only where necessary to make
each individual file valid and readable.

### `docs/MILESTONES.md` after migration

Do not keep the full historical milestone bodies duplicated inside
`docs/MILESTONES.md`. That would defeat the primary token/context goal.

Convert it into a compact index / navigation document containing:

- a short explanation of the new structure;
- the canonical directory;
- the filename convention, including decimals;
- the current/latest milestone reference;
- compact milestone links/status navigation.

It may remain a compatibility entry point. It must not continue
containing the complete milestone corpus.

### Cursor context contract

The normal milestone workflow is:

1. The user/prompt identifies the active milestone number or file.
2. Cursor reads `docs/milestones/MILESTONE_<N>.md`.
3. Cursor inspects current repository code, tests, and docs relevant to
   that milestone.
4. Cursor reads **other** milestone files only when specifically
   necessary to resolve a dependency, historical decision, or ambiguity.
5. Cursor does **not** automatically load every historical milestone.

This is the primary token-saving objective of M60.

Future prompts should be able to say `Read docs/milestones/MILESTONE_61.md`
rather than requiring `Read docs/MILESTONES.md`. Future milestone
definitions are added directly as `docs/milestones/MILESTONE_<N>.md`.
The compact index may then receive a small navigation/status entry.

### Source of truth

1. current repository code;
2. tests;
3. current architecture/docs;
4. active milestone file;
5. latest relevant checkpoint/current documentation;
6. older milestone files/history.

Historical milestone files do not override current implemented
behavior.

### Reference migration

Search the repository for milestone-document references. Update each
reference according to its semantic purpose:

- a specific historical milestone points to its individual file;
- a general navigation/index reference may still point to compact
  `docs/MILESTONES.md` and/or the `docs/milestones/MILESTONE_<N>.md`
  pattern.

Do not change every historical reference to M60. Do not rewrite
historical mentions of `docs/MILESTONES.md` inside old milestone files;
those remain historical records of the workflow at that time.

At minimum inspect:

- `docs/ARCHITECTURE.md`;
- `.cursor/rules/**/*`;
- `.cursor/skills/**/*`;
- other `.cursor` instruction/config files;
- `docs/**/*`;
- root Markdown/workflow files;
- scripts/tests that validate documentation paths.

Use repository-relative Markdown links. Example from
`docs/MILESTONES.md` to M59: `[Milestone 59](milestones/MILESTONE_59.md)`.
Example from a root document:
`[Milestone 59](docs/milestones/MILESTONE_59.md)`.

Canonical directory casing is lowercase `docs/milestones/`. Filename
prefix is uppercase `MILESTONE_`. Paths must work on case-sensitive
filesystems.

### EOL safety

The repository has emitted LF→CRLF warnings for `docs/MILESTONES.md`.
M60 must not become an opportunistic EOL-normalization milestone.

Do not:

- normalize the whole repository;
- run dos2unix/unix2dos across docs;
- introduce a repository-wide line-ending rewrite;
- change global Git settings;
- add a broad `.gitattributes` policy merely because of this warning.

Preserve current repository conventions. Inspect `git diff --stat`.
Avoid a diff where unrelated files appear completely rewritten solely
because of LF/CRLF conversion. Run `git diff --check`.

### No gameplay / editor change

M60 must not modify behavior of Application gameplay, Renderer, Jolt
Physics, Level Editor, Content Browser, Item Pickups, Inventory, Doors,
Pressure Plates, Dynamic Boxes, Level Format, or runtime asset loading.

Source changes are not expected unless an existing source/tool
explicitly depends on milestone documentation paths. If unexpected
runtime source changes appear necessary: STOP and report before
broadening scope.

### Canonical data safety

Do not modify gameplay fixtures. Level 01 remains:

- Item Pickups = 0
- Doors = 0
- Pressure Plates = 0
- Dynamic Boxes = 0
- Static Props = 0

Keep intentionally removed legacy line absent:

    dynamic_box 0 5 0 1 1 1 30

### Explicitly out of scope

Do not implement:

- gameplay features;
- editor features;
- renderer changes;
- physics changes;
- Level Format v2;
- a documentation website;
- a documentation generator framework;
- automatic milestone generation;
- automatic milestone-number inference from Git branch;
- a generic agent context manager;
- repository-wide EOL normalization;
- broad `.gitattributes` policy changes;
- deletion of historical milestone content;
- rewriting historical milestones to current architecture;
- Milestone 61 functionality.

Do not anticipate Milestone 61.

### Validation

Validate that:

1. `docs/milestones/` exists;
2. every milestone previously represented in `docs/MILESTONES.md` has
   exactly one intended individual canonical file;
3. no milestone was omitted;
4. decimal milestone mapping is correct;
5. M59 exists at `docs/milestones/MILESTONE_59.md`;
6. M60 exists at `docs/milestones/MILESTONE_60.md`;
7. `docs/MILESTONES.md` no longer duplicates the complete milestone
   corpus;
8. index links resolve;
9. references to specific historical milestones resolve;
10. Cursor rules use the new per-milestone convention;
11. Cursor skills use the new convention where applicable;
12. ARCHITECTURE/workflow docs use the new convention where applicable;
13. normal implementation instructions do not require loading all
    historical milestones;
14. historical milestone content was not accidentally discarded;
15. no unintended runtime/editor source changes occurred;
16. no unintended EOL-only mass rewrite occurred.

After migration, search again for `docs/MILESTONES.md`, `MILESTONES.md`,
`MILESTONE_`, `docs/milestones`, and wording equivalent to "read all
milestones" / "read milestone history" / "consult all milestones".
Remaining `docs/MILESTONES.md` references are allowed only where they
intentionally mean the compact index.

A small focused validation script/test is acceptable if it provides
clear regression value for the new canonical paths/index. Do not create
a large documentation framework.

### Tests / builds

Run affected documentation/path validation tests. Also run the standard
inexpensive Python validations unless repository inspection establishes
a concrete reason not to:

```text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Follow the repository milestone workflow for builds where applicable:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

At minimum run `git diff --check` and report exactly which commands were
run and their results.

### Manual acceptance

1. Open `docs/milestones/` and confirm individual milestone files exist.
2. Open representative files: one early integer milestone, one later
   integer milestone, one decimal milestone, `MILESTONE_59.md`, and
   `MILESTONE_60.md`.
3. Confirm each file contains only its intended milestone.
4. Open `docs/MILESTONES.md` and confirm it is a compact
   index/navigation file that does not contain the complete milestone
   bodies.
5. Follow several index links and confirm they resolve.
6. Inspect `docs/ARCHITECTURE.md`, relevant `.cursor/rules/**/*`,
   relevant `.cursor/skills/**/*`, and `AGENTS.md`.
7. Confirm Cursor instructions tell the implementation agent to read
   the specific milestone file named by the prompt, and do not normally
   require reading all milestone history.
8. Search for stale monolithic-workflow references. Confirm any
   remaining `docs/MILESTONES.md` references intentionally mean the
   compact index.
9. Inspect `git diff --stat` and confirm no suspicious EOL mass rewrite.
10. Run `git diff --check`.

### Completion criteria

Ready for manual acceptance when `docs/milestones/` contains one
canonical file per existing milestone; `docs/MILESTONES.md` is compact;
this file is the canonical M60 definition; decimal naming is
deterministic; ARCHITECTURE/Cursor rules/skills/other relevant
references use the new convention; normal Cursor implementation reads
the specific milestone named by the prompt; history is preserved; links
resolve; no unintended runtime/editor or EOL changes occurred;
validations pass; canonical Level 01 is clean; and no M61 functionality
was introduced.

### STOP

After migration, validation, documentation updates and report: STOP.

Do not commit, push, merge, start M61, or declare M60 CLOSED. Wait for
user manual acceptance and the separate Git closure workflow.
