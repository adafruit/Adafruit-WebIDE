OccEditor
================
This is a simple editor for learning to program the Raspberry PI.

Installation
============

On the Raspberry PI:

    sudo apt-get install node npm redis-server
    mkdir ~/tmp
    npm config set tmp ~/tmp
    git clone git@github.com:adafruit/OccEditor.git
    cd OccEditor
    npm install
    node server.js
    or
    sudo node server.js (to use I2C, etc)

Browser:

    http://raspberrypi.local:3000

TODO
====
- Loads of stuff
- rename folders/files
- delete folders/files
- how to handle adding to known_hosts
- generate ssh key, and add to bitbucket
- git pull adafruit git repo?
- resizable left-nav?
- file tree for advanced users?
- security...especially with running the ruby and python code
- security...chroot the node process
- simpler installation?


SCREENSHOTS
===========
Coming soon