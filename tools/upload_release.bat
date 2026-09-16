@echo off
setlocal
cd /d "%~dp0\.."

set "REPO=mon5termatt/medicat_installer"

set "TAG=%~1"
if not "%TAG%"=="" goto have_tag

if exist "build_number.txt" set /p TAG=<build_number.txt
if not "%TAG%"=="" goto have_tag

if exist "release_tag.txt" set /p TAG=<release_tag.txt
if not "%TAG%"=="" goto have_tag

REM Single-line for/f only — avoid nested parens / select(...) in do-blocks.
for /f "usebackq delims=" %%T in (`gh release list --repo %REPO% --limit 1 --json tagName -q ".[0].tagName" 2^>nul`) do set "TAG=%%T"

:have_tag
if not "%TAG%"=="" goto files_ready
echo Could not determine GitHub release tag.
echo Pass a tag matching the installer version, e.g. tools\upload_release.bat 1.0.41
echo Or ensure build_number.txt contains the version 1.0.N.
exit /b 1

:files_ready
set "X64_SRC=build\x64\Release\MedicatInstaller.exe"
set "X86_SRC=build\x86\Release\MedicatInstaller-x86.exe"
set "X64_EXE=build\Release\MedicatInstaller.exe"
set "X86_EXE=build\Release\MedicatInstaller-x86.exe"

if not exist "%X64_SRC%" if exist "%X64_EXE%" set "X64_SRC=%X64_EXE%"
if not exist "%X86_SRC%" if exist "%X86_EXE%" set "X86_SRC=%X86_EXE%"
if not exist "%X86_SRC%" if exist "build-x86\Release\MedicatInstaller-x86.exe" set "X86_SRC=build-x86\Release\MedicatInstaller-x86.exe"

if exist "%X64_SRC%" goto have_x64
echo Missing x64 build: %X64_SRC%
echo Run rebuild.bat first.
exit /b 1

:have_x64
if exist "%X86_SRC%" goto have_x86
echo Missing Win32 build: %X86_SRC%
echo Run rebuild.bat first.
exit /b 1

:have_x86
if not exist "build\Release" mkdir "build\Release"
copy /Y "%X64_SRC%" "%X64_EXE%" >nul
copy /Y "%X86_SRC%" "%X86_EXE%" >nul

if exist "%X64_EXE%" goto have_staged_x64
echo Missing x64 build: %X64_EXE%
echo Run rebuild.bat first.
exit /b 1

:have_staged_x64
if exist "%X86_EXE%" goto fetch_linux
echo Missing Win32 build: %X86_EXE%
echo Run rebuild.bat first.
exit /b 1

:fetch_linux
REM Always ship the community Linux installer from the linux branch tip.
set "LINUX_SH=build\Release\Medicat_Installer.sh"
set "LINUX_URL=https://raw.githubusercontent.com/%REPO%/linux/Medicat_Installer.sh"
echo Fetching Linux installer from branch linux...
curl -fsSL -o "%LINUX_SH%" "%LINUX_URL%"
if errorlevel 1 goto linux_fetch_failed
if not exist "%LINUX_SH%" goto linux_fetch_failed
for %%I in ("%LINUX_SH%") do if %%~zI==0 goto linux_fetch_failed
echo   %LINUX_SH% ready (from linux branch)
goto do_upload

:linux_fetch_failed
echo Failed to download Medicat_Installer.sh from:
echo   %LINUX_URL%
echo Ensure branch linux is published and the file exists.
exit /b 1

:do_upload
gh release view "%TAG%" --repo "%REPO%" >nul 2>&1
if errorlevel 1 goto create_release
goto upload_assets

:create_release
echo Creating GitHub release %TAG%...
REM Prefer annotated tag message, else the tagged commit message (not empty
REM generate-notes changelog-only stubs). Resolve notes ourselves: gh rejects
REM --notes-from-tag with --repo, and --notes-from-tag needs the tag locally
REM (workflow_dispatch checkouts often lack refs/tags/1.0.N).
git rev-parse -q --verify "refs/tags/%TAG%" >nul 2>&1
if not errorlevel 1 goto have_local_tag
echo Fetching tag %TAG% for release notes...
git fetch --no-tags origin "refs/tags/%TAG%:refs/tags/%TAG%"
if errorlevel 1 goto create_failed

:have_local_tag
set "NOTES_FILE=%TEMP%\medicat_release_notes_%TAG%.txt"
git tag -l --format=%%(contents) "%TAG%" > "%NOTES_FILE%" 2>nul
for %%A in ("%NOTES_FILE%") do if %%~zA==0 git log -1 --format=%%B "%TAG%" > "%NOTES_FILE%"
gh release create "%TAG%" --title "%TAG%" --notes-file "%NOTES_FILE%" --latest --repo "%REPO%"
if errorlevel 1 goto create_failed
goto upload_assets

:create_failed
echo Failed to create release %TAG%.
exit /b 1

:upload_assets
echo Uploading assets to GitHub release %TAG%...
gh release upload "%TAG%" "%X64_EXE%" "%X86_EXE%" "%LINUX_SH%" --clobber --repo "%REPO%"
if errorlevel 1 goto upload_failed

echo.
echo Uploaded:
echo   %X64_EXE%
echo   %X86_EXE%
echo   %LINUX_SH%  ^(from branch linux^)
echo Release: https://github.com/%REPO%/releases/tag/%TAG%
echo Installer self-update discovers Windows assets via the GitHub Releases API.
exit /b 0

:upload_failed
echo Release upload failed.
exit /b 1
