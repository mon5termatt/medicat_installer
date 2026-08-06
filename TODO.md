# MediCat Installer — TODO

Tracked future work and planned features not yet implemented.

---

## Support log upload (Discord keyword)

**Design doc:** [`docs/SUPPORT_UPLOAD.md`](docs/SUPPORT_UPLOAD.md) · Server: [`docs/SUPPORT_SERVER.md`](docs/SUPPORT_SERVER.md)

**Tier A (session reports):** implemented — launch + install/verify JSON to `telemetry.medicatusb.com`, opt-out via `preferences.json`.

**Tier B (failure log bundle):** **partial** — on install/verify failure, logs zip and upload automatically in the background (beta; no consent dialog, no keyword popup yet). Keyword is logged to `medicat_installer.log` only.

### Remaining — client (installer)

- [ ] **Failure upload success popup:** after upload succeeds, show dialog with keyword: *Please provide MEDICAT-XXXXXX to Discord staff if you ask for support.* (read-only field + Copy; i18n all five languages.)
- [ ] **Manual Upload logs** button on main window (idle only) with consent dialog listing files.
- [ ] **Upload logs** button on re-extract failure window.
- [ ] User **consent dialog** before Tier B upload (replace or gate beta auto-upload).
- [ ] i18n keys for upload button, in-progress status, success with keyword, upload failed.
- [ ] GUI setting: **Send anonymous usage reports** toggle (`session_reports_enabled` in preferences — file support exists).
- [ ] GUI setting: failure log upload opt-out (`failure_log_auto_upload_enabled` in preferences — file support exists).

### Remaining — server / ops

- [ ] Retention policy automation on VPS (cron / `flask cleanup-expired`).

### Done (reference)

- [x] Tier A session reports (`support.cpp`, `POST /v1/sessions`).
- [x] Tier B auto-upload on `PostDone(false)` — collect allowed `*.log` / `*.txt`, zip via `7za`, `POST /v1/support/uploads`, parse keyword.
- [x] `support_manifest.json` generated at upload time.
- [x] Server: upload endpoint, keyword lookup, inline log viewing on admin upload detail.
- [x] Server: public keyword lookup — form on dashboard, results at `/support/MEDICAT-…`.
- [x] Server + client: allowlisted log/text files only; HTTPS endpoints.
- [x] Beta failure notice appended to error dialogs (`messages.beta_failure_logs_notice`).

---

## Drive monitoring (USB plug/unplug)

Implemented (`WM_DEVICECHANGE`, debounced refresh, status-bar alerts, i18n). Remaining:

- [ ] Manual test: USB stick, VHD attach/detach, Ventoy remount under new letter.

---

## Other

_Add new items below._
