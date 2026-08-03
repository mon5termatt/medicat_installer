@echo off
setlocal
cd /d "%~dp0\.."

REM Fixed tag for fielded Medicat_Installer.bat clients (localver=3520).
REM They compare localver to the last 4 chars of /releases/latest tag_name.
REM Tag 3521 -> remver=3521 -> UPDATE. Do not use 1.0.N as Latest for them.
set "BRIDGE_TAG=3521"
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
if exist "%X86_EXE%" goto do_upload
echo Missing Win32 build: %X86_EXE%
echo Run rebuild.bat first.
exit /b 1

:do_upload
if /I "%TAG%"=="%BRIDGE_TAG%" goto skip_version_conflict
goto version_ok

:skip_version_conflict
echo ERROR: C++ release tag cannot be the bridge tag %BRIDGE_TAG%.
exit /b 1

:version_ok
gh release view "%TAG%" --repo "%REPO%" >nul 2>&1
if errorlevel 1 goto create_release
goto upload_assets

:create_release
echo Creating GitHub release %TAG%...
REM Do not use --latest here; the bridge release must remain Latest for batch clients.
gh release create "%TAG%" --title "%TAG%" --generate-notes --repo "%REPO%"
if errorlevel 1 goto create_failed
goto upload_assets

:create_failed
echo Failed to create release %TAG%.
exit /b 1

:upload_assets
echo Uploading assets to GitHub release %TAG%...
gh release upload "%TAG%" "%X64_EXE%" "%X86_EXE%" --clobber --repo "%REPO%"
if errorlevel 1 goto upload_failed

echo.
echo Uploaded:
echo   %X64_EXE%
echo   %X86_EXE%
echo Release: https://github.com/mon5termatt/medicat_installer/releases/tag/%TAG%
echo Installer self-update discovers these assets via the GitHub Releases API.

call :ensure_legacy_bridge
if errorlevel 1 goto bridge_failed
exit /b 0

:upload_failed
echo Release upload failed.
exit /b 1

:bridge_failed
echo C++ release uploaded, but legacy bridge Latest promotion failed.
exit /b 1

REM --- Legacy batch bridge: keep a numeric Latest tag for Medicat_Installer.bat ---
:ensure_legacy_bridge
set "NOTES=%TEMP%\medicat_bridge_release_notes.md"
> "%NOTES%" echo ## Legacy batch bridge
>> "%NOTES%" echo.
>> "%NOTES%" echo This release is GitHub **Latest** so fielded `Medicat_Installer.bat` clients
>> "%NOTES%" echo ^(localver=3520^) take the last 4 characters of the tag name ^(%BRIDGE_TAG%^)
>> "%NOTES%" echo and run their update path.
>> "%NOTES%" echo.
>> "%NOTES%" echo They then download:
>> "%NOTES%" echo https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/update.bat
>> "%NOTES%" echo which installs the current C++ build from semantic release **%TAG%**.
>> "%NOTES%" echo.
>> "%NOTES%" echo No installer assets belong on this tag. C++ self-update uses `1.0.N` releases only.
>> "%NOTES%" echo Re-promoted automatically by tools/upload_release.bat after each C++ upload.

gh release view "%BRIDGE_TAG%" --repo "%REPO%" >nul 2>&1
if errorlevel 1 goto create_bridge

echo Re-promoting bridge release %BRIDGE_TAG% as GitHub Latest...
gh release edit "%BRIDGE_TAG%" --latest --notes-file "%NOTES%" --repo "%REPO%"
if errorlevel 1 exit /b 1
goto bridge_done

:create_bridge
echo Creating bridge release %BRIDGE_TAG% as GitHub Latest...
gh release create "%BRIDGE_TAG%" --title "Legacy bridge - update to C++ installer" --latest --notes-file "%NOTES%" --repo "%REPO%"
if errorlevel 1 exit /b 1

:bridge_done
echo Bridge Latest: https://github.com/%REPO%/releases/tag/%BRIDGE_TAG%
echo   /releases/latest -^> %BRIDGE_TAG%  ^|  C++ assets on %TAG%
exit /b 0
