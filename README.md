# MediCat Installer (PowerShell GUI)

> Alpha preview – actively evolving. Expect rough edges.

This repository contains a modern PowerShell-based GUI installer for preparing a MediCat USB. It automates Ventoy installation/upgrade and extracts the main MediCat archive to the selected drive, with real-time logging and progress where possible.

This version was developed rapidly with assistance from an AI coding assistant to bootstrap the GUI, background job patterns, logging, and module integration. Please review changes carefully and test before production use.

## Status

- Stage: Very early alpha
- Active branch: `pwsh`
- Breaking changes likely; UX and flows may change without notice

## Highlights

- Windows Forms GUI (PowerShell 7+ compatible)
- Auto UAC elevation; preserves original working directory
- Drive picker with filtering
  - Hides `C:` by design
  - Optional hard drive visibility (checkbox)
  - Detects VHD/VHDX files and includes them
  - Defaults to `I:` in debug builds
- Ventoy integration
  - Auto-download of latest Ventoy release
  - Fresh install or non-destructive upgrade (`VTOYCLI`)
  - Optional NTFS format after install (checkbox-controlled)
- MediCat archive extraction
  - Prefers PowerShell modules (7Zip4PowerShell / 7zipWrapper / PS7Zip)
  - Progress tracking via background monitor where available
- Comprehensive logging
  - Dual output to UI and file (`medicat_download.log`)
  - Debug flag to control verbosity

## Requirements

- Windows 10/11 (x64) with admin rights
- PowerShell 7+ recommended (pwsh)
- Internet connectivity (for Ventoy download/module install)
- Sufficient disk space for MediCat extraction (24GB+ archive)

## Getting Started

1. Clone the repo and switch to the `pwsh` branch.
2. Launch the installer as Administrator:
   - Right-click `MedicatInstaller.ps1` → Run with PowerShell
   - Or from an elevated PowerShell:
     ```powershell
     pwsh -NoProfile -ExecutionPolicy Bypass -File .\MedicatInstaller.ps1
     ```
   The script auto-elevates if needed and runs from its own directory.
3. Select the target drive.
   - `C:` is always hidden
   - Toggle "Show hard drives" to include fixed disks
   - VHD/VHDX are detected and listed
4. Choose whether to format to NTFS before install (checkbox).
   - Checked: fresh Ventoy install then NTFS format
   - Unchecked: detect Ventoy; if present, perform non-destructive upgrade
5. Start installation. Watch the status, progress bar, and log output.

## Logs

- Primary log: `medicat_download.log` (created in script directory)
- All status, progress, debug messages, and message box interactions are logged

## Notes on Extraction

- Prefers PowerShell modules over `7z.exe`
- If a module is missing, the script may attempt local installation
- Progress is estimated using file system monitoring when the module doesn’t emit progress

## Known Issues (Alpha)

- Progress may not reflect exact extraction state for some modules
- Module availability and parameters differ by environment
- Ventoy CLI errors are surfaced, but edge cases may exist
- Large archive extraction can take a long time; ensure power and space are sufficient

## Roadmap

- Improve extraction progress fidelity and error messaging
- Pluggable extraction backend with graceful fallback
- Better Ventoy version/upgrade flow and validation
- Configurable defaults and persistent settings
- Localization and accessibility passes

## Contributing

- PRs welcome to the `pwsh` branch
- Please avoid committing large binaries; `.gitignore` excludes common large artifacts (Ventoy2Disk, archives, logs)
- When changing UI or behavior, keep logging and thread-safety patterns consistent (`form.Invoke`, background jobs)

## License

- This repo contains scripts to orchestrate third-party tools (Ventoy, 7-Zip modules). Review and comply with their respective licenses.



---

If you hit an issue, attach `medicat_download.log` and describe the drive type, chosen options, and any on-screen errors.
