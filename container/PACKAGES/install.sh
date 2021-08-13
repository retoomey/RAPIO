# Toomey July 2021
# Doing in a script saves docker layer memory, with the downside
# of taking longer to regenerate if we add a bug

# -------------------------------------------------------------
echo "   Substep Install extra stock packages..."
# Utility stuff/useful for admin 
# color ls on container/install please
yum install vim coreutils-common -y

# Any Repo access
yum install svn git -y

# -------------------------------------------------------------
# Basic compiler stuff, RAPIO core (subset of WDSSII/MRMS)
# Compiler stuff
# gcc-toolset-9-gcc-c++ is on here too, we'll go with stock
# libtool 'should' snag autoconf, automake, m4
yum install gcc-c++ gdb make libtool -y
yum install cmake -y

# Compression/Decompression libraries
yum install unzip xz-devel bzip2-devel -y

# Development libraries
# expat-devel (udunits requirement)
yum install expat-devel libpng-devel openssl-devel -y

# (image) The image plugin for RAPIO uses imagick
yum install GraphicsMagick-c++-devel -y

# The big ones... Third Party. 
yum install boost-devel -y
yum install netcdf-devel -y
yum install udunits2-devel -y
yum install g2clib-devel -y
# This one is like 500mb..projection.  Maybe we could use gdal projection instead?
yum install proj-devel -y
yum install gdal-devel -y

# Doxygen stuff for documentation creation
yum install doxygen -y
yum install graphviz -y

# Save some final image size
yum clean all

# -------------------------------------------------------------
echo "   Substep Install motd..."
mv /tmp/PACKAGES/motd.sh /etc/profile.d/motd.sh

# -------------------------------------------------------------
echo "   Substep Install/Build g2clib 1.6.3 (not available as RPM)..."
mkdir -p /BUILD/Third
#mkdir -p /BUILD/include
#mkdir -p /BUILD/lib
mv /tmp/PACKAGES/g2clib-1.6.3.tar /BUILD/Third/.
cd /BUILD/Third
tar xvf g2clib-1.6.3.tar
cd /BUILD/Third/g2clib-1.6.3
mv makefile makefileBASE
mv /tmp/PACKAGES/g2makefile makefile
make
cp *.h /usr/include/.
cp libg2c_v1.6.3.a /usr/lib64/libgrib2c.a

# -------------------------------------------------------------
echo "   Substep Build/Install RAPIO Development Library..."
cd /BUILD
git clone https://github.com/retoomey/RAPIO/
cd RAPIO
# Migrating to cmake...
./autogen.sh --prefix=/usr --usecmake

# Too high a j here can freak the container (out of memory), we'll try 3
# if you have issues, safest is -j 1
make -j 3 install

# Finally change ownership to developer so they can play, but not install
if id -u "developer" >/dev/null 2>&1; then
  echo "    Changing /BUILD to developer ownership."
  # Allow developer passwordless sudo here (probably not wanted in production)
  cp /tmp/PACKAGES/95-developer-user /etc/sudoers.d/.
  chown developer:developer -R /BUILD
else
  echo "    developer user doesn't exist, root on container not recommended."
fi

