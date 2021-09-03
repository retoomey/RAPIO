# Toomey July 2021
# -------------------------------------------------------------
echo "   Substep Build/Install RAPIO Development Library..."
cd /BUILD
git clone https://github.com/retoomey/RAPIO/
cd RAPIO
# Migrating to cmake...
./autogen.sh --prefix=/usr --usecmake

# Too high a j here can freak the container (out of memory), we'll try 3
# if you have issues, safest is -j 1
cd BUILD
make -j 3 install

# Finally change ownership to developer so they can play, but not install
if id -u "developer" >/dev/null 2>&1; then
  echo "    Changing /BUILD to developer ownership."
  chown developer:developer -R /BUILD
else
  echo "    developer user doesn't exist, root on container not recommended."
fi

