# MediCat Installer — TODO

Tracked future work and planned features not yet implemented.

---

## Support log upload (Discord keyword)

**Goal:** Let users upload diagnostic **log files only** to a server after install/verify failures. Staff look up the upload in Discord using a short **support keyword** the user provides.

**Upload policy:** Only plain-text diagnostics beside the installer — `*.log` and `*.txt`. No archives, executables, or user media. If extra context is needed (installer version, drive letter, checkbox state), generate a small `support_manifest.txt` at upload time rather than uploading non-log artifacts.

### User flow (planned)

1. Install or verify fails (especially after re-extract still fails).
2. User clicks **Upload logs for support** (re-extract failure window and/or main UI).
3. Installer collects allowed log files, zips them, uploads to server, shows a **keyword** (e.g. `MEDICAT-A7X9K2`).
4. User posts that keyword in Discord; staff retrieves the bundle server-side.

### Allowed files (beside exe)

| File | Type | When |
|------|------|------|
| `medicat_installer.log` | `.log` | Every session (includes diagnostic sections) |
| `extract.log` | `.log` | Full 7za output during extract |
| `reextract.log` | `.log` | Selective re-extract (`7za @list`) |
| `check.log` | `.log` | Per-file MD5 verify results |
| `failed_files.txt` | `.txt` | Verification failures list |
| `support_manifest.txt` | `.txt` | **Generated at upload** — version, drive, options, OS summary |

Skip any path that is missing. Reject anything that is not `.log` or `.txt`.

### Client (installer) tasks

- [ ] Define upload API contract (endpoint, auth, max size, response JSON with keyword).
- [ ] `CollectSupportLogs()` — enumerate only `*.log` / `*.txt` in installer directory (+ generated `support_manifest.txt`); skip missing files; zip with bundled `7z` or miniz.
- [ ] `UploadSupportLogs()` — WinHTTP POST multipart or pre-signed URL flow; progress in status bar.
- [ ] Generate or receive **support keyword** from server response; display copy-friendly UI (read-only field + Copy button).
- [ ] Wire into **verification still failed after re-extract** dialog (primary entry point).
- [ ] Optional: **Upload logs** on generic failure (`PostDone(false, …)`).
- [ ] i18n keys: upload button, in-progress status, success with keyword, upload failed, privacy note.
- [ ] Handle offline / upload errors without blocking dismiss.
- [ ] Pre-upload validation: refuse bundle if zero eligible files; never include `MediCat.USB*.7z` or other binaries.

### Server / staff tasks

- [ ] HTTP endpoint to accept log archive (rate limit, size cap).
- [ ] Store bundle with unique id; return **keyword** to client.
- [ ] Staff lookup by keyword (web UI, bot command, or admin panel).
- [ ] Retention policy (e.g. 30 days) and PII review (paths may contain usernames).

### Security & privacy

- [ ] Upload scope limited to log/text diagnostics only (no `.7z`, `.exe`, etc.).
- [ ] HTTPS only; user consent dialog before upload.
- [ ] Keyword entropy high enough to prevent guessing (e.g. 6+ alphanumeric).

### UI hooks already in place

- Re-extract failure message mentions future upload (`messages.verify_still_failed_after_reextract`).
- Re-extract prompt window (`Gui::OpenReExtractPrompt`) — add **Upload logs** button there when implemented.

---

## Drive monitoring (USB plug/unplug)

**Goal:** Detect when USB drives are connected or removed and refresh the drive list automatically, with a clear alert when a new removable drive appears so the user does not need to click **Refresh Drives**.

### Planned behavior

- Register for device change notifications (`WM_DEVICECHANGE` / `DBT_DEVICEARRIVAL` / `DBT_DEVICEREMOVECOMPLETE`) on the main window or a hidden notification window.
- Debounce rapid plug/unplug events before repopulating the combo (avoid UI flicker).
- On **arrival** of a relevant USB/removable volume: refresh drive list, optionally select the new drive if it is eligible (≥ 30 GiB, not `C:`), and show a short status-bar or toast-style message (e.g. “USB drive detected: E:”).
- On **removal**: refresh list; if the selected drive disappeared, clear selection and warn before install/verify.
- Respect **Show all drives** — same filtering rules as manual refresh (`drives.cpp`).
- Do not refresh while a long operation is in progress (`SetBusy`) unless the removed drive is the active target (then block or prompt).

### Tasks

- [x] `WM_DEVICECHANGE` handler in `gui.cpp` (or dedicated `drives_monitor.cpp`).
- [x] Debounced `RefreshDrives()` call from UI thread (post message from notification handler).
- [x] User-visible alert on new eligible USB (status bar + optional balloon/status text).
- [x] Handle selected drive vanishing mid-session (disable install, clear combo selection).
- [x] i18n keys for plug-in / unplug messages.
- [ ] Manual test: USB stick, VHD attach/detach, Ventoy remount under new letter.

---

## Other (add items below)

- [ ] _(none yet)_
