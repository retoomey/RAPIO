#!/bin/sh
release=`cat /etc/redhat-release ` 
cat << EOF
=============================================================================
                                         Operations Docker July 2022
  MRMS BASE CONTAINER                    $release
  ***OPERATIONS***

  Note: This contains all third party libraries required by 
    MRMS, HMET, RAPIO.  However, no code is on this or ability to build.
=============================================================================
EOF
