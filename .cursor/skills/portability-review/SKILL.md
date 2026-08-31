---
name: portability-review
description: Review a proposed or completed change for Windows/Linux/Raspberry Pi/Android/iOS portability risks. Use before introducing platform APIs, filesystem/input/window assumptions, or new dependencies.
---
# Portability Review
Check the change for:
- direct Win32/Linux/mobile APIs leaking into gameplay;
- path separators, case sensitivity, working-directory assumptions;
- desktop-only keyboard/mouse assumptions;
- graphics API/backend leakage;
- endian/alignment/binary serialization assumptions;
- 32-bit vs 64-bit assumptions, especially ARM32 targets;
- compiler-specific extensions;
- dependency platform support;
- excessive CPU/GPU/memory expectations for embedded hardware.

Classify findings as blocker, important, or future concern. Recommend the smallest abstraction needed now; do not over-engineer hypothetical platform layers.
