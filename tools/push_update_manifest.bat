@echo off
setlocal
cd /d "%~dp0\.."

set "MANIFEST=update.json"

if not exist "%MANIFEST%" (
    echo Update manifest not found: %MANIFEST%
    exit /b 1
)

git rev-parse --is-inside-work-tree >nul 2>&1
if errorlevel 1 (
    echo Not a git repository — skipping manifest push.
    exit /b 0
)

git add -- "%MANIFEST%"
git diff --cached --quiet -- "%MANIFEST%"
if not errorlevel 1 (
    echo Update manifest unchanged — nothing to push.
    exit /b 0
)

for /f "usebackq delims=" %%V in (`python -c "import json;print(json.load(open(r'%MANIFEST%',encoding='utf-8')).get('version',''))"`) do set "VERSION=%%V"
if defined VERSION (
    git commit -m "Update installer update manifest to v%VERSION%." -- "%MANIFEST%"
) else (
    git commit -m "Update installer update manifest." -- "%MANIFEST%"
)
if errorlevel 1 (
    echo Manifest commit failed.
    exit /b 1
)

for /f "usebackq delims=" %%B in (`git rev-parse --abbrev-ref HEAD`) do set "BRANCH=%%B"
if /i not "%BRANCH%"=="cpp" (
    echo Pushing update.json to origin/cpp ^(current branch: %BRANCH%^)...
    git push origin HEAD:cpp
) else (
    git push origin cpp
)
if errorlevel 1 (
    echo Manifest push failed.
    exit /b 1
)

echo Pushed %MANIFEST% to origin/cpp
exit /b 0
