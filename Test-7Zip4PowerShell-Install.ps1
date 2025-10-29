# Test script for installing/uninstalling 7Zip4PowerShell module
# Usage: .\Test-7Zip4PowerShell-Install.ps1 [install|uninstall|status]
#   install   - Install 7Zip4PowerShell module
#   uninstall - Uninstall 7Zip4PowerShell module  
#   status    - Show current installation status (default)

param(
    [Parameter(Position=0)]
    [ValidateSet("install", "uninstall", "status")]
    [string]$Action = "status"
)

Write-Host "=== 7Zip4PowerShell Module Installation Tester ===" -ForegroundColor Cyan
Write-Host "Action: $Action" -ForegroundColor White
Write-Host ""

# Check current admin status
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdmin = $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin -and ($Action -eq "install" -or $Action -eq "uninstall")) {
    Write-Host "[WARNING] Not running as Administrator" -ForegroundColor Yellow
    Write-Host "Module installation/uninstallation may require admin privileges" -ForegroundColor Yellow
    Write-Host ""
}

# Function to find module name (handles case variations)
function Get-ModuleName {
    $possibleNames = @("7Zip4PowerShell", "7Zip4Powershell")
    foreach ($name in $possibleNames) {
        $module = Get-Module -ListAvailable -Name $name -ErrorAction SilentlyContinue
        if ($module) {
            return $module.Name
        }
    }
    # Try case-insensitive search
    $allModules = Get-Module -ListAvailable -ErrorAction SilentlyContinue
    $found = $allModules | Where-Object { $_.Name -like "*7zip4*" -or $_.Name -like "*7Zip4*" } | Select-Object -First 1
    if ($found) {
        return $found.Name
    }
    return $possibleNames[0]  # Default to "7Zip4PowerShell"
}

# Function to check module status
function Test-ModuleInstalled {
    param($ModuleName)
    # Try exact name first
    $module = Get-Module -ListAvailable -Name $ModuleName -ErrorAction SilentlyContinue
    if (-not $module) {
        # Try case-insensitive
        $allModules = Get-Module -ListAvailable -ErrorAction SilentlyContinue
        $module = $allModules | Where-Object { $_.Name -eq $ModuleName -or $_.Name -like "*7zip4*" } | Select-Object -First 1
    }
    if ($module) {
        Write-Host "[OK] Module installed: $($module.Name) v$($module.Version)" -ForegroundColor Green
        Write-Host "     Path: $($module.Path)" -ForegroundColor Gray
        return $true
    } else {
        Write-Host "[NOT FOUND] Module not installed: $ModuleName" -ForegroundColor DarkGray
        return $false
    }
}

# Function to list PSGallery repository status
function Show-PSGalleryStatus {
    Write-Host "Checking PowerShell Gallery repository..." -ForegroundColor Yellow
    try {
        $gallery = Get-PSRepository -Name "PSGallery" -ErrorAction SilentlyContinue
        if ($gallery) {
            Write-Host "[OK] PSGallery repository configured:" -ForegroundColor Green
            Write-Host "     Installation Policy: $($gallery.InstallationPolicy)" -ForegroundColor White
            Write-Host "     Source Location: $($gallery.SourceLocation)" -ForegroundColor White
            
            if ($gallery.InstallationPolicy -eq "Untrusted") {
                Write-Host "[WARN] PSGallery is untrusted. May need to set as trusted:" -ForegroundColor Yellow
                Write-Host "     Set-PSRepository -Name PSGallery -InstallationPolicy Trusted" -ForegroundColor Cyan
            }
        } else {
            Write-Host "[WARN] PSGallery repository not found" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "[ERROR] Could not check PSGallery: $($_.Exception.Message)" -ForegroundColor Red
    }
    Write-Host ""
}

# Function to find module in gallery (tries multiple name variations)
function Test-ModuleInGallery {
    param($ModuleName)
    Write-Host "Searching for $ModuleName in PowerShell Gallery..." -ForegroundColor Yellow
    $possibleNames = @("7Zip4PowerShell", "7Zip4Powershell")
    foreach ($name in $possibleNames) {
        try {
            $module = Find-Module -Name $name -Repository PSGallery -ErrorAction Stop
            Write-Host "[OK] Module found in gallery:" -ForegroundColor Green
            Write-Host "     Name: $($module.Name)" -ForegroundColor White
            Write-Host "     Version: $($module.Version)" -ForegroundColor White
            Write-Host "     Description: $($module.Description)" -ForegroundColor White
            Write-Host "     Author: $($module.Author)" -ForegroundColor White
            Write-Host "     Published: $($module.PublishedDate)" -ForegroundColor White
            return $module.Name  # Return the actual module name found
        } catch {
            # Try next name
            continue
        }
    }
    Write-Host "[ERROR] Module not found in gallery with any known name variation" -ForegroundColor Red
    return $null
}

# Determine actual module name (check installed first, then default)
$moduleName = Get-ModuleName

# Show initial status
Write-Host "=== Module Status ===" -ForegroundColor Cyan
$wasInstalled = Test-ModuleInstalled -ModuleName $moduleName
Write-Host ""

# Execute requested action
switch ($Action.ToLower()) {
    "status" {
        # Show detailed status
        Show-PSGalleryStatus
        
        if ($wasInstalled) {
            Write-Host "=== Module Details ===" -ForegroundColor Cyan
            $module = Get-Module -ListAvailable -Name $moduleName -ErrorAction SilentlyContinue
            if (-not $module) {
                $allModules = Get-Module -ListAvailable -ErrorAction SilentlyContinue
                $module = $allModules | Where-Object { $_.Name -like "*7zip4*" } | Select-Object -First 1
            }
            Write-Host "Name: $($module.Name)" -ForegroundColor White
            Write-Host "Version: $($module.Version)" -ForegroundColor White
            Write-Host "Path: $($module.Path)" -ForegroundColor White
            
            # Try to import and show commands
            try {
                Import-Module $module.Name -Force -ErrorAction Stop
                $imported = Get-Module $module.Name
                Write-Host ""
                Write-Host "Exported commands: $($imported.ExportedCommands.Keys.Count)" -ForegroundColor White
                $commands = $imported.ExportedCommands.Keys -join ", "
                Write-Host "  $commands" -ForegroundColor Gray
                
                # Check for common expansion cmdlets
                $expandCmdNames = @("Expand-7Zip", "Expand-SevenZip")
                $foundCmd = $null
                foreach ($cmdName in $expandCmdNames) {
                    $expandCmd = Get-Command $cmdName -ErrorAction SilentlyContinue
                    if ($expandCmd -and $expandCmd.ModuleName -eq $module.Name) {
                        $foundCmd = $expandCmd
                        break
                    }
                }
                
                if ($foundCmd) {
                    Write-Host ""
                    Write-Host "$($foundCmd.Name) cmdlet found" -ForegroundColor Green
                    $params = $foundCmd.Parameters.Keys | Where-Object { $_ -notlike 'Verbose*' -and $_ -notlike 'Debug*' -and $_ -notlike 'Error*' -and $_ -notlike 'Warning*' -and $_ -notlike 'Information*' -and $_ -notlike 'Out*' -and $_ -notlike 'Pipeline*' -and $_ -notlike 'WhatIf' -and $_ -notlike 'Confirm' }
                    Write-Host "  Parameters: $($params -join ', ')" -ForegroundColor Gray
                }
                
                Remove-Module $module.Name -Force -ErrorAction SilentlyContinue
            } catch {
                Write-Host "[WARN] Could not import module: $($_.Exception.Message)" -ForegroundColor Yellow
            }
        }
    }
    
    "uninstall" {
        Write-Host "=== Uninstall 7Zip4PowerShell ===" -ForegroundColor Cyan
        
        if ($wasInstalled) {
            # Get actual installed module name
            $installedModule = Get-Module -ListAvailable -ErrorAction SilentlyContinue | Where-Object { $_.Name -like "*7zip4*" } | Select-Object -First 1
            $moduleToUninstall = if ($installedModule) { $installedModule.Name } else { $moduleName }
            
            Write-Host "Uninstalling $moduleToUninstall module..." -ForegroundColor Yellow
            try {
                Uninstall-Module -Name $moduleToUninstall -AllVersions -Force -ErrorAction Stop
                Write-Host "[OK] Module uninstalled successfully" -ForegroundColor Green
                
                # Verify uninstall
                Start-Sleep -Seconds 1
                $stillInstalled = Test-ModuleInstalled -ModuleName $moduleToUninstall
                if ($stillInstalled) {
                    Write-Host "[WARN] Module still appears to be installed after uninstall" -ForegroundColor Yellow
                    exit 1
                } else {
                    Write-Host "[OK] Uninstall verified - module no longer found" -ForegroundColor Green
                    exit 0
                }
            } catch {
                Write-Host "[ERROR] Uninstall failed: $($_.Exception.Message)" -ForegroundColor Red
                Write-Host "       This may require administrator privileges" -ForegroundColor Yellow
                exit 1
            }
        } else {
            Write-Host "[SKIP] Module not installed, nothing to uninstall" -ForegroundColor DarkGray
            exit 0
        }
    }
    
    "install" {
        Write-Host "=== Install 7Zip4PowerShell ===" -ForegroundColor Cyan
        
        # Show PSGallery status
        Show-PSGalleryStatus
        
        # Check if module exists in gallery and get actual name
        $galleryModuleName = Test-ModuleInGallery -ModuleName $moduleName
        Write-Host ""
        
        if (-not $galleryModuleName) {
            Write-Host "[ERROR] Cannot proceed - 7Zip4PowerShell not found in PowerShell Gallery" -ForegroundColor Red
            exit 1
        }
        
        if ($wasInstalled) {
            $installedModule = Get-Module -ListAvailable -ErrorAction SilentlyContinue | Where-Object { $_.Name -like "*7zip4*" } | Select-Object -First 1
            Write-Host "[INFO] Module already installed (v$($installedModule.Version))" -ForegroundColor Yellow
            $response = Read-Host "Reinstall? (Y/N)"
            if ($response -ne "Y" -and $response -ne "y") {
                Write-Host "Installation cancelled" -ForegroundColor Yellow
                exit 0
            }
            Write-Host "Uninstalling existing version first..." -ForegroundColor Yellow
            try {
                Uninstall-Module -Name $installedModule.Name -AllVersions -Force -ErrorAction Stop
                Start-Sleep -Seconds 1
            } catch {
                Write-Host "[WARN] Could not uninstall existing version: $($_.Exception.Message)" -ForegroundColor Yellow
            }
        }
        
        Write-Host "Installing $galleryModuleName module..." -ForegroundColor Yellow
        
        try {
            # Check for NuGet provider
            $nuget = Get-PackageProvider -Name NuGet -ErrorAction SilentlyContinue
            if (-not $nuget) {
                Write-Host "Installing NuGet package provider..." -ForegroundColor Yellow
                Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Force -Scope CurrentUser | Out-Null
                Write-Host "[OK] NuGet provider installed" -ForegroundColor Green
            }
            
            # Ensure PSGallery is trusted
            try {
                $repo = Get-PSRepository -Name PSGallery -ErrorAction SilentlyContinue
                if ($repo -and $repo.InstallationPolicy -eq "Untrusted") {
                    Write-Host "Setting PSGallery to trusted..." -ForegroundColor Yellow
                    Set-PSRepository -Name PSGallery -InstallationPolicy Trusted -ErrorAction Stop
                    Write-Host "[OK] PSGallery set to trusted" -ForegroundColor Green
                }
            } catch {
                Write-Host "[WARN] Could not set PSGallery policy (may require admin): $($_.Exception.Message)" -ForegroundColor Yellow
                Write-Host "       Continuing anyway..." -ForegroundColor Yellow
            }
            
            # Install module
            Write-Host "Downloading and installing $galleryModuleName..." -ForegroundColor Yellow
            Install-Module -Name $galleryModuleName -Force -Scope CurrentUser -ErrorAction Stop
            Write-Host "[OK] Module installed successfully" -ForegroundColor Green
            
            # Verify install
            Start-Sleep -Seconds 1
            $installedModule = Get-Module -ListAvailable -ErrorAction SilentlyContinue | Where-Object { $_.Name -like "*7zip4*" } | Select-Object -First 1
            if ($installedModule) {
                Write-Host "[OK] Install verified - module found" -ForegroundColor Green
                
                # Test importing
                Write-Host "Testing module import..." -ForegroundColor Yellow
                try {
                    Import-Module $installedModule.Name -Force -ErrorAction Stop
                    $imported = Get-Module $installedModule.Name
                    Write-Host "[OK] Module imported successfully" -ForegroundColor Green
                    Write-Host "     Version: $($imported.Version)" -ForegroundColor White
                    
                    # List commands
                    $commands = $imported.ExportedCommands.Keys
                    Write-Host "     Exported commands: $($commands -join ', ')" -ForegroundColor White
                    
                    # Test Expand-7Zip or Expand-SevenZip if available
                    $expandCmdNames = @("Expand-7Zip", "Expand-SevenZip")
                    $foundCmd = $null
                    foreach ($cmdName in $expandCmdNames) {
                        $expandCmd = Get-Command $cmdName -ErrorAction SilentlyContinue
                        if ($expandCmd -and $expandCmd.ModuleName -eq $installedModule.Name) {
                            $foundCmd = $expandCmd
                            break
                        }
                    }
                    
                    if ($foundCmd) {
                        Write-Host "[OK] $($foundCmd.Name) cmdlet available" -ForegroundColor Green
                        $params = $foundCmd.Parameters.Keys | Where-Object { $_ -notlike 'Verbose*' -and $_ -notlike 'Debug*' -and $_ -notlike 'Error*' -and $_ -notlike 'Warning*' -and $_ -notlike 'Information*' -and $_ -notlike 'Out*' -and $_ -notlike 'Pipeline*' -and $_ -notlike 'WhatIf' -and $_ -notlike 'Confirm' }
                        Write-Host "     Parameters: $($params -join ', ')" -ForegroundColor Gray
                    }
                    
                    Remove-Module $installedModule.Name -Force -ErrorAction SilentlyContinue
                    exit 0
                } catch {
                    Write-Host "[ERROR] Import failed: $($_.Exception.Message)" -ForegroundColor Red
                    exit 1
                }
            } else {
                Write-Host "[ERROR] Install verification failed - module not found" -ForegroundColor Red
                exit 1
            }
            
        } catch {
            Write-Host "[ERROR] Install failed: $($_.Exception.Message)" -ForegroundColor Red
            Write-Host "       Error type: $($_.Exception.GetType().Name)" -ForegroundColor Gray
            if ($_.Exception.InnerException) {
                Write-Host "       Inner: $($_.Exception.InnerException.Message)" -ForegroundColor Gray
            }
            
            # Check if it's a permission issue
            if ($_.Exception.Message -like "*admin*" -or $_.Exception.Message -like "*permission*" -or $_.Exception.Message -like "*access*") {
                Write-Host "" -ForegroundColor Yellow
                Write-Host "[INFO] This may require running as Administrator" -ForegroundColor Yellow
                Write-Host "       Try: Run PowerShell as Administrator and run this script again" -ForegroundColor Cyan
            }
            exit 1
        }
    }
}

Write-Host ""
Write-Host "Operation complete!" -ForegroundColor Cyan

