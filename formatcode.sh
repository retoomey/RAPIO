#!/bin/bash
# Toomey: Pretty print code using uncrustify

#is_user_root () { [ "${EUID:-$(id -u)}" -eq 0 ]; }

folders="base rexample PYTHON tests programs"
crust="uncrustify"

if (! hash $crust 2>/dev/null); then
  echo "$crust does not appear to be in your path"
  echo "If you have docker or podman installed, you could build the uncrustify container (see ./container)"
  exit
fi

echo "You have uncrustify installed, typically I pretty format before git adding/checking in files."
read -p "This will overwrite files (excluding w*.cc) with pretty printed copies, are you sure? [Y/N] >" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
  for f in $folders; do
    echo "Checking pretty print of files in $f (skipping w*.cc)..."
    
    # Use find with -not -name to filter out Emscripten files
    # We use -o (OR) to capture both .cc and .h in one go
    find "$f" -type f \( -name "*.cc" -o -name "*.h" \) -not -name "w*" | while read -r l; do
      
      # Run uncrustify twice.  There's a quirk
      $crust -c uncrustify.cfg --no-backup "$l" | grep FAIL
      $crust -c uncrustify.cfg --no-backup "$l" | grep FAIL
      
      # Sync permissions with CMakeLists.txt
      if [ -f "CMakeLists.txt" ]; then
        chown --reference=CMakeLists.txt "$l"
      fi
    done
  done
fi
