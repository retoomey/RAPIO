#!/bin/sh
# ^^^^^ Ok if using sh we need to be POSIX compliant for things like dash on ubuntu
#        See www.shellcheck.net with #!/bin/dash as prefix to verify script changes,
#        That project is pretty cool.
#
# Toomey Aug 2021/May 2023
# Generic help script in linux to wrap CMAKE installing for root and local folder installation
#
# If you already are a cmake master you can ignore this and just use cmake normally, however
# if you're coming from NSSL autotools/MRMS building this tries to help you somewhat.
# I would recommend first running "autogen.sh --dry-run" and see the output
# Cmake is a bit different it that you don't have to build 'in-place' so you can have multiple
# builds using the same source tree.  This is nice but slightly confusing if you're
# used to the mess that autotools typically makes of your source tree.
# Basically MRMS/HMET and RAPIO we used to do autogen.sh --prefix=/path called from other scripts.
#
# autogen.sh # Attempts to build standard build installed one directory up from script and using
#            # a BUILD folder locally.
#            # Note: the BUILD folder is where it's built
#            #       the PREFIX is where it's installed (bin, lib, etc.)
#            #       the S in the cmake command is the source code location
#
# autogen.sh --prefix=/myinstall/path # cmake to ./BUILD in script directory
# autogen.sh --prefix=/myinstall/path --build=/home/me/buildhere # build to /home/me/buildhere
# autogen.sh --prefix=/myinstall/path --usescanbuild  # Build using clang-analyzer
#
# You can ignore this script if you're comfortable with cmake and do the basic commands.
# The recommendation in cmake is to keep a build seperate from source, but that is personal
# preference:
#    cmake -DCMAKE_INSTALL_PREFIX={$BUILDPREFIX} -S. -B./BUILD
#    cd BUILD; make -j; make install
# So for example, a MRMS build at /home/me/MRMS_01012032 with the bin, lib, etc. you would
# do -DCMAKE_INSTALL_PREFIX=/home/me/MRMS_01012032
# 
PROJECT="RAPIO"  # Just for printing project info (WDSS2, W2ALGS, RAPIO, etc.)
SCRIPT_DIR="$(cd -- "$(dirname -- "$0")" && pwd -P)"
PARENT_DIR="$(dirname -- "$SCRIPT_DIR")"

# Iterate over arguments get the sent options, vs. defaults listed here
BUILDPREFIX="$PARENT_DIR"   # Where we install stuff (--prefix), default is up from script one directory
SOURCEDIR="$SCRIPT_DIR"  # Use the source from the location of this autogen.sh script
USESCANBUILD="false" # Do we try the scan-build static analyzer? I think this auto uses clang compiler
                     # We wrap this around cmake only
OUTFOLDER="./BUILD"  # Default output folder for build (--build)
DRYRUN="false"       # Dry run similiar to aws cli (--dry-run)

# Hack for PATHS.  The set PATH gets searched for libraries but the cmake version isn't
# handling the NO_SYSTEM_ENVIRONMENT_PATH properly.  The PATH can be set in MRMS to
# binaries, etc. with libraries and cmake will find those which is wrong.
# FIXME: to be fair, probably need a rework of CMakeLists.txt
# This seems to be something they changed in cmake back and forth, the searching of the default
# ENV path libraries.
if test -f "/etc/redhat-release"; then
  echo "Enforcing default PATH for redhat release to avoid PATH bug"
  PATH="/bin:/usr/sbin:/usr/bin:/usr/lib64/ccache:/usr/local/bin"
fi

# Match a boolean param --someparam
boolParam() {
  what="$1"
  param="$2"
  match=${what#"--$param"}
  if [ "$match" = "$var" ]; then
    return 1
  fi
  return 0
}

# Match a string param --someparam=string
stringParam() {
  what="$1"    #--prefix=test
  param="$2"   #prefix
  match=${what#"--$param="}
  if [ "$match" = "$var" ]; then
    echo "$3"
  else
    echo "$match"
  fi
}

# Search for our special options.
for var in "$@"
do
  if boolParam "$var" "dry-run"; then
    DRYRUN="true"
  fi
  if boolParam "$var" "usescanbuild"; then
    USESCANBUILD="true"
  fi
  BUILDPREFIX=$(stringParam "$var" "prefix" "$BUILDPREFIX")
  OUTFOLDER=$(stringParam "$var" "build" "$OUTFOLDER")
  SOURCEDIR=$(stringParam "$var" "source" "$SOURCEDIR")
done

if [ "$SOURCEDIR" = "$PWD" ]; then
  SOURCEDIR="." # We're in the same directory
fi

# Check install directory location
# I'm enforcing it preexisting at moment
if [ ! -d "$BUILDPREFIX" ]; then
  echo "Prefix directory $BUILDPREFIX doesn't exist."
  exit
fi

# CMake build.

# My hunting of cmake binary. Some olders OS have cmake3 vs cmake which is version 2
# _or_ like fedora they have cmake3 and cmake currently pointing to 3.
# so I'll hunt for that in case other distros do it.  And of course later I could see
# cmake4 existing, etc. in the continual development vs stable cycle.
# Note: you'll have to mod this if at some point if you have a cmake3 and a cmake that's
# a higher version that you want to use.
# Of course if you're smart enough for this you probably already just typed it yourself
CMAKEBINARY="cmake3"  
if ! type "$CMAKEBINARY" > /dev/null 2>&1
then
  CMAKEBINARY="cmake"
  if ! type "$CMAKEBINARY" > /dev/null 2>&1
  then
    echo "Couldn't find cmake or cmake3 in path for building, ask IT to install."
    exit
  fi
fi

# On scan-build, wrap cmake with scan-build command, this will setup for
# static analyzer
extratext=""
if [ "$USESCANBUILD" = "true" ]; then

  # Check for scan-build and scan-view
  if ! type "scan-build" > /dev/null 2>&1
  then
    if ! type "scan-view" > /dev/null 2>&1
    then
      echo "Can't find scan-build and/or scan-view in your path from clang-analyzer tools."
      echo "You need this for the --usescanbuild option."
      exit
    fi
  fi
  CMAKEBINARY="scan-build $CMAKEBINARY"
  extratext="scan-build "
fi 

echo "$CMAKEBINARY install prefix is $BUILDPREFIX."

# Older cmake doesn't have -S and -B, so we'll just do the cd/mkdir ourselves
#docommand="$CMAKEBINARY -DCMAKE_INSTALL_PREFIX=$BUILDPREFIX -S$SOURCEDIR -B$OUTFOLDER"
docommand="$CMAKEBINARY -DCMAKE_INSTALL_PREFIX=$BUILDPREFIX .."
if [ "$DRYRUN" = "false" ]; then
  # This is just to do a fully 'fresh' build without the cache iff you call autogen.sh
  if test -f "$OUTFOLDER/CMakeCache.txt"; then
    echo "-- Removing CMakeCache.txt for a fresh build (autogen.sh)"
    rm "$OUTFOLDER/CMakeCache.txt"
  fi
   # -DCMAKE_BUILD_TYPE="Debug"

   # ----------------------------------------
   # Older cmake we'll cd into the scriptdir, make sure the outfolder exists
   # then cd into it to run the cmake command.  Which is the same as -S. -B$OUTFOLDER
   cd "$SCRIPT_DIR" || { echo "Failed to change directory to $SCRIPT_DIR"; exit 1; }
   mkdir -p $OUTFOLDER || { echo "Unable to create output build $OUTFOLDER"; exit 1; }
   cd "$OUTFOLDER" || { echo "Unable to cd into $OUTFOLDER"; exit 1; }
   # ----------------------------------------

   $docommand
else
  echo "TEST RUN"
fi
echo "CMake command used:"
echo "  $docommand"
echo "Now type 'cd $OUTFOLDER; ${extratext} make ' to compile $PROJECT."

