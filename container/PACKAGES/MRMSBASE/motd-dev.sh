#!/bin/sh
release=`cat /etc/redhat-release ` 
gcc=`rpm -qi gcc-c++ | grep Version ` 
cat << EOF
=============================================================================
                                         Development Docker July 2022
  MRMS BASE CONTAINER                    $release
  ***DEVELOPMENT***                      g++ $gcc

  Note: This contains all third party libraries required by 
    MRMS, HMET, RAPIO.  However, no code is on this.
=============================================================================
EOF
