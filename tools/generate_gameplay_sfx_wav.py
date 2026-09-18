#!/usr/bin/env python3
"""Generate project-owned Milestone 71 gameplay SFX WAVs.

Damage, death, and respawn cues are short synthesized PCM WAVs. They are not
third-party audio. The existing Milestone 61 pickup chime is owned by
generate_item_pickup_collect_wav.py and is not rewritten here.

Re-run from the repository root:

    python tools/generate_gameplay_sfx_wav.py

Writes:

    game/assets/source/sounds/player_damage.wav
    game/assets/source/sounds/player_death.wav
    game/assets/source/sounds/player_respawn.wav

Cook/stage those source files afterwards; the game never reads this generator
or source/.
"""

from __future__ import annotations

import math
import struct
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SOUNDS = REPO_ROOT / "game" / "assets" / "source" / "sounds"

SAMPLE_RATE = 22050
BITS = 16
CHANNELS = 1


def wav_bytes(pcm: bytes) -> bytes:
    byte_rate = SAMPLE_RATE * CHANNELS * BITS // 8
    block_align = CHANNELS * BITS // 8
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
        CHANNELS,
        SAMPLE_RATE,
        byte_rate,
        block_align,
        BITS,
        b"data",
        data_size,
    )
    return header + pcm


def pcm16_from_samples(samples: list[float]) -> bytes:
    frames = bytearray()
    for value in samples:
        clamped = max(-1.0, min(1.0, value))
        frames.extend(struct.pack("<h", int(clamped * 32767.0)))
    return bytes(frames)


def damage_samples() -> list[float]:
    # Short low thud plus a brief high click. Distinct from pickup/death.
    duration = 0.12
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        envelope = math.exp(-time_seconds * 28.0)
        thud = 0.55 * math.sin(2.0 * math.pi * 110.0 * time_seconds)
        body = 0.28 * math.sin(2.0 * math.pi * 73.0 * time_seconds)
        click = 0.0
        if time_seconds < 0.018:
            click = 0.22 * math.sin(2.0 * math.pi * 1760.0 * time_seconds) * (
                1.0 - time_seconds / 0.018
            )
        samples.append(0.40 * envelope * (thud + body) + click)
    return samples


def death_samples() -> list[float]:
    # Descending tone, longer than damage so a lethal hit is readable alone.
    duration = 0.42
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        t = time_seconds / duration
        frequency = 220.0 + (70.0 - 220.0) * t
        envelope = math.exp(-time_seconds * 4.8) * (1.0 - 0.25 * t)
        fundamental = math.sin(2.0 * math.pi * frequency * time_seconds)
        overtone = 0.35 * math.sin(2.0 * math.pi * frequency * 1.5 * time_seconds)
        samples.append(0.38 * envelope * (fundamental + overtone))
    return samples


def respawn_samples() -> list[float]:
    # Rising two-tone, lower than the M61 pickup chime so the cues stay distinct.
    duration = 0.22
    split = 0.11
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        if time_seconds < split:
            envelope = math.exp(-time_seconds * 14.0)
            frequency = 523.25
        else:
            envelope = math.exp(-(time_seconds - split) * 12.0)
            frequency = 783.99
        samples.append(
            0.32 * envelope * math.sin(2.0 * math.pi * frequency * time_seconds)
        )
    return samples


DAMAGE_NAME = "player_damage.wav"
DEATH_NAME = "player_death.wav"
RESPAWN_NAME = "player_respawn.wav"

CUES = (
    (DAMAGE_NAME, damage_samples),
    (DEATH_NAME, death_samples),
    (RESPAWN_NAME, respawn_samples),
)


def cue_wav_bytes(name: str) -> bytes:
    for cue_name, builder in CUES:
        if cue_name == name:
            return wav_bytes(pcm16_from_samples(builder()))
    raise KeyError(name)


def main() -> int:
    SOUNDS.mkdir(parents=True, exist_ok=True)
    for name, builder in CUES:
        output = SOUNDS / name
        payload = wav_bytes(pcm16_from_samples(builder()))
        output.write_bytes(payload)
        print(f"wrote {output.relative_to(REPO_ROOT).as_posix()} ({len(payload)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
