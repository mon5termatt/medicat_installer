Set "Path=%Path%;%CD%;%CD%\bin;"
wget "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/download/MediCat_USB_v21.12.torrent" -O ./medicat.torrent -q 
if defined ProgramFiles(x86) (set bit=64) else (set bit=32)
wget "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/aria2c-%bit%.exe" -O ./aria2c.exe -q
aria2c.exe --file-allocation=none --seed-time=0 medicat.torrent
MOVE ".\MediCat USB v21.12\MediCat.USB.v21.12.7z" ".\MediCat.USB.v21.12.7z"
RD /S /Q "MediCat USB v21.12"
del medicat.torrent /Q
del aria2c.exe /Q
