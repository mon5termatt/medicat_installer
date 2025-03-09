@echo off
setlocal EnableDelayedExpansion

echo Disclaimer: This script is not affiliated with aria2c or the developers of aria2c.
echo Do not report issues to them.
echo.
echo Press any key to continue...
pause >nul

:: Add current directory and bin to Path
Set "Path=%Path%;%CD%;%CD%\bin;"

:: Determine system architecture
if defined ProgramFiles(x86) (set bit=64) else (set bit=32)

echo Downloading torrent file...
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/download/MediCat_USB_v21.12.torrent" -o "./medicat.torrent" -s -f
if %ERRORLEVEL% neq 0 (
    echo Failed to download torrent file.
    exit /b 1
)

echo Downloading aria2c...
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/aria2c-%bit%.exe" -o "./aria2c.exe" -s -f
if %ERRORLEVEL% neq 0 (
    echo Failed to download aria2c.
    del medicat.torrent 2>nul
    exit /b 1
)

echo Starting torrent download...
aria2c.exe --file-allocation=none --seed-time=0 --summary-interval=15 medicat.torrent
if %ERRORLEVEL% neq 0 (
    echo Failed to download MediCat files.
    del medicat.torrent aria2c.exe 2>nul
    exit /b 1
)

echo Moving files...
if exist ".\MediCat USB v21.12\MediCat.USB.v21.12.7z" (
    move /Y ".\MediCat USB v21.12\MediCat.USB.v21.12.7z" ".\MediCat.USB.v21.12.7z" >nul
    if %ERRORLEVEL% neq 0 (
        echo Failed to move MediCat file.
        exit /b 1
    )
    rd /S /Q "MediCat USB v21.12" 2>nul
) else (
    echo MediCat file not found in expected location.
    exit /b 1
)

echo Cleaning up...
del medicat.torrent aria2c.exe /Q 2>nul

echo Installation completed successfully!
exit /b 0
