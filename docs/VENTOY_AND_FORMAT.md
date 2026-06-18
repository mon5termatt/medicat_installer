# Ventoy install vs NTFS format

Notes on how the C++ installer handles Ventoy and NTFS formatting.  
Code: `src/app.cpp` (`RunInstallThread`), `src/ventoy.cpp` (`RunVentoyInstall`, `FormatDriveNtfs`).

---

## Install order

When both Ventoy and format are enabled:

1. **Ventoy** — download/extract Ventoy2Disk, then `VTOYCLI /I` or `/U`
2. **Reconcile drive letter** — Ventoy may remount under a different letter
3. **NTFS format** — `format.com` on the visible data volume (if format flag is on)
4. **Extract** — `7za` unpack of `MediCat.USB.v*.7z`
5. **Verify** — MD5 check against embedded `MedicatFiles.md5`

Format runs **after** Ventoy, **before** extract. CLI help text (“format before extract”) refers to that position relative to extraction, not relative to Ventoy.

---

## Ventoy filesystem flag (`/FS:`)

Ventoy’s Windows CLI supports choosing the main data-partition filesystem on **install** (`/I` only):

| Flag | Description |
|------|-------------|
| `/FS:NTFS` | NTFS data partition |
| `/FS:EXFAT` | exFAT (Ventoy default if omitted) |
| `/FS:FAT32` | FAT32 |
| `/FS:UDF` | UDF |

Official docs: [Ventoy — Windows Command Line](https://www.ventoy.net/en/doc_windows_cli.html)

Example:

```bat
Ventoy2Disk.exe VTOYCLI /I /Drive:D: /GPT /NOUSBCheck /FS:NTFS
```

**We do not pass `/FS:` today.** Fresh Ventoy installs use Ventoy’s default (**exFAT**). When the user enables format, we NTFS-format afterward with `format.com` instead of asking Ventoy to create NTFS during partition setup.

Current `RunVentoyInstall` arguments: `/I` or `/U`, `/Drive:X:`, optional `/GPT`, `/NOSB`, `/NOUSBCheck` only.

---

## Our NTFS format step

When `format` is true, `FormatDriveNtfs()` runs:

```text
format.com X: /FS:NTFS /X /Q /V:Medicat /Y
```

- Quick NTFS format of the **visible volume letter** (not a full-disk repartition)
- Volume label: **Medicat**
- Failure aborts the install (`errors.format_failed`)

Ventoy **fresh install** (`/I`) already repartitions the disk; the format checkbox adds (or relies on) an explicit NTFS quick format of the data partition before extract.

---

## When format runs

| Scenario | Ventoy | NTFS `format.com` |
|----------|--------|-------------------|
| No Ventoy on drive (forced in GUI / CLI) | Yes — `/I` | Yes — forced |
| Ventoy present + **Format** checked | Yes — `/I` (format forces destructive install) | Yes |
| Ventoy present + **Update Ventoy** only (format off) | Yes — `/U` | No |
| CLI `/format /noventoy` | Skipped | Yes |
| Format off, Ventoy off | Skipped | No |

GUI: `FormatChecked()` returns true when Ventoy is not on the selected drive, regardless of checkbox state.

CLI: `ResolveHeadlessInstallOptions()` forces `format=true` and `runVentoy=true` when Ventoy is not detected; `/noformat` is rejected in that case.

---

## Why both Ventoy and format?

MediCat expects a large NTFS data area. Ventoy defaults to exFAT on `/I` without `/FS:NTFS`. The separate format step ensures NTFS + **Medicat** label before extraction when the user opts in (or when forced for a non-Ventoy drive).

**Possible improvement:** pass `/FS:NTFS` (and optionally `/Label:Medicat` if supported) on destructive Ventoy installs, and treat `format.com` as redundant for filesystem type only — still useful for wiping an existing volume when `/noventoy /format` is used.

---

## References

- Ventoy Windows CLI: https://www.ventoy.net/en/doc_windows_cli.html
- Installer architecture flow: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- CLI flags: [`CLI.md`](../CLI.md)
