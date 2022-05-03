#!/bin/bash
# Toomey: Pretty print code using uncrustify

#is_user_root () { [ "${EUID:-$(id -u)}" -eq 0 ]; }

folders="base base/ionetcdf base/iogrib base/iogdal base/ioimage base/ioraw base/iohmrg rexample PYTHON tests"
crust="uncrustify"
if (! hash $crust 2>/dev/null); then
  echo "$crust does not appear to be in your path"
  echo "If you have docker or podman installed, you could build the uncrustify container (see ./container)"
  exit
fi

echo "You have uncrustify installed, typically I pretty format before git adding/checking in files."
read -p "This will overwrite *.cc and *.h files with pretty printed copies, are you sure? [Y/N] >" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
  for f in $folders; do
    echo "Checking pretty print of *.cc and *.h in $f..."
    # Run it twice on each file, some quirk of uncrustify the files aren't always correct after first format
    $crust -c uncrustify.cfg --no-backup $f/*.cc
    $crust -c uncrustify.cfg --no-backup $f/*.cc
    $crust -c uncrustify.cfg --no-backup $f/*.h
    $crust -c uncrustify.cfg --no-backup $f/*.h
    #if is_user_root; then
    # Match read/write and user to the CMakeLists.txt.  This is especially important
    # on docker which may be running as another user
    chown --reference=CMakeLists.txt $f/*.cc
    chmod --reference=CMakeLists.txt $f/*.cc
    chown --reference=CMakeLists.txt $f/*.h
    chmod --reference=CMakeLists.txt $f/*.h
  done
fi

