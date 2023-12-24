#!/bin/bash

if [[ "$(which docker)" == "" ]]; then
    echo "docker not found, please install docker"
    exit 1
else
    echo "docker found"
fi

if [[ "$(which docker-compose)" == "" ]]; then
    echo "docker-compose not found, please install docker-compose"
    exit 1
else
    echo "docker-compose found"
fi

#docker build . -t chiaki 
#container=$(docker create chiaki)
#mkdir -p out
#docker cp $container:/app/chiaki-rpi ./out/
#docker rm $container

docker-compose run build-rk3566
