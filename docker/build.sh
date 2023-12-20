#!/bin/bash

if [[ "$(which docker)" == "" ]]; then
    echo "docker not found, please install docker"
    exit 1
else
    echo "docker found"
fi

docker build . -t chiaki 
container=$(docker create chiaki)
mkdir -p out
docker cp $container:/app/chiaki-rpi ./out/
docker rm $container
