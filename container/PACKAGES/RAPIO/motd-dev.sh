#!/bin/sh
# FIXME: Add some versions, etc I think like python at some point
release=`cat /etc/redhat-release ` 
gcc=`rpm -qi gcc-c++ | grep Version ` 
cat << EOF
=============================================================================
   ___  ___   ___  ________ 
  / _ \/ _ | / _ \/  _/ __ \             Development Docker July 2022
 / , _/ __ |/ ___// // /_/ /             $release
/_/|_/_/ |_/_/  /___/\____/              g++ $gcc

=============================================================================
EOF

HOSTSTUFF="/BUILD/MYALGS"
# The built location of the config file
DEFAULTCONFIG="/BUILD/RAPIO/RAPIOConfig"

export RAPIO_CONFIG_LOCATION=$DEFAULTCONFIG

if [ -d "$HOSTSTUFF" ]; then

# Set up template folder in myalgs if not already there
if [ ! -d "$HOSTSTUFF/TEMPLATE" ]; then
  echo "...Setting up TEMPLATE into $HOSTSTUFF"
  cp -r /tmp/PACKAGES/RAPIO/TEMPLATE/ $HOSTSTUFF/.
  cp -f /tmp/PACKAGES/RAPIO/makealgdir.sh $HOSTSTUFF/.
  cp -f /tmp/PACKAGES/RAPIO/makedocs.sh $HOSTSTUFF/.
fi

# Try to copy to rapioconfig folder on host if not already there
if [ ! -d "$HOSTSTUFF/RAPIOConfig" ]; then
  echo "...Installing RAPIOConfig settings, etc. into $HOSTSTUFF/RAPIOConfig"
  cp -r $DEFAULTCONFIG $HOSTSTUFF/RAPIOConfig
fi
# and or if the directory valid, use it
if [ -d "$HOSTSTUFF/RAPIOConfig" ]; then
  export RAPIO_CONFIG_LOCATION=$HOSTSTUFF/RAPIOConfig
fi

else
  echo "We expected to find a /BUILD/MYALGS directory."
  echo "Map a host folder to /BUILD/MYALGS if you want host based configuration files,"
  echo "such as colormaps, etc. Otherwise we're using the static ones on the container."
fi

echo "Your configuration location is $RAPIO_CONFIG_LOCATION"
echo "Type rsimplealg to test algorithm linking/working."
