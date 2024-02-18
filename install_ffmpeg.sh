#!/bin/sh

echo ""
echo "This script will install the custom rkmpp version of ffmpeg/avcodec onto your system."
echo "It will have a slightly different and possibly more limited feature set than standard Ubuntu ffmpeg."
echo "Read this script for more details."
echo ""
read -r -p "Go ahead? [Y/n] " input

case $input in
	[yY][eE][sS]|[yY])
	echo "Installing from https://github.com/faithanalog/FFmpeg-v4l2"
	echo ""
	cd third-party/
	sudo apt-get install -y build-essential \
                            meson libepoxy-dev libxcb-dri3-dev libxcb1-dev libx11-dev libx11-xcb-dev libdrm-dev \
                            libudev-dev \
                            libglfw3-dev libgles2-mesa-dev libepoxy-dev \
                            libglfw3-dev libgles2-mesa-dev libepoxy-dev \
                            libaom-dev \
                            libgnutls28-dev

    git clone --branch v4l2-request-hwaccel-4.4 https://github.com/faithanalog/FFmpeg-v4l2 FFmpeg
	cd FFmpeg
	export CPPFLAGS="-I/usr/include/libdrm"
    ./configure --enable-v4l2-request --enable-libudev --enable-libdrm --enable-gnutls 
    make -j $(nproc)
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
