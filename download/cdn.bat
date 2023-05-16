@echo off
:check
cls
set size=0
set propersize=22994783619
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (goto done)
)


:download
echo.Please note download speeds are currently limited due to high traffic
echo.Consider using the torrent as it will be a faster option
sleep 3 > nul

echo.Downloading from Medicat server.
curl -# -L -o MediCat.USB.v21.12.7z -C - https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z





:done
cls
echo.Completed Downloading, Checking File Size.
set size=0
call :filesize "MediCat.USB.v21.12.7z"
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
