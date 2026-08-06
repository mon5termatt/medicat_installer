# MediCat telemetry API

Base URL: `https://telemetry.medicatusb.com` (production) or `http://127.0.0.1:8000` (local Docker).

All JSON API routes live under `/v1`. The health check is at `/health` (no auth).

---

## Authentication

Two credentials are used for different purposes:

| Credential | Env var | Used for |
|------------|---------|----------|
| Ingest token | `INGEST_TOKEN` | Installer writes (POST) |
| App API key | `APP_API_KEY` | Read API (GET) |

### Ingest (installer)

```http
Authorization: Bearer <INGEST_TOKEN>
```

Bearer token only.

### Read API (tools / Cursor)

Either header works:

```http
Authorization: Bearer <APP_API_KEY>
```

```http
X-API-Key: <APP_API_KEY>
```

### Common errors

| Status | Body | Meaning |
|--------|------|---------|
| 401 | `{"error": "unauthorized"}` | Missing or wrong credential |
| 503 | `{"error": "ingest_not_configured"}` | `INGEST_TOKEN` not set |
| 503 | `{"error": "api_not_configured"}` | `APP_API_KEY` not set |

---

## Health

```http
GET /health
```

**Response `200`**

```json
{"status": "ok"}
```

---

## Ingest — session reports

Tier A telemetry from the installer.

```http
POST /v1/sessions
Authorization: Bearer <INGEST_TOKEN>
Content-Type: application/json
```

Rate limit: **60 requests per hour** per client IP.

### Request body

Required fields:

| Field | Type | Notes |
|-------|------|-------|
| `session_id` | string | UUID, max 36 chars |
| `client` | string | e.g. `MedicatInstaller` |
| `operation` | string | e.g. `launch`, `install`, `verify` |
| `outcome` | string | e.g. `opened`, `success`, `install_failed` |

Optional fields:

| Field | Type | Notes |
|-------|------|-------|
| `exit_code` | integer | |
| `installer_version` | string | |
| `installer_build` | integer | |
| `installer_arch` | string | e.g. `x64` |
| `system` | object | `windows_build`, `windows_ubr`, `machine_id_hash` |
| `error` | object | `title`, `detail` (max 512 chars each) |

### Example

```json
{
  "session_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "client": "MedicatInstaller",
  "operation": "install",
  "outcome": "install_failed",
  "exit_code": 1,
  "installer_version": "1.0.20",
  "installer_build": 20,
  "installer_arch": "x64",
  "system": {
    "windows_build": 26100,
    "windows_ubr": 4349
  },
  "error": {
    "title": "Install failed",
    "detail": "Ventoy returned exit code 1."
  }
}
```

### Responses

| Status | Body |
|--------|------|
| 204 | *(empty)* — accepted |
| 400 | `{"error": "expected_json"}` |
| 400 | `{"error": "invalid_payload", "message": "..."}` |
| 401 | `{"error": "unauthorized"}` |

---

## Ingest — support log upload

Tier B failure log bundle (zip).

```http
POST /v1/support/uploads
Authorization: Bearer <INGEST_TOKEN>
Content-Type: multipart/form-data
```

### Form fields

| Field | Required | Notes |
|-------|----------|-------|
| `bundle` | yes | Zip file |
| `session_id` | no | Links upload to a session |
| `manifest` | no | JSON string; merged with manifest inside zip if present |

Zip limits (defaults): 10 MB compressed, 25 MB uncompressed, 20 files.

### Response `201`

```json
{
  "upload_id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "keyword": "MEDICAT-A7X9K2",
  "expires_at": "2026-07-30T12:00:00+00:00",
  "retention_days": 30,
  "merged": false
}
```

### Errors

| Status | Body |
|--------|------|
| 400 | `{"error": "missing_bundle"}` |
| 400 | `{"error": "invalid_manifest"}` |
| 400 | `{"error": "<validation code>", "message": "..."}` |
| 413 | `{"error": "payload_too_large", "message": "..."}` |
| 500 | `{"error": "storage_error"}` |

### Support keywords

Format: `MEDICAT-` + 6–8 characters from `ABCDEFGHJKLMNPQRSTUVWXYZ23456789`.

Example: `MEDICAT-A7X9K2`

Public support page: `https://telemetry.medicatusb.com/support/<keyword>`

---

## Read — list session reports

```http
GET /v1/sessions
Authorization: Bearer <APP_API_KEY>
```

Returns individual session **reports** (one row per ingest event), newest first by default.

### Query parameters

| Param | Default | Notes |
|-------|---------|-------|
| `page` | `1` | |
| `per_page` | `50` | `25`, `50`, or `100` |
| `sort` | `created_at` | `created_at`, `operation`, `outcome`, `installer_build`, `installer_arch`, `client_ip` |
| `dir` | `desc` | `asc` or `desc` |
| `days` | *(none)* | Only reports within the last N days |
| `ip` | *(none)* | Exact IP if valid; otherwise prefix match |
| `error_text` | *(none)* | Search error title, detail, or outcome |
| `error` | *(none)* | Broad log/outcome search (install/verify operations only) |
| `outcomes` | *(admin default)* | Comma-separated outcome keys; empty = all outcomes |

**Default outcome filter** (when `outcomes` is omitted): all outcomes except `opened`.

**Outcome keys:** `opened`, `success`, `cancelled`, `install_failed`, `reextract_failed`, `verify_failed`, `verify_failed_after_reextract`, `verify_error`, `verify_wrong_drive`

### Example

```http
GET /v1/sessions?days=7&outcomes=install_failed,success&per_page=25
```

### Response `200`

```json
{
  "page": 1,
  "per_page": 25,
  "total": 142,
  "pages": 6,
  "has_next": true,
  "has_prev": false,
  "items": [
    {
      "id": 501,
      "session_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
      "created_at": "2026-06-29T18:42:11+00:00",
      "operation": "install",
      "outcome": "install_failed",
      "exit_code": 1,
      "installer_version": "1.0.20",
      "installer_build": 20,
      "installer_arch": "x64",
      "windows_build": "26100.4349",
      "client_ip": "203.0.113.50",
      "machine_hash": "a1b2c3…",
      "error_title": "Install failed",
      "error_detail": "Ventoy returned exit code 1."
    }
  ]
}
```

List responses truncate `error_detail` to 800 characters. Detail endpoints return the full text.

---

## Read — list uploads

```http
GET /v1/uploads
Authorization: Bearer <APP_API_KEY>
```

### Query parameters

| Param | Default | Notes |
|-------|---------|-------|
| `page` | `1` | |
| `per_page` | `50` | `25`, `50`, or `100` |
| `sort` | `created_at` | `created_at`, `expires_at`, `size_bytes` |
| `dir` | `desc` | `asc` or `desc` |
| `days` | *(none)* | Only uploads within the last N days |
| `ip` | *(none)* | Client IP filter |
| `session_id` | *(none)* | UUID |

### Response `200`

Same pagination envelope as `/v1/sessions`. Each item:

```json
{
  "upload_id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "keyword": "MEDICAT-A7X9K2",
  "session_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "created_at": "2026-06-29T18:43:00+00:00",
  "expires_at": "2026-07-29T18:43:00+00:00",
  "size_bytes": 4096,
  "file_count": 2,
  "files_included": ["medicat_installer.log"],
  "error_title": "Install failed",
  "error_detail": "…",
  "support_url": "https://telemetry.medicatusb.com/support/MEDICAT-A7X9K2"
}
```

---

## Read — session detail

```http
GET /v1/sessions/<session_id>
Authorization: Bearer <APP_API_KEY>
```

All reports for one session, plus any linked upload.

### Response `200`

```json
{
  "session_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "reports": [ /* session report objects, full error_detail */ ],
  "upload": { /* upload object, or null */ }
}
```

### Response `404`

```json
{"error": "not_found"}
```

---

## Read — upload by keyword

```http
GET /v1/support/<keyword>
Authorization: Bearer <APP_API_KEY>
```

Looks up by support keyword (e.g. `MEDICAT-A7X9K2`). Case-insensitive.

### Response `200`

```json
{
  "upload": { /* upload object */ },
  "session_id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "reports": [ /* session reports for linked session_id */ ]
}
```

### Errors

| Status | Body |
|--------|------|
| 400 | `{"error": "invalid_keyword"}` |
| 404 | `{"error": "not_found"}` |

---

## Read — dashboard stats

```http
GET /v1/stats
Authorization: Bearer <APP_API_KEY>
```

Aggregate metrics (same data as the admin dashboard).

### Response `200`

```json
{
  "launches_24h": 12,
  "launches_7d": 84,
  "launches_30d": 310,
  "operation_reports_7d": 45,
  "success_rate_7d": 91.1,
  "outcomes_7d": [["success", 30], ["install_failed", 5]],
  "builds_30d": [{"build": 20, "total": 40, "success_rate": 95.0}],
  "windows_builds_30d": [[26100, 120]],
  "arch_split_7d": [["x64", 80]],
  "uploads_7d": {"count": 8, "avg_size_bytes": 6144},
  "high_volume_upload_ips": [["203.0.113.50", 22]],
  "recent_failures": [ /* up to 20 session report objects, detail truncated to 400 chars */ ]
}
```

---

## Read — failure insights

```http
GET /v1/insights?days=90&ip=203.0.113.50
Authorization: Bearer <APP_API_KEY>
```

Aggregated failure signals from uploaded log bundles.

### Query parameters

| Param | Default | Notes |
|-------|---------|-------|
| `days` | `90` | `7`, `30`, `90`, or `365` |
| `ip` | *(none)* | Filter uploads by client IP |

### Response `200`

```json
{
  "upload_count": 15,
  "top_missing_files": [["path/to/file.iso", 4]],
  "top_ventoy_codes": [[1, 3]]
}
```

### Response `400`

```json
{"error": "invalid_ip"}
```

---

## Typical Cursor workflow

1. **List recent failures**

   ```http
   GET /v1/sessions?outcomes=install_failed&days=7&per_page=10
   ```

2. **Or list recent uploads**

   ```http
   GET /v1/uploads?days=7&per_page=10
   ```

3. **Drill into a keyword**

   ```http
   GET /v1/support/MEDICAT-A7X9K2
   ```

4. **Or drill into a session**

   ```http
   GET /v1/sessions/7c9e6679-7425-40de-944b-e07fc1f90ae7
   ```

5. **Check aggregate patterns**

   ```http
   GET /v1/insights?days=30
   ```

---

## curl examples

```bash
# List uploads
curl -sS -H "X-API-Key: $APP_API_KEY" \
  "https://telemetry.medicatusb.com/v1/uploads?per_page=25"

# Session detail
curl -sS -H "Authorization: Bearer $APP_API_KEY" \
  "https://telemetry.medicatusb.com/v1/sessions/7c9e6679-7425-40de-944b-e07fc1f90ae7"

# Post session (installer)
curl -sS -X POST \
  -H "Authorization: Bearer $INGEST_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"session_id":"…","client":"MedicatInstaller","operation":"launch","outcome":"opened"}' \
  "https://telemetry.medicatusb.com/v1/sessions"
```

---

## Notes

- Read endpoints return **metadata and parsed manifest/error text**, not raw zip or log file contents. View full logs in the admin UI or public support page (`support_url`).
- Client IP on ingest uses `CF-Connecting-IP`, then `X-Forwarded-For`, then `remote_addr` when `TRUST_PROXY_HEADERS=true`.
- Admin web UI uses `ADMIN_PASSWORD` (password only) — separate from the API key.
