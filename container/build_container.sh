#!/bin/sh
# Toomey Jul 2022

# First stage, build the operations container for RAPIO and/or MRMS
# I try to make this smaller, but you can't 'build' on it
./build_mrmsbaseops.sh

# Second stage, build the container for RAPIO and/or MRMS building, this
# adds development rpms, etc. to the operations container
./build_mrmsbasedev.sh

# Third stage, build the rapio dev container 
./build_rapiodev.sh
