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
  echo "Checking pretty print of files in $f (skipping w*.cc)..."
  
  # Use find with -not -name to filter out Emscripten files
  # Find .cc and .h files, but strictly exclude w* webassembly files
  find "$f" -type f \( -name "*.cc" -o -name "*.h" \) -not -name "w*" | while read -r l; do
    $crust -c uncrustify.cfg --check "$l" | grep FAIL
  done
done
