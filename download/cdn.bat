@echo off
:check
cls
set size=0
set propersize=22994783619
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (goto done)
echo.Testing Server Download speeds.
echo Testing Server 1 - files.medicatusb.com
FOR /F %%I in ('curl -e https://installer.medicatusb.com --max-time 3 "https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z" -o server1.7z -s -w "%%{speed_download}"') do set server1=%%I
set /a server1=%server1% / 1000000
echo.%server1%mbps
echo Testing Server 2 - medicat.itrio.xyz
FOR /F %%I in ('curl -e https://installer.medicatusb.com --max-time 3 "https://mirrors.itrio.xyz/unpacked/MediCat.USB.v21.12.7z" -o server2.7z -s -w "%%{speed_download}"') do set server2=%%I
set /a server2=%server2% / 1000000
echo.%server2%mbps
echo Testing Server 3 - mirror.fangshdow.trade
FOR /F %%I in ('curl -e https://installer.medicatusb.com --max-time 3 "https://mirror.fangshdow.trade/medicat-usb/MediCat%%20USB%%20v21.12/MediCat.USB.v21.12.7z" -o server3.7z -s -w "%%{speed_download}"') do set server3=%%I
set /a server3=%server3% / 1000000
echo.%server3%mbps



timeout 1 >nul
del server1.7z /q
del server2.7z /q
del server3.7z /q

if %server1% geq %server2% (
    if %server1% geq %server3% (
        set url="https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z" & goto download
    )
)
if %server2% geq %server1% (
    if %server2% geq %server3% (
        set url="https://mirrors.itrio.xyz/unpacked/MediCat.USB.v21.12.7z" & goto download
    )
)
if %server3% geq %server1% (
    if %server3% geq %server2% (
        set url="https://mirror.fangshdow.trade/medicat-usb/MediCat%%20USB%%20v21.12/MediCat.USB.v21.12.7z" & goto download
    )
)

:download
echo.Downloading from fastest server.
curl -e https://installer.medicatusb.com -# -L -o MediCat.USB.v21.12.7z -C - %url%

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
