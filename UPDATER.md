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

`tools/upload_release.bat` creates the GitHub release if the tag is missing, then uploads both exes.

After every C++ upload it also **re-promotes** the fixed legacy bridge tag `3521` as GitHub **Latest** (creates it if missing). Notes on that bridge are updated to point at the C++ tag just uploaded. Do not put installer assets on `3521`.

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

## Version compare

Remote is newer when its **semantic version** is greater (`1.0.42` > `1.0.41`).  
Legacy tags like `3521-BETA` are never treated as updates over a `1.0.N` build.

## Apply flow

1. Download asset to `<exe>.new`
2. Launch hidden `apply_update.cmd` helper
3. Helper waits, replaces the exe, relaunches

## Legacy batch bridge

Old `Medicat_Installer.bat` clients (`localver=3520`) call `/releases/latest` and compare the **last 4 characters** of `tag_name` to `3520`. Tags like `1.0.42` become `0.42` and **never** trigger an update (string compare with `3520`).

| Piece | Role |
|-------|------|
| Latest tag `3521` | Forces batch clients into their update path |
| `main/update.bat` | Downloads C++ `MedicatInstaller.exe` / `-x86` from Releases |
| `main/translate/licence.ps1` | **Backup force-update** — 3520 pulls this every run (no hash). Replaces the old MIT text with download+run of `update.bat` and closes the batch host. |

If the batch ever skips the version gate (wrong Latest tag shape), it still hits `licence.ps1` at the first menu and migrates.
| Semantic tags `1.0.N` | Real C++ builds + self-update |

`upload_release.bat` keeps `3521` as Latest after each C++ release publish.
