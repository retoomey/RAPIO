#!/bin/sh
# Toomey Jul 2022

# Common functions and settings/paths
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
. $SCRIPT_DIR/config.sh

# Second stage, build the container for RAPIO and/or MRMS building, this
# adds development rpms, etc. to the operations container
$DOCKERBIN build --tag mrmsbasedev:f36 -f PACKAGES/MRMSBASE/mrmsbasedev_f36.dock .
