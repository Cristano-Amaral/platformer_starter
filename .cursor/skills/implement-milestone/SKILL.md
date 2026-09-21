---
name: implement-milestone
description: Implement exactly one project milestone with build, smoke-test, and completion checks. Use when asked to start, implement, finish, or continue a milestone.
---
# Implement Milestone
Follow `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md`. Those files are the authoritative instructions.

1. Identify the single active milestone from the user prompt and read `docs/milestones/MILESTONE_<N>.md`. Use `docs/MILESTONES.md` only as a compact index.
2. Inspect current repository code, tests, and current docs before editing. Read other milestone files only for a dependency, historical decision, or ambiguity.
3. Implement only that milestone. Do not implement the next milestone.
4. Configure, build, and run the validation required by that milestone and by `DEVELOPMENT_WORKFLOW.md`.
5. Report changed files, validation, canonical-data status, and remaining manual acceptance.
6. STOP. Do not commit, push, merge, close the milestone, or begin the next milestone.
