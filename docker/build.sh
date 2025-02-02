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

docker-compose build build-rk3566 --progress=plain
mkdir -p out
container=$(docker-compose create build-rk3566)
docker cp $container:/app/chiaki-rpi ./out/
docker rm $container
