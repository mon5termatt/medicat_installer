#!/usr/bin/env python3
"""Download official aria2c.exe and gzip-compress it for exe embedding."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import io
import pathlib
import sys
import urllib.request
import zipfile

ARIA2_VERSION = "1.37.0"

ARCHIVES = {
    "x64": {
        "url": (
            "https://github.com/aria2/aria2/releases/download/"
            f"release-{ARIA2_VERSION}/aria2-{ARIA2_VERSION}-win-64bit-build1.zip"
        ),
        "sha256": "67d015301eef0b612191212d564c5bb0a14b5b9c4796b76454276a4d28d9b288",
    },
    "x32": {
        "url": (
            "https://github.com/aria2/aria2/releases/download/"
            f"release-{ARIA2_VERSION}/aria2-{ARIA2_VERSION}-win-32bit-build1.zip"
        ),
        "sha256": "35f6514cc5dd7e98a87b3c4c2d25a0754b9b063dbe59bc0f22d483464f61e5b6",
    },
}


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def download_zip(url: str, expected_sha256: str) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": "medicat-installer"})
    with urllib.request.urlopen(req, timeout=120) as resp:
        data = resp.read()
    digest = sha256_bytes(data)
    if digest.lower() != expected_sha256.lower():
        raise SystemExit(f"SHA-256 mismatch for {url}: got {digest}, expected {expected_sha256}")
    return data


def extract_aria2c(zip_bytes: bytes) -> bytes:
    with zipfile.ZipFile(io.BytesIO(zip_bytes)) as archive:
        names = [name for name in archive.namelist() if name.replace("\\", "/").lower().endswith("/aria2c.exe")]
        if not names:
            names = [name for name in archive.namelist() if name.replace("\\", "/").lower().endswith("aria2c.exe")]
        if not names:
            raise SystemExit("aria2c.exe not found in official zip")
        data = archive.read(names[0])
    if len(data) < 1024 * 1024:
        raise SystemExit(f"aria2c.exe is unexpectedly small ({len(data)} bytes)")
    return data


def gzip_bytes(data: bytes) -> bytes:
    return gzip.compress(data, compresslevel=9)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--arch", choices=sorted(ARCHIVES), required=True)
    parser.add_argument("--cache-exe", type=pathlib.Path, required=True, help="Uncompressed aria2c.exe cache path")
    parser.add_argument("--output", type=pathlib.Path, required=True, help="Output aria2c.exe.gz")
    args = parser.parse_args()

    spec = ARCHIVES[args.arch]
    cache_exe: pathlib.Path = args.cache_exe
    output: pathlib.Path = args.output

    if cache_exe.is_file() and cache_exe.stat().st_size >= 1024 * 1024:
        exe_bytes = cache_exe.read_bytes()
        print(f"Using cached {cache_exe} ({len(exe_bytes)} bytes)")
    else:
        print(f"Downloading aria2 {ARIA2_VERSION} ({args.arch})...")
        zip_bytes = download_zip(spec["url"], spec["sha256"])
        exe_bytes = extract_aria2c(zip_bytes)
        cache_exe.parent.mkdir(parents=True, exist_ok=True)
        cache_exe.write_bytes(exe_bytes)
        print(f"Wrote {cache_exe} ({len(exe_bytes)} bytes)")

    compressed = gzip_bytes(exe_bytes)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(compressed)
    ratio = (len(compressed) / len(exe_bytes)) * 100.0
    print(f"Wrote {output} ({len(exe_bytes)} -> {len(compressed)} bytes, {ratio:.1f}% of original size)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
