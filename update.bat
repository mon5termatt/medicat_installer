@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
title MediCat Installer Update

cls
echo.
echo  Updating MediCat Installer to the C++ release...
echo  (replacing the legacy batch installer)
echo.

REM --- Backup legacy names (historical spellings) ---
if exist "Medicat Installer.exe" copy /Y "Medicat Installer.exe" "Medicat Installer.exe.bak" >nul
if exist "MediCat_Installer.exe" copy /Y "MediCat_Installer.exe" "MediCat_Installer.exe.bak" >nul
if exist "Medicat Installer.bat" copy /Y "Medicat Installer.bat" "Medicat Installer.bat.bak" >nul
if exist "Medicat_Installer.bat" copy /Y "Medicat_Installer.bat" "Medicat_Installer.bat.bak" >nul
if exist "MediCat_Installer.bat" copy /Y "MediCat_Installer.bat" "MediCat_Installer.bat.bak" >nul

REM --- Architecture ---
set "ARCH=x86"
if /I "%PROCESSOR_ARCHITECTURE%"=="AMD64" set "ARCH=x64"
if /I "%PROCESSOR_ARCHITECTURE%"=="ARM64" set "ARCH=x64"
if defined PROCESSOR_ARCHITEW6432 if /I "%PROCESSOR_ARCHITEW6432%"=="AMD64" set "ARCH=x64"

set "OUT=MedicatInstaller.exe"
set "ASSET=MedicatInstaller.exe"
if /I "%ARCH%"=="x86" (
    set "OUT=MedicatInstaller-x86.exe"
    set "ASSET=MedicatInstaller-x86.exe"
)

echo  Detected arch: %ARCH%
echo  Target file : %OUT%
echo.

where curl >nul 2>&1
if errorlevel 1 (
    echo ERROR: curl.exe not found. Install curl or use Windows 10+ with curl in PATH.
    goto fail
)

where powershell >nul 2>&1
if errorlevel 1 (
    echo ERROR: PowerShell not found.
    goto fail
)

echo  Looking up GitHub releases for %ASSET%...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference = 'Stop';" ^
  "$assetName = $env:ASSET;" ^
  "$releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/mon5termatt/medicat_installer/releases?per_page=20' -Headers @{ 'User-Agent' = 'MedicatInstaller-Update' } -UseBasicParsing;" ^
  "$withAsset = @($releases | Where-Object { -not $_.draft -and ($_.assets | Where-Object { $_.name -eq $assetName }) });" ^
  "$pick = $withAsset | Where-Object { -not $_.prerelease } | Select-Object -First 1;" ^
  "if (-not $pick) { $pick = $withAsset | Select-Object -First 1 };" ^
  "if (-not $pick) { throw 'No release publishes ' + $assetName };" ^
  "$url = ($pick.assets | Where-Object { $_.name -eq $assetName } | Select-Object -First 1).browser_download_url;" ^
  "$url.Trim() | Out-File -FilePath '.\update_url.ini' -Encoding ascii -NoNewline;" ^
  "Write-Host ('  Tag     : ' + $pick.tag_name);" ^
  "Write-Host ('  Release : ' + $pick.name);" ^
  "if ($pick.prerelease) { Write-Host '  Channel : prerelease' } else { Write-Host '  Channel : stable' }"

if errorlevel 1 (
    echo ERROR: Could not find a GitHub release with %ASSET%.
    goto fail
)

set /p URL=<update_url.ini
del update_url.ini >nul 2>&1
if not defined URL (
    echo ERROR: Empty download URL.
    goto fail
)

echo.
echo  Downloading:
echo  %URL%
echo.

if exist "%OUT%.part" del /F /Q "%OUT%.part" >nul 2>&1
curl -L --fail --retry 3 --retry-delay 2 -o "%OUT%.part" "%URL%"
if errorlevel 1 (
    echo ERROR: Download failed.
    if exist "%OUT%.part" del /F /Q "%OUT%.part" >nul 2>&1
    goto fail
)

for %%I in ("%OUT%.part") do if %%~zI==0 (
    echo ERROR: Downloaded file is empty.
    del /F /Q "%OUT%.part" >nul 2>&1
    goto fail
)

if exist "%OUT%" del /F /Q "%OUT%" >nul 2>&1
move /Y "%OUT%.part" "%OUT%" >nul
if errorlevel 1 (
    echo ERROR: Could not replace %OUT%
    goto fail
)

echo.
echo  Download complete: %OUT%
echo  Starting the new installer...
echo.

REM Launch C++ GUI; quote path for spaces under G:\ etc.
start "" "%~dp0%OUT%"

echo.
echo  Update complete. This window will close shortly.
timeout /t 3 /nobreak >nul

REM Deleting this bat while it is still running causes "The batch file cannot be found."
REM Schedule delete after we exit.
start "" /min cmd /c "timeout /t 2 /nobreak >nul & del /f /q \"%~f0\" >nul 2>&1"

endlocal
exit 0

:fail
echo.
echo  Update failed. Your previous Medicat_Installer.bat backup ^(if any^) was left as *.bak
echo  You can also download the C++ installer manually from:
echo  https://github.com/mon5termatt/medicat_installer/releases
echo.
pause
endlocal
exit 1
