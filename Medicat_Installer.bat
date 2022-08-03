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
	echo.Bitte öffnen Sie dieses Skript erneut als Administrator.
	pause
	exit
) else ( goto gotAdmin )

:gotAdmin
    pushd "%CD%"
    CD /D "%~dp0"
:-------------------------------------- 
:UACAdmin


REM CHECK FOR POWERSHELL ON SYSTEM
:pwrshl
if exist "%SYSTEMROOT%\system32\WindowsPowerShell\v1.0\powershell.exe" (goto lang) else (goto pwrshlerr)
:pwrshlerr
mode con:cols=64 lines=18
title Medicat Installer [ERROR]
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                 THIS PROGRAM REQUIRES                 IIII
echo.IIII              POWERSHELL TO BE INSTALLED.              IIII
echo.IIII                                                       IIII
echo.IIII         PLEASE INSTALL POWERSHELL ON YOUR OS          IIII
echo.IIII                 AND TRY AGAIN. THANKS.                IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.If you believe it IS installed and want to bypass this warning,
Set /P _num=type "OK": || Set _num=NothingChosen
If "%_num%"=="NothingChosen" exit
If /i "%_num%"=="ok" goto lang

REM IF POWERSHELL CHECK IS GOOD THEN PROMPT FOR LANGUAGE
:lang
call :binfolder
echo.Select Your Language
call Button 1 2 F2 "ENGLISH" 14 2 F2 "Francais" 28 2 F2 "Portugues" 43 2 F2 "Deutsch" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 
GetInput /M %_Var_Box% /H %_Var_Hover% 
If /I "%Errorlevel%"=="1" (set lang=en && goto checkwget)
If /I "%Errorlevel%"=="2" (set lang=fr && goto checkwget)
If /I "%Errorlevel%"=="3" (set lang=pt && goto checkwget)
If /I "%Errorlevel%"=="4" (set lang=gr && goto checkwget)

REM NOW CHECK FOR REMAINING FILES
:checkwget
echo.
echo.
if exist "bin\wget.exe" (goto curver) else (goto curlwget)
:curlwget
echo.attempting to download wget using curl.
echo.This requires windows 10 version 1703 or higher.
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


REMOVE THESE LINES BEFORE RELEASING TO PUBLIC. THIS BYPASSES THE UPDATE
===========================================================================
===========================================================================
===========================================================================
goto winvercheck0
===========================================================================
===========================================================================
===========================================================================


if "%localver%" == "%remver%" (goto winvercheck0) else (goto updateprogram)
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
set installertext=[31mM[32mE[33mD[34mI[35mC[36mA[31mT[32m I[33mN[34mS[35mT[36mA[31mL[32mL[33mE[34mR[0m
reg add HKEY_CURRENT_USER\Software\Medicat\Installer /v version /t  REG_SZ /d  %localver% /f
if exist "%CD%\MEDICAT_NEW.EXE" (goto renameprogram) else (call:ascii)
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
wget "http://cdn.medicatusb.com/files/install/%lang%/motd.txt" -O ./bin/motd.txt -q
wget "http://cdn.medicatusb.com/files/install/ver.ini" -O ./ver.ini -q
wget "http://cdn.medicatusb.com/files/install/%lang%/LICENSE.txt" -O ./bin/LICENSE.txt -q
set /p medicatver= < ver.ini
DEL ver.ini /Q
REM -- EXTRACT THE 7Z FILES BECAUSE THAT SHIT IS IMPORTANT
:7z
wget "http://cdn.medicatusb.com/files/install/7z/%bit%.exe" -O ./bin/7z.exe -q
wget "http://cdn.medicatusb.com/files/install/7z/%bit%.dll" -O ./bin/7z.dll -q
goto menu














:menu
cls
REM -- THE MAIN MENU, THE HOLY GRAIL.
title Medicat Installer [%localver%]
mode con:cols=100 lines=30
type bin\LICENSE.txt
echo.
echo.Press any Key to Continue (x2)
pause > nul
pause > nul
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
if exist "%CD%\MediCat.USB*.7z" (goto warnventoy) else (goto dlcheck2)
:dlcheck2

if exist "%CD%\MediCat.USB*.001" (goto warnventoy) else (goto dlcheck3)
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
REM -- IF DOWNLOADED IN PARTS, ASK USER IF THEY WANT TO DOWNLOAD THE HASH CHECKER (FIXER.EXE)

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
echo.Installing to (%drivepath%) Close and restart the program if this is wrong!
echo.Otherwise hit any button to continue
pause > nul
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
goto autorun2



:deletefiles
cls
echo.DONE!
pause 10
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
wget "http://cdn.medicatusb.com/files/install/update.bat" -O ./update.bat -q
start cmd /k update.bat
exit





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



:binfolder
@echo off &chcp 850 >nul &pushd "%~dp0"
@set "0=%~f0" &powershell -nop -c $f=[IO.File]::ReadAllText($env:0)-split':bat2file\:.*';iex($f[1]); X(1) &cls &exit/b

:bat2file: Compressed2TXT v6.5
$k='.,;{-}[+](/)_|^=?O123456789ABCDeFGHyIdJKLMoN0PQRSTYUWXVZabcfghijklmnpqrstuvwxz!@#$&~E<*`%\>'; Add-Type -Ty @'
using System.IO;public class BAT91{public static void Dec(ref string[] f,int x,string fo,string key){unchecked{int n=0,c=255,q=0
,v=91,z=f[x].Length; byte[]b91=new byte[256]; while(c>0) b91[c--]=91; while(c<91) b91[key[c]]=(byte)c++; using (FileStream o=new
FileStream(fo,FileMode.Create)){for(int i=0;i!=z;i++){c=b91[f[x][i]]; if(c==91)continue; if(v==91){v=c;}else{v+=c*91;q|=v<<n;if(
(v&8191)>88){n+=13;}else{n+=14;}v=91;do{o.WriteByte((byte)q);q>>=8;n-=8;}while(n>7);}}if(v!=91)o.WriteByte((byte)(q|v<<n));} }}}
'@; cd -Lit($env:__CD__); function X([int]$x=1){[BAT91]::Dec([ref]$f,$x+1,$x,$k); expand -R $x -F:* .; del $x -force}

:bat2file:[ bin
::AVEYO...Q55v......*D........j}?.E-..;>J|..Z)..b}qGy+#?........Rn7cqOb}wR|tPF${5ZD!d$sOkWR5[]..Rn....$hU^L-d,oHTL_X5!d$7ihuSn2+..fZ
::{...$hU^L-d,oHTL_Xo&}8V2BlU=EL..a...tH....,zd68.-.2c!T]8HboU[@+8?XC@M-EI+.;>^)....7kFB[.;P/v<qYbju;hvJ_[U]C.7a_.)@r.....ifapp}wR|t
::PF%fsxRwXy==9+|g/>z...tkA,..;>xC\$@;Ep+$6h>vyJqdtOVW%u}X+P4r}d;}b.yK_P\d[;uGd,;>oca7;.Gz7aM*?lUzM)Ls?{FUxYtQ;?=A^%2DNj.g8&<PJQ~*Fx
::al6W$HN$YNNE3[1K[~TkUHGQ/`D!E}7cp.k)a5A,,zoKEr@JlPbimYUg$,v]A6P.&o1Dw0JG&zWNnx=oCK=Zj!q3(!0?/5mAh#]mhIINp8j(ACQ62QE!Ka79[hS}BPM2sv
::&wRXk8Ogkj6eXnzb<uzeT=tGt5sN<WA#-3Y&XCl@ROZ6-V_t,`km_Zwo.1Y$?`_(U}b}1.6y)fNPda/`(oA3`t)5-(D%qXN?i<b3C9~xF18_;;pa=RrG)e#.%(f}6pdO;7
::AHa%Wpq\B}t@zeR]o]Zp9msnEFKb=\xhaNw*~mDh8W6-/6E8ssD}/,yuhuTf>x&1ng)ra#0oeW4]l6H@APOMYGNr_{a(L76S%cPgh!Nb>p9W=se9dHQ]L4gI~E`yJDhw8G
::>peQUH`f@0eIQv%\ym-A);?]+`cm4B4Q5?KG{p7m<j{n*%X&j4_EI<k<}kV&Y[~=l?\[|4t=[cb-WN>%O@2TN`rZOimuq\f]4y?G.%;,McbL3hDT}?XMXL~0Aa{@3AC5aq
::6]tT^m`+QvH|/uayc(hE#h}u_,Ss~3~]zEO%yW`.YtXq8]7?-2yCbU\>bi6LZb0TFC=oI<#?Oc)}a0nx}PA\MCaDAa2A7ra6[}enhS//5pXA\y(i!(TFCr-)X]L]LNURF}
::6`Uhfr[vCgABT4(6QD5Ez.o~Ko(JpXva(,tWoF166N26@ukH#M}I}6v5iYPyH,aFO931*p[d~JfV@XH~>hlhc\63z4,Qv\~xrG%IQ32ogE$GZzo~)1u+H,IN/8z(pW#=U0
::sKHi6%|!_^xiH(^KqfDyQ}7JLTv}UPT9lakP!kKxM[{RXpk*zNYZXVoj=,W=l_PEowFr`1+Ef]F?5X2fjZ6nr7LR.H8rH(wToqLX#G`5i,&HZ|.]sk>PbJ8d[=?$AZa0]u
::&%)r`GnuK{;#QZVz_wJ;GsTeX]!`k-617u\5S~ix/6#t07Oc.^v|{v<tW]alvl)!{<I.VS)4pH92?-$X3k@{VBW6d7Jz[g[\yJTdG+5Y_nZuP)LB)g$X69~[z=q$9?wja{
::`VA|>_aD4hh@u63wl)s61=tB2*z^q\M@s}W+]va|;=mMdA\[(={DxzMd4mhnuKwk##c%,,@F[D/no+<Zeqv}S%vxLvwV1phAR!CyXvA&=z>J=%7U{Lp&wwa9bY]=g==-ZX
::O>`4mJ>k_Ce7Xy$<`Lks3=?AiS]~=p,Ih{)tH&BCxC54EwB2MbM_7dlJUO{JT(R/o`5l\ChMzHUG`FwTd]7b>gW23#k@hcA<SDM/qc$t*}*(^EPk^2ggf5_yid}M&o6ajm
::i^ZAib;z`f0&N`TZWM6ZS0Pk4%kd3b<kfP;eQMt~j2a8$*4Z.mCk+T%dW!42j4OPRPAZ&1^}3Q~@eldE[&Xk`*6<;g(`L7fHmlD{,Y_mYG0VvBGDlXDCe0`TcPGwx_w?&T
::v63iORa$WtccuUj$3(T4,1@^1wB,gz=2Kp16f!iv&1d+uS.Pt12uo0\W{M2FT3tZ-npfr0`]aDf6vpv*]M-?&!)cs1>Oz7HJ}8U7l0OU_K-`0Pcu(bnkvzyvhPDj0+VbR<
::{myt<xJK(%?68Rzzonq285,#n\g~@CJ!Z@3DJ-,ft)_dM2}L!3~D+ktfOcP${#QsAv_#6r(5|RWG6H\wcNA-;zRc+4Jt~,!Q2]^`fNgYavZJAGg|6Ts<{D_sLD/0HC@zEt
::$qYMd$G[IGxkd1Cn@yDj%$6fZQij}\.LEf_a~geQDAL`;Z8$cKABcp)rUrLqT@%H^`)k`H3q[k$1>X#A5s8U{\;.{pI)P&D<X]`n6F4-WEg;fQTbJx1GlsP_=_UO51Q%q2
::|6|2tvs9O+JH2o,Mr&B)=CW}\(2aNOrna;GuyC7B/fCn)F&XOfi&n61SQ,`k~i1E0$Aa,.HV1(-K)p4N`4^hEf<tJR&l/NhFm/TC=@n41oLZCJht@f7J\Jmv1t\9tzK]c2
::K5>ffs#vKg]TT-fHj-9S=z$C#HbMdSfcWiajS|!MyLF+/ZS&>d9yT*,XEZP^a<rQ[`%-DbM!!cX<0,s\>_Af0x_zpmGdg]HEW3@;hig{Z!_0EP9wiJi~WE9SP8*lCsUb\n
::cxL-kCcR/%d{0w5foIgoJ6+n~vijd)z?@9(eWJU=)cAB0CfB$m,;[Elj)l3U5and8D%/p%d!ZJg^X)2lE!3aFioJR]lRUH@+m~T;-z?$&oV]NrRl0AY$uV_${U8&<Wg9Y0
::83f1SeK-FHOEuWo&H3|A+Bn*>])y#/4JQsJ/)@_r@{_Y&w_l.K;w*4@d]J)xVcaj_mf;X1B6t7~&)ae`U4.w.wEJ#?UN$W9<pF%YL{?kNRK!L-`(ojwSPWt(-x~{2<D,Bz
::Ta1W\Blx;#\C$H<c5#h-4zaEwmT/VzQz#c0D$X8n|MXmblCC6mGNZ3cD[FN8BOY4yoP%wq}L94V7xy}*\8F_IYzN/*Qpo%30U>)rkel11#*|GRSqGFF4B\%dGD?bG_Rx56
::9l9pN(<1~aw|6#iY(*ar3pF,%CP9^IanV_0}fW&%Vrv&0^~ad-X%CxgDwDS(c|#VCS=}X0pDx|zVDz^>g!7J)l~(wBx\QjI9bQJo&*.gYw!JhvXrun$inrB~<JeCIp2A?\
::9Uc00!i?vK7dU4cdCnd!<|N2@7Z61=(8xz?WY3lxpg_Je0EY_tH&M60LU}`@InEt6U$}WqECyW*bB<E>9IBIipee#_k]5Y!xmQf_kX1<Y-(Q/Rsa&RnDR\Y>sV@1fM$;J`
::$wg~aXt&m33m@p]-kE/`%qW`TVNJSz=FV!CT`z$3UOHZ$e5T/ysEB>FM(@zJ[zvB\74~f09AznrXP{Ol]0E=hF7,UJlyr\5zjV+RWaAr&#-/mAG#3G*(ydr=(bi(,Vuf[%
::Ag~!Jf>!&RT&YLd;FVeB!<g!11Iq(I^h/9Kd$}O[)L)EQE@cf$@$ZKU?XeuObYcsxhO~*yUAl9ec5g~M>#UHo82,47b3HDL4(c}d*TY]<$93RSVqi{HRzItr&<2B9+-n,m
::c[Ihv`MFzh!%{Aq&.&{)MC[4jo~uos+|w04MY8G+W=;S>^Rt.VR&c->u%VzVh~4J4nSBsar4TQxc!-Yh[<V(JcQCIge*j1Uz<\?\}A;0{M57R\/69(.6anChIl*0RTOwh#
::$0N,_YT8A1b$-`KMtr*rO#)nVkvo{09AdRBD8ew|6b@U\=jGhSL<QV9EU/9Y)]2K#\&qXoa%!dF8X9LV(HxtmUBoA$pJ$#NwR(5Gp$+=~^sb)P{;aQhw!qd0N92[d,u^hc
::,oQ3U5)T{x}Gq?6s5cqlfaZ7+cAumXwVvT?k=dxZ`Jp[b\y+$@H/I3Q-Wj|,EIS-\A#8}9*_!7j]G$Khb=)A\mbw!HHrponl*_lE?pz~A*UU,\cth5g-4&mpA2i~xvx93V
::&Vumc)sXqT<pElV@y-X~wU>&puVr?qu%`huPq80KSz{m[HicbTNOK|T_?;5j4\Lo}raQsln*{X$&S=rQ,D%cOeLc9BbY77Imm}v&-4[AU+LnlUaCG[(|DWb3*i^#Dfoqo0
::f8jvp0/CP+Z(NDrWgi)D$][(ccyD=>u;Dd^96mOf6Ve[vSJr}Y`sRfUx-<B|nj96cBDsEM`*n!`ZEjsx/{Fzk2;1oLSjv,8dC>&s|DCL,542R}itW1gJk$yPgxg0!oJ+4@
::,l!fG2eziR(GhLA<l%5kBy\,j,89IU<=__[2}BL}2,0Xs*T#s\L@mgbb)Vb0!v=s,y9N@^ryqM}5Z|Vi\-!va#*|1f39z7Bq3/u(u+zSyIwUr<S!K2!%D54D5ZE%DWa,A(
::YbU(,rbs0J_Jh|*weJY02r{,ci[65i^wDNt%.R4`t9LkPPHM#1QEHNMc;s``CP^N5f45EK1#gtKJF!T</nPklI_d34zVIS+tXsy?XjkZ%T)n5[-i-)1BsnP!)T,YNYt%=)
::J<do^z65!|#srxH\[*N0.=iy#iI4uo_n4q#3zAzH^v>4G+iK5Q6RMM]4?CaDJ*><0{[xU7K}d]`EFD(zJnID{xvMs<*,?nMTB|dQ<&DqKaAsMRLrrH9%%X,E5_LGuIis0r
::GY]\hs|m6qLCk<.*JvZ29%Yu)xNnTf}QToucOPwWZ1Rfj0M4,Eog\o?O+XH{[-sF!n0N]IG%;bfVC!~qI^_@^2C@]J5`G%(!7c.[-G#e`6FGe6`W<V+1[N.p-[.$;~O_do
::^(GHzgK?!U^NtdcGJkGL~@dEhR;;Qh/RO~ISkb9d5*X[x[`pk\z1`EYir`itKBHulHN,@CJA-LAUWqvW~5$mFT3<|VhRNMfaph.fY7CJ,(!O>2bJ]Eew#l;1hm]&vo#hR*
::3W~Mi5QrMBYLrRWT|Es^#|(@QO4YOg2k}7$@;^ip]+f=)uP|/7+}1)82%k>R0]~0L<i>A5,GRqi|mPX*S1n5{l.44GEc^O{#sVrBA5/Ipk^icMW|t&+0UJfq$]GWyv}f2=
::=RZ+*m5,wuXB8O;uSWKh`cKFX}4nS4qj/K(wjPmfyCL+{gfD5rZ,F]znum-}A4$}9Da\H`=5q)+!-i@CnY%(M%BAkqj%CdvO+?wzGvHJqe)Xi?UV.BaG}<HgE6JfqLZ^-I
::VZ[u+WMK8S*.|O\]tk>k7*u%.#=-T6U$7~!\\9J{!20|n\E(VA<pi6bitTPiJ.e~]w;286qo`g5dE@/4jV$z_DrL`r&%OxX_&@p0y}A~[fZK.\h<&,g~_7)EQ6i+O*An=_
::rh!ChM{RuK3GD5-ul{h6a-{$vzQ{.*Y~YYt0T;pw@V5p!AE(LpOqj)bkq\hapeLm4YtYq`<~8u~i?ZwPQz7YQ4nPKH4`510wb%D/}!(gqiv=h~fI4G4Q*ZC8A/wK1+v^jI
::bvP;34z)\BGv!F-2a3ficNZa1YLzdWj$Q!/wycGv}q.z<ZPdOJ<NP}p$+ahn}SBz|83gb6o-C[>\{}f5*(k<#oi4M\_WX<*/4Ccte*0I?!L4S9Lj^$pvIAlU+`&%^QrB>#
::$O`#82dc,&>B)Vj*,HhF2K-`M15<WgLG;wTyB$(BVXtVCq~8\b0FSoC$hdYB(_rVAUC!,[*0|!iV%KQRow]3ZX<y5Zk.3R=j(MG[-fc<>%zt4r;nXXef~RQx0<5}Pf(\A-
::4<QxZfqqlz#;Kz-/FThP+#EnV^4F`)=%QJ-&.~`OLnSd/l0%Ltyr5%!Tv.F6L;_n\SzGr}%R%<3}_5$wXP<og<UZ)AN0qw.<pELr_KK\Ep3MsH$ZKiiu@0RrPWEKxQC4W%
::\0O5T&|!BK!m.OW~R&T\%,>%FpWh#7p3P[I^D2=U4L56p)N$T@QT,5?Be9xiVAHQ`NYYaPm|U|>J^?]66flqSC}BW69;|6iZ8{cR>`?\w3/8R!p~txGXJZ[01UF*MtN0.a
::#UI0kAsTPsQNE*2W[_lA/@seUkq*x/$*MT]||{H5[B4-.+`7H4BX2r*7,t*j)Q*FocDIC`2jNUEu1s*9ti{DvFMx05v9c-mDiUCBt}ElbxghUoWCCHw0jl{/#}niQ$di[9
::D7^L.9%XK\odf?nn3LfPcn@^)Gv\fR<AOTU$#v[0jXxjF8R}4g0S/a>stWg1GYiq7~cOLpRW6rY~NQ+h+Vc8|\|]$q<8tq6S`Ed}[v<dVAM@XfN5J\O`c\Nu)$dohPw=+A
::To`fjE~C8sTlU8L0`PMnm\isYM)2Vs5A`[];tB!N\HN^)N71e<}]9Us4[%?9g`.KI+jTfV&tXM&UzW-a/(46kz_{%X^&po`tk`Lq/Kh~Oy.h9\KKohg*ar-Lp|[-VIJMK>
::7z7+My8YO]`T1K!n~%HyWY*&F%vL<CbTK5B,4k5R#%wOKa*`LDh^\nLGQwg%6k)E#/p&;rdgXxnFAr]1Mu*qSxSdV6Q+$znF7uC2P{Z%N|X\q%}WL<n`MmJtzrG`ZGLffG
::]l3J(7&S0y]u&BSkeMrLrHi\SUVA0\Yhc@Ch94Cp\1Bo#Bp|i4R)!aI`NnRa}sWm}2}FA0|[HtdR!wt[#Et;qV}fqLY99kKXKxN-cNN.9)F]-K*ek8?/,]Jf#`tb1^z]xi
::ZCxq5S&^xz>0<%|o0M*9\..<QjRH,BrHBC(_K!P}Uo@TS6uJY*WCq)DVZCR19{n1y@\c%ClAy3_lKnk|Iu#lfzsa^nza/sIlotN*apm5o^rU+O5PVAg+f@WG0(-^jr9=&6
::?#tv>Yg@d2{dZ5zRtpN8{wUj%C/,/Cf^{~Vj;ha,C?Ca\Cc?Fb{9m*]W/eyodSr8H7wd@#d*qWgk65C655Z[@8BZv{7+D(xhpI\qtIDBS#Qph,uVXZ.c~i1kmY]eh9FZ*9
::=wEBAymb8]/\Q@M=?w-wYkO3D${pV,z\dH^16KI<#sG,<NEVz#aDY.%wyiLGF,PF%t1CT,fc37K7h+PNo6/O7sHUX1H?TY~;^GcW|vf|ZNYswDk0%raeNa3z?=Ow*/+bwX
::1[j}CG0X$H|CZ|SmHdJppSkE;x#+[-;sUN%TeKd;2<6no03`DF7z}hHrZ<x`hAsL`O3%Cq=V|Ly=-lfHaM~/u9U.2~q#p8W9(5*-b-ty_zNF|QR0&Ui[6-,%02W_~y.+R;
::#3JYV@#;]PA=iRt#zV|XMeXH5^48Z2o%yA#6`<C){1m7KjVj*mD^%0rw(mq@M2d?o`-[)@r#EaoaYAZuH6<fK6<enX`bmR.!/!yb*eSmD;.f{ZR;hHd([f{JEgcfSfY|UC
::>M=CsSPK*HG$<]Z*<zT\YVHP,?r\i*g.R@7eUQ`p0eT<j\5Msq)!7d1&-zR\I$MHoh!/c)+=IK\lA2qE<kOH3b,oqdl^%pryC9Q.J0~!WE1ew2X}&Z(5Qa@6TDrlGB^HUW
::aw?t0\u_bXH!zzl/6v?p`n>f#W?1({\M9b?Dl[nkukS)K!~KtvT4gK=*>qqhm$\OteYYn<hQlys1QlXom77wq0f9-OT7hXC]uhUmow1@I31\~RG_o!(&M}.Tc_/f>}xj,E
::a9DFu(irt@/I2Gj%WB3gAc`DKr{]|P5W&83[||1L4+NLu%#XTzo1Ub|Xljx3~<ylg.a0QbFt#p<&ijdesOTF]zM]RwwAN;C\S!XT}7(=H^{u@9PM?Wk~BxTjZBOHK4{?]d
::JtlG$o$%9d_%@H7N!_|;_rJNC&2VgV@~WH^o,eJ#uHKLEk>cTAC`.$=t]&2hL>ojAd6k|lV<Xfa@6I)|-TaF,iv^?|O3&]T67`<rUDUp?r#Vhb%mj<;K,#~$bjBX/Ok7)M
::o11uE>!]eND1ga3{9(`{$p&%W2!&hN).e.U>lNMXpsHy},S-j|v`SPV<P*d~=%oz(&g6E_=Yg5[+0J{`F5^z}vI0]YMw_lY,1G/fHC.JLU_a73k$MyQx=KrZ~[qk+H?fnj
::gQbP7[0s16.<nMiq6;iS?l.\`qeBjXV8N_?D%Ws|A9&\73n!R\%!M@3l3EsRPnZCO\;Cz*LE)/|d/qJtzo}b4*[Um^vSKC|b#Ck|iickcoXNUy/l38]$!E0j{LXvmeUR}w
::|r0$\%w\k?@@C@QlYQMl-jY.Sf@0$)C=?!$n,FNk1RS<LN;f/9s\BdKK*Zl|&q,c4&HU^t!r5#2A_Z\ZL_{Sf4kpXN&Tv}1mzHv,#pFG=Sa41ki|(UtvLcEvjHF]C^oZE#
::Mj_b^Q>*M1}G_qO5RdHWpfJYhtkqt)zCL4`HZ]C*qUk.&7xue%9OF\/[Z5YW0nu]HKr{mvBlq.\%Waaov+5!zJWeW(%r+kX-s/VXUNzicI6%^b*eUQm9>Ak1bEBOW9]|l}
::,FRS=kR[W-?ZD6gc^4beqCcb_R]LNDiL|y\m3p4v`luF]Md78ej6~MB7WDw-ot>Z!f/k&oyGRdOJCBG}c/;y0M[r!5+l|oZt?\Xg>%/wH$;I[a1<66&H$7GgIQSH}dDTZ2
::J5GDUgvnUc>%4}u29%;Tyo26OAB$K%V<VVDC9@BS/I5$g~H2HRzH`?9rM~[ZFJ33MVdgD,ot~-6#5H#C.E]}b$9<qg])vAj2$\9LwSWhb+)yHg4^PLZR*mycoyvfx_s[qT
::S4/$ZV_2cgrbf*EVQ1CNV<>IZUbgSwZd\%7navhwbY.@HyIC~Ft|tncX*oDs<V1Bg~45>/FokbLnE=`C6CwB)3ulMH6af3.k9Ku!KB`t.bus\!2Umqn53}>?HS]V/8)2IF
::(I170(f<xh`8>!s+h9~+n/(M~n.)2VX4urtu|V(JL\\awmvgU!?q@8JhF567]*T-cbtm\r!{eU%5oHh&>y2!muW(`C}NSh*9+gX>MW-]&0ldZG>-@lTqyYSi>k_`?P3<Ql
::8Xq?3/j7>w^m1j[`kM$[GE\i$7*%\=dOXGcJKwJ&F^w_~t\@Io95bbxZrLCPOg],HRL,s&ajEB(@98H`}{GBE5^_TcOfb48jcWA[xQX);(7<<ogK/t(vs/ju2&pW5[M*;X
::^gEgI;V!9@$rDn^LaB95^n]h^E39P5BLi)vT0?i2Sd(JSh<I7N}3~~QbcT?if2.RSh)MqX|NyE0\,`jVb\,-*!#Tu`_ra=Uda(dn)Q|eO(4{g-HtD\-T}Th~yQ>)Vc[oF!
::?~<mE+WfoLa!qw/E}UOK;z@CL2=EU%&ekb8njNFzY&ko6L6U8DCCUg`U1Gr}t.>@)&Gyl9]n,^(ioh1dkyu1k(LF@Gob@0kVz&c?TDK@lrGywj,~F@a<_gPnBbheq8Dior
::iW!5FT_Dx_zLEsG8nDB^b0/0L<X%.GA_>eMRdZYW*ia`&gV|~T]S)f4+TLru2Z{PRY(5bB;}>}`k#{]b-e<+idtRy?{fd@?H0]q+)Yl}#bk2U`wU~v5{=bL8ssW9=L=wE)
::MoRJ>6*eTVW=Rz<;csCsWFmED6)E5UEGl&V(NhfJ>G8^e*&]llI$_W.Lg3l#^}bw*8|0zPl1V4Lg`s1uQt$mtWDuwE2\mp~d9w%05P14QK%3.=([VueG_%U4Z)e(-qVoRu
::}XZ+o_7c,CeuTS/pJIxXb9{ao?CBA^@lb4JLz;W6=W06^_wf/~zf;j&I0sNG8<Fp-cB8EzC]w+gQvU>TCuh)*t*7`5FZCjxzz_pB>m]&T-]`A8~nK#otr+)b.s\hf(`<?)
::fJ^^AOI_,Yb9uC|D{9!<h?-yMl.Vt7N}b4fDRMjO}iVleC^)Y93#aAoi5Lgb~1F)mR(P3Zpi%J_&iLS&7;XEh,E.kRlm&mOG}P~5X``}&UN|kYuVBowcmUp!}7BM5JFTur
::7gKh-A}[LC$[$^5ziADP$tMx!B~H6h`F3\7pl*Jx4MV0d]t#]a+f<D-z5<-?-|GwYbz(~5EN9IT)<sJcj&b?x@O}a8gyans8jH-4(!1P4Whj?f>%L-!L#mkW{qVGqVap+R
::u_sgQF6=qwV3pcZ#|]*~LB88,sV+DiU@%G6<r8H6q&Qv&Lhq|8}h3/x<wa&VUzHuDPY}Y52TKJ.JUHfC%+.&yrwGZQ!+Ff$/gvWEAMrytGO({DO/do/JOE+Y~+q;I>FT|;
::2@%Nyl=dl;r1YX+2j&XlgJGX2+/C,O}J`x5cV0<G-0-=1,mf^1iijVaSH`GR||tAHFS/,OU}9UKfNb##kVV2Gxd~E?}S79K%3vp-Fu;LixCwkp9E06$f.wtv5IZqTyXSqh
::FNOSfavh.C$b6>uB0@$G>q9KTx965)XphRD%3aa\=.q~S+lf%0WA9Q6KoF,3_0uOwXoJ-)>EbMH/8Isx3wFsP8TyC+*64%JBaaDZV&n`L9d^9NCms;>HagoArH@OhBVA_F
::BqbE}<A3UdfZr=1c!BtyeY,/k=60U->3s^;@e+]7P&ZfitTvj#}/%<.33eJc[Vy,mQ+16!iN~;~AF+\k.4GXF[v,g[%kPmC6[9\<//T`bk?wPX/^VMmh&(~o5I<v)LJt]J
::Y]tE?g&CWN8YLk#_8Y]4k!AQdRqF-+yG7w&EYA,CpWg4>y!s,?3SfcwS%>lkP*;pmgvl]luCnh/%%dHm<9vxS!3=ZnjP>u$$d=(qGEaG(JEAIK7+n\oAz^*m,w_K($_pi=
::Tlwm>~dmw(iKgsal+sC8Pp(S^]B_<[xqf[uCXg#{u2/0bT.6r$[;od9Zi+hps=#jfHF!OW/7=E1w3vgXG#XyVRJq}d1(qXv9u9t*jkY3;PcT[N-@jf}}&7X7%p,?ur3_TS
::e?awS{&`]u98M=N~xY}l$gkbH;wGd7?t{F~fS4<Ba9D1P,H5PQPEK>Hmd|#BL(|wa?hz^_/lT,+\=g@ifL+6{ekp7cDZ<m[@]wGJcdO^1cVPf\#]waQb)4@Bf<N*26nftA
::L2uAUF3zR6!WW^+cV]d3.M,,<Msz2SE@+?Fch?$wjM>/kzu1hX(Pt#R=I4(SD\^X!y&Ms0!0t-kf3,YwU|3g};GB(L2Z<|5K,{?yMS1/;DSjxRSb+?G`~jYDoaF7,crp.[
::,LU[+,@S}|1%fUwYL&WU[+Jub%DLEmR~kXESAt(Uuh}$}\W>e_4V&sEp<RizDj8=?*]yF&S8vTZAUoQsUCdil)cL#O`VsS7ese;5*DTK]VI?iXz;{6v_Q>}m8-nrp-V04(
::6G{ERWiA0|o-LIFlV_=n`Y-&.rxwU!LTBo{KyOH0IWMgkY[2ZOBlH?cfPE<4#Mwdj(D2O^YztN_YHLCyEU?f^!{,!*|h{AvK%Yn|&[9wyG*2HP(0%FUk8kQRt*[WA(F9,K
::lvOwpQx^KHcrxPxm1ISLHkB|<b+.iH(~@@1T_-_&J<e1&^\#c-\YS+fxXS.7,Nv]W!Ize+Ae9L$,3%,>W[;_`kDqc.8,ulY#D_`t=~9M\y!_-*W-@L,CTTvMjpDul~6$##
::hH,4Xl/A}Cj0kdWWEKwO@KBE%NY8Q9r*C6Kgy?;P5a,EB-{]GYaTQO4$J6siz;Y-xB&X_%o|Gd~Vbbm4?U]axeE.w]a$s\fLo^R]MLiQ~AoB_]vW((v7h/dzOU>9DeTn,}
::R,3I&&?(^;u02D8`688`I9AOciz;iz;Mp_$OJFl-|#+{=[*(4S0=4Bfric(@%w2$todx~d^BC2X=[^raL/+!?jCVJZW=)|k80plv$Ys+BD&=xE%h+;@c4{+<FP+aNMmkIL
::-h.FWF?+P`FSgbmiKOO_G2Mzv@r_L-pn2?A=#!^I*W(Y+Yq1OuzMWyipPUprL{2G7T?wOh%l21_FETP+b$`H@JcNL<F/K<79e?tB87?zhkV6Icrp~|*qR_/ZN`PP;{4IH^
::TrP=hohIPif;IInSPP6=Ov.rHP;W4c@>hx4b5?.$<Th468)W4(l3/]e^7M_{nzl[lb&G;%#3GT.}.kAp\m}d_Lr+Xp6&{hsrC`.{y=o_4<zT3{*A5|x&D-HkUeNP<7%%Z7
::N)XN.0gZvwmu%n<AaBeU{QsJSr>8WM1{;*,lz4k_PWDEZXZj)8Qjak6Pvb01Iy>v*h|f<S^l}SWdI!DnQ}HU$M+UAQs)2UPS.-qje2fRWR%|/+S@#4M]a&lG!To>~0~AfD
::=/c$y9/ke!ZzUF@lt>N8d4N<}q~T|r|}*tpLunS*fj@b]RpS7n?^%?Ku.\BL}wx4u$-6<60CZ~1i%8NrYTWI?vG~8)(F(^;!~*l?wIzN`[u]Ntn)/KIlg<#HNYtEZv%],l
::cA=<)a<28at/Gydne9-Lx16rIiT,8,Qb`My^0NfYA@?%>}/l(EYrh`02$n$vv^Wchy_}|M3lrHP(tlIHr7Ihi0c^EbCc#gqS)Z[;,Uo*`M|8g#<y9Fv=wkxa}xH^d;/BB,
::$XHH0)zpoM2Fzp~~6liV?Kq~Q@p~3?u~2(imkeJ,TDDuXB^sZ3>1?S+5cc&ubX/U5)nIK;6UL^s6Kiv*Lv7QB,w6tEMqR@xILR!-M\PzYy5y7ivkgcWi&6;Nd*jQHjPex#
::$0aYx3?oZ(FZLLmL9[7n9$b[n]f<bje2{Rv^!uJ^GK3/X!KLgJIU-RiAcuOS1KVB;_N_MHqVYD2P/]O8jr9@8Afy7*7OH,HT{J`wEpXOsNT_eM*emf|5x_0tJ,@qR5g7k&
::7Gc3I338)[*)h3U7`o8,784Uhl_MP{?Eb{rY$l35A0Ztx4=`o!)`hQ|kYFa|Ht8[azqtv/``[A]*=~XhGbFZ([Ds5^_&x6,sz4[eEQsH?hW]uln~!H%kg]#KveIjJ.3N+)
::rpMt8\%TyCW=Lo2Cp`Ja6`bsiDyz8J<F(!K3UbT,@j^Z`?u;}odbrq=^uTH17s+<G1\d_{E_u|y2aOG|.i)T4k.l>t?g,dnQv#8adkikZe8Dc#b&>=l2)wEWsM%YNgWXvk
::\8oC,wk[iW0mv/GOEF*-MEZMZFAS~/rW^%#nYz3VRHaDJlmhc&$3;5|^!IY`9S_-@i#5zM]SweJU*BdAhO3F{jCUM7yIXOao(7]Hns{u`~>0Q(n%zx$H8,xO/SzP.T2ShL
::Hyb>co[bT%8Y\sY]<arOsMLg[1WC+q\/{B7,9rD]^pvmC9O4e#pPG.&q*H5GN\cF[r2|H#fyVC;J?`$n~e4(PIyo`[)qXZQix8Vgxb0vJDXVn5$%VM4SzkVv7BV*MdjN,&
::|_rE$k??pAB9Zf~MULwXy]Q~&f-9s&j\9GKvu@n7{<!\*yPb_$`(lD;mP)dK=(-7](]Y$UZ;JA1b+jP|W=O|;|^`k|Ovc]`v~dPbw{_P6lSCbUHolM@E/.g+]+fXc-oZa9
::8ai)}FO$qU2+sbR7vd9@0R(*0~Co}TW0cZqz?iC\iAl1_og|zFUcYQ^YR$BuB5LgP6-\(xC4Y/@@By7W6$A8S#Th~PYF9&Q;_E@y1ibCpO\mql;alEf8>/#s4p5`^oN(F)
::$CdiA6@5b@{@6jD-(T]0i=4yr80!3I{EQ_/n(CJx&,Gi6`rI#Si#sbKi>%_C@*Z)>vJ/Y#4L59%fVlWoA{cj&[4EU!Ur6=5FQ<I2+`B/R]=4nZOP`ooN}Ck|D871\J||;%
::kd`y{&Wl/=c.07RyTt~fO*^QUU@QmuAq}FDb)ZmlFFo28+-{>7P>5Hn}+l-reErc4Bamru#N5Yc;tu2T~y`#iI_Fmu<$c^s8YS*`\c@/)MSxQpQ[fY-84z4]IGd@=tgz~M
::?_0.m)x=Q6&j}jhDGl~{43^PePemupq+2N$LS>3l2/X[mG&Ih)8shpY<E)d`E;+~B*e>MOuazu><YtUg;;I\sb@|Y{Gh\(5p<Sn0,?c]V9%TxYeq#/RQK%2k1V$nbdK@S8
::)5=f6mRt}P>sUf8Sv2`P0?4Sz|Vy&K$i8~n}#cvH(kG0uW`,vj4G4k{@O$4W4U}~^/*/FR@>heqtW8~A;`(KBj3l+T[dn5!2>Fi@Ma)m&qj,~{Y9~`Xhs/f6~R=[<jU541
::=zDVME{r=Itnr9L[fTR6cuZC4v]t`4zMS5hU5bldaXg}akj~$=j(]?+2onGzkD*KyRqBwG4u^zd_e{c7tlf%Y-E]-w}A}}}=T}!6k&{Av,?+<@]quc!%{G|1qIVN]A-%Bx
::$^r#-{yPnXt2E?Vx+wBEg2E{nP[+&;b`b6o][@pQnEP+9cE4X<~i9\bikD;>K]ZD?s9)e2&uTmR18$14AB0Dlh1U|v)!f1XI[8TSTX6ShFBa/Fm5!x,XxriuKGEJiCp05r
::*j2W<d]n1~f|]*V{l,KjDoTmflW`BmASXgJG_538`S36#nN/q5v~yJV#%]7BnQKg*PylK@y4Z!r8eIs$|\$`pY-YdQRqoc&6wvif./.VI2dg1VV)/CvtBULrb-Q=s;`^vW
::lRvE\aAfI8o[UnkJQ]5cXuU#JPm|BjsLJ6O)h=^ye)=~+F9$gfnp\S\BTRsHIt%Oxtzf7NYt8Dw*2I@57?Oeh[u-#VAUT9-5)58m.E8m@Ec[*Y%7EB]@_gL=>T>N_Xu-8Z
::uv(ktb+Tzv3@ct8%*vs0mtoGZs8Bi1p(,lC=.Rt,BK/lW10T^FEW!Xi}q5FW8zbWdZ8[Lx4;0~9ncd$TrEA$I-xiYub(mX$5`!jic>G-`!@}<u5u*?JDM@UUILHeJVn;EV
::NrTv]9;nX2[0g?#Ad-4Y.G<[EqC/8Y}0}?|{7UZaP/^NP[{U0XD%M)]L-ULu~YHt=L&pa\=/e0(8,_tEZz6pL),s)g2-lxPUe@8N?<EW`;`/=T)nH7S/Rx*U%$+?0zf[+g
::oS<!%><Fl;kDU3V!b=wpkwOM1OvVRo,m6+lA8<-bydNw@6(\$s9-sK]qkR`nbo~Wr/,Rb{9#<|V*Gy0OUY2gy?s,M9*F*9yy%Y<@q8WfV[4RW=OZhF*(~=tR/,VV|%J`Al
::)$M)`5@/Rq\.B!5skYPb/[YbUSeja_(p)ceI`8uV#~Z,6]_~$Chg{wt+Vy3@7tN;q-szRG#Cjj~^vWJ}<BVRw_P#mB_rg@6s8A&DfG2tT!@?x*=U/d+5!|=n[VU*9q#6>X
::HL86PKW}lXEh]ghKp<at%u2TxjwEV|eg\F8Y-VOL1fF/mQ3B#)%?e-A8.&mGiYaVk]}`.+;nIzD8m]15ZrhTm`_W$nLC-]`VZk]C!6R~O#dA|J!3_&w~T^Pd865>m[<Pm7
::R{>5]tTYaJlg`demXgR0J%uo>pow~PkH?XfOZ61q8m%z*-+{21UP|](g~!.Y9wRwQy.K+K!(p\=[;%KTriGiuiaXf%/vljYW3rII-)d=W<84LO5IR<}CH?B)tI4`bc0B}h
::%+zfLHx<6FCDY7#7,)z2W`4i<K0W}}JaN}l8\Nw*{!VDr(UB]Hl(^py<0QVC9r9/tRkMaVRD?};i]*JTA;kE/*P6y?7vKug3!V}B,rH(D.?)KI^XbP[,oJP9LN?nQB^c|c
::h=0vI#j|<X&1o!G&bIhIiziMftqHV5W0ji}fbFA,61/FEMButHHj4`%(;zGIhtt#.zHp;PI%-Ch+3o35Z}c@V^Oi?7Kt{;IvgzKJCB,<#eGRU1W@1qzWx>F$CSiypQ!n[h
::-P/CA=G)[HGb=#Jab0uW*lOEhl[~-))D|#)K{hYQYuMjn?,[[zA\Vv\Qvf^S|@`PrjytgcE^FfwI3ct_NY3!kfoKPZ!x6}X8-xK7ug^F,=^1G)Pz[~Et5!24!;l`|_\47o
::S_QR9~/zpp%eLmYz8b)V,@T?P<Ut18?eb>$H~<9{Ac|<YwUtN&/O>1#&%/^%i&,mTn]$Ss9t9or%yCC+Uw/Bo7YaKyEnOH@K<m8_hiym,(CMas,|@t<BR4!@#|@J-/YtCk
::W%#@.REz/\fO1%3b`YQ2.}AUj+FwE)O6z)aI92Ffq+8TbU[dGr/?8mJ3h\#=\=d2@0N.nV$f6j&SulzAvq!].\R~H`{Hv8HSN(/>p__6[k&8j)R<NQz*BD7%hWrDym^jq6
::DE&[WFjP[k\O,fKsFPikUhQP<8x&>g[|B937f.ER3~TK*>(Yy0iQBQJTr9%y.PT0_1wa~)xa!HM[UVk[Kb`(>BnF,U?]8fm%q>5Q^cn<m8I)K?M&xf~D2RC~f>EQKN5!o4
::aeO<c{E4p7s(9-wIcZ%y-yI%0gfF/qyxY12nwOmU}-HXyvN|x{|1]6[S@N3en4K*H)4G5/q!1LIy^u`)plS*_R~~bfRzpJ^5Ld@91F},lU}tQ(aiu5iy<P=#)j;J3\B96N
::<%uNMbBuE0cXEoQk5p1[K-Rb$,>l8NKcG$Mt&A\[J9?7eb.1D[Z5Ss|qXw4gU8$z!(Ir6Vf6Sgk}._pxX&NUw8T?WUX=BxTT%2ZIF)tneq*bzalPZvvsaqKKa{3tp6uPd/
::i?Ji3v%UVj*`+`Am[q#9~=SkpN5{>qwP64Hk.Vo;>2[#^m6+!8=~C^fqX>7Ym$aL([jRO@w01/|_}021^`LKg@nHY(=PA.X(Y!!A*d}Y>]sum&4+!/T`z0ZApHGw9;L!*s
::\2WnF?7?97\WX%,&ePxmYajP!Y.NHH3Qf//R2V!R;^8=@#0rx<t@z;5XaIp[]QLsTwY|bitEn,|cl(`CF^\>$cVPlvk]CPR],Kr%l@$Z<kDc7`jD,{^inBWZ$=@B;sI8,F
::y<QqCn^i6E&Z3K\M/3o^&taa%(.K^VYdQ*/&xtQLK/{wA38YfE+oSY*/)s?lO(XoVoXd>x3UW4DJQ#WEi=00nKhxg-!uQTG_J@~_O~DDsqPGItx2D4)PDM>Mm,t;dtGrR-
::H,|nQ3l/a-.XaY`_LGg{i[aMiFU}S}GFLP;^Pc>9y6K0;VA6!j{p]A_gSY[9+1sEUoLI(rz1\sABTkdK8zN6nZ<41ng|E5/VbP5&}5&~#W8&|{z%`~)KbHrktF%.VwtZh4
::[!bO2Qml{n(sYGh3`/S.oPE\ndQ@ZNCFG)EYA[;H0hN,(E;!JiRW2~EpRhu`,,`zwr%6pm#B/gV%8d%+m_fbqzZbsRs,rP[Qc`WVmj&$m=t(qR-s*plNZk6U1fV&[wl7TH
::.@<t_0>cV2JLE7H_PQW[D!7$}iX\RS)m7#qr8kOT4OyOQ(v%G37/9l,^Lv$P~`_d@SS;AY~=[U0+|0.z;fUzs_(5LMHv@4ei6=?GAttuMAsi%(?a5rNO/i_?XtJzg+^juF
::\g71m>*c=>unPRJz(HeT%S.K0DUs;n@iP|~I0wTBpK=e1sSHFzl{#H57f??(WM/_syWtG7FdA=$)[bAF3!+7Ys?)P)to\sEMh#732T&x-;-r6N*7j|.,d<I?WpJcQ->B<x
::s9.pzx4hyU4VQ-XqPEN`SOC,|bF?+m0#R%N?J{C@ueERI7s+_;,OxZtte)48%wb5,rFJQz,}(,n@@]@p?1zN1WYMan}V{bmXwg$;F^^Q6V!|2=b}N8altWa-M%e]_u=^Mz
::%U|Yz1+^[H^^v)!vS4-L~5}XfM\D?hNXz2bC]#<{ZGI-x,y_})juAhXjY7*SZnP~Wxpo+29DqnXb6*,w`Ch$>J7Qjs_mMZV;jYe2;NF\TwS?-S&[.Y#!h{@}oDvq&!cj!p
::_*}I>SH=~{+=yhTlwFxvfaVCpHjM%G\RcItl1cXMI#<-Zr_}qS\vl%@6g)(W${Ha}/_t\5o8>CB#AmE~TP./-U^9*q8*q08l6vg-45OJX*pDto!svdLf)CF}kI@I~h3V(Q
::[#uqM*)p-^[x*Z^cG+UqlNVA}]gCpY`1tpqhW7gfYC2LI_/&|5K^jH4%x?^EE[m6%4TDt~,k3/\Z,?M_+gcQhZ=%GUIkFYK(Rb.Ji;\IXBs^DH7/*Fi0)cts&FFoLJ>&OC
::eU2GXb\dp@_t1>)(vDM<kAQh,;JE2DkIKm}+o^F]6YI/K=3y3c58!GfrH0N!Ms$p$qPzL+6AhSn$KGeM_8~mF1nW3uo5@;c,Avwp~ptXs`D-~1e8!*_M3rD2UYG505ESI.
::mBs3;3+(TM(uuGHjZxB^=`%7u<6@I%L)OnR7CGmTS{qn?Y=6d}`t_x;_D{}j+NN8t2miCg<gb^|S8F{9274S8@9a-sm1ef/DTpo/u{>b*}\W(uz]r$pRlOGZ;osD_w@pEe
::vYq$gpovOT|Oo@=Y(YK^cO)?z$d@`YhIRe5dByRteT4Yeyk^\P%YowkHiHgh(sZw!53Too81hL*YKI+5jb}j2U=F5]99X&h$jc,1<c]?syHb/hd8dHt^b<!FqOFl{[CMWQ
::V&^!xQqx1-q5rAUh^ugRO4s4a)~Dktob@>hg)fF\cXU^?r^cKL@up3vH_NOxxs[]7Fc2ww{aW0yRn=}I#5dY]_iQ\n*]=|dHmIfL/;5J_N[0#?yXE)Crna{<o6ju89U/]y
::4{wIDNg}(*noWaII<Nft@sm-dqkH9);ALdsO;F(i>CcuIJXwal?/AFHTBb62K8<cND]#KI3@iev|b3zCSYir~{l_V#d`FDAQ_J?Rhi1;Lif%4V17.(ThY]+J}e}8VO]@tQ
::WK)f8^B6Ugi#-Fz&vPK;AuqS<8HH=fLj1Ydx$(h{?s=QO&6KC}%fkVOwyia/n7(}&#Ri*L{$2HPP0R,buQ)TyP[aJRpO[0aYy2vO$&e.WVf#BjG{i4\H2Va3T)Bb`MYF9c
::!(ngA=yu~qB)eUy!{eyeo}f9#snZO@^vym#wl;V^lPc?M4BcVBD]WhZ9ri@UsItb*lA?WV(-qL)ViW9V;)w%z?OSf]sIl&~HZk*QP#NPF?t%2U6lSpxCs/,Sa|wU%T80t[
::cxzcuPzJg>Y;CS@rFm3W@PN;=3W@~ztQL_83}f21|h,]}XFso9KJ~k!lc7+$MT~osq1)fd8%<u~6Uu5y-M@j}z35$h,hGpUXz;,PwQOZ@=3uT*YX4atFealo17`V2S-T6|
::igbh_AP}KWkeA\7jzO%\0U8LK4{%N&|)87ZTK+t_cV=+,usLt.b9o}fi,c476).U\6_]?@74f<J]Y_$$EQUhr3KyGG+8GGHL8rTh8dn7xG}Vgx$_A${&(M.^umc4a|\+Z\
::{yD)c)i{D)0y56uFJK-c*%7_<[rG)jEM.{Ch{!zPjSMhxm0Fs,`^5Bauy;RH_G/X~L;kM-zQ?KSY)!v[tl`Zu{!m8MnU}!RMEE.)>Ds?x,%{++f6^vM(T@[TLk-9YVxT|!
::}X3TLh<bv66p2uc9};DqM6,BFWVSv}H8xx]w!GF`Mb+0cFuGUBhNmR$%to#(ycL=]t#.(qDHc2OV_Ox+KYQN%_g0zp,,!QJ![[@bO[L8UQE$h_U1EYc9z4[IdR<n{g*2~o
::WBP|8PckMP\`I]eTti*^l9Q,c{W`AL/i21>F]@HSN`k\*giSBmq1%|\+>cR;^w(wY4Pt\uD-~l(=xaF3LbAfKp#^}Fok`;}-<.hFYvc]g2cBR7mY`xSaH.4SL`M%KQ%3&Q
::Y,+TRN)Y[D*{x{9_h[7^Af#8!1;L&Z8[66KSmv)p[`/CM2G^mY9wYY-7M@htZmD9;nK9df#ZI?%9XB>_$U3/e$HOS45OBtF&?al*nq)sy.@5<b}-\%4D2mqEa7-r{H-~aj
::-Iqa\^bj+0ZqpHbR*6qfWO(J(XvShO&1VuBSWMcNq[5Ob7~HizZn!V^w]6Q~cAD[qS?bVhLAXuQ9H`/L^Vhq2#%bq5?1^I&kPUH8(0G|uSYcr(dhKIOrV[zve9i!pc^}VG
::x)F8e17wo98!^@\sb&QVo0leT,J5{s_EY0>Y5vkCAT2PL`kF-Qbo.Z>6yAiZY8bM3d8O&Lz5MT;g(;yg_-lEN<tN3jo8~a~kX/\k6L.sXfuhh1p4^63bdjOGdrMDedD*@u
::RW>syIWuAdR$!`45(?R+zq?([J4KPti\|epa&l#d)J<Fn8s`Mky@vlcaSu|gEOa~P8Ea$*S7Sftsjxd|ba7FyRXwBOc6(Ng2_?9&8*ep$lk#.#E7;6Qq2QAFRRk*%Y\t>#
::h3m%EX.0?PcLj7L8U-/}s9QJ/Ld>x`f=(d^hxLMeo\KnGLt#=-l+5O0q/),e-5+M()SIPdr\XW$iEZ6]6h*Cd_7K8~N!VA[KegE+B@E2N[RiRv[xxv[ih`({#Ixg<Ll\nU
::aG??PWC%\%m8I]+Bt&[S&)QxT54IFBOXyV7x8T<3Z{De$9$!9u_\zEk36h^@J^x,y48ye2_((Av@v8qP*^B0T$Sc<RCWc-`;md1?bz#X,!1{/=`LOw1M$xV7Rn8k.%2;W2
::NC^/v(pm1L1>#<Y[XEq^t5~5{x7aM4#0poi3|mlFg``Qx*pMH_ZP24rI@wxu#weqBWm&>\rdYvn;]?RvXHM|8HOLlN7/S[Qv>79oN#ND@=AYfb1klK*2ZqKbx~{E#k6}|_
::7-iweLn&+D.zAuc7g7oV`Vi>OYvDZ@*/84D]IK}k~,I)kC|Gi!tjzZL,F_q`I/kDX~{~KB,5;Q_Utnn$K(5jkrlh+#{U@39o3#7,CD$Scz0DEu+5;hkiHNPT6~)wv3KcA}
::O)iT<K*{ceY3BQ4Q?`e--)a2nhupmh%HRnv![Q45~vXEncA4cR9o\adF&mN$h<7e4$L]\#1RQve)07daOVmvT83Oj_`{/JA@V/mZg@,ltXy{*<3O(Ig&i]YF<\|LP6K0|@
::Yy|W1[i7uwPR(\eGH!w!7hO&?<h8s3EnL!{bd[eND!o<TY>y\za6u9i)\%,aBvYS/\*/<cU|qk$_uA<s^`}9{!PafTOoC3O3YTyn-k/2OFcb7P3iSRgZhPR\uP4OWiz\EH
::w>2j*ukeFinKtxwX{4yu_&~4V9$qN^U{Ag@Yfq5]L7*E[<0$@C<|0YJ7Onp__yq+Y|q+68BANj4h1LvXuiQlXT}|v}=<&=%u9bLyf$j5_e@S-BKru%IH~6(dCvu2cltcpU
::*ON[!_+NnIE`S9#n&QK|=KCN6E\0N*<qI,3>~50g$,26aE9$UHNkw$[y<-!Y~v)6W+U<`^@Yx_3oA?#Q})=c=KoaD&QX9ZeE3#t#Ds+TVms(G*udbfk5^N,[$@SGM`56B?
::_h<0}lF0sMeaNhCSEY{E{nll[fQu1O9KfQB<8#\xo0S;jg,BYqK8UHwTtgu[]9N9u?A+46^x@M`3_Hk(TBs[iI(%GB.QXyzI.-3eeSO7DS&Aa|Z*gTrMGa`RtYxKY}DBGj
::Dd8uRqsR%$_9GdL;6i;kvxr/B\+~`@&#2nOC~0z&B`jYns!$vFJ#%w(-ETzY}`@N[6ssn0hNsne/67^r>+-\>WF[XK%mi8X/~]&<aeIQ\rU4eSEtZenVJq%}2k#y]WUDY1
::iG8!IvQ.$xZzJ=75o2Ngb0Z1_1seU8a,CV*ih23t87>U_Vp[!i~{KZaAMR#}7=w+8?zU45Z4J(G~2]3s=F6GcXH$jOS\&0L^(R5PecR}>txJ]!VyMH?rB|;Ar`rf2n[lv%
::e3b+t%l1UMs(;+X42-(le[J>VhEZ]Dnf+uxe$PA,eJ|R?`cO{*]}uBuxW}&v1|nH&kCUZ$m/,0>3KZmQ$msH\h4<<gC+8p[&hx*HM^iSG90224WTC3k#8cP*.u9C\#+k#v
::y/DSJqn;6q+FD63a\$*UTk`ScSS~$ZC2Qlh2_}Qq1Cc)<yE7Wy=n*g.nPo$b!A3+G=%yN00G=<*X8*;5$ft)e2y.cLt{1.gOFH,,gTnb|]$m`]cSu}B9i,_eq-vlDi]EMN
::(pU(3LSEXA,+^-)+v8)L8m0XebfjNo!74CHuJTRH)])j#+FJN@UcSN6*?Hff7x*@H\,[*fZ@Uc[@{\Sube#A/Qr7>eUxfDvcW#nIcdTF4!+F,oT7>%r<Q0C~0H4m-?@iM#
::|@(9Si{1(h~{{SP,wrVDL@L#dN#o&UOHkb=KGWi?~b[,6|Xs6hb)FU\6vAGoWzqE=&>}MOiP91,NH>n8XQ]tHY0~wJW{q]S^nJRE]t]t\d4eV_d[*aNm]qY2>q|J<>g0M8
::+k!Jp%$9N-BOE50s2A?<XCH+di^kldA=>}i\4/RGLez^ubE*E-tR5v(j,#>1l(uPX5U5HOh!QW%$7vNjC``Gl*)U</w}JBJ1]C=~+JR3CRQZ|d8i4#!Tiac0nx{n[+9WoV
::z{-jI]U-Pgn_<}Kd+)wRd!lSM<RSNqBz6+_`K0f)WVk6{}-va/*C[{\d.s]V]DsD+q2_xKws`zh|$<)bNhz~XO.us3]P>/+P3yXhk5A=<I)|s&|gAB*J+PS}oAJO=ov~4l
::sG+&Q2S7T/IWq;uB75`#]?Hiop74^fDOjZgIbcg.#4MfuKf\p6wZ?k|\rqm0yb-5]<,3oOx{gY52h!}}>M-l6TEW|k$t\ROV]dH^$SR+&TvSs|Dh[;s}Pajo?=VN00tfs.
::e/y{[H9v`iG<&_~^IdwSc+;JSor{H}iYu[DG,?I1Ye46Bp4BbeQD/kxo5Y])nv9]`Oygh\5-neyGY,1I?SPhO2*^Q)-VwQb?[5lT/^rNPWK&V!<2QdoT{7cyiquy0;zcRG
::]x7#Oxtk[V,8g,>K\1mcuMRbIq6c@c<,0KpU${lDf}[c%tw=b2US@oV6r/cXQan#Clm&bd=3RkD}8XSuVZ%QbB@IRUE1bSoobEGmjv$n&]YGk*t&9<1O|HQEXs(MurAC1/
::)l$V7)u?zd*r@[#%36<sPtl!5~_6A8xma}$Z/roZSxn]8_Dh8y,e;KL*Axn1ET881SHk+=$|dZ2Bxh}asgEp|@vPUsI!(d`wO9\Jx-|}4ow/vG&L|srVZ1=pQxLG~U_]bn
::wT=G<~M``Dr4|?@=+K*Ic,@NkaY<UEMF,PWECnkHpa,IzysgGP=yS|o/]Bs;yO8mS&-zNl?w_kCR]{E![dH*1wMa$EGNLK+G6pIV/h+T@=%;f}9izha4(ePhhrn(a[O[WZ
::Kh1R#m8%>K7)KUU|@u|B8FgzVdboc,jC*w.b.xR6Mi91RlXk~Ygi2>kYMKINC0#mlaWMd\-D-sW\D|~n<22+FaA%m{7xxJg6OKu#HVf+1+QViPzj$oLkh5m9@*XW],L1V%
::iGv)JF(|wV(QMLTsu`[|$DlF7Sha-IEi^[&y/Ol]i%$YExew,w.dB=9\&?z*v!apJnGF.Oez\exC5(6RCK]On3EPk2C5peA6(Ew?\87n7\UK{`PtqdW_FdGVyrN!@!TPm\
::Y8+P+!mJ]cH}t4-1[e2E</f=-xboM8Co5^g$,XW6qzbv=s{UAU}mK@O#Z3ik%,6&MHY8MH*,gNy`k<4eH$>TKXN,{x43AP|CxdYZpu1`>W=&5XOh_C^d9vn!`.y6mxw$64
::2ygbqlL|d&g=nW>]X~Su[41dbl84I^OWNaw_zVQ!i]_q9|U[*g.KZ^EN.kMRMRL!I+>HZ=$fxT>4YETvk<fR!ax`nxk(#+|+3~zTlF\xea74fL8_o}iRF|U^B@G5W&LO]`
::w~e`Lwd^fgd525sOU2^~1+\/_-*9x?36(<lFygXV9g/r>jmR_~{S/O![`D_6c&z&MX2ldi>${ga*~c*]E@41ZuvXqaG*mMAoc[4Y|6m!m)5->vL2h|8+SObv\0KnMAT(;I
::^ibj>RvCly|rp%0ef5i){BCnAD{pRQeUX,U9fn?`R}?-3\f4!\v?FeML=i,k0B7<!]5og4#@B\|oc-P&SKQV)H/90LfJ7wU@#Cb]XmdJ3(Q<u%4C<%mQsF-Fs<mth|FKe-
::xTrEO_ciAD*(e7t]!6oi,qYQV@GPo5/Zm?IJ;_g)BIs35*#K,R6OW(!0\ou@&{,HMvL<ycQjrnfbc8.tS=i{&lDo-tZb-2yb+jTn7i+;G_uB@?5],kQTC{2VBJeakQ3s$9
::wT8E3UwQ6;$^N}>z)tkdmh~^3Nn/\S%[)`.A.(}zVR0ID$has*;nGIVX=Km<9@Wz/er{yxFqYLv3u|d=9-xb[$`P![D?v%~t|jk6mk\_#kURG;&V.#lV@[=OkX^!|k9Li,
::+Z/pCQQbC4+D1CFR[X|Kj{Mi&)l8{]jkXv-3K22~]*eymj}7Q{fnWi2P=^k1P{u^$nHEM^#xp5SDYTL<JZHI`_nOiuJ@m<uU8`G,5VQ,_+7+ch)/gkrSV(YuQHJN}`7[wf
::XNn|IC%FX.`mQp$YYC}P7Y>pB5&DLcRmSWO!.l`V;iL3$lZ_ge[#EWSU|-ZLJe!I{isp{f*~*PDyhLg`2-4^A!`{@?)U.!Z(9o.|HQ}Td<eM~yV43Z;Z|t,=3p6=[K@2Jr
::YceLk[,Yq\-l5uR~VvH2cA>Bc<n=lUU|,A|zSy2Z*|!UM3+^(`W\jzx/Vw%0}<1Zf3d0VY\WQt={hdaf+V(8(F><!Yjm(cm~zL%M!GRf`95^]!09eV-vvF0ZjuN[~Vt2{&
::Ul^T4c~Hp-K>X6I<>n4v|a$C)k|}@<;$Ju9+I)LtoD3Nu>T2};#S1F=YUY!?6bl!{2}~;99z\g[qu!neKKS#c;9Hetk~k/7[^E&vuor52tio3Ssjdq6?bI4z5fPZZbez)f
::#g}A#!)+30K=`;9C#1PeV]DCBpgyr^R5+TniS}|,L1WHLL+or0w(JOH4<SrEG[7nEu-f;u|\7hZc*7h@(KhDcDt4;3/L}IWsX=H4v-D}N10@dj}9V;BIe!@C`L0z2fO|pU
::WYtW<5;4!#Rg]9q]S}D$bi65ChhZ\ViIk9;On*QfL?(}5@Cdv{,Nl\QoEaXJ1OQNlg?mCYn2;*]JfY8(=`7*Jv)6n=}y)2q*iaXXDo\2*;8=wciv6wU?PTEeC`g=c/K)2e
::o?ACniLQljAt,Ak6zmdV=zDvY1`Z9[wn\YAb]7JSBpmeQhrW{*)}RJjSNp|hY9q$z#u)#+mp*F+9M#&s[QdO4zm^hK8cQSO1%A?;LYeI/6yzEOKCV7E4AZDKzA034)l/mw
::ME`qt/$)}KrN(@2F4lsbeC-1(n#UM5~93L8G*Br+B.Zr}lZfN9\Ihr5[Xc2L.2P!FcNg2[.JiX&C;|*IJb;>9werhX!Hoc5u!c&H7sFxQYpxKThTiOM_}&}7b31ee|x$4U
::0~fO}<gCz)=tFYUIvI[^yv}3@X.#!WkzUVU<!sb&qbwRp4AOMbep.cmjb,rV#35&}4tJ]6)1uir4dV.|-1]PSe%ruA?kjpVhB[1=S<&xmtO4h*dIKM?K$PF@xQ!#EC^Ch<
::m_;adO(&<$C>&E]T|/|ePlaGM`9vLmT_>))&[bqWQeBbMGxvfoQsxT0S4upFczPVo.,UthH|&6F12EC44XwaR[Kt7$yLzfw)^VfDyH?C@<M;)Rd2_X8Qg2{dzN9w1J{&m)
::viv5}iKGR5.ux_*zN`kslxj\B7)&R%eK]Y[Eqm*jvTR04${g-!^u~uZUZ~!0O#SxJm8/~MLe#N~IX5M+;2t?sfgf![]Y4V](sE~rHT~~$^C7|h{L!=|vmB!=B_p!6Q<-Cq
::@01t$xc3RjeED6),mXCe@;!@p1/U&Vo>P.0rp{2n)!l(Wtdh[9]E(3[>^gdn8)v}*Kvo%}G}Pmq&!F14[aD#9)-!+qmRv%,v!jv^><t3V#kRBxrVW;KExDysz{ufR@CQ1w
::dr|,iUx#x)w4mA(d4wkHB1%K;I!(_8L+QMrMtnF6`<UT|YELm${4?*vNOLmetvFIJHD!^L=o$hm{V4^)R6C<9=BaH^.msA)LNmGneJtltN,1|fV)t[.pRtio.^Wf$dP*Py
::>Thv@ES}S4K\P8Zqict7FlzE.T\4mi-P*VAq^fB2Z\@%AE88MoFR*\eOda=[RIAT#J;C2aWazZCa2(<SfErM214LwuI$1~B0m#wT.s^-u1q&}-oo.4~$j^GGa!|4D5kFdq
::rbDwwJ~M6Bq#Ox|+35F^Xlxpg`B](\f~e5K$Pa`1{);(.$q;f9$%fi#K7x?9}#;9%AEN{iyQ/VU^)r{Q1.FQzEtm4VpM^!-O1Y7U!?{|2tsSPm}5JmJt]o>Q^^0xLNMm*l
::U0L`~D#[\/al$Hlm$5A5edvV)J\Fzk=3(TO;x*dt}b=ac)y]\*NfIDvH$?11/\T3l5H+FVzhVaX97BL~Lh;w<S9px,a!bO?uK=yb?8Rq(tZOeLfWNOek9u|n6p&/%<Z7kP
::$]D\Wu<I6m3UPkXEPT<IM<.{pXX5.<F-F8,T4(}+`d<Np;ExG5Ki^ACB54QR71oofZKy,9Dw`K=8KXPN]2)/?fz0FuD4)kR|pri<E1EKg2.G=#_CT[|WW^U&Ch)tGfxZ?d
::_5xb[o3m+YxDuRSsOqPJ_H>tF!![]B8H#)eh;aJ}OiY)`X=[R}na?lp*LQtdTNizrB&$`?\,\;MnzuGwx_wQKrbkvSvfMke<M|??OJIa64(GD<L_P=Rj>E;3Mvz9(Rv63n
::)2&NifU^FKSy^xwVc4ijuakzcW{\,~dis1U%%.$@{F.m`]5b*]5C6U4%)Rf2GaebYOlcrPz(5LPDloG4D89os!FY=U$j`GcE4nN[%3lQ;H*j|4},H%lGR&CW?`cXjN#7,s
::bH#4q_P3.n^~p,EUK1{}{()g9/KxUAwOVk6zxf1X^_lI-)dH;2Q_J$_T_~4#r8\/-^A`sYu\YuJUcaf6WppG1.0X7a><y>7ndH]+ix)B9Q3W\{{SQo|N?O}`!s\*QK5.43
::yK5F-QA0a$ME!|0F|G+bAJpVkVJgLuE+*Gt!S\xRc${!Ox&&n`Uu}Q7}01&$q.tkbftAuu0}gW5ZR`(Y/=?Mj*%Mf?9nSEmK|>!WIp!M`;Uz(wb$/YJ6VvCOH7{|V(%[Hv
::SfgK6GcJufj@*R5=&Fpe#;rLD@NHyH~JKj=Q-p}$APr)iSgLhKZ6}E>t!bx>K-!?rMY_20KtXu`??K,0/?h{ji[]qJ(]6Z&fJ75liTWa<YVnPtRKUT1w4*07~P)AojN^,T
::?<m%Afm86{8#n[hu\aU>Ax{*dr>w6j6HfiO7PYNuO04z[`IH=dba6!.q/Ap~VDCtnB(]&.4cK475`U&n^TBytB8[Sv@!DFl01H7zg+@zwA1n15<nrlq],-\@Wz,e-o-3yL
::{!&mvTa7\`}\Mfm{RMSirD_u`Ca-]b>X8e2E|][_l9Q<Et~wyC`!XMK,DY6*lCX{@ATz*ml*U%r})1e+Gnm`^Q4!aWmClQ+<_Oi){rm@P1Lh{)ZFa2U5CqYlo4#9]%+ZwO
::R4bich3^zkYCmo?z|>J)4,[,D*uY<8Jr6y7V2MI<QNJ)7,-]d\xe@$d>Kcp-J9S}{Du\N}@C5,!r}nDu.h{w?ENBL\7W[37>te(a&N=~Vh%6r2pIU|AG{997JIQB+/QkGd
::;2/c=5UBvVJ70jCR@Y|qr.>L)</CGc=R2~<ONY2GXqU)\!GY`v5wIBzjD*Z1@XbRswhSrFF}yp(wUw278+]0bFfYTwT2G4B8NcP5H7L8O)#78LEQ9B.{Xc[}RDWB(P9cvQ
::qZpXA{i[3Yt_m|8ZO}q/sxsSqv{r=@D^V7[i&fe{U*00UTK<5+xpxsWSU7ot,,-C9&/YC~Hb!Mf5~x4(1,#_iCS^;7@Y{/nQ;8c9Vx48Twuj-3`o09Lb&cIx4(`ka+%}3%
::4{C7O;WDVUnVd^E4b42*F4y!UJ9adv[n!T.IaX!6mOkC|w6Gv07OR~HjBOUo>379H<^A#_}@Ah^b8,[I`$AyX8&>F_d_bCCaFn|3W&BbJ0Fxu?B~*}|M57,/cwB1CMSDkY
::q9nhe\yjAfoxGQqpS4J0\%o*3`@)/cN%Vb<^X\VI>nic/xeJRH(jt-m]]@WtpZQyXE)wMzg^87+kx_%kvZT@f)]CtO]F~AOPr|ObZw8kl}6HT?GnhHq@!4yv%*}Hy2YSpV
::5wGA0&jjsO[x.mRp6A8<Nlhu8^wO[aMSY!5pwP?c&qZ`5z]FNp?dDq&Z4q/?[/4lON(vTe\i#_RCdPiqPDON9a#2A/4_;`*haE,GD1e9GL*|qBT4&PZvTR1k-kMT_S&Wo&
::[Yf.xC]}vEv1LSF}&<+0~c1rf(4-Np>K2s-[6])}NoF-v3a[W(/g$bQu|s=MXh5p8V$G5Lz5a8v}H/!OLQ_)ao.~gef.@oK0UjKL7gPu5A8fTl0){Ls_u1>Nl,-V*b{LtG
::On*fOZ~N!7?c3+45NF8AIh3vrm+=lZPm{kD46U6|0>v3xO*2$C#1(q|%S3UqT,!g[9Ek<sn4()ztn^|$,fJHj)s./F>YK8#+YdWyAw-7fcX4+D1<n|+(<76p_nc9d,2q8L
::Gus%o`iq6fK9[-Ca1U(*U4,Pz~|?{X$jRB+umaNzQ+i8(#Dvw_!2TJ8#J(,/t!ybNFDr3Xf&sDm3/Gzo=S35)L3FqPHQuN4_UgDy2<LY-dkQK+jz*aF017t@8RVpcdnDxr
::n-M<DAa?6;YcM6-D>#x_Zj]3^cJMl1RMFp}oOU53J0LHFG^2WX`VM*E61.x\PP-W2Fu(T6bjEN?(LtSsvI.Qzh\xmjza>y1p%>HokD^gNq%_5($vYb\!6S>a(l{rek2LGa
::-S;Bcok/YcNx=|Au@cy#>75eq9edk0|w8XDcD1bQMoR[-qN9P>9|F+lX<sIf0b!Gwq?5;+O{8@x,qw~mU|2E/Zb,heke+ZeaI.PR4*j+er(\7{dWQQ;8RmF46l#I..9AM~
::9[Ye}wm2K~_u?F?b>3e2R^|t{w0AtQgD$_C!RkA8cqa5e/$i?(yV+gLKN6<H`Y^AqxtEbv!}Z|7A{H.QjG){6bJL?6K&4)5&ye_57NxGivmHF{&PuCA~cAkt~2kR_PSaEP
::Q&8L5%(J^&KN\F)7iNZGe];bW0&Flx}ATp8MCZH@>b/b5}35Km~%UgFdFU~;rAuBlTi^K]Tr@of}{%JAFv-{)GA\uG25j6&VK5`\9\3VrQUW#VV#]9^?>s7pF`nv7$kohe
::7V9i?IB_#I~<TN*$uIVMVIjG[qSS-W9mKnEiKf_/>BQ_(a%&-G+|}o_ytBA9m0Mo22dFp10MXPg.k5;*d/upVI68~l@,i/@{#GoKbwwcq4c4Fu%]s=>#d^hy_2Vp@1f}b8
::*5%/Vc>)~XRx}CZS~l}4I!Z1;Vh)cwAEx5h!J%o=7Xi5h`x3x5W)2.)2Q{shYSkKlyrc)#d.H@5mq(w1!xg4vN}(,/,S}*zgHve0uzeB&3QrvJ+kh/}?S1UOo`Wtx2Na${
::Z5.HDlWW<zwH]8;8X.~e,R`tB<#]5\2d.ZB6^0FE%E17`5z>.`A/$xM$5l5A>`.|n3iVUDXuAgIXupxKK`|`{f1Eahj#PPntlH}(V<4S<c1X9x.xP\u,}5y3dNa_g}/]*M
::w($jS1*SM3xvew.PZH$.P]*3<2Zm|q,K@Ye[|dl(mTit.h;xYy4?MoZ<8C~<m=smxjZ%5moz>qh/oj}gH}~lpcOKH8[0p}pc*oY&cYF=X|@TW\qlq@JA?52q<)^;ER`0zr
::^j]Y3&3DmJM2]#H?z_7R)=mReFx.7{.Z]kg[Lg4Acu.VWhH=fyEn58V[p}_6bE$mhs{l@+eR(;S(H-!zdX{cNw;~oqHM8qssvMm7fDSatkfFg=Lw<8E}4Zjj|=)fU+lXHF
::ZXG4kj%k4fQ^aryN(`V`7H(IJVWXexf+8uA|p.el?-39y_)&y[f-I7LcIp.*8+Ezc_cEwK03Q}#fn~aV<m(#]!xDDx@-j@|xe2E?/6-[esL&;UuI#jWKiQ;CoLqef^yN,+
::iv`DuLgn4kZ[LeXC&;\Gz3u|Jc!ldAf7#S*y.6<!^i`Q1O$;^@O[Rzxz&mM#Z2?>;x?I?f9qQePcy7?S|>R7.k7a9Esr\gt|P~jC\^8q,,aF?B^]2~f}U.<O!!c5We@sxb
::5^20v)JHRc>X(4!G9q)3i`x=N4^\,vFwp;t39J8Ad^W4cahGGp#QTaGjB8L2Hb_5(V/hvw.>]1]q?d3G$lyI.XD<K4z1)P.}5$brH_v&*HwRg$FU|T$JP%<#a%j*g?W*SD
::c+O!&MU=RV_=[qk#tcOe5?aPN8v;hm=H=qyDqL!eKKv,ggP(,@p(W%b%w;1<bU6)u.B7o;XnL]1>)^/;4PC8n=xlq;,jCf%>}--!+!5lWbg=dc%8GDL=4~\E?{Zh6d1<9?
::cB%x9/RHXtEe]Hl/s\5h{KU&7g4qPj.EK`4_?J2JG=u#FqWJ2@Mt{CCB(E9[AvH#w.y(aD6~gW[SF2\E<UR<h]0Z]3vz97~g=y\Wi](JZcX5XshbpW&MmJ[bz&*rB;aLf{
::|dI]d=a~t%>eo^ioo3z;>lkl$`I7Cl=fxI#Vyi.T!33C&W0@\m!8YJG0L!6Pg;]nts~&a0~Z.i|iB?EqOm1H.G1*?OlE*gY#B7n+P#F[[9rz+Z2a#5ned70H`l=G)ee9[$
::UF{rt_e(ApRUyt9|I7L8l$gveY#2<,FWp1me2Pue]Z=6}YUK2zK[HV!2wOq-<_V(X9_]WySy_4m3z3M?FDOUew(^ox~124#I`KxOF49~m<ZoPO&l3?MOe@&B{[@XfDmiFX
::XK7kS[-hB&t<sCR%h%X7dlGt,HfErTg>8Z%\Q^}+Sd</R9#3T=+ik=QAgBb,~[vd[yW4#aKz?Czn%1kB{+Vk>xq<}a/ETp^zT8fid0AVX`@w[JRQGl%epx*>`=^Dd3fo*V
::e4,Uu40A*dcCA1ih;RUoQ|~;V]R_OM<@&=fqNoT3Ohr0.B{>^V`b>sTKaCy^s9h`+eqCl|YA.uKB3G[r$RV9RA\}LMD6D7jc&Tsa-FbzjImYrW\~[OhQI4?u$Ja,y+U21K
::|{PN@se&6{cZR+*[Wf2$jbHbLJzxmfL]`pl3c2x%vO)nq3p/in}Ji*^5)hnjq[4!dUSb&#]CzIX,ODY=KjGger6_N8u]m#Ug^_!,|Dk{InM5.w5/Vf8NczH+!}fZlDN;o&
::<,}n/S574~_=Ei4m3{}aqUEhzt=5(rU0Xygj[S<1X<-h@)_%w$U6VFPJ+!C|GEZGw(MSv2Fp;mt!M/2pA4JWMwJK&RvQi0&DU~n&p6TL$Y56$P|X)LHW-tj$a(jw!C7HvL
::Z=e?gL)X*`|]-\Np~RB)?CeKJ[9~;`7cZ(iAr*?_&FSO|X9r]iZ)S~xa7X9JuJMZ;S(`vqlA?d-4f^9bMr`).v,wnA@,mKLB=/m|TO%RJ?SZ-4c<)|!a(_Es7%U$fU_|9F
::i=gJ\_B)WMs[qM|eggv}lvxV?O.NPK%.w]uK2@+2IY-xY\Wz-D&O]vJT=y$>v5.h5od)S_A&Q~|G5dPB){<0`HrT3bja<b[c#bn[TJ640BezmL5u;6hG<9qKN8x4%1OIfx
::#ohRN4c,T(hJ<6bxJrzUcangMD1wRL6.aMWD,sp{xuf|ZQg)w%*gt3h5Jo%@N@>eFU;NN9)KJ$Q\RVX8Zj]B3m!YMosl3Yt~}zj{NuPLn8fu+cq~wseiO8F-!kUh[Ar_0p
::p)\YA!.gUwdPAYIJ,f~~bpOs,0-VH\JT~1rLh6Q03)[3RshB@i\@}+I>-Lj~X]URPk6O0ES%zp@iw6\2991uKf!O)12MpTV+.(+J-sD3NfG/}AMkT=8e%0\VU~i@Yz!BTJ
::X5<,Pym+iyBq/v#/HXH_F-K$eh5PM;]ULe?PG*Oe%s9*WltIa[XNT-%b#Z>I9Ju\?OE]Hp]^l;^fNUK9z=qCF?~XPmSrY5<$$oO+tsqK@$J@u?\7+_v$+JJ60,7KU4J4&!
::3~0/8tAkMFyx4Yd}|JrEGK#cuIC6HBJw(B!<1+p+O2Gvq7hV4c|M>C&xSET<;D{}H0z<Jx>DS~2)OK5Lv^\5I=[1*k*~T{R^WhR\yD*p.qZb&d{^HWIokOmc9h{#3##}RC
::>(DV/Jo<a>9~-yW8ux-Km-uQ~hst9).^%(+jOD%}0lt@GL0h=2y,,x0$<4}kYhp#m\yim}TcT2qXF(6d9ZFU,=4WPX_{;F8M[XxyEUqyk(SzBv5d5fr{6q4a(YM!q<w}#b
::%~G`c5IB?![t-I$Cgcq@Hb`RS5YhqDX+ur\ym9Q@X9(f_jgj9UTh[*EKj(Vy^n!ir9rox@Fv+j2qqV&M2mr1,;~QoCZ`a_b,}QDb42kL!_vFTB6!y.$f8?TW2EvQ@M.vbu
::rQ/^2`l_`{<3ieC&_Im6.-;2y8[S-{[G=c;>_E6!Jf14P@[Bx4jsw%+-]}h),SzqWc>G}SVXhZ-H{(&}Wyc9AFc![3am+EU=6Do>Z2?f&$n([+e</\4[e5NLu.1-W~Wqbb
::l[uGDi$*qL74\((5yF<U*A;~%2S#nXH\+W5^g6Xx[6>)u\.K/s|$cj9wZhq*R!21O6i4C;Wr,An(Pi45E5OoRCG2q]A7(pkphlK-dUjy+`.29SL>CO|<g(_K19~g-L(8C`
::>-D^Yu=H_)qCA2@8!1RX4%^x2ylGcl4P2<~l1iGe7`/Wa-V/F1,w4/Vm@MQ{gw1qPJZq2AJ;W|r$E@lrhh+_PE9b)+SKL,v!NP1?#iE,u3(Q~-Jc78A2W`>cHu_6[nh/W@
::*ey_~k+~ER?}Jvzc@XP|Dm8d~tdZWM\WFA<%0@$j1p9$ZrGUY3H3QBZnOmlq*~3GY9Jq4onuT2W6O^B/L2]n\{[Clc0[H3oV7?^5}9#PG[^81+\pX=OT,r%l!%1&;_|8b4
::zcV&Qe-\fKP(te^0znzx!qh*K%J.i6KGX~EyvZ%[oP+RAR#][zO!rB0*d7rEH?]98|&[-gW,ciE0JWl5o8(RK)XX4L*bMbRbUY={~Cb~Bt3vzurmg<7;yIz(GC>A~A`Yzz
::jD;@%X0EHgL{B!B/llG@@R0rmg~Fa5=4@0(fS)llR}iy*BAEF;yj5^mHE1!qexftkxWC7*nX]iU.Y[x35]-7|iQ6{WH?fu219`\<\e_0?T8YN_oZL75<ycqX$H^l&`wI#Z
::iRhO$!%3PB=+&!C?XJb*okIEdBG^Ce@L*Wa0JpP_~p9%Za3#;yB$|w$D|@vIZWGhc}6Y<_TMJymM,,JJA2v>[R*hH]5wdtFQ\+QL4qqK/FdJM+C;7z~y/6Nz^&uae-y2&6
::cb?I/h0r|{m,_M$O-P7TwiV_%9;I3yo-RjWg\,JBA6v_Y_\B7\/ZN2fnMVThao`<!0m<lZ#PyA$PErODTdlwAjZO8ww,8{%&~g*iDk-du{6$$dov+St^Dz6j1V}{G^Cl+)
::<H)eHX=<L@F#>NJYGc(IN<gl;vn*nr8&e_[x+~~f;(8R_pmX^fx;vKg&HWT*bBa_)es;87zQVs6Ig9^h-0F9n(E%&0zu1Zf$,*Wfffz-zv0q<mk_LKn-lc+Y._4ZkqoYg;
::U(sc^gS@5b$7cT4(\|L(mV]Lu?73YT4\l[uC+T4Y78z9H#!v%<DPz_24+E+2;e6IRFx1W[jT>H!#l~Rs9`36^Gj{2pO4{E([4E?MkjOnT3;S8rmHE02TtHFX=!ws,.;@/s
::$z?/$>~,1yHu@<s60fXv+%a4);1aPmar&~NaROgUUrG=`ea/mNs@LWR*63$t/_#^%G`y1xiHag&UBkg]H6U_YJ3FjI8v<jdlc!ejT!OScS0Hsl-VDVSGD/F#%XH-8)TIj0
::?F@N&H;a|;a9]~UULx]*[We=}Cy_>V|9%_nf{l{]J#xGiVQ]yJO*IIUQ=2)fw8z)3r6/[fy@01+k\Ri.jogpp3H.*b|YlL2*[6k1<E$XyPzCht/SDU}K$7rv`UksEOAST+
::1SN3t(4_$EKdt5#AQL_\;Va-m;`#8#*lt5ficD3/2!WtS+Y(6E6(%s9L%(R\2^}lJ3ihP!-Y>^&7&T[Ux{n^fVQ%DIv)QP!WX\SD.ETj[0o6aGSB.k7gX<n?,Azz7`WrKP
::^rK#@kh?SYOYSbM0jh)n\j*\^a[FUQFB|kT%FXt`qmeFuwNRV%nYb(U&M2]p>k7<,m*<%*PF@#xqo5~I]*Z6ljw+JJOUU?2z8g.&=n!}ML!f8xH?n^62okm&^q!1ngVbaf
::5GOziL/]|jme~W/FNC-Ji7A=I5r,Yju-JJb#Ceu#PJPk/lP#q#bko\I6KH~lzL!Z<ZFxKcf#/-Q^&bAhYrH_TsIU-gcvrklBpd>u$MbnQ?SSu&C8@m};~?O@BhhXBV6\M7
::Dy|`,}wKPGEs6B+Kv`m;/%tjaW0|x+5\*%+E>M[~Z&4vzE<7w=xC0CGq0,,-fSMdt0uUF*y||#IhY\c%h>R(0}M)g3Ie%f&h,wIT{cc#|r$q/})>MQx~.cf>)!ufye{vcY
::BtlgJ]9=~Wlbx,mX@mX)7cOm?|be0MVvfK3bmE->D&3F^0J,dnuan;p@3K/hhP`5gA/gnb{\v1g+loHct,d8|0PAKh%P9I7[3vXO!3C4Z4Yq8{m#1r*fBY)+HoR{6S;4Y#
::C,HD}KztiJ^bdKhw|^&V_sSj+/=nG2`ERMvP2!AM![\[E4Yf6W_ZuRl\v`RjEFu%Bk[%WFN$TS=url`28sR+u1p&g|g2zG??r|LAljUy*?>`Lt4s$P\-4&E=$7tXIoq_c^
::XHXOGSMs!$~$%~2AK3L%N[e9F{*y?Jl>`>dAg9bidM%oj-|&h9H5KRl#v-F>}|o[PT9ioBbPVhCpXw;hmtvp=cX,%Z/ya$&-`{G,EbH5~laU|#&tCKMY8R<8\N]hW!vu>L
::j-v(]e_/EK92gb&9Cb06L`.&0bAyDb^Uu@eqPBYD4-[6ew2+;VoWK2N!&]1u<ZB1a|xCaNY4zM$,QE9DZxcN.wG2E,YEffmVFq8nV^MQv2TSBxFt_qJ$r,01jX;[F?X9Cy
::eXDD2Rvf%-jNGLCpSHR9nOqvRib!+*^*4J/UD>}y^^N`+XdMheM-potW\X{p!WE2MG<]2OGqEq<]Dee3a~Di7i8|iAzZ)#6_EkMT~re=2$ziv`;Y`tVWdp~u#7Zjl-F_FQ
::nyb(>BuaDR*!wb$I>c?Xor_woTl@w%lb9IH$!r~2;-f[Ra5ylRxYU59sVI{`s=3Ls]TeF_MZnw.xg}UDGCBm>nj%#[(7&Hbz(;Zl@FnE!eV[+Dp6S@PGJu$WJu$TeQ\2@#
::Go~aUk*AFR\d4?}NN_5l]RzE@Dx*tLIR83<eqt|c|&GOS9;@TeOWA%Ev70&50{YGr${(Dx$R/#CX^Pb4cOI[Qcl8&!l}brnS0|@+Km9F4ia]S9Q-g$3uFT!0;(3JgYZZBt
::zgMOBh_#o(upB|7xdXuF75Dc8?6W!Dt33G70,b6(1[4oD/J4BU@8le@&*dpyEaN{eJ^_.g~asF\)}h*t^}Dw310^J|=~6r^UHLj5%bY$slPuP@$5J|)js)orF9JH1hRWu9
::MAn^57[ZSS@OQ7z&.uUpys20kk4ra$O.kd2PLR^tN;mi%v|9p((q%>-ckuK*;OddT~r0+u`bjSx-,1@~5SeM_3{3s*T^FklukIK[ab6@PGlL#p[Wt>xvDdDN2Jv$fR8=dY
::^X+J)yW\N`|kj(SdkpgB-&bvj~R_a[Atn7&e;{ffUn4Txn#^\$tu.Stm1b7Dl4\5pA~fxM{g0{,^eRN3=b{Fc@JVe#d=J;eYo{pwL0{Blg1oZ>(6C%;z~hJydw%SgZkK;g
::Dxm{I$f2;$zG[ws({)8$A~BACyDbpv\Pc3OZ)|MVvvA./JMA8nf/[e^IVN];.p,?t*GFYOpG{3o?(a#ox%9<Ds%*mN6wAMQ0(d$9UHa*Qi0VLw;sAM5[JHG{XXLh|ak{/-
::|346B@zL$v*x@[m5V}xt*Q;wOxg=Id?@0=MCbA.}qk3HQ]DbWH>lLFC(r@WBT@7me>*ycg5\vyW<EQ|@u_)8J8al-7Ku;[3%#V%<jh-w()~N_{_qr!Yu^nCQ3b7`V1q?_p
::F*9&m+eri1g0XNSkXjp`8X7+p@2EGZ%^`4KS$6I!)&@K][LJovpGR}Jb@9._7G}aiwA`d{In;dK3m9<q+c61(K\&?Z]y[@II0gGx.w}a@JW9L2zZW&]R/t4TE\^QIR/K8X
::;,N|V#*5BYa6I*2M+/H?ydix>P@ZTO>mON;mfm)`\F9|/grA,3$PtX53xDjR]ws7]7b\Q(V-BlD3i3xTAW67mK5#U\X~&#^_{`$y3r/dd\3SnW4~M7QilTaI?lR4M|)W%@
::uT~<Iq?^aik[-e9i`%*Y&U7`_sVVWJ%|BsuyxvL(LVXkXB(c!M]08/NqnWtH#[ha[k-jz*h~%maKGDi@j_&=|8>n|@#.!aqR@[O3yJPPBf8<G@4Y;{ohd~$G|*An$@m9)|
::Z;|G`LrC[-dQSy;dD}Uc!Ph{E*fy^{gs-c9hAG5saD`%eZ}WP+yCwVm4XS>a\jnHd?Xsz)2MNnsL`P;V]lUTZsoCY`.z{NoT.M-h5xq&=P`14C^+BlMACgT7k+oqt$4wF!
::;kYDznf!gNI}wGl6X?F00lGi?jP4spSvBu;x?![eInuH+]b{}9VInJJe_%q*#(//~[Vf-0SM.Xi<sg{HhJ`G_u&h\7>ebaU1wmbY~(<5&.b&Uy~=&/3DQ%R8*W;j/S?BQT
::exV?5A)+I^9}@E}vwx_wQhW8sshG6S57VS.J);ztRU8WLIH*vR)4v5w{Z9{pNo@k>`\w@BJO=b?x+*+Z2J@TR{_RMhB=05}9v1?+YGdIH/g#OMB&cM)~2?/]tZ}9oCqWNu
::guc=djM(1Tv#vg{rhU)J3?=_Alj|uHB)87Pr#LFwr%KU8{hs|C+vR\7Vb</G57CwI`|wSn+@>1JgD3LL@`|~Xd^m5p3.t0jLm26DtfQsPqFgwt*ZsR9nXt8s9RYh~UJ*61
::wU${wFb-F~TU|G\(-~)oC9Id<;rylumlEhM<6^XHiXs$%q=4u*Ke0^iG4v8?zDFo1[bcRg,HPE~DPK=l,qxQb-0N+dBw#+MW%+N!ujXPCTr&]goVfOk#2`0N<;+67ZhT-b
::3no,RZ_]ZnYF%@Ym3ju9F1w=006ixr&f]-AHZDDUUJf$dp0Y.x80l#b9\A@tkH?fIabL)}p_wF.3]VRkZl4GY,E/?g(DPlBl]w8?g.6wGI/M}<Zzh)o0wq]PZ\U2GYH^De
::W1yn_t*N*\S/qu-E]hB}vb`IDJ>*o,k`OOjmLq7UnI$,Q*IkG<W[|1Lr4B%UdRcULb@pkTQe&_X1N,_LMepRcPxj,jc{Yp~;TfPjXd;uHKv*mBz/BqY7nH>tI(RJt4bcs)
::ZccWY$BodLP%e`a3Qi%_D=ZuCM]Y&qE6(oEqJK%6M]%W%1GC\vM8-Y)q/`KR6/^0/R2{$krB9/CLxz8Zh#&DPk`pDyq?OG[C,#H~$aE!ypBbK-zaglw9hX_`MzZH5+w)F5
::Q/,]\mRM(uJ9*LkuZRFc_y!K#$mL9c@C~OS5=-bh*S]/nSw<s7iM~7**c+eSY8_+Wsvl8TOQF)l3L$%FC]>Q\`>a5<mQF[8-qH{hGOyVf^`C9AjB[8xC?8cFMzAhQ;`|d@
::66zoq__cT=vFJ&<tR3fB2l[M@NI}E,@48+D-Rl8|WZ9_Xfa9k-k8-p+E5@q&-=F_;n#K}so5AitM#]tQ36kxcmj*Em|s\aw&_`*Rya0X%]GCi\Hl2|0[R[*a7Fuerif1Vg
::x/17o(lg5z9=T}TPB-mL^\jdukl$27<zN7[YXf,,`p%3?T%#LK!n9.r;1<s~{BW22muo|d~h{@bGA@(7#,GX&hR6@&+@K/M<I,mE!Gf4oI*vyU*Pnb&F\h[/ruIK`yYKTI
::Goj$uGVtnDa6pnGizEy)Y94Am+GU\GlV4-#M]2X69_o[y1a^8`)Qb9]Kt`>H&W+V%KT_lsIs,rz-n!*Yk,(,>c7Z|WnulL8H{a2xq1q+Wj_O*lh/i,O9HS&^p,@O@`z)%s
::.jCwJk1Pls2EhGSw(EYff^+3zKD2DKdJZ_ikFlMa\2<6zvryZD9Zg)S~h%;@RVR9BYy@YnjaKhzg;!5cZ4BQ+&ymz,`TIVpWkY]~pw?1tSt${F7cLc>A)r{OrNeOM`aUB0
::}S<A7$M<E[R47O$~YBusV7^\?c.tXW97\7v*-HdUnv?!e7Vyzx(>;#UA6g,+/S%v!8%s,Sl~fgex4^XzW)R|lDPaRjeXI5&g7rk;siD@2k^,A;dCD-QF+5@XC[)qhV=KsM
::(4P0zciXv7JHOVb&6Y3`XS?|`\]s3j8!MQu0oK}6TH){@S$i3U9?,#V586yfSe4=l(=r,vD@LwzY|R+IxP]F9!N%Kci>j//M.d1Yr%al$WryL0I[3P-D>8Udy`YLxA0qP^
::VCfrsraM-m6K.9igveZf7H)$}{SR6#b$*2lvu7!pfeVATp1D4UClzZw3EQbHD{#(g,vG<~s%4c[%g7FK=;6tB7|2K?_YUUQopB5r*|8ojuJ3+&6^t|FQ;g=s?5*ZW2\1+F
::(tF/L3?DR%01&0RDU4p*IRt1<!U{9-(wU/X]/j5;*,LHs{+y_9l_Q/.Gb1Tf#QHgXHIAa>KOpq}0umM/db[]2*CDfN_J_vCk~Ig{DVKM/&K-JQNWgsBaiw/(fe#.6nc->-
::&ieGSqK40o2d(/pp3udj0g+yQQLguC;I(O_}V$WSXAsfL5z6\q$q2SaHt1Up!*eHwG5nnt`;\*q#4*oPm#tr]G$wZgya>&@/B^uSX)lauol8]K>/q98$S&c|a=yH=D~JZl
::RbJeq~-Ml*rPdbfU<R8Kh@XhGZys>nN*blE/L$#KNoPdMev/.?=\+NLZ&1|%65_pz_Jo9khp(l|WufAdCdm1`11bC7fo/%[q5f@HE$9-!gxF=!^+aWa|MQo?jLI+]ut&~T
::7*lwYVWU-vH^ew^$Ix(\Hk)HW(/-_=P*c}?XI6N|+{&WLx-[H$wvWOHuFKKTwYv<tR{fRMAE/u\n$Dd{{En,^+to4b!PDLB<suD;h_br5!|C9$>aVLus=4BaCD0sge_x\q
::LcK@lFh9U|x3|a{61jt`UNY0[`a/42Ofa(M]-^<.}g6-Y]blgUt=_N{[w?W_Yx6A5f$E,G8Fw+=aLfBtWYDILm_D80]DusbF\!QwlTc3{e/}]nO{^kS>TYt)z@EB]%[!~K
::Yy-wG?wm.5?3_u=J|I)2%lI3+#5(H%xH=VPYcZ1A[tGbgr_pg]>2Gc(2v0%RLA*N[AO1x!?#IfWs#FFGak^a6_$pIgQ9lB+kEl~&iC+T/F6xID4c>ZAvG1G*!}\hPVIrc{
::X]qG97e*hed6-cNV<}bUuL~-\y{P*81GvA*l]~iGHaL@($1b-;9PlwtQ-HgFxJX$u*n`gP\N%U{USrc)hCvI{-Ld5&kx]<cVWfH8(y>o1|BNZ[|D.HzqJ#F#MB!IHiccZ8
::12dzIi;*v*2lvO0SJz_e@^m?-?,Z[rcf]DnA1K6y{vAi|W(SXJH6b]oGlnHoq()=k7*&>mt-Y}=8H4]Jrg@;EO~4b&O>.Ev[Be<*z\qqK3![{)*(OEX-NTE,5UjahJ&)h]
::p#]z<c9oo3iSp8)]v\L/?JTZ$Ma]G[\Bu`-_292vk=8_pJ[zJTl6vwi|rb{r^D.)R#p3^p9$l\C9b9fZ9Ebd)*!*IkUV}3E[fg!J<+,8A|&g[JI/z)o&Nw4[cA?S_1nZ#d
::%pvp<hVhlG9h@/U5wDwCgk1(VYjw*$K7Qscz)^DUunRf{wW}jp81V>\,cx9fhsV10Cr[{_rzFzXg&?Spj,S7=V}2K#Zjfb]i}j[JW2{cH>nWpVj,Q@C6SYfKvd?*Sj<|O%
::mdd{W9o`PdRE}\bi-X[=9;,_tx#`vAi+ha{\pEaI6S|PAf$e38SRt,H}YPOb<QML+/KufX.xzAuNM636``I@,uq)y]`U{UI1gX8-CVaL2;79Olj|P{sU)]r!l_p!lIC,Rn
::P&>p}cV<AUwTPLsQPEQ(4JPRS[)Wphg<0DwGsk|NCZ$pLcf<!m;$_uF=_lmHYuZ@[X~`Fn5+B_hRfEt<E{;dX][Bi.Y/-r67/P~BfQM=MGMu3qEZ>7>cTBE7Q3Zp1d[IyU
::a(}ti5=bam<H~Q{nkRq@%M|VM,V?KGK4>_JU}tPXtB+lbwei&7KM;!oxk\(vLV9#}A~?is)uWjt|G},a*)ecE{gL|dZ(]H=M0sP>[WQr|qiWlMx6F*d^)rYlsCsaz[a5e!
::]Nq)3|oNMcsl%Xap1E[Oo$(RtU-~3Y~#f\1h_Hz0y6xGRyJB|AZ6+Ir12FAz5k_5pqLxvo2b8Z;==cv-%czr@JO8e7)Yr\Q0x|.>&.NnsN}sQoT[z_.W2YAji*U1.FRxa{
::4K1p_`Rq*Fjr1z#s[zSQQ[yWrh|&],<5Vq.mR!f@s}1F$wC,^/b-qoB,EZ9FPt.`6Bkr-5#P`|j|0x9r3{GvO8l{rE{;F9[gb$5rX90t99M)>s@0g|z9Ey4J,@wmA3`}FH
::2Hr{t$fA=D!T5m&_X(d[cwFOgH`#6Y??q}!v2St&([~Hv5Vp)=~0YLV\dm`3(u3T@OWDr)lAI#<@#!DB^5.9Y+}c%p0^I2`dnj}!qy-?#tYyU?2W[[JUqyAig#;s~&%OfT
::Ghb^jRR|dYQ>5wbm/+B?6?_U0{_?2ufT&92dY[[$\$6I_9V;X?qVwR>v7vHs4t\|SO_adxia/i[-P&w*){Am]mMw|6vW#&YSH1~K@k.xJ-DyzBa}#Q|{(8q[&tt!8{LqGG
::oIyZ8p_#,~@eLP^mG*Gk|+T/7+-]C<B#^I?cTK_^-g<_8+B[jyF)!|]t`|L?A6j3{/2O![[X2ZpAPgG!^g_ysCZSA7u~XX[dkl32wGg2B=*keBUqO&nM2H~W_qLM%-/MVX
::h=VVA4fu#BUVD+%L#a~Y]_a2[3jAs[XA{{C@YlTwBR5xoFGk7y$Iv{ByqqK`IoN8e9}XT5a9(X,IKH4|/V7E**9VH0n{o~p}~q!Jd~|\!Q{T?_ZSAx!Y`{rJOkL{]ltT?/
::VdgSWX_h>~#y$MGPj{w>-OW<MwdyYUqJbwsJ+e_;8koqo3c7YcjO`~!d]p`nd^rWiR}b_]$Qe7Z8JpE,/P@8&lzb@(9!CXEbCSVaakW{UQ0Gh*IvcH$d>dQ!by~$NhHTIW
::]!.B^%Ntbo|YHMSq%Gc#k_mn,R4HW}~]H-zndkGJcSvTMHzeCyD#8~/hh-^mt#Z<zGegZ=DnwtamE8QOI3$0&?k0G,%O+so6rVs>ve&{TQ|CHm}=xy0R@9fEkKr/]X^;@-
::e?eZMp.aJf^E7HOZ]JbZ`T5U-IxeNkNpk59wT*r2bn+C&r[}8/%drW}s%ha9;c;mmSe;qSZQjFqd\4!qw^K?#-ke61G}/!u`ne.Sy7+LS]Nd,Us(^*BwxhZA5m3hkjw8M]
::&X[KG@LPcP9X7VK}v_4bd,UiGZK[j/mEBHHx~0]rr/c7iKsRoMCac%*8RO{6}y~d6REG2$33(|R`%)]w*hbee-W7h!bCb)LuJYbL@LTSv]-;1bv9_a<x)(}1rudkG3,OQO
::[i%G)6-o+WM6efGw[&P>+*4P=I4fIy_0sS4uq6K?jKzZ`*j`afDFCh_4M}Sc)cI.Wk-fp1%_1Gs!tBQe2?_0IeJV>5S5Vu(k8{Yq;pu2s={Jq[NU,wvvpgPxu3DyJiSgFg
::4wKu)fE3,G)^rs=^q3SSgH=T|sI@L{w[AD.}SY0oAz2$/jKaJnP<(ufGgEH%Kuz5,kE|F{VQ]v13%n*WOYZ0Hcn^<$uNp+&DLaCJOq4#$b&_86eg1XY=yTp1!,y4\5\_Pa
::(Va|aFU]b(G6~T>7@3#6>fS3AdLHHo&NPk,kT43#YgFS.(XhE#+57phAdi?@<P29R4`sQewv@<]0NJV^QLBBA2)S`&up=1kUpv[$O-ditc?bv>}nZwsH?4{Z#,mH<dQ/}|
::f&>Nq~Yo5k0_GS0ztIsofuO!U(~gF?%Usnc9U54OlA~aFu$Jm-*5,TX+*uD{s_H[=,2AfW<`fXtt&9Xc.dAt#{+BK%t)W7uR<@SP,!VNW8NYibwt[1!U!Z(3[Ks2larpV~
::BkFZ7~zBM<QoLD-76xG6<hGWN?k}J47c`V[(s0q(UuY5y)2F)GMD0gJR9o4%@[)Zfi^#.uZTl)|n+1|nF..4PW)d88I_kOX06%Sd*OFN5AjJ)HROL)hK!]K-nQa4q`Al{J
::fGFG<&wem%24o_/k(`YdPV+`{W%2!gI-E*9d$i}01}1cxJUGotB(V-p,F{m$dI.aP@PcvurQ9wOWs2`6B+t9Fr~JXzFH5PBqf}{6!ODgW[WF2eQk(|IOeW4~&h8|YlJc/0
::O45ChJDS]d~6%D1W7kQki6U@i&3=(#SUr5g(x{U_[^@~HGH@@m<LM81{$S?VC~.Uoqj,Qf6k~Tll/^L=dk}KNQ4zKfTn(A5_/qO=9fF%#EEin6{@tAp/x&+sTNjJ5yFa\R
::D?ij]em|_n`>.NixWU*70o;xXJ$3y&6*]%pSU?c#Pf10f~Nt[Utxh8sAK\KJR=PU$&_{1~RdXJr&ZAz&J>b)<A6,ITM6I?@v&R+t]{sqp}`~WBCKz!)^7(KY{b@nX?4&Lr
::Th.2}NY(tv{B~~N*.E*Wn,ME1[m3fiSUm6]OziIWAe^}LHV+k}K(jp_T/{xNdp(Zt-rN.JVCm<]zv>9l=tep12!a{{eK,3*sDJB#},AGBj%J5l{9!.\]B-U0[oXaW]-U{G
::*K<_?tsi\^5<St6z{h{6OsL1FNdohI+hJthMj<Rbb(0oZ]*[+Z\bd6W}%YIIyo{7k;h&^]AAqiZy9l+Jh4VkHLl?9fVqi!;-2P?bJryy7g]@`)&i7TP#0L{<uuP!Pp^ihN
::j4ah`5K)gR77x9Hyu_VMXup_k+ZXL=Ro7?ACFCXE>nj(S85`B]vWTrK\d!?ANFO8ymYmJo#oFE}{GukYNjaC9BRHSH|^)Cgku>D2Jo_y,G_bw@#fDB>{LqC{1fmX.0fSre
::?^1+S*d<15ua3X}-WuU=M1|L={PSJ42Ag5)FP~-4Om.07-vq\D5g]&B0)[,cM<m8)#/=f$f2S*gOQOHvah(K-tE#d9PZCeSRfW&GNR|q@)xKB$I)lVx_@c5|#HONxsDU_H
::XnyP+&A}g`g\@{rq&qN;7?T#`76ydzqApWwwliF`tx<a~}YW+#-o-KkdM+>jycQ~]qfW7BDA~7N9PpByOU^[rW3?U9yy<iwS,@1k$C`}?J@+KZZ*iKyvE6l52T_~3&P5/l
::L+hc1^<Zy6@MNW!tO2jizTRP|?<4>!cAY(Q%!b<{MgXPC!`W&_q4*S=v\OhQMMfk;+_]5)3qNyvGGH1|a5/$5ggGZpzh|>RY&%jB-0@](8fL?-&Y%4>6N[Da`u=49TFPI+
::F^*uo\S3t\8ui,Kye}dA(R@C0mFyfQh5GAQrJ\}K+FUUmZ!*Y[j(C(<sV&w#8baN^qGSL)\,2Tnpr/(Y,gSCZwzIf.<rs4#afejzKE!T>0.apmC!nu7]{)gV,V=ow#/sk^
::u;2}J5mNztUXO-R$/K81J4)Auj>(Sr|5b];GbR\2MM%h&6k)\mN}R[\I2)7I=#-(zQ#Q+VDPU[N59|>66Y$}+X_C,~Jt89J$I9bM!5){kvWDROT9Cji$(yb#^3-g\SzaN^
::~GO8&(2G.E,O5%Tckr^#Xu,6`W}%\l3%+NSO,+~5GlbwI0O190xV-o[2omDXk8eZ&#3$XrE2`dS9CGUF4A{bL}WG*AJK{&y1s#yu9r|-EwNfSxneRJ{j`bdQ4$lDAYr_Jg
::e\i{3V?];{%]mS^.-|~si%w^Vs?qpwjSw+l@qI2=(I_\M&2BGo|mq,jVXj($=?#6sd.@}DR]K=c^dhd[%ic6)Q]BTS@B0&7b>L;,N^!x,X`C8W/DoKH{AYYuC=aig\GKsk
::|$w#c`Jg7PvTMkNO4\-bHss`4P=(l{0$g*yMH^VUt8Y@*TIL?4o$z;E0d_+UrqEz_E@G34Fq1MXdW8C~C?N^PtHIb{?HR)_D~e<|~]4(2bl2~bwi1kFR<PO[}h1<Q}5+=9
::?9][,[e<SS/xlVvGE!14E1,oV)c2Pn_>k0\kd9(Y7D+LgS}=WPRNojhe[R9@.Mq.Y)(?MOjIyvi%6zaN,*mCP}KC$A4jxl/LFXD,2s$84X_ovw&_Ss_7mUQYB7|I#5Y==x
::_RD=J64jK2xHv*aHB/27k_$g/_)?Y0Q%8`hgHq+vtl%dcXoMeoaWDC2|]Zhqpi1.[1Rh6[DDFk9;*,pEF;;3<JDo[3CH/I%vIG#\OSw^<T9Gs<&LU{NwYoo1D~{3nc2Oa?
::Ahs`VL4j^KnVW1zF#yjTU%Iekz\r~mX>FE$%X78iph/I$?HsYG9&*,Qp7JhW`cb5K-_8xXM`OaYsQurUz&*lJ$jKui5]PJBF~x!F2LEE6b!H4l;`M`o9*q/PZpDGiBqTe@
::ZwF6HTDYo0_eOcM)F-@T.YPFP6L=9kj4\X}_qa[7#@L=7YdbA9t\Ejlx*BI*`_2R67<bMC_,={-aI=EwUa=+f0U&{(xRc?\;-ROeQ!_sY!f3dTk_!6<{U;Bq*C?C0HO%(3
::qUW,a5R_Bc9yp7^nGfG%SE(7fmCgmQ1O$jM/zA;YNy@Sg%1~p5.)#e{XD)tim1<c0%s]s8]{VYA9\\M{~$*b2{i5ux~o4!Mm\PQB1!dWLPi8}In7)^J$zaBGD]K~I,O_J5
::|Ru{p%g5{]k+45NCZ0QTp[+=#>&1JTFHg}%`<h|!zN4~/Y+zMOYgzuZUz^fhs{j%=p-x>g|ST`\X{LG+v?{=Q7sQw8dfG0v^g4T|@=c24b-&_NH8S|l&4j.ScVb[_^95P,
::lB}1Ca.~1?ZOJI6!z1u{2f;lqPdr=7Ll{U\8.va7_reXi8CG~F>G^IIdX9+_&~J0^T}[m&Tnn#.#+xM,7|vBmojce0%Q!mkYYE^4^KcZar9W06YFW0KOo%I0dnMi&PgzxU
::c)$-*P96SDw)NJqs724X@V-9#`$K91lP!M]RieVua5*e)wuqI1I-($|Ks>`K(R%!nR<z?\vVGhb\auh?VdLm|*NX@eMO74mVsPd\~Zo*Mos;JemG`w.\\eS)q(7Lx?ta6o
::4p>89988#t72%gqB9Eskx@8\H+;TglmK{&n7%NCXx#)BGV3[z%FrR=sZrvN4yOb*@{4n~Dj2kq=&U@rD|V04t)2NrvapsMvfd]q9!^vOJe?h!=Hggp*E0Dk8aRm/he(o[z
::FHglk+lpn)6Vh+E,NbjyF!apG&#dZy}dr_Pd#@1k6?{2ubM7[+7=oizg@Z$pX@idSB5(z7C>/>kpMPiOLfvFep(@LDX|t3.3J6cj0@dAgQ9II/nn[bf9jp[_/UarR=EpKm
::y[]vPzl,t`@JlB\vp|ptDqU+8cew-XAQk`PP2,DSa~&ULFFbTVBnzG>WPCjfKRjytl+Go+Xmmehx&g10S%SvQHVcLQ62DG}+bbc/!uzYyqe[&i;k9\>?Cq-+(Z?2~XmCNv
::!4[4-J~K(+y>KUp`veQ@z8#mfeLrJrM*X)9UI4vjn3=&~q(Jc{KC93r4@$W]ki|*STkfij%cP+\T!M$@iQ0L>HLV%NubjQO;B)}4cDiJ4GR\ON5Me~1!I?1ilh~EOB@b%p
::v!=e}r)pw1i);dpu)I>;}AIiz3<j/W##qsRd\VGi>`[uC#U4k-<c@H+N}mw4q$Q5}UEDFa]HW0vsF?iiORr57|PNUW_.NDL-8W<MW2D(Y|}BPV~*flg#)DyYbt69h_8\wT
::zv@W3*$EY+PaSRfv=il7jbRCej~MR2gR?*m}]t=Y~?gQc*ISI=M=`_$49ik*xqg&J4BU}iELV(g5c3&Z5u@*NY#h?b*w}d`drlQ~D!DKvR|11UnbpzT-Z{?`maWY8V<9b\
::i#N=3bz)LY1zh5-}Zqr->FSa6xGA20_yMZHy~9w6f;Rn!_[9cCAoHVWEO\L]6@CwE}]FC[|CQk2=r=~aR)QEp^=S)JX81yjeqABD6TPcGT<.(Y+tB$rvRHvb8JYw_KDmt{
::Z+`cLRc;A{GF.IVS!ZjF`-PF;GRr~QX$ouz6~er+*LIL-2#QY$yE24-/o$aT8MV$}A8z-Z6u?Sl+!jwU_`;rL=&#ixIK.V;p\M/pPza.+4wVgQ5OvXg27?w]rwX~>B2S8)
::;!/|%=\_2X*!DGs02AzXD|?-XcMc3vYtOZY6@*`l7-)2+\Fy@<{w/;0PpeXN$[CNl3qa@K>?kt?|MD*/*FMKn?ge|AB`~4f_+,FWES(CD#9e!g+3M[Ii2899HGnFpqTG1K
::_L1~2RW{5u[vRU-[|n^%$M],4y=)]\/cw6l*G7Qp@cih5y?%>6})77^s3?z5zW1_K&Hw``250(/l^OI]JtCd_/F`Jg@HYg/i}Ym%+9gSpgr~hLk?MqTz;%H.kOYo7jvO$?
::vxx5^$7Q+iZ,{YESmTy}ZK}/OpssUI4Owxq`Cbza0[}~&&A#;C0/ZmfCaDADj#vTjMk3ylbO)`8iWG>1sQ-qYFay]24\HF}smi7m)vDSFJ<sP|e=-Yj8u9Uz%RNfSsl@0l
::hy=0RG3SzsqI=(Nqa0PalA#}]dUXe9e,DX@1a8,<HoxCLC_A8Vj)Glqi|tSCZ~_*5PadZCGg!7/]Gu?RE5x+i1pldGGT+M.]%5rqJdQ8uC!&#_A|lo_zietx((-jxfge4C
::2_a`dv/ZVjT;&+MX,\(Q/2Kd._eWwnp,<~4{Rq!+`\;b_nnyT!A6HrjWV2<}(Gv;\cI!HY8mvqt4NqBh_r{1}F6wj_1F=ZMI13F4,,pv@Q-ocR!hWrS`79X{wVJ[&iD3!C
::IA~FqLCTmt(3x)a_ExTp$YlNttg6n{am{r;+OLC5\_Y/S`h%;6zv,$K;P`[6jK^NN+4v+!WKc`@BV!s=f[Q\j2A?LEAMY=,$Qjc~30H)w8De0Djrc/!++Ri$_5?2P?]3v6
::26M@~_no(J{B0.qw|i-f0#Qt5<!X-Ah|%uevQGSFoSCce-QPF`Ee-6)pz(O<Zi4IqB6_z%eS*9jbrc}8,Z?xN`2&b-*BFY@Zh[[g*ui$7#mppg;[R@M|DOL,g8!{jC5B|m
::1vok6H/7F,5JV-V]99ca7e@%.xmm.!SK96y<?Zz8f;%k]SIKkL$()H0g(W#\^Q(g(vqxJj_24a}PE?ocFZ#[P$ZM\sDd;JK,y9#6fZi]6|wV2$cU8*$50L$<Nt>e-5cb~n
::r1l{X~hj}x0<W2Leyw1UTPV#JvV]z<sM&X\}TD8C1?a7={R7jH,Ljy-d[~yo;trA=7y~\Qf+uAXcB@tf;UuZH{-yp4ZmS=9kT&xC~(>Z+44Wp=v7?7Zd/**+KDXKFd?@^R
::fH2<bvPwg[qsay6T-Co|Lc%Vm7elW}4B!a3CAy#6v`%%[vZf%\4R^S&3q0^DZ[fM_K|AxwYCD9e+4P3QXDTzBn}k/Y48BYYttK|ounGu-DMVPR>oe6v#Dy0PsN%`r;c_9^
::s\a0MRub<E[TI#r|,V[%q^1%8Sx0F-kj]2O4*g~J??^KB)n-BpIDXj8p_[eYX@M~|wzRHgRU~51(e+uQ{tK_#&,leV|wmT6jX=8Rgv-cHeYciTl&]7\0zgeqYMH7AV!\tI
::PJ#NWy|w)uHNIZWya*KgQeLdkh-A#Zo$f*77gnsSj;Gbt/MUs(=FJ9\/PVEyM4BO7&\b*B0mGhp}r/s4Dcbwhw%}yywC{x,=[02_87}h%R1_VuD+n0Rmj?*Uv1<zu+7D{o
::I\.C!<X{$[t<Nsj#@Zw^&\6]7*4q>nU=oxF|%^80z]|GCQNADfkE=99&fNyu_V>MjNys@Bu&`3E/KOBCTr^Qu!NFJ[*jbd[4DK24j#1XP4a|\GVKW_azwcr[8gWh0Oz<`$
::?K(erBn2;qy-j|<9HOvh<-d8&jD8.|sxV;)tcK*ef}az(abzCb9wK9?uXb~MR6#ppBxbYQ4{h&OHq=LPB_fmQA[<u%D/mn,+}4S?E>aB+=2c@Y\|4&Tl\3A-)oHsyC{q#U
::UZssjn&SD`r;G$oTr(lGD6-[V(k~4`wtk,{1ttcXLvD|^iEP;n*f=S\i?|WR,c\&676r{f%6>U?zm,uGG1R$4T}<<l4<mWd6uL}fL`3y?N}*\2X)<Tu5%)-GxdPv?7F$0_
::/N@aD{4Exx@$EBu_Wa@<jtze3.qx`XyxC_fOvBUX/Thn5/Ip{#}BgjG\#[\FxA\Th/0Y3l[pWiCp<R*5)pnV,~F}/$QkZB*!ud,WyHx]\J*gaA?A<ZLuD5nLQB?WBs1Wit
::]dA=TSRh/?k?qWk$%8g[ETlpeP*X>0-s-B<P}<mbgTcvBs_8?8wq<Ca]ZsGg{4T>~UrWc<B*/[4]I2LCJm-15$UggI-|D[xYou%V8nkUF\^sTK.{<+QS?\e#M93_~Vdf_Z
::T_,q5CLf/)A%e/bETduGiqxtu@Nn5mb|\p6Pxaw1,Ie6.aR9wb|RzWE]`l,SI^;5Uz]1cz.Z+]xB]rGRa[t^{dcT$&\`L4*4mYQuRlAw_ISsIW^dCI`Z<5P$Ax|ld<K[J%
::@nES}ME6WFo@/@C\\Q^C|{y7WML-(AJjzeE!%.q-K=g*hmN$R|s~7Mo@[B86Q7Kva\k!<vWARs0qK$E1_lJZ7_PS(,ZtwE~w.}Lit&yp[B&h1J{l.IMD~\wc\7\Gl\tb1?
::`O5?-L#Ncd=+hT1Sw-TN3_8A)cM?eZdF]Z$4e;8$>T32U^yIww#K)k_JzWGypf<0@%SCMZ;i[/1LOx<I.jodaD,15LN8VzW21NHV*s2\Z97bJ7i{lm;vXLH63X#YLmL-%2
::iw*=1F7A?&`wdrRj!r{4F)R2Wp-|^8MkNT[Iff^p~h>p*cnG#|WmCdJ)3Tz0PT,}1SZE~_wV]~HXeP*5qO<.1U/H-6qV)id4v;1f34{<9,kenn[e?v!q9/[v_qAyQC;W#A
::RJVqW{1TWX{$!7k{>Ebej?Lfq5wXt9omV^!0mDlc^ymqy~ny3g;YAA[UF/c?n\^)wxdNFUxxyU`,HOfTsUgf&egV~U?7._,fm|YFyX!YLdO+5B_LJc#%N0b^$I$Ac?2$vL
::U{rw_Yl+*XMJR;1&y}yl1!{h`=UU!yw&;M(~&+~r?K%[D4ni!$SPD7KK{VbO1p__.=\&^$E@W9{Mpt6PkMpOn[CUNS5OcGM^vXSp9JKE?pc*RX*d6]l)_bLBQ;}xPhop7c
::z!1xi@8MK|Ee(#WZbW`c1RXQ.y%Hl8s]la!h46Yas3NkWBHM#RqLkRQWjDv,)/yVC^@Up_K$I0M7@JRy&ka+?8c_UCbl2g%_1&p{wUO/wUa.h8C!=OOIaaIy3W0{G_EVm(
::mIXkuJ;G/=Wi)DTJS]|YBm>|T$<lJ<pi?D5E$NXC9h,ckQBv+^CGpmY{/tyH@q@3tJyJ5jLcQRBDB0rcscd8q{K`e4[[vdJoMC+EO/Hs?GPbz5ES1|x3(1ONy]EixanM`P
::PCa=%RAj19#aG->SN7T#9tX]KYDU)VY-D5b|o5oqY$biKLHc?vw.u/y-,y/1Ln*Uz;k31tD,wCW6u4\l59afknN-{]F7a.E`Hb1,rg`GIz6XE10@Rjn]sPKkZLP(G&)2~^
::S@e62M_+3i/)k7;J.xoPp<urS0II~eQ#Ni!;0{YINY]$IN\0EjccR2&~5)TxP3,$fSK]RJyBcvC;t8Pi}r=4?*#l=y-<|Sr,y#5I{->F_2_>LXI&A{acJT~l|3FUX3Z*QQ
::cS*hGP6;C^,P~h_Tnqu!0m[rD,H2U6cl;_8VmoR,B6M<MR`ChUb/r{\S(fRb+f.K@?[D(D]\Ccli.(g=t*$R`^4?Q2.k~$UP=~+bn#Q18cYSFK?q8<XX>/e-gHFfzYk(@?
::&28hDrT=|*nA>#~k!yN6%LI5W82SR?KK<of!6zt]j6zcj5&Avb-B;tDbR}]uq*rFW(FPsyCuB#%C#=nifC,d*ZYzH%hu#*Sk)0uH!P#l0V8D#ODCCK^ZH,U[Nns&=<vk&^
::~JQkic7LQ?u1[HYE]qFrhCIRYzC-8]rwLX&/%d5Ki;O(Ppn`DYUO8(L+)Cmi5Eki0fe3c>Apf){$/dfIh$!z5Ko@4-\TTvI&c&Npgq~!EO]_qn7`Lk|&EiXut?Rx*@6?b-
::$8FQPMmKct.;CmM-y5q@UBC?O&RT``6b*A!Qs&&qqMirb77One#eEZh)jxuf9C_2SQN#,v,|{,V2fvPq!s(pg8d0L)1*XfK0U\1BjWs8xWdrq;l~6e_WS1v#lAM/g%JdSi
::-O>pGN^K9S%o#@>R)T=_{ki0R/Bc&uZ*&2Ve^n*.vl83T`t73tNdd_J!0}S2GIeoap6r;7YI78goRf/|>HsUjhS]SQHdmkPm?tZ_H^/)!A7iyUdhNsSz;L]_u&q#d4tyr6
::j8RLSSX4`G|`kwtB.zw/VBzqPZ?Rk6h^~7UbA@Q4dj!G9+G\z4%/)q6w.zSGTjTX}B3w#X/DHg-{)to9x8/dEZB-=i#6WX3Wg\)*Tm$`,Fq_+Ft?<LjifCYckdT{q@xl/L
::/o5HbC[WH6?7jc#yM|>7krPvV5g9`L-$,j;^Jx5f6~q#k}0czt65^A-Wf`6PWNF}v[iXL<#XA;\_XiT<%L&[)h^+,o~O0q=4D*d/URoe}QflH}yxdZj!&91Gy~%W|]w(Qn
::M2!9`owwF.aQ1_-H5vOuaSm0^U?nS^#FAUXWCL6Kpmbc49yDN+D!QsjV}m]*n&|Eoofd;ht_gOM$`eSnbL|x@RTs2~;qZQ)=Kuk|MLVrV$]_*s3LYg!I7bQ5+XUwJro.n)
::n9]f^=RnMP%\+h17`~aqz/hu7In\nPFs}M`vE<}=Ia1V2V~gcC.|Fe4|m\0PuLf@P=uDr,k$a6_8d=|EKm.%k0De{&J&>u%8x8fJTN_K0m1s@fXM@1;%9!-hyNA!0mB&Kv
::LbW87X]5=Y+\<o,w>fR7.h^-wD-<!%4`w8GiM=1li2p3Q(R(R/+Y{8|m=!OE<M73iEE`DY47d/@\M%]i=bhOL@P7Vra[LtX\weo?Q)x!JXUGJ`@&2H4|FSzEfeqnNe61-b
::<{y8IRh_;dWQ2UpYNe3sR[Z,!u8~Pc1NXP=4Qe+vGM(?)AN!*cL7e6Wy6^o||aTX>BmN]J*hrLgYnz|BP<T9P5@VI$y&yWEDk6)2EO|~iXB+l8\5,?qgf%@?%N5cd\G>}q
::mM&4vZGuuBxgET=|]m+9QyLrdF51q_LOTm1[j-PpUW|%_J>mAvP?t2m_Mo*?{2*vy(Aq8j9kAYtY$m_JGWZ~6@S90DrBMw|QOn!Oa*MG)P!|FN?-X\~e)fo7|%q$@<fDH5
::]EOLh~*QL38>.y|c_gpIM8Fd@H57ezgPR0?Yy`?vF^CA{eAg.6W-X%d1V(eZta6apcSX+wOa;JiNwvpO%&>~~9ZT#w4?9&,>LQMZ,>t+M6k$q{QC0Kk^@zYdw7W!TjA;>g
::2wL[<6OF`8p@Jj`6dREiu6Aa#w41|VJ,t#NFY!UN.CX`hR3l!LVNy<I0oE1(95kCe(qh/^}{,y$).){u>30[R_,kkJ&C5`Y[U8R{gX2`]2B@ftJfBPeX|Se~QYh0|77x2%
::~=<Yi@j)|nV_>7-xH{eg6kJ*NsLYB}KJ>8,B2BwBQ/p8fO|K07%~=wTlu.Sw0d{{ktmo1V*}=wt_0gesjaEVB*S2DgW??M&2m7ZVff/@7fK/~89N6s{^=I^thpNXK{RD6V
::6+R1SBZ+2?\B!!NELb)g[*Ss2`(q]0MI6u*Ac1UZlau@VfC]+f#\Eyzs9n%6Fs,uKYg[@bYdV[Qa4?s{97^Y!Uc?BF<B9u8T<+J%J<g#gp{SEq*L^}vUb)5CQ*SP\=a\q?
::Rv618?<js|_`u|*Si<@@zIQ;3*.=BEM+rFm_K*4c~-NhBn~2j]`04cjl}p!L)>*.TdQEZ{4Ne`K5QU6E=E0,g^xxYPyE\8OBrUT~D$igy*V4[n(~^2|HJq+X5N\$<Q{<a\
::?G,E`&eY%;51b[o?qK%-3S1BsXf<vjP4NUCdznw+K@5J*0m8|[}=I)3ZFg*7tG~NTEgM&7OpS(}+CGO=Y$.N-J-RJD#&,=LJYy`V{wtxB/De?Q6TCQonO8Sio3Tl.)7+_u
::5X/,3;{RJBkQG4^cSF,|\E~TL3!HAY.)/A3EL4p*OE!C`b/-._yYV4^M$n-Q(kEy!4=M|4(yH)B72zj*0v;gW_oeO=t(EK9}F0kzt&Iqa<X0$)0W~@Vo[IZoMK$^12r5t8
::)14t%(zQ7YhuY%Ys[xVZfmAtcJJzkM9aR2W/,fBAa<ei,iu0hgqeF<%Ll;4dd~**3ZYt/(E$<^ZWcafhigt0R-a-U~X,}v(TbC%9bPV]}talDS\Pma7>^egLcPd,76b0r+
::YuV|Jka`JXlM$l8AtL~HV1@o|#77rNB}58.)Q3g]W@\bnPU^h)o,df|(H|NN+%*Vhtnvg6^{2r/y7(JKQksgc+HBnJO\)C0^Te9e|F>hjB09%))P~5jb6b#b6%-Ttg`~<O
::vYas9g+gq|Miu=}(>$OY\ITO.)~8W9d;|V|MXlPB_h!gSHXAC=SO\NLnt$y\?khQMP#|1#FP%CJ6P]?y8m5uOL*f.[?2x9zq#7OB~S6QIjQD4SfkKvF.nn+&7#IWll6P~r
::N=E_$fzt*uJe-hqnN5+Vkia\&O1Omds6EaBfk`|BHp{Xa0tfB%^{edM9d$)*%$7k2zp^|RYwzG7kDfx;,$<+l_Bu#7S)VmNjsAwwJX?|OaL?Z-3sL~K9KjIYU#AlN(<\-b
::}xUfxI;#P52{n]WE$Xb?/U$~$\(vVlBR($9*1!,t~d.(@nO+}ls%}x&bP)3,Sz^yuIeXC8Y3{+T%{@!yXZxHV[C7~9J1>d6Cs;F<,jeJA9;],$]rO?,B>D@&^E5AX{wD;(
::p=d6e!BU|~tu){4sQ/Cv}t.v?!9N|}]eXCn2|qQj&sH&$thMIEN&l}gr;)Vq8WfWIyQ6gG]-1PS%A0W{sz7C.d>oGwyX4)2};k$qG0rQX6`\tb/*Vtb=V\JmL!q^g<>BAS
::Sf[5\d\@ZW[5oRMx\TDRM<OeFd7(u>a-c}^FAHRau6Wq3]PS/#w4r~ry(ct}Km9wVR{fMbK<%1Bv{l$fNW4W{cp@a,emdJwg^G3Yb3[a8}<&@b\r$1}^;L%#ucVd2D=1L9
::fiKiDX]omYN&cb1(wICcK&w#%`q,w0_7G8Ms745u$HA$`v>UVpj]VahPz|G$z#n|Zi?=BioZFa*e3*?M/Lh@fWxaI@vjZ}LQfUJ~_&%W-E_s@{J)c!A;nDlY!|#RDIb.p~
::/c3^8c(8\FGT.?~^[L0leLd]Rj1U2#6N0Q/\1OdY]!u147S^Y,`_<g0D~0/zJUp]<L&Y%dNop=~H/IyaD8jm^4v>JGPJq8n+d@fx`T[\=Sb7?EB}~u@q0)i[*#krQu-5/b
::k{rbInS*F/jtDg0,w&B,1RBEl4@w>ZYt)s,g/KjZb4_olaTv>H<*Nl~Kgbt4M5k|}#!TN#=]09LoexmsFd}SJq5%u4P^k+$wPrMmJ,jT^G2T8|N,`LJ3YZ!ju<R0*.>I5m
::P3AsH[r>2G1I[AE+Ot@z%tCX+LQ\.]T-uIkOo7z1PABzMuzKL)m[Nh-nB}+$6<pfSQ`s4&sH\aajYOwR8[cth1+wp\77&uwfHreUGUT6_m9_9]_/1(8]~nPY=d]^,cjV5!
::i*_x4#fD=,MA<-TEB)2ra&c7ybgU`/7@e>QS5fYWUl4p~[OIDMYS/bX+@oLoj,JVrv7S2)Fl$I*s=u)_U>a0PHn6MD/vt.{=\OGDXv?V<X4/R1k*6\`zh9j3z@e!N2qaAQ
::_N`AOO#nk`4G{^Em?{e,F7?ksV;h_C+Y]USUq#AG=!<4=YLLKc(94[|-wtyq#EE<4/>`as\D~B/*=Yq_tjDRh*(aA[P)p3/zND~2h_S[^AnI3xF`!%1p{6$FW=$BFB}v]o
::twa3!^F]|X*q/%uk(IXpC{Le83ju1R9^36istFsfL/oYiaaqXQq<OF14ji}U8Gz2vF{DdZBARaB,jfB3m8Qg&h(jP/;$byB$(jb2z&1.J^@E>pB(;~LRuRTN?w5;zMjoSU
::b>v&Qfex;!@)C^;9#>Ru7A_kA_D@su\v^egfD&}vh<t^j[!wY8wVX4mXr&Tq#!DKccyhE(5C@Izq?Y](bv)Si*1aeIO_>~&if%b3d,$5\buBygwZhK9vGROX##RD!RUPCz
::T)}06vqXb@YA?bdJ;hL]3}Y&#AvWT,A3YK}Y2a)26Yg2=T52~w2y__[Fnxm-Ra&9sWwvn5XfVK`8N]#|_g79qI/3;ib/J-95J_F;;55$)]<qjl{[E@+gMRFxa0\CrRyO.h
::0_rYTOY,m/CrL7q`p2\i!(cXA^2V/GqgM%n1pX7dM[IqaABs)LFBfx@PAz]Ku;@K]jbj38aV%i[lSjM6A`oJ8Bof~Wf9c+H!Z;ha/BZAx05n+sRGdKDPDaJd{O;/Ju9Gy.
::Wm)uj_-HzQJ6q$a3xe(X*kgh4[gA9s8RTvwHMy19?7j<K+mGh=knJ\b~H@L!uoF54<VC&wfOtgB9))$kDF^iPwZ7.(w]C<iwLWF~6$9@(OyHi)\j54TgBczv!;28TF[WM4
::KCKZ$GX[h(IepX-DDy*\cY~*E]Ae]\k4*H~1*,&|c~d^{IzBK^xHO<RTF&e%sH6YDM.}QeBDr@0}z*VTW@U=EiU\%&/lCXKz_84E8}6(2oL=5ItgjfuE{kF8Tz}`me$OZz
::%O6Qc*C<wg`7M!miY/bc)<&KHJ}^Pi2G1A=FmqoG&KKwF{<k.jR%5+^]Y%Xws.r<P-!$LCwYq!HsoU@AAz*($M2oPR\4y72,hQ,48o%ix@J~&oiNj!6`?f[3h&.VX9(m/i
::.&D^MbN-wH&Dd8*O%(-]$+!8*)3m2u%1vD,(PhR)B23+Du.5M2TQ(`LjVoFnN1\dvXO)|%WOdzoqm<9\/N7,ER0+F#EtBhHKb)a_W/tkSVg7]{VZ@A;PaLA[a\Q-Ay03!A
::~ig=U~cB5N(h66\3*v\%hXwi|-;aGn+#)8GJf+/Ztdh.yi)N-`e[E\djw}8^B`w*3NdlV3v4fQGMdUr%n!7gzw;jV?tGlw.#,))?R_MFMU&7B$Y`BA%m4K<n6,jlB!$\eh
::-G[DGtul;6oLTw(@6aj[|[kAGu8Y0Ze3dnqVUd82f*,9_va]GwL)$Kj)su}_=1p-N{G9DVbV&Z;*Yu3WR`6tKT2ZDl25;Sw%LB@qmb|j;kYPaW?bB%w/b3p2BAQ@xY`Pzv
::(_3?h~k[d84gQ{jGBX8T=Gb47EV0tjw7Ki5q`\wz6,{to5nJX`#gx{7fPn2[35n+9WDUZg4/A|S-fG/v=bMNowF^4ZPqXV}RjRb@Kgp0;b{$cK5kR!2&MB1V!BWu`U%L3x
::OPl,3HGkJ`G!|`V,P?HX7<xD1,&1X*C;z?v/YU<T<JfH@dYJuYA%0d!\=O4D|}XrGQ#G=+5RK&iQR\wxYyv*;<`SdC8c=|sriS#VVCNYcei(>%F}zNt2b[Lz&^o}1yYmNg
::b_JH8}c4NkHu$RJ^{[9k7UgN2_yUdR%>HblMLZ;*W(*SIbS0lDPVEFEQ77[o}m0mie,Q4AUepWSWex<n!X(niZmC,]Tlk,l{u%A$bx6=wMamzcjU(D|,xkne01`[[|Iy>2
::I+~4kb1I]!l!gn@=Tl+N[T]@lJ}=b|`5{xz/#o9Go9y?A\9Et9cZ=W1p-eBgcRsg*1eY!bRCC*0&2q<u2|I1b2KJTAU8])dF,i,-c*q%;j%;w4R|N=n\5i`vE,d*0oU[K2
::4H@KsW<,$_b-g)$n_#|^M}@FkBjZtdN!jMRuU)^vJ4Uwl8HvL=xOOqZnv*283t>h,#s$CPAq*{5e1vLp[Ryokvk!0|or)rg&r9(fUFip;d#x*~6$DBAf9]}MIf8x~Lq%c7
::pB2qnln7jQi184X]P`eFrd[VnFwZg?V.m87|b-{^T!UQi6o#u0n\8F<cE@ezo?qdX(Q45aJ~!7wso*-M7WB?L|&iWcv%Ly[MULo#nq{]AF`I(jI5Ys*4\cu*[Ivg=a3h;d
::}|OCw!#1=VE#F!>WQf[j7,v)MkeK7RG8-gfTZN{OS^EMdAiQC9lfjlOY~n~DG-b!d&&s<9Y[(\sXc)8Z}2W|>tItW1GY^[_c~l7VDLBzYP6gxahD)o_hYd;*xNEm|,B4i|
::cj9bNjS@Y0TL`uR6QFdc=Q@NM~L(KLKI8C>|KEc@5kE@FwS-Ll}?mOR@{LxdLViah[,F.IEH)8GID$qd4w>[?Iq|>-eZrFC!K<UpY/,_@a<y^KcEpX}ElVzA&=e=K]O`]\
::P;$Z`i=q[m`zhp_p=#Pi/z)3(v$cY$Foh!;I=hb{Uxbx-t(2@0;=Ez7\fTJl@095a=\QHGO2Gv-LqzIU@<r|/PVMZc}I*RCro(Bv=={WEMh`2~`8Pm(I{!<1bT)X)NYZoO
::F-PNHp$sqFI#<yvEmzH[6xY|KJK]shxoPy/o-`!w)\JnNFCrSHkz73!-kj(m8j<])GZjm9P1&fcJ4V2V>\BwNi%OzF7Zy?+-KP]hkBIJo*(xQ3Yl>{R%SwV1H#LNwvl-?Q
::P<ii%hHRvJ.r$fTd%R9-9~He\xL?miC3Wf_6kLRw+d%EGk&*kQ-|v*#JryUfGEprNtB`N[\FJMkR|@jf>%uID~srudP5N4BG^4r7VxzZ2tjB(<SMYch!3F`U7!]]-!gvSm
::?Vo{e=]E4%?Sb-[MNG;nejY{RP>n95>3D0nC|4^sL(4+hh~k,c3AgIqJMu;6jvaGyBF^B]Aid5-[TT.^JJRTD_14_6+Us/[ARzW~d^?0.[Un&oRdzd$S+jzW~ug<F&v,f|
::@Ed3Mek`]yY$`F5$9QeIE/;VX(!=[PCA(J6,`9MC8;Tw@(yVFhJfUbxqBLQgv[VjaG5t^G;2.`w!|[TtR$G\>9E3-4/K-+I`/q)B*ou6jFLl1`vJw(#>1TXu2X}w8~n0(`
::]6e>YhP]_\H?BL8q,Hl-`wZN_Pa}n+>1>hhwonxTMtE\^uqn{/WS&oAN(XIE>.f4gx(fyQH}b4V*QM6.--2sk.{p-;EH}2Td/fV.s{K~~o=.a.>{(<OB#;aF=|esmNB==A
::5}h(C8Z5iX}Djp>J{*~qI(HZO3SFWpl.?>3,i4S5OPlA;9\?_-.EW<%KSkH8r;a/Mz7%PK{;&W{P__jApkA;R_&96.G9P$n]A@iniPK}CN%a6To.eAoad-RffxvzRdAW]d
::,JV.H{gg4Ex;w2O&2\;Ocsakc3,Gw(%fq=XGPg.5LSrhAZ+\maf`.WiyyzIP-+VD]=5Xw`&/WkL\IGIK>n52dTqsz.d|Rn-3qTv0<%`2Rtf}dQ#-$h(P>,;0Oy1Q9;$h_/
::gaq1y,|XNx{P\.UNYo/fj>hy_g%Do{(j.22.W.!(N;t!WmbN,Xrst!F,sV@3E<rN7VgW.>4HerAEc5U99qjJ-.z(Q51;E.!5)9g[&>a7$(%kBam>`|)i>C4{]3Bi2Q8Z^z
::.;/3-G#uF60n_8>|>x`aV_=!}rCJQ<-*QM+-`uc?=r86dEn/XOS@<6lCol?Vt{M6v,=UId{b$X7QC0@,dn|q}jM9-&Ly]W|g@GO[TCr9/hh}WI!GG8uGkS,Pf)!{^PP,I.
::0B[iH>H8%WU].@@-ttem,-Ow_r(F})^Jg*Coon!Z=$<[?-P5xhEeyIimkdShq!Y`h[~;@yy[..`Cpk,.+M6((gE[N)&zAh2?iG9h?WML}TGr8/d&DaYj\6]6-Or/}PXOA}
::~<a)mB$RCr6Q1G\R|pSl)O%yhC&t-i2&$YFpQd8O>mWAo=RK?tnk!Pdy(X5ok,aX6)o)p[#I6lX7BT>D,x_xdZhv9|A,G|;.+7P\T5][N=nP(H}Tr)Na/[<O1#O,M_~K-.
::=/9JvLr>52w6->}.d8j}=Q4|DV0]I;u8dO(.d;<8CjP._9b}h}Y[5DE/ep;nA*\0iVF-ER)7$+1dtINW033SW4*eA<fx4@o/w)AA0U$jN+&^?^}c9#,ghClU`g-9Pp}X(6
::DDyqMsQ9Fz`XX^P[QBb2hiBA{2@,-h$_R#)09<AG{4|UTVh+zZaFtG!oNNjeRZSx)|5Od8sofXEtO^.g9_{z;)J~9nA^-T813(T9mu{2za]xh([_tf8D6zYI[8`9>a7OgL
::VhEy))X!k|Ce\zKI%_\oq<kg2/59/(pbS_KO<?~-shzcR?~.+Wx}>$0k6CM4C6MQ>/#Q~zM3v-XKKUSkJYkI$YDpsR685*#SgK<[)2SZs6dPAB^Lg{up#U*VNWu}I9mp]G
::mi`yk/Q$!-=|%E5kE@%H{V#I|8T|x<lhKe`x`=M1\<y~]=hS@V}YTv.97hQ0@5Zn2%(#PQxm4wu$q*hDS8F_%{0JLi^Z8kRI8=kPNi^|Z,;U2E<?}g7]3K1@ZL/Tu)Qj&N
::Y(Pry`$]<+n*)~iPh9WJFH8@PgmPDtC4JxQ%mJXmwrPqVMg7_.0&u=?J|;m6_#z&0R!EY9W]/8{y{[EwQ9y4RDL/|a{QKU>O?h^g9!-CAz\D8L$kT@9C!5chhHg47rq>ro
::/~s@i?QB@0f16F1j<beX?L3xqVA6|T#cW;5l{[5-;#sy#(U}^AhzqB#(hY>v&^pt(by,2w1<R|0dV[C%2[3!MYidhllK)V.DdW-$E!1Sz~sZ9NE^_jxdt`WwnG{a1)5F`/
::k$6^yd6uGze$ZqB(0zBF`(i9/_9%vKzk!|hw&!28A8XWB}[7<y5#@Tln{!40DK!rgo5m8uux8m-c~3_2%^l/%r{dmRV^;\xt{Hs-/8.@@gn57H/o.|OT/ui6%u#<l<KG4o
::h}zOQ}x0Zg%Y=iaa*53@Or\rLP|[=-7IrTr5S_U}zp5/>.1lb;{H%h-qjmTr/zJmZ!h=NK9.^|oE~Q|dUt_*Wc;IiJswFhY$f%y`zXl%f)wFxwm0f)QK^e*uVakWfxW|%>
::}7+&/<XZzqY;q.)z^fMz!B,d]m`VG/dvpnb%\L>u8de#.2!~1-&CDy48Wz3qBnG,/t3[T=JGtOxn\GtARQNB[RNR3WYsL_Xn1&[!ij$<gO7ib.Qd64>_82$arn]]t&>YQC
::hL)9yt@OzgDUq!m=Dq=ZPg.;/N`t653b\\AhsGUlByKI}g08-~lT+V4L%|.CW%9pW9Tg5B6R)8m{AK}+wAn@?=xjOuHR)svueAY2..fwCvFc)p+?;jY&V$frYT}Q|)B8l7
::?d_|^);?R)Y6A)(}I{i0-~3Mra]#>042@F~nqK>rg@L1/VcHwS>0vr];3Jp7axRT)SXVUil*0,-__<2k#B_2%y8?92(5B6h3?@CPmX?1tK+IZ|>I4QY};MUks$.&.V[Mft
::80Hd2x6Yd(gAOg@4fS,H<Uga/,;7|IVZEXSj4I<oS=3WK2nKEr6o3OQ@vr@B}lbbM|kIC$tThwdv\m`rV/z]Vgp/SFz6ygRYyo!_]-lWkDY!^]e^0_x;k/M{W6_pT[F>+_
::u]Rv*|,kp0\.6OR^G%8V6q1r9(YGZZa29L@[,Wr?svqYi(FO3[w1})@`CbY7dKMd]*m*w/Ca`$T8)\*`;l3lav?nD!Ig.&qKr0Hkmh?|[j[@2ohGypf_5twl<jTt2(&@W*
::<j]JTf+UYML%!ESSf-D6;vm]QKYFDgpbF&g`)!^>$;M^P)Sd0yUFWG2s^=EF@m>_LM[$z)BBDqlOrE4,QFOx0RWjBBlghxNgI<L;VB`A_eKYtJ^C)IJ}%aqa#h[MUfY7aV
::*f0?_3-49E~fX^M[yv^GGS1l-LKw2<nC`mL$PzyE)nI-Y6V/z*C}99!t9K8\q]iXEMb1N&gNNMPE+%)Fxs%(>ovog8r\.I[1H!q(i2;cu%`Ft>]t5<tYL17ion]gTnMl2G
::SZqcxKf(xv#`Dk*ISl6z0S5;JyR#?p#b5,VJ][}{?|w^`sy*?Hgft~?K>AMZRBe~Np`U$bVm}Kw|),O}\^?<{c$H8c]Fe`GCHhO$#]~^83YS-lj0>ZlE&-9=8n2@z${@\c
::snC#<0[o{\xpZp-p6tmEM*hU)hn$Z3(@)n!x|1H+%dI&w%m,7UB\Yo<ypCKb]42zLzoN[+e,)2v^!rkoNx|w56h\Q-KTl]6=~U=81Nbely*tS2{4/VA]q1ZWXA&h)a?aap
::Id[JgZaER%DuX7}JxPWhmxr`c}F&&dgn7#QWEXtY;AR/F_&oLM_84<qJ{iaC2*A^-}+nqF@HbV_}*%Kv)+=HZzP[RQrK6|@{A&i(Y7D2AbNa-~_E=>}6=tie$cI@zfq\/t
::MqIjQ7aMiLm1.]GgHTc!sq!*,l,u(;CUw@N,|~l-1;hN|B;bH%p*GRF;$!CJH\76_&dBhRjYx}c_(j(o@wz-}cg}Al,Xp9-x</,,wGanR=@,m0q\YH,P/4OP}_*[{3[.|y
::0O,P^b_AN6XqS7#>e##p~K({TvH9hWXf,K[E>I|nx*80+B\mLCgj|oFzT;t;?>REB`+)I]w8Z>nE&2`C`^Y\UhvOR24)*U#uZoBM^3Rd]v{DFsfoazuwf?W0q-E4=}tXqi
::RL6laoDGq<RZ]uTIwJE`$=`e4k=KCkm~pJaF{m!&N4.54KTYu&{M|!_MZ3>S{fF<GgMehr[nJ0)oK0FzDE6\W\fGOojI%18Zt<=P{L!~STF>#z%z!-^RG*QL2b~4@X4$bY
::WJwYqn|>(0FpN^Z9\Scn&B_|3*CwZ~YheGtX3r-YJ*7Z,uj=Y(Ga~3Mp{=qOpUl8~B#{sJ%Sn6CpxBwIbX1~=i$VVdCjAhqw^3VWy@pjU)9Le=`r3+(]^mcP4q=&%gPLm*
::_W(r!J|ZRX`|x5r@w;[fo~R@vkAvQW3kDh)U9f5?PO[kGkuTBa\YJ.BXeFN<8Cez.Bx&ni0C-D0j@Jd@U=@qXq*[/AghYay[^etiLSeEIasFJ|@TVJu&vN^7%E%&`dsqaq
::p{N_X#VXm@,1|hv,?U]bga}~9(x2Wv7h4$ACp~liJLda.Z2Ame\E-IC++50<CKlE+TBqd}Oj!=D|tNm|(Us@3KX^\c,`Gonpz*uks!.ltVam-j3,h2\\9-9aApX[K}9SS4
::,*o[;Ak[.g|\#P8sc%DX&cZN/)4|x3sTAJ8d4F+6,pf|XG/S0(s@<o44)dj|3F),[6II*Z.aOoE6_z%A>K4llEE?=PK3QN;,*LI~KIPf\gQfXAYGroeczldLyfUG4!$FD2
::^wvt)H+Ju3bE[OLyQ4e{w]F3D``K3521%XSBbW&(P0{kH$AZ<c8Jxibfl!/Gu!/UDMr5|XCitOvaKIEQtBRrf_86[kQJ@+F1U=l@JT>n<nY7<c!{V>Y=2i^~<v*,K+8tKb
::.e*vXT?rP`Q1=aoyJbQ5C1h>P7sJM&3+@Wrn5Wj}7St!9#G)VfzDo%.?ZK1{Uf$k7~H/FQl3ks`%]n+sdRqE^;)4,CG_!G;8LMcUJQ4+/F%3i1#8d\nx(`ZZ]Ekv4pt,if
::[u.K2|Xd48!~[V5&Faz~)qaOjXd*}$cTs&[6!kTxWW&9+`soC4\KB]NjKeH?v]9(Nrpcu^QyRh09r_g06(5J,;*B6N,q%b^hEvuwZk)6+R[hw72X,59Skmz6tS<&FeZJPb
::~st%_Ywf5[UZCw.JEt.\/JFh?mMCtG18zh?Yy&QqsyIS[<}YZ81gT*FqA9Xq`-fzi7P6h@MLRCb^S?9Ey|z9[]UIS`<5!DzgRS-YSWPGe1Z~~DugQ9KK_E!\=~\]Gcv;km
::\474-rRNc??e7m)4hX9Ejss,RJyNeE(m||q4F;%h6MBE2;>Ltc_mOi(((<7&=1<SBzXQSuR)&%,pT~gX1lyZYvpMY-UCPG54;x-&c4@lt9v\s+XM/HKDh(L+EWtm[(l?Sn
::-*~${xY7R78Ul`{5xHrGFNc1M;,pR8o/8j4zpbMdWkY=r0Wm-hKy^N\*.MSIxzxmYPx}$*|oz;R$%p,6ErvVqcgYp`$mSN9l`@2{Kb}}BXC6{r?HNT%nNksozzH4V<HY1M
::uqZ@P!$2$\Op}^c.XJFEvqYb(~{E_VBDd(Z}/Ysz[7K+y25;1lSaB<YD5l{;_k([F9@S+HJ/JBotWc6Q7=roLEO7$@+(HIe.]y,/d#)ios*`2$w`.8T3$vy`P)EkxdNo.x
::tlJ3NU{rFKlW/h6H;`DFnxFW9V/[%vu-GH4@,Jmr[R\49!tVik2SW$,g;B[oqQ3Gy8&uf7~sA\6C{dsc[BT5-gQV0d;Yx;oS$jE`#@(R[y;AmOBig)gM[)_D\q}&\o[bf<
::HufN](X<PlS;<~)bRUDB+dv#j<@zVnm@{17Ce9G\uf3=e`w2g&\bwl(k;@qmo%7*#j66q(1T.k2{Ye_)@|4Q5WY)HG?tPc_E84|zH7l==HZR0jyuHI2S^N$`*@DM+2#p-t
::XMo9jKqMk&Lc%-x*e%iMUH.$X\*LYn6=>u~J}\l07u5`~#$X1;FcUE)zz;^C%YcPD%FYaGJ{aSJ5)T5fo/~%etgngT?=paq`XM;B9pZmTdwYSdf/-yhpe!,AhSLp*&$0(%
::]ep?t><O$?^AU}iE5)SNwNwP@]!mN_J>;$;xClBVYy`ldCc~%VG!MEdJcx<p$#XZ~xAo7qmmGB\b0{uhqr%dfnCP<Nf,gu6*i{1-qES_[W_|xsT[2vIfPSmHD5m(~F(o_x
::ZWWGY9tqxs+UT,$d!UBTZX3Z`\S&O9sK=)+-dEyn6.j|QSV83L?jj;ALI[Z1E<I9@;K&>]VBqTR<8cJ_A^\&k3m<our\\c}U`WyFfe5Z4^G7ZS*mt>[c|zBgK!0?{h*Gvt
::-R~@974?L}s5kuC@!*W;p/7!eME8?xCKBmJx\2{-vyM_E;bY,B7@OW&+U!FY4&cCxaSxnR.Ek@5I\c2&Xp$#miGX>LPIdi\7p\LL>Fq[n1}3wr>QRy;A8z{u`Mx8G\/E!z
::zV*;ZS1c?2Poleo9Dg\$@3;F*VL_7I;?fYAOEY4-zXS{*qI0fLyDG671<r[$k$9=B$x}cR5]pAvAqgnW$^GYlWRXns>2]KGTX0Ty$_0_r$<e2]@0M+bb@OD@3)s%(B`RY+
::XP*1U&;ELjy=[NPssd$WL1#WvOsv;Gd6zb=!;&K7Sv=A_=U~C{!~LIM#1x~ZO\R9n]tCAE1WGu2+FT5MH-KY3OR[.q</f\}h1?u>TiHu(5DpsQbApbN=|2x%.Hz%1QjIpF
::TV(VR7D|4UJn$KFXpY9w@[*`Zo(cPR-fCO8i,f5-}&q?bd}qL;PZ_ZPMJo[GEoIh0M@xkYV}cfc0XG$?[1.J#T4/&My0[^t[@<RW&NmLh-6ZDNX/o-9nLy4vZRq*R%_A-6
::M;Xif8~}d2P/yF3/It6!@?YT)qZUJn}/5T~iMJ|9dNF/y]=FVc$vlH+%5`dnSh+mi3K+5F-\|CiL*J,O?|)P5@Q=rr$?X}~v932vq$dn2J13v])Ps)1[/BU+c3nMy.;[2`
::d,1d`[[[-E?F,$_@Xzg-y!oOD]7F2)w\\G3mex?{)C-pD_H|>D|E-n/%1.nd!*XY9UgDlZ+>D[Znieh~j^Iu|kN~;%(N(~,Qs!guJ{o[2c6quS6@7&\(ODtavlt<RqG^GX
::a@eiCJimN=0OS;L7STa1O!/XZfz5hlb;\*8oYYlG%i_gn*z\~JpQ*\`/N?4-kG%0F[oE_0z~l.yKoa0/)^lSCc(*#u>TiA1A70r4esIMM>cM6_-r-=v+j~($6*QlumVEa+
::n<p-p7FFQzzzAMcWl?+=ehb_29[M<(z_&|y/|Uy${uz!SrmuWWy~fFlm>l5c%z9k?1<=-1^M/Yjc.cyQXYfT~>2a]ykGh.GgDV+9Gcu2{gKd#YB,(P+pIg79${K@Y}4)q3
::Al8L(,l2q!XXEEP3K}VLtNr2IyyPeEbpWc)t|2^r_mUn1R4m$kQJ\FTC(6G+>1m.P13IDj{pxKk5z^swNEW.%.*5{PG>@{&rp)qa,6q@@>~+u}-.qy,e8Dum[m??MNk?.O
::qElZ|$n5Kn!pVJoC{}pHyU=Tq&U8`+Lt9a+-AlEp<P-DM}=Pz,\zZns>/?|(1.`)(PF7jJ*5/^qGO{b=>djn&<Q.I25-q.E-l@7M~L{p;p]?cYo,f[q[c)WEo5kFyu&aB*
::!8z#ZGE#J,30|RA;v<eNe2_]8MZ]*4t1qqE/F-vp*Y`){@Qcjhr*F;?OP.7elIkz<(LoHH(-_(vM$lV%GBRUN2@<9V4<T{&@PP6NSwYC1Pg~j99;11=B\]%k%)).@VfeW)
::1,+C)4dP`)L7*,$?R3nxfGxe1WwZBp9y?wzgyBDT#xUhB<G7e.]CKl0d@msH9=Y?wz#jvxj*>BZwWh9f0Nn(}c]&TpTZ8%X_>FNggbUqw7bY&glHCh}HOE@kp&XVPN39Xw
::Lm@lcF2k>|Tr=nvl;DQY,%+lkce&~)pUOTgEHO#[v|=]Y<E4.9(`aHY27MJkNf^~8e2~0wW3yhHHRavC3knAPC<JQCF+C4w#&eTWp#1puVyySwlo+1$dn3IkCB]v~a4S**
::\]4J*KZaTrAx$Rlr3zGgP!Q#2!&a%whmj3)DkM)=hEL4mcEeqj5*bWlMUuQZS[GW8^^?RLUhrgvur({DuHVBZSfI_#;yMB+$.^RBtS@I[7a?j+Mj-zgdVNahhftB|hnK=w
::-w3i&W0l`+}!Ie#j0%uy>0fDQSwTxRf%0mqLLf~9wNGq0EA5YS6nw7MY6*ZKPs]!V4?{JI124Bb&IES_}g7!Q8|rqz#{c!kr>r7KBP+=C2%~`XK}Pw~D#c9n^)n1bSU|^@
::YeGvf,3jx/?kZz438f90N<r{`Z,)oieXby<8lmJqQPqag$i&vi9F`WYhI67!wh(g%V*apt;_NhKK^6V\*!aTUf_S]#;!idQCSc|qCY0O_RJwn<Y21YH)@Wr&FaaA!j[D!|
::m0p/>xrl?5z*<tBUW)bk8_TIHj/g)D)$_AEzh36(4gEXdG)W#df85iAOpASZXF&pG+yFZSE,lxo^]?B1Mg~rru.1t5.d%VI(xZVvHU%3/E@\Ipf5Hb|%d+FQObrM_hEOUN
::On|)e[XWOYFB|rA0xy~^?!1w<vSHpG[/1!8V-Wd!UC8L(LX_j7FWZdjTTd4k<~wxeBsm@fu;DI|^a~CsDLY!-ix6[mQg8~9Emu|c6FrMP27\hVbP`rqzaDZb*j\Uq6j5(;
::}eGG=LzJQz5b0?A3VA/Lzc+E8$NWVfNjg-Wax4d1#|P?KMS+/oGs3Hs#6+,f,4JI*2/djhHj}8P6_TAf_P0CQcC$G+3nGu<bk7<sp^n[Y!yC)qq*eCz{\3OtwK-IHOp!sh
::fX3tCKecp!w(CriF=Dz-jZe\DZblQj5QaA<cEvETp8Wt6@1XKvna+M6/nyC^]L5MTdr.r)`/r4Sf5el?[lvpVya2f2,eY`_zB*jVBe9ZOKH6/DRZY0Ihvp;=(jrSAXz3uP
::%ou,eW0h37F<q=hg^FwNs31r&~bC)XkeN\R_Fui)n</Tqbw,Y81Fh~mW-YYXm8!@jw0t=_E$C1@xHH|Zy_([Zfj8pq^(-uvIlbk8^}zSe-htg$QF(k?V+X!Z/2_synPki9
::HGuEN#\foN_2,Sm?$TyM4r~w]`n3Jk2F6}}s-m!DzcEvnl@e&pj!cJa|1$x;.$qa;vG6?^;)B~ggC/Cm2Wu4X&A2W}|$<eTZ`AS~i^0j;R|<(RNqLEy*e5hPk/b`s)X\O)
::I8iZ?SJ3EP7b|(_Ce+?SX5,,Ko~`>P$79mU]vpFW,8[Sb{^7>v\s&G|;8QnjGP7<y[WCugL.7/).832==y\R)PY/V;&z@A]hCGC?1ilNJ&0G,qQ8z|gHgR<aVoNobZgRN)
::Xuqf/C9}|~(n\R2t9y[67?mM,p]7{BYn!d8d7f2oRIokc5ML%y}YL|9LK+Zmn}\+V1mYSFKJhC&)BA9!CAr<ny~q%8euV`V7/+e#aj(}*PzvyEGZt\TvFh;jpIy_B(oip=
::bLjhv@ioe$buaXgmITihX`?$[XHG|y/pfqm7;eK5hlhQY+%CnKZKqxIt)gb&w23E?W>y0el@VjQE.B{~T}rapB!`5RH~hvQ|8zn;@aY7jx-)R,Diw#3i[xV\}~iUge{Wx@
::vy*yzn<0gRfoS7*FzkrO-%hF=a+ew}Sqi;~L~!QMLp#%H1hCbegP82%1rd$l6h0sP<\dK14`-4k4W/WQX]sEfJ#<<Q[]?IVAeut;2}_(,Ky?W!Xt-J|&>YpLSO$T-LH,dH
::R`7Hj$_E_Y%th0MiWYk[o)y-!brP%ips~9qjr1P(jNH1#G19=#&\(&`rPVd0,g1y)&h&a<;Gyd|%L0d]|ii9AH2K,e6K39UZDFQq@b2;5/2m@szGQAi2e>Df&*9a1#]\!+
::V;eIe$&,cn|-.04XFN-N~ZP\E{WUyIAP^d4Gc&GR$+>>hNFw;HaMk9F54Tc|ZU.kB>1PEb=I=`T`Ji+uMO@v;UzdvJKLM^nC]#*hcId@){MAJez,QLQt%!miv05_e!-4){
::Ak=jD*RYRd9wx[l7OI6D-TXk(t|tSIuZLz^z*j%?eX,}fUpV6DE7nKW1L~~5N$cdC=xn)j4E>?kPzkzow;Kn&\r5qu$wZgh[j%9jY|&NlpR~{8w0|`f*n|214vqJ&0?z|n
::mpM%rkHXXQFHl7tH>Bje.pZr<Ia@($1}_uZwe&HNg&Hi>?`R)~9cLarCBQlcl#mH(k|.GE.<#IyL{YiWCvj#jRRnxbcd!*5`AZy$f<VROg44a_Z2Q2NLJ?$2ex-uH1myL9
::?y6vCg=XV@=G~L@M%1KM,4MGDP.IyIP%vq&nH0h8?4tRG1p;O<G9xcbRnjtF|,`bnGMiI@N4?M[Dtp%Q?UX*Icw$7WHMJzdgI}?FUT2joU$2.ewP1csR33,s!Yx_(Awh\y
::pe=5$77a`o^ASDXTYVU*0KHU^29}DV}>%vyw*mu#?3,7&oIdu}hH7R<o?|8B]d5#kmG1V);-9/j|o%fcDRmfM~Nws$bT.\q^]Q_,^J3s[LKh~C_kh\~7>l4)=.$rnS]QzP
::vdYZ*_*EMCxz?j(rr~-5ad8c>8d{ovL|j9u]KTtn.QQ[[IoBMmDYVInm3;BW$@2J(LTxTyZW/q@Q[<44wxE(|P*x0{`_sKW{*^vBlgq9!uqhc_B/UG;FPbK(pXI^c8g~C`
::U&NUr4WgmS!n}gQV[Fw$H7LBs76/x-)NX|>%&zixmv%@@ruW&M+Z/2MGlJLc9`=x_L?t7I`?Jbu5{q+f<y_ollrcOF5z#GME(9+0]_;ob0XVKGeo6,3H7+-bWq]e&X1-pn
::$qHZwA|I\LnnTtN;|K3%S{>TaUAaxx_`[1ok}lBt(w&p|jG{OSJtc<(48dp@@>XSADmobs;?$yY`}e?J-LL&s}W<^Yao8+HLY%5,|cnj)IW~}6*T7z97qBK(6KW`\V~B*)
::f^MesJU]lB;A]ALZXZrR+*sYeSNCJk~N#[/GV]qL0l0K|PuIL\HX7gF8GeQ>>0oZ0A@`cq4)a1f,Qw2K[UY(3$!Rhf+4k\L_fM0PLc,$w2Af#K>%C?(<gK0!Ot5wU(~o1~
::EvLUf9?&`ah,FjXb[kcx\65$WWr|t9uUTdWH4=Ps(PXXstC-0UsaUK-t$T]u&1}iba/{koZ4|VAhiJllxYu2!sjMt2MP?L1@pr7Wqu~W11rSf<-$kF-V>,xnSj&$S{+A2o
::7/>QcO*g.ImSx*r-<m5@,pcl.dGb1B+[E3Y+.7gCS(w7?gfYraKY/J2NaE6>m<&Ry&hwKn4)3f++`4}^xtDORQNs)jGS#AtbD%/rO7hD/u4C1c+oCB*8,_*M69HD1d@Osi
::NomaK!,>oA8ylU>vW=`)jHz#52H<;+5En!Wd39|Q)@P)z\*&]\of9i)u~rE@(55Rij>_@B){Lo!+)J|\*A*W&p4ehL2IeN@Zug^0d$B8.sB8duqpM*hd$R$IuiB$&,EO1*
::_\[M0**w5Of[Z#^#=)zog`aI+RMD7@(QVk,1luZIKAo<?(?DZa]xDj!lu;x#dwx\.>?PP\9jXX`u`d1J@UIJlPVB8hUNh)DK^pP_w}C&D2Ie.H(+v^[Kx,jIh*d4prGdrU
::xaeYLVy]X/iYN[xUsRd*7{RR8JW.d,
:bat2file:]
