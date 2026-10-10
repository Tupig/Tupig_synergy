#!/usr/bin/env python3
# SPDX-FileCopyrightText: (C) 2026 TuPig
# SPDX-License-Identifier: MIT
"""Check that source files use an allowed character encoding.

Default policy: UTF-8 without a BOM. ASCII is accepted by that policy because
it is a subset of UTF-8. The whitelist, scan roots, skipped directories, and
whether a violation fails the process live in encoding-check.json.
"""

from __future__ import annotations

import argparse
import codecs
import json
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = Path(__file__).resolve().parent / "encoding-check.json"
_CHUNK = 65536

# Longer marks first so UTF-32 is not reported as UTF-16.
_BOMS = (
    (b"\xff\xfe\x00\x00", "utf-32-le"),
    (b"\x00\x00\xfe\xff", "utf-32-be"),
    (b"\xef\xbb\xbf", "utf-8-bom"),
    (b"\xff\xfe", "utf-16-le"),
    (b"\xfe\xff", "utf-16-be"),
)

# Names that mean the same thing in the whitelist.
_ALIASES = {
    "utf-8-sig": "utf-8-bom",
    "utf-8-with-bom": "utf-8-bom",
    "us-ascii": "ascii",
}


def detect_encoding(path: Path) -> str:
    """Return a short name for the encoding this file actually uses.

    Valid UTF-8 is decoded in chunks so a large file is not held in memory.
    A file that is not UTF-8 is read fully only to name the fallback encoding.
    """
    ascii_only = True
    failed = False
    with path.open("rb") as handle:
        head = handle.read(4)
        for mark, name in _BOMS:
            if head.startswith(mark):
                return name
        decoder = codecs.getincrementaldecoder("utf-8")()
        chunk = head
        try:
            while chunk:
                if any(byte > 127 for byte in chunk):
                    ascii_only = False
                decoder.decode(chunk)
                chunk = handle.read(_CHUNK)
            decoder.decode(b"", final=True)
        except UnicodeDecodeError:
            failed = True
    if failed:
        return _detect_non_utf8(path.read_bytes())
    return "ascii" if ascii_only else "utf-8"


def _detect_non_utf8(data: bytes) -> str:
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


def canonical_name(name: str) -> str:
    folded = name.strip().lower().replace("_", "-")
    return _ALIASES.get(folded, folded)


def is_allowed(detected: str, allowed: set[str]) -> bool:
    names = {detected}
    if detected == "ascii":
        names.add("utf-8")
    return bool(names & allowed)


def load_config(path: Path) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    for key in ("roots", "extensions", "names", "allowed", "exclude_dirs"):
        if key not in data or not isinstance(data[key], list) or not data[key]:
            raise SystemExit(f"encoding config {path} is missing a non-empty {key} list")
    mode = data.get("on_violation", "fail")
    if mode not in ("fail", "warn"):
        raise SystemExit(f"encoding config {path} on_violation must be fail or warn")
    data["on_violation"] = mode
    data["allowed"] = [canonical_name(item) for item in data["allowed"]]
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
            print(f"encoding SKIP missing={rel}")
            continue
        for dirpath, dirnames, filenames in os.walk(base, followlinks=False):
            dirnames[:] = [name for name in dirnames if name not in excluded]
            for filename in filenames:
                path = Path(dirpath) / filename
                if filename not in names and path.suffix.lower() not in extensions:
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
    parser.add_argument("--allow", help="Comma-separated whitelist. Replaces the config list.")
    parser.add_argument("--exclude", help="Extra directory names to skip, added to the config list.")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--warn", action="store_true", help="Print violations and exit 0.")
    mode.add_argument("--fail", action="store_true", help="Exit 1 on violations. Overrides on_violation.")
    args = parser.parse_args()

    root = args.root.resolve()
    config = load_config(args.config.resolve())
    if args.allow:
        config["allowed"] = [canonical_name(item) for item in args.allow.split(",") if item.strip()]
    if args.exclude:
        config["exclude_dirs"] = list(config["exclude_dirs"]) + [
            item.strip() for item in args.exclude.split(",") if item.strip()
        ]
    if args.warn:
        config["on_violation"] = "warn"
    elif args.fail:
        config["on_violation"] = "fail"

    allowed = set(config["allowed"])
    files = iter_source_files(root, config)
    if not files:
        print(f"encoding FAIL no files under {root}")
        return 1

    violations: list[tuple[str, str]] = []
    for path in files:
        detected = detect_encoding(path)
        if not is_allowed(detected, allowed):
            violations.append((display_path(root, path), detected))

    allowed_text = ",".join(config["allowed"])
    mode_name = config["on_violation"]
    if not violations:
        print(f"encoding OK checked={len(files)} allowed={allowed_text} mode={mode_name}")
        return 0

    kind = "WARN" if mode_name == "warn" else "FAIL"
    level = "warning" if mode_name == "warn" else "error"
    for rel, detected in violations:
        print(f"encoding {kind} {rel} detected={detected} allowed={allowed_text}")
        print(f"::{level} file={rel},line=1::detected {detected}; allowed: {allowed_text}")
    print(f"encoding {kind} count={len(violations)} checked={len(files)} mode={mode_name}")
    return 0 if mode_name == "warn" else 1


if __name__ == "__main__":
    sys.exit(main())
