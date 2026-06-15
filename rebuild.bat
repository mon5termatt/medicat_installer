@echo off
setlocal
cd /d "%~dp0"

echo Regenerating i18n...
python "tools\i18n_codegen.py"
if errorlevel 1 goto fail

if not exist "build\CMakeCache.txt" (
    echo Configuring CMake...
    cmake -B build -G "Visual Studio 17 2022" -A x64
    if errorlevel 1 goto fail
)

echo Building Release...
cmake --build build --config Release
if errorlevel 1 goto fail

echo.
echo Build OK: %~dp0build\Release\MedicatInstaller.exe
exit /b 0

:fail
echo Build failed.
exit /b 1
