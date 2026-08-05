#!/usr/bin/env bash

ScriptVersion="0022"

# See CHANGELOG.md for changes.
#
#--------------------------------Variables------------------------------------#

# Key variables used throughout the script to make maintenance easier.
MedicatVersion="v21.12"
Medicat256Hash='a306331453897d2b20644ca9334bb0015b126b8647cecec8d9b2d300a0027ea4'
Medicat7zFile="MediCat.USB.$MedicatVersion.7z"
Medicat7zFull="MediCat USB ${MedicatVersion}/MediCat.USB.${MedicatVersion}.7z"
# Full archive is ~21.4 GiB; reject clearly truncated downloads before hashing.
Medicat7zMinBytes=$((20 * 1024 * 1024 * 1024))
# Google Drive / Mega multi-volume zip (same MD5s as the Windows C++ installer).
# 7z extracts the set when pointed at the first volume (.001).
MedicatSplitBase="MediCat.USB.${MedicatVersion}.zip"
MedicatSplitFirst="${MedicatSplitBase}.001"
MedicatSplitSizes=(4290772992 4290772992 4290772992 4290772992 4290772992 2917026620)
MedicatSplitMd5s=(
	277793dcf0e31736f0790162a89d07c9
	a4700261f32d4df5092c5dd5ea6aaa2d
	6b523273c5c7ed1ddc5920dec95b8509
	35fac6ff4902d62e6e5fd2dab1050a3f
	7f416a7d9ff0051ae75bbf44a411b8e4
	32d84a280af91ae408f55a7722ee6818
)
# Direct download mirrors (order = try order). files.dog needs insecure SSL.
MedicatUrlMedicatusb="https://files.medicatusb.com/files/${MedicatVersion}/${Medicat7zFile}"
MedicatUrlFilesDog="https://files.dog/OD%20Rips/MediCat/${MedicatVersion}/${Medicat7zFile}"
MedicatTorrentUrl="https://github.com/mon5termatt/medicat_installer/raw/main/download/MediCat_USB_${MedicatVersion}.torrent"
needMedicatDownload=false
location=""
# solid7z | splitzip
archiveKind=""
DownloadMethod=""

# Free-space gates (bytes). Low disk is a common cause of truncated .7z / failed apt / failed extract.
MedicatDownloadMinFreeBytes=$((22 * 1024 * 1024 * 1024)) # room for the archive in the working dir
MedicatWorkMinFreeBytes=$((1 * 1024 * 1024 * 1024))      # deps, Ventoy tar, mountpoint on cwd
MedicatUsbMinFreeBytes=$((26 * 1024 * 1024 * 1024))      # extract onto Ventoy data partition (32GB sticks OK)

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
# Bookworm+/Ubuntu: exfat-utils removed; exfatprogs provides mkfs.exfat
exfat["ubuntu"]="exfatprogs"
exfat["debian"]="exfatprogs"
exfat["fedora"]="exfatprogs"
exfat["centos"]="exfatprogs"
exfat["arch"]="exfatprogs"
exfat["alpine"]="exfatprogs"
exfat["void"]="exfatprogs"
exfat["default"]="exfatprogs"

# Other Variables
sudo="sudo" # By default use sudo with package manager
ventoyFS=true  # By default install ventoy from github(FromSource)
ventoyLauncher="sh ./Ventoy2Disk.sh" # By default use the ventoy script

# Colour / terminal (never name the reset var "clear" — masks /usr/bin/clear)
ansiReset=""
blackN=""; blackB=""
redN=""; redB=""
greenN=""; greenB=""
yellowN=""; yellowB=""
blueN=""; blueB=""
magentaN=""; magentaB=""
cyanN=""; cyanB=""
whiteN=""; whiteB=""

NumColours=$(tput colors 2>/dev/null || echo 0)
if [[ -n "$NumColours" && "$NumColours" -ge 8 ]]; then
	ansiReset="$(tput sgr0 2>/dev/null || true)"
	blackN="$(tput setaf 0 2>/dev/null || true)";  blackB="$(tput bold 2>/dev/null; tput setaf 0 2>/dev/null || true)"
	redN="$(tput setaf 1 2>/dev/null || true)";     redB="$(tput bold 2>/dev/null; tput setaf 1 2>/dev/null || true)"
	greenN="$(tput setaf 2 2>/dev/null || true)";   greenB="$(tput bold 2>/dev/null; tput setaf 2 2>/dev/null || true)"
	yellowN="$(tput setaf 3 2>/dev/null || true)";  yellowB="$(tput bold 2>/dev/null; tput setaf 3 2>/dev/null || true)"
	blueN="$(tput setaf 4 2>/dev/null || true)";    blueB="$(tput bold 2>/dev/null; tput setaf 4 2>/dev/null || true)"
	magentaN="$(tput setaf 5 2>/dev/null || true)"; magentaB="$(tput bold 2>/dev/null; tput setaf 5 2>/dev/null || true)"
	cyanN="$(tput setaf 6 2>/dev/null || true)";    cyanB="$(tput bold 2>/dev/null; tput setaf 6 2>/dev/null || true)"
	whiteN="$(tput setaf 7 2>/dev/null || true)";   whiteB="$(tput bold 2>/dev/null; tput setaf 7 2>/dev/null || true)"
fi
#-----------------------------------------------------------------------------#


#--------------------------------Functions------------------------------------#

# Print optional colour + message. Always treats args as data (never exec).
# Usage: colEcho "plain text"
#        colEcho "$cyanB" "coloured text with ${whiteB}embeds"
function colEcho() {
	if [[ $# -ge 2 ]]; then
		printf '%b\n' "${1}${2}${ansiReset}"
	else
		printf '%b\n' "${1-}${ansiReset}"
	fi
}

# Open interactive input: always prefer /dev/tty so pipes / curl|bash / leftover
# stdin never make keypress prompts return immediately empty.
# Sets global _promptFd (0 = stdin, or an open fd number). Call promptClose after.
_promptFd=0
function promptOpen() {
	_promptFd=0
	if [[ -r /dev/tty ]]; then
		exec 3</dev/tty
		_promptFd=3
	fi
}
function promptClose() {
	if [[ "${_promptFd:-0}" -ne 0 ]]; then
		exec 3<&-
		_promptFd=0
	fi
}

# Function to wait for a user keypress (real terminal, not redirected stdin).
function UserWait() {
	local promptText="${1:-Press any key to continue}"
	promptOpen
	# Prompt on stderr so it still shows if stdout is piped; read one key from TTY.
	printf '%s' "$promptText" >&2
	if ! read -n 1 -s -r <&"$_promptFd"; then
		promptClose
		colEcho $redB "ERROR: Failed to read keypress (EOF). Exiting..." >&2
		exit 1
	fi
	printf '\r                         \r' >&2
	promptClose
}

# Function to ask a Yes/No question.
# Returns 0 for Yes, 1 for No. Exit 1 on unrecoverable input failure.
# Prefer: if YesNo "prompt? (Y/N) "; then ...
function YesNo() {
	local setCheck=""
	promptOpen

	while true; do
		if ! read -r -p "$1" setCheck <&"$_promptFd"; then
			promptClose
			colEcho $redB "ERROR: Failed to read input (EOF). Exiting..." >&2
			exit 1
		fi

		# Strip CR (Windows / paste) and surrounding whitespace.
		setCheck="${setCheck//$'\r'/}"
		setCheck="${setCheck#"${setCheck%%[![:space:]]*}"}"
		setCheck="${setCheck%"${setCheck##*[![:space:]]}"}"

		case "$setCheck" in
			[Yy]|[Yy][Ee][Ss])
				promptClose
				return 0
				;;
			[Nn]|[Nn][Oo])
				promptClose
				return 1
				;;
			*)
				colEcho $redB "Invalid input. Please enter 'Y' or 'N'." >&2
				;;
		esac
	done
}

# Warn if running as root/sudo; allow continue with confirmation.
# As root, clear $sudo so package/privilege calls do not require the sudo binary.
function CheckNotElevated {
    if (( "$EUID" == "0" )); then
        colEcho $redB "WARNING: Running as root (or via sudo)."
        colEcho $yellowB "This script is meant to run as a normal user and elevate only when needed."
        colEcho $yellowB "Running fully as root can break package installs, file ownership, or Ventoy paths."
        if ! YesNo "Continue anyway? Things may break. (Y/N) "; then
            colEcho $cyanB "Exiting. Re-run without sudo/root if possible."
            exit 1
        fi
        sudo=""
        colEcho $yellowB "Continuing as root (elevation prefix disabled).\n"
    fi
}

# Function to handle dependecies list
function dependenciesHandler() {
	$sudo $pkgmgr $update_arg
	local toInstall=()
	local command pkg
	local failedPkgs=()

	for command in "${!depCommands[@]}"; do
		if ! command -v "$command" >/dev/null 2>&1; then
			declare -n ref="${depCommands[$command]}"
			local pkgList
			if [ -z "${ref[$os]}" ]; then
				pkgList="${ref['default']}"
			else
				pkgList="${ref[$os]}"
			fi
			# Expand multi-package entries (e.g. "p7zip p7zip-plugins") into one-by-one installs
			# shellcheck disable=SC2206
			local pkgs=( $pkgList )
			for pkg in "${pkgs[@]}"; do
				toInstall+=("$pkg")
			done
		fi
	done

	if (( ${#toInstall[@]} > 0 )); then
		if [ "$os" == "unknown" ]; then
			colEcho $redB "ERROR: Distro is unknown and some dependencies were not found."
			colEcho $redB "Please install the following packages manually:$whiteB ${toInstall[*]}"
			exit 1
		fi
		colEcho $cyanB "The following dependencies will be installed (one at a time):$whiteB ${toInstall[*]}"
		UserWait
		for pkg in "${toInstall[@]}"; do
			colEcho $cyanB "Installing$whiteB $pkg$cyanB..."
			# shellcheck disable=SC2086
			if ! $sudo $pkgmgr $install_arg $pkg; then
				colEcho $yellowB "WARNING: Failed to install $pkg — continuing with remaining packages."
				failedPkgs+=("$pkg")
			fi
		done
		if (( ${#failedPkgs[@]} > 0 )); then
			colEcho $yellowB "Some packages failed:$whiteB ${failedPkgs[*]}"
		fi
	else
		colEcho $cyanB "All dependencies are already installed.\n"
	fi

	# Only hard-fail for commands that are still missing after best-effort install.
	local stillMissing=""
	for command in "${!depCommands[@]}"; do
		if ! command -v "$command" >/dev/null 2>&1; then
			stillMissing+=" $command"
		fi
	done
	if [ -n "$stillMissing" ]; then
		colEcho $redB "ERROR: Required commands still missing after install:$whiteB$stillMissing"
		colEcho $redB "Install them manually (or install an equivalent package), then re-run this script."
		exit 1
	fi
}

# Function to download ventoy
function downloadVentoy() {
	local os="$1"
	local ventoyPackage=$2
	local apiJson=""
	local tarUrl=""
	local extractedDir=""
	local wgetArgs=()

	colEcho $cyanB "\nLooking up latest Ventoy release..."
	apiJson=$(wget -q -O - https://api.github.com/repos/ventoy/Ventoy/releases/latest 2>/dev/null) || apiJson=""
	venver=$(printf '%s' "$apiJson" | grep -oE '"tag_name"[[:space:]]*:[[:space:]]*"[^"]+"' | head -n1 | cut -d'"' -f4)
	# Keep digits and dots only (strip leading v / junk). Avoid brittle ${venver: -6}.
	venver="${venver//[^0-9.]/}"

	if [[ -z "$venver" ]]; then
		colEcho $redB "ERROR: Could not determine the latest Ventoy version from GitHub."
		colEcho $yellowB "Check network access to api.github.com and try again."
		exit 1
	fi

	tarUrl="https://github.com/ventoy/Ventoy/releases/download/v${venver}/ventoy-${venver}-linux.tar.gz"
	colEcho $cyanB "\nDownloading Ventoy Version:$whiteB $venver"
	colEcho $cyanB "URL:$whiteB $tarUrl"

	wgetArgs=(-q -O ventoy.tar.gz)
	# --show-progress is missing on some wget builds (BusyBox / older distros).
	if wget --help 2>&1 | grep -q -- '--show-progress'; then
		wgetArgs+=(--show-progress)
	fi
	if ! wget "${wgetArgs[@]}" -- "$tarUrl"; then
		colEcho $redB "ERROR: Failed to download Ventoy."
		exit 1
	fi
	if [[ ! -s ventoy.tar.gz ]]; then
		colEcho $redB "ERROR: ventoy.tar.gz is missing or empty after download."
		exit 1
	fi

	colEcho $cyanB "\nExtracting Ventoy..."
	if ! tar -xf ventoy.tar.gz; then
		colEcho $redB "ERROR: Failed to extract ventoy.tar.gz"
		rm -f ventoy.tar.gz
		exit 1
	fi

	colEcho $cyanB "Removing the extracted Ventoy tar.gz file..."
	rm -f ventoy.tar.gz

	# Remove the ./ventoy folder if it exists before renaming ventoy folder.
	if [ -d ./ventoy ]; then
		colEcho $cyanB "Removing the previous ./ventoy folder..."
		rm -rf ./ventoy/
	fi

	extractedDir="ventoy-${venver}"
	if [[ ! -d "$extractedDir" ]]; then
		extractedDir=$(find . -maxdepth 1 -type d -name 'ventoy-*' ! -name 'ventoy-*.tar.gz' 2>/dev/null | head -n1)
		extractedDir="${extractedDir#./}"
	fi
	if [[ -z "$extractedDir" ]] || [[ ! -d "$extractedDir" ]]; then
		colEcho $redB "ERROR: Extracted Ventoy folder not found (expected ventoy-${venver})."
		exit 1
	fi
	if [[ ! -f "$extractedDir/Ventoy2Disk.sh" ]]; then
		colEcho $redB "ERROR: Ventoy2Disk.sh is missing inside $extractedDir"
		exit 1
	fi

	colEcho $cyanB "Renaming ventoy folder to remove the version number..."
	mv "$extractedDir" ventoy
}

# Read a line from the real terminal (prefer /dev/tty; never skip on empty stdin).
function ReadPrompt() {
	local __prompt="$1"
	local __resultVar="$2"
	local __line=""

	promptOpen
	if ! read -r -p "$__prompt" __line <&"$_promptFd"; then
		promptClose
		colEcho $redB "ERROR: Failed to read input (EOF). Exiting..." >&2
		exit 1
	fi
	promptClose

	__line="${__line//$'\r'/}"
	printf -v "$__resultVar" '%s' "$__line"
}

# Normalize a path that may be any of .zip.001-.006 (or a directory containing them) to the .001 volume.
# Prints the .001 path on success; returns 1 if not a usable split set.
function resolveSplitFirstVolume() {
	local input="$1"
	local dir=""
	local first=""
	local i part

	if [[ -d "$input" ]]; then
		dir="${input%/}"
	elif [[ -f "$input" ]]; then
		dir=$(dirname -- "$input")
	else
		return 1
	fi

	first="${dir}/${MedicatSplitFirst}"
	if [[ ! -f "$first" ]]; then
		return 1
	fi

	for i in 1 2 3 4 5 6; do
		part=$(printf '%s/%s.%03d' "$dir" "$MedicatSplitBase" "$i")
		if [[ ! -f "$part" ]]; then
			return 1
		fi
	done
	printf '%s\n' "$first"
	return 0
}

# Verify size + MD5 of all six Drive/Mega volumes. Expects location = ...zip.001
function verifySplitArchive() {
	local dir
	local i part expectMd5 expectMin gotSize gotMd5
	dir=$(dirname -- "$location")

	colEcho $cyanB "Checking Google Drive / Mega split volumes (MD5)..."
	for i in 0 1 2 3 4 5; do
		part=$(printf '%s/%s.%03d' "$dir" "$MedicatSplitBase" "$((i + 1))")
		expectMd5="${MedicatSplitMd5s[$i]}"
		# Allow 5% below expected size (same idea as the C++ installer).
		expectMin=$(( MedicatSplitSizes[i] * 95 / 100 ))
		if [[ ! -f "$part" ]]; then
			colEcho $redB "ERROR: Missing split volume:$whiteB $part"
			exit 1
		fi
		gotSize=$(stat -c%s "$part" 2>/dev/null || stat -f%z "$part" 2>/dev/null || echo 0)
		if [[ "$gotSize" -lt "$expectMin" ]]; then
			colEcho $redB "ERROR: $part looks incomplete or truncated."
			colEcho $cyanB "Size is$whiteB $gotSize$cyanB bytes; expected about$whiteB ${MedicatSplitSizes[$i]}$cyanB."
			exit 1
		fi
		colEcho $cyanB "Hashing$whiteB $(basename -- "$part")$cyanB..."
		gotMd5=$(md5sum "$part" | awk '{print tolower($1)}')
		if [[ "$gotMd5" != "$expectMd5" ]]; then
			colEcho $redB "ERROR: MD5 mismatch for$whiteB $part"
			colEcho $cyanB "Got:$whiteB      $gotMd5"
			colEcho $cyanB "Expected:$whiteB $expectMd5"
			exit 1
		fi
		colEcho $greenB "OK:$whiteB $(basename -- "$part")"
	done
	colEcho $greenB "All six split volumes match. Safe to proceed..."
	colEcho $cyanB "7z will extract from$whiteB $location$cyanB (follows .002-.006 automatically)."
}

# Mount the Medicat NTFS partition. Try ntfs3 (in-kernel), then ntfs (often ntfs-3g),
# then ntfs-3g directly. Fail hard if the mountpoint is not a real mount of $device
# so 7z never extracts into a plain local ./MedicatUSB directory (#81).
function mountMedicatNtfs() {
	local device="$1"
	local mnt="$2"
	local opts="rw,uid=$(id -u),gid=$(id -g),fmask=0111,dmask=0000"
	local attempt=""
	local mountedSource=""

	if [[ ! -b "$device" ]]; then
		colEcho $redB "ERROR: Not a block device:$whiteB $device"
		exit 1
	fi
	mkdir -p "$mnt"

	# Clear a stale mount from a previous run.
	if mountpoint -q "$mnt" 2>/dev/null; then
		colEcho $yellowB "Unmounting existing mount at$whiteB $mnt$yellowB..."
		$sudo umount "$mnt" 2>/dev/null || true
	fi

	for attempt in "ntfs3" "ntfs" "ntfs-3g" "auto"; do
		colEcho $cyanB "Trying mount type:$whiteB $attempt"
		case "$attempt" in
			ntfs3)
				$sudo mount -t ntfs3 -o "$opts" "$device" "$mnt" 2>/dev/null || continue
				;;
			ntfs)
				$sudo mount -t ntfs -o "$opts" "$device" "$mnt" 2>/dev/null || continue
				;;
			ntfs-3g)
				if ! command -v ntfs-3g >/dev/null 2>&1; then
					continue
				fi
				$sudo ntfs-3g -o "$opts" "$device" "$mnt" 2>/dev/null || continue
				;;
			auto)
				$sudo mount -o "$opts" "$device" "$mnt" 2>/dev/null || continue
				;;
		esac

		if ! mountpoint -q "$mnt" 2>/dev/null; then
			# mount may have "succeeded" without binding; try next type
			$sudo umount "$mnt" 2>/dev/null || true
			continue
		fi

		mountedSource=$(findmnt -n -o SOURCE --target "$mnt" 2>/dev/null || true)
		# Accept exact device or a resolved path (e.g. /dev/sdb1).
		if [[ "$mountedSource" != "$device" ]] && [[ "$mountedSource" != "$(readlink -f "$device" 2>/dev/null || true)" ]]; then
			colEcho $yellowB "Mount at$whiteB $mnt$yellowB is$whiteB $mountedSource$yellowB, expected$whiteB $device$yellowB; trying next type..."
			$sudo umount "$mnt" 2>/dev/null || true
			continue
		fi

		colEcho $greenB "Mounted$whiteB $device$greenB at$whiteB $mnt$greenB (type$whiteB $attempt$greenB)."
		return 0
	done

	colEcho $redB "ERROR: Failed to mount$whiteB $device$redB at$whiteB $mnt"
	colEcho $yellowB "Tried ntfs3, ntfs, ntfs-3g, and auto. Install ntfs-3g if needed, then re-run."
	colEcho $redB "Refusing to extract into an unmounted local folder (that would fill your system disk)."
	exit 1
}

# Download the MediCat archive over HTTP (aria2c multi-connection preferred; wget fallback).
# $1=url  $2=mirror name  $3=insecure (true|false) - skip TLS verify when true
function downloadMedicatHttp() {
	local url="$1"
	local mirrorName="$2"
	local insecure="$3"
	local ok=1

	colEcho $cyanB "\nDirect download from$whiteB $mirrorName"
	colEcho $cyanB "URL:$whiteB $url"

	if command -v aria2c >/dev/null 2>&1; then
		# aria2 caps --max-connection-per-server (-x) at 16
		local ariaArgs=(-x16 -s16 -k1M -c --file-allocation=none --summary-interval=5 -o "$Medicat7zFile")
		if [[ "$insecure" == "true" ]]; then
			colEcho $yellowB "Note: this mirror has known TLS certificate issues; certificate checking is disabled for this download only."
			ariaArgs+=(--check-certificate=false)
		fi
		if aria2c "${ariaArgs[@]}" -- "$url"; then
			ok=0
		fi
	elif command -v wget >/dev/null 2>&1; then
		colEcho $yellowB "aria2c not found; falling back to single-stream wget."
		local wgetArgs=(-c --show-progress -O "$Medicat7zFile")
		if [[ "$insecure" == "true" ]]; then
			colEcho $yellowB "Note: this mirror has known TLS certificate issues; certificate checking is disabled for this download only."
			wgetArgs+=(--no-check-certificate)
		fi
		if wget "${wgetArgs[@]}" -- "$url"; then
			ok=0
		fi
	else
		colEcho $redB "ERROR: Neither aria2c nor wget is available for HTTP download."
		return 1
	fi

	if [[ "$ok" -eq 0 ]] && [[ -f "$Medicat7zFile" ]]; then
		location="$Medicat7zFile"
		archiveKind="solid7z"
		colEcho $greenB "Download finished:$whiteB $location"
		return 0
	fi

	colEcho $redB "Download from $mirrorName failed."
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
		archiveKind="solid7z"
	elif [[ -f "$Medicat7zFile" ]]; then
		location="$Medicat7zFile"
		archiveKind="solid7z"
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

# Available bytes on the filesystem that holds $1 (GNU df). Prints nothing / fails on error.
function getAvailBytes() {
	local path="$1"
	df -B1 --output=avail "$path" 2>/dev/null | awk 'NR==2 {print $1; exit}'
}

# Human-readable size from byte count (approx GiB/MiB).
function humanBytes() {
	local bytes="${1:-0}"
	if (( bytes >= 1024 * 1024 * 1024 )); then
		awk -v b="$bytes" 'BEGIN { printf "%.1f GiB", b / (1024*1024*1024) }'
	elif (( bytes >= 1024 * 1024 )); then
		awk -v b="$bytes" 'BEGIN { printf "%.1f MiB", b / (1024*1024) }'
	else
		printf "%s B" "$bytes"
	fi
}

# Warn (or optionally abort) when path has less than minBytes free.
# $1=path  $2=minBytes  $3=purpose label  $4=fatal|warn
# fatal: exit 1 unless user continues after extra confirm (still discouraged)
# warn:  YesNo to continue; decline exits 0
function requireFreeSpace() {
	local path="$1"
	local minBytes="$2"
	local purpose="$3"
	local mode="${4:-warn}"
	local avail

	if [[ ! -e "$path" ]]; then
		colEcho $yellowB "WARNING: Cannot check free space; path does not exist yet:$whiteB $path"
		return 0
	fi

	avail=$(getAvailBytes "$path")
	if [[ -z "$avail" || ! "$avail" =~ ^[0-9]+$ ]]; then
		colEcho $yellowB "WARNING: Unable to read free space for$whiteB $path$yellowB (df failed). Continuing..."
		return 0
	fi

	colEcho $cyanB "Free space on$whiteB $path$cyanB:$whiteB $(humanBytes "$avail")$cyanB (need ~$whiteB$(humanBytes "$minBytes")$cyanB for $purpose)"

	if (( avail >= minBytes )); then
		return 0
	fi

	colEcho $redB "\nWARNING: Not enough free space for $purpose."
	colEcho $redB "  Path:$whiteB $path"
	colEcho $redB "  Available:$whiteB $(humanBytes "$avail")"
	colEcho $redB "  Recommended:$whiteB $(humanBytes "$minBytes")"
	colEcho $yellowB "Low space often causes truncated MediCat downloads, apt failures, or extract errors."
	colEcho $yellowB "Free space, move the archive to a larger disk, or download to another path.\n"
	df -h "$path" 2>/dev/null || true

	if [[ "$mode" == "fatal" ]]; then
		if ! YesNo "Continue anyway? A failed/truncated download is likely. (Y/N) "; then
			colEcho $cyanB "Exiting so you can free space first."
			exit 1
		fi
		colEcho $yellowB "Continuing despite low free space...\n"
		return 0
	fi

	if ! YesNo "Continue anyway? (Y/N) "; then
		colEcho $cyanB "Exiting so you can free space first."
		exit 0
	fi
	colEcho $yellowB "Continuing despite low free space...\n"
	return 0
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
				ReadPrompt "Path to $Medicat7zFile or ${MedicatSplitFirst}: " pathInput
				pathInput="${pathInput//$'\r'/}"
				pathInput="${pathInput#"${pathInput%%[![:space:]]*}"}"
				pathInput="${pathInput%"${pathInput##*[![:space:]]}"}"
				# Expand ~ if present
				pathInput="${pathInput/#\~/$HOME}"
				if [[ -z "$pathInput" ]]; then
					colEcho $redB "No path entered."
					continue
				fi
				if splitFirst=$(resolveSplitFirstVolume "$pathInput"); then
					location="$splitFirst"
					archiveKind="splitzip"
					DownloadMethod="local"
					needMedicatDownload=false
					colEcho $cyanB "Using local split archive:$whiteB $location"
					return 0
				fi
				if [[ ! -f "$pathInput" ]]; then
					colEcho $redB "File not found:$whiteB $pathInput"
					continue
				fi
				location="$pathInput"
				archiveKind="solid7z"
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

command clear 2>/dev/null || true
colEcho "$yellowB" "WELCOME TO THE MEDICAT INSTALLER.\n"
colEcho "$cyanB" "Script version:$whiteB $ScriptVersion"

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
	install_arg="install -y"
	update_arg="update"
elif grep -qs "freebsd" /etc/os-release; then
	os="freebsd"
	pkgmgr="pkg"
	install_arg="install -y"
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
	install_arg="add --no-interactive"
	update_arg="update"
elif [[ -e /etc/debian_version ]]; then
	os="debian"
	pkgmgr="apt"
	install_arg="install -y"
	update_arg="update"
elif [[ -e /etc/almalinux-release || -e /etc/rocky-release || -e /etc/centos-release ]]; then
	colEcho $redB "Fuck Red-Hat for putting source code behind paywalls."
	os="centos"
	pkgmgr="yum"
	install_arg="install -y"
	update_arg="update"
elif [[ -e /etc/fedora-release ]]; then
	os="fedora"
	pkgmgr="dnf"
	install_arg="install -y"
	update_arg="upgrade"
	alias mkexfatfs=mkfs.exfat # Wtf Ventoy?
elif [[ -e /etc/nobara ]]; then
	colEcho $redB "gaming moment"
	os="fedora"
	pkgmgr="yum"
	install_arg="install -y"
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
	install_arg="-Sy" # -y = assume yes
	update_arg="-S"
else
	os="unknown"
	colEcho "WARNING: Distro not recognised - trying to continue...\n"
fi

colEcho $cyanB "Operating System Identified as:$whiteB $os"

# Ensure dependencies are installed: wget, 7z, mkntfs, and aria2c if MediCat will be downloaded
colEcho $cyanB "\nLocating the Medicat archive..."

if [[ -f "$Medicat7zFile" ]]; then
	location="$Medicat7zFile"
	archiveKind="solid7z"
	colEcho $cyanB "Medicat file found:$whiteB $Medicat7zFile\n"
elif [[ -f "$Medicat7zFull" ]]; then
	location="$Medicat7zFull"
	archiveKind="solid7z"
	colEcho $cyanB "Medicat file found:$whiteB $Medicat7zFull\n"
elif splitFirst=$(resolveSplitFirstVolume "."); then
	location="$splitFirst"
	archiveKind="splitzip"
	colEcho $cyanB "Medicat split volumes found:$whiteB $MedicatSplitFirst .. .006\n"
else
	colEcho $yellowB "Medicat archive not found in the current directory (.7z or .zip.001-.006)."
	chooseMedicatSource
fi

colEcho $cyanB "Acquiring any dependencies..."

# Working directory needs headroom for packages / Ventoy tarball / mountpoint.
requireFreeSpace "." "$MedicatWorkMinFreeBytes" "installer working files" "warn"

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
	# Archive lands in the current directory — low space here truncates the .7z.
	requireFreeSpace "." "$MedicatDownloadMinFreeBytes" "MediCat archive download" "fatal"
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

# Infer kind if local path was set without going through locate.
if [[ -z "$archiveKind" ]]; then
	if [[ "$location" == *.zip.001 ]] || basename -- "$location" | grep -qE '\.zip\.001$'; then
		archiveKind="splitzip"
	else
		archiveKind="solid7z"
	fi
fi

if [[ "$archiveKind" == "splitzip" ]]; then
	if ! splitFirst=$(resolveSplitFirstVolume "$location"); then
		colEcho $redB "ERROR: Incomplete Google Drive / Mega split set (need all of ${MedicatSplitBase}.001-.006)."
		exit 1
	fi
	location="$splitFirst"
	verifySplitArchive
else
	# Check size then SHA256 of the solid Medicat .7z
	colEcho $cyanB "Checking size and SHA256 hash of$whiteB $location$cyanB..."

	fileSize=$(stat -c%s "$location" 2>/dev/null || stat -f%z "$location" 2>/dev/null || echo 0)
	if [[ "$fileSize" -lt "$Medicat7zMinBytes" ]]; then
		colEcho $redB "ERROR: $location looks incomplete or truncated."
		colEcho $cyanB "Size is$whiteB $fileSize$cyanB bytes; expected at least$whiteB $Medicat7zMinBytes$cyanB (~20 GiB)."
		colEcho $yellowB "Delete the partial file and download again (prefer aria2c / a stable mirror)."
		exit 1
	fi

	checksha256=$(sha256sum "$location" | awk '{print $1}')

	if [[ "$checksha256" != "$Medicat256Hash" ]]; then
		colEcho $redB "$location SHA256 hash does not match."
		colEcho $redB "File may be corrupted, incomplete, or the wrong format (need the .7z, or all six .zip.001-.006 parts)."
		colEcho $cyanB "Got:$whiteB      $checksha256"
		colEcho $cyanB "Expected:$whiteB $Medicat256Hash"
		colEcho $cyanB "Exiting..."
		exit 1
	else
		colEcho $greenB "$location SHA256 hash matches."
		colEcho $cyanB "Hash is$whiteB $checksha256"
		colEcho $cyanB "Safe to proceed..."
	fi
fi

# Advise user to connect and select the required USB device.
colEcho $yellowB "\nPlease plug your USB in now if it is not already connected..."
colEcho $yellowB "NOTE: A 32GB stick works, but only just barely — prefer 64GB+ if you have one."
colEcho $yellowB "Press any key once it has been detected by your system..."
UserWait "Press any key when ready..."

letter=""
while true; do
	colEcho $yellowB "\nPlease find the ID of your USB drive below (look for TRAN=usb):"
	lsblk --nodeps --output "NAME,SIZE,TRAN,VENDOR,MODEL,SERIAL" | grep -v loop || true

	colEcho $yellowB "Enter the device name NOT including /dev/ or the partition number."
	colEcho $yellowB "For example: sda  or  sdb  or  nvme0n1"
	ReadPrompt "Device: " letter
	letter="${letter//$'\r'/}"
	letter="${letter#/dev/}"
	letter="${letter#"${letter%%[![:space:]]*}"}"
	letter="${letter%"${letter##*[![:space:]]}"}"
	# Strip trailing partition numbers for nvme (nvme0n1p1 -> nvme0n1) is user error; only strip bare trailing digit patterns for sdxN
	if [[ -z "$letter" ]]; then
		colEcho $redB "No device entered. Try again."
		continue
	fi
	if [[ ! -b "/dev/$letter" ]]; then
		colEcho $redB "Block device not found:$whiteB /dev/$letter"
		colEcho $yellowB "Check the name from the list and try again."
		continue
	fi
	break
done

drive=/dev/$letter
# First partition: nvme0n1 -> nvme0n1p1, sda -> sda1
if [[ "$letter" =~ [0-9]$ ]]; then
	drive2="${drive}p1"
else
	drive2="${drive}1"
fi

# Warn when the chosen disk is not USB (internal HDD/SSD/NVMe wipe risk).
driveTran=$(lsblk -ndo TRAN "$drive" 2>/dev/null | head -n1 | tr '[:upper:]' '[:lower:]')
driveSize=$(lsblk -ndo SIZE "$drive" 2>/dev/null | head -n1)
driveModel=$(lsblk -ndo MODEL "$drive" 2>/dev/null | head -n1)
driveModel="${driveModel#"${driveModel%%[![:space:]]*}"}"
driveModel="${driveModel%"${driveModel##*[![:space:]]}"}"
if [[ "$driveTran" != "usb" ]]; then
	colEcho $redB "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	colEcho $redB " WARNING: $drive does not look like a USB drive."
	colEcho $redB " Transport:$whiteB ${driveTran:-unknown}$redB  Size:$whiteB ${driveSize:-unknown}$redB  Model:$whiteB ${driveModel:-unknown}"
	colEcho $redB " Installing here will ERASE this disk (internal HDD/SSD/NVMe)."
	colEcho $redB " MediCat is meant for a removable USB stick — double-check the device name."
	colEcho $redB "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	colEcho $redB "To continue, type CONFIRM in all caps (anything else cancels).\n"
	confirmWipe=""
	ReadPrompt "Type CONFIRM to wipe $drive: " confirmWipe
	if [[ "$confirmWipe" != "CONFIRM" ]]; then
		colEcho $yellowB "Confirmation text did not match. Installation Cancelled."
		exit 0
	fi
fi

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
	$sudo $ventoyLauncher -I -g $drive
	if [ "$?" != "0" ]; then
		colEcho $redB "ERROR: Unable to install Ventoy. Exiting..."
		exit 1
	fi
else
	colEcho $yellowB "Using MBR"
	$sudo $ventoyLauncher -I $drive
	if [ "$?" != "0" ]; then
		colEcho $redB "ERROR: Unable to install Ventoy. Exiting..."
		exit 1
	fi
fi

cd ..

colEcho $cyanB "Unmounting drive$whiteB $drive"
$sudo umount $drive

colEcho $cyanB "Creating Medicat NTFS file system on drive$whiteB $drive2"
if ! $sudo mkntfs --fast --label Medicat $drive2; then
	colEcho $redB "ERROR: mkntfs failed on$whiteB $drive2"
	exit 1
fi

# Create a mountpoint folder for the Medicat NTFS volume
requireFreeSpace "." "$MedicatWorkMinFreeBytes" "MedicatUSB mountpoint" "fatal"
if ! [[ -d MedicatUSB/ ]] ; then
	colEcho $cyanB "Creating a mountpoint for the Medicat NTFS volume..."
	mkdir MedicatUSB
fi

colEcho $cyanB "Mounting Medicat NTFS volume..."
mountMedicatNtfs "$drive2" "./MedicatUSB"

# Target USB must have room for the extracted MediCat tree.
colEcho $yellowB "NOTE: 32GB USBs are fine but JUST BARELY - little room left after extract."
requireFreeSpace "./MedicatUSB" "$MedicatUsbMinFreeBytes" "MediCat extract onto USB" "fatal"

# Belt-and-suspenders: never extract unless ./MedicatUSB is still a mount of $drive2.
if ! mountpoint -q ./MedicatUSB 2>/dev/null; then
	colEcho $redB "ERROR: ./MedicatUSB is not a mounted filesystem. Aborting extract."
	exit 1
fi

colEcho $cyanB "Extracting Medicat to NTFS volume..."
# For split Drive/Mega volumes, $location is the .001 file; 7z opens the rest automatically.
7z x -o./MedicatUSB "$location"
if [ "$?" != "0" ]; then
	colEcho $redB "ERROR: 7z extract failed for$whiteB $location"
	exit 1
fi

colEcho $cyanB "MedicatUSB has been created."

if YesNo "Would you like to unmount ./MedicatUSB? (Y/N) "; then
	colEcho $cyanB "Unmounting MedicatUSB..."
	$sudo umount ./MedicatUSB
	colEcho $cyanB "Unmounted."
else
	colEcho $cyanB "MedicatUSB will not be unmounted."
fi
