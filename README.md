OccEditor
================
This is a simple editor for learning to program the Raspberry PI.

Installation
============

On the Raspberry PI:

    sudo apt-get install nodejs npm redis-server -y
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

TODO:
- Add ability to rename, and delete folders and files.
- Queue up git requests...
- Open project README when editor opened
- Remove settings COG in adafruit repository
- Change /repositories to /repositories/username
- Figure out bitbucket consumer key and secret on multi-user level
- Change active/inactive state of navigator-settings
- Auth for Terminal access
- Better Commit messages?
- Setup git config
    git config --global user.email "you@example.com"
    git config --global user.name "Your Name"
- Basic Command Line access, ability to submit commands. (discussions probably further needed)
- Add bitbucket to known_hosts at server startup
- Generate SSH Key, and use Bitbucket API to add it to account?
- Security (xss, chroot, etc)
- simpler installation

Nice to have:
- Resizable left-navigator
- File Tree for advanced users
- vim/emacs modes in editor
- API Integration
- Maintain state of editor open project and file
- History and back button
- Allow external links #Link and use icon-external-link


SCREENSHOTS
===========
Coming soon