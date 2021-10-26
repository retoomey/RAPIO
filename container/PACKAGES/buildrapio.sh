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

echo "   Substep Build/Install RAPIO Development Library..."
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

