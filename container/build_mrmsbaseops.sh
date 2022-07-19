#!/bin/sh
# Toomey Jul 2022

# Common functions and settings/paths
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
. $SCRIPT_DIR/config.sh

# Build the operations container for RAPIO and/or MRMS
# This should be fairly small
$DOCKERBIN build --tag mrmsbaseops:f36 -f PACKAGES/MRMSBASE/mrmsbaseops_f36.dock .
