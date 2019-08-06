#!/bin/sh

autoreconf -vi
./configure "$@"

echo "Now type 'make ' to compile RAPIO."

