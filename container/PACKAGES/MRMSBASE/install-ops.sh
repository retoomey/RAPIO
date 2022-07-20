# Toomey July 2022
#
# Install ops version of libraries.  No compiler, etc.
# This is meant for smaller size
#
# Doing in a script saves docker layer memory, with the downside
# of taking longer to regenerate if we add a bug

# Break on any error, but messages can be confusing
#set -e

# Run command, break script on failure so docker stops
function run(){
  echo " --> Executing substep: " $1
  $1
  if [ $? -eq 0 ]; then
    echo " --> Successfully executed: " $1
  else
    echo " --> Failed to execute command: " $1
    exit 1
  fi
}

# Install a dnf/yum package with options we want
function install(){
  aCommand="dnf --nodocs -y install --setopt=install_weak_deps=False $1"
  run "$aCommand"
}

# -------------------------------------------------------------
echo "   Substep Install extra stock packages..."
#run "dnf --nodocs -y upgrade-minimal"

# Utility stuff/useful for admin 
# color ls on container/install please
install "vim coreutils-common"

# Compression/Decompression libraries
install "unzip xz bzip2"

# Development libraries
# expat (udunits requirement)
install "expat libpng openssl"

# The big ones... Third Party. 
install "boost"
install "netcdf"
install "udunits2"
install "g2clib-devel"  # This one has it all in one so need devel
install "proj"
install "gdal"
# (image) The image plugin for RAPIO uses imagick
# I'm gonna go ahead and do this on these containers since all dnf
# is slow and we'll rebuild subcontainers a lot more then these base
# ones.  However you could 'squeeze' a bit more space by removing these
echo "   Substep Install supplemental packages..."
install "GraphicsMagick-c++"

# Save some final image size
run "dnf clean all"

# -------------------------------------------------------------
echo "   Substep Install motd..."
run "mv /tmp/PACKAGES/MRMSBASE/motd-ops.sh /etc/profile.d/motd.sh"

# -------------------------------------------------------------
# Setup non versioned g2clib for MRMS/etc...
run "cp /usr/lib64/libg2c_v1.6.3.a /usr/lib64/libgrib2c.a"
