#!/bin/bash

# Toomey June 2025
# A patch script for external cmake project
#

# Usage check
if [[ "$#" -ne 3 ]]; then
  echo "Usage: $0 <mode> <source_dir> <dest_dir>"
  exit 1
fi

mode="$1"
src="$2"
dst="$3"

# Copy a file iff different (we need to update patch)
copy_if_different() {
  csrc="$1"
  cdst="$2"
  if [ ! -f "$cdst" ] || ! cmp -s "$csrc" "$cdst"; then
    cp "$csrc" "$cdst"
    echo "       Copied $csrc -> $cdst"
  else
    echo "       Skipped $csrc, identical"
  fi
}

# First call to update, the directory won't exist.  It has
# to be pulled and then patched by the PATCH_COMMAND
if [ ! -d "$dst" ]; then
  #echo "       Target directory does not exist: $dst — skipping patch."
  exit 0
fi

case "$mode" in
  first)
    #echo "       Applying first time patching to wgrib2"
    ;;
  update)
    #echo "       Updating wgrib2 with new patch"
    ;;
  *)
    echo "       Unknown mode: $mode"
    exit 1
    ;;
esac

# Copy the files/patch to location if changed only.
# cmake's copy_if_different modifies timestamps and triggers false rebuilds
copy_if_different "$src"/fnlist.c "$dst"/fnlist.c
copy_if_different "$src"/fnlist.h "$dst"/fnlist.h
copy_if_different "$src"/f_rapio_callback.c "$dst"/f_rapio_callback.c
copy_if_different "$src"/f_rapio_callback.h "$dst"/f_rapio_callback.h
copy_if_different "$src"/wgrib2_CMakeLists.txt "$dst"/CMakeLists.txt
