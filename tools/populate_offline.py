#!/usr/bin/env python3
"""Download Ventoy zips and helper files into offline/ for air-gapped installs."""

from __future__ import annotations

import argparse
import json
import shutil
import sys
import urllib.error
import urllib.request
from pathlib import Path


def normalize_version(value: str) -> str:
    value = value.strip()
    if value.lower().startswith("v"):
        value = value[1:]
    return value


def fetch_json(url: str) -> object:
    request = urllib.request.Request(
        url,
        headers={"Accept": "application/vnd.github+json", "User-Agent": "medicat-installer-offline"},
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.load(response)


def fetch_versions() -> list[str]:
    versions: list[str] = []
    for page in range(1, 21):
        payload = fetch_json(
            f"https://api.github.com/repos/ventoy/Ventoy/releases?per_page=100&page={page}"
        )
        if not isinstance(payload, list) or not payload:
            break
        for release in payload:
            tag = normalize_version(str(release.get("tag_name", "")))
            if tag:
                versions.append(tag)
        if len(payload) < 100:
            break
    return versions


def download_file(url: str, destination: Path) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": "medicat-installer-offline"})
    with urllib.request.urlopen(request, timeout=300) as response:
        destination.write_bytes(response.read())


def ventoy_zip_name(version: str) -> str:
    return f"ventoy-{version}-windows.zip"


def ventoy_zip_url(version: str) -> str:
    return f"https://github.com/ventoy/Ventoy/releases/download/v{version}/{ventoy_zip_name(version)}"


def main() -> int:
    parser = argparse.ArgumentParser(description="Populate offline/ cache for MediCat Installer")
    parser.add_argument(
        "--output",
        type=Path,
        help="Offline folder path (default: <repo>/offline)",
    )
    parser.add_argument(
        "--versions",
        default="latest",
        help="Comma-separated Ventoy versions, or 'latest' (default: latest)",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=1,
        help="When --versions=latest, download this many newest releases (default: 1)",
    )
    parser.add_argument(
        "--md5",
        action="store_true",
        help="Copy MedicatFiles.md5 from repo root into offline/",
    )
    parser.add_argument(
        "--skip-versions-file",
        action="store_true",
        help="Do not write offline/ventoy_versions.txt",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    offline_dir = args.output or (repo_root / "offline")
    ventoy_dir = offline_dir / "ventoy"
    ventoy_dir.mkdir(parents=True, exist_ok=True)

    try:
        all_versions = fetch_versions()
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError) as exc:
        print(f"Failed to fetch Ventoy versions: {exc}", file=sys.stderr)
        return 1

    if not all_versions:
        print("No Ventoy versions found", file=sys.stderr)
        return 1

    if args.versions.strip().lower() == "latest":
        selected = all_versions[: max(args.count, 1)]
    else:
        selected = [normalize_version(part) for part in args.versions.split(",") if normalize_version(part)]

    if not selected:
        print("No versions selected", file=sys.stderr)
        return 1

    if not args.skip_versions_file:
        versions_path = offline_dir / "ventoy_versions.txt"
        versions_path.write_text("\n".join(all_versions) + "\n", encoding="utf-8")
        print(f"Wrote {len(all_versions)} versions to {versions_path}")

    failures = 0
    for version in selected:
        destination = ventoy_dir / ventoy_zip_name(version)
        if destination.is_file():
            print(f"Skip existing {destination.name}")
            continue

        url = ventoy_zip_url(version)
        print(f"Downloading {url}")
        try:
            download_file(url, destination)
        except (urllib.error.URLError, TimeoutError, OSError) as exc:
            print(f"Failed to download v{version}: {exc}", file=sys.stderr)
            failures += 1
            continue

        size_mb = destination.stat().st_size / (1024 * 1024)
        print(f"Saved {destination} ({size_mb:.1f} MB)")

    if args.md5:
        source = repo_root / "MedicatFiles.md5"
        if not source.is_file():
            print(f"MedicatFiles.md5 not found at {source}", file=sys.stderr)
            failures += 1
        else:
            target = offline_dir / "MedicatFiles.md5"
            shutil.copyfile(source, target)
            print(f"Copied {target}")

    if failures:
        return 1

    print(f"Offline cache ready in {offline_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
