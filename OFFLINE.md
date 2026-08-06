# Offline installer — design notes

Work file for a **single-file offline build** that embeds one pinned Ventoy release so Ventoy install/upgrade works without internet or an `offline/` folder beside the exe.

Not implemented yet — this document captures analysis and a proposed approach.

---

## Goal

Ship a variant of `MedicatInstaller.exe` that includes:

| Asset | Online build (today) | Offline build (proposed) |
|-------|----------------------|-------------------------|
| `7za.exe` (per arch) | Embedded | Embedded |
| `MedicatFiles.md5` | Embedded (gzip) | Embedded (gzip) |
| Ventoy | Downloaded on demand (~16 MB) or `offline/ventoy/*.zip` beside exe | **Single slim Ventoy zip embedded in exe** |
| MediCat `*.7z` | User-supplied or downloaded | Still user-supplied / `offline/*.7z` (not in exe) |

Internet would **not** be required for Ventoy. The MediCat archive remains separate (~21 GB).

---

## How Ventoy is used today

| Function | What it needs |
|----------|----------------|
| Install / upgrade USB | `Ventoy2Disk.exe VTOYCLI /I` or `/U` (+ `/GPT`, `/NOSB`, `/NOUSBCheck`) |
| “Is Ventoy on this drive?” | **Nothing from the Ventoy package** — installer uses `IsVentoyPhysicalDrive()` (VTOYEFI partition layout) |
| Local version check | `Ventoy2Disk/ventoy/version` after extract |
| Version picker UI | GitHub API at runtime, or embedded `ventoy_versions.txt` |
| Debug on USB | `X:\ventoy\version` on the **target drive** after install |

**Current flow** (`EnsureVentoyReady` in `ventoy.cpp`):

1. Resolve target version (latest from GitHub, pin, or offline cache).
2. Download `ventoy-{version}-windows.zip` if missing (~16 MB for v1.1.12).
3. Extract with bundled `7za` to `{exeDir}/ventoy-{version}/`, rename to `Ventoy2Disk/`.
4. Run `Ventoy2Disk.exe` with working directory `{exeDir}/Ventoy2Disk/`.

Ventoy is **not** embedded in the exe today — only a small `ventoy_versions.txt` list is bundled for the advanced version combo.

**Existing offline support (no embed):**

- `offline/ventoy/ventoy-{version}-windows.zip` beside exe
- `offline/ventoy_versions.txt`
- `tools/populate_offline.py` to fill the cache
- `CanInstallVentoyOffline()` in `offline.cpp`

---

## What to embed from Ventoy

Official Windows zip (v1.1.12 ≈ **16 MB** download) contains more than the installer needs.

### Required for `VTOYCLI` install (~13.5 MB compressed in zip)

```
Ventoy2Disk.exe
boot/boot.img
boot/core.img.xz
ventoy/version
ventoy/ventoy.disk.img.xz      # ~81% of zip — EFI partition image written to USB
ventoy/ventoy_4k.disk.img.xz   # tiny; needed for 4K-native-sector USB drives
```

`Ventoy2Disk.exe` is **x86_32** and runs on both 32- and 64-bit Windows — **one embed for x64 and x86 builds**.

Verified: minimal layout above runs `VTOYCLI` without Plugson, plugin, or altexe files.

### Safe to omit (~2.4 MB / ~15% of zip)

| Path | Purpose |
|------|---------|
| `altexe/*` | X64 / ARM / ARM64 alternate GUIs — never invoked |
| `VentoyPlugson.exe` | Plugin web UI |
| `VentoyVlnk.exe` | vlnk helper |
| `ventoy/plugson.tar.xz` | Plugson assets |
| `ventoy/languages.json` | GUI strings |
| `plugin/**` | Sample boot theme / icons |
| `FOR_X64_ARM.txt` | Readme |

### Cannot shrink much further

`ventoy/ventoy.disk.img.xz` is inherent to Ventoy; only Ventoy upstream can change that payload.

---

## Approximate exe sizes

| Build | Approx size |
|-------|-------------|
| Current (7za + gzip MD5) | ~9 MB |
| + slim Ventoy embed | **~22 MB** |
| + full Ventoy zip (no trim) | ~25 MB |

---

## Proposed build design

Use a **CMake option**, not a separate codebase fork:

```cmake
option(MEDICAT_OFFLINE_BUILD "Embed slim Ventoy zip; no Ventoy download" OFF)
set(MEDICAT_VENTOY_VERSION "1.1.12" CACHE STRING "Ventoy version to embed")
```

### Build pipeline

1. **`tools/prepare_ventoy_bundle.py`** — input: official `ventoy-*-windows.zip` (local or CI download); output: `generated/ventoy-{ver}-slim.zip` (strip unused paths).
2. **`bundle.rc.in`** — add `IDR_VENTOY_SLIM RCDATA` when `MEDICAT_OFFLINE_BUILD` is ON.
3. **`bundle.cpp`** — extract embedded zip with bundled `7za` → `{exeDir}/Ventoy2Disk/` (same on-disk layout as today).
4. **`ventoy.cpp`** — `EnsureVentoyReady()`: embedded first, then `offline/` cache, then download (online builds only).
5. **`target_compile_definitions`** — `MEDICAT_EMBEDDED_VENTOY_VERSION="1.1.12"` for version checks and UI.

### Output naming (suggestion)

| Artifact | Behavior |
|----------|----------|
| `MedicatInstaller.exe` | Online — current behavior |
| `MedicatInstaller-Offline.exe` | Embedded Ventoy, no Ventoy network |

Or: `rebuild.bat offline` builds the offline flavor alongside the default.

---

## Runtime behavior (offline flavor)

```
User clicks Install
    → MEDICAT_OFFLINE_BUILD?
        yes → extract embedded slim zip → Ventoy2Disk/
        no  → cache / download (EnsureVentoyReady as today)
    → Ventoy2Disk.exe VTOYCLI /I|/U
    → extract MediCat .7z, verify, etc.
```

**UI / logic changes for offline build:**

- Hide or disable **Pin Ventoy version** — only one version exists.
- Skip `FetchVentoyVersions` / GitHub on startup (or show embedded version only in debug/credits).
- `CanInstallVentoyOffline()` → always true for Ventoy when embedded.

**Unchanged:**

- Install flow: format → Ventoy → extract → verify.
- `Ventoy2Disk/` beside exe after first use (disk cache for subsequent runs).
- Drive detection via VTOYEFI layout (not Ventoy package files on USB).

---

## MediCat archive (still external)

Offline Ventoy ≠ full USB image in one file.

The installer already resolves:

- `MediCat.USB.v*.7z` beside exe
- `offline/MediCat.USB.v*.7z` via `ResolveOfflineArchivePath()`

For a fully air-gapped **workflow**, users still place the `.7z` in `offline/` or next to the exe. Embedding the archive in the exe is out of scope (size).

---

## Tradeoffs

| Topic | Note |
|-------|------|
| Ventoy updates | Rebuild offline exe to bump version; no “always latest” without network |
| Exe size | +~13 MB per release (acceptable for a dedicated offline tool) |
| Licensing | Ventoy is GPL — keep credits / project link in UI |
| Release artifacts | May ship **online** + **offline** exes on GitHub releases |
| Two build matrices | x64 + x86 offline = 2 exes, same embedded Ventoy blob in each |

---

## Implementation checklist (when ready)

- [ ] `tools/prepare_ventoy_bundle.py` — slim repack from official zip
- [ ] `CMakeLists.txt` — `MEDICAT_OFFLINE_BUILD`, custom command, compile definitions
- [ ] `res/resource.h` + `bundle.rc.in` — `IDR_VENTOY_SLIM`
- [ ] `bundle.cpp` — extract embedded Ventoy zip to disk
- [ ] `ventoy.cpp` — embedded-first path in `EnsureVentoyReady`; skip download when offline build
- [ ] `gui.cpp` — lock/hide Ventoy version UI when `MEDICAT_OFFLINE_BUILD`
- [ ] `rebuild.bat` — `offline` argument for offline flavor
- [ ] `tools/upload_release.bat` — optional upload of `MedicatInstaller-Offline.exe`
- [ ] README / FEATURES — document offline artifact vs `offline/` folder layout

---

## Related files (current)

| Path | Role |
|------|------|
| `src/ventoy.cpp` | Download, extract, `VTOYCLI`, detection |
| `src/offline.cpp` | `offline/` zip cache beside exe |
| `src/bundle.cpp` | Embedded `7za` + MD5 |
| `tools/populate_offline.py` | Populate `offline/ventoy/` without embedding |
| `res/ventoy_versions.txt` | Fallback version list for UI |
