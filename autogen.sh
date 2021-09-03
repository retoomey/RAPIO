#!/bin/sh
# Toomey Aug 2021
# Help script in linux to wrap CMAKE installing for my root and local folder installation
# Basically I used to do autogen.sh --prefix=/path called from other scripts,
# so I want this same ability for cmake.  This can be done with the CMAKE_INSTALL_PREFIX
# This will allow me a softer transition from autotools to cmake
# 
# autogen.sh --prefix=/myinstall/path --usecmake # cmake
# autogen.sh --prefix=/myinstall/path            # autotools

# Iterate over arguments get the 'autotool' style stuff we care about
CMAKEPREFIX="/usr"
USEAUTOTOOLS="true"
for var in "$@"
do
  # Get the "--prefix"
  hasprefix=${var#"--prefix="}
  if [ ! "$hasprefix" = "$var" ]; then
    CMAKEPREFIX=$hasprefix
  fi
  # Get the "--usecmake"
  usecmake=${var#"--usecmake"}
  if [ ! "$usecmake" = "$var" ]; then
    USEAUTOTOOLS="false"
  fi
done
if [ ! -d $CMAKEPREFIX ]; then
  echo "Prefix directory $CMAKEPREFIX doesn't exist"
  exit
fi

if [ "$USEAUTOTOOLS" = "true" ]; then
  echo "Using autotools to build"
  autoreconf -vi
  ./configure "$@"
  echo "Now type 'make ' to compile RAPIO."
else
  echo "CMAKE defaulting install prefix to $CMAKEPREFIX"
  rm ./BUILD/CMakeCache.txt
  cmake -DCMAKE_INSTALL_PREFIX=$CMAKEPREFIX -S. -B./BUILD
  echo "Now type 'cd BUILD; make ' to compile RAPIO."
fi

