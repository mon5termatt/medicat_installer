# Medicat Installer — Optimization & Cleanup Findings

Analysis of `Medicat_Installer.bat` (663 lines) and its dependency on `bin/`, with a phased plan to clean up, fix bugs, and reduce external binary reliance.

**Date:** 2026-06-14  
**Branch context:** Analysis started on `main`; continued work targets `origin/pwsh`  
**Files reviewed:** `Medicat_Installer.bat`, `Rewrite_Medicat_Installer.bat`, `bin/*`, `translate/*`, `7z/*`, `hasher/*`, `MedicatFileChecker.ps1`, `origin/pwsh:MedicatInstaller.ps1`, `origin/pwsh:MedicatInstaller-Outline.md`

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current Architecture](#current-architecture)
3. [bin/ Dependency Map](#bin-dependency-map)
4. [Bugs & Issues](#bugs--issues)
5. [Dead & Unreachable Code](#dead--unreachable-code)
6. [Code Smells & Maintenance Pain](#code-smells--maintenance-pain)
7. [Existing Assets in Repo](#existing-assets-in-repo)
8. [pwsh Branch Status](#pwsh-branch-status)
9. [7-Zip Progress Problem (Major Blocker)](#7-zip-progress-problem-major-blocker)
10. [Optimization Plan (Phased)](#optimization-plan-phased)
11. [Open Decisions](#open-decisions)
12. [Suggested Execution Order](#suggested-execution-order)

---

## Executive Summary

`Medicat_Installer.bat` is a monolithic batch script that:

- Downloads **10+ files into `bin/` on every run**, validating integrity with **hard-coded byte sizes** (fragile).
- Uses a **TheBATeam console GUI stack** (`Button` → `Getlen` → `Box` → `batbox` → `GetInput`) for all interactive menus.
- Copies **7z** and **PowerShell translate scripts** into `bin/` even though they already exist elsewhere in the repo.
- Contains several **real bugs** (variable shadowing, broken error detection, infinite loops).
- Has a **partial refactor** in `Rewrite_Medicat_Installer.bat` that fixes some patterns but leaves the install flow incomplete.

The repo already contains better alternatives (`MedicatFileChecker.ps1` with WinForms + `Get-FileHash`, `7z/x64` and `7z/x32` vendored, `translate/*.ps1` in-repo). A phased cleanup can eliminate most or all `bin/` runtime dependency without changing core install behavior (Ventoy + 7z extraction).

---

## Current Architecture

### High-level flow

```
Init + PATH includes bin/
    → initialchecks (internet, curl, PowerShell, admin, OS version)
    → curver (self-update check via GitHub tags)
    → startup (antivirus warning)
    → startbinfiles (download 10+ files to bin/)
    → start (ASCII art splash)
    → menu / menu2 (licence + interactive menu)
    → [Install] check5 (Ventoy version check & download)
    → install2 (drive selection)
    → {Format?} → GPT + Secure Boot prompts → ventoyinstall + format
              → or updateventoy (no format)
    → installver (find/extract MediCat archive)
    → finishup (icon, CheckFiles.bat, exit)
```

### Key variables (startup)

| Variable | Default | Purpose |
|----------|---------|---------|
| `localver` | `3520` | Installer version (used in title, update check) |
| `medicatver` | `21.12` | MediCat USB archive version |
| `format` | `Yes` | Whether to format USB before Ventoy install |
| `formatcolor` | `2F` | Console button color for format toggle |
| `bit` | `64` or `32` | Architecture for 7z binary selection |
| `lang` | from registry | Two-letter OS language for translate scripts |
| `maindir` | `%CD%` | Script root directory |

### PATH setup (line 5)

```bat
Set "Path=%Path%;%CD%;%CD%\bin;"
```

Everything in `bin/` is invoked by bare name (`Button`, `GetInput`, `7z`, `folderbrowse.exe`, etc.).

---

## bin/ Dependency Map

### Files downloaded at runtime (`:startbinfiles`, lines 101–157)

| File | Size check | Purpose |
|------|------------|---------|
| `QuickSFV.exe` | 103424 | MD5 file verification |
| `QuickSFV.ini` | 158 | QuickSFV config |
| `Box.bat` | 5874 | Draw bordered boxes on console |
| `Button.bat` | 5254 | Clickable CMD “buttons” |
| `GetInput.exe` | 3584 | Mouse hit-testing on console buttons |
| `Getlen.bat` | 1897 | String length (for button sizing) |
| `batbox.exe` | 1536 | Console graphics rendering |
| `folderbrowse.exe` | 8192 | Drive/folder picker dialog |
| `7z.exe` | *(skipped)* | Archive extraction (from `7z/%bit%.exe`) |
| `7z.dll` | *(skipped)* | 7z library (from `7z/%bit%.dll`) |
| `motd.ps1` | *(no check)* | Translated MOTD (from `translate/`) |
| `licence.ps1` | *(no check)* | Translated licence (from `translate/`) |

### Files committed in repo under `bin/`

Only batch helpers are in git; binaries are fetched at runtime:

- `Box.bat`
- `Button.bat`
- `Getlen.bat`
- `QuickSFV.ini`
- `md5.bat`

### Console UI plugin chain (TheBATeam)

```
Medicat_Installer.bat
    call Button ... X _Var_Box _Var_Hover
        Button.bat
            call Getlen (string width)
            call Box (draw button border)
                Box.bat → batbox.exe (render)
        Batbox (draw button labels)
    GetInput /M %_Var_Box% /H %_Var_Hover%  (mouse click → errorlevel 1–5)
```

**Used in:** main menu, GPT prompt, Secure Boot prompt, download-yes/no, torrent vs CDN.

### Where each bin asset is called

| Asset | Lines / labels |
|-------|----------------|
| `Button` + `GetInput` | `:menu2`, `:sbask`, `:gptask`, `:installversion2`, `:bigboi` |
| `folderbrowse.exe` | `:install2` (317), `:recheck` (606) |
| `QuickSFV.EXE` | `:hasher` (593) |
| `7z` | Ventoy extract (294), MediCat extract (531, 538, 544) |
| `licence.ps1` / `motd.ps1` | `:menu` (224), `:menu2` (237) — via `bin\` path |

---

## Bugs & Issues

### Critical

| # | Location | Issue |
|---|----------|-------|
| 1 | Lines 287–288 | **Variable collision:** Ventoy local version is read into `localver`, overwriting installer version `3520`. Menu title `%localver%` and version logic break after Ventoy runs. |
| 2 | Lines 548–555 | **Broken 7z success detection:** `if errorlevel 0 goto finishup2` is always true after any command. Real 7z failures (errorlevels 1, 2, 7, 8, 255) are checked above, but the final branch is misleading and fragile. |
| 3 | Lines 413–427 | **Infinite drive reconfirm loop:** `:vencheck` uses `Set /P drivepath=... && goto vencheck`. Pressing Enter without typing does not exit; user can be stuck. |
| 4 | Line 388 | **Typo:** `set set arg2=/NOSB` — duplicate `set`; Secure Boot “NO” path may not set `arg2` correctly. |

### Moderate

| # | Location | Issue |
|---|----------|-------|
| 5 | Line 320 | **Fragile drive validation:** `IF "%drivepath%" == "~0,1"` is a weak sentinel when folder browse fails or returns unexpected output. |
| 6 | Line 132 | **7z never validated:** `goto checkdone` skips `:check64` / `:check32`. Dead validation paths; 7z download failures go unnoticed. |
| 7 | Lines 164–166 | **Triple `pause`** in `:hasherror` — redundant UX. |
| 8 | Line 36–37 | **Admin elevation:** `_args%` referenced but never defined; elevation relaunch may drop arguments. |
| 9 | `:check5` flow | After Ventoy update, execution falls through to `:install2` with no explicit `goto` — works by fall-through but is hard to follow and easy to break. |

### Minor / UX

| # | Location | Issue |
|---|----------|-------|
| 10 | Line 324 | Comment `:: FIX THE !!!. ITS BROKEN` — warning banner issue acknowledged but not fixed. |
| 11 | Line 288 | Displays `Current Local Version` using `%VENVER:~-6%` (remote) instead of local Ventoy version variable. |
| 12 | `:formatswitch` | Awkward `if/else` chain (lines 257–259); could be two-branch toggle. |
| 13 | `install4` | Hard-coded `MediCat.USB.v21.12.7z` and hashes (lines 527–529) while `medicatver` variable exists elsewhere. |

---

## Dead & Unreachable Code

| Label / block | Lines | Notes |
|---------------|-------|-------|
| `:check64` | 135–140 | Never reached; `:startbinfiles` jumps to `:checkdone` |
| `:check32` | 142–147 | Never reached |
| `:checkfile` | 659 | Empty label, immediately followed by `:filesize` |
| `:error` | 432–435 | “Nothing was chosen” — no `goto error` from `:install2` |
| `:exit` | 656–657 | Defined but unused in main flow |
| `::if defined ProgramFiles(x86)...` | 133 | Commented-out arch branch |

---

## Code Smells & Maintenance Pain

1. **Monolithic goto spaghetti** — 663 lines, few subroutines; `Rewrite_Medicat_Installer.bat` started extracting `:check_internet`, `:toggle_format`, etc. but did not finish.

2. **Runtime re-download every run** — Even when `bin/` is populated, `:startbinfiles` re-fetches all files. No “skip if valid” logic.

3. **Integrity = file size only** — Fixed byte counts break when any upstream file changes. No SHA256 manifest.

4. **Duplicated paths** — Scripts copied into `bin/` that already live at `translate/` and `7z/`.

5. **Mixed concerns** — UI, networking, Ventoy CLI, formatting, extraction, and i18n in one file.

6. **Execution policy changes** — Lines 154–155 set `RemoteSigned` for LocalMachine and CurrentUser on every run (heavy-handed).

7. **Temp file litter** — `curver.ini`, `ventoy.zip`, `ventoyversion.txt`, `drivefiles.md5`, `tor.bat`, `cdn.bat`, `.wget-hsts` — no centralized cleanup in main script (Rewrite lists `temp_files` but main script does not).

8. **Architecture detection** — Uses `ProgramFiles(x86)` for 32 vs 64 only; repo has `7z/arm64/` but installer does not use it.

---

## Existing Assets in Repo

These can replace `bin/` dependencies without new downloads:

| Asset | Path | Can replace |
|-------|------|-------------|
| 7-Zip binaries | `7z/x64/`, `7z/x32/`, `7z/arm64/` | `bin/7z.exe`, `bin/7z.dll` |
| Translate scripts | `translate/licence.ps1`, `translate/motd.ps1` | `bin/licence.ps1`, `bin/motd.ps1` |
| File checker (GUI) | `MedicatFileChecker.ps1` | QuickSFV + `hasher/CheckFiles.bat` flow |
| Translations | `translations.json`, `TranslationHelper.ps1` | Hardcoded / Google Translate in licence.ps1 |
| Partial cleanup | `Rewrite_Medicat_Installer.bat` | Patterns for checks, `choice`, subroutines |
| Linux installer | `Medicat_Installer.sh` | Reference for separated concerns |

### Rewrite_Medicat_Installer.bat status

**Improvements already drafted:**

- Subroutines for internet, curl, PowerShell, admin, Windows checks
- `temp_files` cleanup list
- `choice`-based download menu (torrent vs CDN)
- `:toggle_format`, `:check_usb_files` helpers
- `net session` for admin instead of registry S-1-5-19 trick

**Still incomplete:**

- `:check5` does not call `:install_medicat` after Ventoy update
- Still downloads full `bin/` set in `:startbinfiles`
- Still uses `Button` / `GetInput` for main menu
- `:install_medicat` function defined but not wired from menu choice `1`

---

## pwsh Branch Status

**Remote branch:** `origin/pwsh` (4 commits, diverged from `main` — **no merge base**)

Switch locally:

```powershell
git fetch origin pwsh
git checkout -B pwsh origin/pwsh
```

### What exists on `pwsh` (not on `main`)

| File | Purpose |
|------|---------|
| `MedicatInstaller.ps1` | ~1957-line WinForms GUI installer (primary entry point) |
| `MedicatInstaller-Outline.md` | Technical documentation for the PS installer |
| `TranslationHelper.ps1` | i18n loader (also on `main`) |
| `translations.json` | Full UI strings including `extracting_progress`, `7Zip4PowerShell` |
| `MedicatFileChecker.ps1` | File verification GUI (shared with `main`) |
| `Medicat_Installer.bat` | Legacy batch launcher (older copy) |
| `bin/*` | Full binary set **committed** (not just downloaded) |
| `old/*` | Archived batch installer versions |
| `generate_translation.py` / `validate_translations.py` | Translation tooling |

### What `MedicatInstaller.ps1` already implements

From `MedicatInstaller-Outline.md` and source review:

- **Admin auto-elevation** with working-directory preservation
- **WinForms GUI** — drive picker, format toggle, progress bar, log panel
- **Ventoy** — GitHub version check, download, extract, `VTOYCLI /I` and `/U`
- **7Zip4PowerShell** — auto-install from PSGallery, `Expand-7Zip` extraction
- **Progress workaround** — `Start-Job` for extraction + **destination folder size polling** while job runs
- **Logging** — `medicat_download.log` with throttled progress entries
- **Translations** — EN/ES/FR via `TranslationHelper.ps1`

### Tier 2 — vendored DLLs (implemented 2026-06-14)

**`lib/`** ships with the installer (no PSGallery):

| File | Purpose |
|------|---------|
| `SevenZipSharp.dll` | .NET wrapper — `PercentDone` via `Extracting` event |
| `7z64.dll` | Native 7-Zip (64-bit) |
| `7z.dll` | Native 7-Zip (32-bit) |
| `7zARM64.dll` | Native 7-Zip (ARM64) |
| `NOTICE.txt` | LGPL attribution |

`Extract-Archive.ps1` calls `Initialize-MedicatSevenZipSharp` → `SetLibraryPath` → direct extraction.  
`7z.exe` CLI fallback remains if `lib/` is incomplete.

### Extraction progress on `pwsh` — implemented (2026-06-14)

**`Extract-Archive.ps1`** shared helper:

| Method | Progress source |
|--------|-----------------|
| **SevenZipSharp** (primary) | `add_Extracting` → real `PercentDone` (tested: 101 unique 0–100% values) |
| **7z.exe CLI** (fallback) | `-bsp1 -bso1 -bse1` byte-stream parsing |

**Wired into:**

- `MedicatInstaller.ps1` — full archive install (~500 lines of job/polling code removed)
- `MedicatFileChecker.ps1` — partial re-extract with `FileList`
- `MedicatInstaller.ps1` — `Start-ReExtractFiles` updated

**Removed patterns:**

- `Start-Job` + folder size polling (`archiveSize * 2` estimate)
- `Start-Process -Wait` + redirect to temp files (fake 0%→100%)
- Hard failure when `7Zip4PowerShell` missing (fallback re-enabled)

### Previous limitations (resolved)

| Issue | Status |
|-------|--------|
| **No true 7-Zip percent** | **Fixed** — `PercentDone` via SevenZipSharp `Extracting` event |
| **7z.exe fallback disabled** | **Fixed** — `-AllowCliFallback` with `-bsp1` parsing |
| **Ventoy extract still blind** | Open — Ventoy zip still uses `Start-Process -Wait` |
| **Still depends on `bin/`** | Open — `Download-BinFiles()` unchanged |
| **Duplicate re-extract logic** | **Fixed** — shared `Extract-Archive.ps1` |

**Remaining limitations:**

- Solid archives may still report coarse `PercentDone` during early extraction (7-Zip block behavior)
- Ventoy zip extraction has no progress bar yet

### Recommended next steps on `pwsh`

1. ~~**Hook native progress**~~ — Done in `Extract-Archive.ps1`
2. ~~**Re-enable 7z fallback**~~ — Done (`-AllowCliFallback`)
3. ~~**Extract shared helper**~~ — Done (`Extract-Archive.ps1`)
4. **Ventoy extract progress** — Apply same helper or indeterminate bar for Ventoy zip
5. **Reduce `bin/`** — Apply Phase 2–3 from optimization plan
6. **Merge strategy** — No common ancestor with `main`; cherry-pick `main` fixes as needed

### Chat context preserved here

This document captures findings from the `main` branch review session (2026-06-14):

- Full `Medicat_Installer.bat` audit (663 lines, bugs, dead code, `bin/` map)
- Decision that **7-Zip CLI progress is the major blocker** for batch-only approach
- Tested `-bsp1` behavior on vendored `7za.exe` 25.00
- Ranked solutions: 7Zip4PowerShell → folder polling → visible console → indeterminate bar
- Confirmation that `translations.json` on `main` was ahead of `main` code but **matches pwsh direction**

---

**This is the hardest technical problem in the installer.** Reliable extraction progress cannot be obtained from `7z.exe` the way the script currently invokes it. This affects both `Medicat_Installer.bat` and `MedicatFileChecker.ps1`, and it partially explains why the project still depends on vendored `7z` in `bin/` with no real UI feedback during multi-GB extraction.

### Current behavior in this repo

| Location | What it does | Actual progress? |
|----------|--------------|------------------|
| `Medicat_Installer.bat` lines 531–544 | `7z x -O%drivepath%: %file% -r -aoa` — dumps raw output | **No** — user sees scrolling log only |
| `MedicatFileChecker.ps1` lines 683–722 | Comment says "progress monitoring" but uses `Start-Process -Wait` + redirect to temp files | **No** — sets 0%, then 100% when done |
| `translations.json` (on `main`) | Strings for `7Zip4PowerShell`, `extracting_progress` | **Implemented on `pwsh`** via `MedicatInstaller.ps1`; **not on `main`** |

### Why `7z.exe` progress is unreliable

7-Zip does **not** emit progress as normal newline-delimited log output.

1. **Console drawing, not logging** — By default, the `%` indicator is written directly to the console using carriage returns (`\r`) to overwrite the same line. It is not designed for pipes, batch redirects, or `ReadLine()`.

2. **Separate output streams** — Normal messages, errors, and progress use different streams. Without explicit flags, redirected processes often get **nothing until the process exits**.

3. **`-Wait` + redirect kills streaming** — `MedicatFileChecker.ps1` uses:
   ```powershell
   Start-Process ... -Wait -RedirectStandardOutput "7z_output.tmp"
   ```
   This buffers all output until extraction finishes — the worst possible pattern for progress.

4. **Solid archives are coarse** — MediCat ships as a large **solid** `.7z`. Tested locally with a solid archive: stdout contained only `0%`, then `100%`, with occasional filenames — not smooth 1% increments. A multi-GB solid extract can sit at `0%` for a long time even when parsing works.

5. **Batch cannot parse streams** — `Medicat_Installer.bat` has no mechanism to read a live stdout pipe. Any batch-based solution needs PowerShell or a helper executable.

### What partially works: `-bsp1 -bso1 -bse1`

7-Zip supports stream redirection via `-bs{o|e|p}{0|1|2}` (confirmed on vendored `7za.exe` 25.00):

```
-bs{o|e|p}{0|1|2} : set output stream for output/error/progress line
```

**Required flags for programmatic capture:**

```
7z x -bsp1 -bso1 -bse1 -o"DEST" "archive.7z" -aoa
```

| Flag | Meaning |
|------|---------|
| `-bsp1` | Send **progress** line to stdout |
| `-bso1` | Send normal output to stdout |
| `-bse1` | Send errors to stdout |

**Parsing requirements:**

- Do **not** use `ReadLine()` — progress lines end with `\r`, not `\n`.
- Read stdout **byte-by-byte** or in small chunks.
- Apply regex `(\d+)%` on the rolling buffer.
- Expect `\r` overwrite sequences like: `  0%\r    \r100% 5 - file103.bin\r`

**Example PowerShell pattern (console host only — not WinForms):**

```powershell
$psi = @{
    FileName = $sevenZip
    Arguments = "x -bsp1 -bso1 -bse1 -o`"$dest`" `"$archive`" -aoa"
    UseShellExecute = $false
    RedirectStandardOutput = $true
    CreateNoWindow = $true
}
$p = Start-Process @psi -PassThru
$buf = New-Object System.Text.StringBuilder
while (-not $p.StandardOutput.EndOfStream) {
    $c = $p.StandardOutput.Read()
    if ($c -lt 0) { break }
    [void]$buf.Append([char]$c)
    if ([char]$c -eq '%') {
        $m = [regex]::Match($buf.ToString(), '(\d+)%')
        if ($m.Success) { $pct = [int]$m.Groups[1].Value }
    }
}
$p.WaitForExit()
```

**Limitations of this approach:**

- Still **coarse** on solid MediCat archives.
- Fragile across 7-Zip versions/locales.
- Hard to wire into a **WinForms** `ProgressBar` without a background reader thread.
- `7za.exe` vs `7z.exe` — project uses full `7z` with DLL; same `-bs` flags apply.

### Recommended solutions (ranked)

#### Option A — 7Zip4PowerShell (recommended for GUI installer)

`translations.json` already references this module — it was clearly the intended direction.

- Uses **SevenZipSharp** (.NET wrapper around 7-Zip's native API).
- Fires `Extracting` events with `args.PercentDone`.
- Integrates with `Write-Progress` or can drive a WinForms bar directly.

```powershell
# Requires: Install-Module 7Zip4PowerShell
Expand-7Zip -ArchiveFileName $archive -TargetPath $dest
# Emits Write-Progress; hook extractor.Extracting for custom UI
```

| Pros | Cons |
|------|------|
| Real API-level percent events | Requires PS Gallery module (or bundle DLL) |
| Works with WinForms progress bar | Module may lag behind latest 7-Zip |
| Already anticipated in translations | Needs `-CustomInitialization` or bundled `7z.dll` path for vendored binary |

**Best fit if:** Installer UI moves to PowerShell (`MedicatFileChecker.ps1` pattern).

#### Option B — Destination size polling (no new deps)

Before extraction:

```bat
7z l -slt "MediCat.USB.v21.12.7z"
```

Sum the `Size =` fields → total uncompressed bytes. During extraction, poll destination drive used space in a background loop:

```
percent = (current_folder_size / expected_total_size) * 100
```

| Pros | Cons |
|------|------|
| Works with existing vendored `7z.exe` | Approximate — NTFS overhead, Ventoy partitions |
| Smooth bar movement | Overcounts if old files exist on USB |
| No stream parsing | Requires background thread + admin path access |

**Best fit if:** Staying on vendored CLI `7z` without PS Gallery deps.

#### Option C — Show 7z in a visible console window

```bat
start /wait cmd /c "7z x -oE: archive.7z & pause"
```

| Pros | Cons |
|------|------|
| 100% reliable native 7-Zip progress | Ugly UX, extra window |
| Zero parsing | User must not close window |
| Trivial to implement | No integrated progress bar |

**Best fit if:** Quick fix for batch-only installer.

#### Option D — Indeterminate progress + status text

Accept that solid-archive percent is meaningless. Show:

- Marquee / pulsing progress bar
- Elapsed time
- Last parsed filename from `-bsp1` stream (if available)
- Archive size from `7z l`

| Pros | Cons |
|------|------|
| Honest UX | No true percent |
| Easy in WinForms | Users may think it's frozen |

**Best fit if:** Combined with Option A or B as fallback.

#### Option E — File-count progress (partial extracts only)

Works well in `MedicatFileChecker.ps1` re-extract (known file list) — count files verified on disk vs total. **Does not work** for full initial MediCat install (thousands of files, solid block extracts as one stream).

### What not to do

| Approach | Why it fails |
|----------|--------------|
| `7z x ... > log.txt` in batch | Buffered until complete |
| `Start-Process -Wait -RedirectStandardOutput` | Same — no streaming |
| `ReadLine()` on stdout | `\r`-based lines never delimit |
| Parsing default 7z output without `-bsp1` | Progress never enters pipe |
| Expecting smooth % on solid `.7z` | 7-Zip reports block-level progress |

### Impact on optimization plan

The 7-Zip progress problem **pushes the installer toward PowerShell** for extraction:

1. **Batch-only refactor cannot solve progress** — keep batch for Ventoy CLI / admin elevation; delegate extraction to PS.
2. **`bin/7z` can stay vendored** — but extraction should be wrapped, not called naked from batch.
3. **`MedicatFileChecker.ps1` needs rewrite** of `Start-ReExtractFiles` — the comment on line 683 is misleading; it does not monitor progress today.
4. **`translations.json` is ahead of `main` code** — on `pwsh`, `MedicatInstaller.ps1` implements the intended direction; `main` batch installer does not.

### pwsh branch next step (updated)

```
On branch pwsh:
  └── Refine MedicatInstaller.ps1 extraction:
        - Wire 7Zip4PowerShell Extracting event → WinForms ProgressBar (true %)
        - Replace archiveSize*2 estimate with 7z l -slt uncompressed total
        - Re-enable 7z.exe fallback with -bsp1 or visible console
        - Share logic with MedicatFileChecker.ps1 via Extract-Archive.ps1
```

### Original suggested path (from main branch analysis)

```
Phase 1 (quick win)
  └── Add Extract-Archive.ps1 helper:
        - Try 7Zip4PowerShell first (real progress)
        - Fallback: 7z -bsp1 byte-reader + indeterminate bar
        - Fallback: visible console window

Phase 2 (Medicat_Installer.bat)
  └── Replace naked `7z x` calls with:
        powershell -File Extract-Archive.ps1 -Archive ... -Destination ...

Phase 3 (MedicatFileChecker.ps1)
  └── Replace Start-Process -Wait block with shared Extract-Archive.ps1
```

### Tested locally (2026-06-14)

Using vendored `7z\x64\7za.exe` 25.00:

- Without `-bsp1`: progress invisible to redirected stdout.
- With `-bsp1 -bso1 -bse1`: stdout captured, but output uses `\r` overwrites.
- Solid 200-file test archive: only **3** `%` matches in full output (`0`, `100`, and intermediate `100` with filename).

---

## Optimization Plan (Phased)

### Phase 1 — Cleanup only (low risk, same UX)

**Goal:** Maintainable batch file without changing user-facing behavior.

- [ ] Fix all bugs listed in [Bugs & Issues](#bugs--issues)
- [ ] Rename `localver` → `installer_ver`; use `ventoy_local_ver` for Ventoy
- [ ] Remove dead labels (`:check64`, `:check32`, `:checkfile`, unused `:error`, `:exit`)
- [ ] Adopt subroutine pattern from `Rewrite_Medicat_Installer.bat`
- [ ] Centralize temp file cleanup
- [ ] Run `translate\licence.ps1` and `translate\motd.ps1` directly (stop copying to `bin/`)
- [ ] Point PATH at `7z\x64` or `7z\x32` instead of copying into `bin/`
- [ ] Consolidate duplicate menu `If errorlevel` blocks

**Outcome:** Shorter script, fewer network calls, same console-button UI.

---

### Phase 2 — Reduce bin/ dependency (medium risk)

| Remove from bin/ | Replacement |
|------------------|-------------|
| Button + GetInput + batbox + Box + Getlen | `choice /c YN` for yes/no; numbered menu for main menu |
| folderbrowse.exe | PowerShell `FolderBrowserDialog` (see `MedicatFileChecker.ps1`) or existing `mshta` picker in `:installerror` |
| QuickSFV.EXE | `Get-FileHash -Algorithm MD5` in PowerShell |
| 7z in bin/ | `%CD%\7z\x64` or `x32` on PATH |
| motd/licence in bin/ | Always `translate\` |

#### UI replacement options

| Option | Pros | Cons |
|--------|------|------|
| **A. `choice` + `echo` menus** | Pure batch, no extra deps | Less polished than clickable buttons |
| **B. PowerShell WinForms** | Modern UI; reuse `MedicatFileChecker.ps1` | Requires PowerShell |
| **C. Hybrid** | ASCII splash in batch; PS for dialogs only | Two languages, clear split |

**Recommendation:** Option C — batch for orchestration (admin, Ventoy CLI, 7z), PowerShell for user interaction.

**After Phase 2:** 6 of 8 bin downloads removable in one pass.

---

### Phase 3 — Eliminate runtime bin/ downloads (high impact)

Today `:startbinfiles` re-downloads ~10 files every run.

**Alternatives:**

1. **Vendor in repo** — commit stable tools under `lib/`; drop download block for offline use
2. **Lazy fetch** — download only if missing; verify with **SHA256 manifest** instead of file size
3. **Single bootstrap** — one `bootstrap.ps1` fetches and validates once

Removes startup delay and `:hasherror` size-mismatch fragility.

---

### Phase 4 — Structural refactor (optional, larger)

Target layout:

```
Medicat_Installer.bat     → thin launcher (admin, cd, call PowerShell)
Medicat_Installer.ps1       → UI + flow control
lib/ or 7z/                 → vendored 7z only
translate/                  → i18n scripts
hasher/                     → merge into MedicatFileChecker.ps1
```

Aligns Windows installer with `Medicat_Installer.sh` separation model.

---

## Open Decisions

Before implementation, choose:

1. **UI direction** — Console `choice` menus (2A) or PowerShell dialogs (2C)?
2. **Scope** — Phase 1 cleanup only first, or remove Button/GetInput stack immediately?
3. **File checker** — Keep `hasher/CheckFiles.bat` on USB, or standardize on `MedicatFileChecker.ps1`?
4. **7z** — Keep vendored `7z/x64` and `x32`, or require system 7-Zip?
5. **ARM64** — Add `7z/arm64` detection for Windows on ARM?

---

## Suggested Execution Order

| Step | Work | bin/ files removed |
|------|------|-------------------|
| 1 | Fix bugs + dead code + variable names | 0 |
| 2 | Use `translate\` and `7z\x64\` directly | 2 (ps1 copies + 7z copy) |
| 3 | Replace yes/no prompts with `choice` | 5 (Button chain + GetInput) |
| 4 | Replace `folderbrowse.exe` with PS dialog | 1 |
| 5 | Replace QuickSFV with PS hashing / `MedicatFileChecker.ps1` | 2 |
| 6 | Remove `:startbinfiles` entirely | All runtime downloads |

**After step 6:** `bin/` can be deleted or emptied; PATH no longer needs `%CD%\bin`.

---

## What to Keep vs. Drop

### Keep (reasonable dependencies)

- `curl` (Windows built-in)
- `powershell` (required for translate scripts and future UI)
- Ventoy `Ventoy2Disk.exe` (downloaded per release from GitHub)
- `7z` binaries in `7z/` (vendor in repo; do not mirror into `bin/`)

### Drop from bin/ dependency

- Entire TheBATeam console GUI stack (batbox, Box, Button, Getlen, GetInput)
- `folderbrowse.exe`
- `QuickSFV.EXE`
- Runtime download-to-`bin/` for files already in the repo

---

## References

- TheBATeam plugins (acknowledged in README): [batbox](https://github.com/TheBATeam/BATBOX-An-Awesome-Batch-Plugin), [Button](https://github.com/TheBATeam/Button-Function-2.0-by-Kvc), [GetInput](https://github.com/TheBATeam/GetInput-By-Aacini), [Getlen](https://github.com/TheBATeam/Getlen-Function-2.0-by-Kvc)
- [QuickSFV](http://www.quicksfv.org/)
- Partial refactor: `Rewrite_Medicat_Installer.bat`
- GUI file checker: `MedicatFileChecker.ps1`
