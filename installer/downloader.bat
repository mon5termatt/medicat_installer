@echo off
:: https://github.com/prasmussen/gdrive
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/7z.bat' -OutFile './7z.bat'"
CALL 7z.bat
DEL 7z.bat /Q
powershell -c "Invoke-WebRequest -Uri 'https://github.com/prasmussen/gdrive/releases/download/2.1.1/gdrive_2.1.1_windows_amd64.tar.gz' -OutFile './gdrive.tar.gz'"
7z e gdrive.tar.gz -r -aoa
7z e gdrive.tar -r -aoa
DEL gdrive.tar.gz
DEL gdrive.tar
cls
set file1=1q8gulgxsQjEveNcf2ZLSOpQgroJ78CZq
set file2=1xMiquzKsfW1LUd8RmypMO3Z7PzilY9lu
set file3=1nhFdNHYiWhsh__uL6h1qo2QMceEcqhEy
set file4=1uO9I1poXhwJ-FP7n1kFS6NY7na_ZrSZR
SET file5=1ubWGDRP3Cy2bk1yU008TGMQ0zRA2lsVF
set file6=1k78sLJTUyxW-zu7rwn9crHYZjHjah3OE
gdrive download %file1% %options%
gdrive download %file2% %options%
gdrive download %file3% %options%
gdrive download %file4% %options%
gdrive download %file5% %options%
gdrive download %file6% %options%
echo.DONE!
DEL gdrive.exe
exit



gdrive [global] download query [options] <query>

global:
  -c, --config <configDir>         Application path, default: /Users/<user>/.gdrive
  --refresh-token <refreshToken>   Oauth refresh token used to get access token (for advanced users)
  --access-token <accessToken>     Oauth access token, only recommended for short-lived requests because of short lifetime (for advanced users)
  --service-account <accountFile>  Oauth service account filename, used for server to server communication without user interaction (file is relative to config dir)
  
options:
  -f, --force       Overwrite existing file
  -r, --recursive   Download directories recursively, documents will be skipped
  --path <path>     Download path
  --no-progress     Hide progress

