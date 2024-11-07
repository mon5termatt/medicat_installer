@echo off
:check
cls
set size=0
set propersize=22994783619
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (
    echo. File already downloaded.
    timeout 5 > nul
    exit /b
)

set server1=0
set server2=0
set server3=0
set server4=0

echo.Testing Server Download speeds.
echo Testing Server 1 - files.medicatusb.com
FOR /F %%I in ('curl -e https://installer.medicatusb.com --max-time 3 "https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z" -o server1.7z -s -w "%%{speed_download}"') do set server1=%%I
set /a server1=%server1% / 1000000
echo.%server1%mbps

echo Testing Server 2 - medicat.itrio.xyz
FOR /F %%I in ('curl -e https://installer.medicatusb.com --max-time 3 "https://mirrors.itrio.xyz/unpacked/MediCat.USB.v21.12.7z" -o server2.7z -s -w "%%{speed_download}"') do set server2=%%I
set /a server2=%server2% / 1000000
echo.%server2%mbps

echo Testing Server 3 - files.dog
FOR /F %%I in ('curl -e https://installer.medicatusb.com --max-time 3 "https://files.dog/OD%%20Rips/MediCat/v21.12/MediCat.USB.v21.12.7z" -o server3.7z -s -w "%%{speed_download}"') do set server3=%%I
set /a server3=%server3% / 1000000
echo.%server3%mbps

echo Testing Server 4 - cdn.tcbl.dev
FOR /F %%I in ('curl -e https://installer.medicatusb.com --max-time 3 "https://cdn.tcbl.dev/medicat/MediCat.USB.v21.12.7z" -o server4.7z -s -w "%%{speed_download}"') do set server4=%%I
set /a server4=%server4% / 1000000
echo.%server4%mbps


del server1.7z /q
del server2.7z /q
del server3.7z /q
del server4.7z /q

if %server1% geq %server2% (
    if %server1% geq %server3% (
        if %server1% geq %server4% (
            set url="https://files.medicatusb.com/files/v21.12/MediCat.USB.v21.12.7z"
        )
    )
)
if %server2% geq %server1% (
    if %server2% geq %server3% (
        if %server2% geq %server4% (
            set url="https://mirrors.itrio.xyz/unpacked/MediCat.USB.v21.12.7z"
        )
    )
)
if %server3% geq %server1% (
    if %server3% geq %server2% (
        if %server3% geq %server4% (
            set url="https://files.dog/OD%%20Rips/MediCat/v21.12/MediCat.USB.v21.12.7z"
        )
    )
)
if %server4% geq %server1% (
    if %server4% geq %server2% (
        if %server4% geq %server3% (
            set url="https://cdn.tcbl.dev/medicat/MediCat.USB.v21.12.7z"
        )
    )
)

if "%url%"=="" (
    echo.ERROR - No valid server found.
    timeout 5 > nul
    exit /b
)

echo.Downloading from fastest server.
curl -e https://installer.medicatusb.com -# -L -o MediCat.USB.v21.12.7z -C - %url%

:done
cls
echo.Completed Downloading, Checking File Size.
set size=0
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (
    echo.File Appears to be downloaded successfully.
    echo.You may still want to check the hash of the file.
    timeout 5 > nul
) else (
    echo.The file does not appear to be complete.
    timeout 5 > nul
    goto check
)
exit /b

:filesize
  set size=%~z1
  exit /b 0
