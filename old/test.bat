@echo off
setlocal enabledelayedexpansion

set "driveLabel=%~1"

for /f "tokens=2 delims==" %%a in ('wmic volume where "Label='!driveLabel!'" get DeviceID /value') do (
    set "deviceID=%%a"
)

:loop
for /f "tokens=2 delims==" %%a in ('wmic volume where "DeviceID='!deviceID!'" get DriveLetter /value') do (
    set "driveLetter=%%a"
)

echo Drive Letter: !driveLetter!

timeout /t 5 >nul
goto loop