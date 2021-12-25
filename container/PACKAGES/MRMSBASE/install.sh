# Toomey July 2021
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

# -------------------------------------------------------------
echo "   Substep Install extra stock packages..."
# Utility stuff/useful for admin 
# color ls on container/install please
run "yum install vim coreutils-common -y"

# Any Repo access
run "yum install svn git -y"

# -------------------------------------------------------------
# Basic compiler stuff, RAPIO core (subset of WDSSII/MRMS)
# Compiler stuff
# gcc-toolset-9-gcc-c++ is on here too, we'll go with stock
# libtool 'should' snag autoconf, automake, m4
run "yum install gcc-c++ gdb make libtool -y"
run "yum install cmake -y"

# Compression/Decompression libraries
run "yum install unzip xz-devel bzip2-devel -y"

# Development libraries
# expat-devel (udunits requirement)
run "yum install expat-devel libpng-devel openssl-devel -y"

# The big ones... Third Party. 
run "yum install boost-devel -y"
run "yum install netcdf-devel -y"
run "yum install udunits2-devel -y"
run "yum install g2clib-devel -y"
# This one is like 500mb..projection.  Maybe we could use gdal projection instead?
run "yum install proj-devel -y"
run "yum install gdal-devel -y"

# Doxygen stuff for documentation creation
run "yum install doxygen -y"
run "yum install graphviz -y"

# Save some final image size
run "yum clean all"

# -------------------------------------------------------------
echo "   Substep Install motd..."
run "mv /tmp/PACKAGES/MRMSBASE/motd.sh /etc/profile.d/motd.sh"

# -------------------------------------------------------------
# Setup non versioned g2clib for MRMS/etc...
run "cp /usr/lib64/libg2c_v1.6.3.a /usr/lib64/libgrib2c.a"

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

