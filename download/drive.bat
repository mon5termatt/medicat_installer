@echo off
:: https://github.com/prasmussen/gdrive
:7z
REM -- CHECK IF 64BIT
if defined ProgramFiles(x86) (goto 7z64) else (goto 7z32)
:7z32
wget "http://cdn.medicatusb.com/files/install/7z/32.bat" -O ./7z.bat -q
goto 7ze
:7z64
wget "http://cdn.medicatusb.com/files/install/7z/64.bat" -O ./7z.bat -q
goto 7ze
:7ze
CALL 7z.bat
DEL 7z.bat /Q
wget https://github.com/prasmussen/gdrive/releases/download/2.1.1/gdrive_2.1.1_windows_amd64.tar.gz -O ./gdrive.tar.gz
7z e gdrive.tar.gz -r -aoa
7z e gdrive.tar -r -aoa
DEL gdrive.tar.gz
DEL gdrive.tar
cls
echo. Attempting to download from MAIN directory
set file1=1q8gulgxsQjEveNcf2ZLSOpQgroJ78CZq
set file2=1xMiquzKsfW1LUd8RmypMO3Z7PzilY9lu
set file3=1nhFdNHYiWhsh__uL6h1qo2QMceEcqhEy
set file4=1uO9I1poXhwJ-FP7n1kFS6NY7na_ZrSZR
set file5=1ubWGDRP3Cy2bk1yU008TGMQ0zRA2lsVF
set file6=1k78sLJTUyxW-zu7rwn9crHYZjHjah3OE
gdrive download %file1% %options%
gdrive download %file2% %options%
gdrive download %file3% %options%
gdrive download %file4% %options%
gdrive download %file5% %options%
gdrive download %file6% %options%
echo.
title Medicat Installer [HASHCHECK]
cls
if exist "%CD%\*.001" (echo..001 Exists) else (goto gdriveerror)
if exist "%CD%\*.002" (echo..002 Exists) else (goto gdriveerror)
if exist "%CD%\*.003" (echo..003 Exists) else (goto gdriveerror)
if exist "%CD%\*.004" (echo..004 Exists) else (goto gdriveerror)
if exist "%CD%\*.005" (echo..005 Exists) else (goto gdriveerror)
if exist "%CD%\*.006" (echo..006 Exists) else (goto gdriveerror)
goto hashfiles
:gdriveerror
echo. Attempting MIRROR dir
start https://drive.google.com/drive/folders/1FbJtFbWXOPGNSc0ieg8sc5sPvb8PdM_M?usp=sharing
pause
goto verifyfiles
:verifyfiles
Google_Drive_Validate_Files.exe
wget "http://cdn.medicatusb.com/files/hasher/Google_Drive_Validate_Files.exe" -O ./Google_Drive_Validate_Files.exe -q
start Google_Drive_Validate_Files.exe Google_Drive_Validate_Files.exe
:exit
DEL gdrive.exe /q
exit



gdrive [global] download query [options] <query>

global:
  -c, --config <configDir>         Application path, default: /Users/<user>/.gdrive
  --refresh-token <refreshToken>   Oauth refresh token used to get access token (for advanced users)
  --access-token <accessToken>     Oauth access token, only recommended for short-lived requests because of short lifetime (for advanced users)
  --service-account <accountFile>  Oauth service account filename, used for server to server communication without user interaction (file is relative to config dir)
  
options:
  -f, --force       Overwrite existing file
  -r, --recursive   Download directories recursively, documents will be skipped
  --path <path>     Download path


:exit
DEL gdrive.exe /q
exit
