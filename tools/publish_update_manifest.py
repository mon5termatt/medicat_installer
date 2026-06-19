#!/usr/bin/env python3
"""Write installer/update.json for self-update checks."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

REPO = "mon5termatt/medicat_installer"


def read_build_version(path: Path) -> tuple[str, int]:
    text = path.read_text(encoding="utf-8")
    version_match = re.search(r'kInstallerVersion\[\] = "([^"]+)"', text)
    build_match = re.search(r"kInstallerBuildNumber = (\d+)", text)
    version = version_match.group(1) if version_match else "0.0.0"
    build = int(build_match.group(1)) if build_match else 0
    return version, build


def read_release_tag(explicit: str | None) -> str:
    if explicit and explicit.strip():
        return explicit.strip()

    tag_file = Path("release_tag.txt")
    if tag_file.exists():
        tag = tag_file.read_text(encoding="utf-8").strip()
        if tag:
            return tag

    raise SystemExit("Release tag required: pass --tag or set release_tag.txt")


def main() -> int:
    parser = argparse.ArgumentParser(description="Publish installer update manifest")
    parser.add_argument("--tag", help="GitHub release tag, e.g. 3521-BETA (default: release_tag.txt)")
    parser.add_argument(
        "--build-version",
        type=Path,
        default=Path("generated/build_version.cpp"),
        help="Generated build_version.cpp path",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("installer/update.json"),
        help="Output manifest path",
    )
    args = parser.parse_args()

    version, build = read_build_version(args.build_version)
    tag = read_release_tag(args.tag)
    base = f"https://github.com/{REPO}/releases/download/{tag}"
    manifest = {
        "channel": "prerelease",
        "release_tag": tag,
        "version": version,
        "build": build,
        "release_notes_url": f"https://github.com/{REPO}/releases/tag/{tag}",
        "assets": {
            "x64": {
                "name": "MedicatInstaller.exe",
                "url": f"{base}/MedicatInstaller.exe",
            },
            "x86": {
                "name": "MedicatInstaller-x86.exe",
                "url": f"{base}/MedicatInstaller-x86.exe",
            },
        },
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"Wrote {args.output} (v{version}, build {build}, tag {tag})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
