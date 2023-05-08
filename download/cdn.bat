@echo off
:check
cls
set size=0
set propersize=22994783619
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (goto done)
)



:: TEMP CODE

:tempsolution
echo.Unfortunatly the CDN has died for the time being. TOR Will be used in its place. 
timeout 5 > nul
Set "Path=%Path%;%CD%;%CD%\bin;"
wget "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/download/MediCat_USB_v21.12.torrent" -O ./medicat.torrent -q 
if defined ProgramFiles(x86) (set bit=64) else (set bit=32)
wget "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/aria2c-%bit%.exe" -O ./aria2c.exe -q
aria2c.exe --file-allocation=none --seed-time=0 medicat.torrent
MOVE ".\MediCat USB v21.12\MediCat.USB.v21.12.7z" ".\MediCat.USB.v21.12.7z"
RD /S /Q "MediCat USB v21.12"
del medicat.torrent /Q
del aria2c.exe /Q
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
