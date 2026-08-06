# Translations (C++ installer)

## Method

**Source of truth:** `i18n/translations.json` (same format as the `pwsh` branch)

**Build time:** `tools/i18n_codegen.py` generates `cpp/src/i18n_generated.h`

**Runtime:** `cpp/src/i18n.cpp` loads the language pack for the Windows UI locale and exposes:

```cpp
i18n::Tr(L"ui.install_button");
i18n::Tr(L"messages.file_not_found", archiveName);
i18n::Tr(L"ui.drive_format", letter, type, freeGb, totalGb);
```

## Why this approach

| Old (PowerShell) | New (C++) |
|------------------|-----------|
| Load JSON at runtime | Compile strings into the exe at build time |
| `ConvertFrom-Json` | Generated C++ lookup table |
| No startup parse cost | No JSON parser dependency in the exe |
| Same `translations.json` | Same keys, same `{0}` placeholders |

## Workflow

1. Edit `i18n/translations.json`
2. Rebuild (CMake runs codegen automatically) or run:
   ```bat
   python tools/i18n_codegen.py
   ```
3. Use keys as `category.key` (flattened from JSON nesting)

## Languages

Detected from `GetUserDefaultUILanguage()` → `en`, `es`, `fr`, `de` (falls back to `en`).

## Adding a language

1. Copy the `en` block in `translations.json`
2. Translate values (keep keys identical)
3. Rebuild

Optional: use `generate_translation.py` on the `pwsh` branch as an interactive helper, then copy the new block into `i18n/translations.json`.

## Key conventions

- `ui.*` — labels, buttons, checkboxes
- `status.*` — progress bar / status line
- `messages.*` — message boxes
- `titles.*` — message box titles
- `ventoy_warning.*` — Ventoy-specific dialogs

Placeholders: `{0}`, `{1}`, … (same as PowerShell installer)
