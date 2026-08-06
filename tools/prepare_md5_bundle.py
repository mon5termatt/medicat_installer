#!/usr/bin/env python3
"""Strip QuickSFV comment lines and gzip-compress MedicatFiles.md5 for exe embedding."""

from __future__ import annotations

import argparse
import gzip
import pathlib
import sys


def prepare_manifest(input_path: pathlib.Path, output_path: pathlib.Path) -> None:
    text = input_path.read_text(encoding="utf-8", errors="replace")
    lines = [line for line in text.splitlines() if line.strip() and not line.lstrip().startswith(";")]
    if not lines:
        raise SystemExit(f"No MD5 hash lines found in {input_path}")

    payload = ("\n".join(lines) + "\n").encode("utf-8")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    compressed = gzip.compress(payload, compresslevel=9)
    output_path.write_bytes(compressed)

    ratio = (len(compressed) / input_path.stat().st_size) * 100.0
    print(
        f"Wrote {output_path} "
        f"({input_path.stat().st_size} -> {len(compressed)} bytes, "
        f"{len(lines)} entries, {ratio:.1f}% of original size)"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=pathlib.Path, help="Source MedicatFiles.md5")
    parser.add_argument("output", type=pathlib.Path, help="Output MedicatFiles.md5.gz")
    args = parser.parse_args()

    if not args.input.is_file():
        print(f"Input not found: {args.input}", file=sys.stderr)
        return 1

    try:
        prepare_manifest(args.input, args.output)
    except SystemExit as exc:
        print(exc, file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
