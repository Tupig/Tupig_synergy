#!/usr/bin/env python3
# SPDX-FileCopyrightText: (C) 2026 TuPig
# SPDX-License-Identifier: MIT
"""Fail when a source file is not in the allowed encoding set.

The default allowed encoding is UTF-8 without a BOM. The whitelist, the
directories to scan, and the directories to skip live in encoding-check.json
next to this script. Pass --warn to print the same list and exit 0.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = Path(__file__).resolve().parent / "encoding-check.json"

# Longer marks first so UTF-32 is not reported as UTF-16.
_BOMS = (
    (b"\xff\xfe\x00\x00", "utf-32-le"),
    (b"\x00\x00\xfe\xff", "utf-32-be"),
    (b"\xef\xbb\xbf", "utf-8-bom"),
    (b"\xff\xfe", "utf-16-le"),
    (b"\xfe\xff", "utf-16-be"),
)


def detect_encoding(data: bytes) -> str:
    """Return a short name for the encoding these bytes actually use."""
    for mark, name in _BOMS:
        if data.startswith(mark):
            return name
    try:
        data.decode("utf-8")
    except UnicodeDecodeError:
        pass
    else:
        return "utf-8"
    if len(data) % 2 == 0 and b"\x00" in data:
        for name in ("utf-16-le", "utf-16-be"):
            try:
                data.decode(name)
            except UnicodeDecodeError:
                continue
            return name
    for name in ("gb18030", "cp1252"):
        try:
            data.decode(name)
        except UnicodeDecodeError:
            continue
        return name
    return "unknown"


def load_config(path: Path) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    for key in ("roots", "extensions", "names", "allowed", "exclude_dirs"):
        if key not in data or not isinstance(data[key], list) or not data[key]:
            raise SystemExit(f"encoding config {path} is missing a non-empty {key} list")
    return data


def normalize_ext(ext: str) -> str:
    ext = ext.lower()
    return ext if ext.startswith(".") else f".{ext}"


def iter_source_files(root: Path, config: dict) -> list[Path]:
    extensions = {normalize_ext(item) for item in config["extensions"]}
    names = set(config["names"])
    excluded = set(config["exclude_dirs"])
    found: list[Path] = []
    for rel in config["roots"]:
        base = root / rel
        if not base.is_dir():
            print(f"encoding: skip missing directory {rel}")
            continue
        for path in base.rglob("*"):
            if not path.is_file():
                continue
            relative = path.relative_to(root)
            if any(part in excluded for part in relative.parts):
                continue
            if path.name not in names and path.suffix.lower() not in extensions:
                continue
            found.append(path)
    found.sort()
    return found


def display_path(root: Path, path: Path) -> str:
    return path.relative_to(root).as_posix()


def main() -> int:
    parser = argparse.ArgumentParser(description="Check source file encodings.")
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument(
        "--allow",
        help="Comma-separated encoding whitelist. Replaces the config list.",
    )
    parser.add_argument(
        "--exclude",
        help="Comma-separated directory names to skip, added to the config list.",
    )
    parser.add_argument(
        "--warn",
        action="store_true",
        help="Print violations and exit 0 instead of failing the process.",
    )
    args = parser.parse_args()

    root = args.root.resolve()
    config = load_config(args.config.resolve())
    if args.allow:
        config["allowed"] = [item.strip() for item in args.allow.split(",") if item.strip()]
    if args.exclude:
        config["exclude_dirs"] = list(config["exclude_dirs"]) + [
            item.strip() for item in args.exclude.split(",") if item.strip()
        ]
    allowed = set(config["allowed"])
    files = iter_source_files(root, config)
    if not files:
        print(f"encoding: no source files under {root}")
        return 1

    violations: list[tuple[str, str]] = []
    for path in files:
        detected = detect_encoding(path.read_bytes())
        if detected not in allowed:
            violations.append((display_path(root, path), detected))

    allowed_text = ", ".join(config["allowed"])
    print(f"encoding: checked {len(files)} files; allowed: {allowed_text}")
    if not violations:
        print("encoding: ok")
        return 0

    level = "warning" if args.warn else "error"
    print(f"encoding: {len(violations)} file(s) not allowed")
    for rel, detected in violations:
        print(f"::{level} file={rel},line=1::detected {detected}; allowed: {allowed_text}")
        print(f"{rel}  {detected}")
    return 0 if args.warn else 1


if __name__ == "__main__":
    sys.exit(main())
