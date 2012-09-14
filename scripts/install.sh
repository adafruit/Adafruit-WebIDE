#!/bin/sh

# Temp link https://dl.dropbox.com/s/ib27qzu83lhowar/install.sh

set -e
WEBIDE_ROOT="$HOME/Adafruit/WebIDE"

mkdir -p "$WEBIDE_ROOT"
cd "$WEBIDE_ROOT"

echo "**** Downloading the latest version of the WebIDE ****"
curl -s https://dl.dropbox.com/s/w2fr0pkmvzm6imq/editor-0.0.2.tar.gz | tar xzf -

echo "**** Installing required libraries ****"
sudo apt-get install nodejs npm redis-server -y

rm -rf "$WEBIDE_ROOT/tmp"
mkdir "$WEBIDE_ROOT/tmp"
npm config set tmp "$WEBIDE_ROOT/tmp"
npm install
npm config set tmp /tmp

echo "**** Type: 'node webide.js' to run the Adafruit WebIDE ****"