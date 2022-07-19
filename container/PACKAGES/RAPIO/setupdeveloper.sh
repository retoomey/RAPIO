# Toomey July 2022
# Install developer user as current building user, unless root
# This runs on container during build to link the user you are, with
# the developer user on the container.  This makes things easier when
# reading/writing data.
# $1 userid
# $2 usergroupid

ID=$1
GROUP=$2

# Make sure a number
case $ID in
   ''|*[!0-9]*) ID=1000 ;;
   *)  ;;
esac
case $GROUP in
   ''|*[!0-9]*) GROUP=1000 ;;
   *)  ;;
esac

if [ $ID -eq 0 ]; then
  echo " Creating developer as user id 1000 since running as root"
  ID=1000
  GROUP=1000
fi

echo " Using ids $ID/$GROUP for developer"

groupadd -g $2 developer
useradd -u $1 -g $2 developer

# Allow sudo/root for developer
usermod -a -G wheel developer
