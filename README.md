OccEditor
================
This is a simple editor for learning to program the Raspberry PI.

Installation
============

Ensure node.js is installed (tested on 0.6.x):

    which node

Install npm, if not already installed:

    which npm

On the Raspberry PI:

    sudo apt-get install node npm libxml2-dev
    mkdir ~/tmp
    npm config set tmp ~/tmp
    git clone git@github.com:adafruit/OccEditor.git
    cd OccEditor
    export NODE_PATH="/usr/bin/node"
    npm install
    node server.js

Browser:

    http://127.0.0.1:3000

TODO
====
- Loads of stuff

SCREENSHOTS
===========
Coming soon