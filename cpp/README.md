# MediCat Installer (C++)

Fresh Win32 rewrite. No PowerShell, no SevenZipSharp — just shell out to the tools that work.

## What it does

1. Pick a USB drive
2. Optionally install/upgrade Ventoy (`Ventoy2Disk.exe`)
3. Optionally format to NTFS
4. Extract `MediCat.USB.v21.12.7z` with bundled `7za.exe`
5. Show an accurate progress bar (bytes written to drive + 7za status lines)

## Layout

```
cpp/
  src/          application source
  res/          Windows manifest (admin)
  CMakeLists.txt
```

Run from the **repo root** (or any folder with `MediCat.USB.v21.12.7z`). `7za.exe` and `7z.exe` are **embedded in the installer** and extracted to `bundle\` on first run.

## Build (Windows)

```bat
cd cpp
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `cpp/build/Release/MedicatInstaller.exe`

Copy or run from repo root:

```bat
copy cpp\build\Release\MedicatInstaller.exe .
MedicatInstaller.exe
```

## Requirements

- Windows 10+
- Visual Studio 2022 (or Build Tools) with C++ desktop workload
- CMake 3.16+
- Administrator (Ventoy + format)

## Dependencies at runtime

| File | Purpose |
|------|---------|
| `bundle\7za.exe` | Auto-extracted from embedded resource |
| `bundle\7z.exe` | Auto-extracted from embedded resource |
| `MediCat.USB.v21.12.7z` | Archive (user supplies) |
| `Ventoy2Disk/Ventoy2Disk.exe` | Ventoy install (optional) |

Build embeds `7z/x64/7za.exe` (or arch-specific) and `bin/7z.exe` into the `.exe`.

## Design

- **GUI**: Win32 + Common Controls (no Qt/Electron)
- **Extract**: `CreateProcess` + pipe read on `7za` stdout; drive free-space for %
- **Threading**: worker thread posts `WM_APP` to UI thread for progress updates
- **Logging**: `medicat_installer.log` beside the exe
- **i18n**: `i18n/translations.json` → build-time codegen → `i18n::Tr()` (see `i18n/README.md`)

## Feature parity

See **[FEATURES.md](FEATURES.md)** for the restore checklist (Ventoy, file check, etc.).
