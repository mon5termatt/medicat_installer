#!/bin/bash
echo -e "WELCOME TO THE MEDICAT INSTALLER, PLEASE DO NOT RUN THIS AS ROOT\nThis Installer will attempt to Install Ventoy and Medicat\nTHIS IS IN BETA. PLEASE CONTACT MATT IN THE DISCORD FOR ALL ISSUES"
echo "Updated for efficiency and cross-distro use by SkeletonMan"
echo "Waiting for 10 seconds"
sleep 10
if grep -qs "ubuntu" /etc/os-release; then
	os="ubuntu"
	pkgmgr="apt"
	install_arg="install"
	update_arg="update"
elif grep -qs "freebsd" /etc/os-release; then
	os="freebsd"
	pkgmgr="pkg"
	install_arg="install"
	update_arg="update"
elif [[ -e /etc/debian_version ]]; then
	os="debian"
	pkgmgr="apt"
	install_arg="install"
	update_arg="update"
elif [[ -e /etc/almalinux-release || -e /etc/rocky-release || -e /etc/centos-release ]]; then
	os="centos"
	pkgmgr="yum"
	install_arg="install"
	update_arg="update"
elif [[ -e /etc/fedora-release ]]; then
	os="fedora"
	pkgmgr="yum"
	install_arg="install"
	update_arg="update"
elif [[ -e /etc/arch-release ]]; then
	os="arch"
	pkgmgr="pacman"
	install_arg="-S --needed --noconfirm"
	update_arg="-Syy"
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
	echo "Please enter location of MediCat.USB.v21.12.7z if it exists or just press enter to download it via tor."
	read location
	fi
	if [ -z "$location" ] ; then
		echo "Starting to download torrent"
		wget https://github.com/mon5termatt/medicat_installer/raw/main/download/MediCat_USB_v21.12.torrent -O medicat.torrent
		aria2c --file-allocation=none --seed-time=0 medicat.torrent
		location=''MediCat\ USB\ v21.12/MediCat.USB.v21.12.7z''
	fi
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
echo "You want to install Ventoy and Medicat to $drive / $drive2?"
echo "Please enter Y or N"
read checkingconfirm
if [ $checkingconfirm = "N" ]; then
        exit
elif [ $checkingconfirm = "Y" ]; then
        echo "Okay! Will continue in 5 seconds!"
	sleep 5
else
        echo "The only valid options are Y or N"
	exit
fi

sudo sh ./ventoy/Ventoy2Disk.sh -I $drive
umount $drive
sudo mkntfs --fast --label Medicat $drive2
if ! [[ -d MedicatUSB/ ]] ; then
	mkdir MedicatUSB
fi
sudo mount $drive2 ./MedicatUSB
7z x -O./MedicatUSB "$location"
echo "MedicatUSB has been created!"
echo "Would you like to unmount ./MedicatUSB? (Y/N)"
read unmountcheck
if [ $unmountcheck = "Y" ]; then
	echo "MedicatUSB will be unmounted!"
	sudo umount ./MedicatUSB
elif [ $unmountcheck = "N" ]; then
	echo "MedicatUSB will not be unmounted!"
else
	"The only valid options are Y or N"
fi
