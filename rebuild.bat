@echo off
setlocal
cd /d "%~dp0"

set "UPLOAD_RELEASE=0"
set "RELEASE_TAG="

:parse_args
if "%~1"=="" goto args_done
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
echo Usage: rebuild.bat [release [TAG]]
exit /b 1

:args_done

echo Regenerating i18n...
python "tools\i18n_codegen.py"
if errorlevel 1 goto fail

echo Bumping build number...
python "tools\bump_build_number.py" "build_number.txt" "generated\build_version.cpp" --major 1 --minor 0
if errorlevel 1 goto fail
set MEDICAT_BUILD_NUMBER_BUMPED=1

if not exist "build\CMakeCache.txt" (
    echo Configuring x64 CMake...
    cmake -B build -G "Visual Studio 17 2022" -A x64
    if errorlevel 1 goto fail
)

if not exist "build-x86\CMakeCache.txt" (
    echo Configuring Win32 CMake...
    cmake -B build-x86 -G "Visual Studio 17 2022" -A Win32
    if errorlevel 1 goto fail
)

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
)

exit /b 0

:fail
echo Build failed.
exit /b 1
