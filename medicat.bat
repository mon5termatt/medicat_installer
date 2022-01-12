@ECHO OFF
REM BFCPEOPTIONSTART
REM Advanced BAT to EXE Converter www.BatToExeConverter.com
REM BFCPEEXE=F:\Medicat\Medicat Installer.exe
REM BFCPEICON=C:\Users\Matt\Downloads\MediCat_Installer.ico
REM BFCPEICONINDEX=-1
REM BFCPEEMBEDDISPLAY=0
REM BFCPEEMBEDDELETE=1
REM BFCPEADMINEXE=1
REM BFCPEINVISEXE=0
REM BFCPEVERINCLUDE=1
REM BFCPEVERVERSION=2.0.0.6
REM BFCPEVERPRODUCT=MEDICAT INSTALLER
REM BFCPEVERDESC=INSTALL MEDICAT USB WITH GUIDED PROMPTS
REM BFCPEVERCOMPANY="http://MEDICATUSB.XYZ
REM BFCPEVERCOPYRIGHT="http://MON5TERMATT.CLUB
REM BFCPEOPTIONEND
@ECHO ON
@echo OFF & setlocal enabledelayedexpansion
set ver=2008
set maindir=%CD%
set format=Y
set installertext=[31mM[32mE[33mD[34mI[35mC[36mA[31mT[32m I[33mN[34mS[35mT[36mA[31mL[32mL[33mE[34mR[0m
reg add HKEY_CURRENT_USER\Software\Medicat\Installer /v version /t  REG_SZ /d  %ver% /f
:start
call :extractwget
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
wget "http://cdn.medicatusb.xyz/files/installer/curver.ini" -O ./curver.ini -q
set /p remver= < curver.ini
del curver.ini /Q
if "%ver%" == "%remver%" (goto cont) else (goto updateprogram)
:updateprogram
cls
echo.this program must be updated to continue.
timeout 2 > nul
wget "http://cdn.medicatusb.xyz/files/installer/Medicat Installer.exe" -O ./MEDICAT_NEW.EXE -q
wget "http://cdn.medicatusb.xyz/files/installer/update.bat" -O ./update.bat -q
start cmd /k update.bat
exit

REM -- IF NO UPDATE FOUND THEN CONTINUE DOWNLOADING THE REMAINING FILES AND CHECK IF THEY DOWNLOADED
:cont
wget "http://cdn.medicatusb.xyz/files/installer/motd.txt" -O ./motd.txt -q
wget "http://cdn.medicatusb.xyz/files/installer/ver.ini" -O ./ver.ini -q
wget "http://cdn.medicatusb.xyz/files/installer/LICENSE.txt" -O ./LICENSE.txt -q
set /p ver= < ver.ini
DEL ver.ini /Q

:menu
REM -- THE MAIN MENU, THE HOLY GRAIL.
title Medicat Installer [%ver%]
mode con:cols=100 lines=30
type LICENSE.txt
echo.
echo.Press any Key to Continue (x2)
pause > nul
pause > nul
:menu2
mode con:cols=70 lines=20
type motd.txt
echo.
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II
echo.IIII                                                       IIII
echo.IIII                   %installertext%                   IIII
echo.IIII                                                       IIII
echo.IIII          I:        [91mI[0mNSTALL MEDICAT                    IIII
echo.IIII                                                       IIII
echo.IIII          F:     [91mF[0mormatting Drive: [%format%]                 IIII
echo.IIII                                                       IIII
echo.IIII          A:         [91mA[0mUTORUN PATCH                     IIII
echo.IIII                                                       IIII
echo.IIII          S:             [91mS[0mite                          IIII
echo.IIII                                                       IIII
echo.IIII             VERSION %ver% BY MON5TERMATT.             IIII
echo.II-----------------------------------------------------------II
echo.II-----------------------------------------------------------II           
choice /C:IFAS /N /M "Choose an option: I,F,A,S"
if errorlevel 4 cls && goto medicatsite
if errorlevel 3 cls && set goto=exit && goto autorun
if errorlevel 2 cls && goto formatswitch
if errorlevel 1 cls && goto 7z 
:formatswitch
if "%format%" == "Y" (goto fs2) else (echo.>nul)
if "%format%" == "N" (goto fs3) else (goto menu2)
:fs2
set format=N
goto menu2
:fs3
set format=Y
goto menu2




REM -- EXTRACT THE 7Z FILES BECAUSE THAT SHIT IS IMPORTANT
:7z
wget "http://cdn.medicatusb.xyz/files/installer/7z.bat" -O ./7z.bat -q
CALL 7z.bat
DEL 7z.bat /Q
cls
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
mode con:cols=70 lines=18
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
choice /C:YN /N /M "Y/N"
if errorlevel 2 cls && goto install2
if errorlevel 1 cls && goto hasher

:gdriveerror
mode con:cols=70 lines=18
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
7z x" -O%drivepath%: %file% -r -aoa
goto finishup

:install5
set file="MediCat.USB.v%ver%.zip.001"
7z x" -O%drivepath%: %file% -r -aoa
goto finishup

:install6
7z x" -O%drivepath%: "%file%" -r -aoa
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
wget "http://cdn.medicatusb.xyz/files/installer/autorun.ico" -O ./autorun.ico -q
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
wget "http://cdn.medicatusb.xyz/files/hasher/Fixer.exe" -O ./Fixer.exe -q
start Fixer.exe
goto install2


:medicatsite
start "http://medicatusb.xyz
goto menu2

:updateventoy
wget "http://cdn.medicatusb.xyz/files/installer/ventoy.bat" -O ./ventoy.bat -q
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
wget "http://cdn.medicatusb.xyz/files/installer/downloader.bat" -O ./downloader.bat -q
call downloader.bat
del downloader.bat /Q
goto warnventoy

:tordown
wget "http://cdn.medicatusb.xyz/files/installer/tor/tordownload.bat" -O ./tordownload.bat -q
call tordownload.bat
del tordownload.bat /Q
goto warnventoy

:renameprogram
wget "http://cdn.medicatusb.xyz/files/installer/update.bat" -O ./update.bat -q
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
exit/b