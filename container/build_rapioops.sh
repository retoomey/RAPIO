#!/bin/sh
# Toomey Jul 2022

# Common functions and settings/paths
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
. $SCRIPT_DIR/config.sh

# Create an operations image
$DOCKERBIN build --squash --build-arg RUNID=`id -u` --build-arg RUNGROUP=`id -g` --tag rapioops:f36 -f PACKAGES/RAPIO/rapioops_f36.dock .
