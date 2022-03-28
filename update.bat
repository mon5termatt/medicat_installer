@echo off
DEL "Medicat Installer.exe"
DEL "MediCat_Installer.exe"
DEL "Medicat_Installer.bat"
DEL "Medicat Installer.bat"
REN MEDICAT_NEW.bat Medicat_Installer.bat
cls
start cmd /k Medicat_Installer.bat
del %0 && exit