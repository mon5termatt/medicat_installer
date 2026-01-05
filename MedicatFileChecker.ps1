# MediCat File Checker - Standalone File Verification Tool
# PowerShell-based file checker with MD5 verification and re-extraction support

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
}

# Global variables
$script:LogFile = "medicat_filecheck.log"
$script:LastLoggedProgress = $null

# Initialize log file with header
$logHeader = "========================================`r`nMediCat File Checker Log - Started at $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')`r`n========================================"
try {
    Add-Content -Path $script:LogFile -Value $logHeader -ErrorAction SilentlyContinue
} catch {
    # If we can't write to log file initially, continue anyway
}
$script:MediCatVersion = "21.12"
$script:LocalVersion = "1.0.0"
$script:DebugMode = $true  # Set to $true to enable debug logging

# Create main form
$form = New-Object System.Windows.Forms.Form
$form.Text = "MediCat File Checker v$script:LocalVersion"
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
$titleLabel.Text = "MediCat File Checker"
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

# Buttons
$checkFilesButton = New-Object System.Windows.Forms.Button
$checkFilesButton.Text = Get-UITranslation -Key "check_files_button"
$checkFilesButton.Location = New-Object System.Drawing.Point(20, 480)
$checkFilesButton.Size = New-Object System.Drawing.Size(120, 30)
$checkFilesButton.BackColor = [System.Drawing.Color]::LightGreen

$closeButton = New-Object System.Windows.Forms.Button
$closeButton.Text = Get-UITranslation -Key "cancel_button"
$closeButton.Location = New-Object System.Drawing.Point(150, 480)
$closeButton.Size = New-Object System.Drawing.Size(80, 30)

$refreshButton = New-Object System.Windows.Forms.Button
$refreshButton.Text = Get-UITranslation -Key "refresh_button"
$refreshButton.Location = New-Object System.Drawing.Point(240, 480)
$refreshButton.Size = New-Object System.Drawing.Size(100, 30)

# Add controls to form
$mainPanel.Controls.Add($titleLabel)
$mainPanel.Controls.Add($statusLabel)
$mainPanel.Controls.Add($progressBar)
$mainPanel.Controls.Add($logTextBox)
$mainPanel.Controls.Add($driveLabel)
$mainPanel.Controls.Add($driveComboBox)
$mainPanel.Controls.Add($showHardDrivesCheckBox)
$mainPanel.Controls.Add($checkFilesButton)
$mainPanel.Controls.Add($closeButton)
$mainPanel.Controls.Add($refreshButton)

$form.Controls.Add($mainPanel)

# Functions
function Write-Log {
    param($Message)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] $Message"
    
    # Update UI if form is available
    try {
        if ($logTextBox -and $logTextBox.IsHandleCreated) {
            $form.Invoke([System.Action[string]]{
                param($msg)
                $logTextBox.AppendText("$msg`r`n")
                $logTextBox.ScrollToCaret()
            }, $logMessage) | Out-Null
        }
    } catch {
        # UI not available, continue to file logging
    }
    
    # Always log to file
    try {
        Add-Content -Path $script:LogFile -Value $logMessage -ErrorAction SilentlyContinue
    } catch {
        # If logging fails, try to continue
    }
}

function Write-DebugLog {
    param($Message)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] DEBUG: $Message"
    
    # Always log debug messages to file (not just when debug mode is on)
    try {
        Add-Content -Path $script:LogFile -Value $logMessage -ErrorAction SilentlyContinue
    } catch {
        # If logging fails, try to continue
    }
    
    # Only show in UI if debug mode is enabled
    if ($script:DebugMode) {
        try {
            if ($logTextBox -and $logTextBox.IsHandleCreated) {
                $form.Invoke([System.Action[string]]{
                    param($msg)
                    $logTextBox.AppendText("$msg`r`n")
                    $logTextBox.ScrollToCaret()
                }, $logMessage) | Out-Null
            }
        } catch {
            # UI not available, continue
        }
    }
}

function Update-Status {
    param($Message)
    $statusLabel.Text = $Message
    $form.Refresh()
    # Log status updates to file
    Write-Log "STATUS: $Message"
}

function Update-Progress {
    param($Value, $Maximum = 100)
    $progressBar.Maximum = $Maximum
    $progressBar.Value = $Value
    $form.Refresh()
    
    # Calculate percentage
    $percent = if ($Maximum -gt 0) { [math]::Round(($Value / $Maximum) * 100, 1) } else { 0 }
    
    # Log progress updates to file (but throttle to avoid too many log entries)
    # Only log if percentage changed by at least 1% or is 0/100
    if (-not $script:LastLoggedProgress -or 
        [math]::Abs($percent - $script:LastLoggedProgress) -ge 1 -or 
        $percent -eq 0 -or 
        $percent -eq 100) {
        Write-Log "PROGRESS: $percent% ($Value/$Maximum)"
        $script:LastLoggedProgress = $percent
    }
}

# Helper function to show MessageBox and log it
function Show-MessageBox {
    param(
        [string]$Message,
        [string]$Title = "MediCat File Checker",
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

function Get-DriveList {
    # Get checkbox state
    $showHardDrives = $showHardDrivesCheckBox.Checked
    
    # Get all logical disks
    $allDrives = Get-WmiObject -Class Win32_LogicalDisk | Where-Object { 
        $_.DeviceID -ne "C:" -and 
        $_.Size -gt 1GB
    } | Sort-Object DeviceID
    
    # Detect VHD/VHDX drives by checking disk BusType
    # VHD/VHDX drives have BusType = 5 (File Backed Virtual) or BusType = "File Backed Virtual"
    $vhdDrives = @()
    try {
        $allDisks = Get-Disk -ErrorAction SilentlyContinue
        foreach ($disk in $allDisks) {
            # Check if this is a VHD/VHDX (File Backed Virtual)
            # BusType can be an enum (5) or string ("File Backed Virtual")
            $busTypeStr = [string]$disk.BusType
            $isVHD = ($disk.BusType -eq 5 -or 
                     $busTypeStr -eq "File Backed Virtual" -or 
                     $busTypeStr -like "*File Backed Virtual*")
            
            if ($isVHD) {
                # Get all partitions and their drive letters for this VHD disk
                $partitions = Get-Partition -DiskNumber $disk.Number -ErrorAction SilentlyContinue
                foreach ($partition in $partitions) {
                    if ($partition.DriveLetter -and $partition.DriveLetter -ne 'C') {
                        $vhdDrives += "$($partition.DriveLetter):"
                    }
                }
            }
        }
    } catch {
        # Silently fail VHD detection if there's an error (Write-Log may not be available yet)
        # VHD detection failure shouldn't prevent drive list from populating
    }
    
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

function Invoke-Download {
    param($Url, $OutputPath, $ExpectedSize = $null)
    
    try {
        Write-Log "Downloading: $Url"
        $webClient = New-Object System.Net.WebClient
        $webClient.DownloadFile($Url, $OutputPath)
        
        if (Test-Path $OutputPath) {
            $actualSize = (Get-Item $OutputPath).Length
            Write-Log "Downloaded: $OutputPath ($actualSize bytes)"
            
            if ($ExpectedSize -and $actualSize -ne $ExpectedSize) {
                Write-Log "WARNING: Size mismatch. Expected: $ExpectedSize, Got: $actualSize"
                return $false
            }
            return $true
        } else {
            Write-Log "ERROR: File not created: $OutputPath"
            return $false
        }
    } catch {
        Write-Log "ERROR downloading $Url : $($_.Exception.Message)"
        return $false
    }
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

function Start-FileCheck {
    param($DriveLetter)
    
    try {
        $checkFilesButton.Enabled = $false
        $closeButton.Enabled = $false
        $refreshButton.Enabled = $false
        
        Update-Status (Get-StatusTranslation -Key "checking_usb")
        Write-Log "Starting file check on $DriveLetter"
        
        # Check internet connection
        if (-not (Test-InternetConnection)) {
            Show-MessageBox -Message (Get-MessageTranslation -Key "no_internet") -Title (Get-TitleTranslation -Key "no_internet") -Icon "Warning"
            return
        }
        
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
        $checkFilesButton.Enabled = $true
        $closeButton.Enabled = $true
        $refreshButton.Enabled = $true
        Update-Status (Get-StatusTranslation -Key "status_ready")
    }
}

function Start-ReExtractFiles {
    param($DriveLetter, $FailedFiles)
    
    try {
        Update-Status (Get-StatusTranslation -Key "re_extracting")
        Write-Log "Starting re-extraction of $($FailedFiles.Count) files"
        
        # Check if 7z exists - check PATH first, then bin folder
        $sevenZipPath = $null
        
        # Try to find 7z in PATH
        try {
            $path7z = Get-Command "7z.exe" -ErrorAction SilentlyContinue
            if ($path7z) {
                $sevenZipPath = $path7z.Source
                Write-Log "Found 7z.exe in PATH: $sevenZipPath"
            }
        } catch {
            # Not in PATH, continue to check bin folder
        }
        
        # If not in PATH, check bin folder
        if (-not $sevenZipPath) {
            $binPath = ".\bin\7z.exe"
            if (Test-Path $binPath) {
                $sevenZipPath = (Resolve-Path $binPath).Path
                Write-Log "Found 7z.exe in bin folder: $sevenZipPath"
            }
        }
        
        # If still not found, error out
        if (-not $sevenZipPath -or -not (Test-Path $sevenZipPath)) {
            Write-Log "ERROR: 7z.exe not found in PATH or bin folder"
            Show-MessageBox -Message (Get-MessageTranslation -Key "7zip_not_found") -Title (Get-TitleTranslation -Key "7zip_not_found") -Icon "Error"
            return $false
        }
        
        # Find the source archive - check current directory first
        $archiveFile = Join-Path $PWD "MediCat.USB.v$script:MediCatVersion.7z"
        
        # If not found in current directory, prompt user to select the file
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
        Write-Log "Found 7z: $sevenZipPath"
        
        # Normalize all file paths and create file list for batch extraction
        Write-Log "Preparing file list for batch extraction..."
        $normalizedFiles = @()
        
        foreach ($failedFile in $FailedFiles) {
            # Normalize the file path - remove drive letter if present, ensure correct path format
            $fileToExtract = $failedFile.Trim()
            if ($fileToExtract -match '^[A-Z]:\\(.+)') {
                $fileToExtract = $matches[1]
            }
            # Remove leading backslash or forward slash
            $fileToExtract = $fileToExtract.TrimStart('\', '/')
            
            if ($fileToExtract) {
                $normalizedFiles += $fileToExtract
            }
        }
        
        Write-Log "Extracting $($normalizedFiles.Count) files in batch operation..."
        Update-Status "Extracting $($normalizedFiles.Count) files (this may take a while)..."
        Update-Progress -Value 0 -Maximum 100
        
        # Write file list to temporary file for 7z (7z expects one file per line)
        $fileListPath = Join-Path $env:TEMP "medicat_extract_list_$([System.Guid]::NewGuid().ToString().Substring(0,8)).txt"
        try {
            $normalizedFiles | Out-File -FilePath $fileListPath -Encoding UTF8
            Write-Log "Created file list with $($normalizedFiles.Count) files: $fileListPath"
            
            # Extract all files in a single batch operation
            # Using @filename syntax tells 7z to read file list from the file
            $outputDir = $DriveLetter.TrimEnd('\')
            $arguments = "x `"$archiveFile`" -o`"$outputDir`" @`"$fileListPath`" -aoa"
            
            Write-Log "Starting batch extraction..."
            $startTime = Get-Date
            
            # Run extraction with progress monitoring
            $process = Start-Process -FilePath $sevenZipPath -ArgumentList $arguments -Wait -PassThru -NoNewWindow -RedirectStandardOutput "7z_output.tmp" -RedirectStandardError "7z_error.tmp"
            
            $endTime = Get-Date
            $duration = ($endTime - $startTime).TotalSeconds
            Write-Log "Extraction completed in $([math]::Round($duration, 2)) seconds"
            
            # Check for errors
            $errorContent = Get-Content "7z_error.tmp" -ErrorAction SilentlyContinue
            
            if ($process.ExitCode -eq 0) {
                Write-Log "Batch extraction completed successfully"
                $extractedCount = $normalizedFiles.Count
                $failedExtract = @()
            } else {
                Write-Log "[FAIL] Extraction failed with exit code: $($process.ExitCode)"
                if ($errorContent) {
                    Write-Log "Error details: $($errorContent -join "`n")"
                }
                
                # Try to determine which files failed by checking which ones exist
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
                Write-Log "Verified: $extractedCount/$($normalizedFiles.Count) files exist after extraction"
            }
            
            # Clean up temp files
            Remove-Item "7z_output.tmp" -ErrorAction SilentlyContinue
            Remove-Item "7z_error.tmp" -ErrorAction SilentlyContinue
            Remove-Item $fileListPath -ErrorAction SilentlyContinue
            
            Update-Progress -Value 100 -Maximum 100
            Write-Log "Re-extraction complete: $extractedCount/$($normalizedFiles.Count) files extracted successfully"
            
        } catch {
            Write-Log "ERROR preparing file list: $($_.Exception.Message)"
            $failedExtract = $normalizedFiles
            $extractedCount = 0
        } finally {
            # Clean up file list if it still exists
            if (Test-Path $fileListPath) {
                Remove-Item $fileListPath -ErrorAction SilentlyContinue
            }
        }
        
        if ($failedExtract.Count -gt 0) {
            Write-Log "WARNING: Could not extract $($failedExtract.Count) file(s):"
            foreach ($failed in $failedExtract) {
                Write-Log "  - $failed"
            }
            Show-MessageBox -Message (Get-MessageTranslation -Key "re_extract_failed" -FormatArgs $extractedCount, $FailedFiles.Count, $failedExtract.Count) -Title (Get-TitleTranslation -Key "re_extraction_failed") -Icon "Warning"
            return $false
        } else {
            Update-Status (Get-StatusTranslation -Key "re_extracting")
            return $true
        }
        
    } catch {
        Write-Log "ERROR during re-extraction: $($_.Exception.Message)"
        Update-Status (Get-StatusTranslation -Key "re_extracting")
        Show-MessageBox -Message (Get-MessageTranslation -Key "re_extraction_error" -FormatArgs $_.Exception.Message) -Title (Get-TitleTranslation -Key "re_extraction_error") -Icon "Error"
        return $false
    }
}

# Event handlers
$checkFilesButton.Add_Click({
    $selectedDrive = $driveComboBox.SelectedItem
    if ($selectedDrive) {
        $driveLetter = $selectedDrive.Substring(0, 2)
        Start-FileCheck -DriveLetter $driveLetter
    } else {
        Show-MessageBox -Message (Get-MessageTranslation -Key "no_drive_for_check") -Title (Get-TitleTranslation -Key "no_drive_for_check") -Icon "Warning"
    }
})

$closeButton.Add_Click({
    $form.Close()
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

# Initialize
Write-Log "MediCat File Checker v$script:LocalVersion Started"
Get-DriveList

# Show form
$form.ShowDialog()
