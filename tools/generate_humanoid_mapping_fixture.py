#!/usr/bin/env python3
"""Generate an original, tiny multi-joint GLB for M108 mapping tests and editor inspection."""

import json
import struct
from pathlib import Path

from generate_player_glb import build_glb, pad4

OUTPUT = Path(__file__).resolve().parent.parent / "game/assets/source/models/humanoid_mapping_fixture.glb"

NAMES = [
    "Hips", "Spine", "Chest", "Neck", "Head",
    "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
    "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand",
    "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
    "RightUpperLeg", "RightLowerLeg", "RightFoot",
]
# Parent indices in NAMES; -1 denotes the armature root.
PARENTS = [-1, 0, 1, 2, 3, 2, 5, 6, 7, 2, 9, 10, 11, 0, 13, 14, 0, 16, 17]


def main() -> None:
    original = build_glb()
    json_length = struct.unpack_from("<I", original, 12)[0]
    data = json.loads(original[20:20 + json_length])
    bin_start = 20 + json_length + 8
    blob = original[bin_start:]
    nodes = [{"mesh": 0, "skin": 0, "name": "humanoid_fixture"}]
    nodes += [{"name": name} for name in NAMES]
    nodes += [{"name": "Armature", "children": [1]}]
    for index, parent in enumerate(PARENTS):
        if parent >= 0:
            nodes[parent + 1].setdefault("children", []).append(index + 1)
    data["nodes"] = nodes
    data["scenes"] = [{"nodes": [0, len(nodes) - 1]}]
    matrices = struct.pack("<" + "f" * 16 * len(NAMES),
        *([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1] * len(NAMES)))
    data["bufferViews"].append({"buffer": 0, "byteOffset": len(blob), "byteLength": len(matrices)})
    data["accessors"].append({"bufferView": len(data["bufferViews"]) - 1,
        "componentType": 5126, "count": len(NAMES), "type": "MAT4"})
    data["skins"] = [{"name": "HumanoidFixture", "joints": list(range(1, 20)),
        "inverseBindMatrices": len(data["accessors"]) - 1, "skeleton": 1}]
    data["buffers"][0]["byteLength"] = len(blob) + len(matrices)
    data["asset"]["generator"] = "Platformer3D M108 original humanoid mapping fixture"
    json_bytes = pad4(json.dumps(data, separators=(",", ":")).encode(), b" ")
    bin_bytes = pad4(blob + matrices, b"\0")
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(struct.pack("<III", 0x46546C67, 2,
        12 + 8 + len(json_bytes) + 8 + len(bin_bytes))
        + struct.pack("<II", len(json_bytes), 0x4E4F534A) + json_bytes
        + struct.pack("<II", len(bin_bytes), 0x004E4942) + bin_bytes)
    print(OUTPUT)


if __name__ == "__main__":
    main()
