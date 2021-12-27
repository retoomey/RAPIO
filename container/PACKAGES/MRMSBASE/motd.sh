#!/bin/sh
gcc=`rpm -qi gcc-c++ | grep Version ` 
cat << EOF
=============================================================================
                                         Development Docker July 2021
  MRMS BASE CONTAINER                    Built on Fedora 35
                                         g++ $gcc

  Note: This contains all third party libraries required by 
    MRMS, HMET, RAPIO.  However, no code is on this.
=============================================================================
EOF
