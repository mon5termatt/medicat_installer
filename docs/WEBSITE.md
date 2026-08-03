# Website update runbook (for maintainers + agents)

**Audience:** Matt editing medicatusb.com (or related pages), and any AI agent asked to “update the website links.”  
**When:** After a C++ installer release, a branch/tag rename, or any change to how users are told to download/install.  
**Repo this doc lives in:** `medicat_installer` (`main` = C++ installer source).

---

## Mental model (read this first)

| Surface | Example | User-facing? | What it is |
|---------|---------|--------------|------------|
| **C++ product version / GitHub tag** | `1.0.42` | **Yes** — show this as “current installer” | Real build. Assets: `MedicatInstaller.exe`, `MedicatInstaller-x86.exe` |
| **GitHub “Latest release”** | Currently the newest non-prerelease `1.0.N` | **OK if verified** | After the old empty `3521` bridge was removed, Latest should be a C++ release **with exes**. Still verify assets before pointing buttons at “latest” redirects. |
| **Legacy batch tag** | `3520` | Optional, archival only | Old `Medicat_Installer.bat`. Fielded clients migrate via `main/translate/licence.ps1` → `update.bat` (version gate often skips under `1.0.N` tags). |

**Rule for the website:**  
Prefer **explicit** `1.0.N` direct asset URLs for primary CTAs. Avoid:

- Tags that only exist for batch/beta history (`3520`, `3521-BETA`) as primary downloads  
- Empty or missing-asset releases  
- Claiming the batch installer is the current app  

C++ in-app update resolves via the **releases API** and **prefers semver** (`1.0.N`).

**Do not recreate** numeric empty “bridge” releases (`3521`, etc.) unless policy changes again.

---

## How to discover the URLs right now

### 1. Current C++ release (use this for the website)

Prefer the newest stable tag matching `1.0.*` **with both assets present**.

```bash
# List recent versions (agent/human)
gh release list --repo mon5termatt/medicat_installer --limit 15

# Newest semver with installers
gh api "repos/mon5termatt/medicat_installer/releases?per_page=20" \
  --jq '[.[] | select(.draft==false and .prerelease==false)
         | select([.assets[].name] | index("MedicatInstaller.exe"))
         | select(.tag_name | test("^[0-9]+\\.[0-9]+\\.[0-9]+$"))]
        | .[0] | {tag: .tag_name, url: .html_url,
            x64: (.assets[] | select(.name=="MedicatInstaller.exe") | .browser_download_url),
            x86: (.assets[] | select(.name=="MedicatInstaller-x86.exe") | .browser_download_url)}'

# Sanity: GitHub Latest
gh api repos/mon5termatt/medicat_installer/releases/latest \
  --jq '{tag: .tag_name, name: .name, assets: [.assets[].name]}'
```

**Canonical asset names (immutable):**

| Label for site | File name | Arch |
|----------------|-----------|------|
| Windows 64-bit | `MedicatInstaller.exe` | x64 |
| Windows 32-bit | `MedicatInstaller-x86.exe` | Win32 / x86 |
| Linux | `Medicat_Installer.sh` | bash installer from branch `linux` |

**Direct download pattern:**

```text
https://github.com/mon5termatt/medicat_installer/releases/download/<TAG>/MedicatInstaller.exe
https://github.com/mon5termatt/medicat_installer/releases/download/<TAG>/MedicatInstaller-x86.exe
```

**Release page pattern:**

```text
https://github.com/mon5termatt/medicat_installer/releases/tag/<TAG>
```

Example (re-query; do not hardcode forever):

```text
https://github.com/mon5termatt/medicat_installer/releases/download/1.0.42/MedicatInstaller.exe
https://github.com/mon5termatt/medicat_installer/releases/download/1.0.42/MedicatInstaller-x86.exe
https://github.com/mon5termatt/medicat_installer/releases/tag/1.0.42
```

### 2. “Latest” redirect downloads (optional)

If Latest is confirmed to be a `1.0.N` with both exes, these also work:

```text
https://github.com/mon5termatt/medicat_installer/releases/latest/download/MedicatInstaller.exe
https://github.com/mon5termatt/medicat_installer/releases/latest/download/MedicatInstaller-x86.exe
```

Prefer **pinned** `…/download/1.0.N/…` when the site shows a version number next to the button (badge and link stay in sync).

### 3. All releases overview page (secondary)

```text
https://github.com/mon5termatt/medicat_installer/releases
```

---

## Suggested website copy blocks

### Primary CTA

- **Product name:** MediCat Installer (or MediCat USB Installer)
- **Status word:** beta
- **One-line pitch:** Native Windows installer for Ventoy + MediCat USB extract + MD5 verify (GUI and CLI in one exe).

**Download buttons (minimum):**

1. **Download for 64-bit Windows** → `.../download/<1.0.N>/MedicatInstaller.exe`
2. **Download for 32-bit Windows** → `.../download/<1.0.N>/MedicatInstaller-x86.exe`
3. Optional: **Release notes** → `.../releases/tag/<1.0.N>`

Show the version string next to the buttons (e.g. **v1.0.42**).

### Requirements blurb (short)

- Windows 10/11  
- Run **as Administrator**  
- Target USB/VHD/VHDX **≥ 30 GiB** (`C:` is not a valid target)  
- Place **`MediCat.USB.v21.12.7z`** next to the installer **or** use in-app mirrors  

### What not to tell users

- Do **not** recommend `Medicat_Installer.bat` / tag `3520` as the current path (legacy only).  
- Do **not** claim one-click “full MediCat USB ready” without mentioning the `.7z`.

### Support / logs

Beta builds may send anonymous session stats and, on failure, allowlisted `.log` / `.txt` to `telemetry.medicatusb.com` with a **Diag code** for Discord. See [`SUPPORT_UPLOAD.md`](SUPPORT_UPLOAD.md), [`README.md`](../README.md).

### CLI teaser (optional)

```bat
MedicatInstaller.exe /help
MedicatInstaller.exe /list-drives
MedicatInstaller.exe /install /drive:E /yes
MedicatInstaller.exe /verify /drive:E /yes
```

Full flags: repo [`CLI.md`](../CLI.md).

---

## YOURLS / torrent (stable)

| Purpose | Target |
|---------|--------|
| Torrent short-link (e.g. `/torrentdl/`) | Keep a stable file URL — prefer `.../raw/main/download/MediCat_USB_v21.12.torrent` (present on `main`) or a fixed release asset if you relocate later |

Filename: `MediCat_USB_v21.12.torrent`. Do not remove from `main` without retargeting YOURLS.

---

## Links that must change vs stay stable

| Link purpose | Stable URL? | Notes |
|--------------|-------------|--------|
| GitHub releases index | Yes | `…/medicat_installer/releases` |
| Repo source | Yes | `…/medicat_installer` (`main` = C++) |
| **Direct exe downloads** | **No — change every release** | Tag segment = `1.0.N` |
| **Version badge / “vX.Y.Z” text** | **No — change every release** | Match download tags |
| Torrent | Mostly yes | `main/download/MediCat_USB_v21.12.torrent` |
| Old batch release | Only if “legacy” page | `…/releases/tag/3520` |
| Raw `update.bat` | Rarely show | Legacy migration only |
| Telemetry host | Yes | `https://telemetry.medicatusb.com` |

---

## Agent checklist — “update the website for a new installer”

1. **Discover** newest C++ tag with both assets (`gh api` above).  
2. **Locate** every site download `href` for the installer.  
3. **Replace** paths with:
   - x64: `…/releases/download/<newTag>/MedicatInstaller.exe`
   - x86: `…/releases/download/<newTag>/MedicatInstaller-x86.exe`
   - notes: `…/releases/tag/<newTag>`
4. **Update** displayed version string to match `<newTag>`.  
5. **Sanity HTTP check** both asset URLs (200, non-zero).  
6. **Legacy batch** pages only if needed; migration is via `licence.ps1` / `update.bat`, not a bridge release.  
7. Asset names stay **`MedicatInstaller.exe` / `MedicatInstaller-x86.exe`** until the repo renames them.

---

## Publishing a release (website timing)

```bat
rebuild.bat as 1.0.N release 1.0.N
REM or: rebuild.bat release
```

`tools/upload_release.bat` creates/uploads `1.0.N` as Latest with both exes.

Then bump website version + links (don’t publish URLs before assets exist).

Details: [`UPDATER.md`](../UPDATER.md).

---

## Branch / product facts (avoid outdated site text)

| Old text on websites | Correct now |
|----------------------|-------------|
| “cpp branch installer” | Product lives on **`main`** |
| “main is batch” | Batch is **`legacy`**; last big batch tag **`3520`** |
| “Latest is empty 3521 bridge” | **Removed** — Latest is the current C++ `1.0.N` |
| “Latest prerelease 3521-BETA” | Historical only |
| “Download Medicat_Installer.bat” | Prefer C++ exes |

---

## Known failure modes for agents

| Mistake | Result |
|---------|--------|
| Using only x64 and omitting x86 | 32-bit users abandoned |
| Badge / download version mismatch | Support chaos |
| Pointing at `3521-BETA` / `3520` as primary | Stale path |
| Deleting `main/download/*.torrent` without YOURLS retarget | Broken torrent short-links |

---

## Quick reference card

```text
Product download:
  newest 1.0.* with MedicatInstaller.exe + -x86.exe

Latest on GitHub:
  should be that same 1.0.N (no empty bridge)

Asset names:
  MedicatInstaller.exe | MedicatInstaller-x86.exe

Repo:
  https://github.com/mon5termatt/medicat_installer

After rebuild.bat release:
  bump site version + both direct links
```
