#!/bin/bash

# dependencies
# curl jq wget which p7zip-full p7zip mkntfs aria2c lolcat base64


if [[ "$EUID" -eq 0 ]]
  then echo " PLEASE DO NOT RUN THIS AS ROOT !!"
  exit 1
fi

resize -s 30 100 > /dev/null

function Welcome_message() {
  # to obtain title, convert strings to compressed base64
  # cat ./Medicat_title.txt | gzip | base64 -w0
  echo "H4sIAAAAAAAAA62TQRKDIAxF957iL1ibTs+TGS7yDy8kBGjFaRdkFL+JPvJBc94XB7EtWGEpqd9VkVp0pcBd4OshyzWY08oVLCes56ZqTbPpKdOVZq9lDlhMazBGn+BtFBS0YeWzNtm0Aw4bjS5gfSKPFQxaCxg2+RPmNpcwaqnKHzYZ27SYosPKSsiwOW8UIqfWf2SeYOq2XGh8gV0JmmIIaa9M4/t8Vdi+H4DHBcO8AzpPAwAA" | base64 -d | gunzip | lolcat

  printf "\n\tThis Installer will attempt to Install Ventoy and Medicat\n    THIS IS IN BETA. PLEASE CONTACT MATT IN THE DISCORD FOR ALL ISSUES\n\tUpdated for efficiency and cross-distro use by SkeletonMan\n\n\t\t\tWaiting for 10 seconds\n\n"
  sleep 10
}


function Determine_Distrib() {
  # From original script
  # egrep ^ID= /etc/os-release | awk -F= '{ print $2 ;}' instead of
  # if grep -qs "ubuntu" /etc/os-release; then

if [[ -z "$(which lsb_release)"  ]]; then
  if [[ -f "/etc/lsb-release" ]]; then
    DISTRIBTYPE="$(egrep ID= "/etc/lsb-release" | awk -F= '{ print $2 ;}')"
  else
    DISTRIBTYPE="$(egrep ^ID= "/etc/os-release" | awk -F= '{ print $2 ;}')"
  fi
else
  DISTRIBTYPE="$(lsb_release -i)"
fi

if [[ "$DISTRIBTYPE" =~ "Ubuntu" || "$DISTRIBTYPE" =~ "Debian" ]]; then
  #statements
  pkgmgr="apt"
  install_arg="install"
  update_arg="update"
elif [[ "$DISTRIBTYPE" =~ "centos" || "$DISTRIBTYPE" =~ "fedora" || "$DISTRIBTYPE" =~ "rhel" ]]; then
	pkgmgr="yum"
  install_arg="install"
  update_arg="update"
elif [[ "$DISTRIBTYPE" =~ "opensuse" ]]; then
  pkgmgr="zypper"
  install_arg="install"
  update_arg="update"
elif [[ "$DISTRIBTYPE" =~ "arch" ]]; then
  	pkgmgr="pacman"
  	install_arg="-S --needed --noconfirm"
  	update_arg="-Syy"
  elif [[ "$DISTRIBTYPE" =~ "freebsd" ]]; then
  	pkgmgr="pkg"
  	install_arg="install"
  	update_arg="update"
else
    echo "Unknown or unsupported Linux distribution for the moment, sorry the installer will stop."
    exit 2
fi
os="$DISTRIBTYPE"

  echo "OS detected : $os"



}

# from https://github.com/jsamr/bootiso
printUSBDevices() {
  typeset -a usbDevices
  typeset -a devices
  getDeviceType() {
    typeset deviceName=/sys/block/${1#/dev/}
    typeset deviceType=$(udevadm info --query=property --path="$deviceName" | grep -Po 'ID_BUS=\K\w+')
    echo "$deviceType"
  }
  mapfile -t devices < <(lsblk -o NAME,TYPE | grep --color=never -oP '^\K\w+(?=\s+disk$)')
  for device in "${devices[@]}" ; do
    if [ "$(getDeviceType "/dev/$device")" == "usb" ]; then
      usbDevices+=("/dev/$device")
    fi
  done
  echo "${usbDevices[@]}"
}


Welcome_message
Determine_Distrib
echo "USB Key detected at $(printUSBDevices)"
