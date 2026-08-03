#!/usr/bin/env bash

# Script Version 0012
#
# Changelog
# ---------
# 0012
#   - Offer download method choice: direct HTTP (aria2c multi-connection),
#     BitTorrent, or local path when the MediCat archive is missing.
#   - Primary direct mirror: files.medicatusb.com (aria2c -x32 -s32 -k1M -c).
#   - Fallback mirror: files.dog (known broken SSL; aria2c without cert check).
# 0011
#   - Fix YesNo infinite loop: empty/EOF input no longer spins forever on
#     "Invalid input". read failures exit cleanly; non-TTY stdin falls back
#     to /dev/tty (curl|bash, pipelines).
#   - YesNo returns exit status (0=yes, 1=no) instead of echoing true/false;
#     call sites no longer use command substitution $(YesNo ...).
#   - Trim CR/whitespace; accept Y/Yes and N/No (case-insensitive prefix).

#
#--------------------------------Variables------------------------------------#

# Key variables used throughout the script to make maintenance easier.
MedicatVersion="v21.12"
Medicat256Hash='a306331453897d2b20644ca9334bb0015b126b8647cecec8d9b2d300a0027ea4'
Medicat7zFile="MediCat.USB.$MedicatVersion.7z"
Medicat7zFull=''MediCat\ USB\ $MedicatVersion/MediCat.USB.$MedicatVersion.7z''
# Direct download mirrors (order = try order). files.dog needs insecure SSL.
MedicatUrlMedicatusb="https://files.medicatusb.com/files/${MedicatVersion}/${Medicat7zFile}"
MedicatUrlFilesDog="https://files.dog/OD%20Rips/MediCat/${MedicatVersion}/${Medicat7zFile}"
MedicatTorrentUrl="https://github.com/mon5termatt/medicat_installer/raw/main/download/MediCat_USB_${MedicatVersion}.torrent"
needMedicatDownload=false
location=""
DownloadMethod=""

# Dependencies
declare -A depCommands
depCommands["wget"]="wget"
depCommands["7z"]="zip"
depCommands["mkfs.vfat"]="mkfs"
depCommands["mkntfs"]="ntfs"
depCommands["mkfs.exfat"]="exfat"
depCommands["parted"]="parted"
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
zip["void"]="7zip"
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
declare -A parted
parted["default"]="parted"
declare -A exfat
exfat["fedora"]="exfatprogfs"
exfat["default"]="exfat-utils"

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

# Function to ask a Yes/No question.
# Returns 0 for Yes, 1 for No. Exit 1 on unrecoverable input failure.
# Prefer: if YesNo "prompt? (Y/N) "; then ...
# Avoid: if $(YesNo ...); then  # subshell + captures stdout; can hide prompts/EOF loops
function YesNo() {
	local setCheck=""
	local inputFd=0

	# Prompt on the real terminal when stdin is redirected (curl|bash, pipelines, etc.).
	# Without this, `read` can hit EOF immediately and spin on "Invalid input" forever.
	if [[ ! -t 0 ]] && [[ -r /dev/tty ]]; then
		inputFd=3
		exec 3</dev/tty
	fi

	while true; do
		if ! read -r -p "$1" setCheck <&"$inputFd"; then
			colEcho $redB "ERROR: Failed to read input (EOF). Exiting..." >&2
			[[ "$inputFd" -ne 0 ]] && exec 3<&-
			exit 1
		fi

		# Strip CR (Windows / paste) and surrounding whitespace.
		setCheck="${setCheck//$'\r'/}"
		setCheck="${setCheck#"${setCheck%%[![:space:]]*}"}"
		setCheck="${setCheck%"${setCheck##*[![:space:]]}"}"

		case "$setCheck" in
			[Yy]|[Yy][Ee][Ss])
				[[ "$inputFd" -ne 0 ]] && exec 3<&-
				return 0
				;;
			[Nn]|[Nn][Oo])
				[[ "$inputFd" -ne 0 ]] && exec 3<&-
				return 1
				;;
			*)
				colEcho $redB "Invalid input. Please enter 'Y' or 'N'." >&2
				;;
		esac
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

	colEcho $cyanB "Removing the extracted Ventoy tar.gz file..."
	rm -rf ventoy.tar.gz

	# Remove the ./ventoy folder if it exists before renaming ventoy folder.
	if [ -d ./ventoy ]; then
		colEcho $cyanB "Removing the previous ./ventoy folder..."
		rm -rf ./ventoy/
	fi

	colEcho $cyanB "Renaming ventoy folder to remove the version number..."
	mv ventoy-${venver: -6} ventoy
}

# Read a line from the terminal (uses /dev/tty when stdin is redirected).
function ReadPrompt() {
	local __prompt="$1"
	local __resultVar="$2"
	local __line=""
	local inputFd=0

	if [[ ! -t 0 ]] && [[ -r /dev/tty ]]; then
		inputFd=3
		exec 3</dev/tty
	fi

	if ! read -r -p "$__prompt" __line <&"$inputFd"; then
		[[ "$inputFd" -ne 0 ]] && exec 3<&-
		colEcho $redB "ERROR: Failed to read input (EOF). Exiting..." >&2
		exit 1
	fi
	[[ "$inputFd" -ne 0 ]] && exec 3<&-

	__line="${__line//$'\r'/}"
	printf -v "$__resultVar" '%s' "$__line"
}

# Download the MediCat archive over HTTP with aria2c multi-connection.
# $1=url  $2=mirror name  $3=insecure (true|false) — skip TLS verify when true
function downloadMedicatHttp() {
	local url="$1"
	local mirrorName="$2"
	local insecure="$3"
	local ariaArgs=(-x32 -s32 -k1M -c --file-allocation=none --summary-interval=5 -o "$Medicat7zFile")

	colEcho $cyanB "\nDirect download from$whiteB $mirrorName"
	colEcho $cyanB "URL:$whiteB $url"

	if [[ "$insecure" == "true" ]]; then
		colEcho $yellowB "Note: this mirror has known TLS certificate issues; certificate checking is disabled for this download only."
		ariaArgs+=(--check-certificate=false)
	fi

	if aria2c "${ariaArgs[@]}" -- "$url"; then
		if [[ -f "$Medicat7zFile" ]]; then
			location="$Medicat7zFile"
			colEcho $greenB "Download finished:$whiteB $location"
			return 0
		fi
		colEcho $redB "aria2c reported success but $Medicat7zFile was not found."
	else
		colEcho $redB "Download from $mirrorName failed."
	fi
	return 1
}

# Download MediCat via BitTorrent (aria2c).
function downloadMedicatTorrent() {
	colEcho $cyanB "\nStarting MediCat download via BitTorrent..."
	if ! wget -q --show-progress "$MedicatTorrentUrl" -O medicat.torrent; then
		colEcho $redB "ERROR: Failed to download torrent file from:$whiteB $MedicatTorrentUrl"
		return 1
	fi

	if ! aria2c --file-allocation=none --seed-time=0 --summary-interval=15 medicat.torrent; then
		colEcho $redB "ERROR: BitTorrent download failed."
		rm -f medicat.torrent
		return 1
	fi
	rm -f medicat.torrent

	if [[ -f "$Medicat7zFull" ]]; then
		location="$Medicat7zFull"
	elif [[ -f "$Medicat7zFile" ]]; then
		location="$Medicat7zFile"
	else
		colEcho $redB "ERROR: Torrent finished but $Medicat7zFile was not found."
		return 1
	fi

	colEcho $greenB "Medicat successfully downloaded:$whiteB $location"
	return 0
}

# Try direct mirrors in order, then offer torrent on total failure.
function downloadMedicatDirect() {
	# Primary official CDN — preferred.
	if downloadMedicatHttp "$MedicatUrlMedicatusb" "files.medicatusb.com" "false"; then
		return 0
	fi

	# files.dog often fails with wget TLS errors; aria2c + --check-certificate=false is the workaround.
	colEcho $yellowB "\nPrimary mirror failed. Trying fallback (files.dog)..."
	if downloadMedicatHttp "$MedicatUrlFilesDog" "files.dog" "true"; then
		return 0
	fi

	colEcho $redB "\nAll direct download mirrors failed."
	if YesNo "Try BitTorrent instead? (Y/N) "; then
		downloadMedicatTorrent
		return $?
	fi
	return 1
}

# Prompt how to obtain the archive when it is not already on disk.
function chooseMedicatSource() {
	local choice=""
	local pathInput=""

	while true; do
		colEcho $cyanB "\nHow would you like to get$whiteB $Medicat7zFile$cyanB?"
		colEcho $whiteB "  1)$cyanB Direct download (multi-connection HTTPS via aria2c) [recommended]"
		colEcho $whiteB "  2)$cyanB BitTorrent"
		colEcho $whiteB "  3)$cyanB Enter path to an existing file"
		ReadPrompt "Choice [1/2/3]: " choice
		choice="${choice#"${choice%%[![:space:]]*}"}"
		choice="${choice%"${choice##*[![:space:]]}"}"

		case "$choice" in
			1)
				DownloadMethod="direct"
				needMedicatDownload=true
				return 0
				;;
			2)
				DownloadMethod="torrent"
				needMedicatDownload=true
				return 0
				;;
			3)
				ReadPrompt "Path to $Medicat7zFile: " pathInput
				pathInput="${pathInput//$'\r'/}"
				pathInput="${pathInput#"${pathInput%%[![:space:]]*}"}"
				pathInput="${pathInput%"${pathInput##*[![:space:]]}"}"
				# Expand ~ if present
				pathInput="${pathInput/#\~/$HOME}"
				if [[ -z "$pathInput" ]]; then
					colEcho $redB "No path entered."
					continue
				fi
				if [[ ! -f "$pathInput" ]]; then
					colEcho $redB "File not found:$whiteB $pathInput"
					continue
				fi
				location="$pathInput"
				DownloadMethod="local"
				needMedicatDownload=false
				colEcho $cyanB "Using local file:$whiteB $location"
				return 0
				;;
			*)
				colEcho $redB "Invalid choice. Enter 1, 2, or 3."
				;;
		esac
	done
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
	alias mkexfatfs=mkfs.exfat # Wtf Ventoy?
elif [[ -e /etc/nobara ]]; then
	colEcho $redB "gaming moment"
	os="fedora"
	pkgmgr="yum"
	install_arg="install"
	update_arg="update"
	alias mkexfatfs=mkfs.exfat
elif [[ -e /etc/arch-release ]]; then
	os="arch"
	colEcho $blueB "I use Arch btw"
	pkgmgr="pacman"
	install_arg="-S --needed --noconfirm"
	update_arg="-Syy"
elif grep -qs "void" /etc/os-release; then
	os="void"
	colEcho $greenB "Enter the void"
	pkgmgr="xbps-install"
	install_arg="-S" # Technically don't need one
	update_arg="-S"
else
	os="unknown"
	colEcho "WARNING: Distro not recognised - trying to continue...\n"
fi

colEcho $cyanB "Operating System Identified as:$whiteB $os"

# Ensure dependencies are installed: wget, 7z, mkntfs, and aria2c if MediCat will be downloaded
colEcho $cyanB "\nLocating the Medicat 7z file..."

if [[ -f "$Medicat7zFile" ]]; then
	location="$Medicat7zFile"
	colEcho $cyanB "Medicat file found:$whiteB $Medicat7zFile\n"
elif [[ -f "$Medicat7zFull" ]]; then
	location="$Medicat7zFull"
	colEcho $cyanB "Medicat file found:$whiteB $Medicat7zFull\n"
else
	colEcho $yellowB "Medicat archive not found in the current directory."
	chooseMedicatSource
fi

colEcho $cyanB "Acquiring any dependencies..."

if $needMedicatDownload ; then
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

# Download the missing Medicat 7z file when requested
if $needMedicatDownload ; then
	case "$DownloadMethod" in
		direct)
			if ! downloadMedicatDirect; then
				colEcho $redB "ERROR: Unable to obtain MediCat archive. Exiting..."
				exit 1
			fi
			;;
		torrent)
			if ! downloadMedicatTorrent; then
				colEcho $redB "ERROR: Unable to obtain MediCat archive. Exiting..."
				exit 1
			fi
			;;
		*)
			colEcho $redB "ERROR: Unknown download method. Exiting..."
			exit 1
			;;
	esac
fi

if [[ -z "$location" ]] || [[ ! -f "$location" ]]; then
	colEcho $redB "ERROR: MediCat archive path is missing or invalid:$whiteB ${location:-"(empty)"}"
	exit 1
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

if YesNo "You want to install Ventoy and Medicat to $drive / $drive2? (Y/N) "; then
	colEcho $cyanB "Installation confirmed and will commence in 5 seconds..."
	sleep 5
else
	colEcho $yellowB "Installation Cancelled."
	exit 0
fi

colEcho $cyanB "Installing Ventoy on$whiteB $drive"

colEcho $blueB "MBR at max can do up to approximately 2.2 TB and will work with older BIOS systems and UEFI systems that support legacy operating systems. GPT can do up to 18 exabytes and will work with UEFI systems."

cd ventoy #Thanks camellia from Medicat Discord for helping work around Ventoy's bullshit

if YesNo "Device partition layout defaults to MBR.  Would you like to use GPT instead? (Y/N) "; then
	colEcho $yellowB "Using GPT"
	sudo $ventoyLauncher -I -g $drive
	if [ "$?" != "0" ]; then
		colEcho $redB "ERROR: Unable to install Ventoy. Exiting..."
		exit 1
	fi
else
	colEcho $yellowB "Using MBR"
	sudo $ventoyLauncher -I $drive
	if [ "$?" != "0" ]; then
		colEcho $redB "ERROR: Unable to install Ventoy. Exiting..."
		exit 1
	fi
fi

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
sudo mount $drive2 ./MedicatUSB -o user,auto,fmask=0111,dmask=0000

colEcho $cyanB "Extracting Medicat to NTFS volume..."
7z x -o./MedicatUSB "$location"

colEcho $cyanB "MedicatUSB has been created."

if YesNo "Would you like to unmount ./MedicatUSB? (Y/N) "; then
	colEcho $cyanB "Unmounting MedicatUSB..."
	sudo umount ./MedicatUSB
	colEcho $cyanB "Unmounted."
else
	colEcho $cyanB "MedicatUSB will not be unmounted."
fi
