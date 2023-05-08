@echo off
timeout 2 > nul
md bin
Set "Path=%Path%;%CD%;%CD%\bin;"
DEL "Medicat Installer.exe"
DEL "MediCat_Installer.exe"
DEL "Medicat Installer.bat"
DEL "MediCat_Installer.bat"
cls
echo.Updating Medicat Installer. Please wait while the new file is downloaded.
:checkwget
if exist "bin\wget.exe" (goto curver) else (goto curlwget)
:curlwget
echo.
echo.Attempting to download wget using curl.
echo.This requires windows 10 version 1703 or higher.
if not exist bin md bin
if defined ProgramFiles(x86) (set bit=64) else (set bit=32)
curl https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/wget-%bit%.exe -o ./bin/wget.exe
goto checkwget
:curver
curl http://url.medicatusb.com/installerupdate -o Medicat_Installer.bat -q -L
cls
start cmd /k Medicat_Installer.bat Medicat_Installer.bat
del %0 && exit
