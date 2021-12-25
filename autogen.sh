#!/bin/sh
# Toomey Aug 2021
# Generic help script in linux to wrap CMAKE installing for root and local folder installation
# Basically MRMS/HMET and RAPIO we used to do autogen.sh --prefix=/path called from other scripts,
# so we want this same ability for cmake.  This can be done with the CMAKE_INSTALL_PREFIX
# This will allow a softer transition from autotools to cmake
#
# autogen.sh --prefix=/myinstall/path --usecmake # cmake
# autogen.sh --prefix=/myinstall/path            # autotools
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

# Iterate over arguments get the sent options, vs. defaults listed here
BUILDPREFIX="/usr"   # Where we install stuff (--prefix)
USEAUTOTOOLS="true"  # Do we use autotools or cmake (--usecmake)
for var in "$@"
do
  # Get the "--prefix"
  hasprefix=${var#"--prefix="}
  if [ ! "$hasprefix" = "$var" ]; then
    BUILDPREFIX=$hasprefix
  fi
  # Get the "--usecmake"
  usecmake=${var#"--usecmake"}
  if [ ! "$usecmake" = "$var" ]; then
    USEAUTOTOOLS="false"
  fi
done

# Check install directory location
# I'm enforcing it preexisting at moment
if [ ! -d $BUILDPREFIX ]; then
  echo "Prefix directory $BUILDPREFIX doesn't exist."
  exit
fi

# Autotools build.  Which since I'm not using will probably
# break at some point unless I keep checking it.  The plan is
# to retire autotools in RAPIO and MRMS/HMET and join the modern world
if [ "$USEAUTOTOOLS" = "true" ]; then
  echo "Using autotools to build to install prefix $BUILDPREFIX"
  autoreconf -vi
  ./configure "$@"
  echo "Now type 'make ' to compile $PROJECT."
else
  # CMake build.

  # My hunting of cmake binary. Some olders OS have cmake3 vs cmake which is version 2
  # _or_ like fedora they have cmake3 and cmake currently pointing to 3.
  # so I'll hunt for that in case other distros do it.  And of course later I could see
  # cmake4 existing, etc. in the continual development vs stable cycle.
  # Note: you'll have to mod this if at some point if you have a cmake3 and a cmake that's
  # a higher version that you want to use.
  # Of course if you're smart enough for this you probably already just typed it yourself
  CMAKEBINARY="cmake3"  
  if ! type "$CMAKEBINARY" &> /dev/null
  then
    CMAKEBINARY="cmake"
    if ! type "$CMAKEBINARY" &> /dev/null
    then
      echo "Couldn't find cmake or cmake3 in path for building, ask IT to install."
    fi
    exit
  fi

  echo "CMAKE defaulting install prefix to $BUILDPREFIX."
  # This is just to do a fully 'fresh' build without the cache iff you call autogen.sh
  if test -f "./BUILD/CMakeCache.txt"; then
    echo "-- Removing CMakeCache.txt for a fresh build (autogen.sh)"
    rm ./BUILD/CMakeCache.txt
  fi
  $CMAKEBINARY -DCMAKE_INSTALL_PREFIX=$BUILDPREFIX -S. -B./BUILD
  echo "Now type 'cd BUILD; make ' to compile $PROJECT."
fi

