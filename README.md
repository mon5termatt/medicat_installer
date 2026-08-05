# MediCat USB Installer

Windows GUI (and CLI) installer for preparing a MediCat USB: Ventoy install/upgrade, optional NTFS format, MediCat archive extraction, and MD5 verification.

Native Win32 build. Ships **GUI + command-line** in one exe — no batch or PowerShell wrapper.

**Releases:** [GitHub Releases](https://github.com/mon5termatt/medicat_installer/releases) — C++ builds on semantic tags (`1.0.N`); download `MedicatInstaller.exe` / `MedicatInstaller-x86.exe`.

## Status

| Branch | Installer | Notes |
|--------|-----------|-------|
| `main` | **C++ / Win32** (active) | Single `MedicatInstaller.exe`, bundled tools, beta |
| `linux` | Linux helper scripts | Separate branch |

## Downloads

| File | Platform |
|------|----------|
| `MedicatInstaller.exe` | Windows x64 |
| `MedicatInstaller-x86.exe` | Windows 32-bit (x86) |
| `Medicat_Installer.sh` | Linux (from branch [`linux`](https://github.com/mon5termatt/medicat_installer/tree/linux); attached on each release) |

## What it does

**Install flow (GUI or CLI)**

- **Ventoy** — download, extract, fresh install or in-place upgrade
- **Format** — optional NTFS format before extraction (forced when Ventoy is not on the drive)
- **Extract** — `MediCat.USB.v21.12.7z` via bundled 7-Zip with live progress
- **Verify** — post-install or standalone MD5 check against embedded `MedicatFiles.md5`
- **Re-extract** — selective re-extract of failed files after verify (GUI prompt; CLI with `/yes` or `/reextract`)

**GUI**

- Drive picker (USB, VHD/VHDX; optional fixed disks ≥ 30 GiB)
- Dark theme, status bar, optional file-log window during extract/verify
- Inline archive mirror downloads with resume and progress
- i18n: English, Spanish, French, Polish, Turkish, Cat

**Command line**

- Dedicated console window for `/install` and `/verify` (not your parent CMD session)
- Live extract/verify progress in the console (`%` + current file)
- **Cancel** with Ctrl+C or by closing the console window
- Scripting flags: `/drive:`, `/format`, `/ventoy`, `/yes`, `/quiet`, `/allow-fixed`, `/archive:`, and more — run `/help` for the full list
- Exit codes for automation (`0` success, `1` error, `4` cancelled, `5` verify failed, …)

## Quick start (GUI)

1. Download the exe for your architecture from [Releases](https://github.com/mon5termatt/medicat_installer/releases).
2. Place `MediCat.USB.v21.12.7z` in the **same folder** as the installer (or use the built-in download mirrors).
3. Run **as Administrator**.
4. Select a USB drive (≥ 30 GiB; `C:` is hidden). The status bar reports whether Ventoy was detected.
5. Click **Install** or use **Check USB Files** on an existing stick.

## Quick start (CLI)

```bat
MedicatInstaller.exe /help
MedicatInstaller.exe /version
MedicatInstaller.exe /list-drives
MedicatInstaller.exe /install /drive:E /yes
MedicatInstaller.exe /verify /drive:E /yes
```

Logs are written to `medicat_installer.log` beside the exe.

## Requirements

- Windows 10/11
- Administrator elevation (UAC)
- Internet for Ventoy download (first run) and optional archive mirrors
- USB or VHD target with **≥ 30 GiB** capacity
- `MediCat.USB.v21.12.7z` beside the installer or downloaded via UI (~24 GB+ uncompressed)

## Notes

- **Beta** — report issues with `medicat_installer.log` attached, or share the **Diag code** from the error dialog if logs were uploaded.
- Stable batch-installer history remains on tag `3520` and earlier (`legacy` branch).
- CLI help/version strings are English-only for now; `/lang:` affects logged and dialog text where shown.
- Update tooling: see [`UPDATER.md`](UPDATER.md).
- Support telemetry: anonymous session reports and optional failure log upload — see [Logs](#logs-beside-the-exe).

## Build from source

Requires Visual Studio 2022 or newer (or Build Tools), CMake 3.16+, Python 3, `bin/7z/x64/7za.exe` or `bin/7z/x32/7za.exe`, and `MedicatFiles.md5`.

`rebuild.bat` picks the CMake generator from your installed Visual Studio (VS 2022 → `Visual Studio 17 2022`, VS 2026 → `Visual Studio 18 2026`). Override with `set MEDICAT_CMAKE_GENERATOR=...` if needed.

**Both architectures (recommended):**

```bat
rebuild.bat
```

Sets the build number to **one above the latest GitHub release** (so tags are not skipped), then one configure + one build produces:

- `build/Release/MedicatInstaller.exe` (x64)
- `build/Release/MedicatInstaller-x86.exe` (Win32)

Per-arch outputs also land in `build/x64/Release/` and `build/x86/Release/`. Each exe embeds the matching `7za.exe` for its CPU.

**Single architecture (manual):**

```bat
cmake -B build-x64 -G "Visual Studio 17 2022" -A x64
cmake --build build-x64 --config Release

cmake -B build-x86 -G "Visual Studio 17 2022" -A Win32
cmake --build build-x86 --config Release
```

On VS 2026 use `-G "Visual Studio 18 2026"` instead.

**Upload to GitHub** (after a successful build; version = tag = `1.0.N`):

```bat
rebuild.bat release
rebuild.bat as 1.0.42 release 1.0.42
tools\upload_release.bat
```

Creates the `1.0.N` release if needed and uploads both exes as GitHub Latest. Requires [`gh`](https://cli.github.com/) authenticated for the repo. Published releases also fire [`.github/workflows/release-webhook.yml`](.github/workflows/release-webhook.yml) when the `RELEASE_WEBHOOK_URL` secret is set.

See [`FEATURES.md`](FEATURES.md) for the feature checklist and [`TODO.md`](TODO.md) for planned work.  
Developer architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).  
Updater: [`UPDATER.md`](UPDATER.md).

## Logs (beside the exe)

Plain-text diagnostics only:

| File | When |
|------|------|
| `medicat_installer.log` | Always — main session log (includes system/diagnostic sections) |
| `extract.log` | During 7za extraction (raw stdout/stderr) |
| `reextract.log` | During selective re-extract of failed files |
| `check.log` | During MD5 verify — one line per file (`OK` / `FAIL` / `SKIP`) |
| `failed_files.txt` | When verification finds failures |

### Log server (beta)

During the beta, the installer can send diagnostics to the MediCat telemetry/support service:

| What | When |
|------|------|
| **Anonymous session stats** | End of install/verify (success or fail) — small JSON, no log files |
| **Failure logs** | On install/verify **failure** — allowlisted **`.log` / `.txt`** beside the exe only (not the MediCat archive or USB contents) |

On failure upload, the dialog shows a **Diag code** you can paste in Discord support so maintainers can find your bundle. The UI discloses this with a short beta notice; failure log upload can be disabled via settings (`failure_log_auto_upload_enabled`).

You can still attach `medicat_installer.log` manually when filing a GitHub issue. Details: [`docs/SUPPORT_UPLOAD.md`](docs/SUPPORT_UPLOAD.md), [`docs/SUPPORT_SERVER.md`](docs/SUPPORT_SERVER.md).

## Repo layout

```
MedicatInstaller.exe   # release binary (or build/Release/ after build)
MediCat.USB.v21.12.7z  # user-supplied archive (not in repo)
bin/7z/x64/7za.exe or bin/7z/x32/7za.exe  # bundled per build arch
MedicatFiles.md5       # verification manifest (bundled)
src/                   # C++ source
i18n/                  # translations
tools/                 # build-time scripts (Ventoy list, i18n codegen)
docs/                  # ARCHITECTURE.md — module map, flows, conventions
```

Runtime folders (`Ventoy2Disk/`, `build/`, offline cache) are created locally and gitignored.

## Contributing

PRs welcome on the **`main`** branch (active C++ installer). Open issues or discuss large changes first if unsure.

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
