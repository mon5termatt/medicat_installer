:start
powershell -c "$data = wget https://api.github.com/repos/ventoy/ventoy/git/refs/tag -UseBasicParsing | ConvertFrom-Json; $data[-1].ref -replace 'refs/tags/', '' | Out-File -Encoding 'UTF8' -FilePath './ventoyversion.txt'"
set /p VENVER= <./ventoyversion.txt
set vencurver=%VENVER:~-6%
goto check
:check
if exist "%CD%\INSTRUCTIONS\Ventoy2Disk\" (goto checkver) else (goto download)
:checkver
set /p localver= <.\INSTRUCTIONS\Ventoy2Disk\ventoy\version
if "%localver%" == "%vencurver%" (goto end) else (goto download)




:download
wget https://github.com/ventoy/Ventoy/releases/download/v%vencurver%/ventoy-%vencurver%-windows.zip -O ./ventoy.zip -q
7z x ventoy.zip -r -aoa
RMDIR INSTRUCTIONS\Ventoy2Disk /S /Q
MD INSTRUCTIONS
REN ventoy-%vencurver% Ventoy2Disk
MOVE ./Ventoy2Disk "./INSTRUCTIONS/"
DEL ventoy.zip /Q
:end
del ventoyversion.txt /Q
del .wget-hsts /Q
del %0 & exit/b