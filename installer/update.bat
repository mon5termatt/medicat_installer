@echo off
DEL "Medicat Installer.exe"
DEL "MediCat_Installer.exe"
REN MEDICAT_NEW.EXE "Medicat Installer.exe"
cls
start "Medicat Installer.exe" "Medicat Installer.exe"
del %0 && exit