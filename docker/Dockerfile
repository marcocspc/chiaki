FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
RUN  apt-get update
RUN  apt-get install -y cmake \
                        git \
                        python3-protobuf \
                        protobuf-compiler \
                        libopus-dev \
                        libssl-dev \
                        libudev-dev \
                        libevdev-dev \
                        libsdl2-ttf-dev \
                        libsdl2-image-dev \
                        libavutil-dev \
                        libavcodec-dev \
                        libavformat-dev \
                        libswscale-dev \
                        build-essential \
                        wget \
                        unzip \
                        sudo \
                        libdrm-dev \
                        libgbm-dev

RUN  cd / &&  git clone --recursive --single-branch --branch rpi02 https://github.com/marcocspc/chiaki.git chiaki-rpi
RUN  cd chiaki-rpi &&  echo y | ./install_sdl.sh 
RUN  cd chiaki-rpi &&  echo y | ./install_ffmpeg.sh

RUN rm -rf /chiaki-rpi
RUN  cd / &&  git clone --recursive --single-branch --branch rpi02 https://github.com/marcocspc/chiaki.git chiaki-rpi
RUN  mkdir /chiaki-rpi/build
RUN  cd /chiaki-rpi/build &&  cmake ..
RUN  cd /chiaki-rpi/build &&  make -j $(nproc)

RUN  mkdir /app
RUN  mv /chiaki-rpi/build/rpi/chiaki-rpi /app
