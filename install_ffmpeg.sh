#!/bin/sh

echo ""
echo "This script will install the custom Raspberry version of ffmpeg/avcodec onto your system."
echo "It will have a slightly different and possibly more limited feature set than standard Ubuntu ffmpeg."
echo "Read this script for more details."
echo ""
read -r -p "Go ahead? [Y/n] " input

case $input in
	[yY][eE][sS]|[yY])
	echo "Installing from https://github.com/jc-kynesim/rpi-ffmpeg.git"
	echo ""
	cd third-party/
	sudo apt install build-essential
	sudo apt install meson libepoxy-dev libxcb-dri3-dev libxcb1-dev libx11-dev libx11-xcb-dev libdrm-dev
	sudo apt install libudev-dev
	sudo apt install libglfw3-dev libgles2-mesa-dev libepoxy-dev
	git clone --branch  test/4.3.5/rpi_main https://github.com/jc-kynesim/rpi-ffmpeg.git
	cd rpi-ffmpeg
	export CPPFLAGS="-I/usr/include/libdrm"
	./configure --enable-shared --disable-static --enable-sand --enable-v4l2-request --enable-libdrm --enable-libudev --enable-opengl --enable-epoxy --enable-vout-egl  --enable-vout-drm
	make -j3
	sudo make install
	echo ""
	echo "rpi-ffmpeg install finished"
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
