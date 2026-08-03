@echo off
setlocal EnableDelayedExpansion

:: Constants
set "FILE_NAME=MediCat.USB.v21.12.7z"
set "EXPECTED_SIZE=22994783619"
set "EXPECTED_HASH=db50f96a5c7b5ec6dc9ed77ea29fffb0"
set "REFERER=https://installer.medicatusb.com"

:: Server URLs
set "SERVER1=https://files.medicatusb.com/files/v21.12/%FILE_NAME%"
set "SERVER2=https://files.dog/OD%%20Rips/MediCat/v21.12/%FILE_NAME%"
set "SERVER3=https://cat.tcbl.dev/%FILE_NAME%"

:check
cls
if exist "%FILE_NAME%" (
    call :check_file || goto download
    exit /b
)

:download
cls
echo Testing download speeds from available servers...
echo.

:: Array to store server speeds
set "max_speed=0"
set "best_url="

:: Test each server
for %%S in (1 2 3) do (
    set "current_server=!SERVER%%S!"
    
    :: Check HTTP status code first
    for /f %%H in ('curl -e "%REFERER%" --write-out "%%{http_code}" --silent --output nul --head "!current_server!"') do set "http_code=%%H"
    
    if "!http_code!"=="200" (
        :: Only show server info and test speed for successful connections
        echo Testing Server %%S - !current_server!
        echo Status: OK (200^)
        for /f %%I in ('curl -e "%REFERER%" --max-time 3 "!current_server!" -o "test%%S.tmp" -s -w "%%{speed_download}"') do (
            set /a "speed=%%I / 1000000"
            if !speed! gtr 0 (
                echo Speed: !speed! mbps
                
                if !speed! gtr !max_speed! (
                    set "max_speed=!speed!"
                    set "best_url=!current_server!"
                )
            ) else (
                echo Speed: Connection failed
            )
        )
        echo.
    )
    
    if exist "test%%S.tmp" del "test%%S.tmp" /q
)

if not defined best_url (
    echo ERROR: No valid server found.
    echo Please check your internet connection and try again.
    timeout 5 > nul
    exit /b 1
)

echo Downloading from fastest server ^(!max_speed! mbps^)...
echo URL: !best_url!
echo.

:: Download with progress bar and resume capability
curl -e "%REFERER%" -# -L -o "%FILE_NAME%" -C - "!best_url!"
if errorlevel 1 (
    echo Download failed. Please try again.
    timeout 5 > nul
    goto check
)

call :check_file
exit /b

:check_file
echo Verifying downloaded file... 
echo Please wait... While it looks like it's not doing anything, it is.
echo This may take a while depending on your CPU performance.
if not exist "%FILE_NAME%" (
    echo File not found.
    exit /b 1
)

:: Check file size
for %%A in ("%FILE_NAME%") do set "size=%%~zA"
if not "!size!"=="%EXPECTED_SIZE%" (
    echo Size mismatch. Expected: %EXPECTED_SIZE%, Got: !size!
    exit /b 1
)

:: Verify file hash using MD5
certutil -hashfile "%FILE_NAME%" MD5 | findstr /i /v "hash" > hash.tmp
set /p FILEHASH=<hash.tmp
del hash.tmp
if /i "!FILEHASH!"=="%EXPECTED_HASH%" (
    echo Hash verification passed.
) else (
    echo Hash verification failed.
    echo Expected: %EXPECTED_HASH%
    echo Got     : !FILEHASH!
    echo The downloaded file may be corrupted or incomplete.
    exit /b 1
)

echo File verification complete.
echo Size and hash checks passed.
timeout 5 > nul
exit /b 0
