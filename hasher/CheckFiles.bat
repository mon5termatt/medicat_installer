@echo off
cd /d %~dp0
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/MedicatFiles.md5" -o  ./MedicatFiles.md5 -s -L
echo.Now Checking files.
start QuickSFV MedicatFiles.md5
del %0 && exit
