#!/bin/bash
# Toomey July 2021
# Run DOXYGEN in your MYALGS folder
# on the container to generate documentation

if [ ! -d "/BUILD/RAPIO/" ]; then
  echo "Run this logged into the container (I'm looking for /BUILD/RAPIO on container).";
  exit
fi

# Remove any old doxygen from this folder
rm -rf html
rm -rf latex

cd /BUILD/RAPIO
doxygen rapio.dox
cd -
cp -r /BUILD/RAPIO/html .
cp -r /BUILD/RAPIO/latex .



