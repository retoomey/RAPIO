#!/bin/sh
# ^^^^^ Ok if using sh we need to be POSIX compliant for things like dash on ubuntu
#        See www.shellcheck.net with #!/bin/dash as prefix to verify script changes,
#        That project is pretty cool.
# Toomey Aug 2021
# Generic help script in linux to wrap CMAKE installing for root and local folder installation
# Basically MRMS/HMET and RAPIO we used to do autogen.sh --prefix=/path called from other scripts,
# so we want this same ability for cmake.  This can be done with the CMAKE_INSTALL_PREFIX
# This will allow a softer transition from autotools to cmake
#
# autogen.sh --prefix=/myinstall/path --usecmake # cmake
# autogen.sh --prefix=/myinstall/path            # autotools
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

# Iterate over arguments get the sent options, vs. defaults listed here
BUILDPREFIX="/usr"   # Where we install stuff (--prefix)
USEAUTOTOOLS="true"  # Do we use autotools or cmake (--usecmake)
USESCANBUILD="false" # Do we try the scan-build static analyzer? I think this auto uses clang compiler
                     # We wrap this around cmake only

# Hack for PATHS.  The set PATH gets searched for libraries but the cmake version isn't
# handling the NO_SYSTEM_ENVIRONMENT_PATH properly.  The PATH can be set in MRMS to
# binaries, etc. with libraries and cmake will find those which is wrong.
# FIXME: to be fair, probably need a rework of CMakeLists.txt
# This seems to be something they changed in cmake back and forst, the searching of the default
# ENV path libraries.
if test -f "/etc/redhat-release"; then
  echo "Enforcing default PATH for redhat release to avoid PATH bug"
  PATH="/bin:/usr/sbin:/usr/bin:/usr/lib64/ccache:/usr/local/bin"
fi

# Search for our special options.
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
  # Get the "--usescanbuild"
  usescanbuild=${var#"--usescanbuild"}
  if [ ! "$usescanbuild" = "$var" ]; then
    USESCANBUILD="true"
  fi
done

# Check install directory location
# I'm enforcing it preexisting at moment
if [ ! -d "$BUILDPREFIX" ]; then
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

  echo "$CMAKEBINARY defaulting install prefix to $BUILDPREFIX."
  # This is just to do a fully 'fresh' build without the cache iff you call autogen.sh
  if test -f "./BUILD/CMakeCache.txt"; then
    echo "-- Removing CMakeCache.txt for a fresh build (autogen.sh)"
    rm ./BUILD/CMakeCache.txt
  fi
  $CMAKEBINARY -DCMAKE_INSTALL_PREFIX="$BUILDPREFIX" -S. -B./BUILD
  echo "Now type 'cd BUILD; ${extratext} make ' to compile $PROJECT."
fi

