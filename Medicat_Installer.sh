#!/bin/bash
echo -e "WELCOME TO THE MEDICAT INSTALLER, PLEASE DO NOT RUN THIS AS ROOT\nThis Installer will attempt to Install Ventoy and Medicat\nTHIS IS IN BETA. PLEASE CONTACT MATT IN THE DISCORD FOR ALL ISSUES"
echo "Updated for efficiency and cross-distro use by SkeletonMan"
echo "Waiting for 10 seconds"
sleep 10


if which pacman >/dev/null; then
	pkgmgr="pacman"
	install_arg="-S --needed --noconfirm"
	update_arg="-Syy"
elif which apt >/dev/null; then
	pkgmgr="apt"
	install_arg="install"
	update_arg="update"
elif which yum >/dev/null; then
	echo "Fuck Red-Hat for putting source code behind paywalls."
	pkgmgr="yum"
	install_arg="install"
	update_arg="update"
elif which pkg >/dev/null; then
	pkgmgr="pkg"
	install_arg="install"
	update_arg="update"
fi
echo "Acquiring any dependencies"
sudo $pkgmgr $update_arg
if ! [ $(which wget 2>/dev/null) ]; then
	sudo $pkgmgr $install_arg wget
fi
if ! [ $(which curl 2>/dev/null) ]; then
	sudo $pkgmgr $install_arg curl
fi
if ! [ $(which 7z 2>/dev/null) ]; then
	if [[ -e /etc/arch-release ]]; then
		sudo $pkgmgr $install_arg p7zip
	elif [[ -e /etc/fedora-release  ]]; then
		sudo $pkgmgr $install_arg p7zip-full p7zip-plugins
	else
		sudo $pkgmgr $install_arg p7zip-full
	fi
fi
if ! [ $(sudo which mkntfs 2>/dev/null) ]; then 
	sudo $pkgmgr $install_arg ntfs-3g
fi
if ! [ $(which aria2c 2>/dev/null) ]; then
	sudo $pkgmgr $install_arg aria2
fi
venver=$(curl -sL https://api.github.com/repos/ventoy/Ventoy/releases/latest | grep '"tag_name":' | cut -d'"' -f4)
rm latest
echo -e "Attempting to download Ventoy Version: ${venver: -6}\n\n\n"
wget https://github.com/ventoy/Ventoy/releases/download/v${venver: -6}/ventoy-${venver: -6}-linux.tar.gz -O ventoy.tar.gz
tar -xf ventoy.tar.gz
rm -rf ventoy.tar.gz ./ventoy/ventoy-${venver: -6}
mv ventoy-${venver: -6} ventoy
echo -e "\n\n\n\n\n\n"
sha256hash='a306331453897d2b20644ca9334bb0015b126b8647cecec8d9b2d300a0027ea4'
if [[ -f MediCat.USB.v21.12.7z ]]; then
	location='MediCat.USB.v21.12.7z'
fi
if ! [[ -f MediCat.USB.v21.12.7z ]]; then
	if  [[ -f MediCat\ USB\ v21.12/MediCat.USB.v21.12.7z ]]; then
		location=''MediCat\ USB\ v21.12/MediCat.USB.v21.12.7z''
	else
	echo "Please enter location of MediCat.USB.v21.12.7z if it exists or just press enter to download it via torrent."
	read location	
	fi
	if [ -z "$location" ] ; then
		echo "Starting to download torrent"
		wget https://github.com/mon5termatt/medicat_installer/raw/main/download/MediCat_USB_v21.12.torrent -O medicat.torrent
		aria2c --file-allocation=none --seed-time=0 medicat.torrent
		location=''MediCat\ USB\ v21.12/MediCat.USB.v21.12.7z''
	fi
fi

echo -e "Checking Sha256 hash of MediCat.USB.v21.12.7z.."
checksha256=$(sha256sum MediCat.USB.v21.12.7z | awk '{print $1}')
if [[ checksha256 -ne $sha256hash ]]; then
	echo -e "Your MediCat.USB.v21.12.7z SHA256 hash does not match!!!"
	echo -e "Hash is $checksha256"
	echo -e "Exiting for your safety!"
	exit
else
	echo -e "Your MediCat.USB.v21.12.7z sha256 hash matches!"
	echo -e "Hash is $checksha256"
	echo -e "We are safe to proceed"
fi


echo -e "\n\n\n"
echo "Please Plug your USB in now if it is not already"
echo "Waiting 15 seconds..."
sleep 15
echo "Please Find the ID of your USB below"
echo -e "\n\n"
lsblk | awk '{print $1,$4}'
echo "Enter the Letter of the USB drive below NOT INCLUDING /dev/ OR the Number After"
echo "for example enter sda or sdb"
read letter
drive=/dev/$letter
drive2="$drive""1"
checkingconfirm=""
while [[ "$checkingconfirm" != [NnYy]* ]]; do
	read -e -p "You want to install Ventoy and Medicat to $drive / $drive2? (Y/N) " checkingconfirm
	if [[ "$checkingconfirm" == [Nn]* ]]; then
        exit
	elif [[ "$checkingconfirm" == [Yy]* ]]; then
		echo "Okay! Will continue in 5 seconds!"
		sleep 5
	else
		echo "Invalid input!"
	fi
done

sudo sh ./ventoy/Ventoy2Disk.sh -I $drive
umount $drive
sudo mkntfs --fast --label Medicat $drive2
if ! [[ -d MedicatUSB/ ]] ; then
	mkdir MedicatUSB
fi
sudo mount $drive2 ./MedicatUSB
7z x -O./MedicatUSB "$location"
echo "MedicatUSB has been created!"
unmountcheck=""
while [[ "$unmountcheck" != [NnYy]* ]]; do
	read -e -p "Would you like to unmount ./MedicatUSB? (Y/N) " unmountcheck
	if [[ $unmountcheck == [Yy]* ]]; then
		echo "MedicatUSB will be unmounted!"
		sudo umount ./MedicatUSB
	elif [[ $unmountcheck == [Nn]* ]]; then
		echo "MedicatUSB will not be unmounted!"
	else
		echo "Invalid input!"
	fi
done
