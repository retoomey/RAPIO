#!/bin/sh
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
INPUT="$HOME/DATAIN"
mkdir -p $INPUT

# The path to your output folder
OUTPUT="$HOME/DATAOUT"
mkdir -p $OUTPUT

# Checking for podman/docker and auto-picking
# podman also has podman-docker, which is a bit annoying since you need to know
# which you're running for certain settings.  However in that case we should still
# find podman first...
DOCKERBIN="podman"
PODMAN=true
if ! type "$DOCKERBIN" &> /dev/null
then
  DOCKERBIN="docker"
  PODMAN=false
  if ! type "$DOCKERBIN" &> /dev/null
  then
   echo "You don't appear to have podman or docker in your path, stopping"
   exit
  fi
fi

# Function for 'stock' calling docker/podman with the general directories
# you can modify to add extra settings
function dockerrun(){  # extra line

   # Have to be careful to preserve the quoted passed in args for the container
   # $@ does this, but it also include the extra command we passed so pull it out
   #echo "GOT: ${*@Q}"
   command=$1
   shift
# Podman needs the extra Z,U for folder permissions
if [ "$PODMAN" = true ]; 
then
# Podman --------------------------------
$DOCKERBIN run -it -h "rapio" \
    -v "$INPUT":/INPUT:Z,U \
    -v "$OUTPUT":/OUTPUT:Z,U \
    -v "$MYALGPATH":/BUILD/MYALGS:Z,U \
    --user $(id -u):$(id -g) \
    $command "$@"

else
# Docker --------------------------------
$DOCKERBIN run -t \
    -v "$INPUT":/INPUT \
    -v "$OUTPUT":/OUTPUT \
    -v "$MYALGPATH":/BUILD/MYALGS \
    --user $(id -u):$(id -g) \
    $command "$@"

fi
}

