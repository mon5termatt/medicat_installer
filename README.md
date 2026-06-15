# MediCat USB Installer

> **HIGHLY EXPERIMENTAL** — native C++ preview. Expect bugs; test on a spare drive first.

Windows GUI installer for preparing a MediCat USB: Ventoy install/upgrade, optional NTFS format, MediCat archive extraction, and MD5 verification.

**Latest prerelease:** [GitHub Releases](https://github.com/mon5termatt/medicat_installer/releases) (tag `3520+`, `cpp` branch)

## Status

| Branch | Installer | Notes |
|--------|-----------|-------|
| `cpp` | **C++ / Win32** (active) | Single `MedicatInstaller.exe`, bundled tools, experimental |
| `pwsh` | PowerShell GUI | Previous alpha; still on GitHub for reference |
| `main` | Legacy batch/scripts | Older community maintenance |

## Quick start (release build)

1. Download `MedicatInstaller.exe` from [Releases](https://github.com/mon5termatt/medicat_installer/releases).
2. Place `MediCat.USB.v21.12.7z` in the **same folder** as the installer.
3. Run `MedicatInstaller.exe` **as Administrator**.
4. Select your USB drive (≥ 30 GiB; `C:` is hidden).
5. Choose options, confirm the wipe warning, and install — or use **Verify Files** to MD5-check an existing stick.

## What it does

- **Ventoy** — download, extract, fresh install or non-destructive upgrade (`VTOYCLI`)
- **Format** — optional NTFS format before extract
- **Extract** — `MediCat.USB.v21.12.7z` via bundled `7za.exe` with live file log
- **Verify** — post-install or standalone MD5 check against embedded `MedicatFiles.md5`
- **i18n** — English, Spanish, French, Polish, Turkish + in-app language selector

## Requirements

- Windows 10/11 x64 (ARM64 build possible from source)
- Administrator elevation (UAC)
- Internet for Ventoy download (first run)
- USB drive with **≥ 30 GiB** free/total capacity
- `MediCat.USB.v21.12.7z` beside the installer (~24 GB+ uncompressed)

## Build from source

Requires Visual Studio 2022 (or Build Tools), CMake 3.16+, and repo assets `7za.exe`, `bin/7z.exe`, `MedicatFiles.md5`.

```bat
cd cpp
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `cpp\build\Release\MedicatInstaller.exe`

See [`cpp/README.md`](cpp/README.md) for layout and [`cpp/FEATURES.md`](cpp/FEATURES.md) for parity checklist vs the PowerShell installer.

## Logs (beside the exe)

| File | When |
|------|------|
| `medicat_installer.log` | Always — main session log |
| `extract.log` | During 7za extraction (raw stdout/stderr) |
| `check.log` | During MD5 verify — one line per file (`OK` / `FAIL` / `SKIP`) |
| `debug.log` | **On errors only** — OS/hardware info, tool versions, recent log tail |
| `failed_files.txt` | When verification finds failures |

If something breaks, attach `debug.log` and `medicat_installer.log` to your issue.

## Repo layout

```
MedicatInstaller.exe   # release binary (or cpp/build/Release/ after build)
Medicat.USB.v21.12.7z  # user-supplied archive (not in repo)
7za.exe                # bundled into installer at build time
bin/7z.exe             # bundled into installer at build time
MedicatFiles.md5       # verification manifest (bundled)
cpp/                   # C++ source
i18n/                  # translations
tools/                 # build-time scripts (Ventoy list, i18n codegen)
```

Runtime folders (`Ventoy2Disk/`, `cpp/build/`, `bundle/`) are created locally and gitignored.

## Contributing

PRs welcome on the `cpp` branch. Do not commit archives, logs, or Ventoy extract trees. Match existing logging and UI threading patterns (`WM_APP` progress posts from worker threads).

## License

Scripts and installer orchestrate third-party tools (Ventoy, 7-Zip). Comply with their respective licenses.
