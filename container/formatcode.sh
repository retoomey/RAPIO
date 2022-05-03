#!/bin/sh
# Toomey May 2022
# Container version of checkformat.sh, this will use uncrustify on container
# to check pretty printing of this build.  You have to build the container
# first obviously

# FIXME: Guess I could auto build it if not found?

# Go set your settings here
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
. $SCRIPT_DIR/config.sh

RAPIOSOURCE="$SCRIPT_DIR/.."

$DOCKERBIN run --rm -it --workdir="/INPUT" \
	-v "$RAPIOSOURCE":/INPUT \
	uncrustify /INPUT/formatcode.sh
