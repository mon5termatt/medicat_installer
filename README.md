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
2. Place `MediCat.USB.v21.12.7z` in the **same folder** as the installer (or use the built-in download mirrors).
3. Run `MedicatInstaller.exe` **as Administrator**.
4. Select your USB drive (≥ 30 GiB; `C:` is hidden). The status bar reports whether Ventoy was detected.
5. Choose options, confirm the wipe warning, and install — or use **Check USB Files** to MD5-check an existing stick.

## What it does

- **Ventoy** — download, extract, fresh install (`/I`) or in-place upgrade (`/U`) based on drive state and checkboxes
- **Ventoy UI** — if Ventoy is missing: **Install Ventoy** is forced on; if present: optional **Update Ventoy?**
- **Format** — optional NTFS format; forced when Ventoy is not on the drive
- **Extract** — `MediCat.USB.v21.12.7z` via bundled `7za.exe` with live file log + status bar
- **Verify** — post-install or standalone MD5 check against embedded `MedicatFiles.md5`
- **Re-extract** — on verify failure, a window lists failed files and can selectively re-extract via `7za`, then re-verify
- **i18n** — English, Spanish, French, Polish, Turkish + in-app language selector

## Requirements

- Windows 10/11 x64 (ARM64 build possible from source)
- Administrator elevation (UAC)
- Internet for Ventoy download (first run; offline Ventoy zip cache supported)
- USB drive with **≥ 30 GiB** total capacity
- `MediCat.USB.v21.12.7z` beside the installer or downloaded via UI (~24 GB+ uncompressed)

## Build from source

Requires Visual Studio 2022 (or Build Tools), CMake 3.16+, Python 3, and repo assets `7za.exe`, `bin/7z.exe`, `MedicatFiles.md5`.

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build\Release\MedicatInstaller.exe`

See [`FEATURES.md`](FEATURES.md) for the feature checklist and [`TODO.md`](TODO.md) for planned work (e.g. support log upload).

## Logs (beside the exe)

Plain-text diagnostics only — these are the files users (and future support upload) should share:

| File | When |
|------|------|
| `medicat_installer.log` | Always — main session log |
| `extract.log` | During 7za extraction (raw stdout/stderr) |
| `reextract.log` | During selective re-extract of failed files |
| `check.log` | During MD5 verify — one line per file (`OK` / `FAIL` / `SKIP`) |
| `debug.log` | **On errors only** — OS/hardware info, tool versions, recent log tail |
| `failed_files.txt` | When verification finds failures |

If something breaks, attach `debug.log` and `medicat_installer.log` to your issue. A future release will upload **`.log` / `.txt` files only** to a support server with a Discord keyword — see [`TODO.md`](TODO.md).

## Repo layout

```
MedicatInstaller.exe   # release binary (or build/Release/ after build)
MediCat.USB.v21.12.7z  # user-supplied archive (not in repo)
7za.exe                # bundled into installer at build time
bin/7z.exe             # bundled into installer at build time
MedicatFiles.md5       # verification manifest (bundled)
src/                   # C++ source
i18n/                  # translations
tools/                 # build-time scripts (Ventoy list, i18n codegen)
```

Runtime folders (`Ventoy2Disk/`, `build/`, offline cache) are created locally and gitignored.

## Contributing

PRs welcome on the **`cpp`** branch (active C++ installer). Open issues or discuss large changes first if unsure.

### What to include in a PR

- Source changes under `src/`, `i18n/translations.json`, `tools/`, `res/`, `CMakeLists.txt`, and docs as needed.
- **All five languages** when adding or changing user-visible strings (`en`, `es`, `fr`, `pl`, `tr` in `i18n/translations.json`); rebuild so `src/i18n_generated.h` regenerates.
- UI updates from **worker threads** via `PostProgress` / `PostExtractProgress` / `PostStatusBar` / `PostDone` — never call Win32 GUI APIs directly from background threads.
- Status text via `Gui::SetStatusBar` / `App::PostStatusBar` (single line below the progress bar).

### What **not** to commit

These belong on disk locally or in release assets — **not in git**:

| Category | Examples | Why |
|----------|----------|-----|
| **MediCat archive** | `MediCat.USB.v*.7z`, any `*.7z` | Huge; user/downloaded separately |
| **Ventoy trees** | `Ventoy2Disk/`, `ventoy-*`, downloaded `ventoy.zip` | Downloaded/extracted at runtime |
| **Build output** | `build/`, `out/`, `bundle/`, `*.obj`, `*.pdb`, `*.ilk` | Regenerated by CMake/MSVC |
| **Log & diagnostics** | `*.log`, `failed_files.txt`, `medicat_download.log` | Session output beside the exe; may contain paths |
| **7-Zip temp files** | `7z_output.tmp`, `medicat_extract_list_*.txt`, `ventoy_stdout_*.txt` | Scratch from tooling |
| **Test / scratch dirs** | `test_extract/`, `extracted_medicat/`, `TestOutput_*/`, `_pkg/`, `old/` | Local experiments |
| **Offline cache (large)** | `offline/ventoy/`, `offline/*.7z` | Populate locally with `tools/populate_offline.py` |
| **IDE / OS junk** | `.vs/`, `.DS_Store`, `Thumbs.db`, `desktop.ini` | Machine-specific |
| **Secrets** | API keys, tokens, `.env`, personal paths in configs | Security |

**Also avoid:**

- Committing **`MedicatInstaller.exe`** or other built binaries (use [GitHub Releases](https://github.com/mon5termatt/medicat_installer/releases) for distributions).
- Checking in **edited `src/i18n_generated.h` by hand** — run a Release build (or the i18n codegen step) so it stays in sync with `translations.json`.
- PRs that only add **generated MSVC tlogs**, **CMake cache**, or other artifacts under `build/` (already gitignored; do not force-add).
- **Unrelated drive/USB images**, full USB dumps, or screenshots of personal file paths unless redacted.
- Porting **PowerShell installer patterns** into C++ (no `Invoke`/WinForms-style UI updates; use `WM_APP` message posting).

If you are unsure whether a file is local-only, check [`.gitignore`](.gitignore). When filing bugs, attach **log/text diagnostics only** (`*.log`, `*.txt` beside the exe) — not the MediCat `.7z` archive.

### Before you open a PR

1. Build Release locally and smoke-test on a spare USB or VHD if your change touches install/verify/Ventoy.
2. Confirm `git status` shows no `build/`, logs, archives, or `Ventoy2Disk/`.
3. Keep diffs focused — match existing style in the file you edit (C++17, wide strings for Windows paths/UI).

See [`FEATURES.md`](FEATURES.md) for parity goals and [`TODO.md`](TODO.md) for planned features.

## License

Scripts and installer orchestrate third-party tools (Ventoy, 7-Zip). Comply with their respective licenses.
