wget "https://medicatcdn.com/files/v21.12/MediCat_USB_v21.12.torrent" -O ./medicat.torrent -q
powershell -c "$data = wget https://api.github.com/repos/aria2/aria2/git/refs/tag -UseBasicParsing | ConvertFrom-Json; $data[-1].ref -replace 'refs/tags/', '' | Out-File -Encoding 'UTF8' -FilePath './ariaversion.txt'"
set /p ARIAVER= <./ariaversion.txt
set ARIACURVER=%ARIAVER:~-6%
del ariaversion.txt /Q
del .wget-hsts /Q
wget "https://github.com/aria2/aria2/releases/download/release-%ARIACURVER%/aria2-%ARIACURVER%-win-64bit-build1.zip" -O ./aria2-%ARIACURVER%-win-64bit-build1.zip -q
7z x aria2-%ARIACURVER%-win-64bit-build1.zip -r -aoa
MOVE ".\aria2-%ARIACURVER%-win-64bit-build1\aria2c.exe" ".\aria2c.exe"
RD /S /Q "aria2-%ARIACURVER%-win-64bit-build1"
DEL aria2-1.36.0-win-64bit-build1.zip /Q
aria2c.exe --file-allocation=none --seed-time=0 medicat.torrent
MOVE ".\MediCat USB v21.12\MediCat.USB.v21.12.7z" ".\MediCat.USB.v21.12.7z"
RD /S /Q "MediCat USB v21.12"
del medicat.torrent /Q
del aria2c.exe /Q
