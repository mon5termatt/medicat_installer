# MediCat Installer - Modern GUI Version
# PowerShell-based installer with proper error handling and logging

# Check if running as administrator, if not, request elevation
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
        $processStartInfo = New-Object System.Diagnostics.ProcessStartInfo
        $processStartInfo.FileName = "powershell.exe"
        $processStartInfo.Arguments = "-ExecutionPolicy Bypass -NoProfile -File `"$scriptPath`""
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

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

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

# Create main form
$form = New-Object System.Windows.Forms.Form
$form.Text = "MediCat Installer v$script:LocalVersion [Administrator]"
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
$titleLabel.Text = "MediCat USB Installer"
$titleLabel.Font = New-Object System.Drawing.Font("Arial", 16, [System.Drawing.FontStyle]::Bold)
$titleLabel.ForeColor = [System.Drawing.Color]::DarkBlue
$titleLabel.AutoSize = $true
$titleLabel.Location = New-Object System.Drawing.Point(20, 20)

# Status label
$statusLabel = New-Object System.Windows.Forms.Label
$statusLabel.Text = "Ready to install MediCat"
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
$driveLabel.Text = "Select USB Drive:"
$driveLabel.Location = New-Object System.Drawing.Point(20, 450)
$driveLabel.AutoSize = $true

$driveComboBox = New-Object System.Windows.Forms.ComboBox
$driveComboBox.Location = New-Object System.Drawing.Point(150, 448)
$driveComboBox.Size = New-Object System.Drawing.Size(200, 25)
$driveComboBox.DropDownStyle = "DropDownList"

# Show hard drives checkbox
$showHardDrivesCheckBox = New-Object System.Windows.Forms.CheckBox
$showHardDrivesCheckBox.Text = "Show hard drives"
$showHardDrivesCheckBox.Location = New-Object System.Drawing.Point(360, 450)
$showHardDrivesCheckBox.AutoSize = $true
$showHardDrivesCheckBox.Checked = $false

# Format checkbox
$formatCheckBox = New-Object System.Windows.Forms.CheckBox
$formatCheckBox.Text = "Format drive before installation"
$formatCheckBox.Location = New-Object System.Drawing.Point(20, 480)
$formatCheckBox.AutoSize = $true
$formatCheckBox.Checked = $true

# Buttons
$installButton = New-Object System.Windows.Forms.Button
$installButton.Text = "Install MediCat"
$installButton.Location = New-Object System.Drawing.Point(20, 520)
$installButton.Size = New-Object System.Drawing.Size(120, 30)
$installButton.BackColor = [System.Drawing.Color]::LightGreen

$cancelButton = New-Object System.Windows.Forms.Button
$cancelButton.Text = "Cancel"
$cancelButton.Location = New-Object System.Drawing.Point(150, 520)
$cancelButton.Size = New-Object System.Drawing.Size(80, 30)

$checkFilesButton = New-Object System.Windows.Forms.Button
$checkFilesButton.Text = "Check USB Files"
$checkFilesButton.Location = New-Object System.Drawing.Point(240, 520)
$checkFilesButton.Size = New-Object System.Drawing.Size(120, 30)

$refreshButton = New-Object System.Windows.Forms.Button
$refreshButton.Text = "Refresh Drives"
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
$mainPanel.Controls.Add($installButton)
$mainPanel.Controls.Add($cancelButton)
$mainPanel.Controls.Add($checkFilesButton)
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
    $iDriveIndex = -1
    $itemIndex = 0
    
    foreach ($drive in $drives) {
        $size = [math]::Round($drive.Size / 1GB, 2)
        $free = [math]::Round($drive.FreeSpace / 1GB, 2)
        
        # Determine drive type label
        $driveTypeLabel = if ($drive.DeviceID -in $vhdDrives) {
            "(VHD)"
        } elseif ($drive.DriveType -eq 2) {
            "(USB)"
        } else {
            "(HDD)"
        }
        
        $driveComboBox.Items.Add("$($drive.DeviceID) $driveTypeLabel - ${free}GB free of ${size}GB")
        
        # DEBUG: Look for I: drive and remember its index
        if ($drive.DeviceID -eq "I:") {
            $iDriveIndex = $itemIndex
        }
        
        # Remember first drive as default fallback
        if ($defaultDriveIndex -eq -1) {
            $defaultDriveIndex = $itemIndex
        }
        
        $itemIndex++
    }
    
    # DEBUG: Default to I: drive if available, otherwise use first drive
    if ($driveComboBox.Items.Count -gt 0) {
        if ($iDriveIndex -ge 0) {
            $driveComboBox.SelectedIndex = $iDriveIndex
            Write-DebugLog "Selected I: drive as default (DEBUG mode)"
        } else {
            $driveComboBox.SelectedIndex = $defaultDriveIndex
            Write-DebugLog "I: drive not found, selected first available drive"
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

function Download-BinFiles {
    <#
    .SYNOPSIS
    Downloads all required bin files to the bin folder.
    
    .DESCRIPTION
    Downloads all installer binaries and utilities needed for the MediCat installer.
    #>
    try {
        Write-Log "Starting bin files download..."
        Update-Status "Downloading installer files to bin folder..."
        
        # Create bin directory if it doesn't exist
        if (-not (Test-Path "./bin")) {
            New-Item -ItemType Directory -Path "./bin" -Force | Out-Null
            Write-Log "Created bin directory"
        }
        
        # Define all bin files to download
        $binFiles = @(
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/QuickSFV.EXE"; Path="./bin/QuickSFV.exe"; Size=103424},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/QuickSFV.ini"; Path="./bin/QuickSFV.ini"; Size=158},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/Box.bat"; Path="./bin/Box.bat"; Size=5874},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/Button.bat"; Path="./bin/Button.bat"; Size=5254},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/GetInput.exe"; Path="./bin/GetInput.exe"; Size=3584},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/Getlen.bat"; Path="./bin/Getlen.bat"; Size=1897},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/batbox.exe"; Path="./bin/batbox.exe"; Size=1536},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/folderbrowse.exe"; Path="./bin/folderbrowse.exe"; Size=8192}
        )
        
        # Download 7z files based on architecture
        $arch = if ([Environment]::Is64BitOperatingSystem) { "64" } else { "32" }
        $binFiles += @(
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/7z/$arch.exe"; Path="./bin/7z.exe"; Size=""},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/7z/$arch.dll"; Path="./bin/7z.dll"; Size=""}
        )
        
        Update-Progress -Value 0 -Maximum $binFiles.Count
        
        $successCount = 0
        for ($i = 0; $i -lt $binFiles.Count; $i++) {
            $file = $binFiles[$i]
            Write-Log "Downloading: $($file.Path)"
            
            if ($file.Size) {
                # File has size validation
                if (Invoke-Download -Url $file.Url -OutputPath $file.Path -ExpectedSize $file.Size) {
                    $successCount++
                    Write-Log "[GOOD] $($file.Path)"
                } else {
                    Write-Log "[BAD] $($file.Path)"
                }
            } else {
                # No size validation (7z files)
                if (Invoke-Download -Url $file.Url -OutputPath $file.Path) {
                    $successCount++
                    Write-Log "[GOOD] $($file.Path)"
                } else {
                    Write-Log "[BAD] $($file.Path)"
                }
            }
            
            Update-Progress -Value ($i + 1) -Maximum $binFiles.Count
        }
        
        Write-Log "Bin files download complete: $successCount/$($binFiles.Count) files downloaded successfully"
        
        if ($successCount -eq $binFiles.Count) {
            Update-Status "All bin files downloaded successfully"
            return $true
        } else {
            Update-Status "Bin files download completed with errors"
            [System.Windows.Forms.MessageBox]::Show("Downloaded $successCount of $($binFiles.Count) bin files. Some files may have failed. Check the log for details.", "Download Incomplete", "OK", "Warning")
            return $false
        }
        
    } catch {
        Write-Log "ERROR during bin files download: $($_.Exception.Message)"
        Update-Status "Bin files download failed"
        [System.Windows.Forms.MessageBox]::Show("An error occurred while downloading bin files: $($_.Exception.Message)", "Download Error", "OK", "Error")
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
        Update-Status "Checking for Ventoy updates..."
        
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
            
            Update-Status "Downloading Ventoy v$ventoyVersion..."
            
            $ventoyZipUrl = "https://github.com/ventoy/Ventoy/releases/download/v$ventoyVersion/ventoy-$ventoyVersion-windows.zip"
            $ventoyZip = ".\ventoy.zip"
            
            # Download Ventoy zip
            if (-not (Invoke-Download -Url $ventoyZipUrl -OutputPath $ventoyZip)) {
                Write-Log "ERROR: Failed to download Ventoy"
                return $false
            }
            
            # Extract Ventoy zip
            Update-Status "Extracting Ventoy..."
            Write-Log "Extracting Ventoy archive..."
            
            # Find 7z.exe
            $sevenZipPath = $null
            try {
                $path7z = Get-Command "7z.exe" -ErrorAction SilentlyContinue
                if ($path7z) {
                    $sevenZipPath = $path7z.Source
                }
            } catch { }
            
            if (-not $sevenZipPath) {
                $binPath = ".\bin\7z.exe"
                if (Test-Path $binPath) {
                    $sevenZipPath = (Resolve-Path $binPath).Path
                }
            }
            
            if (-not $sevenZipPath) {
                Write-Log "ERROR: 7z.exe not found for extracting Ventoy"
                Remove-Item $ventoyZip -ErrorAction SilentlyContinue
                return $false
            }
            
            # Extract with 7z
            $extractArgs = "x `"$ventoyZip`" -r -aoa"
            $process = Start-Process -FilePath $sevenZipPath -ArgumentList $extractArgs -Wait -PassThru -NoNewWindow
            
            if ($process.ExitCode -ne 0) {
                Write-Log "ERROR: Failed to extract Ventoy archive (exit code: $($process.ExitCode))"
                Remove-Item $ventoyZip -ErrorAction SilentlyContinue
                return $false
            }
            
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
        Update-Status "Installing Ventoy to $DriveLetter (this may take a moment)..."
        
        # Show warning message
        $warningResult = [System.Windows.Forms.MessageBox]::Show(
            "IMPORTANT WARNING:`n`n" +
            "SOMETIMES VENTOY MESSES UP A DRIVE.`n" +
            "IF THE DRIVE DISAPPEARS PLEASE CHECK DISK MANAGER`n" +
            "AND SEE IF THE DRIVE FAILED TO REMOUNT.`n" +
            "THIS IS A VENTOY BUG AND CANNOT BE FIXED ON OUR END.`n`n" +
            "Install Ventoy to $DriveLetter ?`n`n" +
            "IF FROZEN FOR MORE THAN 60 SECONDS, INSTALL VENTOY MANUALLY.",
            "Ventoy Installation Warning",
            "YesNo",
            "Warning"
        )
        
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

function Start-MediatInstallation {
    $installButton.Enabled = $false
    $cancelButton.Enabled = $false
    
    try {
        # Initialize log
        Write-Log "MediCat Installation Started"
        
        # Check internet
        if (-not (Test-InternetConnection)) {
            Show-MessageBox -Message "No internet connection detected. Please check your connection and try again." -Title "No Internet" -Icon "Warning"
            return
        }
        
        # Get selected drive
        $selectedDrive = $driveComboBox.SelectedItem
        if (-not $selectedDrive) {
            Show-MessageBox -Message "Please select a USB drive." -Title "No Drive Selected" -Icon "Warning"
            return
        }
        
        $driveLetter = $selectedDrive.Substring(0, 2)
        Write-Log "Selected drive: $driveLetter"
        
        # Download bin files
        Update-Status "Downloading installer files..."
        Update-Progress -Value 0 -Maximum 8
        
        $binFiles = @(
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/QuickSFV.EXE"; Path="./bin/QuickSFV.exe"; Size=103424},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/QuickSFV.ini"; Path="./bin/QuickSFV.ini"; Size=158},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/Box.bat"; Path="./bin/Box.bat"; Size=5874},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/Button.bat"; Path="./bin/Button.bat"; Size=5254},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/GetInput.exe"; Path="./bin/GetInput.exe"; Size=3584},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/Getlen.bat"; Path="./bin/Getlen.bat"; Size=1897},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/batbox.exe"; Path="./bin/batbox.exe"; Size=1536},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/folderbrowse.exe"; Path="./bin/folderbrowse.exe"; Size=8192}
        )
        
        # Create bin directory
        if (-not (Test-Path "./bin")) {
            New-Item -ItemType Directory -Path "./bin" -Force | Out-Null
        }
        
        $successCount = 0
        for ($i = 0; $i -lt $binFiles.Count; $i++) {
            $file = $binFiles[$i]
            if (Invoke-Download -Url $file.Url -OutputPath $file.Path -ExpectedSize $file.Size) {
                $successCount++
                Write-Log "[GOOD] $($file.Path)"
            } else {
                Write-Log "[BAD] $($file.Path)"
            }
            Update-Progress -Value ($i + 1)
        }
        
        if ($successCount -lt $binFiles.Count) {
            Write-Log "WARNING: Only $successCount of $($binFiles.Count) files downloaded successfully"
        }
        
        # Download 7z files
        Update-Status "Downloading 7-Zip files..."
        $arch = if ([Environment]::Is64BitOperatingSystem) { "64" } else { "32" }
        
        $sevenZipFiles = @(
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/7z/$arch.exe"; Path="./bin/7z.exe"},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/7z/$arch.dll"; Path="./bin/7z.dll"}
        )
        
        foreach ($file in $sevenZipFiles) {
            if (Invoke-Download -Url $file.Url -OutputPath $file.Path) {
                Write-Log "[GOOD] $($file.Path)"
            } else {
                Write-Log "[BAD] $($file.Path)"
            }
        }
        
        # Download translation files
        Update-Status "Downloading translation files..."
        $translationFiles = @(
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/translate/motd.ps1"; Path="./bin/motd.ps1"},
            @{Url="https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/translate/licence.ps1"; Path="./bin/licence.ps1"}
        )
        
        foreach ($file in $translationFiles) {
            if (Invoke-Download -Url $file.Url -OutputPath $file.Path) {
                Write-Log "[GOOD] $($file.Path)"
            } else {
                Write-Log "[BAD] $($file.Path)"
            }
        }
        
        # Check for MediCat files
        Update-Status "Checking for MediCat installation files..."
        $medicatFileName = "MediCat.USB.v$script:MediCatVersion.7z"
        $medicatFile = Join-Path $PWD $medicatFileName
        if (-not (Test-Path $medicatFile)) {
            Write-Log "MediCat file not found: $medicatFile"
            [System.Windows.Forms.MessageBox]::Show("MediCat installation file not found. Please ensure '$medicatFileName' is in the same directory as this installer.", "File Not Found", "OK", "Warning")
            return
        }
        Write-Log "Found MediCat archive: $medicatFile"
        
        # Check if format checkbox is checked
        $shouldFormat = $formatCheckBox.Checked
        Write-Log "Format checkbox is: $(if ($shouldFormat) { 'Checked' } else { 'Unchecked' })"
        
        if ($shouldFormat) {
            # Format is checked - do fresh install
            Write-Log "Formatting is enabled - performing fresh Ventoy installation"
            
            # Install Ventoy (fresh install)
            if (-not (Install-Ventoy -DriveLetter $driveLetter)) {
                Write-Log "ERROR: Ventoy installation failed"
                [System.Windows.Forms.MessageBox]::Show("Ventoy installation failed. Please check the log for details.", "Ventoy Installation Failed", "OK", "Error")
                return
            }
            
            # Format drive to NTFS after Ventoy installation
            Update-Status "Formatting drive to NTFS..."
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
            Update-Status "Checking for existing Ventoy installation..."
            
            $ventoyInstalled = Test-VentoyInstalled -DriveLetter $driveLetter
            
            if (-not $ventoyInstalled) {
                # Ventoy not found - warn user
                Write-Log "WARNING: VTOYEFI partition not found - Ventoy may not be installed"
                
                $confirmResult = [System.Windows.Forms.MessageBox]::Show(
                    "Warning: Could not detect an existing Ventoy installation on this drive.`n`n" +
                    "The VTOYEFI partition was not found.`n`n" +
                    "Are you SURE Ventoy is already installed on $driveLetter ?`n`n" +
                    "If Ventoy is not installed, installation will fail.`n`n" +
                    "Click 'Yes' to continue with upgrade anyway, or 'No' to cancel.",
                    "Ventoy Installation Not Detected",
                    "YesNo",
                    "Warning"
                )
                
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
                [System.Windows.Forms.MessageBox]::Show("Ventoy upgrade failed. Please check the log for details.", "Ventoy Upgrade Failed", "OK", "Error")
                return
            }
            
            Write-Log "Skipping format step (format checkbox is unchecked)"
        }
        
        # Extract MediCat files
        Update-Status "Extracting MediCat archive (this will take a while)..."
        Write-Log "Extracting MediCat files from $medicatFile to $driveLetter"
        
        $outputDir = $driveLetter.TrimEnd('\')
        $usePowerShellModule = $false
        
        # Try to use PowerShell 7Zip4PowerShell module first (native PowerShell solution)
        try {
            Write-DebugLog "Checking for 7Zip4PowerShell module..."
            
            # Search specifically for 7Zip4PowerShell module (case-insensitive)
            $allModules = Get-Module -ListAvailable -ErrorAction SilentlyContinue
            $module = $allModules | Where-Object { 
                $_.Name -eq '7Zip4PowerShell' -or 
                $_.Name -eq '7Zip4Powershell' -or
                $_.Name -like '7Zip4PowerShell*'
            } | Select-Object -First 1
            
            if ($module) {
                $moduleName = $module.Name
                Write-DebugLog "Found 7Zip4PowerShell module: $($module.Name) v$($module.Version) at $($module.Path)"
            } else {
                Write-DebugLog "7Zip4PowerShell module not found in installed modules"
                Write-Log "7Zip4PowerShell module not found. Attempting to install..."
                
                # Attempt to install the module locally
                try {
                    Update-Status "Installing 7Zip4PowerShell module (required for extraction)..."
                    Write-Log "Installing 7Zip4PowerShell module..."
                    
                    # Check for NuGet provider
                    $nuget = Get-PackageProvider -Name NuGet -ErrorAction SilentlyContinue
                    if (-not $nuget) {
                        Write-Log "Installing NuGet package provider..."
                        Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Force -Scope CurrentUser -ErrorAction Stop | Out-Null
                        Write-Log "NuGet provider installed"
                    }
                    
                    # Ensure PSGallery is trusted
                    try {
                        $repo = Get-PSRepository -Name PSGallery -ErrorAction SilentlyContinue
                        if ($repo -and $repo.InstallationPolicy -eq "Untrusted") {
                            Write-Log "Setting PSGallery to trusted..."
                            Set-PSRepository -Name PSGallery -InstallationPolicy Trusted -ErrorAction Stop
                            Write-Log "PSGallery set to trusted"
                        }
                    } catch {
                        Write-Log "WARN: Could not set PSGallery policy (may require admin): $($_.Exception.Message)"
                        Write-Log "Attempting installation anyway..."
                    }
                    
                    # Try to find the module in gallery (try both name variations)
                    $galleryModuleName = $null
                    $moduleNames = @("7Zip4PowerShell", "7Zip4Powershell")
                    foreach ($name in $moduleNames) {
                        try {
                            $galleryModule = Find-Module -Name $name -Repository PSGallery -ErrorAction Stop
                            $galleryModuleName = $galleryModule.Name
                            Write-Log "Found $galleryModuleName v$($galleryModule.Version) in PowerShell Gallery"
                            break
                        } catch {
                            continue
                        }
                    }
                    
                    if (-not $galleryModuleName) {
                        throw "Module not found in PowerShell Gallery. Tried: $($moduleNames -join ', ')"
                    }
                    
                    # Install the module for current user
                    Write-Log "Downloading and installing $galleryModuleName (this may take a moment)..."
                    Install-Module -Name $galleryModuleName -Force -Scope CurrentUser -ErrorAction Stop
                    Write-Log "Successfully installed $galleryModuleName module"
                    
                    # Refresh module list and find the newly installed module
                    Start-Sleep -Seconds 1
                    $allModules = Get-Module -ListAvailable -ErrorAction SilentlyContinue
                    $module = $allModules | Where-Object { 
                        $_.Name -eq $galleryModuleName -or
                        $_.Name -eq '7Zip4PowerShell' -or 
                        $_.Name -eq '7Zip4Powershell' -or
                        $_.Name -like '7Zip4PowerShell*'
                    } | Select-Object -First 1
                    
                    if ($module) {
                        $moduleName = $module.Name
                        Write-Log "7Zip4PowerShell module installed successfully: $($module.Name) v$($module.Version)"
                    } else {
                        throw "Module installed but not found in module list"
                    }
                    
                } catch {
                    $installError = $_.Exception.Message
                    Write-Log "ERROR: Failed to install 7Zip4PowerShell module: $installError"
                    Write-DebugLog "Install exception type: $($_.Exception.GetType().Name)"
                    Write-DebugLog "Install exception details: $($_.Exception | Format-List | Out-String)"
                    
                    if ($_.Exception.Message -like "*admin*" -or $_.Exception.Message -like "*permission*" -or $_.Exception.Message -like "*access*") {
                        [System.Windows.Forms.MessageBox]::Show(
                            "Failed to install 7Zip4PowerShell module. Administrator privileges may be required.`n`nError: $installError`n`nPlease try running PowerShell as Administrator and installing manually:`nInstall-Module 7Zip4PowerShell -Scope CurrentUser",
                            "Module Installation Failed",
                            "OK",
                            "Error"
                        )
                    } else {
                        [System.Windows.Forms.MessageBox]::Show(
                            "Failed to install 7Zip4PowerShell module.`n`nError: $installError`n`nPlease try installing manually:`nInstall-Module 7Zip4PowerShell -Scope CurrentUser",
                            "Module Installation Failed",
                            "OK",
                            "Error"
                        )
                    }
                    $moduleName = $null
                    $module = $null
                }
            }
            
            if ($module) {
                Write-Log "Attempting to import 7Zip4PowerShell module ($moduleName)..."
                Import-Module $moduleName -Force -ErrorAction Stop
                
                # Check for extraction cmdlet names used by 7Zip4PowerShell
                # 7Zip4PowerShell typically uses Expand-7Zip or Expand-SevenZip
                $expandCmdNames = @("Expand-7Zip", "Expand-SevenZip")
                $expandCmd = $null
                
                foreach ($cmdName in $expandCmdNames) {
                    $cmd = Get-Command $cmdName -ErrorAction SilentlyContinue
                    if ($cmd -and $cmd.ModuleName -eq $moduleName) {
                        $expandCmd = $cmd
                        Write-DebugLog "Found extraction cmdlet: $cmdName in module $moduleName"
                        break
                    }
                }
                
                if ($expandCmd) {
                    Write-Log "Using 7Zip4PowerShell module for extraction ($($expandCmd.Name))"
                    $usePowerShellModule = $true
                    $script:Expand7ZipCmdlet = $expandCmd.Name  # Store cmdlet name for use later
                    $script:Expand7ZipModuleName = $moduleName   # Store module name
                } else {
                    Write-Log "WARNING: 7Zip4PowerShell module found but extraction cmdlet not available"
                    $loadedModule = Get-Module $moduleName
                    if ($loadedModule) {
                        $commands = $loadedModule.ExportedCommands.Keys -join ", "
                        Write-DebugLog "Available commands in module: $commands"
                    }
                    $usePowerShellModule = $false
                }
            } else {
                Write-Log "7Zip4PowerShell module not found. Falling back to 7z.exe"
                Write-DebugLog "To install 7Zip4PowerShell: Install-Module 7Zip4PowerShell"
                $usePowerShellModule = $false
            }
        } catch {
            Write-Log "7Zip4PowerShell module check failed: $($_.Exception.Message)"
            Write-DebugLog "Exception details: $($_.Exception.GetType().Name) - $($_.Exception.Message)"
            Write-DebugLog "Stack trace: $($_.ScriptStackTrace)"
            Write-Log "7Zip4PowerShell module not available"
            $usePowerShellModule = $false
        }
        
        if (-not $usePowerShellModule) {
            # TEMP: 7z.exe fallback removed - only using 7Zip4PowerShell module
            Write-Log "ERROR: 7Zip4PowerShell module not available"
            [System.Windows.Forms.MessageBox]::Show(
                "7Zip4PowerShell module is required for extraction.`n`nPlease install it:`nInstall-Module 7Zip4PowerShell`n`nAfter installation, run this installer again.",
                "7Zip4PowerShell Module Required",
                "OK",
                "Error"
            )
            return
            
            # TEMP: Commented out 7z.exe fallback
            # Write-Log "Will use 7z.exe for extraction"
            # # Fall back to 7z.exe
            # $sevenZipPath = $null
            # try {
            #     $path7z = Get-Command "7z.exe" -ErrorAction SilentlyContinue
            #     if ($path7z) {
            #         $sevenZipPath = $path7z.Source
            #         Write-Log "Found 7z.exe in PATH: $sevenZipPath"
            #     }
            # } catch {
            #     # Not in PATH, continue to check bin folder
            # }
            # 
            # if (-not $sevenZipPath) {
            #     $binPath = ".\bin\7z.exe"
            #     if (Test-Path $binPath) {
            #         $sevenZipPath = (Resolve-Path $binPath).Path
            #         Write-Log "Found 7z.exe in bin folder: $sevenZipPath"
            #     }
            # }
            # 
            # if (-not $sevenZipPath -or -not (Test-Path $sevenZipPath)) {
            #     Write-Log "ERROR: 7z.exe not found in PATH or bin folder"
            #     [System.Windows.Forms.MessageBox]::Show(
            #         "No extraction method available.`n`nNeither 7Zip4PowerShell module nor 7z.exe found.`n`nRecommended: Install 7Zip4PowerShell module:`nInstall-Module 7Zip4PowerShell`n`nAlternative: Ensure 7-Zip is installed or bin files are downloaded.",
            #         "Extraction Tool Not Found",
            #         "OK",
            #         "Error"
            #     )
            #     return
            # }
            
            # TEMP: 7z.exe extraction args commented out
            # Extract using 7z.exe with progress monitoring
            # # x = extract with full paths
            # # -o = output directory
            # # -aoa = overwrite all existing files
            # # -bb0 = output progress with percentage
            # $extractArgs = "x `"$medicatFile`" -o`"$outputDir`" -aoa -y -bb0"
            # Write-Log "7z command: $sevenZipPath $extractArgs"
        }
        
        Write-Log "Starting full archive extraction..."
        Write-Log "Archive: $medicatFile"
        Write-Log "Destination: $outputDir"
        
        $startTime = Get-Date
        Update-Progress -Value 0 -Maximum 100
        
        $extractionSuccess = $false
        $extractionError = $null
        
        try {
            if ($usePowerShellModule) {
                # Use PowerShell 7Zip4PowerShell module (native PowerShell)
                $cmdletName = $script:Expand7ZipCmdlet
                Write-Log "Using PowerShell cmdlet: $cmdletName"
                
                # Monitor extraction progress by tracking destination directory changes
                Update-Status "Extracting archive using PowerShell (this may take a while)..."
                Update-Progress -Value 0 -Maximum 100
                
                try {
                    # Use 7Zip4PowerShell module's Expand-7Zip or Expand-SevenZip cmdlet
                    Write-DebugLog "Attempting extraction with 7Zip4PowerShell cmdlet: $cmdletName"
                    Write-DebugLog "Archive: $medicatFile"
                    Write-DebugLog "Target: $outputDir"
                    Write-DebugLog "Module: $script:Expand7ZipModuleName"
                    
                    # Get initial file count/size in destination (may be 0 if empty)
                    $initialFileCount = 0
                    $initialSize = 0
                    if (Test-Path $outputDir) {
                        $initialFiles = Get-ChildItem -Path $outputDir -Recurse -File -ErrorAction SilentlyContinue
                        $initialFileCount = $initialFiles.Count
                        $initialSize = ($initialFiles | Measure-Object -Property Length -Sum -ErrorAction SilentlyContinue).Sum
                    }
                    Write-DebugLog "Initial destination: $initialFileCount files, $([math]::Round($initialSize / 1MB, 2)) MB"
                    
                    # Get archive size for progress estimation
                    $archiveInfo = Get-Item $medicatFile -ErrorAction SilentlyContinue
                    $archiveSize = if ($archiveInfo) { $archiveInfo.Length } else { 0 }
                    Write-DebugLog "Archive size: $([math]::Round($archiveSize / 1GB, 2)) GB"
                    
                    # Get cmdlet and check parameter patterns
                    $extractionCmd = Get-Command $cmdletName
                    $paramNames = $extractionCmd.Parameters.Keys
                    Write-DebugLog "Available parameters: $($paramNames -join ', ')"
                    
                    # Expand-7Zip uses: ArchiveFileName (required), TargetPath (required)
                    # Optional: Password, SecurePassword
                    $params = @{}
                    
                    if ($paramNames -contains "ArchiveFileName") {
                        $params["ArchiveFileName"] = $medicatFile
                        if ($paramNames -contains "TargetPath") {
                            $params["TargetPath"] = $outputDir
                        } else {
                            throw "Expand-7Zip cmdlet is missing required TargetPath parameter"
                        }
                    } elseif ($paramNames -contains "Path") {
                        # Fallback for other cmdlet variations
                        $params["Path"] = $medicatFile
                        if ($paramNames -contains "DestinationPath") {
                            $params["DestinationPath"] = $outputDir
                        } elseif ($paramNames -contains "TargetPath") {
                            $params["TargetPath"] = $outputDir
                        } else {
                            throw "Could not find destination path parameter"
                        }
                    } else {
                        # Try positional parameters as fallback
                        Write-DebugLog "Using positional parameters"
                        & $cmdletName $medicatFile $outputDir
                        $extractionSuccess = $true
                        Write-Log "Extraction completed using positional parameters"
                    }
                    
                    # Run extraction in background job to monitor progress
                    if (-not $extractionSuccess) {
                        Write-DebugLog "Calling $cmdletName with parameters: ArchiveFileName=$medicatFile, TargetPath=$outputDir"
                        
                        # Create a script block for the extraction job
                        $extractScript = {
                            param($CmdletName, $ArchiveFile, $TargetPath, $Params)
                            Import-Module 7Zip4PowerShell -Force -ErrorAction Stop
                            & $CmdletName @Params
                        }
                        
                        # Start the extraction job
                        $extractJob = Start-Job -ScriptBlock $extractScript -ArgumentList $cmdletName, $medicatFile, $outputDir, $params
                        Write-Log "Extraction started in background job (ID: $($extractJob.Id))"
                        
                        # Monitor progress while job is running
                        $lastProgress = 0
                        $progressUpdateInterval = 1.0  # Update every second
                        $lastUpdate = Get-Date
                        
                        while ($extractJob.State -eq "Running") {
                            # Process UI events to keep window responsive
                            [System.Windows.Forms.Application]::DoEvents()
                            
                            # Update progress based on destination directory size
                            if ((Get-Date) - $lastUpdate -gt [TimeSpan]::FromSeconds($progressUpdateInterval)) {
                                if (Test-Path $outputDir) {
                                    try {
                                        $currentFiles = Get-ChildItem -Path $outputDir -Recurse -File -ErrorAction SilentlyContinue
                                        $currentFileCount = $currentFiles.Count
                                        $currentSize = ($currentFiles | Measure-Object -Property Length -Sum -ErrorAction SilentlyContinue).Sum
                                        
                                        # Estimate progress based on size (rough estimate since we don't know final size)
                                        if ($archiveSize -gt 0) {
                                            # Estimated extraction ratio is typically 2-4x archive size (compressed to uncompressed)
                                            # Use a conservative estimate of 2x for progress calculation
                                            $estimatedFinalSize = $archiveSize * 2
                                            $progressPercent = [math]::Min(95, [math]::Round(($currentSize - $initialSize) / $estimatedFinalSize * 100))
                                        } else {
                                            # Fallback: progress based on file count growth (very rough)
                                            if ($currentFileCount -gt $initialFileCount) {
                                                $progressPercent = [math]::Min(95, [math]::Round(($currentFileCount - $initialFileCount) / 1000 * 100))
                                            } else {
                                                $progressPercent = 0
                                            }
                                        }
                                        
                                        # Only update if progress changed significantly (>1%)
                                        if ($progressPercent -ne $lastProgress) {
                                            $lastProgress = $progressPercent
                                            Update-Progress -Value $progressPercent -Maximum 100
                                            Update-Status "Extracting... $progressPercent% ($([math]::Round(($currentSize - $initialSize) / 1MB, 1)) MB extracted)"
                                            Write-DebugLog "Progress: $progressPercent%, Files: $currentFileCount, Size: $([math]::Round($currentSize / 1MB, 1)) MB"
                                        }
                                    } catch {
                                        # Ignore errors during progress monitoring
                                        Write-DebugLog "Progress monitoring error: $($_.Exception.Message)"
                                    }
                                }
                                $lastUpdate = Get-Date
                            }
                            
                            Start-Sleep -Milliseconds 250
                        }
                        
                        # Wait for job to complete
                        Wait-Job $extractJob | Out-Null
                        
                        # Check job state first
                        $jobState = $extractJob.State
                        
                        # Get job results and check for errors
                        $jobResult = Receive-Job $extractJob 2>&1
                        
                        # Check if there were errors in the output stream
                        $jobErrors = $jobResult | Where-Object { $_ -is [System.Management.Automation.ErrorRecord] }
                        
                        # Also check the job's error collection
                        if ($extractJob.Error) {
                            $jobErrors = @($jobErrors) + @($extractJob.Error)
                        }
                        
                        Remove-Job $extractJob -Force
                        
                        if ($jobState -eq "Completed") {
                            if ($jobErrors -and $jobErrors.Count -gt 0) {
                                Write-Log "Warning: Extraction completed with some errors/warnings"
                                foreach ($err in $jobErrors) {
                                    Write-DebugLog "Job warning/error: $err"
                                }
                            }
                            $extractionSuccess = $true
                            Update-Progress -Value 100 -Maximum 100
                            Update-Status "Extraction completed"
                            Write-Log "Extraction completed using 7Zip4PowerShell module ($cmdletName from $script:Expand7ZipModuleName)"
                        } elseif ($jobState -eq "Failed") {
                            $errorMsg = if ($jobErrors -and $jobErrors.Count -gt 0) { 
                                ($jobErrors | ForEach-Object { $_.ToString() } | Out-String).Trim()
                            } else { 
                                "Unknown error - job state: $jobState"
                            }
                            throw "Extraction job failed. Error: $errorMsg"
                        } else {
                            throw "Extraction job stopped unexpectedly. State: $jobState"
                        }
                    }
                } catch {
                    $extractionError = $_.Exception.Message
                    Write-Log "ERROR: PowerShell extraction failed: $extractionError"
                    Write-DebugLog "Exception type: $($_.Exception.GetType().Name)"
                    Write-DebugLog "Exception details: $($_.Exception | Format-List | Out-String)"
                    throw
                }
                
            } else {
                # TEMP: 7z.exe extraction path commented out
                throw "7Zip4PowerShell module is required. 7z.exe fallback is temporarily disabled."
                
                # Use 7z.exe with progress monitoring
                # # Run extraction with progress monitoring
                # $stdoutFile = Join-Path $env:TEMP "medicat_extract_stdout_$([System.Guid]::NewGuid().ToString().Substring(0,8)).txt"
                # $stderrFile = Join-Path $env:TEMP "medicat_extract_stderr_$([System.Guid]::NewGuid().ToString().Substring(0,8)).txt"
                # 
                # # Start process and monitor progress in real-time
                # $processInfo = New-Object System.Diagnostics.ProcessStartInfo
                # $processInfo.FileName = $sevenZipPath
                # $processInfo.Arguments = $extractArgs
                # $processInfo.UseShellExecute = $false
                # $processInfo.RedirectStandardOutput = $true
                # $processInfo.RedirectStandardError = $true
                # $processInfo.CreateNoWindow = $true
                # 
                # $extractProcess = New-Object System.Diagnostics.Process
                # $extractProcess.StartInfo = $processInfo
                # 
                # # Create string builders for output
                # $outputBuilder = New-Object System.Text.StringBuilder
                # $errorBuilder = New-Object System.Text.StringBuilder
                # 
                # # Start the process
                # $extractProcess.Start() | Out-Null
                # 
                # # Read output in real-time with progress updates
                # # Read synchronously but with UI updates
                # $stdOut = $extractProcess.StandardOutput
                # $stdErr = $extractProcess.StandardError
                # $lastPercent = 0
                # 
                # # Read output while process is running
                # while (-not $extractProcess.HasExited -or $stdOut.Peek() -ge 0 -or $stdErr.Peek() -ge 0) {
                #     # Process UI events to keep window responsive
                #     [System.Windows.Forms.Application]::DoEvents()
                #     
                #     # Read from stdout if available
                #     while ($stdOut.Peek() -ge 0) {
                #         try {
                #             $line = $stdOut.ReadLine()
                #             if ($line) {
                #                 $outputBuilder.AppendLine($line) | Out-Null
                #                 
                #                 # Parse progress percentage from 7z output
                #                 # 7z with -bb0 outputs progress as "  XX%" or similar
                #                 if ($line -match '(\d+)%') {
                #                     $percent = [int]$matches[1]
                #                     if ($percent -ge 0 -and $percent -le 100 -and $percent -ne $lastPercent) {
                #                         $lastPercent = $percent
                #                         $progressBar.Value = $percent
                #                         $statusLabel.Text = "Extracting... $percent%"
                #                         $progressBar.Refresh()
                #                         $statusLabel.Refresh()
                #                     }
                #                 }
                #             }
                #         } catch {
                #             break
                #         }
                #     }
                #     
                #     # Read from stderr if available
                #     while ($stdErr.Peek() -ge 0) {
                #         try {
                #             $line = $stdErr.ReadLine()
                #             if ($line) {
                #                 $errorBuilder.AppendLine($line) | Out-Null
                #             }
                #         } catch {
                #             break
                #         }
                #     }
                #     
                #     Start-Sleep -Milliseconds 100
                # }
                # 
                # # Wait for process to fully exit
                # $extractProcess.WaitForExit()
                # 
                # # Get final output
                # $output = $outputBuilder.ToString()
                # $errorOutput = $errorBuilder.ToString()
                # 
                # # Save output to files for logging
                # if ($output) {
                #     $output | Out-File -FilePath $stdoutFile -Encoding UTF8
                # }
                # if ($errorOutput) {
                #     $errorOutput | Out-File -FilePath $stderrFile -Encoding UTF8
                # }
                # 
                # # Clean up temp files
                # Remove-Item $stdoutFile -ErrorAction SilentlyContinue
                # Remove-Item $stderrFile -ErrorAction SilentlyContinue
                # 
                # if ($extractProcess.ExitCode -eq 0) {
                #     $extractionSuccess = $true
                #     Write-Log "Extraction completed using 7z.exe"
                # } else {
                #     $extractionError = "Exit code: $($extractProcess.ExitCode)"
                #     Write-Log "ERROR: Extraction failed with exit code: $($extractProcess.ExitCode)"
                #     
                #     # Log error output
                #     if ($errorOutput) {
                #         Write-Log "Extraction errors:"
                #         $errorOutput.Split("`n") | ForEach-Object { if ($_.Trim()) { Write-Log "  $_" } }
                #     }
                #     throw "Extraction failed"
                # }
            }
            
            if ($extractionSuccess) {
                $endTime = Get-Date
                $duration = ($endTime - $startTime).TotalMinutes
                Write-Log "Extraction completed in $([math]::Round($duration, 2)) minutes"
                Write-Log "MediCat archive extracted successfully"
                Update-Progress -Value 100 -Maximum 100
                Update-Status "Extraction completed successfully"
            }
            
        } catch {
            Write-Log "ERROR: Exception during extraction: $($_.Exception.Message)"
            if ($extractionError) {
                Write-Log "Additional error details: $extractionError"
            }
            Show-MessageBox -Message "Failed to extract MediCat archive.`n`nError: $($_.Exception.Message)`n`nCheck the log for details." -Title "Extraction Failed" -Icon "Error"
            return
        }
        
        # Copy final files
        Update-Status "Copying installer files..."
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
        
        Update-Status "Installation completed successfully!"
        Write-Log "MediCat installation completed successfully"
        Show-MessageBox -Message "MediCat has been installed successfully to $driveLetter" -Title "Installation Complete" -Icon "Information"
        
    } catch {
        Write-Log "ERROR: $($_.Exception.Message)"
        Show-MessageBox -Message "An error occurred during installation: $($_.Exception.Message)" -Title "Installation Error" -Icon "Error"
    } finally {
        $installButton.Enabled = $true
        $cancelButton.Enabled = $true
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
        
        Update-Status "Checking USB files..."
        Write-Log "Starting file check on $DriveLetter"
        
        # Download the MD5 file
        $md5File = "$DriveLetter\MedicatFiles.md5"
        $md5Url = "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/MedicatFiles.md5"
        
        Write-Log "Downloading MD5 file..."
        if (Invoke-Download -Url $md5Url -OutputPath $md5File) {
            Write-Log "MD5 file downloaded successfully"
            Write-Log "Parsing MD5 file and verifying files..."
            Update-Status "Verifying files with PowerShell..."
            
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
                Update-Status "File verification completed - All files OK"
                [System.Windows.Forms.MessageBox]::Show("File verification completed successfully. All $totalFiles files are intact.", "Verification Complete", "OK", "Information")
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
                
                Update-Status "File verification failed - $($failedFiles.Count) file(s) failed"
                
                # Ask if user wants to re-extract missing files
                $result = [System.Windows.Forms.MessageBox]::Show(
                    "File verification found $($failedFiles.Count) file(s) that failed verification.`n`nWould you like to attempt to re-extract the missing files from the archive?",
                    "Verification Failed - Re-extract Files?",
                    "YesNo",
                    "Question"
                )
                
                if ($result -eq "Yes") {
                    Write-Log "User requested re-extraction of failed files"
                    $reExtractResult = Start-ReExtractFiles -DriveLetter $DriveLetter -FailedFiles $failedFiles
                    if ($reExtractResult) {
                        Write-Log "Re-extraction completed successfully"
                        [System.Windows.Forms.MessageBox]::Show("Re-extraction completed. You may want to run file verification again to confirm.", "Re-extraction Complete", "OK", "Information")
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
            Update-Status "Failed to download verification file"
            [System.Windows.Forms.MessageBox]::Show("Failed to download the verification file. Please check your internet connection.", "Download Failed", "OK", "Error")
        }
        
    } catch {
        Write-Log "ERROR during file check: $($_.Exception.Message)"
        Update-Status "File check failed"
        [System.Windows.Forms.MessageBox]::Show("An error occurred during file verification: $($_.Exception.Message)", "Verification Error", "OK", "Error")
    } finally {
        $installButton.Enabled = $true
        $cancelButton.Enabled = $true
        $checkFilesButton.Enabled = $true
        Update-Status "Ready to install MediCat"
    }
}

function Start-ReExtractFiles {
    param($DriveLetter, $FailedFiles)
    
    try {
        Update-Status "Re-extracting missing files..."
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
            [System.Windows.Forms.MessageBox]::Show("7z.exe not found in system PATH or bin folder.`n`nPlease ensure 7-Zip is installed or the installer files are downloaded.", "7-Zip Not Found", "OK", "Error")
            return $false
        }
        
        # Find the source archive - check current directory first
        $archiveFile = Join-Path $PWD "MediCat.USB.v$script:MediCatVersion.7z"
        
        # If not found in current directory, prompt user to select the file
        if (-not (Test-Path $archiveFile)) {
            Write-Log "Archive not found in current directory, prompting user to select file..."
            Update-Status "Please select the MediCat archive file..."
            
            $fileDialog = New-Object System.Windows.Forms.OpenFileDialog
            $fileDialog.Filter = "7z Archive|*.7z|All Files|*.*"
            $fileDialog.Title = "Select MediCat Archive File (MediCat.USB.v$script:MediCatVersion.7z)"
            $fileDialog.CheckFileExists = $true
            $fileDialog.Multiselect = $false
            
            if ($fileDialog.ShowDialog() -eq "OK") {
                $archiveFile = $fileDialog.FileName
                Write-Log "User selected archive: $archiveFile"
            } else {
                Write-Log "User cancelled archive selection"
                [System.Windows.Forms.MessageBox]::Show("Archive file selection was cancelled. Re-extraction cannot proceed.", "Selection Cancelled", "OK", "Warning")
                return $false
            }
        }
        
        if (-not (Test-Path $archiveFile)) {
            Write-Log "ERROR: Source archive not found: $archiveFile"
            [System.Windows.Forms.MessageBox]::Show("Source archive not found: $archiveFile`nPlease ensure the archive exists.", "Archive Not Found", "OK", "Error")
            return $false
        }
        
        Write-Log "Found archive: $archiveFile"
        Write-Log "Found 7z: $sevenZipPath"
        
        # Normalize all file paths and create file list for batch extraction
        Write-Log "Preparing file list for batch extraction..."
        $filesToExtract = @()
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
            $outputContent = Get-Content "7z_output.tmp" -ErrorAction SilentlyContinue
            
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
            [System.Windows.Forms.MessageBox]::Show("Re-extraction completed with errors.`n`nExtracted: $extractedCount/$($FailedFiles.Count)`nFailed: $($failedExtract.Count)`n`nCheck the log for details.", "Re-extraction Complete with Errors", "OK", "Warning")
            return $false
        } else {
            Update-Status "Re-extraction completed successfully"
            return $true
        }
        
    } catch {
        Write-Log "ERROR during re-extraction: $($_.Exception.Message)"
        Update-Status "Re-extraction failed"
        [System.Windows.Forms.MessageBox]::Show("An error occurred during re-extraction: $($_.Exception.Message)", "Re-extraction Error", "OK", "Error")
        return $false
    }
}

$checkFilesButton.Add_Click({
    $selectedDrive = $driveComboBox.SelectedItem
    if ($selectedDrive) {
        $driveLetter = $selectedDrive.Substring(0, 2)
        Start-FileCheck -DriveLetter $driveLetter
    } else {
        Show-MessageBox -Message "Please select a USB drive first." -Title "No Drive Selected" -Icon "Warning"
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

# Initialize
Write-Log "MediCat Installer v$script:LocalVersion Started"
Write-Log "Running with Administrator privileges - Ventoy installation enabled"
Get-DriveList

# Check if bin files exist, download if missing
$requiredBinFiles = @("7z.exe", "7z.dll", "QuickSFV.exe", "GetInput.exe")
$missingFiles = @()
foreach ($file in $requiredBinFiles) {
    if (-not (Test-Path ".\bin\$file")) {
        $missingFiles += $file
    }
}

if ($missingFiles.Count -gt 0) {
    Write-Log "Missing bin files detected, downloading..."
    Download-BinFiles | Out-Null
} else {
    Write-Log "All required bin files present"
}

# Show form
$form.ShowDialog()
