#!/bin/sh
# Toomey Jul 2021

# Go set your settings here
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
. $SCRIPT_DIR/config.sh

# Wrapper to the stock folders, podman/docker binary, etc. Edit this in config.sh
dockerrun "rapiodev:f36 /bin/bash"
