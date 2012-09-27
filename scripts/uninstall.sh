#!/bin/sh

# Temp install:
# curl https://dl.dropbox.com/s/b8n0a28gwgd38he/uninstall.sh | sh


WEBIDE_ROOT="$HOME/Adafruit/WebIDE"

echo "**** Stopping the Adafruit WebIDE ****"
service adafruit-webide.sh stop

echo "**** Removing update-rc.d service ****"
sudo update-rc.d -f adafruit-webide.sh remove
sudo rm /etc/init.d/adafruit-webide.sh
echo "**** Removing the WebIDE Folder ****"
rm -rf "$WEBIDE_ROOT"
echo "**** Removing auto-generated Bitbucket SSH keys ****"
cd "$HOME/.ssh"
rm id_rsa_bitbucket*

echo "**** The Adafruit WebIDE is now uninstalled! ****"