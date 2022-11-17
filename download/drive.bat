@echo off
Set "Path=%Path%;%CD%;%CD%\bin;"
set maindir=%CD%
if defined ProgramFiles(x86) (set bit=64) else (set bit=32)
set file1=1q8gulgxsQjEveNcf2ZLSOpQgroJ78CZq
set file2=1xMiquzKsfW1LUd8RmypMO3Z7PzilY9lu
set file3=1nhFdNHYiWhsh__uL6h1qo2QMceEcqhEy
set file4=1uO9I1poXhwJ-FP7n1kFS6NY7na_ZrSZR
set file5=1ubWGDRP3Cy2bk1yU008TGMQ0zRA2lsVF
set file6=1k78sLJTUyxW-zu7rwn9crHYZjHjah3OE
set file7=1b8ZJonZfq9B3UDc1PSAQ2nynE-wRua9z
if exist "%CD%\WPy%bit%-31080\" (goto download)
wget https://github.com/winpython/winpython/releases/download/5.0.20221030final/Winpython%bit%-3.10.8.0dot.exe
Winpython%bit%-3.10.8.0dot.exe -y
:download
cd WPy%bit%-31080\scripts
call env_for_icons.bat
if not "%WINPYWORKDIR%"=="%WINPYWORKDIR1%" cd %WINPYWORKDIR1%
pip install gdown
cls
echo. Attempting to download from MAIN directory
gdown %file1% --folder %maindir%
gdown %file2% --folder %maindir%
gdown %file3% --folder %maindir%
gdown %file4% --folder %maindir%
gdown %file5% --folder %maindir%
gdown %file6% --folder %maindir%
::gdown %file7% --folder %maindir%
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
start https://drive.google.com/drive/folders/1R_05cS0Vq24xfuX27j75OiZrC_mZgQUB
echo.Manually download these
pause
goto verifyfiles
:verifyfiles
wget "http://cdn.medicatusb.com/files/hasher/Google_Drive_Validate_Files.exe" -O ./Google_Drive_Validate_Files.exe -q
start Google_Drive_Validate_Files.exe Google_Drive_Validate_Files.exe
:exit

exit
