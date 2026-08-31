---
name: debug-crash
description: Diagnose a crash or hard failure from symptoms, logs, debugger call stacks, and recent changes. Use when the game crashes, freezes, exits unexpectedly, or hits an assertion.
---
# Debug Crash
1. Reproduce or reconstruct the failure from the supplied evidence.
2. Prefer concrete evidence: call stack, assertion, logs, exception code, last known working commit/change.
3. Identify the first frame in project code, not merely the final library/OS frame.
4. Form at most three ranked hypotheses.
5. Add minimal diagnostic logging/assertions only when evidence is insufficient.
6. Fix the root cause rather than masking the symptom.
7. Rebuild and run the narrowest reproduction test.
8. State cause, fix, affected files, and regression risk.
