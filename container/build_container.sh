#!/bin/sh
# Toomey Jul 2021

# Common functions and settings/paths
. ./config.sh

# First stage, build the base container for RAPIO and/or MRMS building 
$DOCKERBIN build --build-arg RUNID=`id -u` --build-arg RUNGROUP=`id -g` --tag mrmsbase:f34 -f PACKAGES/MRMSBASE/mrmsbase_f34.dock .

# Second stage, build RAPIO on base 
$DOCKERBIN build --no-cache --tag rapio:f34 -f PACKAGES/RAPIO/rapio_f34.dock .
