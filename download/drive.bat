@echo off
setlocal EnableDelayedExpansion

echo Disclaimer: This script is not affiliated with Google Drive or the developers of gdown.
echo Do not report issues to them.
echo.
echo This script is designed to download the MediCat.USB.v21.12.zip file from Google Drive.
echo It will check for an existing WinPython installation, download WinPython if needed,
echo and then attempt to download the files using gdown.
echo If any files fail to download, it will open browser windows for manual download.
echo It will then verify the downloaded files and run the hash validator.    
echo.
echo Press any key to continue...
pause >nul

REM Part of this script is made using AI, so if you have any questions or suggestions, please contact me.
REM Thank you for using MediCat USB!


:: Set title
title Medicat USB Installer

:: Setup environment
Set "Path=%Path%;%CD%;%CD%\bin;"
set "maindir=%CD%"
if defined ProgramFiles(x86) (set bit=64) else (set bit=32)

:: Define Google Drive file IDs
set file1=1q8gulgxsQjEveNcf2ZLSOpQgroJ78CZq
set file2=1xMiquzKsfW1LUd8RmypMO3Z7PzilY9lu
set file3=1nhFdNHYiWhsh__uL6h1qo2QMceEcqhEy
set file4=1uO9I1poXhwJ-FP7n1kFS6NY7na_ZrSZR
set file5=1ubWGDRP3Cy2bk1yU008TGMQ0zRA2lsVF
set file6=1k78sLJTUyxW-zu7rwn9crHYZjHjah3OE
set file7=1xQJDoqMx72gnmiIDl5HJoWTWCmLWOr7N
set file8=1b8ZJonZfq9B3UDc1PSAQ2nynE-wRua9z
set file9=1ciT345w9PINo95siK05i-gEjYjF0zlwC
set file10=1CoiChOCUgX5uLxz6SmwR2xPAboQ_wLXv
set file11=1SJxlEyIGwbOZBr4OV4fdS23xnl0NTgH4
set file12=1jSYbrQFqg5rhOMWy_kWW_nuGCcZ-VfuK

echo ===================================
echo Medicat USB Installer
echo ===================================
echo.

:: Check for existing WinPython installation
for /d %%i in ("WPy*") do (
    echo Found existing WinPython installation
    set "WPYDIR=%%i"
    goto setup_python
)

:: Download WinPython if not present
echo Downloading WinPython...
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://github.com/winpython/winpython/releases/download/13.1.202502222final/Winpython%bit%-3.12.9.0dot.exe', 'Winpython%bit%-3.12.9.0dot.exe')"
if not exist "Winpython%bit%-3.12.9.0dot.exe" (
    echo Failed to download WinPython
    goto error
)

echo Installing WinPython...
Winpython%bit%-3.12.9.0dot.exe -y
if errorlevel 1 (
    echo Failed to install WinPython
    goto error
)

:: Wait for extraction to complete and find the WinPython directory
echo Waiting for WinPython extraction to complete...
:wait_loop
:: Look for any WinPython directory matching the pattern
for /d %%i in ("WPy*") do (
    set "WPYDIR=%%i"
    goto found_dir
)
timeout /t 1 /nobreak >nul
goto wait_loop

:found_dir
if not defined WPYDIR (
    echo Failed to find WinPython directory
    goto error
)

:: Wait a bit more to ensure all files are extracted
timeout /t 3 /nobreak >nul

:setup_python
:: Change directory to scripts folder with full path to ensure we're in the right place
cd /d "%CD%\%WPYDIR%\scripts"
if errorlevel 1 (
    echo Failed to change directory to WinPython scripts
    goto error
)

call env_for_icons.bat
if not "%WINPYWORKDIR%"=="%WINPYWORKDIR1%" cd %WINPYWORKDIR1%

echo Installing gdown...
pip install gdown
if errorlevel 1 (
    echo Failed to install gdown
    goto error
)

cls
echo Downloading Medicat files from primary source...
echo This may take a while depending on your internet connection.
echo.

:: Try both download sources first
set "browser_urls="
call :download_files primary
set "primary_failed=!failed_parts!"
set "primary_success=!success_count!"

if !success_count! lss 6 (
    echo Primary download failed for some parts, trying mirror...
    echo.
    call :download_files mirror
)

:: If we still don't have all files, show browser links
if !success_count! lss 6 (
    echo.
    echo Some files could not be downloaded automatically.
    echo Opening browser links for manual download...
    echo.
    echo Please for each browser window that opens:
    echo 1. Click [Download anyway] in each browser window
    echo 2. Save the files with the correct names ^(MediCat.USB.v21.12.zip.001, .002, etc^)
    echo 3. Move the downloaded files to: %maindir%
    echo.
    timeout /t 3 /nobreak >nul
    
    :: Try primary source links first
    echo Opening primary source links...
    for %%i in (!primary_failed!) do (
        set "fileid=!file%%i!"
        start "" "https://drive.google.com/uc?id=!fileid!"
        echo Opening browser for part %%i [Primary source]
        timeout /t 2 /nobreak >nul
    )
    
    echo.
    echo Please attempt to download the files from the opened browser windows.
    echo After you have finished downloading ^(or if the downloads failed^),
    choice /c YN /m "Would you like to try the mirror links?"
    if !errorlevel! equ 1 (
        echo.
        echo Opening mirror links...
        for %%i in (!primary_failed!) do (
            set /a "j=%%i+6"
            call set "fileid=%%file!j!%%"
            start "" "https://drive.google.com/uc?id=!fileid!"
            echo Opening browser for part %%i [Mirror source]
            timeout /t 2 /nobreak >nul
        )
    )
    
    echo.
    echo Please ensure all files are downloaded and in the correct location:
    echo %maindir%
    echo.
    pause
)

:: Verify files exist
call :verify_files
if errorlevel 1 goto downloadfail

:: Download and run hash validator
echo Downloading hash validator...
powershell -Command "(New-Object Net.WebClient).DownloadFile('http://cdn.medicatusb.com/files/hasher/Google_Drive_Validate_Files.exe', 'Google_Drive_Validate_Files.exe')"
if exist "Google_Drive_Validate_Files.exe" (
    start Google_Drive_Validate_Files.exe
) else (
    echo Warning: Could not download hash validator
)

echo.
echo Installation completed successfully!
pause
exit /b 0

:download_files
set source=%~1
set "success_count=0"
set "failed_parts="

for /l %%i in (1,1,6) do (
    if "%source%"=="primary" (
        set "fileid=!file%%i!"
    ) else (
        set /a "j=%%i+6"
        call set "fileid=%%file!j!%%"
    )
    echo Downloading part %%i of 6 [ID: !fileid!]...
    
    :: Create a temporary file for the output
    set "tempfile=%temp%\gdown_output.txt"
    gdown "!fileid!" -O "%maindir%\MediCat.USB.v21.12.zip.00%%i" > "!tempfile!" 2>&1
    
    if errorlevel 1 (
        echo Failed to download part %%i
        set "failed_parts=!failed_parts! %%i"
    ) else (
        echo Successfully downloaded part %%i
        set /a "success_count+=1"
    )
    del "!tempfile!" 2>nul
)

echo.
echo Download summary:
echo Successfully downloaded !success_count! of 6 parts
if defined failed_parts echo Failed to download parts:!failed_parts!

:: Only return error if we got none of the files
if !success_count! equ 0 (
    echo No files were downloaded successfully
    exit /b 1
)
exit /b 0

:verify_files
echo Verifying downloaded files...
for %%i in (1,2,3,4,5,6) do (
    if not exist "%maindir%\MediCat.USB.v21.12.zip.00%%i" (
        echo Missing part %%i
        exit /b 1
    )
    echo Part %%i verified
)
exit /b 0

:downloadfail
echo.
echo ERROR: Failed to download required files.
echo Please try using the torrent instead.
echo.
pause
exit /b 1

:error
echo.
echo An error occurred during installation.
echo Please try running the script again as administrator.
echo If the problem persists, please report the issue.
echo.
pause
exit /b 1
