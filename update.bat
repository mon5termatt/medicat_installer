@echo off

REM Backup files before Deleting
IF EXIST "Medicat Installer.exe" (
    copy "Medicat Installer.exe" "Medicat Installer.exe.bak"
)
IF EXIST "MediCat_Installer.exe" (
    copy "MediCat_Installer.exe" "MediCat_Installer.exe.bak"
)
IF EXIST "Medicat Installer.bat" (
    copy "Medicat Installer.bat" "Medicat Installer.bat.bak"
)
IF EXIST "MediCat_Installer.bat" (
    copy "MediCat_Installer.bat" "MediCat_Installer.bat.bak"
)
DEL "Medicat Installer.exe" 2>nul
DEL "MediCat_Installer.exe" 2>nul
DEL "Medicat Installer.bat" 2>nul
DEL "MediCat_Installer.bat" 2>nul
cls
echo.Updating Medicat Installer. Please wait while the new file is downloaded.
powershell -c "$data = curl https://api.github.com/repos/mon5termatt/medicat_installer/git/refs/tag -UseBasicParsing | ConvertFrom-Json; $data[-1].ref -replace 'refs/tags/', '' | Out-File -Encoding 'UTF8' -FilePath './ver.ini'"
set /p ver= < ver.ini
set ver=%ver:~-4%
del ver.ini /Q
echo.Version %ver% found
curl https://github.com/mon5termatt/medicat_installer/releases/download/%ver%/Medicat_Installer.bat -o Medicat_Installer.bat -q -L
REM Check if the download was successful
IF NOT EXIST "Medicat_Installer.bat" (
    echo ERROR: Failed to download the new file.
    REM Restore the backup if it exists
    IF EXIST "Medicat Installer.bat.bak" (
        copy "Medicat Installer.bat.bak" "Medicat Installer.bat"
    )
    pause
    exit /b 1
)

cls
start cmd /k Medicat_Installer.bat
del %0 && exit
