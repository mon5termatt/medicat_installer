wget https://dist.ipfs.tech/ipget/v0.9.1/ipget_v0.9.1_windows-amd64.zip
7z x ipget_v0.9.1_windows-amd64.zip
DEL ipget_v0.9.1_windows-amd64.zip /Q
MOVE ".\ipget\ipget.exe" ".\ipget.exe"
RD /S /Q "ipget"



:actualdl
cls
echo.This May take a while. IPFS is a Distributed filesharing platform. its dependant on how many others are using it..
ipget QmVRySjFZ5xFAjQ6waAseMw8DfiDkJWsgdyxR5Uc4W215h
MOVE ".\QmVRySjFZ5xFAjQ6waAseMw8DfiDkJWsgdyxR5Uc4W215h\v21.12\MediCat.USB.v21.12.7z" ".\MediCat.USB.v21.12.7z"
RD /S /Q "QmVRySjFZ5xFAjQ6waAseMw8DfiDkJWsgdyxR5Uc4W215h"
