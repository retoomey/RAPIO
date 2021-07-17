#!/bin/bash
# Toomey Jul 2021

# The path to your host located algorithms folder. 
# You want to make your algorithms here since containers
# typically reset when you exit and you want to keep your code.
#
# This _MUST_ be an absolute path with docker.
# This gets mapped to /MYALGS on the container
#
MYALGPATH="$HOME/MYALGS"
mkdir -p $MYALGPATH

# The path to your data input folder, archives, etc.
# Algorithms would do -i="/INPUT/May3/code_index.xml", for example
# Note, I don't think FAM will work with container for realtime
# this will requite the ipoll option probably and/or other forms
# of notification.
INPUT="$HOME/DATAIN"
mkdir -p $INPUT

# The path to your output folder
OUTPUT="$HOME/DATAOUT"
mkdir -p $OUTPUT
