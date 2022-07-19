#!/bin/sh
# Toomey Jul 2022

# Common functions and settings/paths
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
. $SCRIPT_DIR/config.sh

# First stage, build the operations container for RAPIO and/or MRMS
# I try to make this smaller, but you can't 'build' on it
$DOCKERBIN build --tag mrmsbaseops:f35 -f PACKAGES/MRMSBASE/mrmsbaseops_f35.dock .

# Second stage, build the container for RAPIO and/or MRMS building, this
# adds development rpms, etc. to the operations container
#$DOCKERBIN build --tag mrmsbasedev:f35 -f PACKAGES/MRMSBASE/mrmsbase_f35.dock .
