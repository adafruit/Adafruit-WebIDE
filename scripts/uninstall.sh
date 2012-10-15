#!/bin/sh

# curl https://raw.github.com/adafruit/Adafruit-WebIDE/release/scripts/uninstall.sh | sudo sh


WEBIDE_ROOT="/usr/share/adafruit"
WEBIDE_HOME="/home/webide"
NODE=$(which node)

echo "**** Removing restartd WebIDE configuration ****"
sed -i '/adafruit-webide.sh/ d' /etc/restartd.conf

echo "**** Removing webide user from sudoers ****"
if [ -f "/etc/sudoers.tmp" ]; then
    rm /etc/sudoers.tmp
fi
cp /etc/sudoers /etc/sudoers.tmp
sed -i '/webide ALL/ d' /etc/sudoers.tmp
visudo -c -f /etc/sudoers.tmp
if [ "$?" -eq "0" ]; then
    cp /etc/sudoers.tmp /etc/sudoers
fi
rm /etc/sudoers.tmp

echo "*** Removing access to port 80"
setcap -r "$NODE"

echo "**** Stopping the Adafruit WebIDE ****"
service adafruit-webide.sh stop

echo "**** Removing update-rc.d service ****"
update-rc.d -f adafruit-webide.sh remove
rm /etc/init.d/adafruit-webide.sh
echo "**** Removing the WebIDE Folder ****"
rm -rf "$WEBIDE_ROOT"
echo "**** Removing webide user ****"
userdel -r webide

echo "**** The Adafruit WebIDE is now uninstalled! ****"