@echo OFF & setlocal enabledelayedexpansion
title Medicat Installer [STARTING]
cd /d %~dp0
Set "Path=%Path%;%CD%;%CD%\bin;"
set maindir=%CD%
set localver=3518
set medicatver=21.12
set installertext=[31mM[32mE[33mD[34mI[35mC[36mA[31mT[32m I[33mN[34mS[35mT[36mA[31mL[32mL[33mE[34mR[0m
set format=Yes
set formatcolor=2F
FOR /F "skip=2 tokens=2*" %%a IN ('REG QUERY "HKEY_CURRENT_USER\Control Panel\International" /v "LocaleName"') DO SET "OSLanguage=%%b"
set lang=%OSLanguage:~0,2%
if defined ProgramFiles(x86) (set bit=64) else (set bit=32)
if not exist bin md bin

:initialchecks
echo.Running Initial Checks

:: Define the list of IP addresses to ping
set "ipAddresses=1.1.1.1 8.8.8.8 github.com"

:: Loop through the IP addresses and attempt to ping each one
set "InternetFound="
for %%A in (%ipAddresses%) do (
    echo.Pinging %%A...
    ping %%A -n 1 -w 2000 > nul
    if not errorlevel 1 (
        set "InternetFound=1"
        goto foundinternet
    )
)

:: If no internet connection is found, exit the script
echo.Could not ping any of the specified IP addresses. Exiting Script...
timeout 5 > nul
exit /b

:foundinternet
echo.Found Internet

:: Check for the existence of required tools (curl and powershell)
for %%T in (curl.exe powershell.exe) do (
    where %%T > nul 2>nul
    if errorlevel 1 (
        echo.Could not find %%T. Exiting Script...
        timeout 5 > nul
        exit /b
    )
)

:: Check for admin permissions
echo.Prompting for admin permissions if not run as admin.
timeout 1 > nul
set "_elev="
if /i "%~1"=="-el" set "_elev=1"
set "_PSarg="""%~f0""" -el %_args%" && set "nul=>nul 2>&1" && setlocal EnableDelayedExpansion

%nul% reg query HKU\S-1-5-19 || (
    if not defined _elev (
        %nul% powershell.exe "start cmd.exe -arg '/c \"!_PSarg:'=''!\"' -verb runas"
        exit /b
    )
    echo.Please reopen this script as admin. 
    echo.Veuillez rouvrir ce script en tant qu'administrateur.
    echo.Por favor, reabra este script como administrador.
    echo.Bitte offnen Sie dieses Skript erneut als Administrator.
    echo.Lutfen bu betigi yonetici olarak yeniden acin.
    timeout 5 > nul
    exit /b
)

pushd "%CD%"
CD /D "%~dp0"

:: Get Windows version
for /f "tokens=4-5 delims=. " %%i in ('ver') do set "os2=%%i.%%j"

:: Check Windows version
if "%os2%" neq "10.0" goto :oscheckwarning
goto :oscheckpass

:oscheckwarning
mode con:cols=64 lines=18
title Medicat Installer [UNSUPPORTED]
ver
echo.Warning: Your version of Windows might not be supported.
echo.If you believe this is an error, you can bypass this warning by typing "I AGREE."
Set /P warn=Type "I AGREE" to continue: || set "warn=no"

:: Handle user input
if /i "%warn%" neq "I AGREE" (
    echo.User did not agree. Exiting...
    timeout 5 > nul
    exit /b
)

:oscheckpass
echo.Using supported version of Windows (10/11).
timeout 1 > nul

:curver
mode con:cols=64 lines=18
cls

REM Download the latest version information from GitHub and extract the version
for /f "tokens=*" %%i in ('powershell -c "curl https://api.github.com/repos/mon5termatt/medicat_installer/git/refs/tag -UseBasicParsing | ConvertFrom-Json | Select-Object -Last 1 | ForEach-Object { $_.ref -replace 'refs/tags/', '' }"') do set remote_version=%%i

REM Compare local and remote versions
if "%localver%" GEQ "%remote_version%" goto startup

:updateprogram
cls
echo.A new version of the program has been released. The program will now restart after updating.

REM Download and execute the update script
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/update.bat" -o ./update.bat -s -L
start cmd /k update.bat
exit

:startup
echo.[41m
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII           MEDICAT CONTAINS TOOLS THAT MAY             IIII
echo.IIII          TRIGGER YOUR ANTIVIRUS DUE TO HOW            IIII
echo.IIII               SOME OF THE TOOLS WORK.                 IIII
echo.IIII                                                       IIII
echo.IIII         WE CANT DO ANYTHING TO CHANGE THAT.           IIII
echo.IIII       IF YOU DONT TRUST THE TOOL DONT USE IT.         IIII
echo.IIII         GIANT THANKS, MON5TERMATT AND JAYRO           IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II[0m
echo.Please wait for 5 seconds, Read the above.  
::PING localhost -n 6 >NUL
echo.                          Press any key to accept this warning.&& pause >nul


:startbinfiles
title Medicat Installer [FILECHECK]
cls
set flag=0
set size=0
echo.Downloading Initial Files, Please wait.
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
::if defined ProgramFiles(x86) (goto check64) else (goto check32)

:check64
call :filesize bin/7z.exe
if "%size%" == "1292288" (echo [GOOD] 7z.exe) else (echo [BAD]  7z.exe && set flag=1)
call :filesize bin/7z.dll
if "%size%" == "403456" (echo [GOOD] 7z.dll) else (echo [BAD]  7z.dll && set flag=1)
goto checkdone

:check32
call :filesize bin/7z.exe
if "%size%" == "" (echo GOOD) else (echo BAD && set flag=1)
call :filesize bin/7z.dll
if "%size%" == "" (echo GOOD) else (echo BAD && set flag=1)
goto checkdone

:checkdone
rem dont hash these they change
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/translate/motd.ps1" -o ./bin/motd.ps1 -s -L
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/translate/licence.ps1" -o ./bin/licence.ps1 -s -L
echo.Setting Powershell Settings for Scripts.
Powershell -c "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope LocalMachine"
Powershell -c "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser"
if "%flag%" == "1" goto hasherror
goto start

:hasherror
echo.ERROR DOWNLOADING BIN FILES. ONE OF THE HASHES DOES NOT MATCH.
echo.PLEASE CHECK YOUR FIREWALL AND CURL AND TRY AGAIN. 
echo.IF YOU CHOOSE TO CONTINUE YOU MAY ENCOUNTER ERRORS
pause
pause
pause



:start
@echo off & cls & mode con:cols=100 lines=55 & echo.@@   @@  @@@@@@  @@       @@@@    @@@@   @@   @@  @@@@@@          @@@@@@   @@@@
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
mode con:cols=100 lines=30
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

if "%Errorlevel%"=="1" cls & goto check5
if "%Errorlevel%"=="2" cls & goto formatswitch
if "%Errorlevel%"=="3" cls & goto discord
if "%Errorlevel%"=="4" cls & goto medicatsite
if "%Errorlevel%"=="5" cls & goto recheck

:formatswitch
if "%format%" == "Yes" (
    set format=No
    set formatcolor=4F
) else if "%format%" == "No" (
    set format=Yes
    set formatcolor=2F
)
goto menu2

:check5
echo.Getting Current Ventoy Version
timeout 0 >nul
powershell -c "$data = curl https://api.github.com/repos/ventoy/ventoy/git/refs/tag -UseBasicParsing | ConvertFrom-Json; $data[-1].ref -replace 'refs/tags/', '' | Out-File -Encoding 'UTF8' -FilePath './ventoyversion.txt'"

:: START TEMP CODE 
set /p VENVER= <./ventoyversion.txt
:: set VENVER=v1.0.91
set vencurver=%VENVER:~-6%
echo.Current Online Version - %VENVER:~-6%

:checkventoyver
echo.Checking if current version found on system.
timeout 1 >nul
if exist "%CD%\Ventoy2Disk\" goto checkver else goto ventoyget

:checkver
set /p localver= <.\Ventoy2Disk\ventoy\version
echo.Current Local Version - %VENVER:~-6%
if "%localver%" == "%vencurver%" goto uptodate else goto ventoyget

:ventoyget
echo.Update Found. Downloading Latest Ventoy.
timeout 1 >nul
curl https://github.com/ventoy/Ventoy/releases/download/v%vencurver%/ventoy-%vencurver%-windows.zip -o ./ventoy.zip -s -L
7z x ventoy.zip -r -aoa
RMDIR Ventoy2Disk /S /Q
REN ventoy-%vencurver% Ventoy2Disk
cls
echo.Downloaded newest version.

:doneventoy
timeout 3 >nul
DEL ventoy.zip /Q
del .wget-hsts /Q
del ventoyversion.txt /Q
goto :eof

:uptodate
echo.Local version matches the latest version. Not attempting to update.
goto doneventoy

:install2
title Medicat Installer [CHOOSEINSTALL]
mode con:cols=100 lines=15
echo.We now need to find out what drive you will be installing to.
REM - FOLDER PROMPT STARTS
for /f "delims=" %%A in ('folderbrowse.exe "Please select the drive you want to install medicat on"') do set "folder=%%A"
REM - AND ENDS
set drivepath=%folder:~0,1%
IF "%drivepath%" == "~0,1" GOTO install2
echo.[41m
echo.!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo.PLEASE CONFIRM NOW THAT THIS IS YOUR USB DRIVE.
echo.MEDICAT IS NOT RESPOSIBLE FOR WIPED HARD DRIVES.
echo.!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo.[0m waiting for 5 seconds...
PING localhost -n 6 >NUL
echo.Installing to (%drivepath%). If this is correct just hit enter.
Set /P drivepath=if this is wrong type the correct drive letter now: || Set drivepath=%drivepath%
IF "%drivepath%" == "C" GOTO IMPORTANTDRIVE
if "%format%" == "Yes" (goto formatdrive) else (goto updateventoy)

:formatdrive
set arg1=/GPT
set arg2=/NOSB


:sbask
cls
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII               WOULD YOU LIKE TO USE GPT               IIII
echo.IIII                                                       IIII
echo.IIII              Most computers should be ok              IIII
echo.IIII              with GPT. However some very              IIII
echo.IIII             old machines may have issues.             IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
call Button 10 12 F2 "YES" 46 12 F4 "NO" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 
REM BELOW IS YES
If /I "%Errorlevel%"=="1" (
    set arg1=/GPT
    goto gptask
)
REM BELOW IS NO
If /I "%Errorlevel%"=="2" (
    set "arg1="
    goto gptask
)

:gptask
cls
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII           WOULD YOU LIKE TO USE SECUREBOOT?           IIII
echo.IIII                                                       IIII
echo.IIII            Recommended for most computers             IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
call Button 10 12 F2 "YES" 46 12 F4 "NO" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 
REM BELOW IS YES
If /I "%Errorlevel%"=="1" (
    set arg2=
    goto ventoyinstall
)
REM BELOW IS NO
If /I "%Errorlevel%"=="2" (
    set arg2=/NOSB
    goto ventoyinstall
)

:ventoyinstall
cls
mode con:cols=64 lines=18
echo.[41mII-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                   IMPORTANT WARNING                   IIII
echo.IIII                                                       IIII
echo.IIII           SOMETIMES VENTOY MESSES UP A DRIVE          IIII
echo.IIII   IF THE DRIVE DISAPPEARS PLEASE CHECK DISK MANAGER   IIII
echo.IIII         AND SEE IF THE DRIVE FAILED TO REMOUNT        IIII
echo.IIII  THIS IS A VENTOY BUG AND CANNOT BE FIXED ON OUR END  IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II[0m
echo.Please Wait, Installing Ventoy. 
echo.Please close the file explorer when done.
echo.IF FROZEN FOR MORE THEN 60 SECONDS INSTALL VENTOY MANUALLY.
cd .\Ventoy2Disk\
Ventoy2Disk.exe VTOYCLI /I /Drive:%drivepath%: /NOUSBCheck %arg1% %arg2% 
cd %maindir% 
:vencheck
cls
echo.[41mII-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                   IMPORTANT WARNING                   IIII
echo.IIII                                                       IIII
echo.IIII    PLEASE VERIFY THE DRIVE LETTER HAS NOT CHANGED     IIII
echo.IIII               CURRENTLY INSTALLING TO: %drivepath%              IIII
echo.IIII  IF THIS IS INCORRECT PLEASE TYPE THE CORRECT LETTER  IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II[0m
Set /P drivepath=enter drive letter or hit ENTER: && goto vencheck || Set drivepath=%drivepath%
REM when done
format %drivepath%: /FS:NTFS /X /Q /V:Medicat /Y
goto installver

:error
echo.nothing was chosen, try again
timeout 5
goto install2

:importantdrive
cls
mode con:cols=64 lines=18
echo.[41mII-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                   IMPORTANT WARNING                   IIII
echo.IIII                                                       IIII
echo.IIII       IT LOOKS LIKE YOU SELECTED THE C DRIVE          IIII
echo.IIII        THIS MAY CAUSE IRREPARABLE DAMAGE TO           IIII
echo.IIII               YOUR COMPUTER SYSTEM..                  IIII
echo.IIII           THE PROGRAM WILL NOW ASK AGAIN              IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II[0m
echo.                                                  Press any key 
pause >nul
goto install2
