@echo off
:check
cls
set size=0
set propersize=22994783619
call :filesize "MediCat.USB.v21.12.7z"
if "%size%" == "%propersize%" (goto done)
)

:download
echo.Downloading from FTP server.
echo.Please Enter the name of Jayros Kitten as the password. First letter Is Capital. (Starts with G)
wget ftp://files.medicatusb.com/tor/MediCat.USB.v21.12.7z --user=Pepe --ask-password -O ./MediCat.USB.v21.12.7z -q --show-progress --progress=bar -c


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