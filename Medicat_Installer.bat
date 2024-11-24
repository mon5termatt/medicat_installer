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
echo.Running initial checks...
ping 1.1.1.1 -n 1 -w 1000 > nul
if errorlevel 1 (Echo.Could not ping 1.1.1.1, attempting backup pings.) else (goto foundinternet)
ping 8.8.8.8 -n 1 -w 2000 > nul
if errorlevel 1 (Echo.Could not ping 8.8.8.8, attempting backup pings.) else (goto foundinternet)
ping github.com -n 1 -w 2000 > nul
if errorlevel 1 (Echo.Could not Ping github.com, Exiting Script && timeout 5 > nul && exit) else (goto foundinternet)
:foundinternet
echo.Found Internet
curl.exe -V > nul
if errorlevel 1 (echo Failed to find cURL, please install it! && pause && exit)
echo.Found cURL
for %%# in (powershell.exe) do @if "%%~$PATH:#"=="" (echo.Could not find PowerShell, please install it! && pause && exit) 
echo.Found Powershell
echo.Prompting for admin permissions if not run as admin.
timeout 1 >nul
set _elev=
if /i "%~1"=="-el" set _elev=1
set _PSarg="""%~f0""" -el %_args% && set "nul=>nul 2>&1" && setlocal EnableDelayedExpansion
%nul% reg query HKU\S-1-5-19 || (
if not defined _elev %nul% powershell.exe "start cmd.exe -arg '/c \"!_PSarg:'=''!\"' -verb runas" && exit /b
	echo.Please reopen this script as admin. 
	echo.Veuillez rouvrir ce script en tant qu'administrateur.
	echo.Por favor, reabra este script como administrador.
	echo.Bitte offnen Sie dieses Skript erneut als Administrator.
	echo.Lutfen bu betigi yonetici olarak yeniden acin. && pause && exit /b
)

pushd "%CD%"
CD /D "%~dp0"
for /f "tokens=4-5 delims=. " %%i in ('ver') do set os2=%%i.%%j
if "%os2%" == "10.0" goto oscheckpass
mode con:cols=64 lines=18
title Medicat Installer [UNSUPPORTED]
ver
echo WARNING!!!

echo Your Windows version is NOT supported, most likely you're using a insider build.
echo Things might break if you continue.
echo If you believe this is an error, you can
set /P warn=bypass this warning by typing exactly "I AGREE": || set warn=no
If "%warn%"=="no" exit
If /i "%warn%"=="I AGREE" goto oscheckpass
:oscheckpass
echo.Using supported version of Windows 10/11.
timeout 1 > nul

:curver
mode con:cols=64 lines=18
cls
powershell -c "$data = curl https://api.github.com/repos/mon5termatt/medicat_installer/git/refs/tag -UseBasicParsing | ConvertFrom-Json; $data[-1].ref -replace 'refs/tags/', '' | Out-File -Encoding 'UTF8' -FilePath './curver.ini'"
set /p remver= < curver.ini
set remver=%remver:~-4%
del curver.ini /Q
if "%localver%" GEQ "%remver%" (goto startup)

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
::if defined ProgramFiles(x86) (goto check64) else (goto check32)

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
echo.Setting Powershell Settings for Scripts.
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


If /I "%Errorlevel%"=="1" (
	cls & goto check5
)
If /I "%Errorlevel%"=="2" (
	cls & goto formatswitch
)
If /I "%Errorlevel%"=="3" (
	cls & goto discord
)
If /I "%Errorlevel%"=="4" (
	cls & goto medicatsite
)
If /I "%Errorlevel%"=="5" (
	cls & goto recheck
)

:formatswitch
if "%format%" == "Yes" (goto fs2) else (echo.>nul)
if "%format%" == "No" (goto fs3) else (goto menu2)
:fs2
set format=No
set formatcolor=4F
goto menu2
:fs3
set format=Yes
set formatcolor=2F
goto menu2



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





:install2
title Medicat Installer [CHOOSEINSTALL]
mode con:cols=100 lines=15
echo.We now need to find out what drive you will be installing to.
REM - FOLDER PROMPT STARTS
for /f "delims=" %%A in ('folderbrowse.exe "Please select the drive you want to install medicat on"') do set "folder=%%A"
REM - AND ENDS
set drivepath=%folder:~0,1%
IF "%drivepath%" == "~0,1" GOTO install2
echo.Installing to (%drivepath%). If this is correct just hit enter.
echo.[41m
echo.!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo.PLEASE CONFIRM NOW THAT THIS IS YOUR USB DRIVE.
echo.MEDICAT IS NOT RESPOSIBLE FOR WIPED USB DRIVES.
echo.!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo.[0m waiting for 5 seconds...
PING localhost -n 6 >NUL
Set /P drivepath=if this is wrong type the correct drive letter now: || Set drivepath=%drivepath%
IF "%drivepath%" == "C" GOTO IMPORTANTDRIVE
if "%format%" == "Yes" (goto formatdrive) else (goto updateventoy)

:formatdrive
set arg1=/GPT
set arg2=/NOSB

:sbask
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII              WOULD YOU LIKE TO USE GPT?               IIII
echo.IIII                                                       IIII
echo.IIII              Most computers should be OK              IIII
echo.IIII             with GPT. However, some very              IIII
echo.IIII             old machines may have issues.             IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
call Button 10 12 F2 "YES" 46 12 F4 "NO" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 
REM BELOW IS YES
If /I "%Errorlevel%"=="1" (
	cls & set arg1=/GPT & goto gptask
)
REM BELOW IS NO
If /I "%Errorlevel%"=="2" (
	cls & set "arg1=" & goto gptask
)
goto sbask

:gptask
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
	cls & set "arg2=" & goto ventoyinstall
)
REM BELOW IS NO
If /I "%Errorlevel%"=="2" (
	cls & set set arg2=/NOSB & goto ventoyinstall
)
goto sbask



:ventoyinstall
echo.[41mII-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                 !IMPORTANT WARNING!                   IIII
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
echo.IIII         CURRENTLY INSTALLING TO: %drivepath%          IIII
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



REM -- GO TO END OF FILE FOR MOST EXTRACTIONS
REM -- WHEN DONE EXTRACTING VENTOY, TYPE LICENCE AND CONTINUE


:updateventoy
echo.Please Wait, Updating Ventoy but not formatting
cd .\Ventoy2Disk\
Ventoy2Disk.exe VTOYCLI /U /Drive:%drivepath%:
cd %maindir%
goto installver

:installver
title Medicat Installer [INSTALL!]
if exist "%CD%\MediCat.USB.v%medicatver%.7z" (goto install4) else (goto installversion2)
:installversion2
if exist "%CD%\MediCat.USB.v%medicatver%.zip.001" (goto hasher)
cls
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII         COULD NOT FIND THE MEDICAT FILE(S).           IIII
echo.IIII            (EITHER *.001 or the main .7z)             IIII
echo.IIII             are they in The same folder?              IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII           WOULD YOU LIKE TO DOWNLOAD THEM?            IIII
echo.IIII       SELECTING NO WILL LET YOU FIND THE FILE.        IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
call Button 10 12 F2 "YES" 46 12 F4 "NO" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 

If /I "%Errorlevel%"=="1" (
	cls & goto bigboi
)
If /I "%Errorlevel%"=="2" (
	cls & goto installerror
)




:installerror
mode con:cols=64 lines=20
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII        THE INSTALLER COULD NOT FIND MEDICAT           IIII
echo.IIII                                                       IIII
echo.IIII          PLEASE MANUALLY SELECT THE FILE!             IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II

set dialog="about:<input type=file id=FILE><script>FILE.click();new ActiveXObject
set dialog=%dialog%('Scripting.FileSystemObject').GetStandardStream(1).WriteLine(FILE.value);
set dialog=%dialog%close();resizeTo(0,0);</script>"

for /f "tokens=* delims=" %%p in ('mshta.exe %dialog%') do set "file=%%p"
mode con:cols=100 lines=15
goto install6


REM -- ACTUALLY EXTRACT/INSTALL

:install4
mode con:cols=100 lines=20
set file="MediCat.USB.v21.12.7z"
set sha256=a306331453897d2b20644ca9334bb0015b126b8647cecec8d9b2d300a0027ea4
set sha1=2cbf5f337849a11084124a79a1b8d7e77eaca7d5
7z x -O%drivepath%: %file% -r -aoa
goto finishup

:install5
mode con:cols=100 lines=20
set file="MediCat.USB.v%medicatver%.zip.001"
7z x -O%drivepath%: %file% -r -aoa
goto finishup

:install6
mode con:cols=100 lines=20
7z x -O%drivepath%: "%file%" -r -aoa
goto finishup

:finishup
if errorlevel 255 goto finisherror
if errorlevel 8 goto finisherror
if errorlevel 7 goto finisherror
if errorlevel 2 goto finisherror
if errorlevel 1 goto finisherror

::it worked
if errorlevel 0 goto finishup2

:finishup2
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/icon.ico" -o %drivepath%:/autorun.ico -s -L
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/CheckFiles.bat" -o %drivepath%:/CheckFiles.bat -s -L
cd /d %drivepath%:
start cmd /k CheckFiles.bat
echo.The Installer Has Completed.
pause
exit

:finisherror
echo.[41m
echo.An error has occurred. Please look above.
echo.If you come to the Discord for support, we will need this error. 
echo.COMMON ERRORS: 
echo.Error: Unexpected end of archive		FIX: Redownload main file.  (download issue)
echo.Error: Cannot find the path specified	FIX: Check disk is mounted. (ventoy issue)
echo.[0m
pause
exit

::---------------------------------------------------------------------------------------------------------------------------------------------------
::---------------------------------------------------------------------------------------------------------------------------------------------------
::---------------------------------------------------------------------------------------------------------------------------------------------------
::---------------------------------------------------------------------------------------------------------------------------------------------------
::---------------------------------------------------------------------------------------------------------------------------------------------------
::---------------------------------------------------------------------------------------------------------------------------------------------------
::---------------------------------------------------------------------------------------------------------------------------------------------------
::---------------------------------------------------------------------------------------------------------------------------------------------------
::---------------------------------------------------------------------------------------------------------------------------------------------------




:hasher
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/drivefiles.md5" -o ./drivefiles.md5 -s -L
start QuickSFV.EXE drivefiles.md5
DEL drivefiles.md5 /Q
goto install5

:medicatsite
start https://medicatusb.com
goto menu2

:discord
start https://url.medicatusb.com/discord
goto menu2

:recheck
for /f "delims=" %%A in ('folderbrowse.exe "Please select the drive you want to install Medicat on"') do set "folder=%%A"
set drivepath=%folder:~0,1%
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/hasher/CheckFiles.bat" -o %drivepath%:/CheckFiles.bat -s -L
cd /d %drivepath%:
start cmd /k CheckFiles.bat
cd /d %maindir%
goto menu2




:bigboi
cls
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII         HOW WOULD YOU LIKE TO GRAB THE FILES?         IIII
echo.IIII                                                       IIII
echo.IIII         FASTER                         DIRECT         IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
call Button 15 6 F2 "TORRENT" 40 6 F2 "CDN" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 
If /I "%Errorlevel%"=="1" (
	cls & goto tordown
)
If /I "%Errorlevel%"=="2" (
	cls & goto cdndown
)
goto bigboi

:tordown
cls
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/download/tor.bat" -o ./tor.bat -s -L
call tor.bat
del tor.bat /Q
goto installver

:cdndown
cls
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/download/cdn.bat" -o ./cdn.bat -s -L
call cdn.bat
del cdn.bat /Q
goto installver

:exit
exit

:checkfile
:filesize
  set size=%~z1
  exit /b 0
