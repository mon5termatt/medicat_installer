::[Bat To Exe Converter]
::
::YAwzoRdxOk+EWAjk
::fBw5plQjdCmDJHSB8EszKQ9oZCWxFE6IOpgZ7OHY7v64i0MOQOMzdIrJlL2NL4A=
::YAwzuBVtJxjWCl3EqQJgSA==
::ZR4luwNxJguZRRnk
::Yhs/ulQjdF+5
::cxAkpRVqdFKZSDk=
::cBs/ulQjdF+5
::ZR41oxFsdFKZSTk=
::eBoioBt6dFKZSDk=
::cRo6pxp7LAbNWATEpCI=
::egkzugNsPRvcWATEpCI=
::dAsiuh18IRvcCxnZtBJQ
::cRYluBh/LU+EWAnk
::YxY4rhs+aU+JeA==
::cxY6rQJ7JhzQF1fEqQJQ
::ZQ05rAF9IBncCkqN+0xwdVs0
::ZQ05rAF9IAHYFVzEqQJQ
::eg0/rx1wNQPfEVWB+kM9LVsJDGQ=
::fBEirQZwNQPfEVWB+kM9LVsJDGQ=
::cRolqwZ3JBvQF1fEqQJQ
::dhA7uBVwLU+EWDk=
::YQ03rBFzNR3SWATElA==
::dhAmsQZ3MwfNWATElA==
::ZQ0/vhVqMQ3MEVWAtB9wSA==
::Zg8zqx1/OA3MEVWAtB9wSA==
::dhA7pRFwIByZRRnk
::Zh4grVQjdCmDJHSB8EszKQ9oZCWxFE6IOpMV8O3poe+fpy0=
::YB416Ek+ZG8=
::
::
::978f952a14a936cc963da21a135fa983
@echo off & pushd %~dp0
mode con:cols=80 lines=40
title GOOGLE DRIVE FILE NAME FIXER - MON5TERMATT#9999

echo.This Program will ask you for the medicat files.
echo.when asked please select one file to be renamed.
echo.go one by one selecting all six files.
echo.as always feel free to ask for help in the discord.
pause
:start
cls
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/hasher/hashes.bat' -OutFile './hashes.bat'"
call hashes.bat
del hashes.bat /Q
REM -- OPENS A FILE BOX

set dialog="about:<input type=file id=FILE><script>FILE.click();new ActiveXObject
set dialog=%dialog%('Scripting.FileSystemObject').GetStandardStream(1).WriteLine(FILE.value);
set dialog=%dialog%close();resizeTo(0,0);</script>"
for /f "tokens=* delims=" %%p in ('mshta.exe %dialog%') do set "FILE=%%p"

REM -- FIND THE HASH
powershell -c "Get-FileHash %FILE% -Algorithm SHA1" > hash


:1
fc /b file1 hash > nul
if errorlevel 1 (goto 2) else (echo Its 001 && set ext=001 &&  goto renamer) 
:2
fc /b file2 hash > nul
if errorlevel 1 (goto 3) else (echo Its 002 && set ext=002 &&  goto renamer) 
:3
fc /b file3 hash > nul
if errorlevel 1 (goto 4) else (echo Its 003 && set ext=003 &&  goto renamer) 
:4
fc /b file4 hash > nul
if errorlevel 1 (goto 5) else (echo Its 004 && set ext=004 &&  goto renamer) 
:5
fc /b file5 hash > nul
if errorlevel 1 (goto 6) else (echo Its 005 && set ext=005 &&  goto renamer)  
:6
fc /b file6 hash > nul
if errorlevel 1 (goto nf) else (echo Its 006 && set ext=006 &&  goto renamer) 
:NF
echo.the sha1 hash did not match any on record.
echo.Please redownload the files.
pause
goto goodbye

REM -- THIS IS WHEN IT RENAMES THE FILES 

:renamer
set /p ver= < ver
echo.would you like to rename this file now? (Y/N)
choice /C:yn /N /M ""
if errorlevel 2 goto more 
if errorlevel 1 goto rename2
:rename2
REN %file% MediCat.USB.v%ver%.zip.%ext%
:more
echo.do you need to rename more files? (Y/N)
choice /C:yn /N /M ""
if errorlevel 2 goto goodbye
if errorlevel 1 goto start
:goodbye
echo.Goodbye!
del file1 /Q
del file2 /Q
del file3 /Q
del file4 /Q
del file5 /Q
del file6 /Q
del ver /Q
timeout 2 > NUL
exit