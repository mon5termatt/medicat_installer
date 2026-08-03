# Installer auto-update

The C++ installer discovers updates via the **GitHub Releases API** only (no `update.json` on the branch).

## Source

```
GET https://api.github.com/repos/mon5termatt/medicat_installer/releases?per_page=20
```

Selection rules (newest first):

1. First **non-prerelease, non-draft** release that includes the platform asset
2. Else first **prerelease** that includes the asset

| Local build | Asset name |
|-------------|------------|
| x64 | `MedicatInstaller.exe` |
| x86 | `MedicatInstaller-x86.exe` |

## Version compare

Embedded at compile time from `release_tag.txt` / `INSTALLER_RELEASE_TAG` and `kInstallerBuildNumber`.

Remote is newer when:

- Remote release tag number &gt; local tag number (e.g. `3522` vs `3521-BETA`), or
- Same numeric tag: stable (no `-`) preferred over local prerelease suffix

## Apply flow

1. Download asset to `<exe>.new`
2. Launch hidden `apply_update.cmd` helper in `%TEMP%\MedicatInstaller\{pid}\`
3. Helper waits, moves over the running exe, relaunches

Implemented in `src/update.cpp`.

## Legacy batch bridge

`update.bat` (repo root of `main`) is fetched by old `Medicat_Installer.bat` (3520) after a **newer Latest** release tag triggers its `:curver` check. It also uses the Releases API to download the C++ binary.

## Publishing

```bat
rebuild.bat release TAG
```

Uploads both exes to that tag. No manifest file is generated or committed.
