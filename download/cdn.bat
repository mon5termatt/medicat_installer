@echo off
:check
cls
set size=0
set propersize=22994783619
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (goto done)
)

echo Testing Server 1 - Medicat CDN
FOR /F %%I in ('curl --max-time 2 "https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z" -o server1.7z -s -w "%%{speed_download}"') do set server1=%%I
set /a server1=%server1% / 1000000
echo Testing Server 2 - medicat.itrio.xyz
FOR /F %%I in ('curl --max-time 2 "https://mirrors.itrio.xyz/unpacked/MediCat.USB.v21.12.7z" -o server2.7z -s -w "%%{speed_download}"') do set server2=%%I
set /a server2=%server2% / 1000000
echo Testing Server 3 - mirror.fangshdow.trade
FOR /F %%I in ('curl --max-time 2 "https://mirror.fangshdow.trade/medicat-usb/MediCat%%20USB%%20v21.12/MediCat.USB.v21.12.7z" -o server3.7z -s -w "%%{speed_download}"') do set server3=%%I
set /a server3=%server3% / 1000000

del server1.7z /q
del server2.7z /q
del server3.7z /q
echo.Please Select the fastest server for you.
echo.%server1% Mbp/s - Server 1 - Medicat CDN
echo.%server2% Mbp/s - Server 2 - medicat.itrio.xyz
echo.%server3% Mbp/s - Server 3 - mirror.fangshdow.trade
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
echo.Downloading from selected server.
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
