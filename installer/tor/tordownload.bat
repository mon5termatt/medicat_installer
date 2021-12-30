powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/tor/MediCat_USB_v21.12.torrent' -OutFile './medicat.torrent'"
powershell -c "Invoke-WebRequest -Uri 'http://cdn.medicatusb.xyz/files/installer/tor/aria2c.exe' -OutFile './aria2c.exe'"
aria2c.exe --file-allocation=none --check-integrity=true --bt-hash-check-seed=false --seed-time=0 medicat.torrent
MOVE ".\MediCat USB v21.06\MediCat Main Partition.zip" ".\MediCat Main Partition.zip"
RD /S /Q "MediCat USB v21.06"
del medicat.torrent /Q
del aria2c.exe /Q
