@echo off
cd /d %~dp0
wget "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/MedicatFiles.md5" -O ./MedicatFiles.md5 -q
echo.Now Checking files.
echo.once you see "29652 files checked" it is done.
echo.you can close the quicksfv program at any time to skip the check.
QuickSFV.EXE MedicatFiles.md5
del MedicatFiles.md5
del %0 && exit
