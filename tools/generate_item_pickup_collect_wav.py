#!/usr/bin/env python3
"""Generate the project-owned Milestone 61 Item Pickup collection WAV.

The runtime sound is a short two-tone chime synthesized here as PCM WAV.
It is not third-party audio. Re-run from the repository root:

    python tools/generate_item_pickup_collect_wav.py

Writes game/assets/source/sounds/item_pickup_collect.wav. Cook/stage that
source file afterwards; the game never reads this generator or source/.
"""

from __future__ import annotations

import math
import struct
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
OUTPUT = REPO_ROOT / "game" / "assets" / "source" / "sounds" / "item_pickup_collect.wav"

SAMPLE_RATE = 22050
DURATION_SECONDS = 0.18
AMPLITUDE = 0.35
FIRST_HZ = 880.0
SECOND_HZ = 1320.0
SPLIT_SECONDS = 0.09


def sample_at(time_seconds: float) -> float:
    if time_seconds < SPLIT_SECONDS:
        envelope = math.exp(-time_seconds * 18.0)
        frequency = FIRST_HZ
    else:
        envelope = math.exp(-(time_seconds - SPLIT_SECONDS) * 16.0)
        frequency = SECOND_HZ
    return AMPLITUDE * envelope * math.sin(2.0 * math.pi * frequency * time_seconds)


def pcm16_frames() -> bytes:
    count = int(SAMPLE_RATE * DURATION_SECONDS)
    frames = bytearray()
    for index in range(count):
        value = sample_at(index / SAMPLE_RATE)
        clamped = max(-1.0, min(1.0, value))
        frames.extend(struct.pack("<h", int(clamped * 32767.0)))
    return bytes(frames)


def wav_bytes(pcm: bytes) -> bytes:
    channels = 1
    bits = 16
    byte_rate = SAMPLE_RATE * channels * bits // 8
    block_align = channels * bits // 8
    data_size = len(pcm)
    riff_size = 36 + data_size
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF",
        riff_size,
        b"WAVE",
        b"fmt ",
        16,
        1,
        channels,
        SAMPLE_RATE,
        byte_rate,
        block_align,
        bits,
        b"data",
        data_size,
    )
    return header + pcm


def main() -> int:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    payload = wav_bytes(pcm16_frames())
    OUTPUT.write_bytes(payload)
    print(f"wrote {OUTPUT.relative_to(REPO_ROOT).as_posix()} ({len(payload)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
