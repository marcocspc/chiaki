#!/bin/sh

echo ""
echo "This script will install SDL 2.0.20 from the Official github release onto your system."
echo "Read this script for more details."
echo ""
read -r -p "Go ahead? [Y/n] " input

case $input in
      [yY][eE][sS]|[yY])
            echo "Installing from https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.0.20.zip"
            echo ""
            cd third-party/
            wget https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.0.20.zip
            unzip release-2.0.20.zip
	    cd SDL-release-2.0.20
	    sudo apt install libgbm-dev
            ./configure --enable-video-kmsdrm --enable-video-opengles
	    make -j3
	    sudo make install
	    echo ""
	    echo "SDL 2.0.20 install finished"
	    echo ""
            ;;
      [nN][oO]|[nN])
            echo "Exiting script"
            exit 1
            ;;
      *)
            echo "Invalid input"
            exit 1
            ;;
esac
