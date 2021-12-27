#!/bin/sh
# Toomey Jul 2021

# Go set your settings here
. ./config.sh

# Wrapper to the stock folders, podman/docker binary, etc. Edit this in config.sh
dockerrun "rapio:f35 /bin/bash"
