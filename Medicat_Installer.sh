#!/usr/bin/env bash

# Script Version 0008

#--------------------------------Variables------------------------------------#

# Key variables used throughout the script to make maintenance easier.
MedicatVersion="v21.12"
Medicat256Hash='a306331453897d2b20644ca9334bb0015b126b8647cecec8d9b2d300a0027ea4'
Medicat7zFile="MediCat.USB.$MedicatVersion.7z"
Medicat7zFull=''MediCat\ USB\ $MedicatVersion/MediCat.USB.$MedicatVersion.7z''

# Dependencies
declare -A depCommands
depCommands["wget"]="wget"
depCommands["curl"]="curl"
depCommands["7z"]="zip"
depCommands["mkfs.vfat"]="mkfs"
depCommands["mkntfs"]="ntfs"
declare -A curl
curl["default"]="curl"
declare -A wget
wget["nixos"]="nixos.wget"
wget["default"]="wget"
declare -A zip
zip["arch"]="p7zip"
zip["nixos"]="nixos.p7zip"
zip["fedora"]="p7zip p7zip-plugins"
zip["nobara"]="p7zip-full p7zip-plugins"
zip["centos"]="p7zip p7zip-plugins"
zip["alpine"]="7zip"
zip["default"]="p7zip-full"
declare -A mkfs
mkfs["nixos"]="nixos.dosfstools"
mkfs["default"]="dosfstools"
declare -A ntfs
ntfs["centos"]="ntfsprogs"
ntfs["nixos"]="nixos.ntfs3g"
ntfs["default"]="ntfs-3g"
declare -A aria
aria["nixos"]="nixos.aria"
aria["default"]="aria2"
declare -A ventoy
ventoy["nixos"]="nixos.ventoy-full"
ventoy["default"]="ventoy"

# Other Variables
sudo="sudo" # By default use sudo with package manager
ventoyFS=true  # By default install ventoy from github(FromSource)
ventoyLauncher="sh ./Ventoy2Disk.sh" # By default use the ventoy script

# Check if the terminal supports colour and set up variables if it does.
NumColours=$(tput colors)

if test -n "$NumColours" && test $NumColours -ge 8; then

    clear="$(tput sgr0)"
    blackN="$(tput setaf 0)";		blackB="$(tput bold setaf 0)"
    redN="$(tput setaf 1)";		redB="$(tput bold setaf 1)"
    greenN="$(tput setaf 2)";		greenB="$(tput bold setaf 2)"
    yellowN="$(tput setaf 3)";		yellowB="$(tput bold setaf 3)"
    blueN="$(tput setaf 4)";		blueB="$(tput bold setaf 4)"
    magentaN="$(tput setaf 5)";		magentaB="$(tput bold setaf 5)"
    cyanN="$(tput setaf 6)";		cyanB="$(tput bold setaf 6)"
    whiteN="$(tput setaf 7)";		whiteB="$(tput bold setaf 7)"

fi
#-----------------------------------------------------------------------------#


#--------------------------------Functions------------------------------------#

# Function to echo text using terminal colour codes.
function colEcho() {
    echo -e "$1$2$clear"
}

# Function to wait for a user keypress.
function UserWait() {
    read -n 1 -s -r -p "Press any key to continue"
    echo -e "\r                         \r"
}

# Function to ask a Yes/No question and return true or false.
function YesNo() {
	local setCheck=""
	while [[ "$setCheck" != [NnYy]* ]]; do
		read -e -p "$1" setCheck
		if [[ $setCheck == [Yy]* ]]; then
			echo true
		elif [[ $setCheck == [Nn]* ]]; then	
			echo false
		else
			colEcho $redB "Invalid input. Please enter 'Y' or 'N'." > /dev/stderr
		fi
	done
}

# Function to check we are not running with the elevated privileges.
function CheckNotElevated {
    if (( "$EUID" == "0" )); then
        colEcho $redB "ERROR: Running with elevated privileges - do not run using sudo\n"
        exit 1
    fi
}

# Function to handle dependecies list
function dependenciesHandler() {
	$sudo $pkgmgr $update_arg
	local toInstall=""
	for command in "${!depCommands[@]}"; do
		if ! [ $(which $command 2>/dev/null) ]; then
		    declare -n ref="${depCommands[$command]}"
			if [ -z "${ref[$os]}" ]; then
				toInstall+=" "${ref['default']}
			else
				toInstall+=" "${ref[$os]}
			fi
		fi
	done
	if [ "$toInstall" != "" ]; then
		if [ $os == "unknown" ]; then
			colEcho $redB "ERROR: Distro is unknown and some dependencies were not found. \n Please install the following dependencies manually: $toInstall"
			exit 1
		fi
		colEcho $cyanB "The following dependencies will be installed: $toInstall"
		UserWait
		$sudo $pkgmgr $install_arg $toInstall
	else
		colEcho $cyanB "All dependencies are already installed.\n"
	fi
}

# Function to download ventoy
function downloadVentoy() {
	local os="$1"
  	local ventoyPackage=$2
  	# Identify latest Ventoy release.
  	venver=$(wget -q -O - https://api.github.com/repos/ventoy/Ventoy/releases/latest | grep '"tag_name":' | cut -d'"' -f4)

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
}
#-----------------------------------------------------------------------------#

#----------------------------------Main Code----------------------------------#

clear
colEcho $yellowB "WELCOME TO THE MEDICAT INSTALLER.\n"

CheckNotElevated

colEcho $cyanB "This Installer will install Ventoy and Medicat.\n"
colEcho $yellowB "THIS IS IN BETA. PLEASE CONTACT MATT IN THE DISCORD FOR ALL ISSUES.\n"
colEcho $cyanB "Updated for efficiency and cross-distro use by SkeletonMan.\n"
colEcho $cyanB "Enhancements by Manganar.\n"
colEcho $cyanB "Thanks to @m3p89goljrf7fu9eched in the Medicat Discord for pointing out a bug.\n"
colEcho $cyanB "Refactored by id3v1669.\n"

# Set variables to support different distros.
# This needs to be fixed later, there is a better way, but I don't currently have the time - LordSkeletonMan
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
elif grep -qs "nixos" /etc/os-release; then
	os="nixos"
	sudo=""
	pkgmgr="nix-env"
	install_arg="-iA"
	update_arg="--upgrade"
	ventoyFS=false
elif grep -qs "alpine" /etc/os-release; then
	os="alpine"
	pkgmgr="apk"
	install_arg="add"
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
	pkgmgr="dnf"
	install_arg="install"
	update_arg="upgrade"
elif [[ -e /etc/nobara ]]; then
	colEcho $redB "gaming moment"
	os="fedora"
	pkgmgr="yum"
	install_arg="install"
	update_arg="update"
elif [[ -e /etc/arch-release ]]; then
	os="arch"
	colEcho $blueB "I use Arch btw"
	pkgmgr="pacman"
	install_arg="-S --needed --noconfirm"
	update_arg="-Syy"
else
	os="unknown"
	colEcho "WARNING: Distro not recognised - trying to continue...\n"
fi

colEcho $cyanB "Operating System Identified as:$whiteB $os"

# Ensure dependencies are installed: wget, 7z, mkntfs, and aria2c only if Medicat 7z file is not present
colEcho $cyanB "\nLocating the Medicat 7z file..."

if [[ -f "$Medicat7zFile" ]]; then
	location="$Medicat7zFile"
	colEcho $cyanB "Medicat file found:$whiteB $Medicat7zFile\n"
elif  [[ -f "$Medicat7zFull" ]]; then
	location="$Medicat7zFull"
	colEcho $cyanB "Medicat file found:$whiteB $Medicat7zFull\n"
else
	colEcho $cyanB "Please enter the location of$whiteB $Medicat7zFile$cyanB if it exists or just press enter to choose between download through MedicatUSB cdn or bittorrent."
	read location
	file_name="$(basename "$location")"
 	if [ -f "$location" ] && [ "$file_name" = "$Medicat7zFile" ]; then
 		colEcho $blueB "7Z file found:$whiteB $location"
	else
		colEcho $redB "Invalid path or name."
		colEcho $blueB "Name needed by the file:$whiteB $Medicat7zFile"
		colEcho $blueB "Name you provided:$whiteB $file_name\n"

		if $(YesNo "${blueB}Are you wanting to download medicat from bittorrent? If no the fallback is the Medicat cdn. (Y/N) ${whiteB}"); then
		colEcho $cyanB "Acquiring any dependencies for bittorent download..."

# Download dep for bittorent part
		if [ -z "$location" ] ; then
			depCommands["aria2c"]="aria"
		fi

		if $ventoyFS ; then
			dependenciesHandler
			downloadVentoy
		else
			colEcho $cyanB "INFO: Handling ventoy as a package."
			depCommands["ventoy"]="ventoy"
			dependenciesHandler
			ventoyLauncher="ventoy"
		fi

# Download the missing Medicat 7z file
		if [ -z "$location" ] ; then
			colEcho $cyanB "Starting to download Medicat via bittorrent"
			wget https://github.com/mon5termatt/medicat_installer/raw/main/download/MediCat_USB_$MedicatVersion.torrent -O medicat.torrent
			aria2c --file-allocation=none --seed-time=0 medicat.torrent
			location="$Medicat7zFull"
			colEcho $cyanB "Medicat successfully downloaded:$whiteB $location"
		fi
		else

# Download dep for cdn part
		if $ventoyFS ; then
			dependenciesHandler
			downloadVentoy
		else
			colEcho $cyanB "INFO: Handling ventoy as a package."
			depCommands["ventoy"]="ventoy"
			dependenciesHandler
			ventoyLauncher="ventoy"
		fi

# Define Download server
		srv1="https://files.medicatusb.com/files/${MedicatVersion}/${Medicat7zFile}"
		srv2="https://files.dog/OD%%20Rips/MediCat/${MedicatVersion}/${Medicat7zFile}"
		srv3="https://cat.tcbl.dev/${Medicat7zFile}"
		referer="https://installer.medicatusb.com"

		colEcho $blueB "Testing download speeds from available servers..."

		best_speed=0
		best_server=""

# Check download speed loop
		for i in 1 2 3; do
			eval server=\$srv$i
			colEcho $greenB "Testing$whiteB $server"

		result=$(curl --referer "$referer" --max-time 3 "$server" --output "test${i}.tmp" --silent --write-out "%{speed_download}")
		speed=$(( result / 1000000 ))

		if [ "$(awk -v s="$speed" -v b="$best_speed" 'BEGIN {print (s > b)}')" -eq 1 ]; then
			best_speed=$speed
			best_server=$server
		else
			colEcho $redB "Speed: connection failed"
		fi
		if [ -e test${i}.tmp ]; then
			rm test${i}.tmp
		fi
		done

		if [ -z "$best_server" ]; then
			colEcho $redB "\nERROR: No valid server found."
			colEcho $blueN "Please check your internet connection and try again."
			sleep 5
			exit 1
		fi

# Download the missing Medicat 7z file
		if [ -z "$location" ] ; then
			colEcho $cyanB "Starting to download Medicat via fastest cdn ($best_server with speed $best_speed)"
			wget --referer="$referer" --progress=bar "$best_server"
			location="$Medicat7zFile"
			colEcho $cyanB "Medicat successfully downloaded:$whiteB $location"
		fi
		fi
	fi
fi

# Check the SHA256 hash of the Medicat zip file.
colEcho $cyanB "Checking SHA256 hash of$whiteB $Medicat7zFile$cyanB..."

checksha256=$(sha256sum "$location" | awk '{print $1}')

if [[ "$checksha256" != "$Medicat256Hash" ]]; then
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
colEcho $yellowB "\nPress any key once it has been detected by your system..."
UserWait

colEcho $yellowB "Please Find the ID of your USB below:"

lsblk --nodeps --output "NAME,SIZE,VENDOR,MODEL,SERIAL" | grep -v loop

colEcho $yellowB "Enter the device for the USB drive NOT INCLUDING /dev/ OR the Number After."
colEcho $yellowB "for example enter sda or sdb"
read letter

drive=/dev/$letter
drive2="$drive""1"

if $(YesNo "You want to install Ventoy and Medicat to $drive / $drive2? (Y/N) "); then
	colEcho $cyanB "Installation confirmed and will commence in 5 seconds..."
	sleep 5
else
	colEcho $yellowB "Installation Cancelled."
	exit 0
fi

colEcho $cyanB "Installing Ventoy on$whiteB $drive"

colEcho $blueB "MBR at max can do up to approximately 2.2 TB and will work with older BIOS systems and UEFI systems that support legacy operating systems. GPT can do up to 18 exabytes and will work with UEFI systems."
if $(YesNo "Device partition layout defaults to MBR.  Would you like to use GPT instead? (Y/N)"); then
	colEcho $yellowB "Using GPT"
# Before launching ventoy install, moving to ventoy dir
	if [ -d ventoy ]; then
		colEcho $blueB "Moving to ventoy dir"
		cd ventoy
	else
		colEcho $redB "Ventoy directory not fount exiting..."
		wait 5
		exit 1
	fi
	sudo $ventoyLauncher -I -g $drive
	if [ "$?" != "0" ]; then
		colEcho $redB "ERROR: Unable to install Ventoy. Exiting..."
		exit 1
	fi
else
	colEcho $yellowB "Using MBR"
# Before launching ventoy install, moving to ventoy dir
	if [ -d ventoy ]; then
		colEcho $blueB "Moving to ventoy dir"
		cd ventoy
	else
		colEcho $redB "Ventoy directory not fount exiting..."
		wait 5
		exit 1
	fi
	sudo $ventoyLauncher -I $drive
	if [ "$?" != "0" ]; then
		colEcho $redB "ERROR: Unable to install Ventoy. Exiting..."
		exit 1
	fi
fi

# Back to medicat folder after Ventoy Install
	colEcho $blueB "Back to the medicat folder"
	cd ..

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

if $(YesNo "Would you like to unmount ./MedicatUSB? (Y/N) "); then
	colEcho $cyanB "Unmounting MedicatUSB..."
	sudo umount ./MedicatUSB
	colEcho $cyanB "Unmounted."
else
	colEcho $cyanB "MedicatUSB will not be unmounted."
fi


# #!/usr/bin/env bash

# # Script Version 0010

# #--------------------------------Variables------------------------------------#

# # Key variables used throughout the script to make maintenance easier.
# MedicatVersion="v21.12"
# Medicat256Hash='a306331453897d2b20644ca9334bb0015b126b8647cecec8d9b2d300a0027ea4'
# Medicat7zFile="MediCat.USB.$MedicatVersion.7z"
# Medicat7zFull=''MediCat\ USB\ $MedicatVersion/MediCat.USB.$MedicatVersion.7z''
# #Google Drive Files.
# Medicat7zGD1=""
# Medicat7zGD1Hash=""


# # Dependencies
# declare -A depCommands
# depCommands["curl"]="curl"
# depCommands["jq"]="jq"
# depCommands["7z"]="zip"
# depCommands["mkfs.vfat"]="mkfs"
# depCommands["mkntfs"]="ntfs"
# declare -A curl
# curl["nixos"]="nixos.curl"
# curl["default"]="curl"
# declare -A jq
# curl["nixos"]="nixos.jq"
# curl["default"]="jq"
# declare -A zip
# zip["arch"]="p7zip"
# zip["nixos"]="nixos.p7zip"
# zip["fedora"]="p7zip p7zip-plugins"
# zip["nobara"]="p7zip-full p7zip-plugins"
# zip["centos"]="p7zip p7zip-plugins"
# zip["alpine"]="7zip"
# zip["opensuse"]="7zip"
# zip["default"]="p7zip-full"
# declare -A mkfs
# mkfs["nixos"]="nixos.dosfstools"
# mkfs["default"]="dosfstools"
# declare -A ntfs
# ntfs["centos"]="ntfsprogs"
# ntfs["nixos"]="nixos.ntfs3g"
# ntfs["default"]="ntfs-3g"
# declare -A aria
# aria["nixos"]="nixos.aria"
# aria["default"]="aria2"
# declare -A ventoy
# ventoy["nixos"]="nixos.ventoy-full"
# ventoy["default"]="ventoy"

# # Other Variables
# sudo="sudo" # By default use sudo with package manager
# ventoyFS=true  # By default install ventoy from github(FromSource)
# ventoyLauncher="sh ./ventoy/Ventoy2Disk.sh" # By default use the ventoy script

# # Check if the terminal supports colour and set up variables if it does.
# NumColours=$(tput colors)

# if test -n "$NumColours" && test $NumColours -ge 8; then

#     clear="$(tput sgr0)"
#     blackN="$(tput setaf 0)";		blackB="$(tput bold setaf 0)"
#     redN="$(tput setaf 1)";		redB="$(tput bold setaf 1)"
#     greenN="$(tput setaf 2)";		greenB="$(tput bold setaf 2)"
#     yellowN="$(tput setaf 3)";		yellowB="$(tput bold setaf 3)"
#     blueN="$(tput setaf 4)";		blueB="$(tput bold setaf 4)"
#     magentaN="$(tput setaf 5)";		magentaB="$(tput bold setaf 5)"
#     cyanN="$(tput setaf 6)";		cyanB="$(tput bold setaf 6)"
#     whiteN="$(tput setaf 7)";		whiteB="$(tput bold setaf 7)"

# fi
# #-----------------------------------------------------------------------------#


# #--------------------------------Functions------------------------------------#

# # Function to echo text using terminal colour codes.
# function colEcho() {
#     echo -e "$1$2$clear"
# }

# # Function to wait for a user keypress.
# function UserWait() {
#     read -n 1 -s -r -p "Press any key to continue"
#     echo -e "\r                         \r"
# }

# # Function to ask a Yes/No question and return true or false.
# function YesNo() {
# 	local setCheck=""
# 	while [[ "$setCheck" != [NnYy]* ]]; do
# 		read -e -p "$1" setCheck
# 		if [[ $setCheck == [Yy]* ]]; then
# 			echo true
# 		elif [[ $setCheck == [Nn]* ]]; then
# 			echo false
# 		else
# 			colEcho $redB "Invalid input. Please enter 'Y' or 'N'." > /dev/stderr
# 		fi
# 	done
# }

# # Function to check we are not running with the elevated privileges.
# function CheckNotElevated {
#     if (( "$EUID" == "0" )); then
#         colEcho $redB "ERROR: Running with elevated privileges - do not run using sudo\n"
#         exit 1
#     fi
# }

# # Function to handle dependecies list
# function dependenciesHandler() {
# 	$sudo $pkgmgr $update_arg
# 	local toInstall=""
# 	for command in "${!depCommands[@]}"; do
# 		if ! [ $(which $command 2>/dev/null) ]; then
# 		    declare -n ref="${depCommands[$command]}"
# 			if [ -z "${ref[$os]}" ]; then
# 				toInstall+=" "${ref['default']}
# 			else
# 				toInstall+=" "${ref[$os]}
# 			fi
# 		fi
# 	done
# 	if [ "$toInstall" != "" ]; then
# 		if [ $os == "unknown" ]; then
# 			colEcho $redB "ERROR: Distro is unknown and some dependencies were not found. \n Please install the following dependencies manually: $toInstall"
# 			exit 1
# 		fi
# 		colEcho $cyanB "The following dependencies will be installed: $toInstall"
# 		UserWait
# 		$sudo $pkgmgr $install_arg $toInstall
# 	else
# 		colEcho $cyanB "All dependencies are already installed.\n"
# 	fi
# }

# # Function to download ventoy
# function downloadVentoy() {
# 	local os="$1"
# 	local ventoyPackage=$2
# 	# Identify latest Ventoy release.
# 	venver="$(curl -L https://api.github.com/repos/ventoy/Ventoy/releases/latest | jq -r '.tag_name')"

# 	# Download latest verion of Ventoy.
# 	colEcho $cyanB "\nDownloading Ventoy version:$whiteB ${venver}"
# 	curl -L "https://github.com/ventoy/Ventoy/releases/download/${venver}/ventoy-${venver:1}-linux.tar.gz" --output ventoy.tar.gz

# 	colEcho $cyanB "\nExtracting Ventoy..."
# 	tar -xf ventoy.tar.gz

# 	colEcho $cyanB "Removing the extracted Ventoy tar.gz file..."
# 	rm -rf ventoy.tar.gz

# 	# Remove the ./ventoy folder if it exists before renaming ventoy folder.
# 	if [ -d ./ventoy ]; then
# 		colEcho $cyanB "Removing the previous ./ventoy folder..."
# 		rm -rf ./ventoy/
# 	fi

# 	colEcho $cyanB "Renaming Ventoy folder to remove the version number..."
# 	mv ventoy-${venver: -6} ventoy
# }
# #-----------------------------------------------------------------------------#

# #----------------------------------Main Code----------------------------------#

# clear
# colEcho $yellowB "WELCOME TO THE MEDICAT INSTALLER.\n"

# CheckNotElevated

# colEcho $cyanB "This installer will install Ventoy and Medicat.\n"
# colEcho $yellowB "THIS IS IN BETA. PLEASE CONTACT MATT IN THE DISCORD FOR ALL ISSUES.\n"
# colEcho $cyanB "Updated for efficiency and cross-distro use by SkeletonMan.\n"
# colEcho $cyanB "Enhancements by Manganar.\n"
# colEcho $cyanB "Thanks to @m3p89goljrf7fu9eched in the Medicat Discord for pointing out a bug.\n"
# colEcho $cyanB "Refactored by id3v1669.\n"

# # Set variables to support different distros.
# # This needs to be fixed later, there is a better way, but I don't currently have the time - LordSkeletonMan
# if grep -qs "ubuntu" /etc/os-release; then
# 	os="ubuntu"
# 	pkgmgr="apt"
# 	install_arg="install"
# 	update_arg="update"
# elif grep -qs "freebsd" /etc/os-release; then
# 	os="freebsd"
# 	pkgmgr="pkg"
# 	install_arg="install"
# 	update_arg="update"
# elif grep -qs "nixos" /etc/os-release; then
# 	os="nixos"
# 	sudo=""
# 	pkgmgr="nix-env"
# 	install_arg="-iA"
# 	update_arg="--upgrade"
# 	ventoyFS=false
# elif grep -qs "alpine" /etc/os-release; then
# 	os="alpine"
# 	pkgmgr="apk"
# 	install_arg="add"
# 	update_arg="update"
# elif [[ -e /etc/debian_version ]]; then
# 	os="debian"
# 	pkgmgr="apt"
# 	install_arg="install"
# 	update_arg="update"
# elif [[ -e /etc/almalinux-release || -e /etc/rocky-release || -e /etc/centos-release ]]; then
# 	colEcho $redB "Fuck Red-Hat for putting source code behind paywalls."
# 	os="centos"
# 	pkgmgr="yum"
# 	install_arg="install"
# 	update_arg="update"
# elif [[ -e /etc/fedora-release ]]; then
# 	os="fedora"
# 	pkgmgr="yum"
# 	install_arg="install"
# 	update_arg="update"
# elif [[ -e /etc/nobara ]]; then
# 	colEcho $redB "gaming moment"
# 	os="fedora"
# 	pkgmgr="yum"
# 	install_arg="install"
# 	update_arg="update"
# elif [[ -e /etc/arch-release ]]; then
# 	os="arch"
# 	colEcho $blueB "I use Arch btw"
# 	pkgmgr="pacman"
# 	install_arg="-S --needed --noconfirm"
# 	update_arg="-Syy"
# elif grep -qs "opensuse-tumbleweed" /etc/os-release; then
# 	os="opensuse"
# 	pkgmgr="zypper"
# 	install_arg="install"
# 	update_arg="update"
# else
# 	os="unknown"
# 	colEcho "WARNING: Distro not recognised - trying to continue...\n"
# fi

# colEcho $cyanB "Operating System Identified as:$whiteB $os"

# # Ensure dependencies are installed: curl, jq, 7z, mkntfs, and aria2c only if Medicat 7z file is not present
# colEcho $cyanB "\nLocating the Medicat 7z file..."

# if [[ -f "$Medicat7zFile" ]]; then
# 	location="$Medicat7zFile"
# 	colEcho $cyanB "Medicat file found:$whiteB $Medicat7zFile\n"
# elif  [[ -f "$Medicat7zFull" ]]; then
# 	location="$Medicat7zFull"
# 	colEcho $cyanB "Medicat file found:$whiteB $Medicat7zFull\n"
# else
# 	colEcho $cyanB "Please enter the location of$whiteB $Medicat7zFile$cyanB if it exists or just press enter to download it via bittorrent."
# 	read location
# fi

# colEcho $cyanB "Acquiring any dependencies..."

# if [ -z "$location" ] ; then
# 	depCommands["aria2c"]="aria"
# fi

# if $ventoyFS ; then
#     dependenciesHandler
# 	downloadVentoy
# else
# 	colEcho $cyanB "INFO: Handling Ventoy as a package."
# 	depCommands["ventoy"]="ventoy"
# 	dependenciesHandler
# 	ventoyLauncher="ventoy"
# fi

# # Download the missing Medicat 7z file
# if [ -z "$location" ] ; then
# 	colEcho $cyanB "Starting to download Medicat via bittorrent"
# 	curl -L https://github.com/mon5termatt/medicat_installer/raw/main/download/MediCat_USB_$MedicatVersion.torrent --output medicat.torrent
# 	aria2c --file-allocation=none --seed-time=0 medicat.torrent
# 	location="$Medicat7zFull"
# 	colEcho $cyanB "Medicat successfully downloaded:$whiteB $location"
# fi

# # Check the SHA256 hash of the Medicat zip file.
# colEcho $cyanB "Checking SHA256 hash of$whiteB $Medicat7zFile$cyanB..."

# checksha256=$(sha256sum "$location" | awk '{print $1}')

# if [[ "$checksha256" != "$Medicat256Hash" ]]; then
# 	colEcho $redB "$Medicat7zFile SHA256 hash does not match."
# 	colEcho $redB "File may be corrupted or compromised."
# 	colEcho $cyanB "Hash is$whiteB $checksha256"
# 	colEcho $cyanB "Exiting..."
# 	exit 1
# else
# 	colEcho $greenB "$Medicat7zFile SHA256 hash matches."
# 	colEcho $cyanB "Hash is$whiteB $checksha256"
# 	colEcho $cyanB "Safe to proceed..."
# fi

# # Advise user to connect and select the required USB device.
# colEcho $yellowB "\nPlease plug your USB in now if it is not already connected..."
# colEcho $yellowB "\nPress any key once it has been detected by your system..."
# UserWait

# colEcho $yellowB "Please find the ID of your USB below:"

# lsblk --nodeps --output "NAME,SIZE,VENDOR,MODEL,SERIAL" | grep -v loop

# colEcho $yellowB "Enter the device for the USB drive NOT INCLUDING /dev/ OR the number after."
# colEcho $yellowB "for example enter sda or sdb"
# read letter

# drive=/dev/$letter
# drive2="$drive""1"

# if $(YesNo "You want to install Ventoy and Medicat to $drive / $drive2? (Y/N) "); then
# 	colEcho $cyanB "Installation confirmed and will commence in 5 seconds..."
# 	sleep 5
# else
# 	colEcho $yellowB "Installation cancelled."
# 	exit 0
# fi

# colEcho $cyanB "Installing Ventoy on$whiteB $drive"

# colEcho $blueB "MBR at max can do up to approximately 2.2 TB and will work with older BIOS systems and UEFI systems that support legacy operating systems. GPT can do up to 18 exabytes and will work with UEFI systems."
# if $(YesNo "Device partition layout defaults to MBR.  Would you like to use GPT instead? (Y/N)"); then
# 	colEcho $yellowB "Using GPT"
# 	sudo $ventoyLauncher -I -g $drive
# 	if [ "$?" != "0" ]; then
# 		colEcho $redB "ERROR: Unable to install Ventoy. Exiting..."
# 		exit 1
# 	fi
# else
# 	colEcho $yellowB "Using MBR"
# 	sudo $ventoyLauncher -I $drive
# 	if [ "$?" != "0" ]; then
# 		colEcho $redB "ERROR: Unable to install Ventoy. Exiting..."
# 		exit 1
# 	fi
# fi

# colEcho $cyanB "Unmounting drive$whiteB $drive"
# sudo umount $drive

# colEcho $cyanB "Creating Medicat NTFS file system on drive$whiteB $drive2"
# sudo mkntfs --fast --label Medicat $drive2

# # Create a mountpoint folder for the Medicat NTFS volume
# if ! [[ -d MedicatUSB/ ]] ; then
# 	colEcho $cyanB "Creating a mountpoint for the Medicat NTFS volume..."
# 	mkdir MedicatUSB
# fi

# colEcho $cyanB "Mounting Medicat NTFS volume..."
# sudo mount $drive2 ./MedicatUSB

# colEcho $cyanB "Extracting Medicat to NTFS volume..."
# 7z x -O./MedicatUSB "$location"

# colEcho $cyanB "MedicatUSB has been created."

# if $(YesNo "Would you like to unmount ./MedicatUSB? (Y/N) "); then
# 	colEcho $cyanB "Unmounting MedicatUSB..."
# 	sudo umount ./MedicatUSB
# 	colEcho $cyanB "Unmounted."
# else
# 	colEcho $cyanB "MedicatUSB will not be unmounted."
# fi
