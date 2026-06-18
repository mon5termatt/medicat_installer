@echo off
setlocal
cd /d "%~dp0\.."

set "TAG=%~1"
if "%TAG%"=="" if exist "release_tag.txt" (
    set /p TAG=<release_tag.txt
)

if "%TAG%"=="" (
    for /f "usebackq delims=" %%T in (`gh release list --repo mon5termatt/medicat_installer --limit 5 --json tagName^,isPrerelease -q ".[] | select(.isPrerelease==true) | .tagName"`) do (
        set "TAG=%%T"
        goto :tag_done
    )
)
:tag_done

if "%TAG%"=="" (
    echo Could not determine GitHub release tag.
    echo Pass a tag: tools\upload_release.bat 3521-BETA
    echo Or create release_tag.txt with the tag name.
    exit /b 1
)

set "X64_EXE=build\Release\MedicatInstaller.exe"
set "X86_EXE=build-x86\Release\MedicatInstaller-x86.exe"

if not exist "%X64_EXE%" (
    echo Missing x64 build: %X64_EXE%
    echo Run rebuild.bat first.
    exit /b 1
)
if not exist "%X86_EXE%" (
    echo Missing Win32 build: %X86_EXE%
    echo Run rebuild.bat first.
    exit /b 1
)

echo Uploading to GitHub release %TAG%...
gh release upload "%TAG%" "%X64_EXE%" "%X86_EXE%" --clobber --repo mon5termatt/medicat_installer
if errorlevel 1 (
    echo Release upload failed.
    exit /b 1
)

echo.
echo Uploaded:
echo   %X64_EXE%
echo   %X86_EXE%
echo Release: https://github.com/mon5termatt/medicat_installer/releases/tag/%TAG%
exit /b 0
