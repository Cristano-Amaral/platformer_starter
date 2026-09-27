#!/usr/bin/env python3
"""Generate two tiny original M109 skinned GLBs from the project-owned Player mesh.

The source keeps conventional role names and Player's real Move/Jump clips.
The target prefixes every joint name and has a different Hips rest orientation.
Both keep the M108 19-joint hierarchy; neither alters the M108 regression asset.
"""

import json
import math
import struct
from pathlib import Path

from generate_player_glb import build_glb, pad4
from generate_humanoid_mapping_fixture import NAMES, PARENTS

OUTPUT_ROOT = Path(__file__).resolve().parent.parent / "game/assets/source/models"


def generate(target: bool) -> bytes:
    original = build_glb()
    json_length = struct.unpack_from("<I", original, 12)[0]
    data = json.loads(original[20:20 + json_length])
    blob = original[20 + json_length + 8:]
    names = ["Target" + name if target else name for name in NAMES]
    nodes = [{"mesh": 0, "skin": 0, "name": "retarget_target" if target else "retarget_source"}]
    nodes += [{"name": name} for name in names]
    nodes += [{"name": "Armature", "children": [1]}]
    for index, parent in enumerate(PARENTS):
        if parent >= 0:
            nodes[parent + 1].setdefault("children", []).append(index + 1)
    if target:
        half = math.pi / 4
        nodes[1]["rotation"] = [0, math.sin(half), 0, math.cos(half)]
        # The mesh is authored in bind position, so the inverse bind cancels
        # the target's 90-degree Y rest rotation.
        inverse_root = [0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1]
    else:
        inverse_root = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    identity = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    matrices = struct.pack("<" + "f" * (16 * len(names)),
        *(inverse_root + identity * (len(names) - 1)))
    data["nodes"] = nodes
    data["scenes"] = [{"nodes": [0, len(nodes) - 1]}]
    data["bufferViews"].append({"buffer": 0, "byteOffset": len(blob), "byteLength": len(matrices)})
    data["accessors"].append({"bufferView": len(data["bufferViews"]) - 1,
        "componentType": 5126, "count": len(names), "type": "MAT4"})
    data["skins"] = [{"name": "RetargetSkin", "joints": list(range(1, len(names) + 1)),
        "inverseBindMatrices": len(data["accessors"]) - 1, "skeleton": 1}]
    data["buffers"][0]["byteLength"] = len(blob) + len(matrices)
    data["asset"]["generator"] = "Platformer3D M109 original retarget fixture"
    json_bytes = pad4(json.dumps(data, separators=(",", ":")).encode(), b" ")
    bin_bytes = pad4(blob + matrices, b"\0")
    return (struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(json_bytes) + 8 + len(bin_bytes))
        + struct.pack("<II", len(json_bytes), 0x4E4F534A) + json_bytes
        + struct.pack("<II", len(bin_bytes), 0x004E4942) + bin_bytes)


def main() -> None:
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    for target in (False, True):
        path = OUTPUT_ROOT / ("retarget_target.glb" if target else "retarget_source.glb")
        path.write_bytes(generate(target))
        print(path)


if __name__ == "__main__":
    main()
