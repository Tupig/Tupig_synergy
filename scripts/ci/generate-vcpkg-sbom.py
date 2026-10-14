#!/usr/bin/env python3
# SPDX-FileCopyrightText: (C) 2026 TuPig
# SPDX-License-Identifier: MIT
"""Convert a vcpkg `install --dry-run` plan into a CycloneDX 1.5 SBOM.

The plan lists every package that would be built (target and host), with the
version resolved from the builtin-baseline pinned in vcpkg.json, for example:

    The following packages will be built and installed:
        openssl:x64-linux@3.5.0#1 -- git+https://github.com/microsoft/vcpkg:...

Usage:
    generate-vcpkg-sbom.py <dry-run-output> <output-sbom.json>

The script is deliberately dependency-free (stdlib only) and ASCII-only.
"""

import json
import re
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path

# One plan line: optional "*" (modified), port name, optional [features],
# optional :triplet, @version, optional #port-version, optional " -- source".
PLAN_LINE = re.compile(
    r"^\s+\*?\s*(?P<name>[A-Za-z0-9][A-Za-z0-9_-]*)"
    r"(?:\[[^\]]*\])?"
    r"(?::[a-z0-9_-]+)?"
    r"@(?P<version>[^#\s]+)"
    r"(?:#(?P<portversion>[0-9]+))?"
    r"(?:\s+--\s+.*)?$"
)


def parse_plan(text):
    """Return a name -> (version, port-version) mapping from plan output.

    Host and target triplets may list the same port twice; the first entry
    wins, since both resolve from the same baseline.
    """
    packages = {}
    for line in text.splitlines():
        match = PLAN_LINE.match(line)
        if not match:
            continue
        name = match.group("name").lower()
        if name in packages:
            continue
        packages[name] = (match.group("version"), match.group("portversion"))
    return packages


def build_sbom(packages):
    components = []
    for name in sorted(packages):
        version, portversion = packages[name]
        component = {
            "type": "library",
            "name": name,
            "version": version,
            "purl": "pkg:generic/{}@{}".format(name, version),
        }
        if portversion:
            component["properties"] = [{"name": "vcpkg:port-version", "value": portversion}]
        components.append(component)

    return {
        "bomFormat": "CycloneDX",
        "specVersion": "1.5",
        "serialNumber": "urn:uuid:{}".format(uuid.uuid4()),
        "version": 1,
        "metadata": {
            "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "component": {"type": "application", "name": "TuPig Synergy"},
            "tools": [{"name": "generate-vcpkg-sbom.py"}],
        },
        "components": components,
    }


def main(argv):
    if len(argv) != 3:
        sys.stderr.write("usage: generate-vcpkg-sbom.py <dry-run-output> <output-sbom.json>\n")
        return 2

    plan_path = Path(argv[1])
    out_path = Path(argv[2])

    packages = parse_plan(plan_path.read_text(encoding="utf-8", errors="replace"))
    if not packages:
        sys.stderr.write("error: no packages found in {}\n".format(plan_path))
        return 1

    out_path.write_text(json.dumps(build_sbom(packages), indent=2) + "\n", encoding="utf-8")
    print("wrote {} with {} components".format(out_path, len(packages)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
