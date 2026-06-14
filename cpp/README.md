# MediCat Installer (C++)

Native Win32 rewrite on the `cpp` branch. User-facing docs: **[../README.md](../README.md)**.

## Build

```bat
cd cpp
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build/Release/MedicatInstaller.exe`

Run from repo root with `MediCat.USB.v21.12.7z` present. `7za.exe`, `bin/7z.exe`, and `MedicatFiles.md5` are embedded in the exe and extracted to `%TEMP%\MedicatInstaller\{pid}\` on first run.

## Source layout

```
src/     application (gui, ventoy, extract, verify, debug, i18n, theme)
res/     manifest, icon, bundle.rc.in, ventoy_versions.txt
```

## Design notes

- **GUI**: Win32 + GDI+ dark theme; worker threads post `WM_MEDICAT_PROGRESS` / `WM_MEDICAT_DONE`
- **Extract**: `7za` subprocess + pipe tee to `extract.log`; progress from stdout + drive free space
- **Verify**: parallel MD5 workers, `check.log`, optional file-log UI
- **Errors**: auto `debug.log` with system/tool diagnostics

Feature parity vs PowerShell installer: **[FEATURES.md](FEATURES.md)**
