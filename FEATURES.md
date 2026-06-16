# C++ Installer — Feature Checklist

Track parity with the PowerShell installer (`pwsh` branch) and C++-specific enhancements.  
Legend: ✅ done · 🟡 partial · ⬜ not started

---

## Core install flow

| Feature | Status | Notes |
|---------|--------|-------|
| Win32 GUI (drive picker, install button) | ✅ | `src/gui.cpp` |
| Admin elevation (UAC manifest) | ✅ | `requireAdministrator` |
| USB drive detection | ✅ | Removable drives; hides drives under 30 GiB |
| VHD/VHDX drive detection | ✅ | `BusTypeFileBackedVirtual` |
| Skip `C:` drive | ✅ | |
| Format checkbox (NTFS) | 🟡 | `format.com` only; forced on when Ventoy not on drive |
| Ventoy action checkbox | ✅ | **Install Ventoy** (forced) vs **Update Ventoy?** (optional) based on detection |
| MediCat `.7z` extraction | ✅ | Bundled `7za.exe`, byte-based progress |
| Post-extract MD5 verification | ✅ | `MedicatFiles.md5` manifest; missing + hash mismatch |
| Progress bar + status bar | ✅ | Single-line status bar below progress; `SetStatusBar` / `PostStatusBar` |
| File log popup during extract/verify | ✅ | Optional listbox window; status bar shows `status.extracting_file` |
| Post-install files (icon, CheckFiles.bat) | ⬜ | PS: downloads from GitHub |
| Install log file | ✅ | `medicat_installer.log` |
| Raw 7za extract log | ✅ | `extract.log` (pipe tee, full stdout/stderr) |

---

## Ventoy

| Feature | Status | Notes |
|---------|--------|-------|
| Detect existing Ventoy on selected drive | ✅ | `TestVentoyInstalled` — `{drive}\ventoy` folder; status bar + checkbox mode |
| Download latest Ventoy from GitHub | ✅ | GitHub releases; optional pin in Advanced |
| Extract Ventoy zip | ✅ | Bundled `7za.exe` |
| Fresh install `VTOYCLI /I` | ✅ | When format checked or Ventoy not detected |
| Non-destructive upgrade `VTOYCLI /U` | ✅ | Ventoy present + Update Ventoy checked + format off |
| Ventoy warning dialog | ✅ | `ventoy_warning.*` |
| Wipe confirmation dialog | ✅ | `wipe_confirm.*` before install starts |
| Drive letter remap after Ventoy | ✅ | Disk-number tracking; confirm after Ventoy + final check before extract |
| Bundle or download Ventoy2Disk | ✅ | Download to `Ventoy2Disk\` beside exe |
| Advanced: pin Ventoy version | ✅ | Optional version combo under Advanced options |
| GPT / Secure Boot options | ✅ | Advanced: GPT checkbox + Secure Boot (`/GPT` / `/NOSB`) |

---

## File checking

| Feature | Status | Notes |
|---------|--------|-------|
| **Check USB Files** button | ✅ | Standalone MD5 verify on selected drive |
| Embedded `MedicatFiles.md5` | ✅ | Bundled in exe, extracted to temp at runtime |
| Verify files via MD5/hash | ✅ | Runs automatically after install extract |
| Show pass/fail summary | ✅ | Log + `failed_files.txt` on failure |
| Re-extract failed files (selective) | ✅ | `Extract7zArchiveSelective` via `7za @list`; `reextract.log` |
| Re-extract prompt window | ✅ | Dedicated window with failed-file list + **Re-extract** button |
| Re-verify after re-extract | ✅ | Normal success message if all pass |
| Still-failed hint (AV/firewall) | ✅ | `messages.verify_still_failed_after_reextract` |
| Support log upload (Discord keyword) | ⬜ | See [`TODO.md`](TODO.md) — logs/text only (`.log`, `.txt`) |

---

## Drive list / UX

| Feature | Status | Notes |
|---------|--------|-------|
| **Refresh drives** button | ⬜ | PS: `refresh_button` (list refreshes on show-all toggle / language change) |
| **Show all drives** checkbox | ✅ | Fixed disks (HDD/SSD) ≥ 30 GiB; `C:` still hidden |
| Default select first VHD | ✅ | |
| Drive size / free display | ✅ | `ui.drive_format`; volume label when set |
| Cancel button | ⬜ | PS had cancel |
| Internet check before install | ✅ | `TestInternetConnection` when Ventoy step runs |
| Archive download mirrors | ✅ | Missing-archive panel + offline cache paths |
| Antivirus / MOTD splash | ⬜ | Batch legacy |

---

## Translations (i18n)

| Feature | Status | Notes |
|---------|--------|-------|
| `translations.json` shared format | ✅ | `i18n/translations.json` |
| Build-time codegen | ✅ | `tools/i18n_codegen.py` |
| Auto-detect OS language | ✅ | `en` / `es` / `fr` / `pl` / `tr` |
| In-app language selector | ✅ | Header combo; live UI refresh |
| All UI strings via i18n | 🟡 | Main window + Ventoy + re-extract wired |
| Language override setting | ⬜ | Future: CLI flag or ini |

---

## Binaries / dependencies

| Feature | Status | Notes |
|---------|--------|-------|
| Bundle `7za.exe` in exe | ✅ | `bundle.cpp` |
| Bundle `7z.exe` in exe | ✅ | For Ventoy zip |
| SevenZipSharp / `lib/` | ⬜ | **Not needed** — 7za subprocess only |
| Self-update / version check | ⬜ | Batch: `curver` |

---

## Suggested next work

1. **Support log upload** — keyword + staff lookup; `.log` / `.txt` only ([`TODO.md`](TODO.md))
2. **Refresh drives** button — explicit UI control
3. **Post-install downloads** (icon, CheckFiles.bat)
4. **Internet check + MOTD**

---

## Reference

PowerShell implementation: `pwsh` branch  
- `MedicatInstaller.ps1` — main GUI  
- `Extract-Archive.ps1` — extraction (replaced by `src/extract.cpp`)
