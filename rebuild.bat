@echo off
setlocal
cd /d "%~dp0"

set "UPLOAD_RELEASE=0"
set "RELEASE_TAG="
set "PIN_BUILD=0"

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="as" (
    set "PIN_BUILD=%~2"
    shift
    shift
    goto parse_args
)
if /i "%~1"=="release" (
    set "UPLOAD_RELEASE=1"
    set "RELEASE_TAG=%~2"
    shift
    shift
    goto parse_args
)
if /i "%~1"=="--release" (
    set "UPLOAD_RELEASE=1"
    shift
    goto parse_args
)
if /i "%~1"=="/release" (
    set "UPLOAD_RELEASE=1"
    shift
    goto parse_args
)
echo Unknown option: %~1
echo Usage: rebuild.bat [as BUILD] [release [TAG]]
exit /b 1

:args_done

call :close_running_installer

echo Regenerating i18n...
python "tools\i18n_codegen.py"
if errorlevel 1 goto fail

if not "%PIN_BUILD%"=="0" (
    echo Pinning build number to %PIN_BUILD% ^(updater test build^)...
    set "MEDICAT_PIN_BUILD=%PIN_BUILD%"
    python "tools\bump_build_number.py" "build_number.txt" "generated\build_version.cpp" --major 1 --minor 0 --set %PIN_BUILD%
    if errorlevel 1 goto fail
) else (
    echo Bumping build number...
    python "tools\bump_build_number.py" "build_number.txt" "generated\build_version.cpp" --major 1 --minor 0
    if errorlevel 1 goto fail
)
set MEDICAT_BUILD_NUMBER_BUMPED=1

set "SRC_DIR=%~dp0"
if "%SRC_DIR:~-1%"=="\" set "SRC_DIR=%SRC_DIR:~0,-1%"

echo Configuring x64 CMake...
call :configure_cmake build x64
if errorlevel 1 goto fail

echo Configuring Win32 CMake...
call :configure_cmake build-x86 Win32
if errorlevel 1 goto fail

echo Building x64 Release...
cmake --build build --config Release
if errorlevel 1 goto fail

echo Building Win32 Release...
cmake --build build-x86 --config Release
if errorlevel 1 goto fail

echo.
echo Build OK:
echo   %~dp0build\Release\MedicatInstaller.exe
echo   %~dp0build-x86\Release\MedicatInstaller-x86.exe

if "%UPLOAD_RELEASE%"=="1" (
    call "tools\upload_release.bat" "%RELEASE_TAG%"
    if errorlevel 1 goto fail
) else if "%PIN_BUILD%"=="0" (
    echo Publishing update manifest...
    python "tools\publish_update_manifest.py" --build-number "build_number.txt" --build-version "generated\build_version.cpp" --output "update.json"
    if errorlevel 1 goto fail
)

if "%UPLOAD_RELEASE%"=="1" goto push_manifest
if "%PIN_BUILD%"=="0" goto push_manifest
echo Skipping update manifest push for pinned test build %PIN_BUILD%.
set "MEDICAT_PIN_BUILD="
exit /b 0

:push_manifest
echo Pushing update manifest to origin/cpp...
call "tools\push_update_manifest.bat"
if errorlevel 1 (
    echo Warning: update manifest push failed.
)
if not "%PIN_BUILD%"=="0" set "MEDICAT_PIN_BUILD="
exit /b 0

:close_running_installer
echo Checking for running installer...
for %%P in (MedicatInstaller.exe MedicatInstaller-x86.exe) do (
    tasklist /FI "IMAGENAME eq %%P" 2>nul | find /I "%%P" >nul
    if not errorlevel 1 (
        echo Closing %%P...
        taskkill /IM %%P /F >nul 2>&1
        if errorlevel 1 (
            echo Warning: could not close %%P. Close it manually if linking fails.
        ) else (
            timeout /t 1 /nobreak >nul
        )
    )
)
exit /b 0

:configure_cmake
set "BUILD_DIR=%~1"
set "VS_ARCH=%~2"
if exist "%BUILD_DIR%\CMakeCache.txt" (
    findstr /I /C:"%SRC_DIR:\=/%" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
    if errorlevel 1 (
        echo Removing stale CMake cache in %BUILD_DIR% ^(project moved or path changed^)...
        rmdir /s /q "%BUILD_DIR%" 2>nul
    )
)
cmake -S "%SRC_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A %VS_ARCH%
exit /b %ERRORLEVEL%

:fail
echo Build failed.
exit /b 1
