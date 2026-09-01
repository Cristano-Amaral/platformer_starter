#!/usr/bin/env python3
"""Copy authored source assets into deterministic cooked runtime output.

Run from the repository root:

    python tools/cook_assets.py

Uses only the Python standard library. Paths are resolved from this file's
location so the cooker does not depend on the current working directory.
"""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
MANIFEST_NAME = "manifest.json"

# Logical identities are portable relative paths. Never use absolute paths or UUIDs.
# .blend files are authoring-only and must never appear here.
KNOWN_ASSETS = (
    {
        "id": "models/test_authored.glb",
        "source": "models/test_authored.glb",
        "cooked": "models/test_authored.glb",
    },
    {
        "id": "models/test_static.glb",
        "source": "models/test_static.glb",
        "cooked": "models/test_static.glb",
    },
    {
        "id": "textures/test_checker.png",
        "source": "textures/test_checker.png",
        "cooked": "textures/test_checker.png",
    },
)


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


def build_manifest(entries: list[dict[str, str]]) -> str:
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


class CookError(Exception):
    pass


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

    manifest_entries: list[dict[str, str]] = []

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

        try:
            source_data = read_bytes(source_path, label="source asset")
            source_hash = sha256_bytes(source_data)
            wrote = write_bytes_if_changed(cooked_path, source_data)
        except CookError as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1

        status = "cooked" if wrote else "unchanged/skipped"
        print(f"[{status}] {identity}")

        manifest_entries.append(
            {
                "cooked": cooked_relative,
                "id": identity,
                "source": source_relative,
                "sourceSha256": source_hash,
            }
        )

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
