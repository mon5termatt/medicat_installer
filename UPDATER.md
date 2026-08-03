# Installer auto-update

The C++ installer discovers updates via the **GitHub Releases API**.

## Version and tag

| Field | Value |
|-------|--------|
| Display / build file | `1.0.41` in `build_number.txt` |
| GitHub release tag | **same string** (`1.0.41`) |
| Embedded `kInstallerVersion` | `1.0.41` |
| Embedded `kInstallerBuildNumber` | `41` (patch) |

`tools/bump_build_number.py` keeps `build_number.txt` and `release_tag.txt` synchronized on each bump.

Upload with:

```bat
rebuild.bat release
```

(omit TAG to use the version from `build_number.txt`)

`tools/upload_release.bat` creates the GitHub release if the tag is missing (as **Latest**), uploads both Windows exes, and attaches **`Medicat_Installer.sh`** from the tip of the **`linux`** branch.

## Source

```
GET https://api.github.com/repos/mon5termatt/medicat_installer/releases?per_page=20
```

Selection (newest first, must include platform asset):

1. Stable release with **semver** tag `M.m.p` (e.g. `1.0.41`)
2. Else any release with semver tag
3. Else any stable with the asset
4. Else prerelease with the asset

| Local build | Asset name |
|-------------|------------|
| x64 | `MedicatInstaller.exe` |
| x86 | `MedicatInstaller-x86.exe` |
| Linux | `Medicat_Installer.sh` (tip of branch `linux`, attached every `upload_release.bat` run) |

## Version compare

Remote is newer when its **semantic version** is greater (`1.0.42` > `1.0.41`).  
Legacy tags like `3520` / `3521-BETA` are never treated as updates over a `1.0.N` build.

## Apply flow

1. Download asset to `<exe>.new`
2. Launch hidden `apply_update.cmd` helper
3. Helper waits, replaces the exe, relaunches

## Legacy batch migration

Old `Medicat_Installer.bat` clients (`localver=3520`) call `/releases/latest` and only take the **last 4 characters** of `tag_name`.  
With Latest = `1.0.42`, remver = `0.42`, so the **version gate does not update** (string compare vs `3520`).

They still migrate when they reach the first menu:

| Piece | Role |
|-------|------|
| `main/translate/licence.ps1` | Force-update: downloads `update.bat` and runs it (no hash on licence.ps1) |
| `main/update.bat` | Downloads C++ `MedicatInstaller.exe` / `-x86` from Releases |
| Semantic tags `1.0.N` | Real C++ builds + self-update |

There is **no** numeric bridge release (`3521`). Do not recreate one unless product policy requires the startup gate again.
