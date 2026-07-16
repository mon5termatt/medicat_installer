# Loads translations.json and exposes Get-*Translation lookup functions.
# Referenced by MedicatFileChecker.ps1 via dot-sourcing; previously missing from the repo,
# which caused every UI label/status/message to fall back to a raw "[key]" placeholder
# and broke the drive dropdown's per-item formatting entirely.

$script:TranslationRoot = $null
$script:TranslationLanguage = "en"

function Load-Translations {
    param(
        [string]$Language = "en"
    )

    $translationsPath = Join-Path $PSScriptRoot "translations.json"
    if (-not (Test-Path -LiteralPath $translationsPath)) {
        Write-Warning "translations.json not found at $translationsPath. Falling back to raw keys."
        return $false
    }

    $allTranslations = Get-Content -LiteralPath $translationsPath -Raw | ConvertFrom-Json
    $script:TranslationLanguage = if ($allTranslations.$Language) { $Language } else { "en" }
    $script:TranslationRoot = $allTranslations.$($script:TranslationLanguage)
    return $true
}

function Get-TranslationValue {
    param(
        [string]$Category,
        [string]$Key,
        $FormatArgs = @()
    )

    $text = $null
    if ($script:TranslationRoot -and $script:TranslationRoot.$Category.$Key) {
        $text = $script:TranslationRoot.$Category.$Key
    }

    if (-not $text) {
        return "[$Key]"
    }

    if ($FormatArgs -and @($FormatArgs).Count -gt 0) {
        return ($text -f @($FormatArgs))
    }

    return $text
}

function Get-UITranslation {
    param([string]$Key, $FormatArgs = @())
    return Get-TranslationValue -Category "ui" -Key $Key -FormatArgs $FormatArgs
}

function Get-StatusTranslation {
    param([string]$Key, $FormatArgs = @())
    return Get-TranslationValue -Category "status" -Key $Key -FormatArgs $FormatArgs
}

function Get-MessageTranslation {
    param([string]$Key, $FormatArgs = @())
    return Get-TranslationValue -Category "messages" -Key $Key -FormatArgs $FormatArgs
}

function Get-TitleTranslation {
    param([string]$Key, $FormatArgs = @())
    return Get-TranslationValue -Category "titles" -Key $Key -FormatArgs $FormatArgs
}
