#!/usr/bin/env python3
"""Generate project-owned Milestone 71/72/73/74 gameplay SFX WAVs.

Damage, death, respawn, footstep, jump, landing, checkpoint, pressure-plate,
door-unlock, level-goal, and collectible cues are short synthesized PCM WAVs.
They are not third-party audio. The existing Milestone 61 pickup chime is owned
by generate_item_pickup_collect_wav.py and is not rewritten here.

Re-run from the repository root:

    python tools/generate_gameplay_sfx_wav.py

Writes:

    game/assets/source/sounds/player_damage.wav
    game/assets/source/sounds/player_death.wav
    game/assets/source/sounds/player_respawn.wav
    game/assets/source/sounds/player_footstep.wav
    game/assets/source/sounds/player_jump.wav
    game/assets/source/sounds/player_land.wav
    game/assets/source/sounds/checkpoint_activate.wav
    game/assets/source/sounds/pressure_plate_activate.wav
    game/assets/source/sounds/pressure_plate_deactivate.wav
    game/assets/source/sounds/door_unlock.wav
    game/assets/source/sounds/level_goal_complete.wav
    game/assets/source/sounds/collectible_collect.wav

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


def footstep_samples() -> list[float]:
    # Short quiet tap, lower and drier than damage so a walking cadence stays
    # readable without matching the Hazard thud.
    duration = 0.07
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        envelope = math.exp(-time_seconds * 42.0)
        body = 0.42 * math.sin(2.0 * math.pi * 92.0 * time_seconds)
        noise = 0.18 * math.sin(2.0 * math.pi * 390.0 * time_seconds)
        samples.append(0.28 * envelope * (body + noise))
    return samples


def jump_samples() -> list[float]:
    # Brief rising blip, distinct from the M71 respawn two-tone.
    duration = 0.11
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        t = time_seconds / duration
        frequency = 240.0 + (420.0 - 240.0) * t
        envelope = math.exp(-time_seconds * 16.0) * (1.0 - 0.2 * t)
        samples.append(0.30 * envelope * math.sin(2.0 * math.pi * frequency * time_seconds))
    return samples


def land_samples() -> list[float]:
    # Heavier low thud than a footstep; shorter than death.
    duration = 0.10
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        envelope = math.exp(-time_seconds * 24.0)
        thud = 0.62 * math.sin(2.0 * math.pi * 64.0 * time_seconds)
        body = 0.22 * math.sin(2.0 * math.pi * 48.0 * time_seconds)
        samples.append(0.36 * envelope * (thud + body))
    return samples


def checkpoint_activate_samples() -> list[float]:
    # Bright three-note crystal, higher than respawn and not the M61 pickup.
    duration = 0.20
    count = int(SAMPLE_RATE * duration)
    notes = ((0.00, 1046.50), (0.06, 1318.51), (0.12, 1567.98))
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        value = 0.0
        for start, frequency in notes:
            if time_seconds < start:
                continue
            local = time_seconds - start
            envelope = math.exp(-local * 18.0)
            value += 0.28 * envelope * math.sin(2.0 * math.pi * frequency * time_seconds)
        samples.append(value)
    return samples


def pressure_plate_activate_samples() -> list[float]:
    # Short mid clack, drier than landing and higher than a footstep.
    duration = 0.08
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        envelope = math.exp(-time_seconds * 36.0)
        body = 0.48 * math.sin(2.0 * math.pi * 310.0 * time_seconds)
        click = 0.22 * math.sin(2.0 * math.pi * 720.0 * time_seconds)
        samples.append(0.34 * envelope * (body + click))
    return samples


def pressure_plate_deactivate_samples() -> list[float]:
    # Lower, slightly longer release than plate activate.
    duration = 0.10
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        envelope = math.exp(-time_seconds * 22.0)
        body = 0.52 * math.sin(2.0 * math.pi * 210.0 * time_seconds)
        click = 0.16 * math.sin(2.0 * math.pi * 480.0 * time_seconds)
        samples.append(0.32 * envelope * (body + click))
    return samples


def door_unlock_samples() -> list[float]:
    # Metallic two-click tumbler, not a plate clack and not a jump blip.
    duration = 0.16
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        if time_seconds < 0.045:
            envelope = math.exp(-time_seconds * 40.0)
            frequency = 2400.0
        else:
            envelope = math.exp(-(time_seconds - 0.045) * 18.0)
            frequency = 980.0
        metallic = math.sin(2.0 * math.pi * frequency * time_seconds)
        overtone = 0.35 * math.sin(2.0 * math.pi * frequency * 1.7 * time_seconds)
        samples.append(0.30 * envelope * (metallic + overtone))
    return samples


def level_goal_complete_samples() -> list[float]:
    # Longer major arpeggio than pickup/respawn; not the descending death cue.
    duration = 0.36
    count = int(SAMPLE_RATE * duration)
    notes = ((0.00, 392.00), (0.09, 523.25), (0.18, 659.25), (0.27, 783.99))
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        value = 0.0
        for start, frequency in notes:
            if time_seconds < start:
                continue
            local = time_seconds - start
            envelope = math.exp(-local * 10.0) * (1.0 - 0.15 * (local / duration))
            value += 0.24 * envelope * math.sin(2.0 * math.pi * frequency * time_seconds)
        samples.append(value)
    return samples


def collectible_collect_samples() -> list[float]:
    # Short high coin sparkle. Higher and briefer than the M61 pickup chime
    # (880/1320) and not the M73 checkpoint arpeggio.
    duration = 0.12
    count = int(SAMPLE_RATE * duration)
    samples = []
    for index in range(count):
        time_seconds = index / SAMPLE_RATE
        if time_seconds < 0.045:
            envelope = math.exp(-time_seconds * 22.0)
            frequency = 1567.98
        else:
            envelope = math.exp(-(time_seconds - 0.045) * 18.0)
            frequency = 2093.00
        sparkle = math.sin(2.0 * math.pi * frequency * time_seconds)
        overtone = 0.28 * math.sin(2.0 * math.pi * frequency * 2.0 * time_seconds)
        samples.append(0.30 * envelope * (sparkle + overtone))
    return samples


DAMAGE_NAME = "player_damage.wav"
DEATH_NAME = "player_death.wav"
RESPAWN_NAME = "player_respawn.wav"
FOOTSTEP_NAME = "player_footstep.wav"
JUMP_NAME = "player_jump.wav"
LAND_NAME = "player_land.wav"
CHECKPOINT_NAME = "checkpoint_activate.wav"
PLATE_ACTIVATE_NAME = "pressure_plate_activate.wav"
PLATE_DEACTIVATE_NAME = "pressure_plate_deactivate.wav"
DOOR_UNLOCK_NAME = "door_unlock.wav"
GOAL_COMPLETE_NAME = "level_goal_complete.wav"
COLLECTIBLE_NAME = "collectible_collect.wav"

CUES = (
    (DAMAGE_NAME, damage_samples),
    (DEATH_NAME, death_samples),
    (RESPAWN_NAME, respawn_samples),
    (FOOTSTEP_NAME, footstep_samples),
    (JUMP_NAME, jump_samples),
    (LAND_NAME, land_samples),
    (CHECKPOINT_NAME, checkpoint_activate_samples),
    (PLATE_ACTIVATE_NAME, pressure_plate_activate_samples),
    (PLATE_DEACTIVATE_NAME, pressure_plate_deactivate_samples),
    (DOOR_UNLOCK_NAME, door_unlock_samples),
    (GOAL_COMPLETE_NAME, level_goal_complete_samples),
    (COLLECTIBLE_NAME, collectible_collect_samples),
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
