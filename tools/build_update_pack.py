#!/usr/bin/env python3
"""Build a MediCat USB update pack (.update.7z) by diffing a version folder against the current release.

Your --content folder is the new version tree (USB-relative paths). The builder loads the
current release MedicatFiles.md5 (--release), then:

  add    = files in --content that are missing from the release or have a different MD5
  remove = (with --complete) release paths that are absent from --content
           plus any extra paths from --remove

Unchanged files are not packed. Payload paths are USB-relative and flat at the
archive root (no version wrapper folder), so the installer can extract straight
onto the drive.

Layout of the produced archive:
  info.json
  MedicatUpdate.md5
  Antivirus/...
  Live_Operating_Systems/...
  (other USB-relative add paths)

info.json contentRoot is "." (flat). archiveMd5 is the MD5 of MedicatUpdate.md5.
A sidecar <out>.json embeds info + md5Manifest (ship .7z + .json).
A sidecar <out>.md5 records the MD5 of the finished .7z for CDN checks.

Example (complete new tree vs release manifest):
  python tools/build_update_pack.py ^
    --content D:\\packs\\beta1\\files ^
    --release MedicatFiles.md5 ^
    --complete ^
    --version 21.12.1-beta1 ^
    --base 21.12 ^
    --out D:\\packs\\MediCat.USB.v21.12.update.7z

Example (delta folder of replacements only; removals from JSON):
  python tools/build_update_pack.py ^
    --content D:\\packs\\beta1\\files ^
    --release MedicatFiles.md5 ^
    --remove D:\\packs\\beta1\\remove.json ^
    --version 21.12.1-beta1 ^
    --base 21.12 ^
    --out D:\\packs\\MediCat.USB.v21.12.update.7z

Example (bootables only; ignore PortableApps and Programs):
  python tools/build_update_pack.py ^
    --content D:\\packs\\beta1\\files ^
    --release MedicatFiles.md5 ^
    --complete --bootables ^
    --version 21.12.1-beta1 ^
    --base 21.12 ^
    --out D:\\packs\\MediCat.USB.v21.12.bootables.update.7z

Example (refresh sidecar .json only; no 7z recompress):
  python tools/build_update_pack.py ^
    --content F:\\MEDICAT2026\\26.12 ^
    --release MedicatFiles.md5 ^
    --complete --bootables --json-only ^
    --version 26.12 --base 21.12 ^
    --out F:\\MEDICAT2026\\MediCat.USB.v26.12.update.7z
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


MANIFEST_NAME = "MedicatUpdate.md5"

# Top-level folders skipped when --bootables is set (case-insensitive).
BOOTABLES_IGNORED_TOP_DIRS = ("portableapps", "programs")

# Root USB chrome files never removed by --bootables complete diffs.
BOOTABLES_PROTECTED_ROOT_FILES = frozenset(
    {
        "start.exe",
        "cdusb.y",
        "license.txt",
        "licence.txt",
        "autorun.inf",
        "autorun.ico",
    }
)


def normalize_rel(path: str) -> str:
    text = path.replace("\\", "/").strip()
    while text.startswith("./"):
        text = text[2:]
    return text.lstrip("/")


def normalize_key(path: str) -> str:
    return normalize_rel(path).lower()


def top_dir_name(rel: str) -> str:
    normalized = normalize_rel(rel)
    if not normalized:
        return ""
    return normalized.split("/", 1)[0].lower()


def is_bootables_ignored_path(rel: str) -> bool:
    return top_dir_name(rel) in BOOTABLES_IGNORED_TOP_DIRS


def is_bootables_protected_root_file(rel: str) -> bool:
    normalized = normalize_rel(rel)
    if not normalized or "/" in normalized:
        return False
    return normalized.lower() in BOOTABLES_PROTECTED_ROOT_FILES


def filter_release_map(
    release_map: dict[str, tuple[str, str]], *, bootables: bool
) -> dict[str, tuple[str, str]]:
    if not bootables:
        return release_map
    return {
        key: value
        for key, value in release_map.items()
        if not is_bootables_ignored_path(value[1]) and not is_bootables_protected_root_file(value[1])
    }


def load_remove_list(path: Path) -> list[str]:
    raw = json.loads(path.read_text(encoding="utf-8"))
    if isinstance(raw, list):
        items = raw
    elif isinstance(raw, dict) and isinstance(raw.get("remove"), list):
        items = raw["remove"]
    else:
        raise SystemExit(f"remove JSON must be a string array or {{\"remove\":[...]}}: {path}")

    out: list[str] = []
    for item in items:
        if not isinstance(item, str) or not item.strip():
            raise SystemExit(f"Invalid remove entry in {path}: {item!r}")
        out.append(normalize_rel(item))
    return sorted(set(out))


def parse_md5_manifest(path: Path) -> dict[str, tuple[str, str]]:
    """Return map of normalize_key(rel_path) -> (lowercase md5 hex, original relative path)."""
    text = path.read_text(encoding="utf-8", errors="replace")
    entries: dict[str, tuple[str, str]] = {}
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or line.startswith(";"):
            continue
        parts = line.split(None, 1)
        if len(parts) != 2:
            continue
        digest, rel = parts[0].strip().lower(), parts[1].strip()
        if rel.startswith("*"):
            rel = rel[1:].strip()
        rel = normalize_rel(rel)
        if len(digest) != 32 or not rel:
            continue
        entries[normalize_key(rel)] = (digest, rel)
    if not entries:
        raise SystemExit(f"No MD5 entries found in release manifest: {path}")
    return entries


def walk_content(content_dir: Path, *, bootables: bool = False) -> list[tuple[str, Path]]:
    files: list[tuple[str, Path]] = []
    for root, dirs, names in os.walk(content_dir):
        root_path = Path(root)
        # Prune ignored top-level dirs early when scanning a full USB tree.
        if bootables and root_path.resolve() == content_dir.resolve():
            dirs[:] = [name for name in dirs if name.lower() not in BOOTABLES_IGNORED_TOP_DIRS]
        for name in names:
            full = root_path / name
            rel = normalize_rel(str(full.relative_to(content_dir)))
            if not rel:
                continue
            if bootables and is_bootables_ignored_path(rel):
                continue
            # Skip manifests generated beside the tree (not USB payload).
            if Path(rel).name.lower() in {"medicatfiles.md5", "medicatupdate.md5"}:
                continue
            files.append((rel, full))
    files.sort(key=lambda item: item[0].lower())
    return files


def md5_file(path: Path) -> str:
    hasher = hashlib.md5()
    with path.open("rb") as handle:
        while True:
            chunk = handle.read(1024 * 1024)
            if not chunk:
                break
            hasher.update(chunk)
    return hasher.hexdigest()


def diff_against_release(
    content_files: list[tuple[str, Path]],
    release_map: dict[str, tuple[str, str]],
    *,
    complete: bool,
    extra_remove: list[str],
) -> tuple[list[tuple[str, Path]], list[str], dict[str, int]]:
    """Return (add_files, remove_paths, stats)."""
    add_files: list[tuple[str, Path]] = []
    content_keys: set[str] = set()
    unchanged = 0
    added = 0
    changed = 0

    total = len(content_files)
    for index, (rel, full) in enumerate(content_files, start=1):
        key = normalize_key(rel)
        content_keys.add(key)
        digest = md5_file(full)
        if index == 1 or index == total or index % 250 == 0:
            print(f"  hashing content {index}/{total}: {rel}")
        release_entry = release_map.get(key)
        if release_entry is None:
            add_files.append((rel, full))
            added += 1
        elif release_entry[0] != digest:
            add_files.append((rel, full))
            changed += 1
        else:
            unchanged += 1

    remove_set: dict[str, str] = {}
    if complete:
        for key, (_digest, original_rel) in release_map.items():
            if key not in content_keys:
                remove_set[key] = original_rel
    for rel in extra_remove:
        remove_set[normalize_key(rel)] = normalize_rel(rel)

    # Prefer original casing from content when a remove overlaps a path we still know.
    remove_paths = sorted(remove_set.values(), key=str.lower)

    overlap = sorted({normalize_key(rel) for rel, _ in add_files} & set(remove_set))
    if overlap:
        raise SystemExit("Paths present in both remove and add after diff:\n  " + "\n  ".join(overlap))

    stats = {
        "content_total": total,
        "unchanged": unchanged,
        "added": added,
        "changed": changed,
        "remove": len(remove_paths),
        "release_entries": len(release_map),
    }
    return add_files, remove_paths, stats


def write_manifest(entries: list[tuple[str, Path]], dest: Path) -> None:
    lines = [f"{md5_file(full)} *{rel}" for rel, full in entries]
    # LF-only bytes: Path.write_text on Windows would translate to CRLF and break archiveMd5.
    dest.write_bytes(("\n".join(lines) + ("\n" if lines else "")).encode("utf-8"))


def find_7za(explicit: Path | None) -> Path:
    if explicit is not None:
        if not explicit.is_file():
            raise SystemExit(f"7za not found: {explicit}")
        return explicit

    repo = Path(__file__).resolve().parents[1]
    candidates = [
        repo / "bin" / "7z" / "x64" / "7za.exe",
        repo / "bin" / "7z" / "x32" / "7za.exe",
        Path(shutil.which("7za") or ""),
        Path(shutil.which("7z") or ""),
    ]
    for candidate in candidates:
        if candidate and candidate.is_file():
            return candidate
    raise SystemExit("Could not find 7za.exe (pass --7za)")


def write_info_json(path: Path, payload: dict[str, Any]) -> None:
    path.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def build_pack(args: argparse.Namespace) -> None:
    content_dir = args.content.resolve()
    if not content_dir.is_dir():
        raise SystemExit(f"Content directory not found: {content_dir}")

    release_path = args.release.resolve()
    if not release_path.is_file():
        raise SystemExit(f"Release MD5 manifest not found: {release_path}")

    bootables = bool(args.bootables)
    print(f"Loading release manifest: {release_path}")
    release_map = parse_md5_manifest(release_path)
    print(f"  release entries: {len(release_map)}")
    if bootables:
        before = len(release_map)
        release_map = filter_release_map(release_map, bootables=True)
        print(
            f"  --bootables: ignoring PortableApps/, Programs/, and USB chrome root files "
            f"({before} -> {len(release_map)} release entries)"
        )

    extra_remove = load_remove_list(args.remove.resolve()) if args.remove else []
    if bootables and extra_remove:
        kept = [
            rel
            for rel in extra_remove
            if not is_bootables_ignored_path(rel) and not is_bootables_protected_root_file(rel)
        ]
        skipped = len(extra_remove) - len(kept)
        if skipped:
            print(
                f"  --bootables: skipped {skipped} --remove path(s) "
                f"(PortableApps/Programs/USB chrome)"
            )
        extra_remove = kept

    content_files = walk_content(content_dir, bootables=bootables)
    if not content_files:
        raise SystemExit(
            f"No files found under content directory"
            f"{' after --bootables filter' if bootables else ''}: {content_dir}"
        )

    print(f"Diffing {len(content_files)} content files against release...")
    add_files, remove_paths, stats = diff_against_release(
        content_files,
        release_map,
        complete=bool(args.complete),
        extra_remove=extra_remove,
    )

    if bootables:
        before_remove = len(remove_paths)
        remove_paths = [
            rel
            for rel in remove_paths
            if not is_bootables_ignored_path(rel) and not is_bootables_protected_root_file(rel)
        ]
        dropped = before_remove - len(remove_paths)
        if dropped:
            print(f"  --bootables: dropped {dropped} protected path(s) from remove[]")
            stats["remove"] = len(remove_paths)

    if not add_files and not remove_paths:
        raise SystemExit("Diff produced an empty pack (no adds and no removes)")

    add_paths = [rel for rel, _ in add_files]
    # Flat archive layout: USB-relative paths at the 7z root. contentRoot "." means
    # the installer extracts add[] straight onto the drive (no temp copy / no wrapper).
    content_root = "."

    out_path = args.out.resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    json_only = bool(args.json_only)
    if json_only and not out_path.is_file():
        raise SystemExit(
            f"--json-only expects an existing pack archive at --out (sidecar path is derived from it):\n  {out_path}"
        )

    staging_parent = args.staging.resolve() if args.staging else Path(tempfile.mkdtemp(prefix="medicat_update_pack_"))
    staging = staging_parent / "stage"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)

    try:
        manifest_path = staging / MANIFEST_NAME
        if add_files:
            write_manifest(add_files, manifest_path)
        else:
            # Removals-only pack still needs a valid manifest file for archiveMd5.
            manifest_path.write_text("", encoding="utf-8")
            # Empty manifest cannot be fingerprinted meaningfully for installer validation
            # (add list empty is rejected). Require at least one add when shipping a pack.
            raise SystemExit(
                "Diff produced removes only. Include at least one changed/new file, "
                "or add a placeholder marker file under --content."
            )

        # Fingerprint the exact on-disk bytes that become md5Manifest / MedicatUpdate.md5.
        manifest_bytes = manifest_path.read_bytes()
        archive_md5 = hashlib.md5(manifest_bytes).hexdigest()
        md5_manifest_text = manifest_bytes.decode("utf-8")

        info: dict[str, Any] = {
            "name": args.name,
            "version": args.version,
            "base": args.base,
            "contentRoot": content_root,
            "manifest": MANIFEST_NAME,
            "archiveMd5": archive_md5,
            "md5Manifest": md5_manifest_text,
            "bootablesOnly": bootables,
            "remove": remove_paths,
            "add": add_paths,
            "releaseManifest": release_path.name,
            "diff": {
                "contentTotal": stats["content_total"],
                "unchangedSkipped": stats["unchanged"],
                "added": stats["added"],
                "changed": stats["changed"],
                "remove": stats["remove"],
                "releaseEntries": stats["release_entries"],
                "completeTree": bool(args.complete),
                "bootablesOnly": bootables,
                "flatContent": True,
            },
        }
        write_info_json(staging / "info.json", info)

        sidecar_json = out_path.with_suffix(".json")
        write_info_json(sidecar_json, info)
        legacy_manifest_sidecar = out_path.with_suffix(".md5manifest")
        if legacy_manifest_sidecar.exists():
            legacy_manifest_sidecar.unlink()
        write_info_json(
            out_path.with_suffix(".diff.json"),
            {
                "add": add_paths,
                "remove": remove_paths,
                "stats": stats,
                "release": str(release_path),
                "content": str(content_dir),
                "bootablesOnly": bootables,
                "flatContent": True,
                "jsonOnly": json_only,
            },
        )

        if json_only:
            print(f"Wrote sidecar only (skipped 7z recompress): {sidecar_json}")
            print(f"  existing archive left untouched: {out_path}")
            print(f"  version={args.version} base={args.base} contentRoot={content_root} (flat)")
            if bootables:
                print("  scope: bootables only (ignored PortableApps/, Programs/)")
            print(
                f"  diff: added={stats['added']} changed={stats['changed']} "
                f"unchanged_skipped={stats['unchanged']} remove={stats['remove']}"
            )
            print(f"  pack add={len(add_paths)} remove={len(remove_paths)}")
            print(f"  archiveMd5 (manifest fingerprint)={archive_md5}")
            print("  note: only use --json-only when the .7z payload already matches this diff")
            return

        for rel, full in add_files:
            dest = staging / Path(rel)
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(full, dest)

        if out_path.exists():
            out_path.unlink()

        seven_za = find_7za(args.seven_za)
        top_levels = sorted({Path(rel).parts[0] for rel, _ in add_files})
        cmd = [
            str(seven_za),
            "a",
            "-t7z",
            "-mx=9",
            "-y",
            str(out_path),
            "info.json",
            MANIFEST_NAME,
            *top_levels,
        ]
        print("Running:", " ".join(cmd))
        completed = subprocess.run(cmd, cwd=staging, check=False)
        if completed.returncode != 0:
            raise SystemExit(f"7za failed with exit code {completed.returncode}")

        seven_md5 = md5_file(out_path)
        sidecar = Path(str(out_path) + ".md5")
        sidecar.write_text(f"{seven_md5} *{out_path.name}\n", encoding="utf-8")

        # Rewrite sidecar after archive exists (same info; keeps timestamps consistent).
        write_info_json(sidecar_json, info)

        print(f"Wrote {out_path}")
        print(f"  version={args.version} base={args.base} contentRoot={content_root} (flat)")
        if bootables:
            print("  scope: bootables only (ignored PortableApps/, Programs/)")
        print(
            f"  diff: added={stats['added']} changed={stats['changed']} "
            f"unchanged_skipped={stats['unchanged']} remove={stats['remove']}"
        )
        print(f"  pack add={len(add_paths)} remove={len(remove_paths)}")
        print(f"  archiveMd5 (manifest fingerprint)={archive_md5}")
        print(f"  sidecar json ({sidecar_json.name}) embeds md5Manifest")
        print(f"  sidecar 7z md5 ({sidecar.name})={seven_md5}")
    finally:
        if args.staging is None:
            shutil.rmtree(staging_parent, ignore_errors=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "--content",
        type=Path,
        required=True,
        help="Version folder (USB-relative tree) to diff against the current release",
    )
    parser.add_argument(
        "--release",
        type=Path,
        required=True,
        help="Current release MedicatFiles.md5 (hash map used for the diff)",
    )
    parser.add_argument(
        "--complete",
        action="store_true",
        help="Treat --content as a complete tree: release paths missing from content become remove[]",
    )
    parser.add_argument(
        "--bootables",
        action="store_true",
        help="Only consider bootables: ignore PortableApps/ and Programs/ in content and release diff",
    )
    parser.add_argument(
        "--remove",
        type=Path,
        help='Optional extra removals JSON: {"remove":["path",...]} or a bare string array',
    )
    parser.add_argument("--version", required=True, help="Pack version string (stored in info.json)")
    parser.add_argument("--base", required=True, help="Base MediCat version this pack targets (e.g. 21.12)")
    parser.add_argument("--name", default="MediCat USB Update Pack", help="Human-readable pack name")
    parser.add_argument("--out", type=Path, required=True, help="Output .7z path (e.g. MediCat.USB.v21.12.update.7z)")
    parser.add_argument(
        "--json-only",
        action="store_true",
        help="Rebuild sidecar <out>.json (with embedded md5Manifest) from the diff; do not recompress the .7z",
    )
    parser.add_argument("--staging", type=Path, default=None, help="Optional staging directory (kept on disk)")
    parser.add_argument("--7za", dest="seven_za", type=Path, default=None, help="Path to 7za.exe")
    args = parser.parse_args()

    build_pack(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
