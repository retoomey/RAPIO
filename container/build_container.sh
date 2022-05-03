#!/bin/sh
# Toomey Jul 2021

# Common functions and settings/paths
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
. $SCRIPT_DIR/config.sh

# First stage, build the base container for RAPIO and/or MRMS building 
$DOCKERBIN build --build-arg RUNID=`id -u` --build-arg RUNGROUP=`id -g` --tag mrmsbase:f35 -f PACKAGES/MRMSBASE/mrmsbase_f35.dock .

# Second stage, build RAPIO on base 
$DOCKERBIN build --no-cache --tag rapio:f35 -f PACKAGES/RAPIO/rapio_f35.dock .
