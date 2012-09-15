#!/bin/sh

# Temp install:
# curl https://dl.dropbox.com/s/ib27qzu83lhowar/install.sh | sh

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
curl -s https://dl.dropbox.com/s/w2fr0pkmvzm6imq/editor-0.0.2.tar.gz | tar xzf -

echo "**** Installing required libraries (node, npm, redis-server) ****"
sudo apt-get install nodejs npm redis-server -y

rm -rf "$WEBIDE_ROOT/tmp"
mkdir "$WEBIDE_ROOT/tmp"
npm config set tmp "$WEBIDE_ROOT/tmp"
npm install
cd "$WEBIDE_ROOT/editor"
npm install
npm config set tmp /tmp

echo "**** The Adafruit WebIDE is installed! ****"
echo "**** To run the editor: ****"
echo "**** cd ~/Adafruit/WebIDE ****"
echo "**** node webide ****"