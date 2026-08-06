# Command-line flags — design draft

Work file for optional **command-line control** of `MedicatInstaller.exe` (GUI-first today; flags enable scripting, diagnostics, and unattended flows).

**Partially implemented** — parsing, help/version/diagnostics, headless `/install` and `/verify` with `/yes` `/quiet`. GUI remains the default when no action flags are passed.

---

## Goal

Support three usage modes without forking the codebase:

| Mode | Behavior |
|------|----------|
| **Default (no args)** | Current Win32 GUI — drive picker, checkboxes, confirm dialogs |
| **Info / exit** | Print version or help to stdout/console and exit |
| **Scripted action** | Run **install** or **verify** headlessly with explicit options; log to file; exit code for automation |

Non-goals for v1:

- Fully silent replace of the running exe (see [UPDATER.md](UPDATER.md)).
- Embedding or downloading the MediCat `.7z` via new flags beyond existing path resolution.
- PowerShell-style `-WhatIf` dry run (could be v2).

---

## Conventions

- **Windows style:** `/flag`, `/flag:value`, `/flag value` (Ventoy / `format.com` parity).
- **Unix style (alias):** `--flag`, `--flag=value`, `--flag value` — accepted for the same options where unambiguous.
- **Case:** Insensitive for flag names; drive letters normalized to `E:`.
- **Parsing:** Single pass over `CommandLineToArgvW` tokens; unknown flags → exit **2** with help hint.
- **Elevation:** Destructive actions still require Administrator (UAC manifest). If not elevated, exit **3** with a clear message (console or `MessageBox` unless `/quiet`).
- **Mutually exclusive groups:** Documented per section; last wins or hard error (prefer **error** for safety).

### Console attachment

Info flags (`/help`, `/version`) should attach or allocate a console (`AttachConsole(ATTACH_PARENT_PROCESS)` then fallback `AllocConsole`) so output is visible when launched from `cmd` / PowerShell / CI. GUI-only launches without those flags behave as today.

---

## Exit codes (proposed)

| Code | Meaning |
|------|---------|
| `0` | Success (install finished + verify passed, or verify-only passed) |
| `1` | Operation failed (Ventoy, extract, verify, network, bundled tools missing) |
| `2` | Invalid or conflicting arguments |
| `3` | Administrator elevation required |
| `4` | User cancelled (confirmation or re-extract prompt in interactive CLI) |
| `5` | Verify found failures (install completed but hash mismatches remain) |
| `6` | Partial success — re-extract offered but skipped or still failing (unattended policy dependent) |

Log detail always goes to `medicat_installer.log` beside the exe (or `/log:` path).

---

## Flags reference

### Help and version

| Flag | Alias | Action |
|------|-------|--------|
| `/help` | `/h`, `/?`, `--help` | Print usage summary to console; exit `0` |
| `/version` | `/v`, `--version` | Print installer version, build number, arch, release tag (when embedded); exit `0` |

**Example output (`/version`):**

```text
MedicatInstaller 1.0.11 (build 11) x64
Release tag: 3521-BETA
MediCat USB: v21.12
```

---

### Language (GUI + messages)

| Flag | Alias | Values | Default |
|------|-------|--------|---------|
| `/lang:` | `--lang=` | `en`, `es`, `fr`, `pl`, `tr` | OS UI language (same as GUI combo) |

Applies before any user-visible text (dialogs, log prefixes). Invalid code → exit **2**.

*(Future: persist in `%AppData%\MedicatInstaller\settings.json` — see [UPDATER.md](UPDATER.md).)*

---

### Target drive

| Flag | Alias | Values | Required when |
|------|-------|--------|---------------|
| `/drive:` | `--drive=` | `E`, `E:`, `E:\` | `/install` or `/verify` |

Rules (same as GUI):

- Must not be `C:`.
- Must meet **≥ 30 GiB** capacity (`kMinDriveCapacityBytes`).
- Must appear in the drive list for the current **show-all-drives** policy unless `/allow-fixed` is set (see below).

Without `/drive:` in GUI mode, user picks from the combo as today.

---

### Actions

| Flag | Alias | Description |
|------|-------|-------------|
| `/install` | `--install` | Full install pipeline: optional Ventoy → optional format → extract → verify |
| `/verify` | `--verify` | MD5 verify only (**Check USB Files**); no Ventoy, format, or extract |

If both are present → exit **2**. If neither is present → normal GUI.

**Re-extract on verify failure:**

| Flag | Default (unattended) | Behavior |
|------|----------------------|----------|
| `/reextract` | off unless `/yes` | On verify failure, run selective `7za @list` then re-verify |
| `/noreextract` | — | Fail with exit **5** after first verify failure |
| `/reextract-only` | — | Skip install/extract; only re-extract paths listed in `failed_files.txt` on the drive (advanced recovery) |

Interactive CLI (no `/quiet`): show re-extract prompt equivalent to `Gui::OpenReExtractPrompt`.

---

### Install options (mirror GUI checkboxes)

Defaults match a **fresh USB without Ventoy** (forced install path). When Ventoy is already on the drive, defaults match GUI: format **off**, Ventoy update **off** unless flags say otherwise.

| Flag | GUI equivalent | Default (no Ventoy on drive) | Default (Ventoy present) |
|------|----------------|------------------------------|---------------------------|
| `/format` | Format checkbox | on (forced) | off |
| `/noformat` | Unchecked format | — | explicit off |
| `/ventoy` | Install / Update Ventoy | on (forced) | off |
| `/noventoy` | Skip Ventoy step | — | explicit skip |
| `/gpt` | Advanced → GPT | off | off |
| `/nogpt` | MBR | — | — |
| `/secureboot` | Advanced → Secure Boot | on | on |
| `/nosb` | `/nosecureboot` → `/NOSB` to Ventoy | — | — |
| `/ventoy-version:` | Pin Ventoy version | latest (or offline embed) | same |
| `/allow-fixed` | Show all drives | off | off |

**Forced Ventoy rule:** When the selected drive has **no Ventoy**, `/noventoy` and `/noformat` are rejected (exit **2**) — same as `RequiresForcedVentoyInstall()` in `gui.cpp`.

**Ventoy version pin:** `/ventoy-version:1.1.12` implies pin enabled; invalid or unavailable version → exit **1** with log line.

---

### Paths and offline assets

| Flag | Alias | Description |
|------|-------|-------------|
| `/archive:` | `--archive=` | Override MediCat `.7z` path (instead of beside exe / `offline/` lookup) |
| `/offline` | `--offline` | Do not download Ventoy or archive from the network; use embedded Ventoy ([OFFLINE.md](OFFLINE.md)) and `offline/` cache only |
| `/log:` | `--log=` | Write session log to a custom file (default: `{exeDir}\medicat_installer.log`) |

`/archive:` does not bypass missing-file UI in GUI mode unless combined with `/install` and `/quiet`.

---

### Unattended / automation

| Flag | Alias | Description |
|------|-------|-------------|
| `/yes` | `/y`, `--yes` | Auto-accept wipe confirmation and Ventoy warning ( **destructive** ) |
| `/quiet` | `/q`, `--quiet` | No message boxes; errors to log + stderr; exit code only |
| `/noprogress` | — | Do not show progress window (future: console `%` lines when console attached) |

`/quiet` without `/yes` on `/install` → exit **4** at first confirmation (fail closed).

**Suggested automation example:**

```bat
MedicatInstaller.exe /install /drive:E /yes /quiet /lang:en
```

---

### Self-update (future — [UPDATER.md](UPDATER.md))

| Flag | Description |
|------|-------------|
| `/check-update` | Fetch manifest / GitHub; print result; exit `0` if up to date, `1` if update available (or invert for CI — TBD) |
| `/no-update-check` | Skip deferred startup update check (when updater is implemented) |

When `MEDICAT_OFFLINE_BUILD` is defined at compile time, `/check-update` is a no-op (exit `0`, message “offline build”).

---

### Diagnostics

| Flag | Description |
|------|-------------|
| `/list-drives` | Print eligible drives (one per line: letter, label, type, size) and exit `0` |
| `/dump-config` | Print resolved paths (exe dir, 7za temp, archive, Ventoy dir, manifest) and exit `0` |

Useful for support scripts; no admin required for `/list-drives` (read-only).

---

## Mode matrix

```text
(no args)                    → GUI
/help, /version              → console, exit
/list-drives, /dump-config  → console, exit
/verify /drive:E             → headless verify (+ optional /quiet)
/install /drive:E /yes …    → headless install
/lang:fr (alone)             → GUI in French
/check-update                → console check, exit (future)
```

---

## Headless install flow (proposed)

Same worker threads as GUI (`RunInstallThread` / `RunVerifyThread`), but:

1. Parse flags → `CliOptions` struct (parallel to `Gui` getters).
2. Skip `gui_.Create()` when action is `/install` or `/verify` and `/drive:` is valid.
3. Replace `MessageBox` confirmations with `/yes` or fail with exit **4**.
4. Replace `PostDone` UI with log + `ExitProcess(code)`.
5. Re-extract prompt: `/reextract` + `/yes` auto-runs; else exit **5** or **6**.

Progress callbacks write to log; optional console progress when stdout is a TTY.

```text
Parse CLI
  → elevation check
  → EnsureBundledTools
  → resolve drive + archive
  → ConfirmWipe (if /install && !/yes) 
  → RunInstallThread or RunVerifyThread
  → join worker
  → exit code
```

---

## Help text (draft)

```text
MediCat USB Installer — usage

  MedicatInstaller.exe                     Open graphical installer
  MedicatInstaller.exe /help               Show this help
  MedicatInstaller.exe /version            Show version and build

Actions:
  /install /drive:E                        Install MediCat to drive E:
  /verify /drive:E                         Verify MD5 hashes on drive E:

Common options:
  /format /noformat                        NTFS format before extract
  /ventoy /noventoy                        Install or update Ventoy
  /gpt /secureboot /nosb                   Ventoy partition options
  /ventoy-version:1.1.12                   Pin Ventoy release
  /archive:"D:\path\MediCat.USB.v21.12.7z" Override archive location
  /lang:en                                 UI language (en es fr pl tr)
  /yes                                     Accept destructive prompts (required with /quiet)
  /quiet                                   No dialogs; use exit codes

Diagnostics:
  /list-drives                             List eligible removable/VHD drives
  /dump-config                             Show resolved paths and options

Exit codes: 0 ok, 1 error, 2 bad args, 3 need admin, 4 cancelled, 5 verify failed

Administrator required for /install. Logs: medicat_installer.log beside the exe.
```

---

## Architecture (proposed modules)

```text
src/cli.h / cli.cpp
  struct CliOptions { ... };
  CliParseResult ParseCommandLine(int argc, wchar_t** argv);
  void PrintHelp();
  void PrintVersion();
  int RunHeadless(App& app, const CliOptions& opts);
```

```text
App::Run()
  auto cli = ParseCommandLine(__argc, __wargv);
  if (cli.showHelp) { PrintHelp(); return 0; }
  if (cli.showVersion) { PrintVersion(); return 0; }
  if (cli.action == Install || Verify) return RunHeadless(*this, cli);
  // existing GUI path
```

Threading unchanged — headless mode still uses worker threads; only the UI sink differs (`PostDone` → exit code).

Settings precedence (when implemented):

```text
defaults ← GUI checkbox rules ← CLI flags ← (optional) settings.json
```

---

## Security and safety

| Topic | Approach |
|-------|----------|
| Destructive ops | Require `/yes` for unattended `/install`; never imply `/yes` from `/quiet` alone |
| Drive validation | Same capacity and `C:` rules as GUI; log physical disk number |
| Forced Ventoy | Reject `/noventoy` when Ventoy missing on target |
| Logging | Always log full CLI argv (redact nothing — no secrets expected) in `medicat_installer.log` |
| Elevation | Manifest stays `requireAdministrator`; document “Run as administrator” for scripts |

---

## i18n

CLI **help and version strings** stay **English** for v1 (console tooling convention). `/lang:` affects confirmation dialogs and log messages that use `i18n::Tr()` when those dialogs are shown (non-`/quiet`).

---

## Implementation checklist

- [x] `ParseCommandLine` using `CommandLineToArgvW` / `__wargv` from `RunApp`
- [x] `CliOptions` + validation (conflicts, forced Ventoy rule)
- [x] Console attach for `/help`, `/version`, `/list-drives`, `/dump-config`
- [x] `RunHeadless` — wire to `RunInstallThread` / `RunVerifyThread` without message loop
- [x] Confirmation bypass via `/yes`; map `PostDone` for headless exit codes
- [x] Exit codes documented in help
- [x] Log argv + parsed options at startup (`LogCommandLine`)
- [ ] Embed `kInstallerReleaseTag` for `/version` (CMake — see [UPDATER.md](UPDATER.md))
- [ ] `/lang:` alone in GUI — sync language combo on startup (partial: i18n loads; combo may lag until refresh)
- [ ] `/reextract-only` recovery path
- [ ] `/check-update`, `/no-update-check` (updater module)
- [ ] Manual test: VHD install/verify with `/yes /quiet`
- [ ] Optional: `--` end-of-options marker for paths with spaces

**Note:** The exe UAC manifest is `requireAdministrator`, so **all** launches (including `/help`) prompt for elevation today.

**Ctrl+C / Ctrl+Break (CLI mode):** Cancels the current operation (exit code **4**), terminates all tracked `7za.exe` child processes, and stops verify worker threads. Enabled automatically for `/install` and `/verify`.

---

## Related files (today)

| Path | Role |
|------|------|
| `src/main.cpp` | Entry — pass cmdline to `App` |
| `src/app.cpp` | Install/verify orchestration, confirmations |
| `src/gui.cpp` | Checkbox defaults, forced Ventoy logic |
| `generated/build_version.cpp` | `kInstallerVersion`, `kInstallerBuildNumber` |
| `src/offline.cpp` | Archive / Ventoy cache paths |
| `src/debug.cpp` | Diagnostics logged at startup |

---

## Open questions

1. Should `/install` imply `/format` when Ventoy is present, or always require explicit `/format` for destructive format?
2. Console progress (`Extracting 42%`) in v1 or log-only until v2?
3. Should `/list-drives` include fixed disks only when `/allow-fixed` is passed?
4. Integrate with Task Scheduler / Intune — need exit **5** distinct from generic **1**? (Proposed: yes.)
5. Export equivalent **response file** (`@options.txt`) for repeatable installs?

---

## See also

- [FEATURES.md](FEATURES.md) — “Language override setting — Future: CLI flag or ini”
- [OFFLINE.md](OFFLINE.md) — `/offline` behavior vs compile-time offline exe
- [UPDATER.md](UPDATER.md) — `/check-update`, `/no-update-check`
