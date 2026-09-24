#!/usr/bin/env python3
# SPDX-FileCopyrightText: (C) 2026 TuPig
# SPDX-License-Identifier: MIT
"""Fail if vcpkg.json version-string disagrees with Version.cmake MAJOR.MINOR.PATCH."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION_CMAKE = ROOT / "extra" / "cmake" / "Version.cmake"
VCPKG_JSON = ROOT / "vcpkg.json"


def read_cmake_base() -> str:
    text = VERSION_CMAKE.read_text(encoding="utf-8")
    majors = re.search(r"set\(SYNERGY_VERSION_MAJOR\s+(\d+)\)", text)
    minors = re.search(r"set\(SYNERGY_VERSION_MINOR\s+(\d+)\)", text)
    patchs = re.search(r"set\(SYNERGY_VERSION_PATCH\s+(\d+)\)", text)
    if not (majors and minors and patchs):
        raise SystemExit(f"could not parse MAJOR/MINOR/PATCH from {VERSION_CMAKE}")
    return f"{majors.group(1)}.{minors.group(1)}.{patchs.group(1)}"


def read_vcpkg_version() -> str:
    data = json.loads(VCPKG_JSON.read_text(encoding="utf-8"))
    value = data.get("version-string")
    if not isinstance(value, str) or not value:
        raise SystemExit(f"missing version-string in {VCPKG_JSON}")
    return value


def main() -> int:
    cmake_base = read_cmake_base()
    vcpkg = read_vcpkg_version()
    if cmake_base != vcpkg:
        print(
            f"version mismatch: Version.cmake base={cmake_base} "
            f"vcpkg.json version-string={vcpkg}",
            file=sys.stderr,
        )
        return 1
    print(f"version OK: {cmake_base}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
