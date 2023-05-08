@echo off
:check
cls
set size=0
set propersize=22994783619
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (goto done)
)



:: TEMP CODE

:tordown
cls
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/download/tor.bat" -o ./tor.bat -s
call tor.bat
del tor.bat /Q
exit /b

:END TEMP CODE










:download
echo.Downloading from Medicat server.

::Normally Use this file
::curl -# -L -o MediCat.USB.v21.12.7z -C - https://medicatcdn.com/files/v21.12/MediCat.USB.v21.12.7z

::TEMP File until CDN comes back online
curl -# -L -o MediCat.USB.v21.12.7z -C - "https://files.medicatusb.com/?download&weblink=24507515e83f89d2abd8640c756d87cb&realfilename=MediCat.USB.v21.12.7z"



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
