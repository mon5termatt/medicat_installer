@echo OFF & setlocal enabledelayedexpansion
title Medicat Installer [STARTING]
cd /d %~dp0

:: Initialize variables
Set "Path=%Path%;%CD%;%CD%\bin;"
set "maindir=%CD%"
set "localver=3520"
set "medicatver=21.12"
set "installertext=[31mM[32mE[33mD[34mI[35mC[36mA[31mT[32m I[33mN[34mS[35mT[36mA[31mL[32mL[33mE[34mR[0m"
set "format=Yes"
set "formatcolor=2F"
set "temp_files=curver.ini ventoy.zip .wget-hsts ventoyversion.txt drivefiles.md5 tor.bat cdn.bat"

:: Get system info
FOR /F "skip=2 tokens=2*" %%a IN ('REG QUERY "HKEY_CURRENT_USER\Control Panel\International" /v "LocaleName"') DO SET "OSLanguage=%%b"
set "lang=%OSLanguage:~0,2%"
if defined ProgramFiles(x86) (set "bit=64") else (set "bit=32")

:: Create bin directory if needed
if not exist bin md bin

:: Initial checks function
:initialchecks
echo.Running initial checks...
call :check_internet || exit
call :check_curl || exit
call :check_powershell || exit
call :check_admin || exit
call :check_windows || exit
goto :curver

:check_internet
for %%a in (1.1.1.1 8.8.8.8 github.com) do (
    ping %%a -n 1 -w 2000 >nul && (
        echo.Found Internet
        exit /b 0
    )
)
echo.Could not connect to the internet
exit /b 1

:check_curl
curl.exe -V >nul 2>&1 || (
    echo Failed to find cURL, please install it!
    exit /b 1
)
echo.Found cURL
exit /b 0

:check_powershell
powershell.exe -? >nul 2>&1 || (
    echo.Could not find PowerShell, please install it!
    exit /b 1
)
echo.Found Powershell
exit /b 0

:check_admin
echo.Prompting for admin permissions if not run as admin.
net session >nul 2>&1 || (
    echo.Please run this script as administrator
    pause
    exit /b 1
)
exit /b 0

:check_windows
for /f "tokens=4-5 delims=. " %%i in ('ver') do set "os2=%%i.%%j"
if "%os2%" == "10.0" exit /b 0
mode con:cols=64 lines=18
title Medicat Installer [UNSUPPORTED]
ver
echo WARNING!!!
echo Your Windows version is NOT supported
echo Things might break if you continue.
set /P "warn=Type exactly 'I AGREE' to continue: " || set "warn=no"
if /i "%warn%"=="I AGREE" exit /b 0
exit /b 1

:curver
mode con:cols=64 lines=18
cls
powershell -c "$data = curl https://api.github.com/repos/mon5termatt/medicat_installer/git/refs/tag -UseBasicParsing | ConvertFrom-Json; $data[-1].ref -replace 'refs/tags/', '' | Out-File -Encoding 'UTF8' -FilePath './curver.ini'"
set /p remver=< curver.ini
set "remver=%remver:~-4%"
del curver.ini /Q
if %localver% GEQ %remver% goto startup

:updateprogram
cls
echo.A new version of the program has been released. The program will now restart.
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/update.bat" -o ./update.bat -s -L
start cmd /k update.bat
exit

:startup
echo.[41m
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                      WARNING!!!                       IIII
echo.IIII           MEDICAT CONTAINS TOOLS THAT MAY             IIII
echo.IIII          TRIGGER YOUR ANTIVIRUS DUE TO HOW            IIII
echo.IIII               SOME OF THE TOOLS WORK.                 IIII
echo.IIII                                                       IIII
echo.IIII         WE CANT DO ANYTHING TO CHANGE THAT.           IIII
echo.IIII       IF YOU DON'T TRUST THE TOOL, DON'T USE IT.      IIII
echo.IIII         GIANT THANKS, MON5TERMATT AND JAYRO           IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II[0m
echo.Please wait for 5 seconds, and read the above.  
PING localhost -n 6 >NUL
echo.                          Press any key to accept this warning.&& pause >nul

:startbinfiles
title Medicat Installer [FILECHECK]
cls
echo.Downloading initial files, please wait...
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/QuickSFV.EXE" -o ./bin/QuickSFV.exe -s -L
call :filesize bin/QuickSFV.exe
if "%size%" == "103424" (echo [GOOD] QuickSFV.exe) else (echo [BAD]  QuickSFV.exe && set flag=1)
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/QuickSFV.ini" -o ./bin/QuickSFV.ini -s -L
call :filesize bin/QuickSFV.ini
if "%size%" == "158" (echo [GOOD] QuickSFV.ini) else (echo [BAD]  QuickSFV.ini && set flag=1)
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/Box.bat" -o ./bin/Box.bat -s -L
call :filesize bin/Box.bat
if "%size%" == "5874" (echo [GOOD] Box.bat) else (echo [BAD]  Box.bat && set flag=1)
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/Button.bat" -o ./bin/Button.bat -s -L
call :filesize bin/Button.bat
if "%size%" == "5254" (echo [GOOD] Button.bat) else (echo [BAD]  Button.bat && set flag=1)
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/GetInput.exe" -o ./bin/GetInput.exe -s -L
call :filesize bin/GetInput.exe
if "%size%" == "3584" (echo [GOOD] GetInput.exe) else (echo [BAD]  GetInput.exe && set flag=1)
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/Getlen.bat" -o ./bin/Getlen.bat -s -L
call :filesize bin/Getlen.bat
if "%size%" == "1897" (echo [GOOD] Getlen.bat) else (echo [BAD]  Getlen.bat && set flag=1)
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/batbox.exe" -o ./bin/batbox.exe -s -L
call :filesize bin/batbox.exe
if "%size%" == "1536" (echo [GOOD] batbox.exe) else (echo [BAD]  batbox.exe && set flag=1)
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/folderbrowse.exe" -o ./bin/folderbrowse.exe -s -L
call :filesize bin/folderbrowse.exe
if "%size%" == "8192" (echo [GOOD] folderbrowse.exe) else (echo [BAD]  folderbrowse.exe && set flag=1)
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/7z/%bit%.exe" -o ./bin/7z.exe -s -L
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/7z/%bit%.dll" -o ./bin/7z.dll -s -L

goto checkdone

:check64
call :filesize bin/7z.exe
if "%size%" == "1269760" (echo [GOOD] 7z.exe) else (echo [BAD]  7z.exe && set flag=1)
call :filesize bin/7z.dll
if "%size%" == "389120" (echo [GOOD] 7z.dll) else (echo [BAD]  7z.dll && set flag=1)
goto checkdone

:check32
call :filesize bin/7z.exe
if "%size%" == "" (echo GOOD) else (echo BAD && set flag=1)
call :filesize bin/7z.dll
if "%size%" == "" (echo GOOD) else (echo BAD && set flag=1)
goto checkdone

:checkdone
rem don't hash these, they change!
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/translate/motd.ps1" -o ./bin/motd.ps1 -s -L
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/translate/licence.ps1" -o ./bin/licence.ps1 -s -L
echo.Setting PowerShell settings for scripts.
Powershell -c "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope LocalMachine"
Powershell -c "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser"
if "%flag%" == "1" goto hasherror
goto start

:hasherror
echo WARNING!!!
echo.ERROR DOWNLOADING BIN FILES. ONE OF THE HASHES DOES NOT MATCH.
echo.PLEASE CHECK YOUR FIREWALL AND CURL AND TRY AGAIN. 
echo.IF YOU CHOOSE TO CONTINUE, YOU MAY ENCOUNTER ERRORS.
pause
pause > nul
pause > nul

:start
cls
mode con:cols=100 lines=55
echo.@@   @@  @@@@@@  @@       @@@@    @@@@   @@   @@  @@@@@@          @@@@@@   @@@@                 
echo.@@   @@  @@      @@      @@  @@  @@  @@  @@@ @@@  @@                @@    @@  @@                
echo.@@ @ @@  @@@@    @@      @@      @@  @@  @@ @ @@  @@@@              @@    @@  @@                
echo.@@@@@@@  @@      @@      @@  @@  @@  @@  @@   @@  @@                @@    @@  @@                
echo. @@ @@   @@@@@@  @@@@@@   @@@@    @@@@   @@   @@  @@@@@@            @@     @@@@                 
echo.                                                                                                
echo.        @@@@@@  @@  @@  @@@@@@          @@   @@  @@@@@@  @@@@@   @@@@@@   @@@@    @@@@   @@@@@@ 
echo.          @@    @@  @@  @@              @@@ @@@  @@      @@  @@    @@    @@  @@  @@  @@    @@   
echo.          @@    @@@@@@  @@@@            @@ @ @@  @@@@    @@  @@    @@    @@      @@@@@@    @@   
echo.          @@    @@  @@  @@              @@   @@  @@      @@  @@    @@    @@  @@  @@  @@    @@   
echo.          @@    @@  @@  @@@@@@          @@   @@  @@@@@@  @@@@@   @@@@@@   @@@@   @@  @@    @@   
echo.                                                                                                
echo.                @@@@@@  @@  @@   @@@@   @@@@@@   @@@@   @@      @@      @@@@@@  @@@@@           
echo.                  @@    @@@ @@  @@        @@    @@  @@  @@      @@      @@      @@  @@          
echo.                  @@    @@ @@@   @@@@     @@    @@@@@@  @@      @@      @@@@    @@@@@           
echo.                  @@    @@  @@      @@    @@    @@  @@  @@      @@      @@      @@  @@          
echo.                @@@@@@  @@  @@   @@@@     @@    @@  @@  @@@@@@  @@@@@@  @@@@@@  @@  @@          
echo.												
echo.  .........                                                              .........  
echo.  ................                     ......                     ................  
echo.  .....................       ........................       .....................  
echo.  .... ................. .................................  ................. ...   
echo.   ... [101m0@@@00ooo[0m....................................................[101mooo00@@@0[0m ...   
echo.   ... [101mo@@@@@@@@@0o[0m..............................................[101mo@@@@@@@@@@o[0m ...   
echo.    ....[101m0@@@@@@0o[0m..................................................[101mo@@@@@@@0[0m....    
echo.    ... [101mo@@@@@o[0m......................................................[101m0@@@@@o[0m ...    
echo.     ....[101m0@@0[0m.........................................................[101mo@@@0[0m....     
echo.     ... [101m.@0[0m............................................................[101m0@[0m.....     
echo.      ... [101mo[0m..............................................................[101mo[0m ...      
echo.       ... .............................................................  ...       
echo.        .. .............................................................. ..        
echo.          ...........[42mo0o.....ooo[0m....................[42mooo.....o0o[0m...........          
echo.          .........[42mo00o[40m       [42mo00o[0m.......oo.......[42mo000[40m       [42mo00o[0m.........          
echo.         .........[42mo00o[40m         [42m000o[0m......00......[42mo00o[40m         [42m000o[0m........          
echo.         ..........[42mo00[40m         [42m000o[0m ....0@@0.....[42m000o[40m         [42m00o[0m.........          
echo.  ...oooooooooo.....[42mo0o[40m       [42mo00o[0m.....0@@@@0.....[42mo00o[40m       [42mo0o[0m.....oooooooooo...  
echo.  ........ooooooooo...[42mooo...oooo[0m.....o0@@@@@@0o.....[42moooo...ooo[0m..oooooooooo......o.  
echo.       ...........oo................o0@@@@@@@@@o...............ooo...........       
echo.       ..ooooooooooo..............o0@@@@@@@@@@@@0o..............ooooooooooo..       
echo..oooooooooooooooooooo..........oo0@@@@@@@@@@@@@@@@0oo..........ooooooooooooooooooo..
echo....      ...................oo0@@@@@@@@@@@@@@@@@@@@@@0oo...................     ....
echo.          ..ooooooooooooo00@@@@@oo@@@@@@0oo0@@@@@0oo@@@@@00ooooooooooooo..          
echo.        .oooo.o00000@@@@@@@@@@@@@ooooooo000oooooo.o@@@@@@@@@@@@@0000o..oooo.        
echo.      ooo.  .0@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@0oo0@@@@@@@@@@@@@@@@@@@o. ..oo.      
echo.              .oo0@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@0oo.              
echo.                  .oo0@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@0oo.                  
echo.                      ..ooo000@@@@@@@@@@@@@@@@@@@@@@@@000ooo..                      
echo.                              ....oooooooooooooooo....                              
pause
:menu
title Medicat Installer [%localver%]
mode con:cols=100 lines=55
cls
powershell bin\licence.ps1 %lang%
echo.
pause

:menu2
cls
mode con:cols=64 lines=20
echo.  %installertext%   %installertext%   %installertext%
call Button 1 2 F2 "INSTALL MEDICAT" 23 2 %formatcolor% "TOGGLE DRIVE FORMAT (CURRENTLY %format%)" 1 7 F9 "MEDICAT DISCORD" 23 7 F9 "VISIT SITE" 40 7 F9 "CHECK USB FILES"  X _Var_Box _Var_Hover
echo.
echo.
echo.
echo.VERSION %localver% BY MON5TERMATT. 
echo.
powershell bin\motd.ps1 %lang%
GetInput /M %_Var_Box% /H %_Var_Hover% 

:: Process menu choices more efficiently
set "choice=%errorlevel%"
if "%choice%"=="1" goto check5
if "%choice%"=="2" call :toggle_format
if "%choice%"=="3" start https://url.medicatusb.com/discord
if "%choice%"=="4" start https://medicatusb.com
if "%choice%"=="5" call :check_usb_files
goto menu2

:toggle_format
if "%format%"=="Yes" (
    set "format=No"
    set "formatcolor=4F"
) else (
    set "format=Yes"
    set "formatcolor=2F"
)
exit /b

:check_usb_files
for /f "delims=" %%A in ('folderbrowse.exe "Please select the drive you want to check"') do set "folder=%%A"
set "drivepath=%folder:~0,1%"
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/CheckFiles.bat" -o %drivepath%:/CheckFiles.bat -s -L
pushd %drivepath%:
start cmd /k CheckFiles.bat
popd
exit /b

:install_medicat
:: Consolidated installation function
set "install_error="
set "ventoy_installed="

:: 1. Get drive selection
call :get_drive_selection || exit /b
if "%drivepath%"=="C" goto drive_warning

:: 2. Format if requested
if "%format%"=="Yes" call :format_drive

:: 3. Install/Update Ventoy
call :install_ventoy || set "install_error=1"

:: 4. Install Medicat
if not defined install_error call :install_medicat_files

:: 5. Finish up
if not defined install_error (
    call :finish_installation
) else (
    call :show_error
)
exit /b

:get_drive_selection
for /f "delims=" %%A in ('folderbrowse.exe "Please select the drive you want to install Medicat on"') do set "folder=%%A"
set "drivepath=%folder:~0,1%"
if "%drivepath%"=="~0,1" exit /b 1
echo.Installing to (%drivepath%). Press Enter to confirm or type a new drive letter.
set /P "drivepath=" || set "drivepath=%drivepath%"
exit /b 0

:format_drive
echo.Formatting drive %drivepath%...
format %drivepath%: /FS:NTFS /X /Q /V:Medicat /Y
exit /b

:install_ventoy
echo.Installing/Updating Ventoy...
cd .\Ventoy2Disk\
if "%format%"=="Yes" (
    Ventoy2Disk.exe VTOYCLI /I /Drive:%drivepath%: /NOUSBCheck %arg1% %arg2%
) else (
    Ventoy2Disk.exe VTOYCLI /U /Drive:%drivepath%:
)
cd %maindir%
exit /b

:install_medicat_files
:: Check for existing files
if exist "%CD%\MediCat.USB.v%medicatver%.7z" (
    7z x -O%drivepath%: "MediCat.USB.v%medicatver%.7z" -r -aoa
) else if exist "%CD%\MediCat.USB.v%medicatver%.zip.001" (
    7z x -O%drivepath%: "MediCat.USB.v%medicatver%.zip.001" -r -aoa
) else (
    call :download_medicat_files
)
exit /b

:finish_installation
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/icon.ico" -o %drivepath%:/autorun.ico -s -L
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/CheckFiles.bat" -o %drivepath%:/CheckFiles.bat -s -L
cd /d %drivepath%:
start cmd /k CheckFiles.bat
echo.Installation Complete!
pause
exit /b

:download_medicat_files
mode con:cols=64 lines=18
echo.Would you like to download Medicat files?
echo.1. Download via Torrent (Faster)
echo.2. Download via CDN (Direct)
echo.3. Cancel
choice /c 123 /n
if errorlevel 3 exit /b 1
if errorlevel 2 (
    curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/download/cdn.bat" -o ./cdn.bat -s -L
    call cdn.bat
    del cdn.bat /Q
) else (
    curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/download/tor.bat" -o ./tor.bat -s -L
    call tor.bat
    del tor.bat /Q
)
exit /b

:show_error
echo.[41m
echo.An error occurred during installation. Common errors:
echo.1. Unexpected end of archive - Redownload main file
echo.2. Cannot find path specified - Check disk mounting
echo.[0m
pause
exit /b

:cleanup
:: Clean up temporary files
for %%f in (%temp_files%) do if exist "%%f" del "%%f" /Q
exit /b

:drive_warning
mode con:cols=64 lines=18
echo.[41m WARNING: C: DRIVE SELECTED [41m
echo.Installing to the C: drive may cause
echo.irreparable damage to your system.
echo.[0m
pause
goto get_drive_selection

:check5
echo.Getting current Ventoy version...
timeout 0 >nul
powershell -c "$data = curl https://api.github.com/repos/ventoy/ventoy/git/refs/tag -UseBasicParsing | ConvertFrom-Json; $data[-1].ref -replace 'refs/tags/', '' | Out-File -Encoding 'UTF8' -FilePath './ventoyversion.txt'"

::START TEMP CODE 
set /p VENVER= <./ventoyversion.txt
::set VENVER=v1.0.91
set vencurver=%VENVER:~-6%
echo.Current online version - %VENVER:~-6%

:checkventoyver
echo.Checking if current version found on system...
timeout 1 >nul
if exist "%CD%\Ventoy2Disk\" (goto checkver) else (goto ventoyget)
:checkver
set /p localver= <.\Ventoy2Disk\ventoy\version
echo.Current Local Version - %VENVER:~-6%
if "%localver%" == "%vencurver%" (goto uptodate) else (goto ventoyget)
:ventoyget
echo.Update found, downloading latest Ventoy...
timeout 1 >nul
curl https://github.com/ventoy/Ventoy/releases/download/v%vencurver%/ventoy-%vencurver%-windows.zip -o ./ventoy.zip -s -L
7z x ventoy.zip -r -aoa
RMDIR Ventoy2Disk /S /Q
REN ventoy-%vencurver% Ventoy2Disk
cls
echo.Downloaded newest version.
goto doneventoy
:uptodate
echo.Local version matches latest version. Not attempting to update.
:doneventoy
timeout 3 >nul
DEL ventoy.zip /Q
del .wget-hsts /Q
del ventoyversion.txt /Q

:exit
exit

:filesize
  set size=%~z1
  exit /b 0
