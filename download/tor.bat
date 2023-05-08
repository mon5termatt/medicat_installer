Set "Path=%Path%;%CD%;%CD%\bin;"
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/download/MediCat_USB_v21.12.torrent" -o ./medicat.torrent -s 
if defined ProgramFiles(x86) (set bit=64) else (set bit=32)
curl "https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/bin/aria2c-%bit%.exe" -o ./aria2c.exe -s
aria2c.exe --file-allocation=none --seed-time=0 medicat.torrent
MOVE ".\MediCat USB v21.12\MediCat.USB.v21.12.7z" ".\MediCat.USB.v21.12.7z"
RD /S /Q "MediCat USB v21.12"
del medicat.torrent /Q
del aria2c.exe /Q
