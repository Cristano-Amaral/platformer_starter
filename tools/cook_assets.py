#!/usr/bin/env python3
"""Copy known runtime-source assets into deterministic cooked output.

Run from the repository root:

    python tools/cook_assets.py

Paths are resolved from this file's location so the cooker does not depend on
the current working directory.

Standalone runtime PNGs (`kind: runtime_png`) are listed explicitly and may be
downscaled with cooker-only Pillow. Blender authoring PNGs and `.blend` files
are not cooker inputs. GLBs are opaque copies. Level v1 files (`kind: level_v1`)
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
#   copy         = opaque byte copy (GLBs; embedded images are not inspected)
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
        "id": "levels/level_01.level",
        "source": "levels/level_01.level",
        "cooked": "levels/level_01.level",
        "kind": KIND_LEVEL_V1,
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


def cook() -> int:
    root = repo_root()
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
    current_ids = {asset["id"] for asset in KNOWN_ASSETS}

    try:
        remove_stale_outputs(cooked, previous_manifest, current_ids)
    except CookError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    manifest_entries: list[dict[str, Any]] = []

    for asset in KNOWN_ASSETS:
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


if __name__ == "__main__":
    sys.exit(cook())
