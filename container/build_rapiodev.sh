#!/bin/sh
# Toomey Jul 2022

# Common functions and settings/paths
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
. $SCRIPT_DIR/config.sh

# Build RAPIO development container
# We'll create the userid on it based on this user, unless we're running as root
$DOCKERBIN build --build-arg RUNID=`id -u` --build-arg RUNGROUP=`id -g` --tag rapiodev:f36 -f PACKAGES/RAPIO/rapiodev_f36.dock .
