#!/bin/sh

# curl https://raw.github.com/adafruit/Adafruit-WebIDE/release/scripts/install.sh | sh

set -e
WEBIDE_ROOT="$HOME/Adafruit/WebIDE"

#NODE_PATH="/usr/local/lib/node"

#if [ ! -d "$NODE_PATH" ]; then
#  mkdir -p "$NODE_PATH"
#    # Control will enter here if $DIRECTORY doesn't exist.
#fi

mkdir -p "$WEBIDE_ROOT"
cd "$WEBIDE_ROOT"

echo "**** Downloading the latest version of the WebIDE ****"
curl -sL https://github.com/downloads/adafruit/Adafruit-WebIDE/editor-0.1.8.tar.gz | tar xzf -

echo "**** Installing required libraries (node, npm, redis-server) ****"
sudo apt-get install nodejs npm redis-server git -y

rm -rf "$WEBIDE_ROOT/tmp"
mkdir "$WEBIDE_ROOT/tmp"
npm config set tmp "$WEBIDE_ROOT/tmp"
npm install
cd "$WEBIDE_ROOT/editor"
npm install
npm config set tmp /tmp

echo "**** Installing the WebIDE as a service ****"
echo "**** (to uninstall service, execute: 'sudo update-rc.d -f adafruit-webide.sh remove') ****"
sudo cp "$WEBIDE_ROOT/scripts/adafruit-webide.sh" "/etc/init.d"
cd /etc/init.d
sudo chmod 755 adafruit-webide.sh
sudo update-rc.d adafruit-webide.sh defaults
service adafruit-webide.sh start
echo "**** Starting the server...(please wait) ****"
sleep 15s

echo "**** The Adafruit WebIDE is installed and running! ****"
echo "**** Commands: service adafruit-webide.sh {start,stop,restart} ****"
echo "**** Navigate to http://raspberrypi.local:3000 to use the WebIDE"
#echo "**** To run the editor: ****"
#echo "**** cd ~/Adafruit/WebIDE ****"
#echo "**** node webide ****"