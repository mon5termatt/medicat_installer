#!/usr/bin/env bash

# Check if the terminal supports colour and set up variables if it does.
NumColours=$(tput colors)

if test -n "$NumColours" && test $NumColours -ge 8; then

    clear="$(tput sgr0)"
    blackN="$(tput setaf 0)";		blackN="$(tput bold setaf 0)"
    redN="$(tput setaf 1)";			redB="$(tput bold setaf 1)"
    greenN="$(tput setaf 2)";		greenB="$(tput bold setaf 2)"
    yellowN="$(tput setaf 3)";		yellowB="$(tput bold setaf 3)"
    blueN="$(tput setaf 4)";		blueB="$(tput bold setaf 4)"
    magentaN="$(tput setaf 5)";		magentaB="$(tput bold setaf 5)"
    cyanN="$(tput setaf 6)";		cyanB="$(tput bold setaf 6)"
    whiteN="$(tput setaf 7)";		whiteB="$(tput bold setaf 7)"

fi

# Function to echo text using terminal colour codes ###########################
function colEcho() {
    echo -e "$1$2$clear"
}

# Function to wait for a user keypress.
UserWait () {
    read -n 1 -s -r -p "Press any key to continue"
    echo -e "\r                         \r"
}

# Function to check we are not running with the elevated privileges. ##########
function CheckNotElevated {

    if (( "$EUID" == "0" )); then
        colEcho $redB "ERROR: Running with elevated privileges - do not run using sudo\n"
        exit 1
    fi
}

# Main Code Start. ############################################################

# Key variables used throughout the script to make maintenance easier.
Medicat256Hash='a306331453897d2b20644ca9334bb0015b126b8647cecec8d9b2d300a0027ea4'
Medicat7zFile="MediCat.USB.v21.12.7z"
Medicat7zFull=''MediCat\ USB\ v21.12/MediCat.USB.v21.12.7z''

clear
colEcho $yellowB "WELCOME TO THE MEDICAT INSTALLER.\n"

CheckNotElevated

colEcho $cyanB "This Installer will install Ventoy and Medicat.\n"
colEcho $yellowB "THIS IS IN BETA. PLEASE CONTACT MATT IN THE DISCORD FOR ALL ISSUES.\n"
colEcho $cyanB "Updated for efficiency and cross-distro use by SkeletonMan.\n"
colEcho $cyanB "Enhancements by Manganar.\n"

# Set variables to support different distros.
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
	colEcho $redB "Fuck Red-Hat for putting source code behind paywalls."
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
else
	colEcho "ERROR: Distro not recognised - exiting..."
	exit 1
fi

colEcho $cyanB "Operating System Identified:$whiteB $os \n"

# Ensure dependencies are installed: wget, curl, 7z, mkntfs, aria2c
colEcho $cyanB "Acquiring any dependencies..."

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
	elif [ "$os" == "centos" ]; then
		sudo $pkgmgr $install_arg p7zip p7zip-plugins
	else
		sudo $pkgmgr $install_arg p7zip-full
	fi
fi

if ! [ $(sudo which mkntfs 2>/dev/null) ]; then 
	if [ "$os" == "centos" ]; then
		sudo $pkgmgr $install_arg ntfsprogs
	else
		sudo $pkgmgr $install_arg ntfs-3g
	fi
fi

if ! [ $(which aria2c 2>/dev/null) ]; then
	sudo $pkgmgr $install_arg aria2
fi

# Identify latest Ventoy release.
venver=$(curl -sL https://api.github.com/repos/ventoy/Ventoy/releases/latest | grep '"tag_name":' | cut -d'"' -f4)

# Download latest verion of Ventoy.
colEcho $cyanB "\nDownloading Ventoy Version:$whiteB ${venver: -6}"
wget -q --show-progress https://github.com/ventoy/Ventoy/releases/download/v${venver: -6}/ventoy-${venver: -6}-linux.tar.gz -O ventoy.tar.gz

colEcho $cyanB "\nExtracting Ventoy..."
tar -xf ventoy.tar.gz

colEcho $cyanB "Removing the extracted Ventory tar.gz file..."
rm -rf ventoy.tar.gz

# Remove the ./ventoy folder if it exists before renaming ventoy folder.
if [ -d ./ventoy ]; then
	colEcho $cyanB "Removing the previous ./ventoy folder..."
	rm -rf ./ventoy/
fi

colEcho $cyanB "Renaming ventoy folder to remove the version number..."
mv ventoy-${venver: -6} ventoy

colEcho $cyanB "\nLocating the Medicat 7z file..."

if [[ -f "$Medicat7zFile" ]]; then
	location="$Medicat7zFile"
else
	if  [[ -f "$Medicat7zFull" ]]; then
		location="$Medicat7zFull"
	else
		colEcho $cyanB "Please enter the location of$whiteB $Medicat7zFile$cyanB if it exists or just press enter to download it via bittorrent."
		read location
	fi

	if [ -z "$location" ] ; then
		colEcho $cyanB "Starting to download torrent"
		wget https://github.com/mon5termatt/medicat_installer/raw/main/download/MediCat_USB_v21.12.torrent -O medicat.torrent
		aria2c --file-allocation=none --seed-time=0 medicat.torrent
		location="$Medicat7zFull"
	fi
fi

colEcho $cyanB "Medicat 7z file found:$whiteB $location"

# Check the SHA256 hash of the Medicat zip file.
colEcho $cyanB "Checking SHA256 hash of$whiteB $Medicat7zFile$cyanB..."

checksha256=$(sha256sum "$location" | awk '{print $1}')

if [[ "$checksha256" -ne "$Medicat256Hash" ]]; then
	colEcho $redB "$Medicat7zFile SHA256 hash does not match."
	colEcho $redB "File may be corrupted or compromised."
	colEcho $cyanB "Hash is$whiteB $checksha256"
	colEcho $cyanB "Exiting..."
	exit 1
else
	colEcho $greenB "$Medicat7zFile SHA256 hash matches."
	colEcho $cyanB "Hash is$whiteB $checksha256"
	colEcho $cyanB "Safe to proceed..."
fi

# Advise user to connect and select the required USB device.
colEcho $yellowB "\nPlease Plug your USB in now if it is not already connected..."
UserWait

colEcho $yellowB "Please Find the ID of your USB below:"

lsblk --scsi --nodeps --output "NAME,SIZE,MOUNTPOINTS"

colEcho $yellowB "Enter the device for the USB drive NOT INCLUDING /dev/ OR the Number After."
colEcho $yellowB "for example enter sda or sdb"
read letter

drive=/dev/$letter
drive2="$drive""1"
checkingconfirm=""

while [[ "$checkingconfirm" != [NnYy]* ]]; do
	read -e -p "You want to install Ventoy and Medicat to $drive / $drive2? (Y/N) " checkingconfirm
	if [[ "$checkingconfirm" == [Nn]* ]]; then
		colEcho $yellowB "Installation Cancelled."
		exit
	elif [[ "$checkingconfirm" == [Yy]* ]]; then
		colEcho $cyanB "Installation confirmed and will commence in 5 seconds..."
		sleep 5
	else
		colEcho $redB "Invalid input. Please enter 'Y' or 'N'."
	fi
done

colEcho $cyanB "Installing Ventoy on$whiteB $drive"
sudo sh ./ventoy/Ventoy2Disk.sh -I $drive
if [ "$?" != "0" ]; then
	colEcho $redB "ERROR: Unable to install Ventoy. Exiting..."
	exit 1
fi

colEcho $cyanB "Unmounting drive$whiteB $drive"
sudo umount $drive

colEcho $cyanB "Creating Medicat NTFS file system on drive$whiteB $drive2"
sudo mkntfs --fast --label Medicat $drive2

# Create a mountpoint folder for the Medicat NTFS volume
if ! [[ -d MedicatUSB/ ]] ; then
	colEcho $cyanB "Creating a mountpoint for the Medicat NTFS volume..."
	mkdir MedicatUSB
fi

colEcho $cyanB "Mounting Medicat NTFS volume..."
sudo mount $drive2 ./MedicatUSB

colEcho $cyanB "Extracting Medicat to NTFS volume..."
7z x -O./MedicatUSB "$location"

colEcho $cyanB "MedicatUSB has been created."

unmountcheck=""
while [[ "$unmountcheck" != [NnYy]* ]]; do
	read -e -p "Would you like to unmount ./MedicatUSB? (Y/N) " unmountcheck
	if [[ $unmountcheck == [Yy]* ]]; then
		colEcho $cyanB "Unmounting MedicatUSB..."
		sudo umount ./MedicatUSB
		colEcho $cyanB "Unmounted."
	elif [[ $unmountcheck == [Nn]* ]]; then
		colEcho $cyanB "MedicatUSB will not be unmounted."
	else
		colEcho $redB "Invalid input. Please enter 'Y' or 'N'."
	fi
done
