#!/bin/bash
# Pretty test for changes using uncrustify
folders="base rexample PYTHON tests programs"
crust="uncrustify"
if (! hash $crust 2>/dev/null); then
  echo "$crust does not appear to be in your path"
  echo "If you have docker or podman installed, you could build the uncrustify container (see ./container)"
  exit
fi

for f in $folders; do
  echo "Checking pretty print of *.cc and *.h in $f..."
  LIST=`find "$f" -type f -name "*.cc"`
  #echo $LIST
  for l in $LIST; do
    $crust -c uncrustify.cfg --check $l | grep FAIL
  done
  LIST=`find "$f" -type f -name "*.h"`
  for l in $LIST; do
    $crust -c uncrustify.cfg --check $l | grep FAIL
  done
done
