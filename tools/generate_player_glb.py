#!/usr/bin/env python3
"""Generate the project-owned skinned Player GLB with M103 locomotion clips.

This is original in-repo geometry, not a marketplace character and not a
third-party mesh. The runtime never reads this generator or source/; cook
and stage copy the written GLB as an opaque static asset.

Re-run from the repository root:

    python tools/generate_player_glb.py

Writes:

    game/assets/source/models/player.glb

glTF / engine convention:
    +Y up
    model local +Z is the authored forward (nose)
    presentation yaw 0 faces world +Z; +X movement uses yaw +90 deg
    one root joint intentionally keeps the canonical placeholder small while
    exercising the complete skin/inverse-bind/animation/runtime path
"""

from __future__ import annotations

import json
import struct
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
OUTPUT = REPO_ROOT / "game" / "assets" / "source" / "models" / "player.glb"

GLB_MAGIC = 0x46546C67
GLB_VERSION = 2
JSON_CHUNK = 0x4E4F534A
BIN_CHUNK = 0x004E4942

BODY_COLOR = (0.847, 0.376, 0.282, 1.0)
NOSE_COLOR = (0.18, 0.12, 0.10, 1.0)


def pad4(data: bytes, pad: bytes) -> bytes:
    remainder = len(data) % 4
    if remainder == 0:
        return data
    return data + pad * (4 - remainder)


def append_box(
    positions: list[float],
    normals: list[float],
    indices: list[int],
    center: tuple[float, float, float],
    size: tuple[float, float, float],
) -> None:
    cx, cy, cz = center
    hx, hy, hz = size[0] * 0.5, size[1] * 0.5, size[2] * 0.5
    faces = (
        ((1.0, 0.0, 0.0), ((hx, -hy, -hz), (hx, -hy, hz), (hx, hy, hz), (hx, hy, -hz))),
        ((-1.0, 0.0, 0.0), ((-hx, -hy, hz), (-hx, -hy, -hz), (-hx, hy, -hz), (-hx, hy, hz))),
        ((0.0, 1.0, 0.0), ((-hx, hy, -hz), (hx, hy, -hz), (hx, hy, hz), (-hx, hy, hz))),
        ((0.0, -1.0, 0.0), ((-hx, -hy, hz), (hx, -hy, hz), (hx, -hy, -hz), (-hx, -hy, -hz))),
        ((0.0, 0.0, 1.0), ((-hx, -hy, hz), (-hx, hy, hz), (hx, hy, hz), (hx, -hy, hz))),
        ((0.0, 0.0, -1.0), ((hx, -hy, -hz), (hx, hy, -hz), (-hx, hy, -hz), (-hx, -hy, -hz))),
    )
    for normal, corners in faces:
        base = len(positions) // 3
        for ox, oy, oz in corners:
            positions.extend((cx + ox, cy + oy, cz + oz))
            normals.extend(normal)
        indices.extend((base, base + 1, base + 2, base, base + 2, base + 3))


def pack_mesh(boxes: list[tuple[tuple[float, float, float], tuple[float, float, float]]]) -> tuple[bytes, bytes, bytes, int, int]:
    positions: list[float] = []
    normals: list[float] = []
    indices: list[int] = []
    for center, size in boxes:
        append_box(positions, normals, indices, center, size)
    position_bytes = struct.pack("<" + "f" * len(positions), *positions)
    normal_bytes = struct.pack("<" + "f" * len(normals), *normals)
    index_bytes = struct.pack("<" + "H" * len(indices), *indices)
    vertex_count = len(positions) // 3
    index_count = len(indices)
    return position_bytes, normal_bytes, index_bytes, vertex_count, index_count


def accessor_minmax(values: list[float], stride: int) -> tuple[list[float], list[float]]:
    mins = [min(values[i::stride]) for i in range(stride)]
    maxs = [max(values[i::stride]) for i in range(stride)]
    return mins, maxs


def unpack_floats(data: bytes) -> list[float]:
    count = len(data) // 4
    return list(struct.unpack("<" + "f" * count, data))


def build_glb() -> bytes:
    body_boxes = [
        ((0.0, 0.08, 0.0), (0.42, 0.62, 0.28)),  # torso
        ((0.0, 0.55, 0.0), (0.32, 0.32, 0.32)),  # head
        ((-0.11, -0.54, 0.0), (0.16, 0.52, 0.16)),  # left leg
        ((0.11, -0.54, 0.0), (0.16, 0.52, 0.16)),  # right leg
        ((-0.30, 0.08, 0.0), (0.12, 0.48, 0.12)),  # left arm
        ((0.30, 0.08, 0.0), (0.12, 0.48, 0.12)),  # right arm
    ]
    nose_boxes = [
        ((0.0, 0.52, 0.22), (0.10, 0.10, 0.14)),  # +Z forward cue
    ]
    body_p, body_n, body_i, body_verts, body_indices = pack_mesh(body_boxes)
    nose_p, nose_n, nose_i, nose_verts, nose_indices = pack_mesh(nose_boxes)

    body_joints = bytes(body_verts * 4)
    nose_joints = bytes(nose_verts * 4)
    body_weights = struct.pack("<" + "f" * body_verts * 4, *([1.0, 0.0, 0.0, 0.0] * body_verts))
    nose_weights = struct.pack("<" + "f" * nose_verts * 4, *([1.0, 0.0, 0.0, 0.0] * nose_verts))
    inverse_bind = struct.pack("<16f", 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
    times = struct.pack("<3f", 0.0, 0.5, 1.0)
    idle_translation = struct.pack("<9f", 0, 0, 0, 0, 0.025, 0, 0, 0, 0)
    move_translation = struct.pack("<9f", 0, 0, 0, 0, 0.06, 0, 0, 0, 0)
    jump_translation = struct.pack("<9f", 0, 0, 0, 0, 0.14, 0, 0, 0.14, 0)
    move_rotation = struct.pack(
        "<12f", 0, 0, 0, 1, 0, 0, 0.0871557, 0.9961947, 0, 0, 0, 1)
    chunks = [body_p, body_n, pad4(body_i, b"\x00"), body_joints, body_weights,
              nose_p, nose_n, pad4(nose_i, b"\x00"), nose_joints, nose_weights,
              inverse_bind, times, idle_translation, move_translation, jump_translation,
              move_rotation]
    offsets = []
    cursor = 0
    for chunk in chunks:
        offsets.append(cursor)
        cursor += len(chunk)
    blob = b"".join(chunks)

    body_pos = unpack_floats(body_p)
    body_norm = unpack_floats(body_n)
    nose_pos = unpack_floats(nose_p)
    nose_norm = unpack_floats(nose_n)
    body_pos_min, body_pos_max = accessor_minmax(body_pos, 3)
    body_norm_min, body_norm_max = accessor_minmax(body_norm, 3)
    nose_pos_min, nose_pos_max = accessor_minmax(nose_pos, 3)
    nose_norm_min, nose_norm_max = accessor_minmax(nose_norm, 3)

    gltf = {
        "asset": {
            "version": "2.0",
            "generator": "Platformer3D M103 project-owned animated player",
        },
        "scene": 0,
        "scenes": [{"nodes": [0, 2]}],
        "nodes": [
            {"mesh": 0, "skin": 0, "name": "player"},
            {"name": "Root"},
            {"name": "Armature", "children": [1]},
        ],
        "skins": [{"name": "PlayerSkin", "joints": [1], "inverseBindMatrices": 10, "skeleton": 1}],
        "meshes": [
            {
                "name": "player",
                "primitives": [
                    {
                        "attributes": {"POSITION": 0, "NORMAL": 1, "JOINTS_0": 3, "WEIGHTS_0": 4},
                        "indices": 2,
                        "material": 0,
                    },
                    {
                        "attributes": {"POSITION": 5, "NORMAL": 6, "JOINTS_0": 8, "WEIGHTS_0": 9},
                        "indices": 7,
                        "material": 1,
                    },
                ],
            }
        ],
        "materials": [
            {
                "name": "player_body",
                "pbrMetallicRoughness": {
                    "baseColorFactor": list(BODY_COLOR),
                    "metallicFactor": 0.0,
                    "roughnessFactor": 0.85,
                },
            },
            {
                "name": "player_forward",
                "pbrMetallicRoughness": {
                    "baseColorFactor": list(NOSE_COLOR),
                    "metallicFactor": 0.0,
                    "roughnessFactor": 0.85,
                },
            },
        ],
        "accessors": [
            {
                "bufferView": 0,
                "componentType": 5126,
                "count": body_verts,
                "type": "VEC3",
                "min": body_pos_min,
                "max": body_pos_max,
            },
            {
                "bufferView": 1,
                "componentType": 5126,
                "count": body_verts,
                "type": "VEC3",
                "min": body_norm_min,
                "max": body_norm_max,
            },
            {
                "bufferView": 2,
                "componentType": 5123,
                "count": body_indices,
                "type": "SCALAR",
            },
            {"bufferView": 3, "componentType": 5121, "count": body_verts, "type": "VEC4"},
            {"bufferView": 4, "componentType": 5126, "count": body_verts, "type": "VEC4"},
            {"bufferView": 5, "componentType": 5126, "count": nose_verts, "type": "VEC3", "min": nose_pos_min, "max": nose_pos_max},
            {"bufferView": 6, "componentType": 5126, "count": nose_verts, "type": "VEC3", "min": nose_norm_min, "max": nose_norm_max},
            {"bufferView": 7, "componentType": 5123, "count": nose_indices, "type": "SCALAR"},
            {"bufferView": 8, "componentType": 5121, "count": nose_verts, "type": "VEC4"},
            {"bufferView": 9, "componentType": 5126, "count": nose_verts, "type": "VEC4"},
            {"bufferView": 10, "componentType": 5126, "count": 1, "type": "MAT4"},
            {"bufferView": 11, "componentType": 5126, "count": 3, "type": "SCALAR", "min": [0.0], "max": [1.0]},
            {"bufferView": 12, "componentType": 5126, "count": 3, "type": "VEC3"},
            {"bufferView": 13, "componentType": 5126, "count": 3, "type": "VEC3"},
            {"bufferView": 14, "componentType": 5126, "count": 3, "type": "VEC3"},
            {"bufferView": 15, "componentType": 5126, "count": 3, "type": "VEC4"},
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": offsets[0], "byteLength": len(body_p), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[1], "byteLength": len(body_n), "target": 34962},
            {
                "buffer": 0,
                "byteOffset": offsets[2],
                "byteLength": len(body_i),
                "target": 34963,
            },
            {"buffer": 0, "byteOffset": offsets[3], "byteLength": len(body_joints), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[4], "byteLength": len(body_weights), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[5], "byteLength": len(nose_p), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[6], "byteLength": len(nose_n), "target": 34962},
            {
                "buffer": 0,
                "byteOffset": offsets[7],
                "byteLength": len(nose_i),
                "target": 34963,
            },
            {"buffer": 0, "byteOffset": offsets[8], "byteLength": len(nose_joints), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[9], "byteLength": len(nose_weights), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[10], "byteLength": len(inverse_bind)},
            {"buffer": 0, "byteOffset": offsets[11], "byteLength": len(times)},
            {"buffer": 0, "byteOffset": offsets[12], "byteLength": len(idle_translation)},
            {"buffer": 0, "byteOffset": offsets[13], "byteLength": len(move_translation)},
            {"buffer": 0, "byteOffset": offsets[14], "byteLength": len(jump_translation)},
            {"buffer": 0, "byteOffset": offsets[15], "byteLength": len(move_rotation)},
        ],
        "buffers": [{"byteLength": len(blob)}],
        "animations": [
            {"name": "Idle", "samplers": [{"input": 11, "output": 12, "interpolation": "LINEAR"}],
             "channels": [{"sampler": 0, "target": {"node": 1, "path": "translation"}}]},
            {"name": "Move", "samplers": [
                {"input": 11, "output": 13, "interpolation": "LINEAR"},
                {"input": 11, "output": 15, "interpolation": "LINEAR"}],
             "channels": [
                {"sampler": 0, "target": {"node": 1, "path": "translation"}},
                {"sampler": 1, "target": {"node": 1, "path": "rotation"}}]},
            {"name": "Jump", "samplers": [{"input": 11, "output": 14, "interpolation": "LINEAR"}],
             "channels": [{"sampler": 0, "target": {"node": 1, "path": "translation"}}]},
        ],
    }
    json_bytes = pad4(json.dumps(gltf, separators=(",", ":")).encode("utf-8"), b" ")
    bin_bytes = pad4(blob, b"\x00")
    total = 12 + 8 + len(json_bytes) + 8 + len(bin_bytes)
    header = struct.pack("<III", GLB_MAGIC, GLB_VERSION, total)
    json_header = struct.pack("<II", len(json_bytes), JSON_CHUNK)
    bin_header = struct.pack("<II", len(bin_bytes), BIN_CHUNK)
    return header + json_header + json_bytes + bin_header + bin_bytes


def main() -> int:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(build_glb())
    print(f"Wrote {OUTPUT.as_posix()} ({OUTPUT.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
