# Translation System Documentation

This document describes the translation system for the MediCat Installer.

## Files

- **`translations.json`**: Contains all translatable strings organized by language
- **`TranslationHelper.ps1`**: PowerShell module with helper functions to load and use translations

## Supported Languages

Currently supported languages:
- **en** (English) - Default
- **es** (Spanish / Español)
- **fr** (French / Français)

## Translation File Structure

The `translations.json` file is organized as follows:

```json
{
  "en": {
    "ui": { ... },           // UI element labels (buttons, checkboxes, etc.)
    "status": { ... },       // Status bar messages
    "messages": { ... },     // MessageBox messages
    "titles": { ... },        // MessageBox titles
    "ventoy_warning": { ... },
    "ventoy_not_detected": { ... },
    "log": { ... },          // Log messages
    "errors": { ... }        // Error messages
  },
  "es": { ... },
  "fr": { ... }
}
```

## Using Translations in Code

### Basic Usage

```powershell
# Import the translation helper
. .\TranslationHelper.ps1

# Load translations (defaults to English)
Load-Translations

# Or load a specific language
Load-Translations -Language "es"

# Get a translation
$title = Get-Translation -Key "ui.form_title" -FormatArgs "1.0.0"
# Returns: "MediCat Installer v1.0.0 [Administrator]"

# Get UI translation
$buttonText = Get-UITranslation -Key "install_button"
# Returns: "Install MediCat"

# Get status message
$status = Get-StatusTranslation -Key "downloading_files"
# Returns: "Downloading installer files..."

# Get message box content
$message = Get-MessageTranslation -Key "no_internet"
$title = Get-TitleTranslation -Key "no_internet"
```

### Example: Updating UI Elements

```powershell
# Load translations
Load-Translations -Language "es"

# Update form title
$form.Text = Get-UITranslation -Key "form_title" -FormatArgs $script:LocalVersion

# Update button text
$installButton.Text = Get-UITranslation -Key "install_button"
$cancelButton.Text = Get-UITranslation -Key "cancel_button"

# Update status
Update-Status (Get-StatusTranslation -Key "status_ready")
```

### Example: Showing Message Boxes

```powershell
# Simple message
$message = Get-MessageTranslation -Key "no_internet"
$title = Get-TitleTranslation -Key "no_internet"
Show-MessageBox -Message $message -Title $title -Icon "Warning"

# Message with formatting
$message = Get-MessageTranslation -Key "installation_complete" -FormatArgs "F:"
$title = Get-TitleTranslation -Key "installation_complete"
Show-MessageBox -Message $message -Title $title -Icon "Information"
```

### Example: Ventoy Warning

```powershell
$warning = Get-VentoyWarning -DriveLetter "F:"
Show-MessageBox -Message $warning.Message -Title $warning.Title -Buttons "YesNo" -Icon "Warning"
```

## String Formatting

Many translation strings support formatting using PowerShell's `-f` operator. Use `{0}`, `{1}`, etc. as placeholders:

```json
{
  "ui": {
    "form_title": "MediCat Installer v{0} [Administrator]"
  }
}
```

Usage:
```powershell
Get-UITranslation -Key "form_title" -FormatArgs "1.0.0"
# Returns: "MediCat Installer v1.0.0 [Administrator]"
```

## Adding a New Language

1. Open `translations.json`
2. Add a new top-level key with the language code (e.g., `"de"` for German)
3. Copy the structure from `"en"` and translate all strings
4. Save the file

Example:
```json
{
  "en": { ... },
  "es": { ... },
  "fr": { ... },
  "de": {
    "ui": {
      "form_title": "MediCat Installer v{0} [Administrator]",
      ...
    },
    ...
  }
}
```

5. Update the `ValidateSet` in `Set-Language` function if needed:
```powershell
[ValidateSet("en", "es", "fr", "de")]
```

## Adding New Translation Keys

1. Add the key to all language sections in `translations.json`
2. Use dot notation for nested keys (e.g., `"ui.new_button"`)
3. For formatting, use `{0}`, `{1}`, etc. as placeholders

Example:
```json
{
  "en": {
    "ui": {
      "new_button": "New Button Text"
    }
  },
  "es": {
    "ui": {
      "new_button": "Texto del Nuevo Botón"
    }
  }
}
```

Usage:
```powershell
$button.Text = Get-UITranslation -Key "new_button"
```

## Language Detection

You can detect the system language and auto-load translations:

```powershell
# Get system language
$culture = [System.Globalization.CultureInfo]::CurrentCulture
$langCode = $culture.TwoLetterISOLanguageName

# Map to supported languages
$supportedLanguages = @{
    "en" = "en"
    "es" = "es"
    "fr" = "fr"
}

$language = if ($supportedLanguages.ContainsKey($langCode)) {
    $supportedLanguages[$langCode]
} else {
    "en"  # Default to English
}

Load-Translations -Language $language
```

## Fallback Behavior

- If a translation key is not found, the function returns `[$Key]` as a placeholder
- If a language is not found, the system falls back to English
- If the translation file is missing, the system uses hardcoded English strings (if implemented)

## Notes

- All translation strings should be UTF-8 encoded
- Special characters and emojis are supported
- Line breaks in messages use `\n` (will be converted to actual line breaks when displayed)
- The translation system is case-sensitive for keys

