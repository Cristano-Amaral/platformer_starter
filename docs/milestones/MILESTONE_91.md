# Milestone 91 — Agent-Agnostic Development Workflow

## Status

**IMPLEMENTED — awaiting manual acceptance**

## Branch

`milestone/91-agent-agnostic-development-workflow`

## Purpose

Make the repository itself the authoritative development context so that implementation work can be performed safely by multiple coding agents — initially Cursor, OpenAI Codex, and Google Antigravity — without duplicating the project's core rules for each provider.

M91 is a development-infrastructure milestone. It must not change gameplay, rendering behavior, Level Format, authored assets, physics, editor functionality, or runtime behavior.

The goal is not to make all agents behave identically. The goal is to give every supported agent the same authoritative repository instructions, milestone contract, validation commands, safety boundaries, and STOP conditions.

## Motivation

The project currently relies heavily on detailed implementation prompts. Many requirements repeated in those prompts are repository-wide invariants rather than milestone-specific requirements:

- source-of-truth order;
- milestone lifecycle;
- branch discipline;
- build/test commands;
- canonical Level safety;
- editor authority rules;
- no opportunistic future-scope work;
- manual acceptance requirement;
- Git closure ownership;
- STOP before commit/push/merge.

These rules should live primarily in the repository so that changing coding agents does not change the development process.

## Core Principle

The repository is authoritative; the coding agent is replaceable.

Expected flow:

```text
ChatGPT / User
      |
      v
MILESTONE_XX.md
      |
      v
Repository instructions
  AGENTS.md
  DEVELOPMENT_WORKFLOW.md
  architecture/docs/tests
      |
      +----------+-----------+
      |          |           |
      v          v           v
   Cursor      Codex     Antigravity
      |
      v
Implementation report
      |
      v
ChatGPT review
      |
      v
User manual acceptance
      |
      v
Git closure
```

## Scope

### 1. Audit Current Agent Instructions

Inspect the repository for existing agent/tool instruction files, including at minimum:

- `AGENTS.md`;
- Cursor project rules/configuration;
- development workflow documentation;
- milestone documentation conventions;
- build/test documentation;
- any existing Codex- or Gemini/Antigravity-specific instructions.

Determine which rules are authoritative, duplicated, stale, tool-specific, or missing.

Do not delete working tool configuration merely for uniformity.

### 2. Establish a Canonical `AGENTS.md`

Use the repository's existing `AGENTS.md` as the common entry point where practical.

It must remain concise enough to be useful to coding agents and should point to authoritative detailed documents rather than duplicating them wholesale.

It should establish at minimum:

- repository source-of-truth order;
- requirement to inspect current code/tests/docs before implementation;
- active milestone authority;
- branch/milestone isolation;
- prohibition on opportunistic future-scope work;
- required validation;
- canonical Level safety;
- manual acceptance ownership;
- prohibition on commit/push/merge/next milestone during implementation;
- where to find the detailed development workflow.

If repository structure benefits from narrower `AGENTS.md` files in subdirectories, add them only when they provide genuinely local instructions. Do not create hierarchy for its own sake.

### 3. Preserve One Authoritative Development Workflow

`DEVELOPMENT_WORKFLOW.md` remains the detailed lifecycle authority unless repository inspection demonstrates that its current location/name differs.

Avoid maintaining separate full workflows for Cursor, Codex, and Antigravity.

Tool-specific files may reference the canonical workflow and contain only integration details that cannot be expressed portably.

### 4. Cursor Compatibility

Preserve current Cursor behavior.

Audit `.cursor/` rules/configuration if present.

Cursor-specific rules must not silently contradict `AGENTS.md` or the canonical development workflow.

Prefer thin Cursor integration over duplicated project policy.

Existing Cursor implementation workflow must continue to support:

- repository inspection;
- milestone implementation;
- builds/tests;
- report;
- STOP without commit/push/merge.

### 5. OpenAI Codex Compatibility

Make the repository directly understandable to Codex through repository-native instructions.

Codex must be able to discover:

- project purpose and structure;
- active milestone;
- authoritative docs;
- build commands;
- test commands;
- milestone boundaries;
- canonical-data safety rules;
- STOP conditions.

Do not require a giant Codex-only copy of the development workflow.

Add Codex-specific configuration only if repository inspection and current supported behavior show that it provides a concrete benefit.

### 6. Google Antigravity Compatibility

Provide a thin compatibility path for Google Antigravity/Gemini-based coding agents.

Do not assume unsupported instruction filenames or configuration semantics.

Inspect the currently supported project-instruction mechanism before adding tool-specific files.

If Antigravity cannot consume `AGENTS.md` directly, add the smallest supported adapter that directs it to the same canonical repository instructions.

Do not duplicate the complete workflow.

### 7. Agent Bootstrap / Orientation

Provide a concise repository-native orientation path so a newly started coding agent can answer, from repository contents alone:

1. What project is this?
2. What is the source-of-truth order?
3. What milestone is active?
4. What branch should be used?
5. What files define the requested work?
6. How is the project configured and built?
7. Which tests are required?
8. Which canonical files require special care?
9. Who approves manual acceptance?
10. Is the agent allowed to commit/push/merge?
11. When must the agent stop?

Prefer links/references to authoritative documents over copied prose.

### 8. Portable Milestone Prompt Contract

Document a short, provider-neutral implementation prompt pattern.

Future implementation prompts should be able to approach:

```text
Implement Milestone XX as defined in:

docs/milestones/MILESTONE_XX.md

Follow AGENTS.md and the repository development workflow as authoritative instructions.

Inspect the current repository before implementation.
Preserve existing architecture and milestone boundaries.
Run all validation required by the milestone and repository workflow.

Report implementation, tests, builds, canonical-data status, and remaining manual acceptance.

STOP after reporting.
Do not commit, push, merge, close the milestone, or begin the next milestone.
```

This is a template/convention, not a replacement for milestone-specific requirements.

Complex corrections may still require detailed prompts.

### 9. Machine-Checkable Consistency

Where practical, add a lightweight validation script/test that detects important documentation/instruction drift without creating a documentation framework.

Candidate checks include:

- canonical instruction files referenced by `AGENTS.md` exist;
- current milestone documents follow expected naming/location conventions;
- portable agent instructions contain the required STOP/ownership references;
- tool-specific adapters reference canonical instructions rather than embedding conflicting workflow copies.

Keep checks narrow and maintainable.

### 10. Documentation

Update only documentation needed to explain the multi-agent workflow.

Document:

- common repository instructions;
- supported coding-agent entry points;
- which files are canonical versus adapters;
- how to start a milestone with Cursor, Codex, or Antigravity;
- expected agent completion report;
- user/ChatGPT/agent responsibilities.

## Authority and Responsibilities

### User

The User:

- chooses project direction;
- runs/observes local game behavior as needed;
- performs manual acceptance;
- explicitly approves milestone completion;
- performs or authorizes Git closure according to the established workflow.

### ChatGPT

ChatGPT:

- discusses architecture and scope;
- defines milestones;
- prepares milestone implementation/correction prompts;
- reviews agent reports critically;
- evaluates manual acceptance feedback;
- provides Git closure only after approval.

### Coding Agent

Cursor, Codex, Antigravity, or another implementation agent:

- inspects repository state;
- follows repository instructions and active milestone;
- implements only current scope;
- validates;
- reports;
- stops.

The coding agent does not become project authority merely because it has terminal/repository access.

## Source of Truth

Preserve the established order:

1. current repository code;
2. automated tests;
3. current authoritative documentation;
4. active milestone definition;
5. latest relevant checkpoint;
6. older milestone/chat context.

If the repository's existing canonical workflow specifies a more precise formulation, use that wording consistently.

## Canonical Data Safety

M91 must preserve the existing protections around canonical authored Level files.

At minimum, pre-report validation must include:

```bash
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
```

M91 should not modify canonical Level content.

Do not normalize line endings merely because Git reports CRLF/LF warnings.

## Required Validation

At minimum run the repository's standard configuration/build matrix:

```bash
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run relevant documentation/workflow tests, including any validation introduced by M91.

Run the established Python validation suite relevant to repository workflow, including where present:

```bash
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

If M91 adds a new agent-instruction consistency test, run it explicitly.

## Cross-Agent Acceptance

M91 does not require three independent implementations of game code.

Acceptance instead requires proving that the repository gives each target agent a clear path to the same workflow.

For Cursor, Codex, and Antigravity, verify/document that a fresh agent can discover:

- canonical instructions;
- active milestone location;
- build/test commands;
- no-commit/push/merge rule;
- manual acceptance ownership;
- canonical Level safety.

Where practical, perform a read-only/bootstrap exercise rather than making three agents edit production code.

The exercise may ask each agent to inspect the repository and report:

- authoritative instruction files;
- current branch;
- active milestone;
- required build/test commands;
- prohibited Git actions;
- STOP condition.

No agent should modify source files during this portability smoke test.

## Success Criteria

M91 is successful when:

- the repository has one clear common agent entry point;
- detailed workflow policy has one canonical authority;
- Cursor remains supported;
- Codex can operate from repository-native instructions;
- Antigravity has a supported thin adapter if required;
- tool-specific rules do not contain divergent copies of core workflow policy;
- future milestone prompts can be materially shorter without losing safety;
- required build/test/canonical-data rules remain discoverable;
- no runtime/game/editor behavior changes;
- automated validation passes;
- the user manually confirms the workflow/documentation is understandable;
- implementation agent stops before Git closure.

## Out of Scope

Do NOT:

- change gameplay;
- change renderer behavior;
- change physics;
- change editor behavior;
- change Level Format;
- change authored Level data;
- change assets;
- introduce gameplay/editor features;
- introduce M92 work;
- create a generalized AI orchestration platform;
- create automatic model routing;
- create cloud-agent infrastructure;
- create autonomous Git merging;
- store API keys/tokens;
- add vendor SDKs merely for documentation compatibility;
- duplicate the full workflow separately for every agent;
- benchmark/rank models;
- redesign CI unless a minimal validation hook is genuinely necessary for M91.

## No Vendor Lock-In

Tool-specific adapters are allowed only for capabilities/instruction-discovery mechanisms that genuinely require them.

Project policy, architecture rules, milestone lifecycle, tests, and safety constraints must remain repository-owned and provider-neutral.

## STOP Rule

After implementation and validation, the coding agent must provide its completion report and STOP.

It must NOT:

- commit;
- push;
- merge;
- close M91;
- begin M92.

Manual acceptance and Git closure remain separate steps.

## Expected Completion Report

The implementation agent should report:

1. repository instruction files discovered before changes;
2. duplicated/conflicting rules found;
3. canonical instruction architecture after M91;
4. `AGENTS.md` changes;
5. Cursor adapter/rule changes;
6. Codex compatibility work;
7. Antigravity compatibility work;
8. portable milestone prompt convention;
9. validation/check added;
10. documentation changes;
11. cross-agent/bootstrap verification;
12. build results;
13. Python/test results;
14. `git diff --check`;
15. canonical `level_01.level` and `level_02.level` diff status;
16. remaining manual acceptance;
17. legitimate Tuning Backlog candidates;
18. confirmation that no commit/push/merge/M92 work was performed.
