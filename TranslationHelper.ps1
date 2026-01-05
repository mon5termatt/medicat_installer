# Translation Helper Functions for MediCat Installer
# This script provides functions to load and use translations from translations.json

# Global variable to store translations
$script:Translations = $null
$script:CurrentLanguage = "en"

function Get-TranslationFile {
    <#
    .SYNOPSIS
    Gets the path to the translations.json file.
    
    .DESCRIPTION
    Looks for translations.json in the script directory, then current directory.
    #>
    $scriptPath = $MyInvocation.PSCommandPath
    if (-not $scriptPath) {
        $scriptPath = $PSCommandPath
    }
    
    if ($scriptPath) {
        $scriptDir = Split-Path -Parent $scriptPath
        $translationFile = Join-Path $scriptDir "translations.json"
        if (Test-Path $translationFile) {
            return $translationFile
        }
    }
    
    # Fallback to current directory
    $translationFile = Join-Path $PWD "translations.json"
    if (Test-Path $translationFile) {
        return $translationFile
    }
    
    return $null
}

function Load-Translations {
    <#
    .SYNOPSIS
    Loads translations from translations.json file.
    
    .DESCRIPTION
    Loads the translation file and stores it in $script:Translations.
    Falls back to English if the file cannot be loaded.
    
    .PARAMETER Language
    The language code to use (e.g., "en", "es", "fr"). Defaults to "en".
    
    .EXAMPLE
    Load-Translations -Language "es"
    #>
    param(
        [string]$Language = "en"
    )
    
    $translationFile = Get-TranslationFile
    
    if (-not $translationFile) {
        Write-Warning "Translation file not found. Using English (hardcoded fallback)."
        $script:Translations = @{
            "ui" = @{}
            "status" = @{}
            "messages" = @{}
            "titles" = @{}
        }
        $script:CurrentLanguage = "en"
        return $false
    }
    
    try {
        $translationData = Get-Content -Path $translationFile -Raw -Encoding UTF8 | ConvertFrom-Json
        
        if ($translationData.PSObject.Properties.Name -contains $Language) {
            $script:Translations = $translationData.$Language
            $script:CurrentLanguage = $Language
            Write-Debug "Translations loaded for language: $Language"
            return $true
        } else {
            Write-Warning "Language '$Language' not found in translation file. Falling back to English."
            if ($translationData.PSObject.Properties.Name -contains "en") {
                $script:Translations = $translationData.en
                $script:CurrentLanguage = "en"
                return $true
            } else {
                Write-Error "English translations not found in translation file."
                return $false
            }
        }
    } catch {
        Write-Error "Failed to load translations: $($_.Exception.Message)"
        $script:Translations = @{
            "ui" = @{}
            "status" = @{}
            "messages" = @{}
            "titles" = @{}
        }
        $script:CurrentLanguage = "en"
        return $false
    }
}

function Get-Translation {
    <#
    .SYNOPSIS
    Gets a translated string from the loaded translations.
    
    .DESCRIPTION
    Retrieves a translated string from the loaded translation data.
    Supports nested paths like "ui.form_title" or "messages.no_internet".
    Supports string formatting with -FormatArgs parameter.
    
    .PARAMETER Key
    The translation key, using dot notation for nested paths (e.g., "ui.form_title").
    
    .PARAMETER FormatArgs
    Optional array of arguments for string formatting (uses -f operator).
    
    .PARAMETER Default
    Optional default value if translation key is not found.
    
    .EXAMPLE
    Get-Translation -Key "ui.form_title" -FormatArgs "1.0.0"
    # Returns: "MediCat Installer v1.0.0 [Administrator]"
    
    .EXAMPLE
    Get-Translation -Key "messages.no_internet"
    # Returns: "No internet connection detected..."
    #>
    param(
        [Parameter(Mandatory=$true)]
        [string]$Key,
        
        [object[]]$FormatArgs = @(),
        
        [string]$Default = $null
    )
    
    # Ensure translations are loaded
    if ($null -eq $script:Translations) {
        Load-Translations
    }
    
    # Navigate through nested structure
    $parts = $Key -split '\.'
    $value = $script:Translations
    
    foreach ($part in $parts) {
        if ($value -and $value.PSObject.Properties.Name -contains $part) {
            $value = $value.$part
        } else {
            # Key not found
            if ($Default) {
                if ($FormatArgs.Count -gt 0) {
                    return $Default -f $FormatArgs
                } else {
                    return $Default
                }
            } else {
                Write-Warning "Translation key not found: $Key (Language: $script:CurrentLanguage)"
                return "[$Key]"
            }
        }
    }
    
    # Apply formatting if arguments provided
    if ($FormatArgs.Count -gt 0) {
        try {
            return $value -f $FormatArgs
        } catch {
            Write-Warning "Failed to format translation '$Key' with provided arguments: $($_.Exception.Message)"
            return $value
        }
    }
    
    return $value
}

function Get-UITranslation {
    <#
    .SYNOPSIS
    Gets a UI element translation.
    
    .PARAMETER Key
    The UI key (e.g., "form_title", "install_button").
    
    .PARAMETER FormatArgs
    Optional formatting arguments.
    #>
    param(
        [Parameter(Mandatory=$true)]
        [string]$Key,
        
        [object[]]$FormatArgs = @()
    )
    
    return Get-Translation -Key "ui.$Key" -FormatArgs $FormatArgs
}

function Get-StatusTranslation {
    <#
    .SYNOPSIS
    Gets a status message translation.
    
    .PARAMETER Key
    The status key (e.g., "downloading_files", "extracting_archive").
    
    .PARAMETER FormatArgs
    Optional formatting arguments.
    #>
    param(
        [Parameter(Mandatory=$true)]
        [string]$Key,
        
        [object[]]$FormatArgs = @()
    )
    
    return Get-Translation -Key "status.$Key" -FormatArgs $FormatArgs
}

function Get-MessageTranslation {
    <#
    .SYNOPSIS
    Gets a message box message translation.
    
    .PARAMETER Key
    The message key (e.g., "no_internet", "installation_complete").
    
    .PARAMETER FormatArgs
    Optional formatting arguments.
    #>
    param(
        [Parameter(Mandatory=$true)]
        [string]$Key,
        
        [object[]]$FormatArgs = @()
    )
    
    return Get-Translation -Key "messages.$Key" -FormatArgs $FormatArgs
}

function Get-TitleTranslation {
    <#
    .SYNOPSIS
    Gets a message box title translation.
    
    .PARAMETER Key
    The title key (e.g., "no_internet", "installation_complete").
    
    .PARAMETER FormatArgs
    Optional formatting arguments.
    #>
    param(
        [Parameter(Mandatory=$true)]
        [string]$Key,
        
        [object[]]$FormatArgs = @()
    )
    
    return Get-Translation -Key "titles.$Key" -FormatArgs $FormatArgs
}

function Get-VentoyWarning {
    <#
    .SYNOPSIS
    Gets the Ventoy installation warning message and title.
    
    .PARAMETER DriveLetter
    The drive letter to include in the message.
    
    .OUTPUTS
    PSCustomObject with Title and Message properties.
    #>
    param(
        [string]$DriveLetter
    )
    
    return [PSCustomObject]@{
        Title = Get-Translation -Key "ventoy_warning.title"
        Message = Get-Translation -Key "ventoy_warning.message" -FormatArgs $DriveLetter
    }
}

function Get-VentoyNotDetected {
    <#
    .SYNOPSIS
    Gets the Ventoy not detected warning message and title.
    
    .PARAMETER DriveLetter
    The drive letter to include in the message.
    
    .OUTPUTS
    PSCustomObject with Title and Message properties.
    #>
    param(
        [string]$DriveLetter
    )
    
    return [PSCustomObject]@{
        Title = Get-Translation -Key "ventoy_not_detected.title"
        Message = Get-Translation -Key "ventoy_not_detected.message" -FormatArgs $DriveLetter
    }
}

function Set-Language {
    <#
    .SYNOPSIS
    Sets the current language and reloads translations.
    
    .PARAMETER Language
    The language code (e.g., "en", "es", "fr").
    
    .EXAMPLE
    Set-Language -Language "es"
    #>
    param(
        [Parameter(Mandatory=$true)]
        [ValidateSet("en", "es", "fr")]
        [string]$Language
    )
    
    Load-Translations -Language $Language
    Write-Host "Language set to: $Language" -ForegroundColor Green
}

# Auto-load translations on script import
# Note: When dot-sourced, Export-ModuleMember is not needed and will cause an error
# Functions are available in the parent scope when dot-sourced
if ($MyInvocation.InvocationName -ne '.') {
    Load-Translations
}

