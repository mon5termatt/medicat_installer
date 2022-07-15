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
echo.If your OS is older then 1703, first off. Update ffs.
echo.second off. Manually download wget from https://eternallybored.org/misc/wget/1.21.3/64/wget.exe
curl -O -s https://eternallybored.org/misc/wget/1.21.3/64/wget.exe
move .\wget.exe .\bin\wget.exe
goto checkwget
:curver
wget "http://url.medicatusb.com/installerupdate" -O ./Medicat_Installer.bat -q
cls
start cmd /k Medicat_Installer.bat Medicat_Installer.bat
del %0 && exit
