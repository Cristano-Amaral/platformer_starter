#!/usr/bin/env python3
"""Copy known runtime-source assets into deterministic cooked output.

Run from the repository root:

    python tools/cook_assets.py

Paths are resolved from this file's location so the cooker does not depend on
the current working directory.

Standalone runtime PNGs (`kind: runtime_png`) are listed explicitly and may be
downscaled with cooker-only Pillow. Milestone 88 additionally cooks Terrain
textures referenced by authored Levels; unrelated `source/textures/*.png` files
are not globbed. Blender authoring PNGs and `.blend` files are not cooker
inputs unless a Level references a PNG. Known GLBs plus extra valid
`source/models/*.glb` files are opaque copies after static-GLB compatibility
checks. Extra valid `source/levels/*.level` files are cooked as `level_v1`
after the header
check so a newly created Level can enter cook/stage without a per-level
hardcoded list. Cooked `levels/*.level` files that are no longer in the
current required-plus-discovered Level inventory are removed; other cooked
categories are not scanned. The M61 collection WAV and later gameplay SFX
WAVs are explicit opaque `copy`. Canonical Level v1 files (`kind: level_v1`)
are UTF-8 text copies after a header check; C++ owns full grammar validation.
"""

from __future__ import annotations

import hashlib
import json
import struct
import sys
from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
MANIFEST_NAME = "manifest.json"

# Asset kinds are declarative. Do not glob/discover every PNG under source/textures.
# Extra valid source/models/*.glb files are discovered (M47). Explicit runtime
# PNGs remain listed; M88 additionally cooks Terrain textures referenced by
# authored Levels. Extra valid source/levels/*.level files are discovered
# (M64.1). Canonical level_01/level_02 remain required inventory.
# Milestone 85 lighting shaders are explicit opaque `copy`.
#   copy         = opaque byte copy (GLBs and the M61 collection WAV;
#                  embedded GLB images are not inspected)
#   runtime_png  = standalone runtime PNG (M19 policy applies to these only)
KIND_COPY = "copy"
KIND_RUNTIME_PNG = "runtime_png"
KIND_LEVEL_V1 = "level_v1"

RUNTIME_PNG_MAX_DIMENSION = 512
RUNTIME_PNG_RECIPE = "runtime_png.max512.lanczos.v1"
PILLOW_PIN = "12.3.0"
PNG_SAVE_COMPRESS_LEVEL = 6

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
PNG_IHDR_MIN_BYTES = 33

# Logical identities are portable relative paths. Never use absolute paths or UUIDs.
# .blend files are authoring-only and must never appear here.
# Blender authoring PNGs (e.g. textures/test_textured_basecolor.png) are not
# runtime assets and must not be listed here.
# Cooker test fixtures under tools/fixtures/ are not runtime assets.
KNOWN_ASSETS = (
    {
        "id": "models/player.glb",
        "source": "models/player.glb",
        "cooked": "models/player.glb",
        "kind": KIND_COPY,
    },
    {
        "id": "models/test_authored.glb",
        "source": "models/test_authored.glb",
        "cooked": "models/test_authored.glb",
        "kind": KIND_COPY,
    },
    {
        "id": "models/test_static.glb",
        "source": "models/test_static.glb",
        "cooked": "models/test_static.glb",
        "kind": KIND_COPY,
    },
    {
        "id": "models/test_textured.glb",
        "source": "models/test_textured.glb",
        "cooked": "models/test_textured.glb",
        "kind": KIND_COPY,
    },
    {
        "id": "textures/test_checker.png",
        "source": "textures/test_checker.png",
        "cooked": "textures/test_checker.png",
        "kind": KIND_RUNTIME_PNG,
    },
    {
        "id": "textures/test_ground_cover_tuft.png",
        "source": "textures/test_ground_cover_tuft.png",
        "cooked": "textures/test_ground_cover_tuft.png",
        "kind": KIND_RUNTIME_PNG,
    },
    {
        "id": "levels/level_01.level",
        "source": "levels/level_01.level",
        "cooked": "levels/level_01.level",
        "kind": KIND_LEVEL_V1,
    },
    {
        "id": "levels/level_02.level",
        "source": "levels/level_02.level",
        "cooked": "levels/level_02.level",
        "kind": KIND_LEVEL_V1,
    },
    {
        "id": "gameplay/definitions.gameplay",
        "source": "gameplay/definitions.gameplay",
        "cooked": "gameplay/definitions.gameplay",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/item_pickup_collect.wav",
        "source": "sounds/item_pickup_collect.wav",
        "cooked": "sounds/item_pickup_collect.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/player_damage.wav",
        "source": "sounds/player_damage.wav",
        "cooked": "sounds/player_damage.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/player_death.wav",
        "source": "sounds/player_death.wav",
        "cooked": "sounds/player_death.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/player_respawn.wav",
        "source": "sounds/player_respawn.wav",
        "cooked": "sounds/player_respawn.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/player_footstep.wav",
        "source": "sounds/player_footstep.wav",
        "cooked": "sounds/player_footstep.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/player_jump.wav",
        "source": "sounds/player_jump.wav",
        "cooked": "sounds/player_jump.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/player_land.wav",
        "source": "sounds/player_land.wav",
        "cooked": "sounds/player_land.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/checkpoint_activate.wav",
        "source": "sounds/checkpoint_activate.wav",
        "cooked": "sounds/checkpoint_activate.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/pressure_plate_activate.wav",
        "source": "sounds/pressure_plate_activate.wav",
        "cooked": "sounds/pressure_plate_activate.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/pressure_plate_deactivate.wav",
        "source": "sounds/pressure_plate_deactivate.wav",
        "cooked": "sounds/pressure_plate_deactivate.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/door_unlock.wav",
        "source": "sounds/door_unlock.wav",
        "cooked": "sounds/door_unlock.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/level_goal_complete.wav",
        "source": "sounds/level_goal_complete.wav",
        "cooked": "sounds/level_goal_complete.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/collectible_collect.wav",
        "source": "sounds/collectible_collect.wav",
        "cooked": "sounds/collectible_collect.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/ui_navigate.wav",
        "source": "sounds/ui_navigate.wav",
        "cooked": "sounds/ui_navigate.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/ui_confirm.wav",
        "source": "sounds/ui_confirm.wav",
        "cooked": "sounds/ui_confirm.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/pause_open.wav",
        "source": "sounds/pause_open.wav",
        "cooked": "sounds/pause_open.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/pause_close.wav",
        "source": "sounds/pause_close.wav",
        "cooked": "sounds/pause_close.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/inventory_open.wav",
        "source": "sounds/inventory_open.wav",
        "cooked": "sounds/inventory_open.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "sounds/inventory_close.wav",
        "source": "sounds/inventory_close.wav",
        "cooked": "sounds/inventory_close.wav",
        "kind": KIND_COPY,
    },
    {
        "id": "shaders/world_lit.vs",
        "source": "shaders/world_lit.vs",
        "cooked": "shaders/world_lit.vs",
        "kind": KIND_COPY,
    },
    {
        "id": "shaders/world_lit.fs",
        "source": "shaders/world_lit.fs",
        "cooked": "shaders/world_lit.fs",
        "kind": KIND_COPY,
    },
    {
        "id": "shaders/shadow_depth.vs",
        "source": "shaders/shadow_depth.vs",
        "cooked": "shaders/shadow_depth.vs",
        "kind": KIND_COPY,
    },
    {
        "id": "shaders/shadow_depth.fs",
        "source": "shaders/shadow_depth.fs",
        "cooked": "shaders/shadow_depth.fs",
        "kind": KIND_COPY,
    },
)


class CookError(Exception):
    pass


@dataclass(frozen=True)
class RuntimePngCookResult:
    cooked_data: bytes
    source_width: int
    source_height: int
    cooked_width: int
    cooked_height: int
    recipe: str
    resized: bool


def portable_relative(path: str) -> str:
    return Path(path).as_posix()


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def source_root(root: Path) -> Path:
    return root / "game" / "assets" / "source"


def cooked_root(root: Path) -> Path:
    return root / "game" / "assets" / "cooked"


def display_repo_path(root: Path, path: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def pillow_missing_message() -> str:
    return (
        f"Pillow {PILLOW_PIN} is required for runtime PNG cooking.\n"
        "Install with:\n"
        "  python -m pip install -r tools/requirements.txt"
    )


def require_pillow():
    try:
        from PIL import Image
    except ImportError as exc:
        raise CookError(pillow_missing_message()) from exc
    return Image


def read_png_dimensions(data: bytes) -> tuple[int, int]:
    """Read width/height from PNG IHDR. Does not decode pixels."""
    if len(data) < 8:
        raise CookError("truncated PNG (too small to contain signature)")
    if data[:8] != PNG_SIGNATURE:
        raise CookError("not a PNG (invalid signature)")
    if len(data) < PNG_IHDR_MIN_BYTES:
        raise CookError("truncated PNG (too small to contain IHDR)")
    chunk_length = struct.unpack(">I", data[8:12])[0]
    if chunk_length != 13:
        raise CookError("invalid PNG IHDR chunk length")
    if data[12:16] != b"IHDR":
        raise CookError("PNG missing IHDR as first chunk")
    width, height = struct.unpack(">II", data[16:24])
    if width == 0 or height == 0:
        raise CookError(f"PNG has invalid dimensions {width}x{height}")
    return width, height


def scaled_png_dimensions(
    width: int,
    height: int,
    max_dimension: int = RUNTIME_PNG_MAX_DIMENSION,
) -> tuple[int, int]:
    """Return cooked pixel size. Never upscales. Preserves aspect ratio."""
    if width <= 0 or height <= 0:
        raise CookError(f"PNG has invalid dimensions {width}x{height}")
    if max_dimension <= 0:
        raise CookError(f"invalid max dimension {max_dimension}")
    if width <= max_dimension and height <= max_dimension:
        return width, height
    if width >= height:
        cooked_width = max_dimension
        cooked_height = max(1, round(height * max_dimension / width))
    else:
        cooked_height = max_dimension
        cooked_width = max(1, round(width * max_dimension / height))
    return cooked_width, cooked_height


def encode_png_bytes(image: Any) -> bytes:
    """Encode PNG with explicit, machine-independent save options."""
    clean = image.copy()
    clean.info.clear()
    buffer = BytesIO()
    clean.save(
        buffer,
        format="PNG",
        optimize=False,
        compress_level=PNG_SAVE_COMPRESS_LEVEL,
    )
    return buffer.getvalue()


def cook_runtime_png_bytes(
    source_data: bytes,
    *,
    max_dimension: int = RUNTIME_PNG_MAX_DIMENSION,
    recipe: str = RUNTIME_PNG_RECIPE,
) -> RuntimePngCookResult:
    source_width, source_height = read_png_dimensions(source_data)
    cooked_width, cooked_height = scaled_png_dimensions(
        source_width, source_height, max_dimension
    )
    if cooked_width == source_width and cooked_height == source_height:
        return RuntimePngCookResult(
            cooked_data=source_data,
            source_width=source_width,
            source_height=source_height,
            cooked_width=cooked_width,
            cooked_height=cooked_height,
            recipe=recipe,
            resized=False,
        )

    Image = require_pillow()
    try:
        with Image.open(BytesIO(source_data)) as image:
            image.load()
            working = image
            if working.mode not in ("RGB", "RGBA", "L", "LA"):
                working = working.convert("RGBA")
            resized = working.resize(
                (cooked_width, cooked_height),
                resample=Image.Resampling.LANCZOS,
            )
            cooked_data = encode_png_bytes(resized)
    except CookError:
        raise
    except Exception as exc:
        raise CookError(f"failed to decode or resize PNG: {exc}") from exc

    return RuntimePngCookResult(
        cooked_data=cooked_data,
        source_width=source_width,
        source_height=source_height,
        cooked_width=cooked_width,
        cooked_height=cooked_height,
        recipe=recipe,
        resized=True,
    )


def read_bytes(path: Path, *, label: str) -> bytes:
    try:
        return path.read_bytes()
    except OSError as exc:
        raise CookError(f"unreadable {label}: {path.as_posix()}\n{exc}") from exc


def write_bytes_if_changed(path: Path, data: bytes) -> bool:
    if path.is_file():
        try:
            if path.read_bytes() == data:
                return False
        except OSError as exc:
            raise CookError(f"cannot read existing output: {path.as_posix()}\n{exc}") from exc

    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = path.with_name(path.name + ".tmp")
        temporary.write_bytes(data)
        temporary.replace(path)
    except OSError as ext:
        raise CookError(f"failed to create or write output: {path.as_posix()}\n{ext}") from ext
    return True


def load_previous_manifest(path: Path) -> dict[str, Any]:
    if not path.is_file():
        return {}
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    if not isinstance(payload, dict):
        return {}
    return payload


def runtime_png_output_is_current(
    previous_entry: dict[str, Any],
    source_hash: str,
    recipe: str,
) -> bool:
    """sourceSha256 + recipe must both match before cooked PNG bytes can be reused."""
    return (
        previous_entry.get("sourceSha256") == source_hash
        and previous_entry.get("recipe") == recipe
    )


def safe_cooked_file(cooked_dir: Path, relative: str) -> Path | None:
    relative_path = Path(portable_relative(relative))
    if relative_path.is_absolute() or ".." in relative_path.parts:
        return None
    candidate = (cooked_dir / relative_path).resolve()
    try:
        candidate.relative_to(cooked_dir.resolve())
    except ValueError:
        return None
    return candidate


def build_manifest(entries: list[dict[str, Any]]) -> str:
    payload = {
        "assets": sorted(entries, key=lambda item: item["id"]),
        "schemaVersion": SCHEMA_VERSION,
    }
    return json.dumps(payload, indent=2, sort_keys=True) + "\n"


def remove_stale_outputs(
    cooked_dir: Path,
    previous_manifest: dict[str, Any],
    current_ids: set[str],
) -> None:
    previous_assets = previous_manifest.get("assets")
    if not isinstance(previous_assets, list):
        return

    for item in previous_assets:
        if not isinstance(item, dict):
            continue
        identity = item.get("id")
        cooked_relative = item.get("cooked")
        if not isinstance(identity, str) or identity in current_ids:
            continue
        if not isinstance(cooked_relative, str):
            continue
        stale = safe_cooked_file(cooked_dir, cooked_relative)
        if stale is None or not stale.is_file():
            continue
        try:
            stale.unlink()
            print(f"[removed-stale] {portable_relative(cooked_relative)}")
        except OSError as exc:
            raise CookError(
                f"failed to remove stale cooked output: {stale.as_posix()}\n{exc}"
            ) from exc


def current_level_v1_identities(assets: list[dict[str, str]]) -> set[str]:
    return {
        portable_relative(asset["id"])
        for asset in assets
        if asset.get("kind") == KIND_LEVEL_V1
    }


def remove_stale_cooked_levels(cooked_dir: Path, current_level_ids: set[str]) -> None:
    """Converge cooked/levels/*.level to the current Level inventory.

    Ownership is the Level category only: non-recursive `*.level` files under
    cooked/levels/. Unrelated cooked categories are not scanned or deleted.
    This also removes leftovers that were never recorded in manifest.json.
    """
    levels_dir = cooked_dir / LEVELS_DIRECTORY
    if not levels_dir.is_dir():
        return
    for path in sorted(levels_dir.iterdir(), key=lambda item: item.name):
        if not path.is_file() or not path.name.endswith(LEVEL_SUFFIX):
            continue
        identity = portable_relative(f"{LEVELS_DIRECTORY}/{path.name}")
        if identity in current_level_ids:
            continue
        stale = safe_cooked_file(cooked_dir, identity)
        if stale is None or not stale.is_file():
            continue
        try:
            stale.unlink()
            print(f"[removed-stale] {identity}")
        except OSError as exc:
            raise CookError(
                f"failed to remove stale cooked output: {stale.as_posix()}\n{exc}"
            ) from exc


GLB_MAGIC = b"glTF"
GLB_VERSION = 2
GLB_JSON_CHUNK = 0x4E4F534A
GLB_BIN_CHUNK = 0x004E4942
STATIC_MODELS_DIRECTORY = "models"
STATIC_GLB_SUFFIX = ".glb"
LEVELS_DIRECTORY = "levels"
LEVEL_SUFFIX = ".level"
STATIC_GLB_IMPORT_TEMP_SUFFIX = ".importing.tmp"
RUNTIME_TEXTURES_DIRECTORY = "textures"
RUNTIME_PNG_SUFFIX = ".png"
RUNTIME_PNG_NONE_TOKEN = "-"


def _glb_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def validate_static_glb(data: bytes) -> None:
    """Reject GLBs that are not self-contained static glTF 2.0 binaries.

    Import is the Development gate; the cooker reuses the same checks so a
    discovered models/*.glb cannot enter cooked output with different rules.
    """
    if len(data) < 12:
        raise CookError("GLB is too small to contain a header")
    if data[:4] != GLB_MAGIC:
        raise CookError("not a GLB (missing glTF magic)")
    version = _glb_u32(data, 4)
    if version != GLB_VERSION:
        raise CookError("unsupported GLB version (expected glTF 2.0 binary)")
    declared = _glb_u32(data, 8)
    if declared != len(data):
        raise CookError("GLB length does not match file size")

    offset = 12
    json_text: str | None = None
    saw_bin = False
    while offset < len(data):
        if offset + 8 > len(data):
            raise CookError("truncated GLB chunk header")
        chunk_length = _glb_u32(data, offset)
        chunk_type = _glb_u32(data, offset + 4)
        offset += 8
        if offset + chunk_length > len(data):
            raise CookError("truncated GLB chunk payload")
        if chunk_length % 4 != 0:
            raise CookError("GLB chunk length is not 4-byte aligned")
        payload = data[offset : offset + chunk_length]
        offset += chunk_length
        if chunk_type == GLB_JSON_CHUNK:
            if json_text is not None:
                raise CookError("GLB contains more than one JSON chunk")
            json_text = payload.decode("utf-8", errors="strict").rstrip(" \0")
        elif chunk_type == GLB_BIN_CHUNK:
            saw_bin = True
        else:
            raise CookError("GLB contains an unsupported chunk type")

    if json_text is None:
        raise CookError("GLB is missing the JSON chunk")
    if not saw_bin:
        raise CookError("static GLB must embed binary data in a BIN chunk")
    try:
        payload_json = json.loads(json_text)
    except json.JSONDecodeError as exc:
        raise CookError(f"GLB JSON is malformed: {exc}") from exc
    if not isinstance(payload_json, dict):
        raise CookError("GLB JSON chunk is not an object")
    asset = payload_json.get("asset")
    if not isinstance(asset, dict) or not str(asset.get("version", "")).startswith("2"):
        raise CookError("GLB JSON is not glTF 2.x")
    meshes = payload_json.get("meshes")
    if not isinstance(meshes, list) or not meshes:
        raise CookError("static GLB must contain at least one mesh")
    animations = payload_json.get("animations")
    skins = payload_json.get("skins")
    has_animations = isinstance(animations, list) and bool(animations)
    has_skins = isinstance(skins, list) and bool(skins)
    if has_animations != has_skins:
        raise CookError("animated character GLB must contain both a skin and animation clips")
    if has_animations:
        if len(skins) != 1:
            raise CookError("animated character GLB must contain exactly one skin")
        skin = skins[0]
        if not isinstance(skin, dict) or not skin.get("joints") or "inverseBindMatrices" not in skin:
            raise CookError("animated character skin requires joints and inverse bind matrices")
        for animation in animations:
            if not isinstance(animation, dict) or not animation.get("name"):
                raise CookError("animated character clips must be named")
            for sampler in animation.get("samplers") or []:
                if sampler.get("interpolation", "LINEAR") not in ("LINEAR", "STEP"):
                    raise CookError("animated character uses unsupported interpolation")
            for channel in animation.get("channels") or []:
                path = (channel.get("target") or {}).get("path")
                if path not in ("translation", "rotation", "scale"):
                    raise CookError("animated character uses unsupported channel path")
    for buffer in payload_json.get("buffers") or []:
        if isinstance(buffer, dict) and buffer.get("uri"):
            raise CookError("GLB must be self-contained (no external buffer URIs)")
    for image in payload_json.get("images") or []:
        if not isinstance(image, dict):
            raise CookError("GLB images must be embedded")
        uri = image.get("uri")
        if uri:
            if not str(uri).startswith("data:"):
                raise CookError(
                    "GLB images must be embedded (bufferView or data URI); "
                    "external files are rejected"
                )
        elif "bufferView" not in image:
            raise CookError(
                "GLB images must be embedded (bufferView or data URI); "
                "external files are rejected"
            )


def is_safe_static_glb_file_name(name: str) -> bool:
    if not name or not name.endswith(STATIC_GLB_SUFFIX) or name.startswith("."):
        return False
    if STATIC_GLB_IMPORT_TEMP_SUFFIX in name:
        return False
    stem = name[: -len(STATIC_GLB_SUFFIX)]
    if not stem or stem in {".", ".."}:
        return False
    if any(ch in name for ch in '\\/:*?"<>|') or any(ord(ch) < 32 for ch in name):
        return False
    if name[0] == " " or name[-1] == " " or stem[-1] in ". ":
        return False
    return True


def discover_extra_static_glb_assets(sources: Path) -> list[dict[str, str]]:
    """models/*.glb not already listed in KNOWN_ASSETS. Non-recursive."""
    models = sources / STATIC_MODELS_DIRECTORY
    extras: list[dict[str, str]] = []
    if not models.is_dir():
        return extras
    for path in sorted(models.iterdir(), key=lambda item: item.name):
        if not path.is_file():
            continue
        name = path.name
        if not is_safe_static_glb_file_name(name):
            continue
        identity = portable_relative(f"{STATIC_MODELS_DIRECTORY}/{name}")
        extras.append(
            {
                "id": identity,
                "source": identity,
                "cooked": identity,
                "kind": KIND_COPY,
            }
        )
    return extras


def is_valid_level_id_token(token: str) -> bool:
    if not token:
        return False
    first = token[0]
    if not (("A" <= first <= "Z") or ("a" <= first <= "z") or first == "_"):
        return False
    return all(
        ("A" <= ch <= "Z") or ("a" <= ch <= "z") or ("0" <= ch <= "9") or ch == "_"
        for ch in token
    )


def is_safe_level_file_name(name: str) -> bool:
    if not name or not name.endswith(LEVEL_SUFFIX) or name.startswith("."):
        return False
    stem = name[: -len(LEVEL_SUFFIX)]
    return is_valid_level_id_token(stem)


def discover_extra_level_v1_assets(sources: Path) -> list[dict[str, str]]:
    """levels/*.level not already listed in KNOWN_ASSETS. Non-recursive."""
    levels = sources / LEVELS_DIRECTORY
    extras: list[dict[str, str]] = []
    if not levels.is_dir():
        return extras
    for path in sorted(levels.iterdir(), key=lambda item: item.name):
        if not path.is_file():
            continue
        name = path.name
        if not is_safe_level_file_name(name):
            continue
        identity = portable_relative(f"{LEVELS_DIRECTORY}/{name}")
        extras.append(
            {
                "id": identity,
                "source": identity,
                "cooked": identity,
                "kind": KIND_LEVEL_V1,
            }
        )
    return extras


def is_safe_runtime_png_file_name(name: str) -> bool:
    if not name or not name.endswith(RUNTIME_PNG_SUFFIX) or name.startswith("."):
        return False
    stem = name[: -len(RUNTIME_PNG_SUFFIX)]
    if not stem or stem in {".", ".."}:
        return False
    if any(ch in name for ch in '\\/:*?"<>| ') or any(ord(ch) < 32 for ch in name):
        return False
    if name[0] == " " or name[-1] == " " or stem[-1] in ". ":
        return False
    return True


def is_valid_runtime_png_identity(identity: str) -> bool:
    if not identity or "\\" in identity:
        return False
    prefix = f"{RUNTIME_TEXTURES_DIRECTORY}/"
    if not identity.startswith(prefix):
        return False
    name = identity[len(prefix) :]
    if not is_safe_runtime_png_file_name(name):
        return False
    return portable_relative(f"{RUNTIME_TEXTURES_DIRECTORY}/{name}") == identity


def iter_level_source_files(sources: Path) -> list[Path]:
    levels = sources / LEVELS_DIRECTORY
    if not levels.is_dir():
        return []
    files: list[Path] = []
    for path in sorted(levels.iterdir(), key=lambda item: item.name):
        if path.is_file() and is_safe_level_file_name(path.name):
            files.append(path)
    return files


def is_valid_static_model_identity(identity: str) -> bool:
    """models/<file>.glb, matching the cooker file-name rules. Spaces allowed."""
    if not identity or "\\" in identity:
        return False
    prefix = f"{STATIC_MODELS_DIRECTORY}/"
    if not identity.startswith(prefix):
        return False
    name = identity[len(prefix) :]
    if "/" in name or not is_safe_static_glb_file_name(name):
        return False
    return portable_relative(f"{STATIC_MODELS_DIRECTORY}/{name}") == identity


def extract_terrain_vegetation_model_identities(level_text: str) -> list[str]:
    """Model identities named by terrain_veg_entry. Not static_prop records."""
    identities: list[str] = []
    for raw_line in level_text.splitlines():
        stripped = raw_line.strip(" \t")
        if not stripped:
            continue
        tokens = stripped.split()
        if not tokens or tokens[0] != "terrain_veg_entry" or len(tokens) < 8:
            continue
        identity = " ".join(tokens[7:])
        if is_valid_static_model_identity(identity):
            identities.append(identity)
    return identities


def discover_level_referenced_static_glb_assets(sources: Path) -> list[dict[str, str]]:
    """GLBs named only by Terrain vegetation. Missing source files are skipped."""
    extras: list[dict[str, str]] = []
    seen: set[str] = set()
    for path in iter_level_source_files(sources):
        try:
            text = path.read_text(encoding="utf-8")
        except OSError:
            continue
        for identity in extract_terrain_vegetation_model_identities(text):
            if identity in seen:
                continue
            if not (sources / identity).is_file():
                continue
            seen.add(identity)
            extras.append(
                {
                    "id": identity,
                    "source": identity,
                    "cooked": identity,
                    "kind": KIND_COPY,
                }
            )
    extras.sort(key=lambda item: item["id"])
    return extras


def extract_terrain_texture_identities(level_text: str) -> list[str]:
    identities: list[str] = []
    for raw_line in level_text.splitlines():
        stripped = raw_line.strip(" \t")
        if not stripped:
            continue
        tokens = stripped.split()
        if not tokens:
            continue
        identity = None
        if tokens[0] == "terrain_material" and 3 <= len(tokens) <= 5:
            for token in tokens[1:2] + tokens[3:]:
                if token != RUNTIME_PNG_NONE_TOKEN and is_valid_runtime_png_identity(token):
                    identities.append(token)
            continue
        elif tokens[0] == "terrain_layer" and 4 <= len(tokens) <= 6:
            for token in tokens[2:3] + tokens[4:]:
                if token != RUNTIME_PNG_NONE_TOKEN and is_valid_runtime_png_identity(token):
                    identities.append(token)
            continue
        elif tokens[0] == "terrain_cover_entry" and len(tokens) >= 8:
            identity = " ".join(tokens[7:])
        else:
            continue
        if identity == RUNTIME_PNG_NONE_TOKEN:
            continue
        if is_valid_runtime_png_identity(identity):
            identities.append(identity)
    return identities


def discover_level_referenced_runtime_png_assets(sources: Path) -> list[dict[str, str]]:
    """runtime_png identities referenced by Terrain material, layer, and
    ground-cover records.

    Does not glob source/textures. Unrelated PNGs stay out of cooked output
    unless a Level names them. Explicit KNOWN_ASSETS entries still win.
    """
    extras: list[dict[str, str]] = []
    seen: set[str] = set()
    for path in iter_level_source_files(sources):
        try:
            text = path.read_text(encoding="utf-8")
        except OSError:
            continue
        for identity in extract_terrain_texture_identities(text):
            if identity in seen:
                continue
            seen.add(identity)
            extras.append(
                {
                    "id": identity,
                    "source": identity,
                    "cooked": identity,
                    "kind": KIND_RUNTIME_PNG,
                }
            )
    extras.sort(key=lambda item: item["id"])
    return extras


def collect_cook_assets(sources: Path) -> list[dict[str, str]]:
    merged: dict[str, dict[str, str]] = {
        portable_relative(asset["id"]): dict(asset) for asset in KNOWN_ASSETS
    }
    for extra in discover_extra_static_glb_assets(sources):
        merged.setdefault(extra["id"], extra)
    for extra in discover_level_referenced_static_glb_assets(sources):
        merged.setdefault(extra["id"], extra)
    for extra in discover_extra_level_v1_assets(sources):
        merged.setdefault(extra["id"], extra)
    for extra in discover_level_referenced_runtime_png_assets(sources):
        merged.setdefault(extra["id"], extra)
    return [merged[key] for key in sorted(merged)]


def validate_level_v1_header(data: bytes) -> None:
    """Cheap header gate only. C++ ParseLevelText is the format authority."""
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise CookError(f"level file is not UTF-8: {exc}") from exc
    if text.startswith("\ufeff"):
        raise CookError("level file must not start with a UTF-8 BOM")
    first = ""
    for raw_line in text.splitlines():
        stripped = raw_line.strip(" \t")
        if stripped:
            first = stripped
            break
    tokens = first.split()
    if len(tokens) == 2 and tokens[0] == "PLATFORMER_LEVEL" and tokens[1] == "1":
        return
    if len(tokens) >= 1 and tokens[0] == "PLATFORMER_LEVEL":
        raise CookError(f"unsupported level format header: {first}")
    raise CookError("level file must start with PLATFORMER_LEVEL 1")


def cook(root: Path | None = None) -> int:
    root = repo_root() if root is None else root
    sources = source_root(root)
    cooked = cooked_root(root)
    manifest_path = cooked / MANIFEST_NAME

    try:
        cooked.mkdir(parents=True, exist_ok=True)
    except OSError as exc:
        print(f"error: cannot create cooked directory: {display_repo_path(root, cooked)}", file=sys.stderr)
        print(exc, file=sys.stderr)
        return 1

    previous_manifest = load_previous_manifest(manifest_path)
    assets = collect_cook_assets(sources)
    current_ids = {asset["id"] for asset in assets}

    try:
        remove_stale_outputs(cooked, previous_manifest, current_ids)
        remove_stale_cooked_levels(cooked, current_level_v1_identities(assets))
    except CookError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    manifest_entries: list[dict[str, Any]] = []

    for asset in assets:
        identity = portable_relative(asset["id"])
        source_relative = portable_relative(asset["source"])
        cooked_relative = portable_relative(asset["cooked"])
        source_path = sources / Path(source_relative)
        cooked_path = cooked / Path(cooked_relative)

        if not source_path.is_file():
            print("error: required source asset is missing.", file=sys.stderr)
            print(f"  logical id: {identity}", file=sys.stderr)
            print(f"  expected:   {display_repo_path(root, source_path)}", file=sys.stderr)
            return 1

        kind = asset.get("kind", KIND_COPY)
        if kind not in (KIND_COPY, KIND_RUNTIME_PNG, KIND_LEVEL_V1):
            print("error: unknown asset kind.", file=sys.stderr)
            print(f"  logical id: {identity}", file=sys.stderr)
            print(f"  kind:       {kind}", file=sys.stderr)
            return 1

        try:
            source_data = read_bytes(source_path, label="source asset")
            source_hash = sha256_bytes(source_data)
            png_note = ""
            if kind == KIND_RUNTIME_PNG:
                png_result = cook_runtime_png_bytes(source_data)
                cooked_data = png_result.cooked_data
                png_note = (
                    f"source {png_result.source_width}x{png_result.source_height}; "
                    f"cooked {png_result.cooked_width}x{png_result.cooked_height}; "
                    f"recipe {png_result.recipe}"
                    + ("; resized" if png_result.resized else "; copy unchanged")
                )
                wrote = write_bytes_if_changed(cooked_path, cooked_data)
                manifest_entries.append(
                    {
                        "cooked": cooked_relative,
                        "cookedHeight": png_result.cooked_height,
                        "cookedWidth": png_result.cooked_width,
                        "id": identity,
                        "recipe": png_result.recipe,
                        "source": source_relative,
                        "sourceHeight": png_result.source_height,
                        "sourceSha256": source_hash,
                        "sourceWidth": png_result.source_width,
                    }
                )
            else:
                if kind == KIND_LEVEL_V1:
                    validate_level_v1_header(source_data)
                if kind == KIND_COPY and identity.endswith(STATIC_GLB_SUFFIX):
                    validate_static_glb(source_data)
                cooked_data = source_data
                wrote = write_bytes_if_changed(cooked_path, cooked_data)
                manifest_entries.append(
                    {
                        "cooked": cooked_relative,
                        "id": identity,
                        "source": source_relative,
                        "sourceSha256": source_hash,
                    }
                )
        except CookError as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1

        status = "cooked" if wrote else "unchanged/skipped"
        print(f"[{status}] {identity}")
        if png_note:
            print(f"  {png_note}")

    manifest_text = build_manifest(manifest_entries)
    try:
        wrote_manifest = write_bytes_if_changed(
            manifest_path, manifest_text.encode("utf-8")
        )
    except CookError as exc:
        print(f"error: manifest failure: {exc}", file=sys.stderr)
        return 1

    if wrote_manifest:
        print(f"[cooked] {MANIFEST_NAME}")
    else:
        print(f"[unchanged/skipped] {MANIFEST_NAME}")

    return 0


def cook_imported_runtime_png(source_path: Path, cooked_path: Path) -> RuntimePngCookResult:
    """Cook one imported runtime PNG using runtime_png.max512.lanczos.v1.

    This is the focused Import Texture cook path. It does not glob
    source/textures and does not implement a second recipe.
    """
    source_data = read_bytes(source_path, label="imported runtime PNG")
    result = cook_runtime_png_bytes(source_data)
    write_bytes_if_changed(cooked_path, result.cooked_data)
    return result


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else list(argv)
    if args and args[0] == "--cook-runtime-png":
        if len(args) != 3:
            print(
                "usage: python tools/cook_assets.py --cook-runtime-png <source.png> <cooked.png>",
                file=sys.stderr,
            )
            return 2
        try:
            result = cook_imported_runtime_png(Path(args[1]), Path(args[2]))
        except CookError as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
        print(f"[cooked] {Path(args[2]).as_posix()}")
        print(
            "  "
            f"recipe {result.recipe}; "
            f"{result.source_width}x{result.source_height} -> "
            f"{result.cooked_width}x{result.cooked_height}"
            + ("; resized" if result.resized else "; copy unchanged")
        )
        return 0
    return cook()


if __name__ == "__main__":
    sys.exit(main())
