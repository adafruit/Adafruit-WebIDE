Adafruit WebIDE
================
This is a simple editor designed to help learn the Raspberry PI components, and more.

Easiest Installation
============

On the Raspberry PI:

    curl https://raw.github.com/adafruit/Adafruit-WebIDE/release/scripts/install.sh | sh

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

    curl https://raw.github.com/adafruit/Adafruit-WebIDE/release/scripts/uninstall.sh | sh

Manual Uninstallation
============

On the Raspberry PI:

    rm -r Adafruit-WebIDE
    rm ~/.ssh/id_rsa_bitbucket*  

Running the Editor
============

Using Firefox or Chrome (and likely any other webkit browser) on any computer in your internal network:

    http://raspberrypi.local:3000

Offline Mode
============

Yup, there is basic support for offline mode.  Just switch the 'offline' flag in the editor/config/config.js file to true when you're coding on a boat, in a submarine, or on your bicycle (not recommended).

SCREENSHOTS
===========
Coming soon