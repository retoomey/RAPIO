#!/bin/sh
# Toomey May 2022
# Build a uncrustify docker for pretty formatting code

# Can also just build it, it's a pretty simple docker
#docker build --tag uncrustify -f PACKAGES/uncrustify.dock .

# Common functions and settings/paths
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
. $SCRIPT_DIR/config.sh

$DOCKERBIN build --tag uncrustify -f PACKAGES/UNCRUSTIFY/uncrustify.dock .
