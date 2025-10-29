# MediCat Installer - Technical Documentation

## Project Structure

```
MedicatInstaller.ps1          # Main PowerShell GUI installer
Medicat_Installer.bat         # Original batch script (legacy)
Test-7Zip4PowerShell-Install.ps1  # 7Zip4PowerShell module testing
Test-7zipWrapper-Progress.ps1     # 7zipWrapper module testing  
Test-PS7Zip-Install.ps1           # PS7Zip module testing
.gitignore                    # Git ignore rules for large files
.cursorrules                  # Development guidelines
```

## Core Functionality Overview

### 1. Administrator Elevation & Directory Management
- **Auto-elevation**: Detects admin privileges, requests UAC if needed
- **Working Directory**: Preserves script directory after elevation
- **Path Resolution**: Handles script path detection across execution contexts

### 2. Windows Forms GUI Components
- **Main Form**: 800x600 fixed dialog with centered positioning
- **Status Display**: Real-time status updates and progress bar
- **Drive Selection**: ComboBox with refresh capability
- **Configuration**: Checkboxes for hard drive visibility and formatting options
- **Action Buttons**: Install, Cancel, Check Files, Refresh Drives

### 3. Drive Management System
- **Drive Detection**: Uses WMI and PowerShell cmdlets for comprehensive drive enumeration
- **Filtering Logic**: 
  - Always excludes C: drive
  - Optional HDD inclusion via checkbox
  - Automatic VHD/VHDX detection
  - USB drive prioritization
- **Ventoy Detection**: Identifies existing VTOYEFI partitions (32MB FAT EFI system)

### 4. Ventoy Installation & Management
- **Version Detection**: GitHub API integration for latest Ventoy releases
- **Download System**: Automatic download and extraction of Ventoy
- **Installation Modes**:
  - Fresh Install: `VTOYCLI /I /Drive:X: /NOUSBCheck`
  - Upgrade Mode: `VTOYCLI /U /Drive:X:`
- **Drive Formatting**: Optional NTFS formatting (user-controlled)

### 5. Archive Extraction System
- **Module Priority**: Prefers PowerShell modules over external executables
- **7Zip4PowerShell**: Primary extraction method with progress monitoring
- **Progress Tracking**: File system monitoring for real-time progress updates
- **Background Processing**: Extraction runs in background jobs to maintain UI responsiveness

### 6. Comprehensive Logging System
- **Dual Output**: Simultaneous logging to file and UI
- **Message Types**: Status, Progress, Debug, MessageBox interactions
- **Throttling**: Progress updates throttled to prevent log spam
- **Persistence**: All operations logged to `medicat_download.log`

## Function Reference

### Core Functions

#### `Write-Log($Message)`
- **Purpose**: General logging with UI and file output
- **Usage**: All status updates, errors, and important events
- **Thread Safety**: Uses `form.Invoke()` for UI updates

#### `Write-DebugLog($Message)`
- **Purpose**: Debug-only logging (respects `$script:DebugMode`)
- **Usage**: Detailed debugging information
- **Behavior**: Always logs to file, UI logging controlled by debug flag

#### `Update-Status($Message)`
- **Purpose**: Updates main status label
- **Usage**: User-facing status messages
- **Side Effect**: Automatically logs status to file

#### `Update-Progress($Value, $Maximum)`
- **Purpose**: Updates progress bar with throttling
- **Usage**: Long-running operations
- **Throttling**: Only logs progress changes ≥1% or at 0%/100%

#### `Show-MessageBox($Message, $Title, $Buttons, $Icon)`
- **Purpose**: Centralized message box with logging
- **Usage**: User confirmations and error dialogs
- **Logging**: Captures all MessageBox interactions and results

### Drive Management Functions

#### `Get-DriveList()`
- **Purpose**: Enumerates and filters available drives
- **Logic**: 
  - Excludes C: drive
  - Respects "Show hard drives" checkbox
  - Detects VHD/VHDX files
  - Defaults to I: drive (debug mode)
- **Returns**: Populated ComboBox with drive options

#### `Refresh-DriveList()`
- **Purpose**: Refreshes drive list and updates UI
- **Usage**: Manual refresh button and automatic refresh

#### `Test-VentoyInstalled($DriveLetter)`
- **Purpose**: Detects existing Ventoy installation
- **Detection Methods**:
  - VTOYEFI partition detection (32MB FAT EFI system)
  - Ventoy folder presence check
- **Returns**: Boolean indicating Ventoy presence

### Installation Functions

#### `Install-Ventoy($DriveLetter, [-Upgrade])`
- **Purpose**: Downloads and installs/upgrades Ventoy
- **Parameters**:
  - `$DriveLetter`: Target drive letter
  - `-Upgrade`: Switch for non-destructive upgrade
- **Process**: Download → Extract → Install via CLI
- **Error Handling**: Comprehensive error capture and logging

#### `Start-MediatInstallation()`
- **Purpose**: Main installation workflow coordinator
- **Logic Flow**:
  1. Drive validation
  2. Format decision (based on checkbox)
  3. Ventoy installation/upgrade
  4. MediCat archive extraction
- **Error Recovery**: Handles failures at each step

### Utility Functions

#### `Download-BinFiles()`
- **Purpose**: Downloads required binary files
- **Files**: Ventoy, 7z executables, etc.
- **Validation**: MD5 checksum verification

#### `Test-InternetConnection()`
- **Purpose**: Validates internet connectivity
- **Usage**: Pre-download validation

#### `Invoke-Download($Url, $Destination)`
- **Purpose**: Generic file download with progress
- **Features**: Progress tracking, error handling

## Configuration Variables

### Script-Level Variables
- `$script:LogFile` - Log file path (`medicat_download.log`)
- `$script:LastLoggedProgress` - Progress throttling state
- `$script:DownloadPath` - Download directory
- `$script:MediCatVersion` - MediCat version (`21.12`)
- `$script:LocalVersion` - Installer version (`1.0.0`)
- `$script:DebugMode` - Debug logging toggle (`$true`)

### UI Configuration
- **Form Size**: 800x600 pixels
- **Font**: Arial for labels, Consolas for log display
- **Colors**: DarkBlue title, LightGreen install button
- **Layout**: Fixed dialog with padding

## Error Handling Patterns

### Standard Error Handling
```powershell
try {
    # Operation
    Write-Log "Operation completed successfully"
} catch {
    Write-Log "ERROR: $($_.Exception.Message)"
    Write-DebugLog "Exception type: $($_.Exception.GetType().Name)"
    Write-DebugLog "Exception details: $($_.Exception.ToString())"
    # Recovery or exit logic
}
```

### Background Job Error Handling
```powershell
$job = Start-Job -ScriptBlock { ... }
# Monitor job and handle errors
if ($job.State -eq "Failed") {
    Write-Log "ERROR: Background job failed"
    $job | Receive-Job | ForEach-Object { Write-DebugLog $_ }
}
```

## Thread Safety Considerations

### UI Updates from Background Jobs
```powershell
# Correct pattern for thread-safe UI updates
$form.Invoke([System.Action[string]]{
    param($msg)
    $logTextBox.AppendText("$msg`r`n")
    $logTextBox.ScrollToCaret()
}, $message)
```

### Progress Updates
```powershell
# Progress updates must be thread-safe
$form.Invoke([System.Action[int,int]]{
    param($value, $max)
    $progressBar.Maximum = $max
    $progressBar.Value = $value
}, $progressValue, $progressMaximum)
```

## Testing Scenarios

### Drive Testing
- USB drives (various sizes)
- VHD/VHDX files
- Hard drives (with/without checkbox)
- C: drive exclusion

### Installation Testing
- Fresh Ventoy installation
- Ventoy upgrade scenarios
- Format vs. no-format options
- MediCat extraction (24GB archive)

### Error Testing
- Network connectivity issues
- Insufficient disk space
- Ventoy installation failures
- Archive corruption scenarios

### UI Testing
- Progress bar accuracy
- Log scrolling behavior
- Button state management
- Message box interactions

## Performance Considerations

### Background Jobs
- Long-running operations use background jobs
- Proper job cleanup and monitoring
- UI remains responsive during operations

### Progress Throttling
- Progress updates throttled to prevent UI lag
- Log file updates limited to prevent disk I/O issues

### Memory Management
- Background jobs properly disposed
- Large file operations streamed where possible
- UI updates batched to prevent excessive refreshes

## Security Considerations

### Administrator Privileges
- Required for disk operations
- Automatic UAC elevation
- Working directory preservation

### File Integrity
- MD5 verification for downloads
- Safe extraction practices
- Confirmation dialogs for destructive operations

### Logging Security
- All operations logged for audit trail
- Sensitive data handling considerations
- Log file access controls
