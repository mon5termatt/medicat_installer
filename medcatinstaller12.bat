::[Bat To Exe Converter]
::
::YAwzoRdxOk+EWAjk
::fBw5plQjdCmDJHSB8EszKQ9oYQGGNUi7Eoo15vvv7uaLp19TXeEwGA==
::YAwzuBVtJxjWCl3EqQJgSA==
::ZR4luwNxJguZRRnk
::Yhs/ulQjdF+5
::cxAkpRVqdFKZSDk=
::cBs/ulQjdF+5
::ZR41oxFsdFKZSTk=
::eBoioBt6dFKZSDk=
::cRo6pxp7LAbNWATEpSI=
::egkzugNsPRvcWATEpSI=
::dAsiuh18IRvcCxnZtBJQ
::cRYluBh/LU+EWAnk
::YxY4rhs+aU+IeA==
::cxY6rQJ7JhzQF1fEqQJiZksaHErUXA==
::ZQ05rAF9IBncCkqN+0xwdVsGAlTMbCXsZg==
::ZQ05rAF9IAHYFVzEqQIdLR9dTwWWfEK0FaEd5OT+/Yo=
::eg0/rx1wNQPfEVWB+kM9LVsJDGQ=
::fBEirQZwNQPfEVWB+kM9LVsJDGQ=
::cRolqwZ3JBvQF1fEqQIZBihgbSiufEafIpw/ydy72tml4no0YMp/X7ry/pGqYNAhxWjUQaVN
::dhA7uBVwLU+EWHGwwHIDclQbYSGmFUibMoAvyqbD1tDn
::YQ03rBFzNR3SWATElA==
::dhAmsQZ3MwfNWATE3HYEGCgOA0uvE0XvMpAuxcnP26Skjng/NA==
::ZQ0/vhVqMQ3MEVWAtB9wSA==
::Zg8zqx1/OA3MEVWAtB9wSA==
::dhA7pRFwIByZRRnk
::Zh4grVQjdCmDJHSB8EszKQ9oeCGxCDqGK7AY4ev6+6qurF4JVe4zfZ2V36yLQA==
::YB416Ek+ZW8=
::
::
::978f952a14a936cc963da21a135fa983
@echo OFF & setlocal enabledelayedexpansion
set ver=2006
set maindir=%CD%
:start
if exist "%CD%\MEDICAT_NEW.EXE" (goto renameprogram) else (call:ascii)
pause
mode con:cols=70 lines=18
cls && goto:startup
REM -- WARN FOR ANTIVIRUS AND CHECK FOR UPDATE TO PROGRAM
:startup
title Medicat Installer [ANTIVIRUS]
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
ECHO.GETTING REQUIRED FILES FROM SERVER.
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/curver.ini' -OutFile './curver.ini'"
set /p remver= < curver.ini
del curver.ini /Q
if "%ver%" == "%remver%" (goto cont) else (goto updateprogram)
:updateprogram
cls
echo.this program must be updated to continue.
timeout 2 > nul
powershell -c "Invoke-WebRequest -Uri 'https://cdn.medicatusb.xyz/files/installer/Medicat Installer.exe' -OutFile './MEDICAT_NEW.EXE'"
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/update.bat' -OutFile './update.bat'"
start cmd /k update.bat
exit

REM -- IF NO UPDATE FOUND THEN CONTINUE DOWNLOADING THE REMAINING FILES AND CHECK IF THEY DOWNLOADED
:cont
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/motd.txt' -OutFile './motd.txt'"
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/7z.bat' -OutFile './7z.bat'"
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/ver.ini' -OutFile './ver.ini'"
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/LICENSE.txt' -OutFile './LICENSE.txt'"
set /p ver= < ver.ini
DEL ver.ini /Q
REM -- EXTRACT THE 7Z FILES BECAUSE THAT SHIT IS IMPORTANT
:7z
CALL 7z.bat
cls
:check5
set goto=filedone
goto updateventoy

REM -- GO TO END OF FILE FOR MOST EXTRACTIONS

REM -- WHEN DONE EXTRACTING VENTOY, TYPE LICENCE AND CONTINUE

:filedone
mode con:cols=100 lines=30
type LICENSE.txt
echo.
echo.Press any Key to Continue (x2)
pause > nul
pause > nul
goto askdownload

:askdownload
if exist "%CD%\MediCat.USB.v21.12.7z" (goto warnventoy) else (goto dlcheck2)
:dlcheck2
if exist "%CD%\*.001" (goto warnventoy) else (goto dlcheck3)
:dlcheck3
cls
mode con:cols=70 lines=18
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
choice /C:YN /N /M "Y/N"
if errorlevel 2 cls && goto warnventoy
if errorlevel 1 cls && goto bigboi



REM -- PROMPT USER TO INSTALL VENTOY TO THE USB DRIVE. VENTOY STILL NEEDS TO BE THERE EVEN IF USER ALREADY HAS IT.

:warnventoy
title Medicat Installer [VENTOYCHECK]

cd .\INSTRUCTIONS\Ventoy2Disk\
start Ventoy2Disk.exe
cd %maindir%
mode con:cols=70 lines=18
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



REM -- THE MAIN MENU, THE HOLY GRAIL.

:menu
title Medicat Installer [%ver%]
mode con:cols=70 lines=18
type motd.txt
echo.
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                   MEDICAT INSTALLER                   IIII
echo.IIII                                                       IIII
echo.IIII                  [1] INSTALL MEDICAT                  IIII
echo.IIII                                                       IIII
echo.IIII                   [2] AUTORUN PATCH                   IIII
echo.IIII                                                       IIII
echo.IIII                   [3] Site [4] Exit                   IIII
echo.IIII                                                       IIII
echo.IIII             VERSION %ver% BY MON5TERMATT.             IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II           
choice /C:123 /N /M "Choice 1-4:"
if errorlevel 4 cls && exit
if errorlevel 3 cls && goto medicatsite
if errorlevel 2 cls && set goto=exit && goto autorun
if errorlevel 1 cls && goto install1



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
echo.All Drive Files Exist (HASHES NOT CHECKED)
echo.
echo.IT LOOKS LIKE YOU DOWNLOADED MEDICAT IN PARTS.
echo.WOULD YOU LIKE TO MAKE SURE THEY DOWNLOADED PROPERLY!?
choice /C:YN /N /M "Y/N"
if errorlevel 2 cls && goto install2
if errorlevel 1 cls && goto hasher

:gdriveerror
echo.IT LOOKS LIKE YOU DOWNLOADED MEDICAT IN PARTS.
echo.WE COULDNT FIND ONE OF THEM. DID YOU DOWNLOAD ALL SIX?
goto bigboi

:install2
mode con:cols=100 lines=15
echo.We now need to find out what drive you will be installing to.
REM - FOLDER PROMPT STARTS
set "psCommand="(new-object -COM 'Shell.Application')^
.BrowseForFolder(0,'Please choose a folder.',0,0).self.path""
for /f "usebackq delims=" %%I in (`powershell %psCommand%`) do set "folder=%%I"
REM - AND ENDS
set drivepath=%folder:~0,1%


echo.FORMAT THE DRIVE AND START FROM SCRATCH?
echo.
echo.IF NO THEN IT WILL OVERRIDE ALL EXISTING FILES WITH SAME NAME
echo.BUT WILL NOT DELETE THE OLD ONES IF NAMES HAVE CHANGED
echo.THIS MAY RESULT IN DOUBLE FILES AND SIZE

choice /C:YN /N /M "Y/N:"
if errorlevel 2 cls && goto installversion
if errorlevel 1 cls && goto formatdrive
:formatdrive
Echo Warning this will reformat the entire %drivepath%: disk!
ECHO. you will be prompted to hit enter a few times.
pause
format %drivepath%: /FS:NTFS /x /q /V:Medicat
goto installversion

:error
echo.nothing was chosen, try again
timeout 5
goto install2

REM -- CHECK WHICH VERSION USER DOWNLOADED

:installversion
if exist "%CD%\MediCat.USB.v21.12.7z" (goto install4) else (goto installversion2)
:installversion2
if exist "%CD%\MediCat.USB.v%ver%.zip.001" (goto install5) else (goto installerror)

:installerror
mode con:cols=70 lines=18
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
7z x -o%drivepath%: %file% -r -aoa
goto finishup

:install5
set file="MediCat.USB.v%ver%.zip.001"
7z x -o%drivepath%: %file% -r -aoa
goto finishup

:install6
7z x -o%drivepath%: "%file%" -r -aoa
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
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/autorun.ico' -OutFile './autorun.ico'"
COPY autorun.ico %drivepath%:/autorun.ico
DEL autorun.ico /Q
goto %goto%
:deletefiles
echo.Would you like to delete the downloaded files?
echo.(everything in the folder you ran this from)
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
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/Fixer.exe' -OutFile './Fixer.exe'"
start Fixer.exe
goto warnventoy


:medicatsite
start https://medicatusb.xyz
goto menu

:updateventoy
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/ventoy.bat' -OutFile './ventoy.bat'"
cls
call ventoy.bat
cls
goto %goto%


:exit
exit


:bigboi
cls
mode con:cols=70 lines=18
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII           WOULD YOU LIKE TO USE THE TORRENT           IIII
echo.IIII            TO DOWNLOAD THE LATEST VERSION?            IIII
echo.IIII                                                       IIII
echo.IIII                                                       IIII
echo.IIII                         Y / N                         IIII
echo.IIII                                                       IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
choice /C:YN /N /M "Y/N"
if errorlevel 2 cls && goto drivedown
if errorlevel 1 cls && goto tordown
:drivedown
cls
mode con:cols=70 lines=18
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
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/downloader.bat' -OutFile './downloader.bat'"
call downloader.bat
del downloader.bat /Q
goto warnventoy

:tordown
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/tor/tordownload.bat' -OutFile './tordownload.bat'"
call tordownload.bat
del tordownload.bat /Q
goto warnventoy

:renameprogram
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/update.bat' -OutFile './update.bat'"
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
echo.CODED BY MON5TERMATT#9999 With Help from AAA3A#1157, Daan Breur#6262, Jayro#0783, and many others
