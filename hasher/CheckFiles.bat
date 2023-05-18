@echo off
cd /d %~dp0
wget "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/MedicatFiles.md5" -O ./MedicatFiles.md5 -q
echo.Now Checking files.
start QuickSFV MedicatFiles.md5
del %0 && exit
