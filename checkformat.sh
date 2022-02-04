#!/bin/bash
# Pretty test for changes using uncrustify
folders="base base/ionetcdf base/iogrib base/iogdal base/ioimage base/ioraw base/iohmrg rexample PYTHON tests"
crust="uncrustify"
if (! hash $crust 2>/dev/null); then
  echo "$crust does not appear to be in your path"
  exit
fi

for f in $folders; do
  echo "Checking pretty print of *.cc and *.h in $f..."
  $crust -c uncrustify.cfg --check $f/*.cc | grep FAIL
  $crust -c uncrustify.cfg --check $f/*.h | grep FAIL
done
