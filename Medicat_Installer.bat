@echo OFF & setlocal enabledelayedexpansion
title Medicat Installer [STARTING]
cd /d %~dp0
Set "Path=%Path%;%CD%;%CD%\bin;"
set ver=3102
set maindir=%CD%
set format=Yes
set formatcolor=2F

REM GET ADMIN CODE MUST GO FIRST

:admin
    IF "%PROCESSOR_ARCHITECTURE%" EQU "amd64" (
>nul 2>&1 "%SYSTEMROOT%\SysWOW64\cacls.exe" "%SYSTEMROOT%\SysWOW64\config\system"
) ELSE (
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
)

if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    set params= %*
    echo UAC.ShellExecute "cmd.exe", "/c ""%~s0"" %params:"=""%", "", "runas", 1 >> "%temp%\getadmin.vbs"

    "%temp%\getadmin.vbs"
    del "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    pushd "%CD%"
    CD /D "%~dp0"
:-------------------------------------- 
:UACAdmin

:pwrshl
if exist "C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe" (goto lang) else (goto pwrshlerr)
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
:lang
call :binfolder
echo.Select Your Language (only changes some things so far)
call Button 1 2 F2 "ENGLISH" 14 2 F2 "Francais" 28 2 F2 "Portugues" 43 2 F2 "Deutsch" X _Var_Box _Var_Hover
GetInput /M %_Var_Box% /H %_Var_Hover% 
If /I "%Errorlevel%"=="1" (goto en)
If /I "%Errorlevel%"=="2" (goto fr)
If /I "%Errorlevel%"=="3" (goto pt)
If /I "%Errorlevel%"=="4" (goto gr)
:en
set lang=en
goto checkwget
:fr
set lang=fr
goto checkwget
:pt
set lang=pt
goto checkwget
:gr
set lang=gr
goto checkwget


REM GOT ADMIN. NOW CHECK FOR WGET, VERSION, AND BIN FILES

:checkwget
echo.
echo.
echo.Selected %lang%
timeout 2 >nul
if exist "bin\wget.exe" (goto curver) else (goto curlwget)
:curlwget
echo.attempting to download wget using curl.
echo.This requires windows 10 version 1703 or higher.
curl -O -s http://cdn.medicatusb.com/files/install/wget.exe
move .\wget.exe .\bin\wget.exe
goto checkwget
:curver
REM == CHECK FOR UPDATE FIRST. DO NOT PASS GO. DO NOT COLLECT $200
wget "http://cdn.medicatusb.com/files/install/curver.ini" -O ./curver.ini -q
set /p remver= < curver.ini
del curver.ini /Q
if "%ver%" == "%remver%" (goto winvercheck0) else (goto updateprogram)
:updateprogram
cls
echo.A new version of the program has been released. The program will now restart.
wget "http://url.medicatusb.com/installerupdate" -O ./MEDICAT_NEW.bat -q
wget "http://cdn.medicatusb.com/files/install/update.bat" -O ./update.bat -q
start cmd /k update.bat
exit

REM == CHECK IF USER IS RUNNING SUPPORTED OS. OTHERWISE WARN.


:winvercheck0
for /f "tokens=2 delims=," %%i in ('wmic os get caption^,version /format:csv') do set os=%%i
set os=%os:~0,20%
if "%os%" == "Microsoft Windows 8" (goto start) else (goto winvercheck1)
:winvercheck1
if "%os%" == "Microsoft Windows 10" (goto start) else (goto winvercheck2)
:winvercheck2
if "%os%" == "Microsoft Windows 11" (goto start) else (goto backupcheck)
:backupcheck
for /f "tokens=4-5 delims=. " %%i in ('ver') do set os2=%%i.%%j
if "%os2%" == "10.0" goto start
goto winvererror
:winvererror
mode con:cols=64 lines=18
title Medicat Installer [UNSUPPORTED]
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                  [91m%os%[0m                 IIII
echo.IIII                   Is Not Supported.                   IIII
echo.IIII                                                       IIII
echo.IIII            PLEASE UPDATE TO WINDOWS 10/11             IIII
echo.IIII                                                       IIII
echo.IIII          [91mINSIDER BUILDS MAY HAVE THIS ERROR[0m           IIII
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
reg add HKEY_CURRENT_USER\Software\Medicat\Installer /v version /t  REG_SZ /d  %ver% /f
if exist "%CD%\MEDICAT_NEW.EXE" (goto renameprogram) else (call:ascii)
pause >nul
mode con:cols=64 lines=18
cls && goto:startup
REM -- WARN FOR ANTIVIRUS AND CHECK FOR UPDATE TO PROGRAM
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
wget "http://cdn.medicatusb.com/files/install/%lang%/motd.txt" -O ./bin/motd.txt -q
wget "http://cdn.medicatusb.com/files/install/ver.ini" -O ./ver.ini -q
wget "http://cdn.medicatusb.com/files/install/%lang%/LICENSE.txt" -O ./bin/LICENSE.txt -q
set /p ver= < ver.ini
DEL ver.ini /Q
REM -- EXTRACT THE 7Z FILES BECAUSE THAT SHIT IS IMPORTANT
:7z
REM -- CHECK IF 64BIT
if defined ProgramFiles(x86) (goto 7z64) else (goto 7z32)
:7z32
wget "http://cdn.medicatusb.com/files/install/7z/32.exe" -O ./bin/7z.exe -q
wget "http://cdn.medicatusb.com/files/install/7z/32.dll" -O ./bin/7z.dll -q
goto menu
:7z64
wget "http://cdn.medicatusb.com/files/install/7z/64.exe" -O ./bin/7z.exe -q
wget "http://cdn.medicatusb.com/files/install/7z/64.dll" -O ./bin/7z.dll -q
goto menu














:menu
cls
REM -- THE MAIN MENU, THE HOLY GRAIL.
title Medicat Installer [%ver%]
mode con:cols=100 lines=30
type bin\LICENSE.txt
echo.
echo.Press any Key to Continue (x2)
pause > nul
pause > nul
:menu2
mode con:cols=64 lines=20
echo.  %installertext%   %installertext%   %installertext%
call Button 1 2 F2 "Install Medicat" 23 2 %formatcolor% "Toggle Drive Format (Currently %format%)" 1 7 F9 " Autorun Patch " 23 7 F9 "  Visit The Site  " 49 7 FC "  Exit.  "  X _Var_Box _Var_Hover
echo.
echo.
echo.
echo.VERSION %ver% BY MON5TERMATT. 
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
	cls & set goto=exit && goto autorun
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
set goto=askdownload
goto updateventoy




REM -- GO TO END OF FILE FOR MOST EXTRACTIONS

REM -- WHEN DONE EXTRACTING VENTOY, TYPE LICENCE AND CONTINUE

:askdownload
if exist "%CD%\MediCat.USB.v21.12.7z" (goto warnventoy) else (goto dlcheck2)
:dlcheck2
if exist "%CD%\*.001" (goto warnventoy) else (goto dlcheck3)
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
call Button 10 14 F2 "YES" 46 14 F4 "NO" X _Var_Box _Var_Hover
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
echo.IIII            BEFORE ATTEMPTING TO RUN THE               IIII
echo.IIII                   INSTALL SCRIPT                      IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.                          Press any key to bypass this warning.&& pause >nul

REM -- INSTALLER

:install1

if exist "%CD%\*.001" (goto warnhash) else (goto install2)
REM -- IF DOWNLOADED IN PARTS, ASK USER IF THEY WANT TO DOWNLOAD THE HASH CHECKER (FIXER.EXE)

:warnhash
title Medicat Installer [HASHCHECK]
cls
if exist "%CD%\*.001" (echo..001 Exists) else (goto gdriveerror)
if exist "%CD%\*.002" (echo..002 Exists) else (goto gdriveerror)
if exist "%CD%\*.003" (echo..003 Exists) else (goto gdriveerror)
if exist "%CD%\*.004" (echo..004 Exists) else (goto gdriveerror)
if exist "%CD%\*.005" (echo..005 Exists) else (goto gdriveerror)
if exist "%CD%\*.006" (echo..006 Exists) else (goto gdriveerror)
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
call Button 10 14 F2 "YES" 20 14 F4 "NO " X _Var_Box _Var_Hover
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
if "%format%" == "Y" (goto formatdrive) else (goto installversion)
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
if exist "%CD%\MediCat.USB.v%ver%.zip.001" (goto install5) else (goto installerror)

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
set file="MediCat.USB.v%ver%.zip.001"
7z x -O%drivepath%: %file% -r -aoa
goto finishup

:install6
7z x -O%drivepath%: "%file%" -r -aoa
goto finishup


REM -- FILE CLEANUP

:finishup
del motd.txt /q
del 7z.exe /Q
del 7z.dll /Q
set goto=deletefiles
goto autorun2


:autorun
mode con:cols=100 lines=15
echo.Please Select Your Medicat Drive
REM - FOLDER PROMPT STARTS
set "psCommand="(new-object -COM 'Shell.Application')^
.BrowseForFolder(0,'Please choose a folder.',0,0).self.path""
for /f "usebackq delims=" %%I in (`powershell %psCommand%`) do set "folder=%%I"
REM - AND ENDS
set drivepath=%folder:~0,1%


:autorun2
wget "http://cdn.medicatusb.com/files/install/autorun.ico" -O %drivepath%:/autorun.ico -q
wget "http://cdn.medicatusb.com/files/hasher/Validate_Files.exe" -O %drivepath%:/Validate_Files.exe -q
cd /d %drivepath%:
start "%drivepath%:/Validate_Files.exe" "%drivepath%:/Validate_Files.exe"
goto %goto%




:deletefiles
cd /d %maindir%
echo.Would you like to delete the downloaded files?
echo.(everything in the folder you ran this from)
echo.(%maindir%)
choice /C:YN /N /M "Y/N"
if errorlevel 2 cls && exit
if errorlevel 1 cls && goto deleter

:deleter
cls
Set _folder="%~dp0"
Attrib +R %0
PUSHD %_folder%
If %errorlevel% NEQ 0 goto:eof
ECHO Delete all contents of the folder: %_folder% ?
choice /C:YN /N /M "Y/N"
if errorlevel 2 cls && exit
if errorlevel 1 cls && goto YYEESS
:YYEESS
Del /f /q /s /a:-R %_folder% >NUL
:: Delete the folders
For /d %%G in (%_folder%\*) do RD /s /q "%%G"
Attrib -R %0
Popd
exit


:hasher
wget "http://cdn.medicatusb.com/files/hasher/Google_Drive_Validate_Files.exe" -O ./Google_Drive_Validate_Files.exe -q
start Google_Drive_Validate_Files.exe
goto install2


:medicatsite
start https://medicatusb.com
goto menu2

:updateventoy
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
goto %goto%


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

call Button 10 14 F2 "YES" 20 14 F4 "NO " X _Var_Box _Var_Hover
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
::AVEYO...Q53(......*D........j}?.R{..;>H(..k]..d,qGy+#?........Rn7cqOb}wR|tPF${5ZD!d$sOkWR5[]..Rn....$hU^L-d,oHTL_X5!d$7ihuSn2+..fZ
::{...$hU^L-d,oHTL_Xo&}8V2BlU=EL..a...tH....,zd68.-.2c!T]8HboU[@+8?XC@M-EI+.;>^)....7kFB[.;P/v<qYbju;hvJ_[U]C.>%>aDx?[NbP.%wt.d|J;,P
::ZrSS[.kk>d`xo?7A_ifwF&`}u60h9l|F>~(hKL/002[z(h=veCq@HC,o,mvM?zM%PN*u-~-Y(Z<_^ynXzuyRg]&H,J/{xqDujAr>)Qhx^;Ou~U7B<pEoQQ<$<_}..t>!-_
::..W-s^HKiV^v80cdl$#`Q~`?O|V(9rSw_CEYyLtC8`A=EabM7<f4T|Y#v#kz+WV`P6zB}W${,_d~4hSM>1>;NH,N|z6k^AEtFeA0\\tRa}26..YQ5D._{PX!I!EOpT8F0L
::ty5.iY=CKJ(f1_}4.DRag}_-]*8P}LUnSmWppO!17>IO1)*=Z}p9I7ZOt?jK(GB]B3NIP$wz-BbV.Y_C-MyOC>f-w!F.Noyc=!~?b}ko5[}le%jD^.47i=mnpnrujtqP|z
::`(oVJ/Hv3MBn?;QsH)(XI;?GE\o\}*7^s]Fo*@d&O7oEdG_)24eT<xqO^Us;X*!@sb9nN+}64TzERi24;ll0SO_bCNr+tVEw7CwaX[v\W+sE5)i\e8GXr]+](@usfYIw4z
::8xC[@Hb0/gm`yXhuU%mPL=sk+)Lj9~r-.P{D8tNJ[r~U+qlHbYC5Gqq!6jaVF[y[buA-4#DFZ+CGwPY{4Hu0V1oPD\$)[s^{)SomH4d`-.Ojl%Maf>eXGUuSv8Wv[$wj`S
::1\P|r~`rG;OgZosTQ~}t%%q`q{[P>sy@UZ6[vL1Nc?C2bP[|Do^3;-eY\hm8hs/FBn1Qk<)707A@DL6SHhCLaR8FQ1t;+v$O6l;\IM0Ao7JTZhP{w;hm(0Ba=k(MIDt`V#
::2IYkpM;.*@HPdt9%Wal3NzO{v3cD{j@}UUynmJ<wfZmY)~n6N<$Q(*z70%{8[[T$;pD%K4vCY*8=KWFI|aX!-Lp0!{duj@odsP9ZhuM%e!VT9]LQ@r4wLncdD933yv3)F,
::sT_p&2b@h[DP]ho[dBs4u0Gp=%pRmSY<>FTgM3~n#YCiVG[#}/cM.wgUsj*ry_|2^L3X-zwsKGNg;IgTWU!4#IQ+T_R=6$Xt7Fo.ixuLt(nr<;du%`d6|dhAbod8ZT+Kja
::O+@wpK<VMLH=RKw)vtlfQV;<_Yzy/YB6&Od.6Fo#P_P[V{mP$E|Uo2KmMX[V\>qmTgjVS%m}aH,ugT5*hpp,8#N&%sW@`8]?HyF/B+?7=rRQ]@,vQZz{eL,;Z*JuZ18X[S
::*XF1V-}GUm([>#Y5,i%u)6Ojd0xaAcU,i5^eUSV8_.Q#ZVy#T#Y0E24>k.8p0/<,48?z]2{|)OTt8U-qJB^4rk<O@6Y/qD_sd;$}4~Q5no59o7ke9^{7V2fK;JxtabnOiF
::V#qAsCWc>QEYO8VWd^g1pEz7&7~yx,p*xQuKZ>U{QQ9#<P)<YpKKdcrqx!$aPzK~lTd/Y|Xynq(ViYq~g?;srJ`Tm;D1O^.FJ-Dd5djqpIj7a9k)\qM<N>bx,![SR;K+N[
::B9(,43@B|86!sMM9sa~8@vLvR37AoW+GN4T;.+d]~;rtzu9]6)g5APQht7q[vzjHrM.[OR@]?Hv^)2EV1o_$.Fy4>z#2&NKp8^&Mly|8EA<YBRE>OA]tfRN{MC]/SHCmCW
::Wz!k[6NaUq>uE+\HFUUX532Osx4fQH1ID$JBV/efOha7&rlMWvMzvPSkOp@4@,ubvB]Us|zt<q~eB4IUMc\l~N4e/&E!k^~|ooQy#536Sdq5LspQYS;b_sD];BIIb|,ygG
::$Y.tJp/+gKi=nRTqt*P(RBX)A)4@oz#fX=YvMXx!Usw,{)CsN|SANv4Mva*1$8McsM+OUTV^;]D&5[/Mb,GLB5kLJ-=lZ+%<?Fe=W6)m#G~H6P(2M2*[bx3;{Bt6CF9,B,
::Ym+{~qS9Hy5}4J!g!>W(sefqWjx_Mz/NvB>(|}WVut*fEPl%{0xu.5j^$~jcYy]grC@akBNRt7[,UG]P(LrAS7iI)c1hp6DSN2KyTllO)]=[ZYPL.%wrH_nqMDR_5ap{LV
::~k}3dIgI+{NQVSX\?1q=(lp%9Jlx1c~a}(k_T)zP[2/YS;%VHok@}2SItp^a3>T0y*1k&=[Q03F{t37H]=K9F|Of_hU^|UV0C`&B.C8^\0usrn<(,D43vfi<bI_0@U^bU]
::25=U1d?&nw#N!7)OTXR@*FN-CB&XpQgp(bfBY*c(ktELHCWVrsE2jhN%HdK^@_VR|FsP[P.td40?Z;Xpn3O=[PCp{Bq=`W<u@wXer?#1Ed5c~e?j<3bqn-8S6{hiC>3db4
::m)uX)6SJQb.^qK8hk2i?Ey]dMWnu{Nk8Z~9i$|BP4Og&XnW,3MyyEC(sVQs>.DaHB[M]p6\_7cpyUHd|/l><J30CHBdVkL_75/|%6_j+s8\?E[{i<(x<P97/Ux0FlA,0A#
::#8u}$_.Rq|=Y#g<G[X~QK9Sn#No4`~GQZ[0^$BK9~vd0S~gYahJ6?\rvSM<`i~=+/|[\?0C<Dwv-(aR<_)C_{YFpO#;XZ}-XyjQJUr^@tGL9W\6f(?}F*)Be;1vg1F|VKF
::h$bVgB6LITX|L4QJ~]TBB`tX/hzv}kaaGh)|xPHxRwj-T-e_h1`p\o|DeOIP#HCih]#?^}OmQt=Cq}~y~_$7?-bnsjy\fH}qegE/+08m%]t!!qDK!1=|F.c>[wrFG;%e05
::X@db6^|PumCrPrSz#v>\;JM`d8`|-mpgql8{jB(}Oc[d&;\A=k68H8x,-`!79-=yQMP.<0L9hHd6<PI=6Ob&azw=Sg=_.rlUQv9MPWzH&NhbZ-+1[7W=7Xc7WYr~=fhE7T
::;aSF`h)~_DOYYmUf`z7ck,?--*=[}]1i$(R*j4VxV|TYtdPva`OviWT(u]mb&gV|}nTX#sr3ns*9^(nD|K>93}fBI{NHAa@_bHMP8do7?i+duAMn.<_ou_B6y%79\VwT-O
::{a_tuU`H9E^Jb9)Us2%sT~gB7scE%6sSw*FnTL;Xt~Ml,#Z4@2&|l{V5HS-ja<;)KRY!O)Z0y|2It\<}tLE@TnQ-2~ChGfzjjwc-K/ZCTF,mVPLb/>t^Ey*WgxBFFK9L1i
::2uMDfK.Rq|OI!~@=\j{]Q3kI>w\qO9\rKH>kcVv08]l4HUY=k8%7zN3FUw)`X.hw!nC#Jd,mj]~7\qr\$sd0xc2Sti9P84jcHxSrXMn,_#ZoM65X;,vw+@FI\rmdJr1)QA
::,%tw8]pqma@/.ybxMS?-fG8xLQWOl(4]G;~QBQlJY(mOq]-nYt#pLW>u^$KPk(?w2W7&)8/)TP)~GzUsM098/^HF}-G\=EgpG\a5`$9PPAE+afLqg7o91cV7^_o6$WuT0@
::ptUXx9!B~Ew2;RNG[l>f7^u~xz@g?Y`^a`U3m?qp9A${YoNLk<Z5aHEKm)w(I$/[(qssyi`1qgjm@aLtma._g@V?!>m@{hy7c;Kw,5pgb{gAp{aUzel3~~kY&5CCq~5(Cp
::Bsq3,m\VJj}=3OL{ZtoT\K[n6_qo%2qSo8!cOwVH[x0dO%uZydHG+E$,2eG__bK?;)#l?2p+;^Vd1?irb@[LEFKlE;,u>.h3lh>m7DQKl/>LGr>r_^F<%|E|Xs,WFAnIwA
::T6}`+8bSKS`2E[m,oe9CpfR@bnY(AS`<`l5g2Ctw5TeDQs,55ApR#*&yOVUd6;1fDrWyfH*R)s*E-$(d;!k*EgZ]_k)J8]S-mgw}~BiubtFQMD^Oy(3v*3*U*ag`rVQjE=
::59LMM)9kuX.,u12VuOnW<+{G&;H6Xag*~]tApu+,e1qL`lvV*+Z-Y4RZ&fyxiezzMv]+3C7qmC`epW6sRtrBp,D7NorS\W~$h9+Rx@5B)VZ\|JzE3t\7yXM5-##fAu1y61
::4[YcP`D62AuKt?M$4aEXv+M[#aE<4=4x_R0>RGCi7[3zkM|GTeQpVFnyI$rf*_`V2U9u\C9C@q3tkC+{4RMN|HEFpn%<BjF$,A5GA<0wd47+C8h1Iv~83zpRW#E8TA!B,l
::~@VhUyZOMQl#dlH%^MnoqC!.V3Bzvx6+Ru5oZ-lymO<X/d3D=X0q0j^gzN24{~QL79JWsf;`{eKi,N&H=MGG-VXVBsmkU@3XVaC+wVxqH}FfGb0D43s=B#(%dgFtlT7V}]
::pqG_?ftjm-&!,jyG-vSm41t^Q2r*QouBz5WZBnCd^WM1Azf;tSL~ka5{RF=B}U1Z<@<yv^OHNY_q=&p15_I92[[)@o2khm2_AD3[SyIhcly7+;8JtsEgjp{y40R)9dA`=U
::Z_?Uu>)jUZi}q&zN,,4en9#~rG9|6U_x~lxC@Fwow=lKDEQT{$59]5r{=fbq(Pw%yC&r,GfT$fu@UqCn#I+!KeUmQdgK|HL*=E\XpcP7nj$7k<)grxAOu-g&^m+M1CBF?X
::)F;*#~M.I?.mw1=cBeBcRUmdy`+CwJ??6#2#xtprl)}y^5VRo{MVR;3ixGJY#iVk9\75_.95o(!%$HJN9w^S{h-[|}sNo8Z*kK>ZgFo#roIht02H7=*8)W`nvV+v~D=x?o
::VO)!cC7%wo[X<\GF#C&+FaX![4~Fy/HP.y+1{GWfg;MK$dk*Y[6b}MA,BH]o&2E+[s}oLslX7PCIHdoNcs_cJa4Hb#PH&O~pG(~q1(=v.D-HeSz(~[b9@JBm>h!g$DAdf;
::w2\+_OEj<hhA|f=J1^1CC[7J)D20Fc&[kt94*V{Z`L?z@5qMD1$2k7QybA%h-b>06;FfkL#`P$D,m\4D!BYFmXWT|v#vFW4o+P]QVr;mUp~R`BKHHV\iwQ(F?]QR;m6y&v
::p`%iW?4K;`)0RfG/!F8g~r]HeACJXyySPL^MQ*3q_I6Sx%HP-oc_A\{QRA$~sDTVopOHm6M_/WlsUOIF_!I2e\r#zv~kV(\a!;xFLW70~-i-OQ~qri^OpV,)aP1QS-+S0T
::UI|^,~+,dL`QC~OJI?9X0Ss[&z1&_zmB>|%kXpFN@<=1K)?Q%J3dBgPm@iW|l@hzYLks^.<Q+9CdlcN}xF8;\[~Kje=p\O{^y+zE`_F9X%Qda9\7xrC5F|fu;wom9AL#O_
::ukFAU4@lrqWxL.8I}k~)pc%EhR?gPo|/iw8Y,~QV.M!@B!NHz;3Re|J}GRe}+6W4|ty3.0U`2g,]aZg]OxdnoqA_V<hiCVKrcEAhF_sShdE,@o_5p{7PEJ&RV&Qw~.*4n*
::.xdc~d?CNy_CM=`rjW.1Jw6fNsVdRtlO?fi;m3sMEYOUtA*tBC\x>\+H1{j2wTvj6Rx~y|-O]\&zljf+BG.7dwLo<(Fk;l[978YM/#=9n#0rS1yh*M$N.]+HO?M\T2.e_(
::`f.4oYhDIAZ^zEdz7Emrx5RR&3Q#\Q<t_S{SG7Ruh4dk@PGpg;;+NTc~{YRwp(elhih%\j{W%Sy}X#>}/[Gvf/Xs(Z/QlRp]R+D~g`X(Y7]dTLUfvhG_q9PFs1QGy|R3nt
::oam]*(Dn(NhDQ,}]a,hshT;o2u7IMp)-+!7dXA9_hL6a_uxMP;,kiLe5?3WqYG/f[4/D(.4-ycrvZF1tf+=)HW5,Ex8R!HgPNM&mE,.i8DY0@<I;$%(Mdb?3ShIdS7QqhN
::Wx&]XPYK`8ojFor.wKhWe_S/K6d,lc_B7{#!|ovUS!Jyf}Z%/ud%sh_tTr=+=cYjOAndz=ct\r~z&7uKE/wxWB[U4P+<%h~Jed6q\^oG^|A[QTa823ONdiNnUI_V;nrqCf
::wV{a/BI`\~A\u?]VIQd*_55ngr]\JmlN[V_\ZLXAW@`B%M8PGIc<,W,T4!`Lrwh4sGgQ2Iy`2E5`hE/U<Z*zr&_u-2AjtdK`T91_,5)ixR#8UbsF#JyXzYG.h%U6DWb;eR
::d]bsKNdZ#o-@Fjv0\i~hq<;Jja%\@w@N+`2&<0-,K*U4%|pcVz3Ot*V%.YWGgJdoKXG5-&=KB$&XMAy!|~2RWb^n@yntK7A-6Rq;w/VO64G,Sp4~3XI,j<u1>S&,0Dvqdq
::[]u/`Tens=l$au.Ry)fDtAZ!pwc;_\&_eFR0v*e4{`XJBlIF&ostZb>*#*H]>*p}!syZ*8(!D]ahK%*tK9ku=`8`h%dnN;]X3lwk(W;#CqLnR_-9RXP_;yu[^gIOwd`n4%
::<)xJe`qfmn7qp<bg|l~*cR2hKn{,jdaNEv/yM%ykNZR%M4xg`$s&,[c+$Dwj~5\zUO*L_\{)9+OhFz9]vHj;6)9CZGq)aclmVWDF$$PkL>4oa=Y$ml?Eh;6r&UhL\r,>a*
::/MEnH3goTe2Da2@v.2Y07Zj!~f}mE\2H9n5bw/C42o[QH3=l.\!omxg0q4;^PlZ\9^8qu;ZjfP[vJnNHA#AtVtw=Z;/N!R0ETtz1<\j8c4Dao]ci-f5\6j!fQJB3y[?cnk
::w-F=1X}p>0~<C<;kJzw{Q@E)kmiV~AHYX{^R3;_+YY;-{FC0;sw8>adhq|\ypn+Fm@tD#uDE9/w]<Si][4%TzQv-Vx~_0tcns;?N8p/0.\e[I@rMk0aAR!|0th*2^\>.H]
::Lhvo7JU\,HtRArSzVCB<h!9l/TG,x]V]5[tUM^s~fxU\Haj15a`I^+UA8+P6O6m[V01hk8E*onA\dEr>EQBEg4Y`%5$w`\fXV]S<Epr0v4|22w5js.`n0z`oVfE&h1OuBz
::^`?ZbvPM3&!zmkof*d6E|qvQs%)F%&xhxNLt(aRsRI\a)6MvG9=Ji+/F&tOE|3?n^5<I8=@~gRs6p\l~8ZQ`^fEq2bgGxh%4W;}rKnP8GkM\.5N{N\a/i9GyWeuQ\H9\|n
::3y$tj#Le*!00btRuT*7$ShJo0Ki\(CejR?PS[Z}BzeuyDYtm(_Rv&\e6yJ5$+Z5`Ys1.!wjGiC=!BA$>!.~IpbElng4@S40qlziAS\GoNZSC}.bHdzf[YT5$Ov;8hk[&2v
::P?ZlN3v`Ti7<Ya$DELz0ZJ,*N0f<w@g4V~k$NY|`I<F%KdXEaqopYLB`PET`#v.icpmuvjfE;yd/a%[yP5/bb?RD@`_`E(q^m9Qu\qP=7u&Qh[vge@Z$}.UCE[4wMH5iw|
::D=|Z>*R@#b@P_[qGTVQ5i#i[7Ud>Jy#=wt!*3jU;%84EOY0FrN%dQ`%caW=FvNSG3Q5$xMV@f=z#BF3V1Khk,a<]9Cv3bJ#@@LK0!(ifq/]4P~pz?NVAqtfpWN#rQi6VDi
::K[mlbf@~Nvk0D-.$63!Msjb*>0|z)?Ty+Wj3(JD!*Hr(Q<9&MgCJ%|,xjDLfU$eFLy]u#+DA(*?LS=WNUz#ERZH@Pdkb`%I!Ar`j%d64w?PYLM\.)DDv0`]d[BG#0m=C0K
::6v__aM88f4@c.L`!&.uq]582k1JS5m9m<IQ,c7=2z*VOp_}s{JB3gM9*]!^C]O%]J=IRoLn?`2t3D?#,QrR82GoUmwXkuZ~xKY[pG_GC@eVznXrZo(kVVOFSfO{g*L#8Ho
::5XIlEztKJF,HlKXUQ!KH&0g+ak/UbBk$E45Duf5Pnr[^J{6nTS!=bNVfhq*d^?Lj%qa,fWRqf;Sgok>zR^jbDV#}?3fj1DTJ9]@SZB;OFF{H_jLym&KeaG]#ZDa9bz^F8P
::R`\#z~jvJhY|)fnzT^\l6vIb[[rwz/7xc&S3,ecmh$k`T$,v[137o+K7\U!!1NjBB=X+N@L+l{Tu4y}A^18+ONK+Dpq{fm^tIX6$H*dZQUh2[lkR/*d8~Hrq1plU<WC2U`
::02vA#U>%9|1R!t@s]Jy{ygLl<*hxgn_mFnCYsCC<ht2Q.44hr(qXmL!k&N|Q7z(>Tx,[IND)UeNfCae)P7\GzlRUJDcX$;nK`N7hSZH,=lk7A8AL}0#`#_[#So=kJs^_-E
::`a)E>;hT,d24k@\}kFUbZkKRP5l&xxHI#$+s*O~DC`3jV#]K/A3fmaIc}RiZd[uw&)EBV3x7aU$S^hZT-*]&V9[!mm@Bgo@kWsb=6tYx!~*;]P)}=h12il4ySB7}YAj>7l
::x-e*)}%#*+F9`qEC{MJOloBGL7;9TBNY$eCLsm1zd7=[dagN,&8f.5wy^TQH;S6u//jp(e+F7JEmvOo6MOuF#rsfc<F9t85c=vGJG=BY[5#jQ=nvL|ns?SkZhZE_uylxPk
::AgnS-+nX^yr_bbUi#knhvO*(*nDSrK!b#E.xOhL#c1}MaI,]~m\gaD)N+>+
:bat2file:]
