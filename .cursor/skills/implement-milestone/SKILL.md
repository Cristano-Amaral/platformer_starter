---
name: implement-milestone
description: Implement exactly one project milestone with build, smoke-test, and completion checks. Use when asked to start, implement, finish, or continue a milestone.
---
# Implement Milestone
1. Identify the single active milestone from the user prompt (number or `docs/milestones/MILESTONE_<N>.md`).
2. Read that specific canonical file. Use `docs/MILESTONES.md` only as a compact index if the prompt did not name a file.
3. Do not load all historical milestone files by default. Read other `docs/milestones/MILESTONE_*.md` files only when needed for a dependency, historical decision, or ambiguity.
4. Inspect current repository code, tests, and architecture docs relevant to that milestone. Current code/tests/docs remain authority over older milestone files.
5. Give a compact plan focused only on this milestone.
6. Implement in the smallest coherent steps.
7. Configure and build Development; build Debug too when diagnostics matter.
8. Run tests or a smoke test appropriate to the milestone.
9. Fix build/test regressions before stopping.
10. Report:
   - files changed;
   - behavior implemented;
   - commands used to build/test;
   - known limitations;
   - whether acceptance criteria are satisfied.
11. Never implement the next milestone implicitly.
