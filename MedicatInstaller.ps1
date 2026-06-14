# MediCat Installer - Modern GUI Version
# PowerShell-based installer with proper error handling and logging

# Check if running as administrator, if not, request elevation
function Get-MedicatPowerShellHost {
    # Always elevate with built-in Windows PowerShell 5.1 — works even when
    # the user launched this script from pwsh (where $PSHOME\powershell.exe does not exist).
    $systemRoot = [Environment]::GetFolderPath('System')
    $windowsPs = Join-Path $systemRoot 'WindowsPowerShell\v1.0\powershell.exe'
    if (Test-Path -LiteralPath $windowsPs) {
        return (Resolve-Path -LiteralPath $windowsPs).Path
    }

    if ($PSVersionTable.PSEdition -eq 'Desktop') {
        $desktopHost = Join-Path $PSHOME 'powershell.exe'
        if (Test-Path -LiteralPath $desktopHost) {
            return (Resolve-Path -LiteralPath $desktopHost).Path
        }
    }

    $pwsh = Get-Command pwsh.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source
    if ($pwsh) {
        return $pwsh
    }

    throw "Windows PowerShell not found"
}

$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdmin = $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "Administrator privileges required. Requesting elevation..."
    
    # Get the script path and working directory
    $scriptPath = $MyInvocation.MyCommand.Path
    if (-not $scriptPath) {
        $scriptPath = $PSCommandPath
    }
    
    # Get the directory where the script is located (this is where we need to run from)
    $scriptDir = if ($scriptPath) {
        Split-Path -Parent $scriptPath
    } else {
        $PWD.Path
    }
    
    # Re-launch the script with administrator privileges
    # Set working directory to script directory to ensure all file paths work correctly
    try {
        $hostExe = Get-MedicatPowerShellHost
        $hostArgs = "-ExecutionPolicy Bypass -NoProfile -File `"$scriptPath`""

        $processStartInfo = New-Object System.Diagnostics.ProcessStartInfo
        $processStartInfo.FileName = $hostExe
        $processStartInfo.Arguments = $hostArgs
        $processStartInfo.WorkingDirectory = $scriptDir  # Set working directory to script location
        $processStartInfo.Verb = "runas"  # This triggers the UAC prompt
        $processStartInfo.UseShellExecute = $true
        [System.Diagnostics.Process]::Start($processStartInfo) | Out-Null
        exit
    } catch {
        Write-Host "Failed to elevate privileges: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "`nThis installer requires administrator privileges to install Ventoy." -ForegroundColor Yellow
        Write-Host "Please right-click and select 'Run as Administrator'." -ForegroundColor Yellow
        Read-Host "Press Enter to exit"
        exit 1
    }
}

# Ensure we're in the correct directory (script directory)
$scriptPath = $MyInvocation.MyCommand.Path
if (-not $scriptPath) {
    $scriptPath = $PSCommandPath
}
if ($scriptPath) {
    $scriptDir = Split-Path -Parent $scriptPath
    if ($scriptDir -ne $PWD.Path) {
        Set-Location $scriptDir
    }
}

$extractArchivePath = Join-Path $PSScriptRoot "Extract-Archive.ps1"
if (Test-Path $extractArchivePath) {
    . $extractArchivePath
} else {
    throw "Required file not found: Extract-Archive.ps1"
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# Load translation helper
$translationHelperPath = Join-Path $PSScriptRoot "TranslationHelper.ps1"
if (Test-Path $translationHelperPath) {
    . $translationHelperPath
    # Detect system language and load translations
    $culture = [System.Globalization.CultureInfo]::CurrentCulture
    $langCode = $culture.TwoLetterISOLanguageName
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
    Load-Translations -Language $language | Out-Null
} else {
    Write-Warning "TranslationHelper.ps1 not found. Using English (hardcoded)."
    # Create stub functions to prevent errors
    function Get-UITranslation { param($Key, $FormatArgs = @()) return "[$Key]" }
    function Get-StatusTranslation { param($Key, $FormatArgs = @()) return "[$Key]" }
    function Get-MessageTranslation { param($Key, $FormatArgs = @()) return "[$Key]" }
    function Get-TitleTranslation { param($Key, $FormatArgs = @()) return "[$Key]" }
    function Get-VentoyWarning { param($DriveLetter) return @{Title="Ventoy Installation Warning"; Message="Install Ventoy to $DriveLetter ?"} }
    function Get-VentoyNotDetected { param($DriveLetter) return @{Title="Ventoy Installation Not Detected"; Message="Warning: Could not detect Ventoy on $DriveLetter"} }
}

# Global variables
$script:LogFile = "medicat_download.log"
$script:LastLoggedProgress = $null

# Initialize log file with header
$logHeader = "========================================`r`nMediCat Installer Log - Started at $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')`r`n========================================"
try {
    Add-Content -Path $script:LogFile -Value $logHeader -ErrorAction SilentlyContinue
} catch {
    # If we can't write to log file initially, continue anyway
}
$script:DownloadPath = ""
$script:MediCatVersion = "21.12"
$script:LocalVersion = "1.0.0"
$script:DebugMode = $true  # Set to $true to enable debug logging
$script:MedicatArchiveExtractor = $null
$script:MedicatExtractWorker = $null

# Create main form
$form = New-Object System.Windows.Forms.Form
$form.Text = Get-UITranslation -Key "form_title" -FormatArgs $script:LocalVersion
$form.Size = New-Object System.Drawing.Size(800, 600)
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = "FixedDialog"
$form.MaximizeBox = $false

# Create main panel
$mainPanel = New-Object System.Windows.Forms.Panel
$mainPanel.Dock = "Fill"
$mainPanel.Padding = New-Object System.Windows.Forms.Padding(20)

# Title label
$titleLabel = New-Object System.Windows.Forms.Label
$titleLabel.Text = Get-UITranslation -Key "title_label"
$titleLabel.Font = New-Object System.Drawing.Font("Arial", 16, [System.Drawing.FontStyle]::Bold)
$titleLabel.ForeColor = [System.Drawing.Color]::DarkBlue
$titleLabel.AutoSize = $true
$titleLabel.Location = New-Object System.Drawing.Point(20, 20)

# Status label
$statusLabel = New-Object System.Windows.Forms.Label
$statusLabel.Text = Get-StatusTranslation -Key "status_ready"
$statusLabel.Font = New-Object System.Drawing.Font("Arial", 10)
$statusLabel.AutoSize = $true
$statusLabel.Location = New-Object System.Drawing.Point(20, 60)

# Progress bar
$progressBar = New-Object System.Windows.Forms.ProgressBar
$progressBar.Location = New-Object System.Drawing.Point(20, 90)
$progressBar.Size = New-Object System.Drawing.Size(740, 23)
$progressBar.Style = "Continuous"

# Log textbox
$logTextBox = New-Object System.Windows.Forms.TextBox
$logTextBox.Location = New-Object System.Drawing.Point(20, 130)
$logTextBox.Size = New-Object System.Drawing.Size(740, 300)
$logTextBox.Multiline = $true
$logTextBox.ScrollBars = "Vertical"
$logTextBox.ReadOnly = $true
$logTextBox.Font = New-Object System.Drawing.Font("Consolas", 9)

# Drive selection
$driveLabel = New-Object System.Windows.Forms.Label
$driveLabel.Text = Get-UITranslation -Key "drive_label"
$driveLabel.Location = New-Object System.Drawing.Point(20, 450)
$driveLabel.AutoSize = $true

$driveComboBox = New-Object System.Windows.Forms.ComboBox
$driveComboBox.Location = New-Object System.Drawing.Point(150, 448)
$driveComboBox.Size = New-Object System.Drawing.Size(200, 25)
$driveComboBox.DropDownStyle = "DropDownList"

# Show hard drives checkbox
$showHardDrivesCheckBox = New-Object System.Windows.Forms.CheckBox
$showHardDrivesCheckBox.Text = Get-UITranslation -Key "show_hard_drives"
$showHardDrivesCheckBox.Location = New-Object System.Drawing.Point(360, 450)
$showHardDrivesCheckBox.AutoSize = $true
$showHardDrivesCheckBox.Checked = $false

# Format checkbox
$formatCheckBox = New-Object System.Windows.Forms.CheckBox
$formatCheckBox.Text = Get-UITranslation -Key "format_checkbox"
$formatCheckBox.Location = New-Object System.Drawing.Point(20, 480)
$formatCheckBox.AutoSize = $true
$formatCheckBox.Checked = $true

# Skip Ventoy checkbox
$skipVentoyCheckBox = New-Object System.Windows.Forms.CheckBox
$skipVentoyCheckBox.Text = Get-UITranslation -Key "skip_ventoy_checkbox"
$skipVentoyCheckBox.Location = New-Object System.Drawing.Point(300, 480)
$skipVentoyCheckBox.AutoSize = $true
$skipVentoyCheckBox.Checked = $false

# Buttons
$installButton = New-Object System.Windows.Forms.Button
$installButton.Text = Get-UITranslation -Key "install_button"
$installButton.Location = New-Object System.Drawing.Point(20, 520)
$installButton.Size = New-Object System.Drawing.Size(120, 30)
$installButton.BackColor = [System.Drawing.Color]::LightGreen

$cancelButton = New-Object System.Windows.Forms.Button
$cancelButton.Text = Get-UITranslation -Key "cancel_button"
$cancelButton.Location = New-Object System.Drawing.Point(150, 520)
$cancelButton.Size = New-Object System.Drawing.Size(80, 30)

$checkFilesButton = New-Object System.Windows.Forms.Button
$checkFilesButton.Text = Get-UITranslation -Key "check_files_button"
$checkFilesButton.Location = New-Object System.Drawing.Point(240, 520)
$checkFilesButton.Size = New-Object System.Drawing.Size(120, 30)

$refreshButton = New-Object System.Windows.Forms.Button
$refreshButton.Text = Get-UITranslation -Key "refresh_button"
$refreshButton.Location = New-Object System.Drawing.Point(370, 520)
$refreshButton.Size = New-Object System.Drawing.Size(100, 30)

# Add controls to form
$mainPanel.Controls.Add($titleLabel)
$mainPanel.Controls.Add($statusLabel)
$mainPanel.Controls.Add($progressBar)
$mainPanel.Controls.Add($logTextBox)
$mainPanel.Controls.Add($driveLabel)
$mainPanel.Controls.Add($driveComboBox)
$mainPanel.Controls.Add($showHardDrivesCheckBox)
$mainPanel.Controls.Add($formatCheckBox)
$mainPanel.Controls.Add($skipVentoyCheckBox)
$mainPanel.Controls.Add($installButton)
$mainPanel.Controls.Add($cancelButton)
$mainPanel.Controls.Add($checkFilesButton)
$mainPanel.Controls.Add($refreshButton)

$form.Controls.Add($mainPanel)

# Functions
function Invoke-InstallerUi {
    param([scriptblock]$Action)

    if ($form -and $form.IsHandleCreated -and $form.InvokeRequired) {
        [void]$form.BeginInvoke($Action)
    } else {
        & $Action
    }
}

function Set-InstallButtonsEnabled {
    param([bool]$Enabled)
    Invoke-InstallerUi {
        $installButton.Enabled = $Enabled
        $cancelButton.Enabled = $Enabled
        $checkFilesButton.Enabled = $Enabled
        $refreshButton.Enabled = $Enabled
    }
}

function Write-Log {
    param($Message)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] $Message"

    try {
        Add-Content -Path $script:LogFile -Value $logMessage -ErrorAction SilentlyContinue
    } catch { }

    Invoke-InstallerUi {
        try {
            if ($logTextBox) {
                $logTextBox.AppendText("$logMessage`r`n")
                $logTextBox.ScrollToCaret()
            }
        } catch { }
    }
}

function Write-DebugLog {
    param($Message)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] DEBUG: $Message"

    try {
        Add-Content -Path $script:LogFile -Value $logMessage -ErrorAction SilentlyContinue
    } catch { }

    if ($script:DebugMode) {
        Invoke-InstallerUi {
            try {
                if ($logTextBox) {
                    $logTextBox.AppendText("$logMessage`r`n")
                    $logTextBox.ScrollToCaret()
                }
            } catch { }
        }
    }
}

function Update-Status {
    param(
        $Message,
        [bool]$Log = $true
    )
    Invoke-InstallerUi {
        $statusLabel.Text = $Message
        if ($Log) {
            $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
            $logMessage = "[$timestamp] STATUS: $Message"
            try { Add-Content -Path $script:LogFile -Value $logMessage -ErrorAction SilentlyContinue } catch { }
        }
    }
}

function Update-Progress {
    param(
        $Value,
        $Maximum = 100,
        [bool]$Log = $true
    )
    Invoke-InstallerUi {
        $progressBar.Maximum = $Maximum
        $progressBar.Value = [math]::Min($Value, $Maximum)
    }

    if (-not $Log) {
        return
    }

    $percent = if ($Maximum -gt 0) { [math]::Round(($Value / $Maximum) * 100, 1) } else { 0 }

    if (-not $script:LastLoggedProgress -or
        [math]::Abs($percent - $script:LastLoggedProgress) -ge 1 -or
        $percent -eq 0 -or
        $percent -eq 100) {
        Write-Log "PROGRESS: $percent% ($Value/$Maximum)"
        $script:LastLoggedProgress = $percent
    }
}

function Reset-MedicatExtractionProgressState {
    $script:ExtractionUiState = @{
        LastPercent = -1
        LastPercentShown = -1
        LastFile = $null
        LastUiUpdate = [datetime]::MinValue
        LastFileLog = [datetime]::MinValue
    }
    $script:LastLoggedProgress = $null
    $script:LastFolderProgressPoll = $null
    $script:ExtractionHeartbeat = $null
    $script:MedicatLastWrittenBytes = $null
    $script:MedicatLastWrittenChange = $null
}

function Get-MedicatExtractionProgressHandler {
    Reset-MedicatExtractionProgressState

    return {
        param($Percent, $Status, $BytesExtracted)

        $now = Get-Date
        $percentChanged = $Percent -ne $script:ExtractionUiState.LastPercent
        $fileChanged = $false

        if ($Status -and $Status -notin @('complete', 'starting')) {
            if ($Status -ne $script:ExtractionUiState.LastFile) {
                $fileChanged = $true
                $script:ExtractionUiState.LastFile = $Status
            }
        }

        if (-not ($percentChanged -or $fileChanged) -and $Percent -ne 100) {
            return
        }

        $elapsedMs = ($now - $script:ExtractionUiState.LastUiUpdate).TotalMilliseconds
        if ($elapsedMs -lt 150 -and $Percent -ne 0 -and $Percent -ne 100 -and -not $fileChanged) {
            return
        }
        $script:ExtractionUiState.LastUiUpdate = $now

        if ($percentChanged) {
            $script:ExtractionUiState.LastPercent = $Percent
            Update-Progress -Value $Percent -Maximum 100 -Log:$false

            if ($Percent -eq 0 -or $Percent -eq 100 -or ($Percent % 10 -eq 0)) {
                Write-Log "PROGRESS: $Percent%"
            } elseif (($now - $script:ExtractionUiState.LastFileLog).TotalSeconds -ge 5) {
                Write-DebugLog "PROGRESS: $Percent%"
                $script:ExtractionUiState.LastFileLog = $now
            }
        }

        if ($fileChanged -and ($now - $script:ExtractionUiState.LastFileLog).TotalSeconds -ge 2) {
            $script:ExtractionUiState.LastFileLog = $now
            $shortLogFile = if ($Status.Length -gt 120) { '...' + $Status.Substring($Status.Length - 117) } else { $Status }
            Write-DebugLog "Extracting ($Percent%): $shortLogFile"
        }

        $script:ExtractionUiState.LastPercentShown = $Percent
        $currentFile = $script:ExtractionUiState.LastFile

        if ($currentFile) {
            $shortFile = if ($currentFile.Length -gt 90) {
                '...' + $currentFile.Substring($currentFile.Length - 87)
            } else {
                $currentFile
            }
            Update-Status (Get-StatusTranslation -Key "extracting_file" -FormatArgs $Percent, $shortFile) -Log:$false
        } elseif ($BytesExtracted -gt 0) {
            $mb = [math]::Round($BytesExtracted / 1MB, 1)
            Update-Status (Get-StatusTranslation -Key "extracting_progress" -FormatArgs $Percent, $mb) -Log:$false
        } elseif ($Percent -ge 0) {
            Update-Status (Get-StatusTranslation -Key "extracting_progress" -FormatArgs $Percent, 0) -Log:$false
        }
    }
}

function Initialize-MedicatExtractRunspace {
    if ($script:MedicatExtractRunspace) {
        return
    }

    $script:MedicatExtractRunspace = [runspacefactory]::CreateRunspace()
    $script:MedicatExtractRunspace.Open()

    $initPs = [powershell]::Create()
    $initPs.Runspace = $script:MedicatExtractRunspace
    $root = $PSScriptRoot.Replace("'", "''")
    [void]$initPs.AddScript(". '$root\Extract-Archive.ps1'")
    $initPs.Invoke() | Out-Null
    $initPs.Dispose()
}

function Start-MedicatExtractionWorker {
    param(
        [hashtable]$Context
    )

    if ($script:MedicatExtractTimer) {
        return
    }

    if (-not (Test-MedicatCliReady -ScriptRoot $PSScriptRoot)) {
        $fail = [pscustomobject]@{
            Success = $false
            ErrorMessage = '7za.exe not found'
        }
        Complete-MedicatInstallationAfterExtract -Result $fail -Context $Context
        Set-InstallButtonsEnabled $true
        return
    }

    $script:MedicatActiveProgressHandler = Get-MedicatExtractionProgressHandler

    $destinationPath = Get-MedicatDestinationRoot -DriveLetter $Context.DestinationPath
    if (-not $destinationPath) {
        $destinationPath = Get-MedicatDestinationRoot -DriveLetter $Context.DriveLetter
    }

    $deviceId = Get-MedicatDriveDeviceId -DestinationPath $destinationPath
    $initialFreeBytes = if ($deviceId) { Get-MedicatDriveFreeBytes -DeviceId $deviceId } else { 0 }
    $sevenZipExe = Get-Medicat7ZipExecutable -ScriptRoot $PSScriptRoot
    $uncompressedTotal = Get-ArchiveUncompressedSize `
        -ArchivePath $Context.ArchivePath `
        -ScriptRoot $PSScriptRoot `
        -SevenZipExe $sevenZipExe `
        -PreferCli `
        -OnLog { param($m) Write-DebugLog $m }

    $Context.DestinationPath = $destinationPath
    $Context.UncompressedTotal = $uncompressedTotal
    $Context.InitialFreeBytes = $initialFreeBytes
    $script:MedicatExtractContext = $Context

    Write-Log "Extraction started via 7za.exe"
    Write-DebugLog "Destination: $destinationPath (initial free: $([math]::Round($initialFreeBytes / 1GB, 2)) GB)"
    Write-DebugLog "Uncompressed size: $([math]::Round($uncompressedTotal / 1MB, 2)) MB"
    & $script:MedicatActiveProgressHandler 0 "starting" 0

    Initialize-MedicatExtractRunspace

    $script:MedicatCliProgressQueue = New-Object System.Collections.Concurrent.ConcurrentQueue[object]
    $script:MedicatCliExtractPs = [powershell]::Create()
    $script:MedicatCliExtractPs.Runspace = $script:MedicatExtractRunspace

    [void]$script:MedicatCliExtractPs.AddScript({
        param($ArchivePath, $DestinationPath, $ScriptRoot, $ProgressQueue, $UncompressedTotal, $InitialFreeBytes, $LogFile)

        $onLog = {
            param($m)
            $line = "[$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')] DEBUG: $m"
            Add-Content -LiteralPath $LogFile -Value $line -ErrorAction SilentlyContinue
        }

        Invoke-MedicatCliArchiveExtraction `
            -ArchivePath $ArchivePath `
            -DestinationPath $DestinationPath `
            -ScriptRoot $ScriptRoot `
            -UncompressedTotal $UncompressedTotal `
            -InitialFreeBytes $InitialFreeBytes `
            -ProgressQueue $ProgressQueue `
            -OnLog $onLog
    })
    [void]$script:MedicatCliExtractPs.AddArgument($Context.ArchivePath)
    [void]$script:MedicatCliExtractPs.AddArgument($destinationPath)
    [void]$script:MedicatCliExtractPs.AddArgument($PSScriptRoot)
    [void]$script:MedicatCliExtractPs.AddArgument($script:MedicatCliProgressQueue)
    [void]$script:MedicatCliExtractPs.AddArgument($uncompressedTotal)
    [void]$script:MedicatCliExtractPs.AddArgument($initialFreeBytes)
    [void]$script:MedicatCliExtractPs.AddArgument($script:LogFile)

    $script:MedicatCliExtractAsync = $script:MedicatCliExtractPs.BeginInvoke()

    $script:MedicatExtractTimer = New-Object System.Windows.Forms.Timer
    $script:MedicatExtractTimer.Interval = 100
    $script:MedicatExtractTimer.Add_Tick({
        $entry = $null
        while ($script:MedicatCliProgressQueue.TryDequeue([ref]$entry)) {
            & $script:MedicatActiveProgressHandler $entry[0] $entry[1] $entry[2]
        }

        if (-not $script:MedicatCliExtractAsync.IsCompleted) {
            $ctx = $script:MedicatExtractContext
            if ($ctx -and $ctx.InitialFreeBytes -gt 0 -and $ctx.UncompressedTotal -gt 0) {
                $now = Get-Date
                if (-not $script:LastFolderProgressPoll -or ($now - $script:LastFolderProgressPoll).TotalMilliseconds -ge 2000) {
                    $script:LastFolderProgressPoll = $now
                    $written = Get-MedicatDriveWrittenBytes -DestinationPath $ctx.DestinationPath -InitialFreeBytes $ctx.InitialFreeBytes
                    if ($null -ne $written -and $written -gt 0) {
                        $pct = [int][math]::Min(99, [math]::Round(($written / $ctx.UncompressedTotal) * 100))
                        $bytes = [int64][math]::Min($ctx.UncompressedTotal, $written)
                        $file = if ($script:ExtractionUiState) { $script:ExtractionUiState.LastFile } else { $null }
                        & $script:MedicatActiveProgressHandler $pct $file $bytes
                    }
                }
            }
            return
        }

        $script:MedicatExtractTimer.Stop()
        $script:MedicatExtractTimer.Dispose()
        $script:MedicatExtractTimer = $null

        try {
            $output = $script:MedicatCliExtractPs.EndInvoke($script:MedicatCliExtractAsync)
            $result = if ($output -is [array] -and $output.Count -eq 1) { $output[0] } else { $output }

            if (-not $result) {
                $errParts = @()
                foreach ($err in $script:MedicatCliExtractPs.Streams.Error) {
                    if ($err.Exception.Message) { $errParts += $err.Exception.Message }
                }
                $errMsg = if ($errParts.Count -gt 0) { $errParts -join '; ' } else { 'Extraction returned no result' }
                $result = [pscustomobject]@{ Success = $false; ErrorMessage = $errMsg }
            }

            Complete-MedicatInstallationAfterExtract -Result $result -Context $script:MedicatExtractContext
        } catch {
            $fail = [pscustomobject]@{ Success = $false; ErrorMessage = $_.Exception.Message }
            Complete-MedicatInstallationAfterExtract -Result $fail -Context $script:MedicatExtractContext
        } finally {
            if ($script:MedicatCliExtractPs) {
                $script:MedicatCliExtractPs.Dispose()
                $script:MedicatCliExtractPs = $null
            }
            $script:MedicatCliProgressQueue = $null
            Set-InstallButtonsEnabled $true
        }
    })
    $script:MedicatExtractTimer.Start()
}

# Helper function to show MessageBox and log it
function Show-MessageBox {
    param(
        [string]$Message,
        [string]$Title = "MediCat Installer",
        [ValidateSet("OK", "OKCancel", "YesNo", "YesNoCancel")]
        [string]$Buttons = "OK",
        [ValidateSet("Information", "Warning", "Error", "Question")]
        [string]$Icon = "Information"
    )
    
    # Log the message box
    $iconLabel = switch ($Icon) {
        "Error" { "ERROR" }
        "Warning" { "WARNING" }
        "Question" { "QUESTION" }
        default { "INFO" }
    }
    Write-Log "MESSAGEBOX [$iconLabel] [$Buttons] [$Title]: $Message"
    
    # Convert string buttons to enum
    $buttonEnum = switch ($Buttons) {
        "OKCancel" { [System.Windows.Forms.MessageBoxButtons]::OKCancel }
        "YesNo" { [System.Windows.Forms.MessageBoxButtons]::YesNo }
        "YesNoCancel" { [System.Windows.Forms.MessageBoxButtons]::YesNoCancel }
        default { [System.Windows.Forms.MessageBoxButtons]::OK }
    }
    
    $iconEnum = switch ($Icon) {
        "Error" { [System.Windows.Forms.MessageBoxIcon]::Error }
        "Warning" { [System.Windows.Forms.MessageBoxIcon]::Warning }
        "Question" { [System.Windows.Forms.MessageBoxIcon]::Question }
        default { [System.Windows.Forms.MessageBoxIcon]::Information }
    }
    
    # Show the message box and log result
    $result = [System.Windows.Forms.MessageBox]::Show($Message, $Title, $buttonEnum, $iconEnum)
    $resultStr = switch ($result) {
        "Yes" { "Yes" }
        "No" { "No" }
        "OK" { "OK" }
        "Cancel" { "Cancel" }
        default { $result.ToString() }
    }
    Write-Log "MESSAGEBOX RESULT: $resultStr"
    return $result
}



function Test-MedicatVhdDisk {
    param(
        $BusType,
        [string]$FriendlyName
    )

    $busTypeStr = [string]$BusType
    if ($BusType -in 5, 15) { return $true }
    if ($busTypeStr -eq 'File Backed Virtual' -or $busTypeStr -like '*File Backed Virtual*') { return $true }
    if ($FriendlyName -like '*Virtual Disk*') { return $true }
    return $false
}

function Get-VhdDriveLetters {
    $vhdDrives = @()
    try {
        $disks = Get-CimInstance -Namespace root/Microsoft/Windows/Storage -ClassName MSFT_Disk -ErrorAction SilentlyContinue
        foreach ($disk in $disks) {
            if (-not (Test-MedicatVhdDisk -BusType $disk.BusType -FriendlyName $disk.FriendlyName)) {
                continue
            }

            $partitions = Get-CimInstance -Namespace root/Microsoft/Windows/Storage -ClassName MSFT_Partition `
                -Filter "DiskNumber = $($disk.Number)" -ErrorAction SilentlyContinue
            foreach ($partition in $partitions) {
                if ($partition.DriveLetter -and $partition.DriveLetter -ne 'C') {
                    $vhdDrives += "$($partition.DriveLetter):"
                }
            }
        }
    } catch {
        # VHD detection failure should not block the drive list
    }
    return $vhdDrives
}

function Get-DriveList {
    # Get checkbox state
    $showHardDrives = $showHardDrivesCheckBox.Checked
    
    # Get all logical disks
    $allDrives = Get-WmiObject -Class Win32_LogicalDisk | Where-Object { 
        $_.DeviceID -ne "C:" -and 
        $_.Size -gt 1GB
    } | Sort-Object DeviceID
    
    # Detect VHD/VHDX drives via Storage CIM (fast; Get-Disk takes ~15s on some systems)
    $vhdDrives = Get-VhdDriveLetters
    
    # Filter drives based on checkbox state
    # Always include: Removable (USB) drives and VHD drives
    # Conditionally include: Fixed (HDD) drives if checkbox is checked
    $drives = $allDrives | Where-Object {
        $_.DriveType -eq 2 -or  # Always include removable drives
        $_.DeviceID -in $vhdDrives -or  # Always include VHD drives
        ($showHardDrives -and $_.DriveType -eq 3)  # Include hard drives only if checkbox is checked
    }
    
    $driveComboBox.Items.Clear()
    $defaultDriveIndex = -1
    $vhdDriveIndex = -1
    $itemIndex = 0
    
    foreach ($drive in $drives) {
        $size = [math]::Round($drive.Size / 1GB, 2)
        $free = [math]::Round($drive.FreeSpace / 1GB, 2)
        
        # Determine drive type label
        $driveTypeLabel = if ($drive.DeviceID -in $vhdDrives) {
            Get-UITranslation -Key "drive_type_vhd"
        } elseif ($drive.DriveType -eq 2) {
            Get-UITranslation -Key "drive_type_usb"
        } else {
            Get-UITranslation -Key "drive_type_hdd"
        }
        
        $driveText = Get-UITranslation -Key "drive_format" -FormatArgs $drive.DeviceID, $driveTypeLabel, $free, $size
        $driveComboBox.Items.Add($driveText)
        
        # Remember first VHD drive as preferred default
        if ($drive.DeviceID -in $vhdDrives -and $vhdDriveIndex -eq -1) {
            $vhdDriveIndex = $itemIndex
        }
        
        # Remember first drive as default fallback
        if ($defaultDriveIndex -eq -1) {
            $defaultDriveIndex = $itemIndex
        }
        
        $itemIndex++
    }
    
    # Default to first VHD drive if available, otherwise use first drive
    if ($driveComboBox.Items.Count -gt 0) {
        if ($vhdDriveIndex -ge 0) {
            $driveComboBox.SelectedIndex = $vhdDriveIndex
        } else {
            $driveComboBox.SelectedIndex = $defaultDriveIndex
        }
    }
}

function Refresh-DriveList {
    <#
    .SYNOPSIS
    Refreshes the USB drive list in the combo box.
    
    .DESCRIPTION
    This function can be called from anywhere in the script to refresh the list of available USB drives.
    
    .EXAMPLE
    Refresh-DriveList
    #>
    Get-DriveList
    Write-Log "Drive list refreshed"
}

function Test-InternetConnection {
    try {
        $ping = Test-Connection -ComputerName "8.8.8.8" -Count 1 -Quiet
        if ($ping) {
            Write-Log "Internet connection verified"
            return $true
        } else {
            Write-Log "No internet connection detected"
            return $false
        }
    } catch {
        Write-Log "Error checking internet connection: $($_.Exception.Message)"
        return $false
    }
}

function Invoke-Download {
    param($Url, $OutputPath, $ExpectedSize = $null)
    
    try {
        Write-Log "Downloading: $Url"

        $parentDir = Split-Path -Parent $OutputPath
        if ($parentDir -and -not (Test-Path -LiteralPath $parentDir)) {
            New-Item -ItemType Directory -Path $parentDir -Force | Out-Null
        }

        $resolvedOutput = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
        $prevProtocol = [Net.ServicePointManager]::SecurityProtocol
        $tls = [Net.SecurityProtocolType]::Tls12
        if ([enum]::IsDefined([Net.SecurityProtocolType], 'Tls13')) {
            $tls = $tls -bor [Net.SecurityProtocolType]::Tls13
        }
        [Net.ServicePointManager]::SecurityProtocol = $tls

        try {
            Invoke-WebRequest -Uri $Url -OutFile $resolvedOutput -UseBasicParsing -TimeoutSec 600 `
                -Headers @{ 'User-Agent' = 'MediCat-Installer/1.0' }
        } finally {
            [Net.ServicePointManager]::SecurityProtocol = $prevProtocol
        }
        
        if (Test-Path -LiteralPath $resolvedOutput) {
            $actualSize = (Get-Item -LiteralPath $resolvedOutput).Length
            Write-Log "Downloaded: $resolvedOutput ($actualSize bytes)"
            
            if ($ExpectedSize -and $actualSize -ne $ExpectedSize) {
                Write-Log "WARNING: Size mismatch. Expected: $ExpectedSize, Got: $actualSize"
                return $false
            }
            return $true
        } else {
            Write-Log "ERROR: File not created: $resolvedOutput"
            return $false
        }
    } catch {
        Write-Log "ERROR downloading $Url : $($_.Exception.Message)"
        return $false
    }
}

function Test-VentoyInstalled {
    param($DriveLetter)
    
    try {
        # VTOYEFI is a 32MB FAT EFI system partition without a drive letter
        # We need to find it by checking disk partitions
        
        $driveLetterOnly = $DriveLetter.TrimEnd('\', ':')
        $partitions = Get-Partition | Where-Object { $_.DriveLetter -eq $driveLetterOnly -or $_.AccessPaths -like "*$DriveLetter*" }
        
        # Get the disk number for this drive
        $diskNumber = $null
        foreach ($partition in $partitions) {
            if ($partition.DriveLetter -eq $driveLetterOnly) {
                $diskNumber = $partition.DiskNumber
                break
            }
        }
        
        if ($diskNumber -ne $null) {
            # Check all partitions on this disk for VTOYEFI (EFI system partition)
            $allPartitions = Get-Partition -DiskNumber $diskNumber -ErrorAction SilentlyContinue
            Write-DebugLog "Checking $($allPartitions.Count) partition(s) on disk $diskNumber"
            
            foreach ($part in $allPartitions) {
                try {
                    # Check for EFI system partition type
                    # GPT type for EFI system partition: C12A7328-F81F-11D2-BA4B-00A0C93EC93B
                    $efiSystemPartitionGUID = "{C12A7328-F81F-11D2-BA4B-00A0C93EC93B}"
                    
                    # Check if this is an EFI system partition
                    $isEFISystem = $false
                    if ($part.GptType) {
                        if ($part.GptType -eq $efiSystemPartitionGUID) {
                            $isEFISystem = $true
                        }
                    }
                    
                    # Get partition size (should be around 32MB for VTOYEFI)
                    $sizeMB = [math]::Round($part.Size / 1MB, 0)
                    
                    # Try to get volume info (may fail for partitions without drive letters)
                    $volume = $null
                    try {
                        $volume = Get-Volume -Partition $part -ErrorAction SilentlyContinue
                    } catch {
                        # Expected for partitions without drive letters
                    }
                    
                    # Check characteristics of VTOYEFI partition:
                    # 1. EFI system partition type (GptType matches EFI GUID)
                    # 2. Size is approximately 32MB (check for 30-35MB range)
                    # 3. No drive letter (VTOYEFI doesn't get a letter)
                    # 4. FAT filesystem if we can detect it
                    if ($isEFISystem -or ($sizeMB -ge 30 -and $sizeMB -le 35 -and -not $part.DriveLetter)) {
                        Write-DebugLog "Partition $($part.PartitionNumber) - Size: ${sizeMB}MB, EFI System: $isEFISystem, Drive Letter: $($part.DriveLetter), GptType: $($part.GptType)"
                        
                        if ($volume) {
                            Write-DebugLog "Partition $($part.PartitionNumber) - FileSystem: $($volume.FileSystemType), Label: '$($volume.FileSystemLabel)'"
                            
                            # Check if it's FAT (FAT12, FAT16, or FAT32)
                            if ($volume.FileSystemType -and ($volume.FileSystemType -like "*FAT*" -or $volume.FileSystemType -eq "FAT")) {
                                if ($sizeMB -ge 30 -and $sizeMB -le 35) {
                                    Write-Log "Found VTOYEFI partition: ${sizeMB}MB FAT EFI system partition (no drive letter)"
                                    return $true
                                }
                            }
                        } else {
                            # No accessible volume, check based on characteristics
                            if ($isEFISystem -and $sizeMB -ge 30 -and $sizeMB -le 35 -and -not $part.DriveLetter) {
                                # EFI system partition, ~32MB, no drive letter - likely VTOYEFI
                                Write-Log "Found likely VTOYEFI partition: ${sizeMB}MB EFI system partition (no drive letter, no accessible volume)"
                                return $true
                            }
                        }
                    }
                } catch {
                    Write-DebugLog "Error checking partition $($part.PartitionNumber): $($_.Exception.Message)"
                }
            }
        }
        
        # Fallback: Check if Ventoy folder exists on the main drive partition
        $ventoyFolder = Join-Path $DriveLetter "ventoy"
        if (Test-Path $ventoyFolder) {
            Write-Log "Found ventoy folder on drive - Ventoy appears to be installed"
            return $true
        }
        
        return $false
    } catch {
        Write-Log "ERROR checking for Ventoy installation: $($_.Exception.Message)"
        return $false
    }
}

function Install-Ventoy {
    param($DriveLetter, [switch]$Upgrade)
    
    try {
        Write-Log "Checking Ventoy version..."
        Update-Status (Get-StatusTranslation -Key "checking_ventoy")
        
        # Get latest Ventoy version from GitHub API
        $ventoyApiUrl = "https://api.github.com/repos/ventoy/ventoy/git/refs/tag"
        Write-Log "Fetching latest Ventoy version from GitHub..."
        
        try {
            $refs = Invoke-RestMethod -Uri $ventoyApiUrl -UseBasicParsing
            $latestTag = $refs[-1].ref -replace 'refs/tags/', ''
            $ventoyVersion = $latestTag.Substring($latestTag.Length - 6)  # Get last 6 chars (e.g., "1.0.91")
            Write-Log "Latest Ventoy version: v$ventoyVersion"
        } catch {
            Write-Log "ERROR: Could not fetch Ventoy version: $($_.Exception.Message)"
            return $false
        }
        
        $ventoyDir = ".\Ventoy2Disk"
        $ventoyExe = Join-Path $ventoyDir "Ventoy2Disk.exe"
        $localVersion = $null
        
        # Check if Ventoy2Disk exists and get local version
        if (Test-Path $ventoyDir) {
            $versionFile = Join-Path $ventoyDir "ventoy\version"
            if (Test-Path $versionFile) {
                $localVersion = (Get-Content $versionFile -Raw).Trim()
                Write-Log "Local Ventoy version: $localVersion"
            }
        }
        
        # Download Ventoy if it doesn't exist or version is outdated
        if (-not (Test-Path $ventoyExe) -or $localVersion -ne $ventoyVersion) {
            if ($localVersion -ne $ventoyVersion) {
                Write-Log "Update found. Downloading latest Ventoy v$ventoyVersion..."
            } else {
                Write-Log "Ventoy not found. Downloading latest Ventoy v$ventoyVersion..."
            }
            
            Update-Status (Get-StatusTranslation -Key "downloading_ventoy" -FormatArgs $ventoyVersion)
            
            $ventoyZipUrl = "https://github.com/ventoy/Ventoy/releases/download/v$ventoyVersion/ventoy-$ventoyVersion-windows.zip"
            $ventoyZip = ".\ventoy.zip"
            
            # Download Ventoy zip
            if (-not (Invoke-Download -Url $ventoyZipUrl -OutputPath $ventoyZip)) {
                Write-Log "ERROR: Failed to download Ventoy"
                return $false
            }
            
            # Extract Ventoy zip (lib/ SevenZipSharp or 7za.exe fallback)
            Update-Status (Get-StatusTranslation -Key "extracting_ventoy")
            Write-Log "Extracting Ventoy archive..."

            $extractResult = Invoke-MedicatArchiveExtraction `
                -ArchivePath (Resolve-Path $ventoyZip).Path `
                -DestinationPath $PWD.Path `
                -AllowCliFallback `
                -ScriptRoot $PSScriptRoot `
                -OnLog { param($m) Write-DebugLog $m } `
                -OnProgress (Get-MedicatExtractionProgressHandler)

            if (-not $extractResult.Success) {
                Write-Log "ERROR: Failed to extract Ventoy archive: $($extractResult.ErrorMessage)"
                Remove-Item $ventoyZip -ErrorAction SilentlyContinue
                return $false
            }

            Write-Log "Ventoy archive extracted via $($extractResult.Method)"
            
            # Remove old Ventoy2Disk directory if it exists
            if (Test-Path $ventoyDir) {
                Remove-Item $ventoyDir -Recurse -Force -ErrorAction SilentlyContinue
            }
            
            # Rename extracted folder
            $extractedDir = ".\ventoy-$ventoyVersion"
            if (Test-Path $extractedDir) {
                Rename-Item -Path $extractedDir -NewName "Ventoy2Disk" -Force
                Write-Log "Ventoy extracted and renamed to Ventoy2Disk"
            }
            
            # Clean up zip
            Remove-Item $ventoyZip -ErrorAction SilentlyContinue
            Write-Log "Ventoy download and extraction complete"
        } else {
            Write-Log "Local Ventoy version matches latest version. Skipping download."
        }
        
        # Verify Ventoy2Disk.exe exists
        if (-not (Test-Path $ventoyExe)) {
            Write-Log "ERROR: Ventoy2Disk.exe not found after download/extraction"
            return $false
        }
        
        # Install Ventoy to the drive
        Write-Log "Installing Ventoy to $DriveLetter"
        Update-Status (Get-StatusTranslation -Key "installing_ventoy" -FormatArgs $DriveLetter)
        
        # Show warning message
        $ventoyWarning = Get-VentoyWarning -DriveLetter $DriveLetter
        $warningResult = Show-MessageBox -Message $ventoyWarning.Message -Title $ventoyWarning.Title -Buttons "YesNo" -Icon "Warning"
        
        if ($warningResult -eq "No") {
            Write-Log "User cancelled Ventoy installation"
            return $false
        }
        
        # Verify drive exists before installation
        Write-DebugLog "Verifying drive $DriveLetter exists..."
        $driveInfo = Get-WmiObject -Class Win32_LogicalDisk -Filter "DeviceID='$DriveLetter'" -ErrorAction SilentlyContinue
        if (-not $driveInfo) {
            Write-Log "ERROR: Drive $DriveLetter does not exist or is not accessible"
            return $false
        }
        Write-DebugLog "Drive $DriveLetter found - Type: $($driveInfo.DriveType), Size: $([math]::Round($driveInfo.Size / 1GB, 2))GB, Free: $([math]::Round($driveInfo.FreeSpace / 1GB, 2))GB"
        
        # Run Ventoy2Disk in CLI mode
        # VTOYCLI /I = Install (destructive)
        # VTOYCLI /U = Upgrade (non-destructive, preserves data)
        # /Drive:X: = Target drive (format: letter + colon, e.g., "F:")
        # /NOUSBCheck = Skip USB check (only for install, not needed for upgrade)
        # For testing, we'll use default options (no GPT/SecureBoot flags)
        # Ensure drive letter format is correct (should already be "F:" from Substring(0, 2))
        $driveParam = $DriveLetter.TrimEnd('\')
        Write-DebugLog "Drive parameter formatted as: '$driveParam'"
        
        if ($Upgrade) {
            Write-Log "Running Ventoy in UPGRADE mode (non-destructive)"
            $ventoyArgs = "VTOYCLI /U /Drive:$driveParam"
        } else {
            Write-Log "Running Ventoy in INSTALL mode (destructive)"
            $ventoyArgs = "VTOYCLI /I /Drive:$driveParam /NOUSBCheck"
        }
        
        Write-DebugLog "Ventoy2Disk.exe path: $ventoyExe"
        Write-DebugLog "Working directory: $ventoyDir"
        Write-DebugLog "Full command: `"$ventoyExe`" $ventoyArgs"
        Write-Log "Running: Ventoy2Disk.exe $ventoyArgs"
        
        # Capture stdout and stderr for debugging
        $stdoutFile = Join-Path $env:TEMP "ventoy_stdout_$([System.Guid]::NewGuid().ToString().Substring(0,8)).txt"
        $stderrFile = Join-Path $env:TEMP "ventoy_stderr_$([System.Guid]::NewGuid().ToString().Substring(0,8)).txt"
        
        try {
            $ventoyProcess = Start-Process -FilePath $ventoyExe -ArgumentList $ventoyArgs -Wait -PassThru -NoNewWindow -WorkingDirectory $ventoyDir -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile
            
            Write-DebugLog "Ventoy2Disk process completed"
            Write-DebugLog "Exit code: $($ventoyProcess.ExitCode)"
            
            # Read and log output
            if (Test-Path $stdoutFile) {
                $stdout = Get-Content $stdoutFile -ErrorAction SilentlyContinue
                if ($stdout) {
                    Write-DebugLog "Ventoy2Disk STDOUT:"
                    foreach ($line in $stdout) {
                        Write-Log "  $line"
                    }
                }
            }
            
            if (Test-Path $stderrFile) {
                $stderr = Get-Content $stderrFile -ErrorAction SilentlyContinue
                if ($stderr) {
                    Write-DebugLog "Ventoy2Disk STDERR:"
                    foreach ($line in $stderr) {
                        Write-Log "  $line"
                    }
                }
            }
            
            if ($ventoyProcess.ExitCode -eq 0) {
                Write-Log "Ventoy installed successfully to $DriveLetter"
                return $true
            } else {
                Write-Log "ERROR: Ventoy installation failed with exit code: $($ventoyProcess.ExitCode)"
                Write-Log "Check the debug output above for details"
                return $false
            }
        } catch {
            Write-Log "ERROR: Exception while running Ventoy2Disk: $($_.Exception.Message)"
            Write-Log "Exception type: $($_.Exception.GetType().Name)"
            if ($_.Exception.InnerException) {
                Write-Log "Inner exception: $($_.Exception.InnerException.Message)"
            }
            return $false
        } finally {
            # Clean up temp files
            Remove-Item $stdoutFile -ErrorAction SilentlyContinue
            Remove-Item $stderrFile -ErrorAction SilentlyContinue
        }
        
    } catch {
        Write-Log "ERROR during Ventoy installation: $($_.Exception.Message)"
        Write-Log "Stack trace: $($_.ScriptStackTrace)"
        return $false
    }
}

function Complete-MedicatInstallationAfterExtract {
    param(
        $Result,
        [hashtable]$Context
    )

    $driveLetter = $Context.DriveLetter
    $startTime = $Context.StartTime

    if (-not $Result -or -not $Result.Success) {
        $errorText = if ($Result -and $Result.ErrorMessage) { $Result.ErrorMessage } else { 'Extraction returned no result' }
        Write-Log "ERROR: Extraction failed: $errorText"
        Show-MessageBox -Message (Get-MessageTranslation -Key "extraction_failed" -FormatArgs $errorText) -Title (Get-TitleTranslation -Key "extraction_failed") -Icon "Error"
        return
    }

    $duration = ((Get-Date) - $startTime).TotalMinutes
    Write-Log "Extraction completed via $($Result.Method) in $([math]::Round($duration, 2)) minutes"
    Write-Log "MediCat archive extracted successfully"
    Update-Progress -Value 100 -Maximum 100
    Update-Status (Get-StatusTranslation -Key "extraction_success")
    Update-Status (Get-StatusTranslation -Key "copying_files")
    $finalFiles = @(
        @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/icon.ico"; Path="$driveLetter/autorun.ico"},
        @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/CheckFiles.bat"; Path="$driveLetter/CheckFiles.bat"}
    )

    foreach ($file in $finalFiles) {
        if (Invoke-Download -Url $file.Url -OutputPath $file.Path) {
            Write-Log "[GOOD] $($file.Path)"
        } else {
            Write-Log "[BAD] $($file.Path)"
        }
    }

    Update-Status (Get-StatusTranslation -Key "installation_complete")
    Write-Log "MediCat installation completed successfully"
    Show-MessageBox -Message (Get-MessageTranslation -Key "installation_complete" -FormatArgs $driveLetter) -Title (Get-TitleTranslation -Key "installation_complete") -Icon "Information"
}

function Start-MediatInstallation {
    Set-InstallButtonsEnabled $false
    $asyncExtractStarted = $false

    try {
        # Initialize log
        Write-Log "MediCat Installation Started"
        
        # Check internet
        if (-not (Test-InternetConnection)) {
            Show-MessageBox -Message (Get-MessageTranslation -Key "no_internet") -Title (Get-TitleTranslation -Key "no_internet") -Icon "Warning"
            return
        }
        
        # Get selected drive
        $selectedDrive = $driveComboBox.SelectedItem
        if (-not $selectedDrive) {
            Show-MessageBox -Message (Get-MessageTranslation -Key "no_drive_selected") -Title (Get-TitleTranslation -Key "no_drive_selected") -Icon "Warning"
            return
        }
        
        $driveLetter = $selectedDrive.Substring(0, 2)
        Write-Log "Selected drive: $driveLetter"
        
        # Check for MediCat files
        Update-Status (Get-StatusTranslation -Key "checking_files")
        $medicatFileName = "MediCat.USB.v$script:MediCatVersion.7z"
        $medicatFile = Join-Path $PWD $medicatFileName
        if (-not (Test-Path $medicatFile)) {
            Write-Log "MediCat file not found: $medicatFile"
            Show-MessageBox -Message (Get-MessageTranslation -Key "file_not_found" -FormatArgs $medicatFileName) -Title (Get-TitleTranslation -Key "file_not_found") -Icon "Warning"
            return
        }
        Write-Log "Found MediCat archive: $medicatFile"
        
        # Check install options
        $shouldFormat = $formatCheckBox.Checked
        $skipVentoy = $skipVentoyCheckBox.Checked
        Write-Log "Format checkbox is: $(if ($shouldFormat) { 'Checked' } else { 'Unchecked' })"
        Write-Log "Skip Ventoy checkbox is: $(if ($skipVentoy) { 'Checked' } else { 'Unchecked' })"

        if ($skipVentoy) {
            Write-Log "Skipping Ventoy install/upgrade (user option)"
            if ($shouldFormat) {
                Update-Status (Get-StatusTranslation -Key "formatting_drive")
                Write-Log "Formatting $driveLetter to NTFS with label 'Medicat' (Ventoy skipped)..."
                try {
                    $driveForFormat = $driveLetter.TrimEnd('\', ':') + ":"
                    $formatArgs = "$driveForFormat /FS:NTFS /X /Q /V:Medicat /Y"
                    Write-Log "Running: format.com $formatArgs"
                    $formatProcess = Start-Process -FilePath "format.com" -ArgumentList $formatArgs -Wait -PassThru -NoNewWindow
                    if ($formatProcess.ExitCode -eq 0) {
                        Write-Log "Drive formatted successfully to NTFS"
                    } else {
                        Write-Log "WARNING: Format command returned exit code: $($formatProcess.ExitCode)"
                    }
                } catch {
                    Write-Log "WARNING: Could not format drive: $($_.Exception.Message)"
                }
            }
        } elseif ($shouldFormat) {
            # Format is checked - do fresh install
            Write-Log "Formatting is enabled - performing fresh Ventoy installation"
            
            # Install Ventoy (fresh install)
            if (-not (Install-Ventoy -DriveLetter $driveLetter)) {
                Write-Log "ERROR: Ventoy installation failed"
                Show-MessageBox -Message (Get-MessageTranslation -Key "ventoy_install_failed") -Title (Get-TitleTranslation -Key "ventoy_install_failed") -Icon "Error"
                return
            }
            
            # Format drive to NTFS after Ventoy installation
            Update-Status (Get-StatusTranslation -Key "formatting_drive")
            Write-Log "Formatting $driveLetter to NTFS with label 'Medicat'..."
            
            try {
                # Use format.com to format the drive (matching batch script behavior)
                # /FS:NTFS = NTFS file system
                # /X = Force dismount if needed
                # /Q = Quick format
                # /V:Medicat = Volume label
                # /Y = Assume yes to all prompts
                $driveForFormat = $driveLetter.TrimEnd('\', ':') + ":"
                $formatArgs = "$driveForFormat /FS:NTFS /X /Q /V:Medicat /Y"
                
                Write-Log "Running: format.com $formatArgs"
                $formatProcess = Start-Process -FilePath "format.com" -ArgumentList $formatArgs -Wait -PassThru -NoNewWindow
                
                if ($formatProcess.ExitCode -eq 0) {
                    Write-Log "Drive formatted successfully to NTFS"
                } else {
                    Write-Log "WARNING: Format command returned exit code: $($formatProcess.ExitCode)"
                    # Don't fail installation, continue anyway (Ventoy may have already formatted it)
                }
            } catch {
                Write-Log "WARNING: Could not format drive: $($_.Exception.Message)"
                Write-Log "Continuing installation - Ventoy may have already formatted the drive"
                # Don't fail installation, Ventoy installation may have already formatted the drive
            }
        } else {
            # Format is unchecked - check if Ventoy is installed and do upgrade
            Write-Log "Formatting is disabled - checking if Ventoy is already installed..."
            Update-Status (Get-StatusTranslation -Key "checking_existing")
            
            $ventoyInstalled = Test-VentoyInstalled -DriveLetter $driveLetter
            
            if (-not $ventoyInstalled) {
                # Ventoy not found - warn user
                Write-Log "WARNING: VTOYEFI partition not found - Ventoy may not be installed"
                
                $ventoyWarning = Get-VentoyNotDetected -DriveLetter $driveLetter
                $confirmResult = Show-MessageBox -Message $ventoyWarning.Message -Title $ventoyWarning.Title -Buttons "YesNo" -Icon "Warning"
                
                if ($confirmResult -eq "No") {
                    Write-Log "User cancelled installation"
                    return
                }
                
                Write-Log "User confirmed to proceed with upgrade despite no detected installation"
            } else {
                Write-Log "Ventoy installation detected - proceeding with non-destructive upgrade"
            }
            
            # Do non-destructive upgrade
            if (-not (Install-Ventoy -DriveLetter $driveLetter -Upgrade)) {
                Write-Log "ERROR: Ventoy upgrade failed"
                Show-MessageBox -Message (Get-MessageTranslation -Key "ventoy_upgrade_failed") -Title (Get-TitleTranslation -Key "ventoy_upgrade_failed") -Icon "Error"
                return
            }
            
            Write-Log "Skipping format step (format checkbox is unchecked)"
        }
        
        # Extract MediCat files
        Update-Status (Get-StatusTranslation -Key "extracting_archive")
        Write-Log "Extracting MediCat files from $medicatFile to $driveLetter"
        
        $outputDir = Get-MedicatDestinationRoot -DriveLetter $driveLetter
        $startTime = Get-Date
        Update-Progress -Value 0 -Maximum 100

        $asyncExtractStarted = $true
        Start-MedicatExtractionWorker -Context @{
            ArchivePath = $medicatFile
            DestinationPath = $outputDir
            DriveLetter = $driveLetter
            StartTime = $startTime
        }
        return

    } catch {
        Write-Log "ERROR: $($_.Exception.Message)"
        Show-MessageBox -Message (Get-MessageTranslation -Key "installation_error" -FormatArgs $_.Exception.Message) -Title (Get-TitleTranslation -Key "installation_error") -Icon "Error"
    } finally {
        if (-not $asyncExtractStarted) {
            Set-InstallButtonsEnabled $true
        }
    }
}

# Event handlers
$installButton.Add_Click({
    Start-MediatInstallation
})

$cancelButton.Add_Click({
    $form.Close()
})

function Start-FileCheck {
    param($DriveLetter)
    
    try {
        $installButton.Enabled = $false
        $cancelButton.Enabled = $false
        $checkFilesButton.Enabled = $false
        
        Update-Status (Get-StatusTranslation -Key "checking_usb")
        Write-Log "Starting file check on $DriveLetter"
        
        # Download the MD5 file
        $md5File = "$DriveLetter\MedicatFiles.md5"
        $md5Url = "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/MedicatFiles.md5"
        
        Write-Log "Downloading MD5 file..."
        if (Invoke-Download -Url $md5Url -OutputPath $md5File) {
            Write-Log "MD5 file downloaded successfully"
            Write-Log "Parsing MD5 file and verifying files..."
            Update-Status (Get-StatusTranslation -Key "verifying_files")
            
            # Read and parse MD5 file
            $md5Content = Get-Content $md5File
            $totalFiles = 0
            $verifiedFiles = 0
            $failedFiles = @()
            
            foreach ($line in $md5Content) {
                # Skip empty lines and comments
                if ([string]::IsNullOrWhiteSpace($line) -or $line.Trim().StartsWith(';')) {
                    continue
                }
                
                # Parse MD5 format: MD5_HASH  FILENAME or FILENAME  MD5_HASH
                # Try both formats
                $parts = $line.Trim() -split '\s+', 2
                
                if ($parts.Count -eq 2) {
                    $expectedHash = $parts[0].ToLower()
                    $fileName = $parts[1].Trim().TrimStart('*').Trim()
                    
                    $totalFiles++
                    
                    # Handle paths that might be relative or absolute
                    $filePath = $fileName
                    if (-not [System.IO.Path]::IsPathRooted($fileName)) {
                        $filePath = Join-Path $DriveLetter $fileName
                    }
                    
                    # Check if file exists
                    if (Test-Path $filePath) {
                        # Calculate MD5 hash using PowerShell
                        try {
                            $actualHash = (Get-FileHash -Path $filePath -Algorithm MD5).Hash.ToLower()
                            
                            if ($actualHash -eq $expectedHash) {
                                $verifiedFiles++
                                Write-Log "[OK] $fileName"
                            } else {
                                $failedFiles += $fileName
                                Write-Log "[FAIL] $fileName - Hash mismatch"
                            }
                        } catch {
                            $failedFiles += $fileName
                            Write-Log "[FAIL] $fileName - Error calculating hash: $($_.Exception.Message)"
                        }
                    } else {
                        $failedFiles += $fileName
                        Write-Log "[FAIL] $fileName - File not found"
                    }
                    
                    # Update progress
                    $progress = [math]::Round(($verifiedFiles + $failedFiles.Count) / $totalFiles * 100)
                    Update-Progress -Value ($verifiedFiles + $failedFiles.Count) -Maximum $totalFiles
                }
            }
            
            # Report results
            Write-Log "Verification complete: $verifiedFiles/$totalFiles files verified"
            
            if ($failedFiles.Count -eq 0) {
                Write-Log "All files verified successfully!"
                Update-Status (Get-StatusTranslation -Key "verification_complete")
                Show-MessageBox -Message (Get-MessageTranslation -Key "verification_complete" -FormatArgs $totalFiles) -Title (Get-TitleTranslation -Key "verification_complete") -Icon "Information"
            } else {
                Write-Log "WARNING: $($failedFiles.Count) file(s) failed verification:"
                foreach ($failed in $failedFiles) {
                    Write-Log "  - $failed"
                }
                
                # Write failed files to text file
                $failedFilesPath = Join-Path $PSScriptRoot "failed_files.txt"
                try {
                    $failedFiles | Out-File -FilePath $failedFilesPath -Encoding UTF8
                    Write-Log "Failed files list written to: $failedFilesPath"
                } catch {
                    Write-Log "ERROR: Could not write failed files list: $($_.Exception.Message)"
                }
                
                Update-Status (Get-StatusTranslation -Key "verification_failed" -FormatArgs $failedFiles.Count)
                
                # Ask if user wants to re-extract missing files
                $result = Show-MessageBox -Message (Get-MessageTranslation -Key "verification_failed" -FormatArgs $failedFiles.Count) -Title (Get-TitleTranslation -Key "verification_failed") -Buttons "YesNo" -Icon "Question"
                
                if ($result -eq "Yes") {
                    Write-Log "User requested re-extraction of failed files"
                    $reExtractResult = Start-ReExtractFiles -DriveLetter $DriveLetter -FailedFiles $failedFiles
                    if ($reExtractResult) {
                        Write-Log "Re-extraction completed successfully"
                        Show-MessageBox -Message (Get-MessageTranslation -Key "re_extract_complete") -Title (Get-TitleTranslation -Key "re_extraction_complete") -Icon "Information"
                    } else {
                        Write-Log "Re-extraction failed"
                    }
                }
            }
            
            # Clean up MD5 file
            if (Test-Path $md5File) {
                Remove-Item $md5File -Force
                Write-Log "Cleaned up temporary MD5 file"
            }
        } else {
            Write-Log "ERROR: Failed to download MD5 file"
            Update-Status (Get-StatusTranslation -Key "verification_failed")
            Show-MessageBox -Message (Get-MessageTranslation -Key "download_failed") -Title (Get-TitleTranslation -Key "download_failed") -Icon "Error"
        }
        
    } catch {
        Write-Log "ERROR during file check: $($_.Exception.Message)"
        Update-Status (Get-StatusTranslation -Key "verification_failed")
        Show-MessageBox -Message (Get-MessageTranslation -Key "verification_error" -FormatArgs $_.Exception.Message) -Title (Get-TitleTranslation -Key "verification_error") -Icon "Error"
    } finally {
        $installButton.Enabled = $true
        $cancelButton.Enabled = $true
        $checkFilesButton.Enabled = $true
        Update-Status (Get-StatusTranslation -Key "status_ready")
    }
}

function Start-ReExtractFiles {
    param($DriveLetter, $FailedFiles)
    
    try {
        Update-Status (Get-StatusTranslation -Key "re_extracting")
        Write-Log "Starting re-extraction of $($FailedFiles.Count) files"
        
        $archiveFile = Join-Path $PWD "MediCat.USB.v$script:MediCatVersion.7z"
        
        if (-not (Test-Path $archiveFile)) {
            Write-Log "Archive not found in current directory, prompting user to select file..."
            Update-Status (Get-StatusTranslation -Key "select_archive")
            
            $fileDialog = New-Object System.Windows.Forms.OpenFileDialog
            $fileDialog.Filter = "7z Archive|*.7z|All Files|*.*"
            $fileDialog.Title = Get-TitleTranslation -Key "file_dialog_title" -FormatArgs $script:MediCatVersion
            $fileDialog.CheckFileExists = $true
            $fileDialog.Multiselect = $false
            
            if ($fileDialog.ShowDialog() -eq "OK") {
                $archiveFile = $fileDialog.FileName
                Write-Log "User selected archive: $archiveFile"
            } else {
                Write-Log "User cancelled archive selection"
                Show-MessageBox -Message (Get-MessageTranslation -Key "selection_cancelled") -Title (Get-TitleTranslation -Key "selection_cancelled") -Icon "Warning"
                return $false
            }
        }
        
        if (-not (Test-Path $archiveFile)) {
            Write-Log "ERROR: Source archive not found: $archiveFile"
            Show-MessageBox -Message (Get-MessageTranslation -Key "archive_not_found" -FormatArgs $archiveFile) -Title (Get-TitleTranslation -Key "archive_not_found") -Icon "Error"
            return $false
        }
        
        Write-Log "Found archive: $archiveFile"
        
        $normalizedFiles = @()
        foreach ($failedFile in $FailedFiles) {
            $fileToExtract = $failedFile.Trim()
            if ($fileToExtract -match '^[A-Z]:\\(.+)') {
                $fileToExtract = $matches[1]
            }
            $fileToExtract = $fileToExtract.TrimStart('\', '/')
            if ($fileToExtract) {
                $normalizedFiles += $fileToExtract
            }
        }
        
        Write-Log "Extracting $($normalizedFiles.Count) files..."
        Update-Status "Extracting $($normalizedFiles.Count) files (this may take a while)..."
        Update-Progress -Value 0 -Maximum 100
        
        $outputDir = Get-MedicatDestinationRoot -DriveLetter $DriveLetter
        $extractResult = Invoke-MedicatArchiveExtraction `
            -ArchivePath $archiveFile `
            -DestinationPath $outputDir `
            -FileList $normalizedFiles `
            -AllowCliFallback `
            -ScriptRoot $PSScriptRoot `
            -OnLog { param($m) Write-DebugLog $m } `
            -OnProgress (Get-MedicatExtractionProgressHandler)
        
        if (-not $extractResult.Success) {
            Write-Log "ERROR: Re-extraction failed: $($extractResult.ErrorMessage)"
            Show-MessageBox -Message (Get-MessageTranslation -Key "re_extraction_error" -FormatArgs $extractResult.ErrorMessage) -Title (Get-TitleTranslation -Key "re_extraction_error") -Icon "Error"
            return $false
        }
        
        $extractedCount = 0
        $failedExtract = @()
        foreach ($file in $normalizedFiles) {
            $fullPath = Join-Path $outputDir $file
            if (Test-Path $fullPath) {
                $extractedCount++
            } else {
                $failedExtract += $file
            }
        }
        
        Update-Progress -Value 100 -Maximum 100
        Write-Log "Re-extraction complete via $($extractResult.Method): $extractedCount/$($normalizedFiles.Count) files verified on disk"
        
        if ($failedExtract.Count -gt 0) {
            Write-Log "WARNING: Could not extract $($failedExtract.Count) file(s):"
            foreach ($failed in $failedExtract) {
                Write-Log "  - $failed"
            }
            Show-MessageBox -Message (Get-MessageTranslation -Key "re_extract_failed" -FormatArgs $extractedCount, $FailedFiles.Count, $failedExtract.Count) -Title (Get-TitleTranslation -Key "re_extraction_failed") -Icon "Warning"
            return $false
        }
        
        Update-Status (Get-StatusTranslation -Key "re_extracting")
        return $true
        
    } catch {
        Write-Log "ERROR during re-extraction: $($_.Exception.Message)"
        Update-Status (Get-StatusTranslation -Key "re_extracting")
        Show-MessageBox -Message (Get-MessageTranslation -Key "re_extraction_error" -FormatArgs $_.Exception.Message) -Title (Get-TitleTranslation -Key "re_extraction_error") -Icon "Error"
        return $false
    }
}

$checkFilesButton.Add_Click({
    $selectedDrive = $driveComboBox.SelectedItem
    if ($selectedDrive) {
        $driveLetter = $selectedDrive.Substring(0, 2)
        Start-FileCheck -DriveLetter $driveLetter
    } else {
        Show-MessageBox -Message (Get-MessageTranslation -Key "no_drive_for_check") -Title (Get-TitleTranslation -Key "no_drive_for_check") -Icon "Warning"
    }
})

$refreshButton.Add_Click({
    Refresh-DriveList
})

# When checkbox is toggled, refresh the drive list
$showHardDrivesCheckBox.Add_CheckedChanged({
    Refresh-DriveList
})

$form.Add_FormClosing({
    param($sender, $e)
    $e.Cancel = $false
})

# Add keyboard shortcut handler for Ctrl+C
$form.Add_KeyDown({
    param($sender, $e)
    if ($e.Control -and $e.KeyCode -eq [System.Windows.Forms.Keys]::C) {
        $form.Close()
    }
})

# Enable keyboard focus for the form
$form.KeyPreview = $true

$form.Add_Shown({
    Write-Log "MediCat Installer v$script:LocalVersion Started"
    Write-Log "PowerShell host: $($PSVersionTable.PSEdition) $($PSVersionTable.PSVersion)"
    Write-Log "Running with Administrator privileges - Ventoy installation enabled"
    Update-Status (Get-StatusTranslation -Key "loading_drives")
    Get-DriveList

    if (Test-MedicatCliReady -ScriptRoot $PSScriptRoot) {
        Write-Log "Extraction ready (7za.exe)"
    } elseif (Test-MedicatLibReady -ScriptRoot $PSScriptRoot) {
        Write-Log "Extraction libraries ready (lib/ SevenZipSharp)"
    } else {
        Write-Log "WARNING: lib/ extraction libraries incomplete - will fall back to 7za.exe if available"
    }

    Update-Status (Get-StatusTranslation -Key "status_ready")
})

# Show form
$form.ShowDialog()
