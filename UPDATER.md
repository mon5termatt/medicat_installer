# Automatic installer updater — design draft

Work file for checking whether a newer `MedicatInstaller.exe` is available (likely via GitHub Releases) and prompting or performing an update.

Not implemented yet.

---

## Goal

On startup (or on demand), the installer should:

1. Know **what it is running** (version, build, arch, release channel).
2. Ask a **trusted remote source** what the latest build is.
3. If newer → notify the user and offer **download / open release page / dismiss**.
4. Avoid interrupting install/verify work; fail quietly when offline.

Non-goals for v1:

- Silent background auto-replace while the app is running (Windows file locks).
- Updating MediCat `.7z`, Ventoy, or USB contents — **installer exe only**.

---

## Version identity today (problem to solve)

Two parallel numbering schemes exist:

| Source | Example | Where set |
|--------|---------|-----------|
| **Installer version** | `1.0.11` | `kInstallerVersion` in `generated/build_version.cpp` |
| **Build counter** | `11` | `kInstallerBuildNumber` / `build_number.txt` (auto-increment per build) |
| **GitHub release tag** | `3521-BETA` | `release_tag.txt`, `gh release upload` |

The UI shows `v1.0.11` (from `kInstallerVersion`). Releases are tagged `3520`, `3521-BETA`, etc. **These do not map 1:1** without extra metadata.

**Recommendation:** embed at compile time (CMake → `build_version.cpp` or compile defs):

```cpp
extern const char kInstallerReleaseTag[];   // e.g. "3521-BETA"
extern const char kInstallerChannel[];      // "prerelease" | "stable"
extern const char kInstallerRepo[];         // "mon5termatt/medicat_installer"
```

Publish the same fields in a small **update manifest** on each release so the client can compare apples to apples.

---

## Update source options

### Option A — GitHub Releases API (recommended)

Already used for Ventoy version fetch (`download.cpp` / `ventoy.cpp`). Same WinHTTP stack.

**Latest prerelease (matches current shipping model):**

```
GET https://api.github.com/repos/mon5termatt/medicat_installer/releases
    ?per_page=5
```

Filter `prerelease == true`, take newest by `published_at` (or first entry if API order is newest-first).

**Or pinned tag channel:**

```
GET https://api.github.com/repos/mon5termatt/medicat_installer/releases/tags/3521-BETA
```

**Assets to match:**

| Local arch | Asset name |
|------------|------------|
| x64 | `MedicatInstaller.exe` |
| x86 | `MedicatInstaller-x86.exe` |

Parse `tag_name`, `published_at`, `html_url`, and `assets[].browser_download_url` + `assets[].size`.

**Pros:** No extra hosting; matches what `tools/upload_release.bat` already publishes.  
**Cons:** Must parse JSON in C++ (small hand-rolled parser like Ventoy tag parsing, or minimal JSON subset); rate limits without token (usually fine for client check).

### Option B — Static manifest in repo

Commit or generate `installer/update.json` on each release:

```json
{
  "channel": "prerelease",
  "release_tag": "3521-BETA",
  "version": "1.0.11",
  "build": 11,
  "published": "2026-06-16T01:35:54Z",
  "assets": {
    "x64": {
      "name": "MedicatInstaller.exe",
      "url": "https://github.com/.../releases/download/3521-BETA/MedicatInstaller.exe",
      "sha256": "..."
    },
    "x86": { ... }
  },
  "release_notes_url": "https://github.com/.../releases/tag/3521-BETA"
}
```

Fetch:

```
GET https://raw.githubusercontent.com/mon5termatt/medicat_installer/cpp/installer/update.json
```

(branch = `cpp` for active C++ line)

**Pros:** Simple compare (`build` integer); room for SHA256; works even if release asset names change.  
**Cons:** Extra release step to refresh manifest; must keep in sync with `gh release upload`.

### Option C — Hybrid (best long-term)

- **Compare** using manifest `build` (or semver).
- **Download URL** from manifest or from Releases API asset list.
- **Verify** using manifest `sha256` after download.

---

## When to check

| Trigger | Default | Notes |
|---------|---------|-------|
| App startup | Once per session, ~2s after UI shown | Debounced; skip if busy |
| Manual menu | “Check for updates…” | Footer or Help |
| After failed op | No | Don’t pile on |

Skip check when:

- No internet (`TestInternetConnection` — same as Ventoy path).
- User dismissed “skip this version” for current `release_tag` (registry or `%AppData%` json).
- `MEDICAT_OFFLINE_BUILD` / enterprise policy flag (future).

---

## Comparison logic

Proposed primary key: **`kInstallerBuildNumber`** vs manifest `build`.

Fallback: parse semver `major.minor.build` from `kInstallerVersion`.

Release tag compare is **string/channel specific** — use only for display (“Update 3521-BETA available”), not ordering, unless tags become strictly numeric.

```text
if (remote.build > local.build) → update available
if (remote.build == local.build && remote.release_tag != local.release_tag) → log warning (misbuild); no forced update
if (remote.build < local.build) → local is newer (dev build); no prompt
```

**Channel:** prerelease client only offers prerelease updates unless user opts into stable (`3520` line).

---

## User experience (draft)

### Update available

Non-blocking dialog or status-bar entry:

- Title: “Update available”
- Body: “A newer installer is available (v1.0.12 / 3522-BETA). You have v1.0.11 / 3521-BETA.”
- Buttons:
  - **Download** — fetch to `{exeDir}\MedicatInstaller.new.exe` (or arch-specific name)
  - **Open release page** — `ShellExecute` on `html_url`
  - **Later** — dismiss for session
  - **Skip this version** — don’t ask again until a newer remote build exists

### Download in progress

- Reuse download progress UI (`SetDownloadProgress` / status bar) — same patterns as archive mirrors.
- Show bytes / speed; log to `medicat_installer.log`.

### After download

Windows cannot overwrite the running exe. v1 options:

1. **Relaunch helper (simplest)** — write `update.bat` beside exe:
   ```bat
   @echo off
   timeout /t 2 /nobreak >nul
   move /y "MedicatInstaller.new.exe" "MedicatInstaller.exe"
   start "" "MedicatInstaller.exe"
   del "%~f0"
   ```
   Exit current app → user runs batch, or app spawns batch then `ExitProcess`.

2. **Open folder + instructions** — download complete, user closes and replaces manually.

3. **Separate tiny updater exe** — future; more polish.

Recommend **v1 = download + Open release page**; optional **v2 = batch relaunch**.

### Integrity

- If manifest includes `sha256`, verify after download before offering replace.
- HTTPS only; pin host `api.github.com` / `github.com` / `raw.githubusercontent.com`.
- Code signing: note in UI if new exe is unsigned (same as today).

---

## Architecture (proposed modules)

```text
src/update.h / update.cpp
  UpdateCheckResult CheckForInstallerUpdate(const UpdateCheckOptions&);
  UpdateDownloadResult DownloadInstallerUpdate(const std::wstring& url, const std::wstring& dest);
  void OpenReleasePage(const std::wstring& url);
  bool ShouldSkipUpdatePrompt(const std::wstring& releaseTag);
  void RememberSkippedVersion(const std::wstring& releaseTag);
```

```text
Gui / App
  OnStartup → PostMessage deferred check (worker thread)
  Worker → CheckForInstallerUpdate → PostMessage WM_MEDICAT_UPDATE_RESULT
  Gui → ShowUpdateDialog(payload)
```

Threading: same rule as drive refresh — **WinHTTP on worker**, UI on main thread via `WM_MEDICAT_*`.

---

## Release pipeline changes

When cutting a release:

1. `rebuild.bat` / CI produces x64 + x86 exes.
2. `tools/upload_release.bat 3522-BETA` uploads assets.
3. **New:** `tools/publish_update_manifest.py` writes `installer/update.json` with build number, tag, SHA256 of both exes.
4. Commit manifest to `cpp` branch (or attach as release asset `update.json`).

Embed in exe at build time:

```text
INSTALLER_RELEASE_TAG=3521-BETA
INSTALLER_CHANNEL=prerelease
```

so logs/debug show the same tag users see on GitHub.

---

## Settings / persistence

Store beside user profile (not beside exe — may be read-only):

```text
%AppData%\MedicatInstaller\settings.json
```

```json
{
  "skipped_release_tags": ["3521-BETA"],
  "last_update_check_utc": "2026-06-17T12:00:00Z",
  "check_on_startup": true,
  "channel": "prerelease"
}
```

Optional: “Check for updates automatically” checkbox in UI (default on).

---

## i18n keys (placeholder list)

- `update.checking` — “Checking for updates…”
- `update.available_title` / `update.available_message` — with `{0}` local, `{1}` remote version
- `update.download` / `update.open_page` / `update.later` / `update.skip_version`
- `update.downloading` / `update.download_complete` / `update.download_failed`
- `update.up_to_date` — optional toast when manual check finds nothing
- `update.offline` — silent skip; only for manual check

All five languages in `translations.json` when implemented.

---

## Security & privacy

| Topic | Approach |
|-------|----------|
| Transport | HTTPS only (WinHTTP) |
| User-Agent | `MedicatInstaller/{version}` (already used) |
| Telemetry | No extra analytics; log check result locally only |
| Token | No GitHub token in client; anonymous API is enough for public repo |
| Supply chain | SHA256 from signed manifest; warn if hash mismatch |
| Elevation | Updater download does not need admin; replace may need user to close installer |

---

## Edge cases

| Case | Behavior |
|------|----------|
| Offline | Skip silently on startup; message on manual check |
| Rate limit (403/429) | Log debug; no user error on startup |
| Running from zip / temp | Download to same dir as exe; warn if not writable |
| Portable copy on USB | Same — write `*.new.exe` next to current |
| Dev build (`build` not in manifest) | Treat as “unknown”; optional “you may be on a dev build” |
| Two arch on same folder | Each binary only offers its own arch asset |

---

## Implementation checklist

- [ ] Decide version source: **manifest `build`** + embedded `release_tag`
- [ ] CMake: embed `INSTALLER_RELEASE_TAG`, `INSTALLER_CHANNEL`
- [ ] `tools/publish_update_manifest.py` + release docs
- [ ] `src/update.cpp` — GitHub API or raw manifest fetch + compare
- [ ] `WM_MEDICAT_UPDATE_RESULT` + startup timer in `Gui`
- [ ] Update dialog (Win32 `MessageBox` v1 or small custom dialog)
- [ ] Download to `MedicatInstaller.new.exe` with progress
- [ ] Optional `update.bat` relaunch flow
- [ ] Skip-version persistence in `%AppData%`
- [ ] i18n keys (en, es, fr, pl, tr)
- [ ] Manual “Check for updates” entry (footer button or credits window)
- [ ] Log lines in `medicat_installer.log` / `debug.log` on failure

---

## Related files (today)

| Path | Role |
|------|------|
| `generated/build_version.cpp` | `kInstallerVersion`, `kInstallerBuildNumber` |
| `tools/bump_build_number.py` | Per-build increment |
| `release_tag.txt` | Human tag for `upload_release.bat` |
| `tools/upload_release.bat` | Publishes `MedicatInstaller.exe` + `-x86` |
| `src/download.cpp` | WinHTTP GET / download — reuse for update fetch |
| `src/app.cpp` | `TestInternetConnection` before network work |

---

## Open questions

1. **Prerelease only** vs offer stable `3520` to users who want batch-era builds?
2. **Auto-download** without prompt, or always confirm?
3. **Code signing** — block replace if unsigned/new cert different?
4. Should update check run **before** or **after** UAC elevation (installer already requires admin)?
5. Single **offline** exe flavor — skip update check entirely when `MEDICAT_OFFLINE_BUILD`?

---

## See also

- [OFFLINE.md](OFFLINE.md) — offline exe with embedded Ventoy (separate from installer self-update)
- [TODO.md](TODO.md) — support log upload, drive monitoring
