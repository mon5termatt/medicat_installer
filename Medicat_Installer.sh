echo -e "WELCOME TO THE MEDICAT INSTALLER, PLEASE DO NOT RUN THIS AS ROOT\nThis Installer will attempt to Install Ventoy and Medicat\nTHIS IS IN ALPHA. PLEASE CONTACT MATT IN THE DISCORD FOR ALL ISSUES"
echo "waiting for 10 seconds"
sleep 10
echo "Attempting to download the required dependancies"
echo -e "\n\n\n\n\n\n"
sudo apt update
sudo apt install aria2 wget p7zip-full ntfs-3g
wget "https://api.github.com/repos/ventoy/Ventoy/releases/latest"
cat latest | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/' >> version
venver=$(cat version)
rm version
rm latest
echo -e "Attempting to download Ventoy Version: ${venver: -6}\n\n\n"
wget https://github.com/ventoy/Ventoy/releases/download/v${venver: -6}/ventoy-${venver: -6}-linux.tar.gz -O ventoy.tar.gz
tar -xf ventoy.tar.gz
rm ventoy.tar.gz
mv ventoy-${venver: -6} ventoy
echo -e "\n\n\n\n\n\n"
if [[ -f MediCat.USB.v21.12.7z ]]; then
location='MediCat.USB.v21.12.7z'
fi
if ! [[ -f MediCat.USB.v21.12.7z ]]; then
	if  [[ -f MediCat\ USB\ v21.12/MediCat.USB.v21.12.7z ]]; then
		location=''MediCat\ USB\ v21.12/MediCat.USB.v21.12.7z''
	else
	echo "Please enter location of MediCat.USB.v21.12.7z if it exists or just press enter"
	read location
	fi
	if [ -z "$location" ] ; then
		echo "Starting to download torrent"
		wget https://cdn.medicatusb.com/files/install/download/MediCat_USB_v21.12.torrent -O medicat.torrent
		aria2c --file-allocation=none --seed-time=0 medicat.torrent
		location=''MediCat\ USB\ v21.12/MediCat.USB.v21.12.7z''
	fi
fi
echo -e "\n\n\n\n\n\n"
echo "Please Plug your USB in now if it is not already"
echo "You will need to have the USB MOUNTED"
echo "Please Find the ID of your USB below"
echo -e "\n\n"
df | grep -v ^/dev/loop
echo "Enter the Letter of the USB drive below NOT INCLUDING /dev/ OR the Number After"
echo "for example enter sda or sdb"
read letter
drive=/dev/$letter
drive2="$drive""1"
echo "you want to install Ventoy and Medicat to $drive / $drive2?"
echo "If not please press CTRL+C now (timeout in 5 seconds)"
sleep 5
sudo sh ./ventoy/Ventoy2Disk.sh -I $drive
umount $drive
sudo mkntfs --fast --label Medicat $drive2
if ! [[ -d USB/ ]] ; then
	mkdir USB
fi
sudo mount $drive2 ./USB
7z x -O./USB "$location"
