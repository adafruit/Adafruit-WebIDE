#!/usr/bin/env bash

# curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/alpha/scripts/uninstall.sh | sudo sh


WEBIDE_ROOT="/usr/share/adafruit"
WEBIDE_HOME="/home/webide"
NODE_PATH=""

echo "**** Removing restartd WebIDE configuration ****"
sed -i '/adafruit-webide.sh/ d' /etc/restartd.conf
#kill all restartd processes, and restart one
pkill -f restartd || true
sleep 5s
restartd

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

echo "**** Removing access to port 80 for node ****"
NODE_PATH=""
ARCH=$(dpkg --print-architecture)
if [ $ARCH = armhf ]; then
  NODE_PATH="/usr/share/adafruit/webide/bin/node_hf/node"
else
  NODE_PATH="/usr/share/adafruit/webide/bin/node_sf/node"
fi
setcap -r "$NODE_PATH"

echo "**** Stopping the Adafruit WebIDE ****"
service adafruit-webide.sh stop
sleep 5s

echo "**** Removing update-rc.d service ****"
update-rc.d -f adafruit-webide.sh remove
rm /etc/init.d/adafruit-webide.sh
echo "**** Removing the WebIDE Folder ****"
rm -rf "$WEBIDE_ROOT"
echo "**** Removing webide user ****"
userdel -r webide

echo "**** Removing redis server setting ****"
redis-cli del server

echo "**** The Adafruit WebIDE is now uninstalled! ****"
echo "**** During the installation process, there were a few ****"
echo "**** libraries installed that we did not uninstall as ****"
echo "**** we're not able to determine if other applications are dependent ****"
echo "**** on them. If you are not using them, you can uninstall by executing ****"
echo "**** the following command: ****"
echo "**** sudo apt-get remove nodejs npm redis-server git restartd libcap2-bin avahi-daemon i2c-tools python-smbus ****"