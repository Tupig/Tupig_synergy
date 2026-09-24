# SPDX-FileCopyrightText: (C) 2026 TuPig
# SPDX-License-Identifier: MIT
"""One-shot: fold deskflow action icons into synergy.qrc as aliases (U-07 D1)."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DESKFLOW_QRC = ROOT / "src" / "apps" / "res" / "deskflow.qrc"
OUT_QRC = ROOT / "extra" / "src" / "apps" / "res" / "synergy.qrc"
REL_PREFIX = "../../../../src/apps/res/"


def main() -> None:
    files = re.findall(r"<file>([^<]+)</file>", DESKFLOW_QRC.read_text(encoding="utf-8"))
    lines = [
        "<RCC>",
        "    <qresource>",
        "        <file>image/logo-dark.png</file>",
        "        <file>image/logo-light.png</file>",
        '        <file alias="icons/synergy-dark/index.theme">synergy-dark.theme</file>',
        '        <file alias="icons/synergy-light/index.theme">synergy-light.theme</file>',
        '        <file alias="icons/synergy-dark/apps/64/com.tupig.synergy.svg">synergy.svg</file>',
        '        <file alias="icons/synergy-dark/apps/64/com.tupig.synergy-symbolic.svg">synergy-symbolic-dark.svg</file>',
        '        <file alias="icons/synergy-light/apps/64/com.tupig.synergy.svg">synergy.svg</file>',
        '        <file alias="icons/synergy-light/apps/64/com.tupig.synergy-symbolic.svg">synergy-symbolic-light.svg</file>',
    ]
    added = skipped = 0
    for path in files:
        if path.endswith("index.theme") or "/apps/64/" in path:
            skipped += 1
            continue
        if path.startswith("icons/deskflow-dark/"):
            alias = "icons/synergy-dark/" + path[len("icons/deskflow-dark/") :]
        elif path.startswith("icons/deskflow-light/"):
            alias = "icons/synergy-light/" + path[len("icons/deskflow-light/") :]
        else:
            skipped += 1
            continue
        lines.append(f'        <file alias="{alias}">{REL_PREFIX}{path}</file>')
        added += 1
    lines.extend(["    </qresource>", "</RCC>", ""])
    OUT_QRC.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {OUT_QRC} added={added} skipped={skipped}")


if __name__ == "__main__":
    main()
