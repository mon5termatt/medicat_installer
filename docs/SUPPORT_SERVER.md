# Support server — what gets uploaded

The telemetry backend runs at **`telemetry.medicatusb.com`**. Implementation and deployment live in the private repo **[medicat-support-server](https://github.com/mon5termatt/medicat-support-server)**.

This doc describes **what data leaves the installer** and **what the server stores**. Client behavior, consent, and opt-out are in [`SUPPORT_UPLOAD.md`](SUPPORT_UPLOAD.md).

---

## Two tiers

| Tier | When | User prompt? | Payload |
|------|------|--------------|---------|
| **A — Session report** | End of every install/verify (if enabled) | No | Small JSON (~1 KB) |
| **B — Failure bundle** | Install/verify failure only | Yes | Zip of allowlisted log files |

Tier A has **no log files**. Tier B may contain **paths and usernames inside log text** — only sent with explicit consent.

---

## Tier A — Session report (`POST /v1/sessions`)

Automatic JSON at session end. Fire-and-forget; server responds `204 No Content`.

### Required fields

| Field | Type | Example |
|-------|------|---------|
| `session_id` | string (UUID) | `7c9e6679-7425-40de-944b-e07fc1f90ae7` |
| `client` | string | `MedicatInstaller` |
| `operation` | string | `install`, `verify` |
| `outcome` | string | `success`, `verify_failed`, `cancelled`, … |

### Optional fields (stored if present)

| Field | Type | Notes |
|-------|------|-------|
| `exit_code` | int | CLI/GUI exit code |
| `installer_version` | string | e.g. `1.0.6` |
| `installer_build` | int | Build number |
| `installer_arch` | string | `x64`, `x86` |
| `system` | object | See below |

`system` object (all optional):

| Field | Type | Example |
|-------|------|---------|
| `windows_build` | int | `26100` |

Additional keys in the JSON body are kept in the stored payload but are not required by the server today. Planned client fields (see [`SUPPORT_UPLOAD.md`](SUPPORT_UPLOAD.md)) include `medicat_usb_version`, `release_tag`, `duration_ms`, `locale`, `elevated`, and an `options` object — the server accepts and stores the full JSON as-is.

### Not included in Tier A (by design)

- Usernames, computer names
- Drive letters or USB paths
- Failure message text
- Log file contents
- IP address (client does not send IP)

### Server-side additions (not from installer)

| Field | Purpose |
|-------|---------|
| `created_at` | UTC timestamp when received |
| `client_ip_hash` | SHA256(IP + salt) for abuse/rate limits only |

Retention: **90 days** (configurable on server).

---

## Tier B — Failure bundle (`POST /v1/support/uploads`)

Multipart upload after user confirms on a failure dialog. Server responds `201` with a support keyword.

### Form fields

| Part | Required | Description |
|------|----------|-------------|
| `bundle` | yes | Zip file |
| `session_id` | no | Links to Tier A row if present |
| `manifest` | no | JSON copy for indexing (optional duplicate of manifest in zip) |

### Allowed files inside the zip

Basename allowlist (plus any file ending in `.log`, `.txt`, or `.json`):

| File | Typical source |
|------|----------------|
| `medicat_installer.log` | Every session |
| `extract.log` | After full extract |
| `reextract.log` | After selective re-extract |
| `check.log` | After verify |
| `failed_files.txt` | Verify failures |
| `support_manifest.json` | Generated at upload time |
| `support_manifest.txt` | Alternative manifest format |

**Rejected:** executables, archives, `.7z`, path traversal entries, zip bombs.

**Limits:** 10 MB zip, 25 MB uncompressed total, 20 files max.

### What log files may contain

Plain-text diagnostics beside the installer. May include:

- Windows username or profile paths (in log lines)
- Drive letters and volume paths
- Installer options and error details
- Ventoy / 7za command output

`support_manifest.json` (Tier B only) may add structured context such as drive letter and checkbox state — **not** sent in Tier A.

### Server response

```json
{
  "upload_id": "550e8400-e29b-41d4-a716-446655440000",
  "keyword": "MEDICAT-A7X9K2",
  "expires_at": "2026-07-18T04:12:00Z",
  "retention_days": 30
}
```

User shares **`keyword`** in Discord for staff lookup.

### Server-side storage

| Stored | Notes |
|--------|-------|
| Zip blob | `{upload_id}.zip` on disk |
| `keyword` | Staff lookup (`MEDICAT-XXXXXX`) |
| `session_id` | Optional link to Tier A |
| `manifest_json` | Parsed manifest if provided |
| `file_count`, `size_bytes` | Metadata |
| `client_ip_hash` | Abuse tracking (hashed IP) |
| `expires_at` | **30-day** TTL; deleted by cleanup job |

Staff can view log text inline on the upload detail page (first 256 KB per file) or download the zip via the admin dashboard. Anyone with the keyword can look it up from the public dashboard form and view logs on `/support/MEDICAT-…`.

---

## Rate limits

| Endpoint | Limit |
|----------|-------|
| `POST /v1/sessions` | 60 per hour per IP |
| `POST /v1/support/uploads` | 5 per hour per IP |

Lifetime upload count per IP is logged as a warning after **20** total uploads (not blocked).

---

## Public vs staff views

| Audience | URL | Data shown |
|----------|-----|------------|
| Public | `/` | Aggregate stats with inline support keyword form (`#support`) |
| Public | `/support/<keyword>` | Support log bundle detail (after lookup) |
| Staff | `/admin` | Session list, upload lookup by keyword, zip download |

---

## References

- Client design & consent: [`SUPPORT_UPLOAD.md`](SUPPORT_UPLOAD.md)
- Installer tasks: [`TODO.md`](../TODO.md)
- Server ops (Docker, env): [medicat-support-server README](https://github.com/mon5termatt/medicat-support-server)
