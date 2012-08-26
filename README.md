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

    sudo apt-get install node npm
    mkdir ~/tmp
    npm config set tmp ~/tmp
    git clone git@github.com:adafruit/OccEditor.git
    cd OccEditor
    npm install
    node server.js
    or
    sudo node server.js (to use I2C, etc)

Browser:

    http://127.0.0.1:3000

TODO
====
- Loads of stuff
- Clone or Create repositories
- Create "Projects"
- Create files
- Persistent sessions
- how to handle adding to known_hosts
- generate ssh key, and add to bitbucket
- git pull adafruit git repo?
- resizable left-nav?
- file tree for advanced users
- security...especially with running the ruby and python code
- security...chroot the node process
- simpler installation?


SCREENSHOTS
===========
Coming soon