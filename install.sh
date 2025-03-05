#!/bin/bash

if [[ "$EUID" -ne 0 ]] && [[ "$OSTYPE" != "darwin"* ]]
  then echo "Please run as root! (sudo ./install.sh)"
  exit
fi

rm -rf build/
mkdir build/
make
cp build/seven-square /usr/bin
cp other/sevensquare.desktop /usr/share/applications
cp other/sevensquare_icon.png /usr/share/pixmaps
rm -rf build/

