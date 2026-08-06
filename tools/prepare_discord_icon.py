#!/usr/bin/env python3
"""Build res/discord.ico from res/discord.png (replace PNG to update the icon)."""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image

REPO = Path(__file__).resolve().parents[1]
PNG_PATH = REPO / "res" / "discord.png"
ICO_PATH = REPO / "res" / "discord.ico"
ICO_SIZES = (16, 24, 32, 48)


def resize_icon(source: Image.Image, size: int) -> Image.Image:
    resized = source.resize((size, size), Image.Resampling.LANCZOS)
    if resized.mode != "RGBA":
        return resized.convert("RGBA")
    return resized


def main() -> int:
    if not PNG_PATH.is_file():
        print(f"Missing source icon: {PNG_PATH}", file=sys.stderr)
        print("Add res/discord.png (square PNG with transparency), then rebuild.", file=sys.stderr)
        return 1

    source = Image.open(PNG_PATH).convert("RGBA")
    icons = [resize_icon(source, size) for size in ICO_SIZES]
    icons[0].save(
        ICO_PATH,
        format="ICO",
        sizes=[(img.width, img.height) for img in icons],
        append_images=icons[1:],
    )

    print(f"Wrote {ICO_PATH.relative_to(REPO)} from {PNG_PATH.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
