@echo off
cd /d %~dp0
wget "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/MedicatFiles.md5" -O ./MedicatFiles.md5 -q
echo.THIS WILL LOOK FROZEN, DONT PANIC, ITS WORKING!
echo.CANCEL AT ANY TIME BY CLOSING THE QUICKSFV BOX!
QuickSFV.EXE MedicatFiles.md5
del MedicatFiles.md5
del %0 && exit