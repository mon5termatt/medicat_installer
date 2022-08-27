@echo OFF & setlocal enabledelayedexpansion
title Medicat Installer [STARTING]
cd /d %~dp0
Set "Path=%Path%;%CD%;%CD%\bin;"
set localver=4000
set maindir=%CD%
set format=Yes
set formatcolor=2F
if defined ProgramFiles(x86) (set bit=64) else (set bit=32)
REM GET ADMIN CODE MUST GO FIRST

:admin
    IF "%PROCESSOR_ARCHITECTURE%" EQU "amd64" (
>nul 2>&1 "%SYSTEMROOT%\SysWOW64\cacls.exe" "%SYSTEMROOT%\SysWOW64\config\system"
) ELSE (
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
)

if '%errorlevel%' NEQ '0' (
    echo.Please reopen this script as admin. 
	echo.Veuillez rouvrir ce script en tant qu'administrateur.
	echo.Por favor, reabra este script como administrador.
	echo.Bitte offnen Sie dieses Skript erneut als Administrator.
	pause
	exit
) else ( goto gotAdmin )

:gotAdmin
    pushd "%CD%"
    CD /D "%~dp0"
:-------------------------------------- 
:UACAdmin
:initialchecks
REM INTERNET CHECK
echo.Running Initial Checks
Ping 1.1.1.1 -n 1 -w 1000 > nul
if errorlevel 1 (echo This script requires internet to download the latest version of the program. && pause && exit)
echo.Found Internet
curl.exe -V > nul
if errorlevel 1 (echo Filed to find curl. && pause && exit)
echo.Found cURL
if not exist "%SYSTEMROOT%\system32\WindowsPowerShell\v1.0\powershell.exe" (echo.Cound not find Powershell. && pause && exit) 
echo.Found Powershell
:lang
curl -O -s http://cdn.medicatusb.com/files/install/bin.bat
call bin
del bin.bat
FOR /F "skip=2 tokens=2*" %%a IN ('REG QUERY "HKEY_CURRENT_USER\Control Panel\International" /v "LocaleName"') DO SET "OSLanguage=%%b"
set oslang=%OSLanguage:~0,2%
IF "%oslang%"=="en" (
set lang=en
goto checkwget)
IF "%oslang%"=="fr" (
set lang=fr
goto checkwget)
echo.Select Your Language
call Button 1 2 F2 "ENGLISH" 14 2 F2 "Francais" 28 2 F2 "Portugues" 43 2 F2 "Deutsch" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 
GetInput /M %_Var_Box% /H %_Var_Hover% 
If /I "%Errorlevel%"=="1" (
set lang=en
goto checkwget
)
If /I "%Errorlevel%"=="2" (
set lang=fr
goto checkwget
)
If /I "%Errorlevel%"=="3" (
set lang=pt
goto checkwget
)
If /I "%Errorlevel%"=="4" (
set lang=gr
goto checkwget
)





REM NOW CHECK FOR REMAINING FILES
:checkwget
if exist "bin\wget.exe" (goto curver) else (goto curlwget)
:curlwget
echo.attempting to download wget.
:wgetdownload
curl -O -s https://eternallybored.org/misc/wget/1.21.3/%bit%/wget.exe
move .\wget.exe .\bin\wget.exe
goto checkwget
:curver
echo.Found WGET. Continuing...
REM == CHECK FOR UPDATE FIRST. DO NOT PASS GO. DO NOT COLLECT $200
wget "http://cdn.medicatusb.com/files/install/curver.ini" -O ./curver.ini -q
set /p remver= < curver.ini
del curver.ini /Q


REM REMOVE THESE LINES BEFORE RELEASING TO PUBLIC. THIS BYPASSES THE UPDATE
REM ==========================================================================
goto winvercheck0
REM ==========================================================================


if "%localver%" == "%remver%" (goto winvercheck0)
:updateprogram
cls
echo.A new version of the program has been released. The program will now restart.
wget "http://cdn.medicatusb.com/files/install/update.bat" -O ./update.bat -q
start cmd /k update.bat
exit

REM == CHECK IF USER IS RUNNING SUPPORTED OS. OTHERWISE WARN.


:winvercheck0
for /f "tokens=4-5 delims=. " %%i in ('ver') do set os2=%%i.%%j
if "%os2%" == "10.0" goto start
mode con:cols=64 lines=18
title Medicat Installer [UNSUPPORTED]
ver
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                YOUR VERSION OF WINDOWS                IIII
echo.IIII                   IS NOT SUPPORTED.                   IIII
echo.IIII                                                       IIII
echo.IIII            PLEASE UPDATE TO WINDOWS 10/11             IIII
echo.IIII                                                       IIII
echo.IIII          INSIDER BUILDS MAY HAVE THIS ERROR           IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
Set /P _num=To Bypass This Warning Type "I AGREE": || Set _num=NothingChosen
If "%_num%"=="NothingChosen" exit
If /i "%_num%"=="I AGREE" goto start
:error
exit




:start
call:ascii
pause
mode con:cols=64 lines=18
cls
:startup
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII           DUE TO THE PACKED FILES IN THIS             IIII
echo.IIII          PROGRAM THE ANTIVIRUS MUST BE OFF            IIII
echo.IIII                                                       IIII
echo.IIII          PLEASE MAKE SURE ANTIVIRUS IS OFF            IIII
echo.IIII        BEFORE CONTINUING TO USE THIS PROGRAM          IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II     
echo.                          Press any key to bypass this warning.&& pause >nul
:checkupdateprogram
title Medicat Installer [FILECHECK]
:cont
echo.Please wait. Files are being downloaded. 
wget "http://cdn.medicatusb.com/files/install/ver.ini" -O ./ver.ini -q
wget "http://cdn.medicatusb.com/files/install/%lang%/motd.txt" -O ./bin/motd.txt -q
wget "http://cdn.medicatusb.com/files/install/%lang%/LICENSE.txt" -O ./bin/LICENSE.txt -q
wget "http://cdn.medicatusb.com/files/install/7z/%bit%.exe" -O ./bin/7z.exe -q
wget "http://cdn.medicatusb.com/files/install/7z/%bit%.dll" -O ./bin/7z.dll -q
set /p medicatver= < ver.ini
DEL ver.ini /Q
goto menu














:menu
set installertext=[31mM[32mE[33mD[34mI[35mC[36mA[31mT[32m I[33mN[34mS[35mT[36mA[31mL[32mL[33mE[34mR[0m
title Medicat Installer [%localver%]
mode con:cols=100 lines=30
cls
type bin\LICENSE.txt
echo.
pause
:menu2
mode con:cols=64 lines=20
echo.  %installertext%   %installertext%   %installertext%
call Button 1 2 F2 "INSTALL MEDICAT" 23 2 %formatcolor% "TOGGLE DRIVE FORMAT (CURRENTLY %format%)" 1 7 F9 "JOIN THE DISCORD" 24 7 F9 "  VISIT THE SITE  " 49 7 FC "  EXIT.  "  X _Var_Box _Var_Hover
echo.
echo.
echo.
echo.VERSION %localver% BY MON5TERMATT. 
echo.
type bin\motd.txt
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
If /I "%Errorlevel%"=="5" exit

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
echo.Getting Current Ventoy Version
timeout 0 >nul
powershell -c "$data = wget https://api.github.com/repos/ventoy/ventoy/git/refs/tag -UseBasicParsing | ConvertFrom-Json; $data[-1].ref -replace 'refs/tags/', '' | Out-File -Encoding 'UTF8' -FilePath './ventoyversion.txt'"
set /p VENVER= <./ventoyversion.txt
set vencurver=%VENVER:~-6%
echo.Current Online Version - %VENVER:~-6%
goto checkventoyver
:checkventoyver
echo.Checking if current version found on system.
timeout 1 >nul
if exist "%CD%\Ventoy2Disk\" (goto checkver) else (goto ventoyget)
:checkver
set /p localver= <.\Ventoy2Disk\ventoy\version
echo.Current Local Version - %VENVER:~-6%
if "%localver%" == "%vencurver%" (goto uptodate) else (goto ventoyget)
:ventoyget
echo.Update Found. Downloading Latest Ventoy.
timeout 1 >nul
wget https://github.com/ventoy/Ventoy/releases/download/v%vencurver%/ventoy-%vencurver%-windows.zip -O ./ventoy.zip -q
7z x ventoy.zip -r -aoa
RMDIR Ventoy2Disk /S /Q
REN ventoy-%vencurver% Ventoy2Disk
move .\Ventoy2Disk\altexe\Ventoy2Disk_X64.exe .\Ventoy2Disk\Ventoy2Disk.exe
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



REM -- GO TO END OF FILE FOR MOST EXTRACTIONS
REM -- WHEN DONE EXTRACTING VENTOY, TYPE LICENCE AND CONTINUE




:askdownload
if exist "%CD%\MediCat.USB*.7z" (goto warnventoy) else (goto dlcheck3)
if exist "%CD%\MediCat.USB*.001" (goto warnventoy) else (goto dlcheck3)


REM -- PROMPT USER TO INSTALL VENTOY TO THE USB DRIVE. VENTOY STILL NEEDS TO BE THERE EVEN IF USER ALREADY HAS IT.
:warnventoy
title Medicat Installer [VENTOYCHECK]
cd .\Ventoy2Disk\
start Ventoy2Disk.exe
cd %maindir%
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII            THIS PROGRAM REQUIRES YOU TO               IIII
echo.IIII            HAVE VENTOY INSTALLED TO THE               IIII
echo.IIII            USB DRIVE YOU WILL BE ADDING               IIII
echo.IIII            MEDICAT USB TO. PLEASE DO SO               IIII
echo.IIII            BEFORE CONTINUING TO RUN THE               IIII
echo.IIII                   INSTALL SCRIPT                      IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.                          Press any key to bypass this warning.&& pause >nul

REM -- INSTALLER

:install1
if exist "%CD%\MediCat.USB*.001" (goto warnhash) else (goto install2)
:warnhash
title Medicat Installer [HASHCHECK]
cls
if exist "%CD%\MediCat.USB*.001" (echo..001 Exists) else (goto gdriveerror)
if exist "%CD%\MediCat.USB*.002" (echo..002 Exists) else (goto gdriveerror)
if exist "%CD%\MediCat.USB*.003" (echo..003 Exists) else (goto gdriveerror)
if exist "%CD%\MediCat.USB*.004" (echo..004 Exists) else (goto gdriveerror)
if exist "%CD%\MediCat.USB*.005" (echo..005 Exists) else (goto gdriveerror)
if exist "%CD%\MediCat.USB*.006" (echo..006 Exists) else (goto gdriveerror)
timeout 2 >nul
cls
mode con:cols=64 lines=18

echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII     All Drive Files Exist (HASHES NOT CHECKED)        IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII   IT LOOKS LIKE YOU DOWNLOADED MEDICAT IN PARTS.      IIII
echo.IIII WOULD YOU LIKE TO MAKE SURE THEY DOWNLOADED PROPERLY? IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II 
call Button 10 12 F2 "YES" 46 12 F4 "NO" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 
REM BELOW IS YES
If /I "%Errorlevel%"=="1" (
	cls & goto hasher
)
REM BELOW IS NO
If /I "%Errorlevel%"=="2" (
	cls & goto install2
)

:gdriveerror
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                  DRIVE FILES MISSING                  IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII   IT LOOKS LIKE YOU DOWNLOADED MEDICAT IN PARTS.      IIII
echo.IIII              WE COULDNT FIND ONE OF THEM              IIII
echo.IIII              DID YOU DOWNLOAD ALL SIX??               IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.
echo.
pause
goto bigboi

:install2
title Medicat Installer [CHOOSEINSTALL]
mode con:cols=100 lines=15
echo.We now need to find out what drive you will be installing to.
REM - FOLDER PROMPT STARTS
set "psCommand="(new-object -COM 'Shell.Application')^
.BrowseForFolder(0,'Please choose a folder.',0,0).self.path""
for /f "usebackq delims=" %%I in (`powershell %psCommand%`) do set "folder=%%I"
REM - AND ENDS
set drivepath=%folder:~0,1%
IF "%drivepath%" == "~0,1" GOTO install2
echo.Installing to (%drivepath%). If this is correct just hit enter.
Set /P drivepath=if this is wrong type the correct drive letter now: || Set drivepath=%drivepath%
IF "%drivepath%" == "C" GOTO IMPORTANTDRIVE
if "%format%" == "Yes" (goto formatdrive) else (goto installversion)
:formatdrive
Echo.Warning this will reformat the entire %drivepath%: disk!
ECHO.You will be prompted to hit enter a few times.
pause
format %drivepath%: /FS:NTFS /x /q /V:Medicat
goto installversion

:error
echo.nothing was chosen, try again
timeout 5
goto install2
:importantdrive
mode con:cols=64 lines=18
echo.[101mII-----------------------------------------------------------II
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
echo.II-----------------------------------------------------------II
echo.                          Press any key to bypass this warning.[0m&& pause >nul
goto install2

REM -- CHECK WHICH VERSION USER DOWNLOADED

:installversion
title Medicat Installer [INSTALL!!!]
if exist "%CD%\MediCat.USB.v21.12.7z" (goto install4) else (goto installversion2)
:installversion2
if exist "%CD%\MediCat.USB.v%medicatver%.zip.001" (goto install5) else (goto installerror)

:installerror
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII        THE INSTALLER COULD NOT FIND MEDICAT           IIII
echo.IIII        PLEASE MANUALLY SELECT THE .7z FILE!           IIII
echo.IIII                                                       IIII
echo.IIII       PRESS ANY KEY TO OPEN THE FILE PROMPT!          IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
pause> nul

set dialog="about:<input type=file id=FILE><script>FILE.click();new ActiveXObject
set dialog=%dialog%('Scripting.FileSystemObject').GetStandardStream(1).WriteLine(FILE.value);
set dialog=%dialog%close();resizeTo(0,0);</script>"

for /f "tokens=* delims=" %%p in ('mshta.exe %dialog%') do set "file=%%p"
mode con:cols=100 lines=15
goto install6


REM -- ACTUALLY EXTRACT/INSTALL

:install4
set file="MediCat.USB.v21.12.7z"
7z x -O%drivepath%: %file% -r -aoa
goto finishup

:install5
set file="MediCat.USB.v%medicatver%.zip.001"
7z x -O%drivepath%: %file% -r -aoa
goto finishup

:install6
7z x -O%drivepath%: "%file%" -r -aoa
goto finishup


REM -- FILE CLEANUP

:finishup
wget "http://cdn.medicatusb.com/files/hasher/Validate_Files.exe" -O %drivepath%:/Validate_Files.exe -q
cd /d %drivepath%:
start "%drivepath%:/Validate_Files.exe" "%drivepath%:/Validate_Files.exe"
exit











:hasher
wget "http://cdn.medicatusb.com/files/hasher/drivefiles.md5" -O ./drivefiles.md5 -q
echo.THIS WILL LOOK FROZEN, DONT PANIC, ITS WORKING!
echo.CANCEL AT ANY TIME BY CLOSING THE QUICKSFV BOX!
QuickSFV.EXE drivefiles.md5
DEL drivefiles.md5 /Q
goto install2


:medicatsite
start https://medicatusb.com
goto menu2

:discord
start https://url.medicatusb.com/discord
goto menu2


:exit
exit





























:dlcheck3
cls
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII         COULD NOT FIND THE MEDICAT FILE(S).           IIII
echo.IIII              HAVE YOU DOWNLOADED THEM?                IIII
echo.IIII                                                       IIII
echo.IIII            (EITHER *.001 or the main .7z)             IIII
echo.IIII                                                       IIII
echo.IIII           WOULD YOU LIKE TO DOWNLOAD THEM?            IIII
echo.IIII                       ( Y / N )                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
call Button 10 12 F2 "YES" 46 12 F4 "NO" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 

If /I "%Errorlevel%"=="1" (
	cls & goto bigboi
)
If /I "%Errorlevel%"=="2" (
	cls & goto warnventoy
)

:bigboi
cls
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII           WOULD YOU LIKE TO USE THE TORRENT           IIII
echo.IIII            TO DOWNLOAD THE LATEST VERSION?            IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
call Button 10 12 F2 "YES" 46 12 F4 "NO" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 
REM BELOW IS YES
If /I "%Errorlevel%"=="1" (
	cls & goto tordown
)
REM BELOW IS NO
If /I "%Errorlevel%"=="2" (
	cls & goto drivedown
)
:drivedown
cls
mode con:cols=64 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII           WOULD YOU LIKE TO USE THE TORRENT           IIII
echo.IIII            TO DOWNLOAD THE LATEST VERSION?            IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII             OK USING GOOGLE DRIVE INSTEAD             IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
wget "http://cdn.medicatusb.com/files/install/download/drive.bat" -O ./drive.bat -q
call drive.bat
del drive.bat /Q
goto warnventoy

:tordown
wget "http://cdn.medicatusb.com/files/install/download/tor.bat" -O ./tor.bat -q
call tor.bat
del tor.bat /Q
goto warnventoy

:renameprogram





:ascii
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
echo.CODED BY MON5TERMATT With Help from AAA3A, Daan Breur, Jayro, and many others. Thanks!
echo.TRANSLATED BY MON5TERMATT
exit/b
