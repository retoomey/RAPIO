#!/bin/bash
# Toomey Jul 2021
# Simple developer environment for creating new algorithm 
# We want to run stuff as the building user to write to host folders
# using same user name. I'm not sure how to do this for windows native yet, you need the 'id' command.

# First stage, build the base container for RAPIO and/or MRMS building 
docker build --build-arg RUNID=`id -u` --build-arg RUNGROUP=`id -g` --tag mrmsbase:f34 -f PACKAGES/MRMSBASE/mrmsbase_f34.dock .

# Second stage, build RAPIO on base 
docker build --no-cache --tag rapio:f34 -f PACKAGES/RAPIO/rapio_f34.dock .
