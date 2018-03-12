#!/usr/bin/env bash

# curl https://raw.githubusercontent.com/adafruit/Adafruit-WebIDE/alpha/scripts/uninstall.sh | sudo sh


WEBIDE_ROOT="/usr/share/adafruit/webide"
WEBIDE_HOME="/home/webide"
NODE_PATH=""

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

echo "**** Stopping the Adafruit WebIDE ****"
service adafruit-webide.sh stop
sleep 5s

echo "**** Removing systemd service ****"
systemctl stop adafruit-webide
systemctl disable adafruit-webide
rm /etc/systemd/system/adafruit-webide.service
systemctl daemon-reload
systemctl reset-failed

echo "**** Removing the WebIDE Folder ****"
shopt -s extglob
rm -rf "$WEBIDE_ROOT"/!(repositories)
echo "**** Removing webide user ****"
userdel -r webide

echo "**** The Adafruit WebIDE is now uninstalled! ****"
echo "**** Your code and repositories remain at $WEBIDE_ROOT ****"
echo "**** During the installation process, there were a few ****"
echo "**** libraries installed that we did not uninstall as ****"
echo "**** we're not able to determine if other applications are dependent ****"
echo "**** on them. If you are not using them, you can uninstall by executing ****"
echo "**** the following command: ****"
echo "**** sudo apt-get remove nodejs-legacy yarn git i2c-tools python-smbus ntp libkrb5-dev ****"
