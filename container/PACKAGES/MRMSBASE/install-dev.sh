# Toomey July 2022
#
# Add -devel rpms, compilers, etc.  This makes the image a bit bigger
# than the operations one
#

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
  $aCommand
}

# -------------------------------------------------------------
echo "   Substep Install extra stock packages..."
#run "dnf --nodocs -y upgrade-minimal"

# Any Repo access
install "svn git"

# -------------------------------------------------------------
# Basic compiler stuff, RAPIO core (subset of WDSSII/MRMS)
# Compiler stuff
# gcc-toolset-9-gcc-c++ is on here too, we'll go with stock
# libtool 'should' snag autoconf, automake, m4
install "gcc-c++ gdb make libtool"
install "cmake"

install "xz-devel bzip2-devel"

# Development libraries
# expat-devel (udunits requirement)
install "expat-devel libpng-devel openssl-devel"

# The big ones... Third Party. 
install "boost-devel"
install "netcdf-devel"
install "udunits2-devel"
install "g2clib-devel"
# This one is like 500mb..projection.  Maybe we could use gdal projection instead?
install "proj-devel"
install "gdal-devel"
# (image) The image plugin for RAPIO uses imagick
# I'm gonna go ahead and do this on these containers since all dnf
# is slow and we'll rebuild subcontainers a lot more then these base
# ones.  However you could 'squeeze' a bit more space by removing these
echo "   Substep Install supplemental packages..."
install "GraphicsMagick-c++-devel"

# Doxygen stuff for documentation creation
install "doxygen"
install "graphviz"

# Save some final image size
run "dnf clean all"

# -------------------------------------------------------------
echo "   Substep Install motd..."
run "mv /tmp/PACKAGES/MRMSBASE/motd-dev.sh /etc/profile.d/motd.sh"

# -------------------------------------------------------------
# Setup non versioned g2clib for MRMS/etc...
#run "cp /usr/lib64/libg2c_v1.6.3.a /usr/lib64/libgrib2c.a"

# I'm gonna leave this as an example of building custom,
# even though we have the rpm now in Fedora 35
#echo "   Substep Install/Build g2clib 1.6.3 (not available as RPM)..."
#run "mkdir -p /BUILD/Third"
#run "mv /tmp/PACKAGES/MRMSBASE/g2clib-1.6.3.tar /BUILD/Third/."
#run "cd /BUILD/Third"
#run "tar xvf g2clib-1.6.3.tar"
#run "cd /BUILD/Third/g2clib-1.6.3"
#run "mv makefile makefileBASE"
#run "mv /tmp/PACKAGES/MRMSBASE/g2makefile makefile"
#run "make"
#run "cp *.h /usr/include/."
#run "cp libg2c_v1.6.3.a /usr/lib64/libgrib2c.a"

# Allow developer passwordless sudo here (probably not wanted in production)
run "cp /tmp/PACKAGES/MRMSBASE/95-developer-user /etc/sudoers.d/."

