# Toomey July 2021
# -------------------------------------------------------------

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

# (image) The image plugin for RAPIO uses imagick
# I put this into mrmsbase to speed up redoing this container
# since the mrmsbase should rarely change
#echo "   Substep Install GraphicsMagick-c++ for ioimage plugin..."
#install "GraphicsMagick-c++-devel"

# -------------------------------------------------------------
echo "   Substep Install RAPIO motd..."
run "mv /tmp/PACKAGES/RAPIO/motd-dev.sh /etc/profile.d/motd.sh"

echo "   Substep Build/Install RAPIO Development Library..."
run "mkdir -p /BUILD"
run "cd /BUILD"
run "git clone https://github.com/retoomey/RAPIO/"
run "cd RAPIO"
# Migrating to cmake...
run "./autogen.sh --prefix=/usr --usecmake"

# Too high a j here can freak the container (out of memory), we'll try 3
# if you have issues, safest is -j 1
run "cd BUILD"
run "make -j 3 install"

# Finally change ownership to developer so they can play, but not install
if id -u "developer" >/dev/null 2>&1; then
  echo "    Changing /BUILD to developer ownership."
  run "chown developer:developer -R /BUILD"
else
  echo "    developer user doesn't exist, root on container not recommended."
fi

