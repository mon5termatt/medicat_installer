@echo off
timeout 2 > nul
DEL "Medicat Installer.exe"
DEL "MediCat_Installer.exe"
DEL "Medicat Installer.bat"
DEL "MediCat_Installer.bat"
cls
echo.Updating Medicat Installer. Please wait while the new file is downloaded.
:: FUCK FUCK FUCK FUCK FUCK FUCK FUCK FUCK FUCK FUCK FUCK FUCK FUCK FUCK FUCK 
curl https://github.com/mon5termatt/medicat_installer/releases/download/3506/Medicat_Installer.bat -o Medicat_Installer.bat -q -L
cls
start cmd /k Medicat_Installer.bat Medicat_Installer.bat
del %0 && exit
