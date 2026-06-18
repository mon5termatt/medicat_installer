@echo off
setlocal
cd /d "%~dp0"

echo Regenerating i18n...
python "tools\i18n_codegen.py"
if errorlevel 1 goto fail

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
echo   %~dp0build-x86\Release\MedicatInstaller.exe
exit /b 0

:fail
echo Build failed.
exit /b 1
