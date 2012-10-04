Adafruit WebIDE
================
This is a simple editor designed to help learn the Raspberry PI components, and more.

Easiest Installation
============

On the Raspberry PI:

    curl https://dl.dropbox.com/s/ib27qzu83lhowar/install.sh | sh

Manual Installation
============

On the Raspberry PI:

    git clone git://github.com/adafruit/Adafruit-WebIDE.git
    cd Adafruit-WebIDE
    sudo apt-get install nodejs npm redis-server -y
    mkdir tmp
    npm config set tmp tmp
    npm install
    cd editor
    npm install
    cd ..
    node webide.js

Uninstallation
============

On the Raspberry PI:

    curl https://dl.dropbox.com/s/b8n0a28gwgd38he/uninstall.sh | sh

Running the Editor
============

    http://raspberrypi.local:3000

SCREENSHOTS
===========
Coming soon