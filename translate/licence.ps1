# MediCat USB — legacy batch bridge (loaded by Medicat_Installer.bat from main/translate/).
# Fielded clients curl this URL every run (no hash). Replace licence display with a
# forced hand-off to update.bat → C++ MedicatInstaller.exe.
#
# Path (locked by 3520 clients):
#   https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/translate/licence.ps1
#
# Args: %lang% (optional; ignored for migration)

$ErrorActionPreference = 'Continue'
$WorkDir = (Get-Location).Path
$UpdateBat = Join-Path $WorkDir 'update.bat'
$UpdateUrl = 'https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/update.bat'

function Write-Banner {
    Write-Host ''
    Write-Host '################################################################'
    Write-Host '#  MediCat Installer has moved to a new C++ application.      #'
    Write-Host '#  The old batch installer is no longer maintained.           #'
    Write-Host '################################################################'
    Write-Host ''
    Write-Host 'Downloading the updater and installing MedicatInstaller...'
    Write-Host ''
}

function Get-ParentCmdProcessId {
    try {
        $me = Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop
        return [int]$me.ParentProcessId
    } catch {
        return 0
    }
}

Write-Banner

try {
    # Prefer curl.exe (same tool the bat uses); fall back to Invoke-WebRequest.
    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($curl) {
        & curl.exe -fsSL $UpdateUrl -o $UpdateBat
        if ($LASTEXITCODE -ne 0) { throw "curl failed with exit $LASTEXITCODE" }
    } else {
        Invoke-WebRequest -Uri $UpdateUrl -OutFile $UpdateBat -UseBasicParsing
    }
} catch {
    Write-Host "ERROR: Could not download update.bat"
    Write-Host $_.Exception.Message
    Write-Host ''
    Write-Host 'Manual download:'
    Write-Host '  https://github.com/mon5termatt/medicat_installer/releases'
    Write-Host '  Get MedicatInstaller.exe (x64) or MedicatInstaller-x86.exe'
    Write-Host ''
    Write-Host 'Press Enter to close...'
    [void][System.Console]::ReadLine()
    exit 1
}

if (-not (Test-Path -LiteralPath $UpdateBat) -or (Get-Item -LiteralPath $UpdateBat).Length -lt 100) {
    Write-Host 'ERROR: update.bat missing or empty after download.'
    Write-Host 'Open: https://github.com/mon5termatt/medicat_installer/releases'
    [void][System.Console]::ReadLine()
    exit 1
}

$parentId = Get-ParentCmdProcessId

# Start updater in a new console (same as :updateprogram in Medicat_Installer.bat).
Start-Process -FilePath 'cmd.exe' -ArgumentList '/k', "`"$UpdateBat`"" -WorkingDirectory $WorkDir

# Close the legacy batch host if we can (so it does not continue into the old menu).
if ($parentId -gt 0 -and $parentId -ne $PID) {
    try {
        $parent = Get-CimInstance Win32_Process -Filter "ProcessId=$parentId" -ErrorAction SilentlyContinue
        if ($parent -and $parent.Name -match '^(cmd|powershell|pwsh)\.exe$') {
            Stop-Process -Id $parentId -Force -ErrorAction SilentlyContinue
        }
    } catch { }
}

exit 0
