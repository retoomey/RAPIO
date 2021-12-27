#!/bin/sh
# FIXME: Add some versions, etc I think like python at some point
gcc=`rpm -qi gcc-c++ | grep Version ` 
cat << EOF
=============================================================================
   ___  ___   ___  ________ 
  / _ \/ _ | / _ \/  _/ __ \             Development Docker July 2021
 / , _/ __ |/ ___// // /_/ /             Built on Fedora 35
/_/|_/_/ |_/_/  /___/\____/              g++ $gcc

=============================================================================
EOF

# Set up template folder in myalgs if not already there
if [ ! -d "/BUILD/MYALGS/TEMPLATE" ]; then
  echo "...Setting up TEMPLATE into /BUILD/MYALGS"
  cp -r /tmp/PACKAGES/RAPIO/TEMPLATE/ /BUILD/MYALGS/.
  cp -f /tmp/PACKAGES/RAPIO/makealgdir.sh /BUILD/MYALGS/.
  cp -f /tmp/PACKAGES/RAPIO/makedocs.sh /BUILD/MYALGS/.
fi

if [ ! -d "/BUILD/MYALGS/RAPIOConfig" ]; then
  echo "...Installing RAPIOConfig settings, etc. into /BUILD/MYALGS"
  cp -r /BUILD/RAPIO/RAPIOConfig /BUILD/MYALGS/RAPIOConfig
fi
