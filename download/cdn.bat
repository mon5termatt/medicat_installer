@echo off
:check
cls
set size=0
set propersize=22994783619
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (goto done)
)


echo.Checking Server Speeds
set server=1
:startchecks
if %server%==4 goto dlcheckdone
goto server%server%
:server1
set server=1
set url=https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z
goto startdltest
:server2
set server=2
set url=https://mirrors.itrio.xyz/archive/MediCatUSBv21.12.7z
goto startdltest
:server3
set server=3
set url="https://mirror.fangshdow.trade/medicat-usb/MediCat%%20USB%%20v21.12/MediCat.USB.v21.12.7z"
goto startdltest
:startdltest
curl --max-time 3 %url% -o server%server%.7z -s
set /a server=(%server%)+1
goto startchecks

:dlcheckdone
for %%I in (server1.7z) do set /a size1=%%~zI
for %%I in (server2.7z) do set /a size2=%%~zI
for %%I in (server3.7z) do set /a size3=%%~zI
del server1.7z /q
del server2.7z /q
del server3.7z /q
echo.Please Select the LARGEST number from the following
echo.Mirror 1 - %size1%
echo.Mirror 2 - %size2%
echo.Mirror 3 - %size3%
echo.
choice /C:123 /N /M "1/2/3"

if errorlevel 3 set /a server=3 && goto set
if errorlevel 2 set /a server=2 && goto set 
if errorlevel 1 set /a server=1 && goto set

:set
if %server% == 1 set url="https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z"
if %server% == 2 set url="https://mirrors.itrio.xyz/archive/MediCatUSBv21.12.7z"
if %server% == 3 set url="https://mirror.fangshdow.trade/medicat-usb/MediCat%%20USB%%20v21.12/MediCat.USB.v21.12.7z"


:download
echo.Downloading from Medicat server.
curl -# -L -o MediCat.USB.v21.12.7z -C - %url%

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
