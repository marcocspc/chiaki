#!/bin/sh

echo ""
echo "This script will install the custom rkmpp version of ffmpeg/avcodec onto your system."
echo "It will have a slightly different and possibly more limited feature set than standard Ubuntu ffmpeg."
echo "Read this script for more details."
echo ""
read -r -p "Go ahead? [Y/n] " input

case $input in
	[yY][eE][sS]|[yY])
	#echo "Installing from https://github.com/jc-kynesim/rpi-ffmpeg.git"
	echo "Installing from https://github.com/hbiyik/FFmpeg"
	echo ""
	cd third-party/
	sudo apt install -y build-essential 
	sudo apt install -y meson libepoxy-dev libxcb-dri3-dev libxcb1-dev libx11-dev libx11-xcb-dev libdrm-dev
	sudo apt install -y libudev-dev
	sudo apt install -y libglfw3-dev libgles2-mesa-dev libepoxy-dev
	#git clone --branch  test/4.3.5/rpi_main https://github.com/jc-kynesim/rpi-ffmpeg.git
	git clone --branch  exp_refactor_all https://github.com/hbiyik/FFmpeg
	cd rpi-ffmpeg
	export CPPFLAGS="-I/usr/include/libdrm"
	#./configure --enable-shared --disable-static --enable-sand --enable-v4l2-request --enable-libdrm --enable-libudev --enable-opengl --enable-epoxy --enable-vout-egl  --enable-vout-drm
    ./configure --enable-rkmpp --enable-version3 --enable-lib
drm --enable-nonfree --enable-gpl --enable-version3 --enable-libx264 --enable-librtmp --enable-shared --enable-static --enable-libx265 --enable-libmp3lame --enable-libpulse --enable-openssl --enable-libopus --enable-libvorbis --enable-libaom --enable-libass --enable-libdav1d --enable-libx265 --enable-libvpx
	make -j3
	sudo make install
	echo ""
	echo "rkmpp-ffmpeg install finished"
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
