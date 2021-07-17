#!/bin/bash
# Toomey Jul 2021

# Go set your settings here
source config.sh

# Docker run command
# Recommend not running as root
mkdir -p $MYALGPATH
docker run -it -h "rapio" \
    -v "$INPUT":/INPUT \
    -v "$OUTPUT":/OUTPUT \
    -v "$MYALGPATH":/BUILD/MYALGS \
    --user $(id -u):$(id -g) \
    rapio:f34 /bin/bash
