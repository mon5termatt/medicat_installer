@echo off

echo Disclaimer: This script is not affiliated with IPFS or the developers of ipget.
echo Also, I think that the file is down. It doesnt appear to be downloading.

set /p CONFIRM="Would you like to proceed anyway? (Y/N): "
if /i "%CONFIRM%" NEQ "Y" exit /b

curl -L -o ipget_v0.9.2_windows-amd64.zip https://dist.ipfs.tech/ipget/v0.9.2/ipget_v0.9.2_windows-amd64.zip -s
7z x ipget_v0.9.2_windows-amd64.zip
DEL ipget_v0.9.2_windows-amd64.zip /Q
MOVE ".\ipget\ipget.exe" ".\ipget.exe"
RD /S /Q "ipget"
:check
set size=0
set propersize=22994783619
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (goto done)
)

:download
cls
echo.This May take a while. IPFS is a Distributed filesharing platform. its dependant on how many others are using it..
ipget QmZ4c4QuujGK2e4kyL975Hg2LLAzsgbzq7CbuYAvAzrAhb -o MediCat.USB.v21.12.7z --progress




:done
cls
echo.Completed Downloading, Checking File Size.
set size=0
call :filesize ".\QmVRySjFZ5xFAjQ6waAseMw8DfiDkJWsgdyxR5Uc4W215h\v21.12\MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (goto exit)
echo.the file doesnt appear to be complete.
timeout 3 > nul
goto check


:exit
echo.File Appears to be downloaded successfully.
timeout 5 > nul
exit /b


:filesize
  set size=%~z1
  exit /b 0
