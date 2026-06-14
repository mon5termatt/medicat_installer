#!/usr/bin/env python3
"""Fetch all Ventoy release versions from GitHub and write a plain-text list."""

from __future__ import annotations

import argparse
import json
import shutil
import sys
import urllib.error
import urllib.request
from pathlib import Path

API = "https://api.github.com/repos/ventoy/Ventoy/releases"


def normalize_version(tag: str) -> str:
    return tag.lstrip("vV")


def fetch_versions() -> list[str]:
    versions: list[str] = []
    page = 1
    while page <= 20:
        url = f"{API}?per_page=100&page={page}"
        req = urllib.request.Request(url, headers={"User-Agent": "medicat-installer"})
        with urllib.request.urlopen(req, timeout=60) as resp:
            releases = json.load(resp)
        if not releases:
            break
        for release in releases:
            tag = release.get("tag_name", "")
            version = normalize_version(tag)
            if version:
                versions.append(version)
        if len(releases) < 100:
            break
        page += 1
    return versions


def main() -> int:
    parser = argparse.ArgumentParser(description="Fetch Ventoy release versions")
    parser.add_argument("output", nargs="?", help="Output text file path")
    parser.add_argument("--fallback", help="Copy this file if GitHub fetch fails")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    out = Path(args.output) if args.output else repo_root / "ventoy_versions.txt"
    fallback = Path(args.fallback) if args.fallback else None

    versions: list[str] = []
    try:
        versions = fetch_versions()
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError) as exc:
        print(f"Ventoy version fetch failed: {exc}", file=sys.stderr)
        if fallback and fallback.is_file():
            shutil.copyfile(fallback, out)
            versions = [
                normalize_version(line)
                for line in fallback.read_text(encoding="utf-8").splitlines()
                if normalize_version(line)
            ]
            print(f"Used fallback list from {fallback}")
        else:
            return 1

    if not versions:
        print("No Ventoy versions fetched", file=sys.stderr)
        return 1

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(versions) + "\n", encoding="utf-8")
    print(f"Wrote {len(versions)} versions to {out}")
    print(f"Latest: v{versions[0]}  Oldest: v{versions[-1]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
