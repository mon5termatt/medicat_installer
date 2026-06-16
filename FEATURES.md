# C++ Installer — Feature Restore Checklist

Track parity with the PowerShell installer (`pwsh` branch).  
Legend: ✅ done · 🟡 partial · ⬜ not started

---

## Core install flow

| Feature | Status | Notes |
|---------|--------|-------|
| Win32 GUI (drive picker, install button) | ✅ | `cpp/src/gui.cpp` |
| Admin elevation (UAC manifest) | ✅ | `requireAdministrator` |
| USB drive detection | ✅ | Removable drives; hides drives under 30 GiB |
| VHD/VHDX drive detection | ✅ | `BusTypeFileBackedVirtual` |
| Skip `C:` drive | ✅ | |
| Format checkbox (NTFS) | 🟡 | `format.com` only; no GPT/Ventoy format path |
| Skip Ventoy checkbox | ✅ | Checkbox + full skip path |
| MediCat `.7z` extraction | ✅ | Bundled `7za.exe`, byte-based progress |
| Post-extract MD5 verification | ✅ | `MedicatFiles.md5` manifest; missing + hash mismatch |
| Progress bar + status text | ✅ | Uses `status.extracting_*` keys |
| Current filename during extract | ✅ | From 7za stdout parser |
| Post-install files (icon, CheckFiles.bat) | ⬜ | PS: downloads from GitHub |
| Install log file | ✅ | `medicat_installer.log` |
| Raw 7za extract log | ✅ | `extract.log` (pipe tee, full stdout/stderr) |

---

## Ventoy

| Feature | Status | Notes |
|---------|--------|-------|
| Detect existing Ventoy (`VTOYEFI` / folder) | ✅ | `TestVentoyInstalled` — `{drive}\ventoy` folder |
| Download latest Ventoy from GitHub | ✅ | GitHub `releases/latest`; optional pin in Advanced |
| Extract Ventoy zip | ✅ | Bundled `7za.exe` |
| Fresh install `VTOYCLI /I` | ✅ | When format checked |
| Non-destructive upgrade `VTOYCLI /U` | ✅ | When format unchecked |
| Ventoy warning dialog | ✅ | `ventoy_warning.*` |
| Ventoy-not-detected confirm dialog | ✅ | `ventoy_not_detected.*` when upgrade + no folder |
| GPT / Secure Boot prompts | ✅ | Advanced options: GPT checkbox + Secure Boot (`/GPT` / `/NOSB`) |
| Wipe confirmation dialog | ✅ | `wipe_confirm.*` before install starts |
| Drive letter remap after Ventoy | ✅ | Disk-number tracking; confirm after Ventoy + final check before extract |
| Bundle or download Ventoy2Disk | ✅ | Download to `Ventoy2Disk\` beside exe |
| Advanced: pin Ventoy version | ✅ | Optional version field under Advanced options |

---

## File checking

| Feature | Status | Notes |
|---------|--------|-------|
| **Check USB Files** button | ✅ | Standalone MD5 verify on selected drive without re-extract |
| Download `MedicatFiles.md5` | ✅ | Embedded in exe, extracted to temp at runtime |
| Verify files via MD5/hash | ✅ | Runs automatically after install extract |
| Show pass/fail summary | ✅ | Log + `failed_files.txt` on failure |
| Re-extract missing files | ⬜ | PS: `Start-ReExtractFiles` with file list |
| Archive picker for re-extract | ⬜ | |

---

## Drive list / UX

| Feature | Status | Notes |
|---------|--------|-------|
| **Refresh drives** button | ⬜ | PS: `refresh_button` |
| **Show all drives** checkbox | ✅ | Includes fixed disks (HDD/SSD) ≥ 30 GiB; `C:` still hidden |
| Default select first VHD | ✅ | |
| Drive size / free display | ✅ | `ui.drive_format`; volume label shown when set |
| Cancel button | ⬜ | PS had cancel |
| Internet check before install | ✅ | `TestInternetConnection` when Ventoy enabled |
| Antivirus / MOTD splash | ⬜ | Batch legacy |

---

## Translations (i18n)

| Feature | Status | Notes |
|---------|--------|-------|
| `translations.json` shared format | ✅ | `i18n/translations.json` |
| Build-time codegen | ✅ | `tools/i18n_codegen.py` |
| Auto-detect OS language | ✅ | `en` / `es` / `fr` / `pl` / `tr` |
| In-app language selector | ✅ | Header combo; live UI refresh |
| All UI strings via i18n | 🟡 | Main window + Ventoy flow wired |
| Language override setting | ⬜ | Future: CLI flag or ini |

---

## Binaries / dependencies

| Feature | Status | Notes |
|---------|--------|-------|
| Bundle `7za.exe` in exe | ✅ | `bundle.cpp` |
| Bundle `7z.exe` in exe | ✅ | For Ventoy zip / future use |
| SevenZipSharp / `lib/` | ⬜ | **Not needed** — 7za subprocess only |
| Self-update / version check | ⬜ | Batch: `curver` |

---

## Suggested restore order

1. **Ventoy download + install/upgrade** — blocks real USB installs
2. **Check USB Files + MD5 verify** — high user value, simpler than Ventoy
3. **Refresh drives + show HDD** — quick UI wins
4. **Post-install downloads** (icon, CheckFiles.bat)
5. **Re-extract missing files**
6. **Internet check + MOTD**

---

## Reference

PowerShell implementation: `origin/pwsh` branch  
- `MedicatInstaller.ps1` — main GUI  
- `MedicatFileChecker.ps1` — file check (if separate)  
- `Extract-Archive.ps1` — extraction (replaced by `cpp/src/extract.cpp`)
