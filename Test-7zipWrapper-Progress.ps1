# Test script for 7zipWrapper module with progress bar GUI
# Tests extraction progress monitoring and GUI integration

param(
    [Parameter(Position=0)]
    [string]$ArchivePath = $null,
    [Parameter(Position=1)]
    [string]$ExtractPath = $null
)

# Setup logging
$logFile = "test_7zipwrapper_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
$script:LogEnabled = $true

function Write-Log {
    param($Message, [switch]$NoConsole)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] $Message"
    
    if (-not $NoConsole) {
        Write-Host $logMessage
    }
    
    if ($script:LogEnabled) {
        try {
            Add-Content -Path $logFile -Value $logMessage -ErrorAction SilentlyContinue
        } catch {
            # If logging fails, continue anyway
        }
    }
}

function Write-DebugLog {
    param($Message)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] DEBUG: $Message"
    
    # Always log to file, optionally show in console
    if ($script:LogEnabled) {
        try {
            Add-Content -Path $logFile -Value $logMessage -ErrorAction SilentlyContinue
        } catch { }
    }
    
    Write-Host "DEBUG: $Message" -ForegroundColor Gray
}

# Initialize log file
$logHeader = @"
========================================
7zipWrapper Progress Test Log
Started: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Archive: $ArchivePath
Destination: $ExtractPath
========================================
"@
try {
    Add-Content -Path $logFile -Value $logHeader -ErrorAction SilentlyContinue
    Write-Host "Logging to: $logFile" -ForegroundColor Cyan
} catch {
    Write-Host "WARNING: Could not initialize log file: $logFile" -ForegroundColor Yellow
    $script:LogEnabled = $false
}

Write-Log "=== 7zipWrapper Module Progress Test ==="
Write-Log ""

# Check if module is installed
$moduleName = "7zipWrapper"
$module = Get-Module -ListAvailable -Name $moduleName -ErrorAction SilentlyContinue

if (-not $module) {
    Write-Log "7zipWrapper module not found. Attempting to install..."
    try {
        # Check for NuGet provider
        $nuget = Get-PackageProvider -Name NuGet -ErrorAction SilentlyContinue
        if (-not $nuget) {
            Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Force -Scope CurrentUser | Out-Null
        }
        
        # Set PSGallery to trusted if needed
        try {
            $repo = Get-PSRepository -Name PSGallery -ErrorAction SilentlyContinue
            if ($repo -and $repo.InstallationPolicy -eq "Untrusted") {
                Set-PSRepository -Name PSGallery -InstallationPolicy Trusted -ErrorAction Stop
            }
        } catch { }
        
        Install-Module -Name $moduleName -Force -Scope CurrentUser -ErrorAction Stop
        Write-Log "Module installed successfully"
        Start-Sleep -Seconds 1
        $module = Get-Module -ListAvailable -Name $moduleName -ErrorAction SilentlyContinue
    } catch {
        Write-Log "ERROR: Failed to install module: $($_.Exception.Message)"
        exit 1
    }
}

if ($module) {
    Write-Log "[OK] Module found: $($module.Name) v$($module.Version)"
    Write-Log ""
    
    # Import module
    try {
        Import-Module $moduleName -Force -ErrorAction Stop
        Write-Log "Module imported successfully"
    } catch {
        Write-Log "ERROR: Failed to import module: $($_.Exception.Message)"
        exit 1
    }
    
    # List available cmdlets
    Write-Log "Available cmdlets:"
    $cmdlets = Get-Command -Module $moduleName
    foreach ($cmd in $cmdlets) {
        Write-Log "  - $($cmd.Name)"
    }
    Write-Log ""
    
    # Check Expand-7zArchive parameters
    $expandCmd = Get-Command Expand-7zArchive -ErrorAction SilentlyContinue
    if ($expandCmd) {
        Write-Log "=== Expand-7zArchive Parameters ==="
        $params = $expandCmd.Parameters.Keys | Sort-Object
        foreach ($param in $params) {
            $paramInfo = $expandCmd.Parameters[$param]
            $required = if ($paramInfo.IsMandatory) { "(Required)" } else { "" }
            $types = if ($paramInfo.ParameterType.IsArray) { "$($paramInfo.ParameterType.GetElementType().Name)[]" } else { $paramInfo.ParameterType.Name }
            Write-Log "  -$param : $types $required"
        }
        Write-Log ""
        
        # Check for progress-related parameters
        $progressParams = $params | Where-Object { 
            $_ -like '*progress*' -or 
            $_ -like '*verbose*' -or 
            $_ -like '*passthru*' 
        }
        if ($progressParams) {
            Write-Log "Progress-related parameters found: $($progressParams -join ', ')"
        } else {
            Write-Log "No explicit progress parameters found"
        }
        Write-Log ""
    }
    
    # Test extraction with GUI progress bar
    if ($ArchivePath -and $ExtractPath) {
        Write-Log "=== Testing Extraction with Progress Bar ==="
        Write-Log "Archive: $ArchivePath"
        Write-Log "Destination: $ExtractPath"
        Write-Log ""
        
        # Create a simple GUI form for progress
        Add-Type -AssemblyName System.Windows.Forms
        Add-Type -AssemblyName System.Drawing
        
        $form = New-Object System.Windows.Forms.Form
        $form.Text = "7zipWrapper Extraction Progress"
        $form.Size = New-Object System.Drawing.Size(500, 200)
        $form.StartPosition = "CenterScreen"
        $form.FormBorderStyle = "FixedDialog"
        $form.MaximizeBox = $false
        $form.MinimizeBox = $false
        
        # Status label
        $statusLabel = New-Object System.Windows.Forms.Label
        $statusLabel.Location = New-Object System.Drawing.Point(20, 20)
        $statusLabel.Size = New-Object System.Drawing.Size(460, 30)
        $statusLabel.Text = "Preparing extraction..."
        $form.Controls.Add($statusLabel)
        
        # Progress bar
        $progressBar = New-Object System.Windows.Forms.ProgressBar
        $progressBar.Location = New-Object System.Drawing.Point(20, 60)
        $progressBar.Size = New-Object System.Drawing.Size(460, 30)
        $progressBar.Style = "Continuous"
        $form.Controls.Add($progressBar)
        
        # Log textbox
        $logBox = New-Object System.Windows.Forms.TextBox
        $logBox.Location = New-Object System.Drawing.Point(20, 100)
        $logBox.Size = New-Object System.Drawing.Size(460, 60)
        $logBox.Multiline = $true
        $logBox.ScrollBars = "Vertical"
        $logBox.ReadOnly = $true
        $logBox.Font = New-Object System.Drawing.Font("Consolas", 8)
        $form.Controls.Add($logBox)
        
        # Function to update progress (must be defined as script block accessible to Invoke)
        $script:updateProgressAction = {
            param($Percent, $Message)
            if ($progressBar -and $statusLabel) {
                $progressBar.Value = $Percent
                $statusLabel.Text = $Message
                $form.Refresh()
                [System.Windows.Forms.Application]::DoEvents()
            }
            # Log progress updates
            Write-Log "PROGRESS: $Percent% - $Message" -NoConsole
        }
        
        # Function to append log
        $script:appendLogAction = {
            param($Message)
            if ($logBox) {
                $logBox.AppendText("$Message`r`n")
                $logBox.SelectionStart = $logBox.Text.Length
                $logBox.ScrollToCaret()
                $form.Refresh()
            }
            # Log all output
            Write-Log "OUTPUT: $Message" -NoConsole
        }
        
        # Wrapper functions for Invoke calls
        $updateProgress = $script:updateProgressAction
        $appendLog = $script:appendLogAction
        
        # Show form
        $form.Add_Shown({ $form.Activate() })
        $form.Show()
        $form.Refresh()
        [System.Windows.Forms.Application]::DoEvents()
        
        Write-Log "GUI form displayed. Starting extraction..."
        Write-Log ""
        
        # Test 1: Try with Verbose output to capture progress
        Write-Log "Test 1: Extraction with Verbose parameter"
        $progressBar.Maximum = 100
        $progressBar.Value = 0
        $statusLabel.Text = "Starting extraction..."
        $logBox.AppendText("Starting extraction...`r`n")
        
        try {
            Write-Log "Creating extraction job..."
            Write-Log "Archive: $ArchivePath"
            Write-Log "Destination: $ExtractPath"
            
            # Verify archive exists
            if (-not (Test-Path $ArchivePath)) {
                throw "Archive file not found: $ArchivePath"
            }
            Write-Log "Archive file exists: $((Get-Item $ArchivePath).Length / 1GB) GB"
            
            # Create destination if needed
            if (-not (Test-Path $ExtractPath)) {
                Write-Log "Creating destination directory: $ExtractPath"
                New-Item -Path $ExtractPath -ItemType Directory -Force | Out-Null
            }
            
            $extractJob = Start-Job -ScriptBlock {
                param($Archive, $Dest, $Module)
                $VerbosePreference = "Continue"
                $ErrorActionPreference = "Continue"
                Import-Module $Module -Force
                
                # Use correct parameter names: -ArchivePath and -Destination
                # Use -Switches "x" to extract with full paths (instead of "e" which extracts without paths)
                # "-aoa" = overwrite all files without prompt
                try {
                    Write-Host "Starting extraction with path preservation (x mode)..."
                    # Capture all streams and use x mode (extract with full paths)
                    Expand-7zArchive -ArchivePath $Archive -Destination $Dest -Switches "x -aoa" -Verbose 4>&1 5>&1
                } catch {
                    Write-Error "Extraction error: $($_.Exception.Message)"
                    throw
                }
            } -ArgumentList $ArchivePath, $ExtractPath, $moduleName
            
            Write-Log "Job started (ID: $($extractJob.Id))"
            Write-Log "Job state: $($extractJob.State)"
            
            $startTime = Get-Date
            $lastPercent = 0
            $lastUpdate = Get-Date
            $iterationCount = 0
            
            # Monitor job and update progress
            while ($extractJob.State -eq "Running") {
                $iterationCount++
                [System.Windows.Forms.Application]::DoEvents()
                
                # Log job state periodically
                if ($iterationCount % 10 -eq 0) {
                    Write-DebugLog "Job still running (iteration $iterationCount, state: $($extractJob.State))"
                }
                
                # Try to get all output streams from job
                $output = Receive-Job $extractJob -ErrorAction SilentlyContinue 2>&1
                
                if ($output) {
                    foreach ($line in $output) {
                        $lineStr = $line.ToString()
                        if ($lineStr -and $lineStr.Trim()) {
                            # Invoke appendLog properly
                            $form.Invoke([System.Action[string]]{
                                param($msg)
                                & $script:appendLogAction $msg
                            }, $lineStr)
                            Write-DebugLog "Job output: $lineStr"
                            
                            # Try to parse progress from verbose output
                            if ($lineStr -match '(\d+)%' -or $lineStr -match '(\d+)\s*%') {
                                $percent = [int]$matches[1]
                                if ($percent -ge 0 -and $percent -le 100 -and $percent -ne $lastPercent) {
                                    $form.Invoke([System.Action[int,string]]{
                                        param($pct, $msg)
                                        & $script:updateProgressAction $pct $msg
                                    }, $percent, "Extracting... $percent%")
                                    $lastPercent = $percent
                                    Write-DebugLog "Parsed progress: $percent%"
                                }
                            } elseif ($lineStr -like '*extracting*' -or $lineStr -like '*extract*' -or $lineStr -like '*processing*') {
                                $form.Invoke([System.Action[int,string]]{
                                    param($pct, $msg)
                                    & $script:updateProgressAction $pct $msg
                                }, $lastPercent, $lineStr)
                            }
                        }
                    }
                }
                
                # Fallback: Monitor destination directory size
                if ((Get-Date) - $lastUpdate -gt [TimeSpan]::FromSeconds(2)) {
                    if (Test-Path $ExtractPath) {
                        try {
                            $files = Get-ChildItem -Path $ExtractPath -Recurse -File -ErrorAction SilentlyContinue
                            $fileCount = $files.Count
                            if ($fileCount -gt 0) {
                                $estimatedPercent = [math]::Min(95, $fileCount / 100)
                                # Fix: Use proper delegate invocation
                                $form.Invoke([System.Action[int,string]]{
                                    param($Percent, $Message)
                                    & $script:updateProgressAction $Percent $Message
                                }, $estimatedPercent, "Extracted $fileCount files...")
                                Write-DebugLog "Directory monitoring: $fileCount files found (estimated $estimatedPercent%)"
                            }
                        } catch {
                            Write-DebugLog "Error monitoring directory: $($_.Exception.Message)"
                            Write-DebugLog "Stack trace: $($_.Exception.StackTrace)"
                        }
                    }
                    $lastUpdate = Get-Date
                }
                
                Start-Sleep -Milliseconds 500
            }
            
            # Wait for completion (with timeout logging)
            Write-Log "Waiting for job to complete..."
            $waitStart = Get-Date
            Wait-Job $extractJob | Out-Null
            $waitDuration = ((Get-Date) - $waitStart).TotalSeconds
            Write-Log "Job wait completed in $([math]::Round($waitDuration, 2)) seconds"
            
            # Get final job state before receiving output (state is lost after Remove-Job)
            $finalJobState = $extractJob.State
            Write-Log "Final job state: $finalJobState"
            
            # Receive all remaining output (including verbose stream)
            $finalOutput = Receive-Job $extractJob -ErrorAction SilentlyContinue 2>&1
            
            if ($finalOutput) {
                Write-Log "Received $($finalOutput.Count) lines of final output"
                foreach ($line in $finalOutput) {
                    $lineStr = if ($line -is [System.Management.Automation.ErrorRecord]) { 
                        $line.Exception.Message 
                    } else { 
                        $line.ToString() 
                    }
                    if ($lineStr -and $lineStr.Trim()) {
                        $form.Invoke([System.Action[string]]{
                            param($msg)
                            & $script:appendLogAction $msg
                        }, $lineStr)
                        Write-Log "FINAL OUTPUT: $lineStr"
                    }
                }
            } else {
                Write-Log "No final output received from job"
            }
            
            # Get any errors from the job
            if ($extractJob.Error -and $extractJob.Error.Count -gt 0) {
                Write-Log "Job has $($extractJob.Error.Count) error(s)"
                foreach ($error in $extractJob.Error) {
                    $errorStr = $error.ToString()
                    $form.Invoke([System.Action[string]]{
                        param($msg)
                        & $script:appendLogAction $msg
                    }, "ERROR: $errorStr")
                    Write-Log "JOB ERROR: $errorStr"
                    if ($error.Exception) {
                        Write-Log "JOB ERROR EXCEPTION: $($error.Exception.Message)"
                        if ($error.Exception.InnerException) {
                            Write-Log "JOB ERROR INNER: $($error.Exception.InnerException.Message)"
                        }
                        if ($error.Exception.StackTrace) {
                            Write-Log "JOB ERROR STACK: $($error.Exception.StackTrace)"
                        }
                    }
                }
            } else {
                Write-Log "No errors reported by job"
            }
            
            Remove-Job $extractJob -Force
            Write-Log "Job removed"
            
            $endTime = Get-Date
            $duration = ($endTime - $startTime).TotalSeconds
            Write-Log "Total extraction duration: $([math]::Round($duration, 2)) seconds"
            
            if ($finalJobState -eq "Completed") {
                $form.Invoke([System.Action[int,string]]{
                    param($pct, $msg)
                    & $script:updateProgressAction $pct $msg
                }, 100, "Extraction completed successfully")
                $form.Invoke([System.Action[string]]{
                    param($msg)
                    & $script:appendLogAction $msg
                }, "Extraction completed in $([math]::Round($duration, 2)) seconds")
                Write-Log "Extraction completed successfully in $([math]::Round($duration, 2)) seconds"
            } elseif ($finalJobState -eq "Failed") {
                $form.Invoke([System.Action[int,string]]{
                    param($pct, $msg)
                    & $script:updateProgressAction $pct $msg
                }, 0, "Extraction failed")
                $form.Invoke([System.Action[string]]{
                    param($msg)
                    & $script:appendLogAction $msg
                }, "ERROR: Job state - $finalJobState")
                Write-Log "ERROR: Extraction failed - Job state: $finalJobState"
            } else {
                $form.Invoke([System.Action[int,string]]{
                    param($pct, $msg)
                    & $script:updateProgressAction $pct $msg
                }, 0, "Extraction stopped unexpectedly")
                $form.Invoke([System.Action[string]]{
                    param($msg)
                    & $script:appendLogAction $msg
                }, "WARNING: Job state - $finalJobState")
                Write-Log "WARNING: Unexpected job state: $finalJobState"
            }
            
        } catch {
            $form.Invoke([System.Action[int,string]]{
                param($pct, $msg)
                & $script:updateProgressAction $pct $msg
            }, 0, "Error occurred")
            $form.Invoke([System.Action[string]]{
                param($msg)
                & $script:appendLogAction $msg
            }, "ERROR: $($_.Exception.Message)")
            Write-Log "ERROR: Exception during extraction: $($_.Exception.Message)"
            Write-Log "Exception type: $($_.Exception.GetType().Name)"
            if ($_.Exception.StackTrace) {
                Write-Log "Stack trace: $($_.Exception.StackTrace)"
            }
        }
        
        Write-Log ""
        Write-Log "Test completed. Check log file for details: $logFile"
        
        # Keep form open until closed
        $form.Add_FormClosing({
            param($sender, $e)
            $e.Cancel = $false
        })
        
        # Run form application loop
        [System.Windows.Forms.Application]::Run($form)
        
    } else {
        Write-Log "=== Usage ==="
        Write-Log "To test extraction with progress bar, run:"
        Write-Log "  .\Test-7zipWrapper-Progress.ps1 -ArchivePath 'C:\path\to\archive.7z' -ExtractPath 'C:\path\to\extract'"
        Write-Log ""
        Write-Log "Example:"
        Write-Log "  .\Test-7zipWrapper-Progress.ps1 -ArchivePath '.\MediCat.USB.v21.12.7z' -ExtractPath '.\test_extract'"
    }
    
} else {
    Write-Log "ERROR: Module not found or could not be imported"
    exit 1
}

# Write footer to log
$logFooter = @"

========================================
Test Complete: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Log file: $logFile
========================================
"@
if ($script:LogEnabled) {
    try {
        Add-Content -Path $logFile -Value $logFooter -ErrorAction SilentlyContinue
    } catch { }
}

Write-Log ""
Write-Log "=== Test Complete ==="
Write-Host ""
Write-Host "All output has been logged to: $logFile" -ForegroundColor Cyan

